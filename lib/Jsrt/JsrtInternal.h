//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "JsrtExceptionBase.h"
#include "Exceptions/EvalDisabledException.h"

//A class to ensure that even when exceptions are thrown we update any event recording info we were in the middle of
#if ENABLE_TTD
typedef TTD::TTDJsRTActionResultAutoRecorder TTDRecorder;
#else
typedef struct {} TTDRecorder;
#endif

#ifdef NTBUILD // non-browser
#define JSRT_VERIFY_RUNTIME_STATE
#endif

// JSRT Unsafe mode leaves runtime health-state responsibility to the host
#ifndef JSRT_VERIFY_RUNTIME_STATE
#define JSRT_MAYBE_TRUE false
#else
#define JSRT_MAYBE_TRUE true
#endif

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
JsErrorCode GlobalAPIWrapper_Core(Fn fn)
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
    CATCH_STATIC_JAVASCRIPT_EXCEPTION_OBJECT(errCode)

    CATCH_OTHER_EXCEPTIONS(errCode)

    return errCode;
}

template <class Fn>
JsErrorCode GlobalAPIWrapper(Fn fn)
{
    TTDRecorder _actionEntryPopper;
    JsErrorCode errCode = GlobalAPIWrapper_Core([&fn, &_actionEntryPopper]() -> JsErrorCode
    {
        return fn(_actionEntryPopper);
    });

#if ENABLE_TTD
    _actionEntryPopper.CompleteWithStatusCode(errCode);
#endif

    return errCode;
}

template <class Fn>
JsErrorCode GlobalAPIWrapper_NoRecord(Fn fn)
{
    return GlobalAPIWrapper_Core([&fn]() -> JsErrorCode
    {
        return fn();
    });
}

JsErrorCode CheckContext(JsrtContext *currentContext, bool verifyRuntimeState, bool allowInObjectBeforeCollectCallback = false);

template <bool verifyRuntimeState, class Fn>
JsErrorCode ContextAPIWrapper_Core(Fn fn)
{
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode errCode = CheckContext(currentContext, verifyRuntimeState);

    if(errCode != JsNoError)
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
    catch(Js::OutOfMemoryException)
    {
        errCode = JsErrorOutOfMemory;
    }
    catch(const Js::JavascriptException& err)
    {
        scriptContext->GetThreadContext()->SetRecordedException(err.GetAndClear());
        errCode = JsErrorScriptException;
    }
    catch(Js::ScriptAbortException)
    {
        Assert(scriptContext->GetThreadContext()->GetRecordedException() == nullptr);
        scriptContext->GetThreadContext()->SetRecordedException(scriptContext->GetThreadContext()->GetPendingTerminatedErrorObject());
        errCode = JsErrorScriptTerminated;
    }
    catch(Js::EvalDisabledException)
    {
        errCode = JsErrorScriptEvalDisabled;
    }

    CATCH_OTHER_EXCEPTIONS(errCode)

    return errCode;
}

template <bool verifyRuntimeState, class Fn>
JsErrorCode ContextAPIWrapper(Fn fn)
{
    TTDRecorder _actionEntryPopper;
    JsErrorCode errCode = ContextAPIWrapper_Core<verifyRuntimeState>([&fn, &_actionEntryPopper](Js::ScriptContext* scriptContext) -> JsErrorCode
    {
        return fn(scriptContext, _actionEntryPopper);
    });

#if ENABLE_TTD
    _actionEntryPopper.CompleteWithStatusCode(errCode);
#endif

    return errCode;
}

template <bool verifyRuntimeState, class Fn>
JsErrorCode ContextAPIWrapper_NoRecord(Fn fn)
{
    return ContextAPIWrapper_Core<verifyRuntimeState>([&fn](Js::ScriptContext* scriptContext) -> JsErrorCode
    {
        return fn(scriptContext);
    });
}

// allowInObjectBeforeCollectCallback only when current API is guaranteed not to do recycler allocation.
template <class Fn>
JsErrorCode ContextAPINoScriptWrapper_Core(Fn fn, bool allowInObjectBeforeCollectCallback = false,
    bool scriptExceptionAllowed = false)
{
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode errCode = CheckContext(currentContext, /*verifyRuntimeState*/JSRT_MAYBE_TRUE,
        allowInObjectBeforeCollectCallback);

    if(errCode != JsNoError)
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
    CATCH_STATIC_JAVASCRIPT_EXCEPTION_OBJECT(errCode)

    catch(const Js::JavascriptException& err)
    {
        AssertMsg(false, "Should never get JavascriptExceptionObject for ContextAPINoScriptWrapper.");
        scriptContext->GetThreadContext()->SetRecordedException(err.GetAndClear());
        errCode = JsErrorScriptException;
    }

    catch(Js::ScriptAbortException)
    {
        Assert(scriptContext->GetThreadContext()->GetRecordedException() == nullptr);
        scriptContext->GetThreadContext()->SetRecordedException(scriptContext->GetThreadContext()->GetPendingTerminatedErrorObject());
        errCode = JsErrorScriptTerminated;
    }

    CATCH_OTHER_EXCEPTIONS(errCode)

    return errCode;
}

template <class Fn>
JsErrorCode ContextAPINoScriptWrapper(Fn fn, bool allowInObjectBeforeCollectCallback = false, bool scriptExceptionAllowed = false)
{
    TTDRecorder _actionEntryPopper;
    JsErrorCode errCode = ContextAPINoScriptWrapper_Core([&fn, &_actionEntryPopper](Js::ScriptContext* scriptContext) -> JsErrorCode
    {
        return fn(scriptContext, _actionEntryPopper);
    }, allowInObjectBeforeCollectCallback, scriptExceptionAllowed);

#if ENABLE_TTD
    _actionEntryPopper.CompleteWithStatusCode(errCode);
#endif

    return errCode;
}

template <class Fn>
JsErrorCode ContextAPINoScriptWrapper_NoRecord(Fn fn, bool allowInObjectBeforeCollectCallback = false, bool scriptExceptionAllowed = false)
{
    return ContextAPINoScriptWrapper_Core([&fn](Js::ScriptContext* scriptContext) -> JsErrorCode
    {
        return fn(scriptContext);
    }, allowInObjectBeforeCollectCallback, scriptExceptionAllowed);
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
    catch (const Js::JavascriptException& err)
    {
        scriptContext->GetThreadContext()->SetRecordedException(err.GetAndClear());
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
    CATCH_OTHER_EXCEPTIONS(errorCode)
    AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
    JsrtContext::TrySetCurrent(oldContext);
    return errorCode;
}

void HandleScriptCompileError(Js::ScriptContext * scriptContext, CompileScriptException * se, const WCHAR * sourceUrl = nullptr);

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
//A class to ensure that even when exceptions are thrown we update any event recording info we were in the middle of
#if ENABLE_TTD
#define PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX) (CTX)->ShouldPerformRecordAction()

#define PERFORM_JSRT_TTD_RECORD_ACTION(CTX, ACTION_CODE, ...) \
    if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) \
    { \
        (CTX)->GetThreadContext()->TTDLog->##ACTION_CODE##(_actionEntryPopper, ##__VA_ARGS__); \
    }

#define PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(CTX, RESULT) if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) \
    { \
        _actionEntryPopper.SetResult(RESULT); \
    }

//TODO: find and replace all of the occourences of this in jsrt.cpp
#define PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(CTX) if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) { AssertMsg(false, "Need to implement support here!!!"); }
#else
#define PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX) false

#define PERFORM_JSRT_TTD_RECORD_ACTION(CTX, ACTION_CODE, ...)
#define PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(CTX, RESULT)

#define PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(CTX)
#endif
