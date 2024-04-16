//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef _M_AMD64
extern "C" void *amd64_CallWithFakeFrame(void *target,
                                         void *frame,
                                         size_t spillSize,
                                         size_t argsSize,
                                         void *arg0 = nullptr) throw(...);
#elif defined(_M_ARM)
extern "C" void *arm_CallEhFrame(void *target, void *framePtr, void *localsPtr, size_t argsSize) throw(...);
extern "C" void *arm_CallCatch(void *target, void *framePtr, void *localsPtr, size_t argsSize, void *catchObj) throw(...);
#elif defined(_M_ARM64)
extern "C" void *arm64_CallEhFrame(void *target, void *framePtr, void *localsPtr, size_t argsSize) throw(...);
extern "C" void *arm64_CallCatch(void *target, void *framePtr, void *localsPtr, size_t argsSize, void *catchObj) throw(...);
#endif

namespace Js
{
    class JavascriptExceptionContext;

    class JavascriptExceptionOperators  /* All static */
    {
    public:
        static const uint64 DefaultStackTraceLimit = 10;
        static const uint64 MaxStackTraceLimit = _UI64_MAX;

        // AutoCatchHandlerExists tracks where an exception will be caught and not propagated out.
        // It should be included wherever an exception is caught and swallowed.
        class AutoCatchHandlerExists
        {
          private:
            bool m_previousCatchHandlerExists;
            bool m_previousCatchHandlerToUserCodeStatus;
            ThreadContext* m_threadContext;
            void FetchNonUserCodeStatus(ScriptContext *scriptContext);

          public:
            AutoCatchHandlerExists(ScriptContext* scriptContext, bool isPromiseHandled = true);
            ~AutoCatchHandlerExists();
        };

        class TryHandlerAddrOfReturnAddrStack
        {
          private:
            void * m_prevTryHandlerAddrOfReturnAddr;
            ThreadContext* m_threadContext;

          public:
            TryHandlerAddrOfReturnAddrStack(ScriptContext* scriptContext, void *addrOfReturnAddr);
            ~TryHandlerAddrOfReturnAddrStack();
        };

        class HasBailedOutPtrStack
        {
        private:
            bool * m_prevHasBailedOutPtr;
            ThreadContext* m_threadContext;

        public:
            HasBailedOutPtrStack(ScriptContext* scriptContext, bool *hasBailedOutPtr);
            ~HasBailedOutPtrStack();
        };

        class PendingFinallyExceptionStack
        {
        private:
            ThreadContext* m_threadContext;

        public:
            PendingFinallyExceptionStack(ScriptContext* scriptContext, Js::JavascriptExceptionObject *exceptionObj);
            ~PendingFinallyExceptionStack();
        };

        static void DECLSPEC_NORETURN OP_Throw(Var object, ScriptContext* scriptContext);
        static void DECLSPEC_NORETURN Throw(Var object, ScriptContext* scriptContext);
        static void DECLSPEC_NORETURN ThrowExceptionObject(Js::JavascriptExceptionObject* exceptionObject, ScriptContext* scriptContext, bool considerPassingToDebugger = false, PVOID returnAddress = NULL, bool resetStack = false);
        static void DECLSPEC_NORETURN RethrowExceptionObject(Js::JavascriptExceptionObject* exceptionObject, ScriptContext* scriptContext, bool considerPassingToDebugger = false);

        static void DECLSPEC_NORETURN DoThrow(JavascriptExceptionObject* exceptionObject, ScriptContext* scriptContext);
        static void DECLSPEC_NORETURN DoThrowCheckClone(JavascriptExceptionObject* exceptionObject, ScriptContext* scriptContext);

#ifdef _M_X64
        static void *OP_TryCatch(void *try_, void *catch_, void *frame, size_t spillSize, size_t argsSize, int hasBailedOutOffset, ScriptContext *scriptContext);
        static void *OP_TryFinally(void *try_, void *finally_, void *frame, size_t spillSize, size_t argsSize, int hasBailedOutOffset, ScriptContext *scriptContext);
        static void *OP_TryFinallyNoOpt(void *try_, void *finally_, void *frame, size_t spillSize, size_t argsSize, ScriptContext *scriptContext);
#elif defined(_M_ARM32_OR_ARM64)
        static void* OP_TryCatch(void* continuationAddr, void* handlerAddr, void* framePtr, void *localsPtr, size_t argsSize, int hasBailedOutOffset, ScriptContext* scriptContext);
        static void* OP_TryFinally(void* continuationAddr, void* handlerAddr, void* framePtr, void *localsPtr, size_t argsSize, int hasBailedOutOffset, ScriptContext* scriptContext);
        static void* OP_TryFinallyNoOpt(void* continuationAddr, void* handlerAddr, void* framePtr, void *localsPtr, size_t argsSize, ScriptContext* scriptContext);
#else
        static void* OP_TryCatch(void* continuationAddr, void* handlerAddr, void* framePtr, int hasBailedOutOffset, ScriptContext* scriptContext);
        static void* OP_TryFinally(void* continuationAddr, void* handlerAddr, void* framePtr, int hasBailedOutOffset, ScriptContext* scriptContext);
        static void* OP_TryFinallyNoOpt(void* continuationAddr, void* handlerAddr, void* framePtr, ScriptContext* scriptContext);
#endif
#if defined(DBG) && defined(_M_IX86)
        static void DbgCheckEHChain();
#endif
        static JavascriptExceptionObject* GetOutOfMemoryExceptionObject(ScriptContext* scriptContext);

        static Var OP_RuntimeTypeError(MessageId messageId, ScriptContext* scriptContext);
        static Var OP_RuntimeRangeError(MessageId messageId, ScriptContext* scriptContext);
        static Var OP_RuntimeReferenceError(MessageId messageId, ScriptContext* scriptContext);
        static Var OP_WebAssemblyRuntimeError(MessageId messageId, ScriptContext* scriptContext);
        static void DECLSPEC_NORETURN ThrowOutOfMemory(ScriptContext* scriptContext);
        static void DECLSPEC_NORETURN ThrowStackOverflow(ScriptContext* scriptContext, PVOID returnAddress);

        static uint64 GetStackTraceLimit(Var thrownObject, ScriptContext* scriptContext);
        static Var ThrowTypeErrorRestrictedPropertyAccessor(RecyclableObject* function, CallInfo callInfo, ...);
        static Var StackTraceAccessor(RecyclableObject* function, CallInfo callInfo, ...);
        static void WalkStackForExceptionContext(ScriptContext& scriptContext, JavascriptExceptionContext& exceptionContext, Var thrownObject, uint64 stackCrawlLimit, PVOID returnAddress, bool isThrownException = true, bool resetSatck = false);
#if ENABLE_NATIVE_CODEGEN
        static void WalkStackForCleaningUpInlineeInfo(ScriptContext *scriptContext, PVOID returnAddress, PVOID tryCatchFrameAddr);
#endif
        static void AddStackTraceToObject(Var obj, JavascriptExceptionContext::StackTrace* stackTrace, ScriptContext& scriptContext, bool isThrownException = true, bool resetSatck = false);
        static uint64 StackCrawlLimitOnThrow(Var thrownObject, ScriptContext& scriptContext);

        class EntryInfo
        {
        public:
            static FunctionInfo StackTraceAccessor;

            // For strict mode
            static FunctionInfo ThrowTypeErrorRestrictedPropertyAccessor;
        };

      private:
        static JavascriptFunction * WalkStackForExceptionContextInternal(ScriptContext& scriptContext, JavascriptExceptionContext& exceptionContext, Var thrownObject, uint32& callerByteCodeOffset,
            uint64 stackCrawlLimit, PVOID returnAddress, bool isThrownException, bool resetStack = false);

        static void ThrowExceptionObjectInternal(Js::JavascriptExceptionObject * exceptionObject, ScriptContext* scriptContext, bool fillExceptionContext, bool considerPassingToDebugger, PVOID returnAddress, bool resetStack);
        static BOOL GetCaller(JavascriptStackWalker& walker, _Out_opt_ JavascriptFunction*& jsFunc);
        static void DumpStackTrace(JavascriptExceptionContext& exceptionContext, bool isThrownException = true);
        static JavascriptExceptionContext::StackTrace* TrimStackTraceForThrownObject(JavascriptExceptionContext::StackTrace* stackTraceOriginal, Var thrownObject, ScriptContext& scriptContext);
        static void AppendExternalFrameToStackTrace(CompoundString* bs, LPCWSTR functionName, LPCWSTR fileName, ULONG lineNumber, LONG characterPosition);
        static void AppendLibraryFrameToStackTrace(CompoundString* bs, LPCWSTR functionName);
        static bool IsErrorInstance(Var thrownObject);

        static bool CrawlStackForWER(Js::ScriptContext& scriptContext);
        static void DispatchExceptionToDebugger(Js::JavascriptExceptionObject * exceptionObject, ScriptContext* scriptContext);
    };

} // namespace Js
