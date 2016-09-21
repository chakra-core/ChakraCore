//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "JsrtExceptionBase.h"
#include "Exceptions/EvalDisabledException.h"

#define PARAM_NOT_NULL(p) \
    if (p == nullptr) \
    { \
        return JsErrorNullArgument; \
    }

#define VALIDATE_JSREF(p) \
    if (p == JS_INVALID_REFERENCE) \
    { \
        return JsErrorInvalidArgument; \
    } \

#define MARSHAL_OBJECT(p, scriptContext) \
        Js::RecyclableObject* __obj = Js::RecyclableObject::FromVar(p); \
        if (__obj->GetScriptContext() != scriptContext) \
        { \
            if(__obj->GetScriptContext()->GetThreadContext() != scriptContext->GetThreadContext()) \
            {  \
                return JsErrorWrongRuntime;  \
            }  \
            p = Js::CrossSite::MarshalVar(scriptContext, __obj); \
        }

#define VALIDATE_INCOMING_RUNTIME_HANDLE(p) \
        { \
            if (p == JS_INVALID_RUNTIME_HANDLE) \
            { \
                return JsErrorInvalidArgument; \
            } \
        }

#define VALIDATE_INCOMING_PROPERTYID(p) \
          { \
            if (p == JS_INVALID_REFERENCE || \
                    Js::IsInternalPropertyId(((Js::PropertyRecord *)p)->GetPropertyId())) \
            { \
                return JsErrorInvalidArgument; \
            } \
        }

#define VALIDATE_INCOMING_REFERENCE(p, scriptContext) \
        { \
            VALIDATE_JSREF(p); \
            if (Js::RecyclableObject::Is(p)) \
            { \
                MARSHAL_OBJECT(p, scriptContext)   \
            } \
        }

#define VALIDATE_INCOMING_OBJECT(p, scriptContext) \
        { \
            VALIDATE_JSREF(p); \
            if (!Js::JavascriptOperators::IsObject(p)) \
            { \
                return JsErrorArgumentNotObject; \
            } \
            MARSHAL_OBJECT(p, scriptContext)   \
        }

#define VALIDATE_INCOMING_OBJECT_OR_NULL(p, scriptContext) \
        { \
            VALIDATE_JSREF(p); \
            if (!Js::JavascriptOperators::IsObjectOrNull(p)) \
            { \
                return JsErrorArgumentNotObject; \
            } \
            MARSHAL_OBJECT(p, scriptContext)   \
        }

#define VALIDATE_INCOMING_FUNCTION(p, scriptContext) \
        { \
            VALIDATE_JSREF(p); \
            if (!Js::JavascriptFunction::Is(p)) \
            { \
                return JsErrorInvalidArgument; \
            } \
            MARSHAL_OBJECT(p, scriptContext)   \
        }

template <class Fn>
JsErrorCode GlobalAPIWrapper(Fn fn)
{
    JsErrorCode errCode = JsNoError;
    try
    {
        // For now, treat this like an out of memory; consider if we should do something else here.

        AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

        errCode = fn();

        // These are error codes that should only be produced by the wrappers and should never
        // be produced by the internal calls.
        Assert(errCode != JsErrorFatal &&
            errCode != JsErrorNoCurrentContext &&
            errCode != JsErrorInExceptionState &&
            errCode != JsErrorInDisabledState &&
            errCode != JsErrorOutOfMemory &&
            errCode != JsErrorScriptException &&
            errCode != JsErrorScriptTerminated);
    }
    CATCH_STATIC_JAVASCRIPT_EXCEPTION_OBJECT

    CATCH_OTHER_EXCEPTIONS

    return errCode;
}

JsErrorCode CheckContext(JsrtContext *currentContext, bool verifyRuntimeState, bool allowInObjectBeforeCollectCallback = false);

template <bool verifyRuntimeState, class Fn>
JsErrorCode ContextAPIWrapper(Fn fn)
{
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode errCode = CheckContext(currentContext, verifyRuntimeState);

    if (errCode != JsNoError)
    {
        return errCode;
    }

    Js::ScriptContext *scriptContext = currentContext->GetScriptContext();
    try
    {
        AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_JavascriptException));

        // Enter script
        BEGIN_ENTER_SCRIPT(scriptContext, true, true, true)
        {
            errCode = fn(scriptContext);
        }
        END_ENTER_SCRIPT

            // These are error codes that should only be produced by the wrappers and should never
            // be produced by the internal calls.
            Assert(errCode != JsErrorFatal &&
            errCode != JsErrorNoCurrentContext &&
            errCode != JsErrorInExceptionState &&
            errCode != JsErrorInDisabledState &&
            errCode != JsErrorOutOfMemory &&
            errCode != JsErrorScriptException &&
            errCode != JsErrorScriptTerminated);
    }
    catch (Js::OutOfMemoryException)
    {
      return JsErrorOutOfMemory;
    }
    catch (Js::JavascriptExceptionObject *  exceptionObject)
    {
        scriptContext->GetThreadContext()->SetRecordedException(exceptionObject);
        return JsErrorScriptException;
    }
    catch (Js::ScriptAbortException)
    {
        Assert(scriptContext->GetThreadContext()->GetRecordedException() == nullptr);
        scriptContext->GetThreadContext()->SetRecordedException(scriptContext->GetThreadContext()->GetPendingTerminatedErrorObject());
        return JsErrorScriptTerminated;
    }
    catch (Js::EvalDisabledException)
    {
        return JsErrorScriptEvalDisabled;
    }

    CATCH_OTHER_EXCEPTIONS

    return errCode;
}

// allowInObjectBeforeCollectCallback only when current API is guaranteed not to do recycler allocation.
template <class Fn>
JsErrorCode ContextAPINoScriptWrapper(Fn fn, bool allowInObjectBeforeCollectCallback = false, bool scriptExceptionAllowed = false)
{
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode errCode = CheckContext(currentContext, /*verifyRuntimeState*/true, allowInObjectBeforeCollectCallback);

    if (errCode != JsNoError)
    {
        return errCode;
    }

    Js::ScriptContext *scriptContext = currentContext->GetScriptContext();

    try
    {
        // For now, treat this like an out of memory; consider if we should do something else here.

        AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

        errCode = fn(scriptContext);

        // These are error codes that should only be produced by the wrappers and should never
        // be produced by the internal calls.
        Assert(errCode != JsErrorFatal &&
            errCode != JsErrorNoCurrentContext &&
            errCode != JsErrorInExceptionState &&
            errCode != JsErrorInDisabledState &&
            errCode != JsErrorOutOfMemory &&
            (scriptExceptionAllowed || errCode != JsErrorScriptException) &&
            errCode != JsErrorScriptTerminated);
    }
    CATCH_STATIC_JAVASCRIPT_EXCEPTION_OBJECT

    catch (Js::JavascriptExceptionObject *  exceptionObject)
    {
        AssertMsg(false, "Should never get JavascriptExceptionObject for ContextAPINoScriptWrapper.");
        scriptContext->GetThreadContext()->SetRecordedException(exceptionObject);
        return JsErrorScriptException;
    }

    catch (Js::ScriptAbortException)
    {
        Assert(scriptContext->GetThreadContext()->GetRecordedException() == nullptr);
        scriptContext->GetThreadContext()->SetRecordedException(scriptContext->GetThreadContext()->GetPendingTerminatedErrorObject());
        return JsErrorScriptTerminated;
    }

    CATCH_OTHER_EXCEPTIONS

    return errCode;
}

template <class Fn>
JsErrorCode SetContextAPIWrapper(JsrtContext* newContext, Fn fn)
{
    JsrtContext* oldContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = newContext->GetScriptContext();

    JsErrorCode errorCode = JsNoError;
    try
    {
        // For now, treat this like an out of memory; consider if we should do something else here.

        AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow | ExceptionType_JavascriptException));
        if (JsrtContext::TrySetCurrent(newContext))
        {
            // Enter script
            BEGIN_ENTER_SCRIPT(scriptContext, true, true, true)
            {
                errorCode = fn(scriptContext);
            }
            END_ENTER_SCRIPT
        }
        else
        {
            errorCode = JsErrorWrongThread;
        }

        // These are error codes that should only be produced by the wrappers and should never
        // be produced by the internal calls.
        Assert(errorCode != JsErrorFatal &&
            errorCode != JsErrorNoCurrentContext &&
            errorCode != JsErrorInExceptionState &&
            errorCode != JsErrorInDisabledState &&
            errorCode != JsErrorOutOfMemory &&
            errorCode != JsErrorScriptException &&
            errorCode != JsErrorScriptTerminated);
    }
    catch (Js::OutOfMemoryException)
    {
        errorCode = JsErrorOutOfMemory;
    }
    catch (Js::JavascriptExceptionObject *  exceptionObject)
    {
        scriptContext->GetThreadContext()->SetRecordedException(exceptionObject);
        errorCode = JsErrorScriptException;
    }
    catch (Js::ScriptAbortException)
    {
        Assert(scriptContext->GetThreadContext()->GetRecordedException() == nullptr);
        scriptContext->GetThreadContext()->SetRecordedException(scriptContext->GetThreadContext()->GetPendingTerminatedErrorObject());
        errorCode = JsErrorScriptTerminated;
    }
    catch (Js::EvalDisabledException)
    {
        errorCode = JsErrorScriptEvalDisabled;
    }
    catch (Js::StackOverflowException)
    {
        return JsErrorOutOfMemory;
    }
    CATCH_OTHER_EXCEPTIONS
    JsrtContext::TrySetCurrent(oldContext);
    return errorCode;
}

void HandleScriptCompileError(Js::ScriptContext * scriptContext, CompileScriptException * se);

#if DBG
#define _PREPARE_RETURN_NO_EXCEPTION __debugCheckNoException.hasException = false;
#else
#define _PREPARE_RETURN_NO_EXCEPTION
#endif

#define BEGIN_JSRT_NO_EXCEPTION  BEGIN_NO_EXCEPTION
#define END_JSRT_NO_EXCEPTION    END_NO_EXCEPTION return JsNoError;
#define RETURN_NO_EXCEPTION(x)   _PREPARE_RETURN_NO_EXCEPTION return x

////
//Define compact TTD macros for use in the JSRT API's
#if ENABLE_TTD
#define PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX) (CTX)->ShouldPerformRecordAction()

#define PERFORM_JSRT_TTD_RECORD_ACTION(WRAPPER_TAG, CTX, ACTION_CODE, ...) TTD::TTDJsRTActionResultAutoRecorder _actionEntryPopper(WRAPPER_TAG); \
    if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) \
    { \
        (CTX)->GetThreadContext()->TTDLog->##ACTION_CODE##(_actionEntryPopper, ##__VA_ARGS__); \
    }

#define PERFORM_JSRT_TTD_RECORD_ACTION_STD_GLOBALWRAPPER(ACTION_CODE, ...) PERFORM_JSRT_TTD_RECORD_ACTION(TTD::ContextWrapperEnterExitStatus::EnterGlobalAPIWrapper, scriptContext, ACTION_CODE, ##__VA_ARGS__)
#define PERFORM_JSRT_TTD_RECORD_ACTION_STD_CONTEXTWRAPPER(ACTION_CODE, ...) PERFORM_JSRT_TTD_RECORD_ACTION(TTD::ContextWrapperEnterExitStatus::EnterContextAPIWrapper, scriptContext, ACTION_CODE, ##__VA_ARGS__)
#define PERFORM_JSRT_TTD_RECORD_ACTION_STD_NOSCRIPTWRAPPER(ACTION_CODE, ...) PERFORM_JSRT_TTD_RECORD_ACTION(TTD::ContextWrapperEnterExitStatus::EnterContextAPINoScriptWrapper, scriptContext, ACTION_CODE, ##__VA_ARGS__)

#define PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(RESULT) if(_actionEntryPopper.IsSetForRecord()) \
        { \
            _actionEntryPopper.NormalCompletionWResult(RESULT); \
        }

#define PERFORM_JSRT_TTD_RECORD_ACTION_COMPLETE_NO_RESULT() if(_actionEntryPopper.IsSetForRecord()) \
        { \
            _actionEntryPopper.NormalCompletion(); \
        }

//TODO: find and replace all of the occourences of this in jsrt.cpp
#define PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(CTX) if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) { AssertMsg(false, "Need to implement support here!!!"); }
#else
#define PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX) false

#define PERFORM_JSRT_TTD_RECORD_ACTION(WRAPPER_TAG, CTX, ACTION_CODE, ...) 

#define PERFORM_JSRT_TTD_RECORD_ACTION_STD_GLOBALWRAPPER(ACTION_CODE, ...) 
#define PERFORM_JSRT_TTD_RECORD_ACTION_STD_CONTEXTWRAPPER(ACTION_CODE, ...) 
#define PERFORM_JSRT_TTD_RECORD_ACTION_STD_NOSCRIPTWRAPPER(ACTION_CODE, ...) 

#define PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(RESULT) 
#define PERFORM_JSRT_TTD_RECORD_ACTION_COMPLETE_NO_RESULT() 

//TODO: find and replace all of the occourences of this in jsrt.cpp
#define PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(CTX) 
#endif
