//----------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "JsrtPch.h"
#include "jsrtInternal.h"
#include "RuntimeDebugPch.h"
#include "ThreadContextTLSEntry.h"
#include "JsrtDebugUtils.h"

#define VALIDATE_DEBUG_OBJECT(obj) \
    if (obj == nullptr) \
    { \
        return JsErrorDiagNotDebugging; \
    }

#define VALIDATE_RUNTIME_IS_AT_BREAK(runtime) \
    if (!runtime->GetThreadContext()->GetDebugManager()->IsAtDispatchHalt()) \
    { \
        return JsErrorDiagNotAtBreak; \
    }

STDAPI_(JsErrorCode) JsDiagStartDebugging(
    _In_ JsRuntimeHandle runtimeHandle,
    _In_ JsDiagDebugEventCallback debugEventCallback,
    _In_opt_ void* callbackState)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {

        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        // ToDo (SaAgarwa): If a null debugEventCallback is passed should it be treated as Stop debugging?
        PARAM_NOT_NULL(debugEventCallback);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
        ThreadContext * threadContext = runtime->GetThreadContext();

        if (threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress())
        {
            return JsErrorHeapEnumInProgress;
        }
        else if (threadContext->IsInThreadServiceCallback())
        {
            return JsErrorInThreadServiceCallback;
        }
        else if (threadContext->IsInScript())
        {
            return JsErrorRuntimeInUse;
        }
        else if (runtime->GetDebugObject() != nullptr)
        {
            return JsErrorDiagAlreadyInDebugMode;
        }

        ThreadContextScope scope(threadContext);

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        // Create the debug object to save callback function and data
        runtime->EnsureDebugObject();

        JsrtDebug* debugObject = runtime->GetDebugObject();

        debugObject->SetDebugEventCallback(debugEventCallback, callbackState);

        if (threadContext->GetDebugManager() != nullptr)
        {
            threadContext->GetDebugManager()->SetLocalsDisplayFlags(Js::DebugManager::LocalsDisplayFlags::LocalsDisplayFlags_NoGroupMethods);
        }

        for (Js::ScriptContext *scriptContext = threadContext->GetScriptContextList(); scriptContext != nullptr; scriptContext = scriptContext->next)
        {
            if (!scriptContext->IsClosed())
            {
                Assert(!scriptContext->IsScriptContextInSourceRundownOrDebugMode());

                Js::DebugContext* debugContext = scriptContext->GetDebugContext();

                if (debugContext->GetHostDebugContext() == nullptr)
                {
                    debugContext->SetHostDebugContext(debugObject);
                }

                if (FAILED(scriptContext->OnDebuggerAttached()))
                {
                    AssertMsg(false, "Failed to start debugging");
                    return JsErrorFatal; // Inconsistent state, we can't continue from here?
                }

                Js::ProbeContainer* probeContainer = debugContext->GetProbeContainer();
                probeContainer->InitializeInlineBreakEngine(debugObject);
                probeContainer->InitializeDebuggerScriptOptionCallback(debugObject);
            }
        }

        return JsNoError;
    });
}


STDAPI_(JsErrorCode)
JsDiagGetScripts(
    _Out_ JsValueRef *scriptsArray)
{
    return ContextAPIWrapper<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(scriptsArray);

        *scriptsArray = JS_INVALID_REFERENCE;

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtDebug* debugObject = currentContext->GetRuntime()->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        Js::JavascriptArray* scripts = debugObject->GetScripts(scriptContext);

        if (scripts == nullptr)
        {
            return JsErrorDiagUnableToPerformAction;
        }

        *scriptsArray = Js::CrossSite::MarshalVar(scriptContext, scripts);

        return JsNoError;
    });
}


STDAPI_(JsErrorCode)
JsDiagGetSource(
    _In_ unsigned int scriptId,
    _Out_ JsValueRef *source)
{
    return ContextAPIWrapper<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(source);

        *source = JS_INVALID_REFERENCE;

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtDebug* debugObject = currentContext->GetRuntime()->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        Js::DynamicObject* sourceObject = debugObject->GetSource(scriptId);
        if (sourceObject == nullptr)
        {
            return JsErrorInvalidArgument;
        }

        *source = Js::CrossSite::MarshalVar(scriptContext, sourceObject);

        return JsNoError;
    });
}


STDAPI_(JsErrorCode)
JsDiagRequestAsyncBreak(
    _In_ JsRuntimeHandle runtimeHandle)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {

        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            debugObject->EnableAsyncBreak(scriptContext);
        }

        return JsNoError;
    });
}


STDAPI_(JsErrorCode)
JsDiagGetBreakpoints(
    _Out_ JsValueRef *breakPoints)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {

        PARAM_NOT_NULL(breakPoints);

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        Js::JavascriptArray* bpsArray = currentContext->GetScriptContext()->GetLibrary()->CreateArray();

        JsrtRuntime * runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            debugObject->GetBreakpoints(&bpsArray, scriptContext);
        }

        *breakPoints = bpsArray;

        return JsNoError;
    });
}

STDAPI_(JsErrorCode)
JsDiagSetBreakpoint(
    _In_ unsigned int scriptId,
    _In_ unsigned int lineNumber,
    _In_ unsigned int columnNumber,
    _Out_ JsValueRef *breakPoint)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {

        PARAM_NOT_NULL(breakPoint);

        *breakPoint = JS_INVALID_REFERENCE;

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtRuntime * runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        VALIDATE_DEBUG_OBJECT(runtime->GetDebugObject());

        Js::Utf8SourceInfo* utf8SourceInfo = nullptr;

        for (Js::ScriptContext *scriptContext = runtime->GetThreadContext()->GetScriptContextList();
        scriptContext != nullptr && utf8SourceInfo == nullptr && !scriptContext->IsClosed();
            scriptContext = scriptContext->next)
        {
            scriptContext->GetSourceList()->MapUntil([&](int i, RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfoWeakRef) -> bool
            {
                Js::Utf8SourceInfo* sourceInfo = sourceInfoWeakRef->Get();
                if (sourceInfo != nullptr && sourceInfo->GetSourceInfoId() == scriptId)
                {
                    utf8SourceInfo = sourceInfo;
                    return true;
                }
                return false;
            });
        }

        if (utf8SourceInfo != nullptr && utf8SourceInfo->HasDebugDocument())
        {
            JsrtDebug* debugObject = runtime->GetDebugObject();

            Js::DynamicObject* bpObject = debugObject->SetBreakPoint(utf8SourceInfo, lineNumber, columnNumber);

            if(bpObject != nullptr)
            {
                *breakPoint = Js::CrossSite::MarshalVar(currentContext->GetScriptContext(), bpObject);
                return JsNoError;
            }
            else
            {
                return JsErrorDiagUnableToPerformAction;
            }
        }
        return JsErrorDiagObjectNotFound;
    });
}


STDAPI_(JsErrorCode)
JsDiagRemoveBreakpoint(
    _In_ unsigned int breakpointId)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtRuntime* runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        if (!debugObject->RemoveBreakpoint(breakpointId))
        {
            return JsErrorInvalidArgument;
        }

        return JsNoError;
    });
}

STDAPI_(JsErrorCode)
JsDiagSetBreakOnException(
    _In_ JsDiagBreakOnExceptionType exceptionType)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtRuntime* runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        debugObject->SetBreakOnException(exceptionType);

        return JsNoError;
    });
}

STDAPI_(JsErrorCode)
JsDiagGetBreakOnException(
    _Out_ JsDiagBreakOnExceptionType* exceptionType)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {

        PARAM_NOT_NULL(exceptionType);

        JsrtContext *currentContext = JsrtContext::GetCurrent();

        JsrtRuntime* runtime = currentContext->GetRuntime();

        ThreadContextScope scope(runtime->GetThreadContext());

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        *exceptionType = debugObject->GetBreakOnException();

        return JsNoError;
    });
}

STDAPI_(JsErrorCode)
JsDiagSetStepType(
    _In_ JsDiagStepType stepType)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext * scriptContext) -> JsErrorCode {

        JsrtContext *currentContext = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = currentContext->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        if (stepType == JsDiagStepTypeStepIn)
        {
            debugObject->SetResumeType(BREAKRESUMEACTION_STEP_INTO);
        }
        else if (stepType == JsDiagStepTypeStepOut)
        {
            debugObject->SetResumeType(BREAKRESUMEACTION_STEP_OUT);
        }
        else if (stepType == JsDiagStepTypeStepOver)
        {
            debugObject->SetResumeType(BREAKRESUMEACTION_STEP_OVER);
        }

        return JsNoError;
    });
}

STDAPI_(JsErrorCode)
JsDiagGetFunctionPosition(
    _In_ JsValueRef func,
    _Out_ JsValueRef *funcInfo)
{
    return ContextAPIWrapper<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        VALIDATE_INCOMING_REFERENCE(func, scriptContext);
        PARAM_NOT_NULL(funcInfo);

        *funcInfo = JS_INVALID_REFERENCE;

        if (!Js::RecyclableObject::Is(func) || !Js::ScriptFunction::Is(func))
        {
            return JsErrorInvalidArgument;
        }

        Js::ScriptFunction* jsFunction = Js::ScriptFunction::FromVar(func);

        Js::FunctionBody* functionBody = jsFunction->GetFunctionBody();
        if (functionBody != nullptr)
        {
            Js::Utf8SourceInfo* utf8SourceInfo = functionBody->GetUtf8SourceInfo();
            if (utf8SourceInfo != nullptr && !utf8SourceInfo->GetIsLibraryCode())
            {
                ULONG lineNumber = functionBody->GetLineNumber();
                ULONG columnNumber = functionBody->GetColumnNumber();
                uint startOffset = functionBody->GetStatementStartOffset(0);
                ULONG stmtStartLineNumber;
                LONG stmtStartColumnNumber;

                if (functionBody->GetLineCharOffsetFromStartChar(startOffset, &stmtStartLineNumber, &stmtStartColumnNumber))
                {
                    Js::DynamicObject* funcInfoObject = scriptContext->GetLibrary()->CreateObject();

                    if (funcInfoObject != nullptr)
                    {
                        JsrtDebugUtils::AddScriptIdToObject(funcInfoObject, utf8SourceInfo);
                        JsrtDebugUtils::AddFileNameToObject(funcInfoObject, utf8SourceInfo);
                        JsrtDebugUtils::AddPropertyToObject(funcInfoObject, JsrtDebugPropertyId::line, lineNumber, scriptContext);
                        JsrtDebugUtils::AddPropertyToObject(funcInfoObject, JsrtDebugPropertyId::column, columnNumber, scriptContext);
                        JsrtDebugUtils::AddPropertyToObject(funcInfoObject, JsrtDebugPropertyId::stmtStartLine, stmtStartLineNumber, scriptContext);
                        JsrtDebugUtils::AddPropertyToObject(funcInfoObject, JsrtDebugPropertyId::stmtStartColumn, stmtStartColumnNumber, scriptContext);

                        *funcInfo = funcInfoObject;
                        return JsNoError;
                    }
                }
            }
        }

        return JsErrorDiagObjectNotFound;
    });
}


STDAPI_(JsErrorCode)
JsDiagGetStackTrace(
    _Out_ JsValueRef *stackTrace)
{
    return ContextAPIWrapper<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(stackTrace);

        *stackTrace = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        *stackTrace = debugObject->GetStackFrames(scriptContext);

        return JsNoError;
    });
}


STDAPI_(JsErrorCode)
JsDiagGetStackProperties(
    _In_ unsigned int stackFrameIndex,
    _Out_ JsValueRef *properties)
{
    return ContextAPIWrapper<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(properties);

        *properties = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        DebuggerObjectBase* debuggerObject = nullptr;
        if (!debugObject->TryGetFrameObjectFromFrameIndex(scriptContext, stackFrameIndex, &debuggerObject))
        {
            return JsErrorDiagObjectNotFound;
        }

        DebuggerObjectStackFrame* debuggerStackFrame = (DebuggerObjectStackFrame*)debuggerObject;

        Js::DynamicObject* localsObject = debuggerStackFrame->GetLocalsObject();

        if (localsObject != nullptr)
        {
            *properties = Js::CrossSite::MarshalVar(scriptContext, localsObject);
        }

        return JsNoError;
    });
}

STDAPI_(JsErrorCode)
JsDiagGetProperties(
    _In_ unsigned int objectHandle,
    _In_ unsigned int fromCount,
    _In_ unsigned int totalCount,
    _Out_ JsValueRef *propertiesObject)
{

    return ContextAPIWrapper<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(propertiesObject);

        *propertiesObject = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        DebuggerObjectBase* debuggerObject = nullptr;
        if (!debugObject->GetDebuggerObjectsManager()->TryGetDebuggerObjectFromHandle(objectHandle, &debuggerObject) || debuggerObject == nullptr)
        {
            return JsErrorDiagInvalidHandle;
        }

        Js::DynamicObject* properties = debuggerObject->GetChildrens(scriptContext, fromCount, totalCount);
        if (properties != nullptr)
        {
            *propertiesObject = properties;
            return JsNoError;
        }

        return JsErrorDiagUnableToPerformAction;
    });
}


STDAPI_(JsErrorCode)
JsDiagGetObjectFromHandle(
    _In_ unsigned int objectHandle,
    _Out_ JsValueRef *handleObject)
{
    return ContextAPIWrapper<false>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(handleObject);

        *handleObject = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        DebuggerObjectBase* debuggerObject = nullptr;
        if (!debugObject->GetDebuggerObjectsManager()->TryGetDebuggerObjectFromHandle(objectHandle, &debuggerObject) || debuggerObject == nullptr)
        {
            return JsErrorDiagInvalidHandle;
        }

        Js::DynamicObject* object = debuggerObject->GetJSONObject(scriptContext);

        if (object != nullptr)
        {
            *handleObject = object;
            return JsNoError;
        }

        return JsErrorDiagUnableToPerformAction;
    });
}

STDAPI_(JsErrorCode)
JsDiagEvaluate(
    _In_ const wchar_t *script,
    _In_ unsigned int stackFrameIndex,
    _Out_ JsValueRef *evalResult)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PARAM_NOT_NULL(script);
        PARAM_NOT_NULL(evalResult);

        *evalResult = JS_INVALID_REFERENCE;

        JsrtContext* context = JsrtContext::GetCurrent();
        JsrtRuntime* runtime = context->GetRuntime();

        VALIDATE_RUNTIME_IS_AT_BREAK(runtime);

        JsrtDebug* debugObject = runtime->GetDebugObject();

        VALIDATE_DEBUG_OBJECT(debugObject);

        DebuggerObjectBase* debuggerObject = nullptr;
        if (!debugObject->TryGetFrameObjectFromFrameIndex(scriptContext, stackFrameIndex, &debuggerObject))
        {
            return JsErrorDiagObjectNotFound;
        }

        DebuggerObjectStackFrame* debuggerStackFrame = (DebuggerObjectStackFrame*)debuggerObject;

        Js::DynamicObject* result = debuggerStackFrame->Evaluate(script, false);
        if (result != nullptr)
        {
            *evalResult = Js::CrossSite::MarshalVar(scriptContext, result);
        }

        return JsNoError;
    });
}
