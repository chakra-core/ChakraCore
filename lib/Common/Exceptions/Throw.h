//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef STACK_BACK_TRACE
class StackBackTrace;
#endif

namespace Js {

    class ScriptContext;

    class Throw
    {
    public:
        static void __declspec(noreturn) OutOfMemory();
        static void __declspec(noreturn) StackOverflow(ScriptContext *scriptContext, PVOID returnAddress);
        static void __declspec(noreturn) NotImplemented();
        static void __declspec(noreturn) InternalError();
        static void __declspec(noreturn) FatalInternalError();
        static void __declspec(noreturn) FatalInternalErrorEx(int scenario);
        static void __declspec(noreturn) FatalProjectionError();
#if ENABLE_JS_REENTRANCY_CHECK
        static void __declspec(noreturn) FatalJsReentrancyError();
#endif

        static void CheckAndThrowOutOfMemory(BOOLEAN status);

        static bool ReportAssert(__in LPCSTR fileName, uint lineNumber, __in LPCSTR error, __in LPCSTR message);
        static void LogAssert();
#ifdef GENERATE_DUMP
        static int GenerateDump(PEXCEPTION_POINTERS exceptInfo, LPCWSTR filePath, int ret = EXCEPTION_CONTINUE_SEARCH, bool needLock = false);
        static void GenerateDump(LPCWSTR filePath, bool terminate = false, bool needLock = false);
        static void GenerateDumpForAssert(LPCWSTR filePath);
    private:
        static CriticalSection csGenerateDump;
#ifdef STACK_BACK_TRACE
        THREAD_LOCAL static  StackBackTrace * stackBackTrace;

        static const int StackToSkip = 2;
        static const int StackTraceDepth = 40;
#endif
#endif
    };

    // Info:        Verify the result or throw catastrophic
    // Parameters:  HRESULT
    inline void VerifyOkCatastrophic(__in HRESULT hr)
    {
        if (hr == E_OUTOFMEMORY)
        {
            Js::Throw::OutOfMemory();
        }
        else if (FAILED(hr))
        {
            Js::Throw::FatalProjectionError();
        }
    }

    // Info:        Verify the result or throw catastrophic
    // Parameters:  bool
    template<typename TCheck>
    inline void VerifyCatastrophic(__in TCheck result)
    {
        if (!result)
        {
            Assert(false);
            Js::Throw::FatalProjectionError();
        }
    }


} // namespace Js

#define BEGIN_TRANSLATE_TO_HRESULT(type) \
{\
    try \
    { \
        AUTO_HANDLED_EXCEPTION_TYPE(type);

#define BEGIN_TRANSLATE_TO_HRESULT_NESTED(type) \
{\
    try \
    { \
        AUTO_NESTED_HANDLED_EXCEPTION_TYPE(type);

#define BEGIN_TRANSLATE_OOM_TO_HRESULT BEGIN_TRANSLATE_TO_HRESULT(ExceptionType_OutOfMemory)
#define BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED BEGIN_TRANSLATE_TO_HRESULT_NESTED(ExceptionType_OutOfMemory)

#define END_TRANSLATE_OOM_TO_HRESULT(hr) \
    } \
    catch (Js::OutOfMemoryException) \
    {   \
        hr = E_OUTOFMEMORY; \
    }\
}

#define END_TRANSLATE_OOM_TO_HRESULT_AND_EXCEPTION_OBJECT(hr, scriptContext, exceptionObject) \
    } \
    catch(Js::OutOfMemoryException) \
    {   \
        hr = E_OUTOFMEMORY; \
        *exceptionObject = Js::JavascriptExceptionOperators::GetOutOfMemoryExceptionObject(scriptContext); \
    } \
    }

#define BEGIN_TRANSLATE_EXCEPTION_TO_HRESULT BEGIN_TRANSLATE_TO_HRESULT((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow))
#define BEGIN_TRANSLATE_EXCEPTION_TO_HRESULT_NESTED BEGIN_TRANSLATE_TO_HRESULT_NESTED((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow))
#define BEGIN_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT BEGIN_TRANSLATE_TO_HRESULT((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow | ExceptionType_JavascriptException))
#define BEGIN_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT_NESTED BEGIN_TRANSLATE_TO_HRESULT_NESTED((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow | ExceptionType_JavascriptException))

#define END_TRANSLATE_KNOWN_EXCEPTION_TO_HRESULT(hr) \
    } \
    catch (Js::OutOfMemoryException) \
    {   \
        hr = E_OUTOFMEMORY; \
    }   \
    catch (Js::StackOverflowException) \
    { \
        hr = VBSERR_OutOfStack; \
    } \
    catch (Js::NotImplementedException)  \
    {   \
        hr = E_NOTIMPL; \
    } \
    catch (Js::ScriptAbortException)  \
    {   \
        hr = E_ABORT; \
    }  \
    catch (Js::AsmJsParseException)  \
    {   \
        hr = JSERR_AsmJsCompileError; \
    }

// This should be the inverse of END_TRANSLATE_KNOWN_EXCEPTION_TO_HRESULT, and catch all the same cases.
#define THROW_KNOWN_HRESULT_EXCEPTIONS(hr, scriptContext) \
    if (hr == E_OUTOFMEMORY) \
    { \
        JavascriptError::ThrowOutOfMemoryError(scriptContext); \
    } \
    else if (hr == VBSERR_OutOfStack) \
    { \
        JavascriptError::ThrowStackOverflowError(scriptContext); \
    } \
    else if (hr == E_NOTIMPL) \
    { \
        throw Js::NotImplementedException(); \
    } \
    else if (hr == E_ABORT) \
    { \
        throw Js::ScriptAbortException(); \
    } \
    else if (hr == JSERR_AsmJsCompileError) \
    { \
        throw Js::AsmJsParseException(); \
    } \
    else if (FAILED(hr)) \
    { \
        /* Intended to be the inverse of E_FAIL in CATCH_UNHANDLED_EXCEPTION */ \
        AssertOrFailFast(false); \
    }

#define CATCH_UNHANDLED_EXCEPTION(hr) \
    catch (...) \
    { \
        AssertOrFailFastMsg(FALSE, "invalid exception thrown and didn't get handled"); \
        hr = E_FAIL; /* Suppress C4701 */ \
    } \
    }


#define END_TRANSLATE_EXCEPTION_TO_HRESULT(hr) \
    END_TRANSLATE_KNOWN_EXCEPTION_TO_HRESULT(hr)\
    CATCH_UNHANDLED_EXCEPTION(hr)

#define END_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT(hr) \
    Assert(!JsUtil::ExternalApi::IsScriptActiveOnCurrentThreadContext()); \
    END_TRANSLATE_KNOWN_EXCEPTION_TO_HRESULT(hr) \
    END_TRANSLATE_ERROROBJECT_TO_HRESULT(hr) \
    CATCH_UNHANDLED_EXCEPTION(hr)

// Use this version if execution is in script (use rarely)
#define END_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT_INSCRIPT(hr) \
    Assert(JsUtil::ExternalApi::IsScriptActiveOnCurrentThreadContext()); \
    END_TRANSLATE_KNOWN_EXCEPTION_TO_HRESULT(hr) \
    END_TRANSLATE_ERROROBJECT_TO_HRESULT_INSCRIPT(hr) \
    CATCH_UNHANDLED_EXCEPTION(hr)

#define END_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT_NOASSERT(hr) \
    END_TRANSLATE_KNOWN_EXCEPTION_TO_HRESULT(hr) \
    END_TRANSLATE_ERROROBJECT_TO_HRESULT(hr) \
    CATCH_UNHANDLED_EXCEPTION(hr)

#define END_TRANSLATE_ERROROBJECT_TO_HRESULT_EX(hr, GetRuntimeErrorFunc) \
    catch(const Js::JavascriptException& err)  \
    {   \
        Js::JavascriptExceptionObject* exceptionObject = err.GetAndClear(); \
        GET_RUNTIME_ERROR_IMPL(hr, GetRuntimeErrorFunc, exceptionObject); \
    }

#define GET_RUNTIME_ERROR_IMPL(hr, GetRuntimeErrorFunc, exceptionObject) \
    {   \
        Js::Var errorObject = exceptionObject->GetThrownObject(nullptr);   \
        if (errorObject != nullptr && (Js::JavascriptError::Is(errorObject) ||  \
            Js::JavascriptError::IsRemoteError(errorObject)))   \
        {       \
            hr = GetRuntimeErrorFunc(Js::RecyclableObject::FromVar(errorObject), nullptr);   \
        }   \
        else if (errorObject != nullptr) \
        {  \
            hr = JSERR_UncaughtException; \
        }  \
        else \
        {  \
            AssertMsg(errorObject == nullptr, "errorObject should be NULL");  \
            hr = E_OUTOFMEMORY;  \
        }  \
    }

#define GET_RUNTIME_ERROR(hr, exceptionObject) \
    GET_RUNTIME_ERROR_IMPL(hr, Js::JavascriptError::GetRuntimeErrorWithScriptEnter, exceptionObject)

#define END_TRANSLATE_ERROROBJECT_TO_HRESULT(hr) \
    END_TRANSLATE_ERROROBJECT_TO_HRESULT_EX(hr, Js::JavascriptError::GetRuntimeErrorWithScriptEnter)

#define END_GET_ERROROBJECT(hr, scriptContext, exceptionObject) \
    catch (const Js::JavascriptException& err)  \
    {   \
        Js::JavascriptExceptionObject *  _exceptionObject = err.GetAndClear(); \
        BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED \
            exceptionObject = _exceptionObject; \
            exceptionObject = exceptionObject->CloneIfStaticExceptionObject(scriptContext);  \
        END_TRANSLATE_OOM_TO_HRESULT(hr) \
    }

#define CATCH_STATIC_JAVASCRIPT_EXCEPTION_OBJECT(errCode) \
    catch (Js::OutOfMemoryException)  \
    {  \
        errCode = JsErrorOutOfMemory;  \
    } catch (Js::StackOverflowException)  \
    {  \
        errCode = JsErrorOutOfMemory;  \
    }  \

#if ENABLE_TTD
#define CATCH_OTHER_EXCEPTIONS(errCode)  \
    catch (JsrtExceptionBase& e)  \
    {  \
        errCode = e.GetJsErrorCode();  \
    }   \
    catch (Js::ExceptionBase)   \
    {   \
        AssertMsg(false, "Unexpected engine exception.");   \
        errCode = JsErrorFatal;    \
    }   \
    catch (TTD::TTDebuggerAbortException)   \
    {   \
        throw; /*don't set errcode we treat this as non-termination of the code that was executing*/  \
    }   \
    catch (...) \
    {   \
        AssertMsg(false, "Unexpected non-engine exception.");   \
        errCode = JsErrorFatal;    \
    }
#else
#define CATCH_OTHER_EXCEPTIONS(errCode)  \
    catch (JsrtExceptionBase& e)  \
    {  \
        errCode = e.GetJsErrorCode();  \
    }   \
    catch (Js::ExceptionBase)   \
    {   \
        AssertMsg(false, "Unexpected engine exception.");   \
        errCode = JsErrorFatal;    \
    }   \
    catch (...) \
    {   \
        AssertMsg(false, "Unexpected non-engine exception.");   \
        errCode = JsErrorFatal;    \
    }
#endif

// Use this version if execution is in script (use rarely)
#define END_TRANSLATE_ERROROBJECT_TO_HRESULT_INSCRIPT(hr) \
    END_TRANSLATE_ERROROBJECT_TO_HRESULT_EX(hr, Js::JavascriptError::GetRuntimeError)

#define TRANSLATE_EXCEPTION_TO_HRESULT_ENTRY(ex) \
    } \
    catch (ex) \
    {

#define DEBUGGER_ATTACHDETACH_FATAL_ERROR_IF_FAILED(hr) if (hr != S_OK) Debugger_AttachDetach_fatal_error(hr);
