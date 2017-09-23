//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    const DWORD  ExceptionParameters = 1;
    const int    ExceptionObjectIndex = 0;

    class JavascriptExceptionContext;

    class JavascriptExceptionObject
    {
    public:
        typedef Var (__stdcall *HostWrapperCreateFuncType)(Var var, ScriptContext * sourceScriptContext, ScriptContext * destScriptContext);

        JavascriptExceptionObject(Var object, ScriptContext * scriptContext, JavascriptExceptionContext* exceptionContextIn, bool isPendingExceptionObject = false) :
            thrownObject(object), isPendingExceptionObject(isPendingExceptionObject),
            scriptContext(scriptContext), tag(true), 
#ifdef ENABLE_SCRIPT_DEBUGGING
            isDebuggerSkip(false), byteCodeOffsetAfterDebuggerSkip(Constants::InvalidByteCodeOffset), hasDebuggerLogged(false),
            isFirstChance(false), isExceptionCaughtInNonUserCode(false), ignoreAdvanceToNextStatement(false),
#endif
            hostWrapperCreateFunc(nullptr), isGeneratorReturnException(false),
            next(nullptr)
        {
            if (exceptionContextIn)
            {
                exceptionContext = *exceptionContextIn;
            }
            else
            {
                memset(&exceptionContext, 0, sizeof(exceptionContext));
            }
#if ENABLE_DEBUG_STACK_BACK_TRACE
            this->stackBackTrace = nullptr;
#endif
        }

        Var GetThrownObject(ScriptContext * requestingScriptContext);

        // ScriptContext can be NULL in case of OOM exception.
        ScriptContext * GetScriptContext() const
        {
            return scriptContext;
        }

        FunctionBody * GetFunctionBody() const;
        JavascriptFunction* GetFunction() const
        {
            return exceptionContext.ThrowingFunction();
        }

        const JavascriptExceptionContext* GetExceptionContext() const
        {
            return &exceptionContext;
        }
#if ENABLE_DEBUG_STACK_BACK_TRACE
        void FillStackBackTrace();
#endif

        void FillError(JavascriptExceptionContext& exceptionContext, ScriptContext *scriptContext, HostWrapperCreateFuncType hostWrapperCreateFunc = NULL);
        void ClearError();

#ifdef ENABLE_SCRIPT_DEBUGGING
        void SetDebuggerSkip(bool skip)
        {
            isDebuggerSkip = skip;
        }

        bool IsDebuggerSkip()
        {
            return isDebuggerSkip;
        }

        int GetByteCodeOffsetAfterDebuggerSkip()
        {
            return byteCodeOffsetAfterDebuggerSkip;
        }

        void SetByteCodeOffsetAfterDebuggerSkip(int offset)
        {
            byteCodeOffsetAfterDebuggerSkip = offset;
        }

        void SetDebuggerHasLogged(bool has)
        {
            hasDebuggerLogged = has;
        }

        bool HasDebuggerLogged()
        {
            return hasDebuggerLogged;
        }

        void SetIsFirstChance(bool is)
        {
            isFirstChance = is;
        }

        bool IsFirstChanceException()
        {
            return isFirstChance;
        }

        void SetIsExceptionCaughtInNonUserCode(bool is)
        {
            isExceptionCaughtInNonUserCode = is;
        }

        bool IsExceptionCaughtInNonUserCode()
        {
            return isExceptionCaughtInNonUserCode;
        }

        void SetIgnoreAdvanceToNextStatement(bool is)
        {
            ignoreAdvanceToNextStatement = is;
        }

        bool IsIgnoreAdvanceToNextStatement()
        {
            return ignoreAdvanceToNextStatement;
        }
#endif

        void SetHostWrapperCreateFunc(HostWrapperCreateFuncType hostWrapperCreateFunc)
        {
            this->hostWrapperCreateFunc = hostWrapperCreateFunc;
        }

        uint32 GetByteCodeOffset()
        {
            return exceptionContext.ThrowingFunctionByteCodeOffset();
        }

        void ReplaceThrownObject(Var object)
        {
            AssertMsg(RecyclableObject::Is(object), "Why are we replacing a non recyclable thrown object?");
            AssertMsg(this->GetScriptContext() != RecyclableObject::FromVar(object)->GetScriptContext(), "If replaced thrownObject is from same context what's the need to replace?");
            this->thrownObject = object;
        }

        void SetThrownObject(Var object)
        {
            // Only pending exception object and generator return exception use this API.
            Assert(this->isPendingExceptionObject || this->isGeneratorReturnException);
            this->thrownObject = object;
        }
        JavascriptExceptionObject* CloneIfStaticExceptionObject(ScriptContext* scriptContext);

        void ClearStackTrace()
        {
            exceptionContext.SetStackTrace(NULL);
        }

        bool IsPendingExceptionObject() const { return isPendingExceptionObject; }


        void SetGeneratorReturnException(bool is)
        {
            isGeneratorReturnException = is;
        }

        bool IsGeneratorReturnException()
        {
            // Used by the generators to throw an exception to indicate the return from generator function
            return isGeneratorReturnException;
        }

    private:
        friend class ::ThreadContext;
        static void Insert(Field(JavascriptExceptionObject*)* head, JavascriptExceptionObject* item);
        static void Remove(Field(JavascriptExceptionObject*)* head, JavascriptExceptionObject* item);

    private:
        Field(Var)      thrownObject;
        Field(ScriptContext *) scriptContext;
        
#ifdef ENABLE_SCRIPT_DEBUGGING
        Field(int)        byteCodeOffsetAfterDebuggerSkip;
#endif

        Field(const bool) tag : 1;               // Tag the low bit to prevent possible GC false references
        Field(bool)       isPendingExceptionObject : 1;
        Field(bool)       isGeneratorReturnException : 1;

#ifdef ENABLE_SCRIPT_DEBUGGING
        Field(bool)       isDebuggerSkip : 1;
        Field(bool)       hasDebuggerLogged : 1;
        Field(bool)       isFirstChance : 1;      // Mentions whether the current exception is a handled exception or not
        Field(bool)       isExceptionCaughtInNonUserCode : 1; // Mentions if in the caller chain the exception will be handled by the non-user code.
        Field(bool)       ignoreAdvanceToNextStatement : 1;  // This will be set when user had setnext while sitting on the exception
                                                // So the exception eating logic shouldn't try and advance to next statement again.
#endif

        FieldNoBarrier(HostWrapperCreateFuncType) hostWrapperCreateFunc;

        Field(JavascriptExceptionContext) exceptionContext;
#if ENABLE_DEBUG_STACK_BACK_TRACE
        Field(StackBackTrace*) stackBackTrace;
        static const int StackToSkip = 2;
        static const int StackTraceDepth = 30;
#endif

        Field(JavascriptExceptionObject*) next;  // to temporarily store list of throwing exceptions

        PREVENT_COPY(JavascriptExceptionObject)
    };

    class GeneratorReturnExceptionObject : public JavascriptExceptionObject
    {
    public:
        GeneratorReturnExceptionObject(Var object, ScriptContext * scriptContext)
            : JavascriptExceptionObject(object, scriptContext, nullptr)
        {
#ifdef ENABLE_SCRIPT_DEBUGGING
            this->SetDebuggerSkip(true);
            this->SetIgnoreAdvanceToNextStatement(true);
#endif
            this->SetGeneratorReturnException(true);
        }
    };
}
