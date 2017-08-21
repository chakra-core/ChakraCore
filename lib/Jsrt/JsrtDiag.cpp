//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JsrtPch.h"
#ifdef ENABLE_SCRIPT_DEBUGGING
#include "JsrtInternal.h"
#include "RuntimeDebugPch.h"
#include "ThreadContextTlsEntry.h"
#include "JsrtDebugUtils.h"
#include "Codex/Utf8Helper.h"

#define VALIDATE_IS_DEBUGGING(jsrtDebugManager) \
    if (jsrtDebugManager == nullptr || !jsrtDebugManager->IsDebugEventCallbackSet()) \
    { \
        return JsErrorDiagNotInDebugMode; \
    }

#define VALIDATE_RUNTIME_IS_AT_BREAK(runtime) \
    if (runtime->GetThreadContext()->GetDebugManager() == nullptr || !runtime->GetThreadContext()->GetDebugManager()->IsAtDispatchHalt()) \
    { \
        return JsErrorDiagNotAtBreak; \
    }

#define VALIDATE_RUNTIME_STATE_FOR_START_STOP_DEBUGGING(threadContext) \
    if (threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress()) \
    { \
        return JsErrorHeapEnumInProgress; \
    } \
    else if (threadContext->IsInThreadServiceCallback()) \
    { \
        return JsErrorInThreadServiceCallback; \
    } \
    else if (threadContext->IsInScript()) \
    { \
        return JsErrorRuntimeInUse; \
    } \
    ThreadContextScope scope(threadContext); \
    if (!scope.IsValid()) \
    { \
        return JsErrorWrongThread; \
    }
#endif

CHAKRA_API JsDiagStartDebugging(
    _In_ JsRuntimeHandle runtimeHandle,
    _In_ JsDiagDebugEventCallback debugEventCallback,
    _In_opt_ void* callbackState)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        PARAM_NOT_NULL(debugEventCallback);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
        ThreadContext * threadContext = runtime->GetThreadContext();

        VALIDATE_RUNTIME_STATE_FOR_START_STOP_DEBUGGING(threadContext);

        if (runtime->GetJsrtDebugManager() != nullptr && runtime->GetJsrtDebugManager()->IsDebugEventCallbackSet())
        {
            return JsErrorDiagAlreadyInDebugMode;
        }

        // Create the debug object to save callback function and data
        runtime->EnsureJsrtDebugManager();

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        jsrtDebugManager->SetDebugEventCallback(debugEventCallback, callbackState);

        if (threadContext->GetDebugManager() != nullptr)
        {
            threadContext->GetDebugManager()->SetLocalsDisplayFlags(Js::DebugManager::LocalsDisplayFlags::LocalsDisplayFlags_NoGroupMethods);
        }

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            Assert(!scriptContext->IsScriptContextInDebugMode());

            Js::DebugContext* debugContext = scriptContext->GetDebugContext();

            if (debugContext->GetHostDebugContext() == nullptr)
            {
                debugContext->SetHostDebugContext(jsrtDebugManager);
            }

            HRESULT hr;
            if (FAILED(hr = scriptContext->OnDebuggerAttached()))
            {
                Debugger_AttachDetach_fatal_error(hr); // Inconsistent state, we can't continue from here
                return JsErrorFatal;
            }

            Js::ProbeContainer* probeContainer = debugContext->GetProbeContainer();
            probeContainer->InitializeInlineBreakEngine(jsrtDebugManager);
            probeContainer->InitializeDebuggerScriptOptionCallback(jsrtDebugManager);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagStopDebugging(
    _In_ JsRuntimeHandle runtimeHandle,
    _Out_ void** callbackState)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        PARAM_NOT_NULL(callbackState);

        *callbackState = nullptr;

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
        ThreadContext * threadContext = runtime->GetThreadContext();

        VALIDATE_RUNTIME_STATE_FOR_START_STOP_DEBUGGING(threadContext);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            Assert(scriptContext->IsScriptContextInDebugMode());

            HRESULT hr;
            if (FAILED(hr = scriptContext->OnDebuggerDetached()))
            {
                Debugger_AttachDetach_fatal_error(hr); // Inconsistent state, we can't continue from here
                return JsErrorFatal;
            }

            Js::DebugContext* debugContext = scriptContext->GetDebugContext();

            Js::ProbeContainer* probeContainer = debugContext->GetProbeContainer();
            probeContainer->UninstallInlineBreakpointProbe(nullptr);
            probeContainer->UninstallDebuggerScriptOptionCallback();

            jsrtDebugManager->ClearBreakpointDebugDocumentDictionary();
        }

        *callbackState = jsrtDebugManager->GetAndClearCallbackState();

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagGetScripts(
    _Out_ JsValueRef *scriptsArray)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(scriptsArray);

        *scriptsArray = JS_INVALID_REFERENCE;

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtDebugManager* jsrtDebugManager = currentContext->GetRuntime()->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        Js::JavascriptArray* scripts = jsrtDebugManager->GetScripts(scriptContext);

        if (scripts != nullptr)
        {
            *scriptsArray = scripts;
            return JsNoError;
        }

        return JsErrorDiagUnableToPerformAction;
    });
#endif
}

CHAKRA_API JsDiagGetSource(
    _In_ unsigned int scriptId,
    _Out_ JsValueRef *source)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(source);

        *source = JS_INVALID_REFERENCE;

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtDebugManager* jsrtDebugManager = currentContext->GetRuntime()->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        Js::DynamicObject* sourceObject = jsrtDebugManager->GetSource(scriptContext, scriptId);

        if (sourceObject != nullptr)
        {
            *source = sourceObject;
            return JsNoError;
        }

        return JsErrorInvalidArgument;
    });
#endif
}

CHAKRA_API JsDiagRequestAsyncBreak(
    _In_ JsRuntimeHandle runtimeHandle)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            jsrtDebugManager->EnableAsyncBreak(scriptContext);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagGetBreakpoints(
    _Out_ JsValueRef *breakpoints)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        PARAM_NOT_NULL(breakpoints);

        *breakpoints = JS_INVALID_REFERENCE;

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        Js::JavascriptArray* bpsArray = currentContext->GetScriptContext()->GetLibrary()->CreateArray();

        JsrtRuntime * runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            jsrtDebugManager->GetBreakpoints(&bpsArray, scriptContext);
        }

        *breakpoints = bpsArray;

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagSetBreakpoint(
    _In_ unsigned int scriptId,
    _In_ unsigned int lineNumber,
    _In_ unsigned int columnNumber,
    _Out_ JsValueRef *breakpoint)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        PARAM_NOT_NULL(breakpoint);

        *breakpoint = JS_INVALID_REFERENCE;

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtRuntime * runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        VALIDATE_IS_DEBUGGING(runtime->GetJsrtDebugManager());

        Js::Utf8SourceInfo* utf8SourceInfo = nullptr;

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && utf8SourceInfo == nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            scriptContext->MapScript([&](Js::Utf8SourceInfo* sourceInfo) -> bool
            {
                if (sourceInfo->GetSourceInfoId() == scriptId)
                {
                    utf8SourceInfo = sourceInfo;
                    return true;
                }
                return false;
            });
        }

        if (utf8SourceInfo != nullptr && utf8SourceInfo->HasDebugDocument())
        {
            JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

            Js::DynamicObject* bpObject = jsrtDebugManager->SetBreakPoint(currentContext->GetScriptContext(), utf8SourceInfo, lineNumber, columnNumber);

            if(bpObject != nullptr)
            {
                *breakpoint = bpObject;
                return JsNoError;
            }

            return JsErrorDiagUnableToPerformAction;
        }

        return JsErrorDiagObjectNotFound;
    });
#endif
}

CHAKRA_API JsDiagRemoveBreakpoint(
    _In_ unsigned int breakpointId)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtRuntime* runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        if (!jsrtDebugManager->RemoveBreakpoint(breakpointId))
        {
            return JsErrorInvalidArgument;
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagSetBreakOnException(
    _In_ JsRuntimeHandle runtimeHandle,
    _In_ JsDiagBreakOnExceptionAttributes exceptionAttributes)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        jsrtDebugManager->SetBreakOnException(exceptionAttributes);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagGetBreakOnException(
    _In_ JsRuntimeHandle runtimeHandle,
    _Out_ JsDiagBreakOnExceptionAttributes* exceptionAttributes)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {

        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        PARAM_NOT_NULL(exceptionAttributes);

        *exceptionAttributes = JsDiagBreakOnExceptionAttributeNone;

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        *exceptionAttributes = jsrtDebugManager->GetBreakOnException();

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagSetStepType(
    _In_ JsDiagStepType stepType)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<true>([&](Js::ScriptContext * scriptContext) -> JsErrorCode {

        JsrtContext *currentContext = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = currentContext->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        if (stepType == JsDiagStepTypeStepIn)
        {
            jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_STEP_INTO);
        }
        else if (stepType == JsDiagStepTypeStepOut)
        {
            jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_STEP_OUT);
        }
        else if (stepType == JsDiagStepTypeStepOver)
        {
            jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_STEP_OVER);
        }
        else if (stepType == JsDiagStepTypeStepBack)
        {
#if ENABLE_TTD
            ThreadContext* threadContext = runtime->GetThreadContext();
            if(!threadContext->IsRuntimeInTTDMode())
            {
                //Don't want to fail hard when user accidentally clicks this so pring message and step forward 
                fprintf(stderr, "Must be in replay mode to use reverse-step - launch with \"--replay-debug\" flag in Node.");
                jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_STEP_OVER);
            }
            else
            {
                threadContext->TTDExecutionInfo->SetPendingTTDStepBackMove();

                //don't worry about BP suppression because we are just going to throw after we return
                jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_CONTINUE);
            }
#else
            return JsErrorInvalidArgument;
#endif
        }
        else if (stepType == JsDiagStepTypeReverseContinue)
        {
#if ENABLE_TTD
            ThreadContext* threadContext = runtime->GetThreadContext();
            if(!threadContext->IsRuntimeInTTDMode())
            {
                //Don't want to fail hard when user accidentally clicks this so pring message and step forward 
                fprintf(stderr, "Must be in replay mode to use reverse-continue - launch with \"--replay-debug\" flag in Node.");
                jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_CONTINUE);
            }
            else
            {
                threadContext->TTDExecutionInfo->SetPendingTTDReverseContinueMove(JsTTDMoveMode::JsTTDMoveScanIntervalForContinue);

                //don't worry about BP suppression because we are just going to throw after we return
                jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_CONTINUE);
            }
#else
            return JsErrorInvalidArgument;
#endif
        }
        else if (stepType == JsDiagStepTypeContinue)
        {
            jsrtDebugManager->SetResumeType(BREAKRESUMEACTION_CONTINUE);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagGetFunctionPosition(
    _In_ JsValueRef function,
    _Out_ JsValueRef *functionPosition)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        VALIDATE_INCOMING_REFERENCE(function, scriptContext);
        PARAM_NOT_NULL(functionPosition);

        *functionPosition = JS_INVALID_REFERENCE;

        if (!Js::RecyclableObject::Is(function) || !Js::ScriptFunction::Is(function))
        {
            return JsErrorInvalidArgument;
        }

        Js::ScriptFunction* jsFunction = Js::ScriptFunction::FromVar(function);

        BOOL fParsed = jsFunction->GetParseableFunctionInfo()->IsFunctionParsed();
        if (!fParsed)
        {
            Js::JavascriptFunction::DeferredParseCore(&jsFunction, fParsed);
        }

        if (fParsed)
        {
            Js::FunctionBody* functionBody = jsFunction->GetFunctionBody();
            if (functionBody != nullptr)
            {
                Js::Utf8SourceInfo* utf8SourceInfo = functionBody->GetUtf8SourceInfo();
                if (utf8SourceInfo != nullptr && !utf8SourceInfo->GetIsLibraryCode())
                {
                    ULONG lineNumber = functionBody->GetLineNumber();
                    ULONG columnNumber = functionBody->GetColumnNumber();
                    uint startOffset = functionBody->GetStatementStartOffset(0);
                    ULONG firstStatementLine;
                    LONG firstStatementColumn;

                    if (functionBody->GetLineCharOffsetFromStartChar(startOffset, &firstStatementLine, &firstStatementColumn))
                    {
                        Js::DynamicObject* funcPositionObject = (Js::DynamicObject*)Js::CrossSite::MarshalVar(utf8SourceInfo->GetScriptContext(), scriptContext->GetLibrary()->CreateObject());

                        if (funcPositionObject != nullptr)
                        {
                            JsrtDebugUtils::AddScriptIdToObject(funcPositionObject, utf8SourceInfo);
                            JsrtDebugUtils::AddFileNameOrScriptTypeToObject(funcPositionObject, utf8SourceInfo);
                            JsrtDebugUtils::AddPropertyToObject(funcPositionObject, JsrtDebugPropertyId::line, (uint32)lineNumber, scriptContext);
                            JsrtDebugUtils::AddPropertyToObject(funcPositionObject, JsrtDebugPropertyId::column, (uint32)columnNumber, scriptContext);
                            JsrtDebugUtils::AddPropertyToObject(funcPositionObject, JsrtDebugPropertyId::firstStatementLine, (uint32)firstStatementLine, scriptContext);
                            JsrtDebugUtils::AddPropertyToObject(funcPositionObject, JsrtDebugPropertyId::firstStatementColumn, (int32)firstStatementColumn, scriptContext);

                            *functionPosition = funcPositionObject;

                            return JsNoError;
                        }
                    }
                }
            }
        }

        return JsErrorDiagObjectNotFound;
    });
#endif
}

CHAKRA_API JsDiagGetStackTrace(
    _Out_ JsValueRef *stackTrace)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(stackTrace);

        *stackTrace = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        *stackTrace = jsrtDebugManager->GetStackFrames(scriptContext);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsDiagGetStackProperties(
    _In_ unsigned int stackFrameIndex,
    _Out_ JsValueRef *properties)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(properties);

        *properties = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        JsrtDebuggerStackFrame* debuggerStackFrame = nullptr;
        if (!jsrtDebugManager->TryGetFrameObjectFromFrameIndex(scriptContext, stackFrameIndex, &debuggerStackFrame))
        {
            return JsErrorDiagObjectNotFound;
        }

        Js::DynamicObject* localsObject = debuggerStackFrame->GetLocalsObject(scriptContext);

        if (localsObject != nullptr)
        {
            *properties = localsObject;
            return JsNoError;
        }

        return JsErrorDiagUnableToPerformAction;
    });
#endif
}

CHAKRA_API JsDiagGetProperties(
    _In_ unsigned int objectHandle,
    _In_ unsigned int fromCount,
    _In_ unsigned int totalCount,
    _Out_ JsValueRef *propertiesObject)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(propertiesObject);

        *propertiesObject = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        JsrtDebuggerObjectBase* debuggerObject = nullptr;
        if (!jsrtDebugManager->GetDebuggerObjectsManager()->TryGetDebuggerObjectFromHandle(objectHandle, &debuggerObject) || debuggerObject == nullptr)
        {
            return JsErrorDiagInvalidHandle;
        }

        Js::DynamicObject* properties = debuggerObject->GetChildren(scriptContext, fromCount, totalCount);

        if (properties != nullptr)
        {
            *propertiesObject = properties;
            return JsNoError;
        }

        return JsErrorDiagUnableToPerformAction;
    });
#endif
}

CHAKRA_API JsDiagGetObjectFromHandle(
    _In_ unsigned int objectHandle,
    _Out_ JsValueRef *handleObject)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(handleObject);

        *handleObject = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        JsrtDebuggerObjectBase* debuggerObject = nullptr;
        if (!jsrtDebugManager->GetDebuggerObjectsManager()->TryGetDebuggerObjectFromHandle(objectHandle, &debuggerObject) || debuggerObject == nullptr)
        {
            return JsErrorDiagInvalidHandle;
        }

        Js::DynamicObject* object = debuggerObject->GetJSONObject(scriptContext, /* forceSetValueProp */ false);

        if (object != nullptr)
        {
            *handleObject = object;
            return JsNoError;
        }

        return JsErrorDiagUnableToPerformAction;
    });
#endif
}

CHAKRA_API JsDiagEvaluate(
    _In_ JsValueRef expressionVal,
    _In_ unsigned int stackFrameIndex,
    _In_ JsParseScriptAttributes parseAttributes,
    _In_ bool forceSetValueProp,
    _Out_ JsValueRef *evalResult)
{
#ifndef ENABLE_SCRIPT_DEBUGGING
    return JsErrorCategoryUsage;
#else
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(expressionVal);
        PARAM_NOT_NULL(evalResult);

        bool isArrayBuffer = Js::ArrayBuffer::Is(expressionVal),
             isString = false;
        bool isUtf8   = !(parseAttributes & JsParseScriptAttributeArrayBufferIsUtf16Encoded);

        if (!isArrayBuffer)
        {
            isString = Js::JavascriptString::Is(expressionVal);
            if (!isString)
            {
                return JsErrorInvalidArgument;
            }
        }

        const size_t len = isArrayBuffer ?
            Js::ArrayBuffer::FromVar(expressionVal)->GetByteLength() :
            Js::JavascriptString::FromVar(expressionVal)->GetLength();

        if (len > INT_MAX)
        {
            return JsErrorInvalidArgument;
        }

        const WCHAR* expression;
        utf8::NarrowToWide wide_expression;
        if (isArrayBuffer && isUtf8)
        {
            wide_expression.Initialize(
                (const char*)Js::ArrayBuffer::FromVar(expressionVal)->GetBuffer(), len);
            if (!wide_expression)
            {
                return JsErrorOutOfMemory;
            }
            expression = wide_expression;
        }
        else
        {
            expression = !isArrayBuffer ?
                Js::JavascriptString::FromVar(expressionVal)->GetSz() // String
                :
                (const WCHAR*)Js::ArrayBuffer::FromVar(expressionVal)->GetBuffer(); // ArrayBuffer;
        }

        *evalResult = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        VALIDATE_IS_DEBUGGING(jsrtDebugManager);

        JsrtDebuggerStackFrame* debuggerStackFrame = nullptr;
        if (!jsrtDebugManager->TryGetFrameObjectFromFrameIndex(scriptContext, stackFrameIndex, &debuggerStackFrame))
        {
            return JsErrorDiagObjectNotFound;
        }

        Js::DynamicObject* result = nullptr;
        bool success = debuggerStackFrame->Evaluate(scriptContext, expression, static_cast<int>(len), false, forceSetValueProp, &result);

        if (result != nullptr)
        {
            *evalResult = result;
        }

        return success ? JsNoError : JsErrorScriptException;

    }, false /*allowInObjectBeforeCollectCallback*/, true /*scriptExceptionAllowed*/);
#endif
}
