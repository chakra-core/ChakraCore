//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JsrtPch.h"
#ifdef ENABLE_SCRIPT_DEBUGGING
#include "JsrtDebugManager.h"
#include "JsrtDebugEventObject.h"
#include "JsrtDebugUtils.h"
#include "JsrtDebuggerObject.h"
#include "RuntimeDebugPch.h"
#include "screrror.h"   // For CompileScriptException

JsrtDebugManager::JsrtDebugManager(ThreadContext* threadContext) :
    HostDebugContext(nullptr),
    threadContext(threadContext),
    debugEventCallback(nullptr),
    callbackState(nullptr),
    resumeAction(BREAKRESUMEACTION_CONTINUE),
    debugObjectArena(nullptr),
    debuggerObjectsManager(nullptr),
    debugDocumentManager(nullptr),
    stackFrames(nullptr),
    breakOnExceptionAttributes(JsDiagBreakOnExceptionAttributeUncaught)
{
    Assert(threadContext != nullptr);
}

JsrtDebugManager::~JsrtDebugManager()
{
    if (this->debuggerObjectsManager != nullptr)
    {
        Adelete(this->debugObjectArena, this->debuggerObjectsManager);
        this->debuggerObjectsManager = nullptr;
    }

    if (this->debugDocumentManager != nullptr)
    {
        Adelete(this->debugObjectArena, this->debugDocumentManager);
        this->debugDocumentManager = nullptr;
    }

    if (this->debugObjectArena != nullptr)
    {
        this->threadContext->GetRecycler()->UnregisterExternalGuestArena(this->debugObjectArena);
        HeapDelete(this->debugObjectArena);
        this->debugObjectArena = nullptr;
    }

    this->debugEventCallback = nullptr;
    this->callbackState = nullptr;
    this->threadContext = nullptr;
}

void JsrtDebugManager::SetDebugEventCallback(JsDiagDebugEventCallback debugEventCallback, void* callbackState)
{
    Assert(this->debugEventCallback == nullptr);
    Assert(this->callbackState == nullptr);

    this->debugEventCallback = debugEventCallback;
    this->callbackState = callbackState;
}

void* JsrtDebugManager::GetAndClearCallbackState()
{
    void* currentCallbackState = this->callbackState;

    this->debugEventCallback = nullptr;
    this->callbackState = nullptr;

    return currentCallbackState;
}

bool JsrtDebugManager::IsDebugEventCallbackSet() const
{
    return this->debugEventCallback != nullptr;
}

bool JsrtDebugManager::CanHalt(Js::InterpreterHaltState* haltState)
{
    // This is registered as the callback for inline breakpoints.
    // We decide here if we are at a reasonable stop location that has source code.
    Assert(haltState->IsValid());

    Js::FunctionBody* pCurrentFuncBody = haltState->GetFunction();
    int byteOffset = haltState->GetCurrentOffset();
    Js::FunctionBody::StatementMap* map = pCurrentFuncBody->GetMatchingStatementMapFromByteCode(byteOffset, false);

    // Resolve the dummy ret code.
    return map != nullptr && (!pCurrentFuncBody->GetIsGlobalFunc() || !Js::FunctionBody::IsDummyGlobalRetStatement(&map->sourceSpan));
}

void JsrtDebugManager::DispatchHalt(Js::InterpreterHaltState* haltState)
{
    switch (haltState->stopType)
    {
    case Js::STOP_BREAKPOINT: /*JsDiagDebugEventBreakpoint*/
    case Js::STOP_INLINEBREAKPOINT: /*JsDiagDebugEventDebuggerStatement*/
    case Js::STOP_ASYNCBREAK: /*JsDiagDebugEventAsyncBreak*/
        this->ReportBreak(haltState);
        break;
    case Js::STOP_STEPCOMPLETE: /*JsDiagDebugEventStepComplete*/
        this->SetResumeType(BREAKRESUMEACTION_CONTINUE);
        this->ReportBreak(haltState);
        break;
    case Js::STOP_EXCEPTIONTHROW: /*JsDiagDebugEventRuntimeException*/
        this->ReportExceptionBreak(haltState);
        break;
    case Js::STOP_MUTATIONBREAKPOINT:
        AssertMsg(false, "Not yet handled");
        break;
    default:
        AssertMsg(false, "Unhandled stop type");
    }

    this->HandleResume(haltState, this->resumeAction);
}

bool JsrtDebugManager::CanAllowBreakpoints()
{
    return true;
}

void JsrtDebugManager::CleanupHalt()
{
}

bool JsrtDebugManager::IsInClosedState()
{
    return this->debugEventCallback == nullptr;
}

bool JsrtDebugManager::IsExceptionReportingEnabled()
{
    return this->GetBreakOnException() != JsDiagBreakOnExceptionAttributeNone;
}

bool JsrtDebugManager::IsFirstChanceExceptionEnabled()
{
    return (this->GetBreakOnException() & JsDiagBreakOnExceptionAttributeFirstChance) == JsDiagBreakOnExceptionAttributeFirstChance;
}

HRESULT JsrtDebugManager::DbgRegisterFunction(Js::ScriptContext* scriptContext, Js::FunctionBody* functionBody, DWORD_PTR dwDebugSourceContext, LPCWSTR title)
{
    Js::Utf8SourceInfo* utf8SourceInfo = functionBody->GetUtf8SourceInfo();

    if (!utf8SourceInfo->GetIsLibraryCode() && !utf8SourceInfo->HasDebugDocument())
    {
        JsrtDebugDocumentManager* debugDocumentManager = this->GetDebugDocumentManager();
        Assert(debugDocumentManager != nullptr);

        Js::DebugDocument* debugDocument = HeapNewNoThrow(Js::DebugDocument, utf8SourceInfo, functionBody);
        if (debugDocument != nullptr)
        {
            utf8SourceInfo->SetDebugDocument(debugDocument);
        }

        // Raising events during the middle of a source reparse allows the host to reenter the
        // script context and cause memory race conditions. Suppressing these events during a
        // reparse prevents the issue. Since the host was already expected to call JsDiagGetScripts
        // once the attach is completed to get the list of parsed scripts, there is no change in
        // behavior.
        if (this->debugEventCallback != nullptr &&
            !scriptContext->GetDebugContext()->GetIsReparsingSource())
        {
            JsrtDebugEventObject debugEventObject(scriptContext);
            Js::DynamicObject* eventDataObject = debugEventObject.GetEventDataObject();
            JsrtDebugUtils::AddSourceMetadataToObject(eventDataObject, utf8SourceInfo);

            this->CallDebugEventCallback(JsDiagDebugEventSourceCompile, eventDataObject, scriptContext, false /*isBreak*/);
        }
    }

    return S_OK;
}

#if ENABLE_TTD
void JsrtDebugManager::ReportScriptCompile_TTD(Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException, bool notify)
{
    if(this->debugEventCallback == nullptr)
    {
        return;
    }

    Js::ScriptContext* scriptContext = utf8SourceInfo->GetScriptContext();

    JsrtDebugEventObject debugEventObject(scriptContext);
    Js::DynamicObject* eventDataObject = debugEventObject.GetEventDataObject();

    JsrtDebugDocumentManager* debugDocumentManager = this->GetDebugDocumentManager();
    Assert(debugDocumentManager != nullptr);

    // Create DebugDocument and then report JsDiagDebugEventSourceCompile event
    Js::DebugDocument* debugDocument = HeapNewNoThrow(Js::DebugDocument, utf8SourceInfo, body);
    if(debugDocument != nullptr)
    {
        utf8SourceInfo->SetDebugDocument(debugDocument);
    }

    JsrtDebugUtils::AddSourceMetadataToObject(eventDataObject, utf8SourceInfo);

    if(notify)
    {
        this->CallDebugEventCallback(JsDiagDebugEventSourceCompile, eventDataObject, scriptContext, false /*isBreak*/);
    }
}
#endif

void JsrtDebugManager::ReportScriptCompile(Js::JavascriptFunction* scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException)
{
    if (this->debugEventCallback != nullptr)
    {
        Js::ScriptContext* scriptContext = utf8SourceInfo->GetScriptContext();

        JsrtDebugEventObject debugEventObject(scriptContext);
        Js::DynamicObject* eventDataObject = debugEventObject.GetEventDataObject();

        JsDiagDebugEvent jsDiagDebugEvent = JsDiagDebugEventCompileError;

        if (scriptFunction == nullptr)
        {
            // Report JsDiagDebugEventCompileError event
            JsrtDebugUtils::AddPropertyToObject(eventDataObject, JsrtDebugPropertyId::error, compileException->ei.bstrDescription, ::SysStringLen(compileException->ei.bstrDescription), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(eventDataObject, JsrtDebugPropertyId::line, compileException->line, scriptContext);
            JsrtDebugUtils::AddPropertyToObject(eventDataObject, JsrtDebugPropertyId::column, compileException->ichMin - compileException->ichMinLine - 1, scriptContext); // Converted to 0-based
            JsrtDebugUtils::AddPropertyToObject(eventDataObject, JsrtDebugPropertyId::sourceText, compileException->bstrLine, ::SysStringLen(compileException->bstrLine), scriptContext);
        }
        else
        {
            JsrtDebugDocumentManager* debugDocumentManager = this->GetDebugDocumentManager();
            Assert(debugDocumentManager != nullptr);

            // Create DebugDocument and then report JsDiagDebugEventSourceCompile event
            Js::DebugDocument* debugDocument = HeapNewNoThrow(Js::DebugDocument, utf8SourceInfo, scriptFunction->GetFunctionBody());
            if (debugDocument != nullptr)
            {
                utf8SourceInfo->SetDebugDocument(debugDocument);
            }

            jsDiagDebugEvent = JsDiagDebugEventSourceCompile;
        }

        JsrtDebugUtils::AddSourceMetadataToObject(eventDataObject, utf8SourceInfo);

        this->CallDebugEventCallback(jsDiagDebugEvent, eventDataObject, scriptContext, false /*isBreak*/);
    }
}

void JsrtDebugManager::ReportBreak(Js::InterpreterHaltState* haltState)
{
    if (this->debugEventCallback != nullptr)
    {
        Js::FunctionBody* functionBody = haltState->GetFunction();
        Assert(functionBody != nullptr);

        Js::Utf8SourceInfo* utf8SourceInfo = functionBody->GetUtf8SourceInfo();
        int currentByteCodeOffset = haltState->GetCurrentOffset();
        Js::ScriptContext* scriptContext = utf8SourceInfo->GetScriptContext();

        JsDiagDebugEvent jsDiagDebugEvent = this->GetDebugEventFromStopType(haltState->stopType);

        JsrtDebugEventObject debugEventObject(scriptContext);

        Js::DynamicObject* eventDataObject = debugEventObject.GetEventDataObject();

        Js::ProbeContainer* probeContainer = scriptContext->GetDebugContext()->GetProbeContainer();

        if (jsDiagDebugEvent == JsDiagDebugEventBreakpoint)
        {
            UINT bpId = 0;
            probeContainer->MapProbesUntil([&](int i, Js::Probe* pProbe)
            {
                Js::BreakpointProbe* bp = (Js::BreakpointProbe*)pProbe;
                if (bp->Matches(functionBody, utf8SourceInfo->GetDebugDocument(), currentByteCodeOffset))
                {
                    bpId = bp->GetId();
                    return true;
                }
                return false;
            });

            AssertMsg(bpId != 0, "How come we don't have a breakpoint id for JsDiagDebugEventBreakpoint");

            JsrtDebugUtils::AddPropertyToObject(eventDataObject, JsrtDebugPropertyId::breakpointId, bpId, scriptContext);
        }

        JsrtDebugUtils::AddScriptIdToObject(eventDataObject, utf8SourceInfo);
        JsrtDebugUtils::AddLineColumnToObject(eventDataObject, functionBody, currentByteCodeOffset);
        JsrtDebugUtils::AddSourceLengthAndTextToObject(eventDataObject, functionBody, currentByteCodeOffset);

        this->CallDebugEventCallbackForBreak(jsDiagDebugEvent, eventDataObject, scriptContext);
    }
}

void JsrtDebugManager::ReportExceptionBreak(Js::InterpreterHaltState* haltState)
{
    if (this->debugEventCallback != nullptr)
    {
        Assert(haltState->stopType == Js::STOP_EXCEPTIONTHROW);

        Js::Utf8SourceInfo* utf8SourceInfo = haltState->GetFunction()->GetUtf8SourceInfo();
        Js::ScriptContext* scriptContext = utf8SourceInfo->GetScriptContext();

        JsDiagDebugEvent jsDiagDebugEvent = JsDiagDebugEventRuntimeException;

        JsrtDebugEventObject debugEventObject(scriptContext);

        Js::DynamicObject* eventDataObject = debugEventObject.GetEventDataObject();

        JsrtDebugUtils::AddScriptIdToObject(eventDataObject, utf8SourceInfo);

        Js::FunctionBody* functionBody = haltState->topFrame->GetFunction();

        Assert(functionBody != nullptr);
        
        int currentByteCodeOffset = haltState->topFrame->GetByteCodeOffset();
        JsrtDebugUtils::AddLineColumnToObject(eventDataObject, functionBody, currentByteCodeOffset);
        JsrtDebugUtils::AddSourceLengthAndTextToObject(eventDataObject, functionBody, currentByteCodeOffset);
        JsrtDebugUtils::AddPropertyToObject(eventDataObject, JsrtDebugPropertyId::uncaught, !haltState->exceptionObject->IsFirstChanceException(), scriptContext);

        Js::ResolvedObject resolvedObject;
        resolvedObject.scriptContext = scriptContext;
        resolvedObject.name = _u("{exception}");
        resolvedObject.typeId = Js::TypeIds_Error;
        resolvedObject.address = nullptr;
        resolvedObject.obj = scriptContext->GetDebugContext()->GetProbeContainer()->GetExceptionObject();

        if (resolvedObject.obj == nullptr)
        {
            resolvedObject.obj = resolvedObject.scriptContext->GetLibrary()->GetUndefined();
        }

        JsrtDebuggerObjectBase::CreateDebuggerObject<JsrtDebuggerObjectProperty>(this->GetDebuggerObjectsManager(), resolvedObject, scriptContext, /* forceSetValueProp */ false, [&](Js::Var marshaledObj)
        {
            JsrtDebugUtils::AddPropertyToObject(eventDataObject, JsrtDebugPropertyId::exception, marshaledObj, scriptContext);
        });
        
        this->CallDebugEventCallbackForBreak(jsDiagDebugEvent, eventDataObject, scriptContext);
    }
}

void JsrtDebugManager::HandleResume(Js::InterpreterHaltState* haltState, BREAKRESUMEACTION resumeAction)
{
    Assert(resumeAction != BREAKRESUMEACTION_ABORT);

    Js::ScriptContext* scriptContext = haltState->framePointers->Peek()->GetScriptContext();

    scriptContext->GetThreadContext()->GetDebugManager()->stepController.HandleResumeAction(haltState, resumeAction);
}

void JsrtDebugManager::SetResumeType(BREAKRESUMEACTION resumeAction)
{
    this->resumeAction = resumeAction;
}

bool JsrtDebugManager::EnableAsyncBreak(Js::ScriptContext* scriptContext)
{
    if (!scriptContext->IsDebugContextInitialized())
    {
        // Although the script context exists, it hasn't been fully initialized yet.
        return false;
    }

    Js::ProbeContainer* probeContainer = scriptContext->GetDebugContext()->GetProbeContainer();

    // This can be called when we are already at break
    if (!probeContainer->IsAsyncActivate())
    {
        probeContainer->AsyncActivate(this);
        if (Js::Configuration::Global.EnableJitInDebugMode())
        {
            scriptContext->GetThreadContext()->GetDebugManager()->GetDebuggingFlags()->SetForceInterpreter(true);
        }
        return true;
    }
    return false;
}

void JsrtDebugManager::CallDebugEventCallback(JsDiagDebugEvent debugEvent, Js::DynamicObject* eventDataObject, Js::ScriptContext* scriptContext, bool isBreak)
{
    class AutoClear
    {
    public:
        AutoClear(JsrtDebugManager* jsrtDebug, void* dispatchHaltFrameAddress)
        {
            this->jsrtDebugManager = jsrtDebug;
            this->jsrtDebugManager->GetThreadContext()->GetDebugManager()->SetDispatchHaltFrameAddress(dispatchHaltFrameAddress);
        }

        ~AutoClear()
        {
            if (jsrtDebugManager->debuggerObjectsManager != nullptr)
            {
                jsrtDebugManager->GetDebuggerObjectsManager()->ClearAll();
            }

            if (jsrtDebugManager->stackFrames != nullptr)
            {
                Adelete(jsrtDebugManager->GetDebugObjectArena(), jsrtDebugManager->stackFrames);
                jsrtDebugManager->stackFrames = nullptr;
            }
            this->jsrtDebugManager->GetThreadContext()->GetDebugManager()->SetDispatchHaltFrameAddress(nullptr);
            this->jsrtDebugManager = nullptr;
        }
    private:
        JsrtDebugManager* jsrtDebugManager;
    };

    auto funcPtr = [&]()
    {
        if (isBreak)
        {
            void *frameAddress = _AddressOfReturnAddress();
            // If we are reporting break we should clear all objects after call returns

            // Save the frame address, when asking for stack we will only give stack which is under this address
            // because host can execute javascript after break which should not be part of stack.
            AutoClear autoClear(this, frameAddress);
            this->debugEventCallback(debugEvent, eventDataObject, this->callbackState);
        }
        else
        {
            this->debugEventCallback(debugEvent, eventDataObject, this->callbackState);
        }
    };

    if (scriptContext->GetThreadContext()->IsScriptActive())
    {
        BEGIN_LEAVE_SCRIPT(scriptContext)
        {
            funcPtr();
        }
        END_LEAVE_SCRIPT(scriptContext);
    }
    else
    {
        funcPtr();
    }
}

void JsrtDebugManager::CallDebugEventCallbackForBreak(JsDiagDebugEvent debugEvent, Js::DynamicObject* eventDataObject, Js::ScriptContext* scriptContext)
{
    AutoSetDispatchHaltFlag autoSetDispatchHaltFlag(scriptContext, scriptContext->GetThreadContext());

    this->CallDebugEventCallback(debugEvent, eventDataObject, scriptContext, true /*isBreak*/);

    for (Js::ScriptContext *tempScriptContext = scriptContext->GetThreadContext()->GetScriptContextList();
    tempScriptContext != nullptr && !tempScriptContext->IsClosed();
        tempScriptContext = tempScriptContext->next)
    {
        tempScriptContext->GetDebugContext()->GetProbeContainer()->AsyncDeactivate();
    }

    if (Js::Configuration::Global.EnableJitInDebugMode())
    {
        scriptContext->GetThreadContext()->GetDebugManager()->GetDebuggingFlags()->SetForceInterpreter(false);
    }
}

Js::DynamicObject* JsrtDebugManager::GetScript(Js::Utf8SourceInfo* utf8SourceInfo)
{
    Js::DynamicObject* scriptObject = utf8SourceInfo->GetScriptContext()->GetLibrary()->CreateObject();
    JsrtDebugUtils::AddSourceMetadataToObject(scriptObject, utf8SourceInfo);

    return scriptObject;
}

Js::JavascriptArray* JsrtDebugManager::GetScripts(Js::ScriptContext* scriptContext)
{
    Js::JavascriptArray* scriptsArray = scriptContext->GetLibrary()->CreateArray();

    int index = 0;

    for (Js::ScriptContext *tempScriptContext = scriptContext->GetThreadContext()->GetScriptContextList();
    tempScriptContext != nullptr && !tempScriptContext->IsClosed();
        tempScriptContext = tempScriptContext->next)
    {
        tempScriptContext->MapScript([&](Js::Utf8SourceInfo* utf8SourceInfo)
        {
            if (!utf8SourceInfo->GetIsLibraryCode() && utf8SourceInfo->HasDebugDocument())
            {
                bool isCallerLibraryCode = false;

                bool isDynamic = utf8SourceInfo->IsDynamic();

                if (isDynamic)
                {
                    // If the code is dynamic (eval or new Function) only return the script if parent is non-library
                    Js::Utf8SourceInfo* callerUtf8SourceInfo = utf8SourceInfo->GetCallerUtf8SourceInfo();

                    while (callerUtf8SourceInfo != nullptr && !isCallerLibraryCode)
                    {
                        isCallerLibraryCode = callerUtf8SourceInfo->GetIsLibraryCode();
                        callerUtf8SourceInfo = callerUtf8SourceInfo->GetCallerUtf8SourceInfo();
                    }
                }

                if (!isCallerLibraryCode)
                {
                    Js::DynamicObject* sourceObj = this->GetScript(utf8SourceInfo);

                    if (sourceObj != nullptr)
                    {
                        Js::Var marshaledObj = Js::CrossSite::MarshalVar(scriptContext, sourceObj);
                        scriptsArray->DirectSetItemAt(index, marshaledObj);
                        index++;
                    }
                }
            }
        });
    }

    return scriptsArray;
}

Js::DynamicObject* JsrtDebugManager::GetSource(Js::ScriptContext* scriptContext, uint scriptId)
{
    Js::Utf8SourceInfo* utf8SourceInfo = nullptr;

    for (Js::ScriptContext *tempScriptContext = this->threadContext->GetScriptContextList();
    tempScriptContext != nullptr && utf8SourceInfo == nullptr && !tempScriptContext->IsClosed();
        tempScriptContext = tempScriptContext->next)
    {
        tempScriptContext->MapScript([&](Js::Utf8SourceInfo* sourceInfo) -> bool
        {
            if (sourceInfo->IsInDebugMode() && sourceInfo->GetSourceInfoId() == scriptId)
            {
                utf8SourceInfo = sourceInfo;
                return true;
            }
            return false;
        });
    }

    Js::DynamicObject* sourceObject = nullptr;

    if (utf8SourceInfo != nullptr)
    {
        sourceObject = (Js::DynamicObject*)Js::CrossSite::MarshalVar(utf8SourceInfo->GetScriptContext(), scriptContext->GetLibrary()->CreateObject());

        JsrtDebugUtils::AddSourceMetadataToObject(sourceObject, utf8SourceInfo);
        JsrtDebugUtils::AddSourceToObject(sourceObject, utf8SourceInfo);
    }

    return sourceObject;
}

Js::JavascriptArray* JsrtDebugManager::GetStackFrames(Js::ScriptContext* scriptContext)
{
    if (this->stackFrames == nullptr)
    {
        this->stackFrames = Anew(this->GetDebugObjectArena(), JsrtDebugStackFrames, this);
    }

    return this->stackFrames->StackFrames(scriptContext);
}

bool JsrtDebugManager::TryGetFrameObjectFromFrameIndex(Js::ScriptContext *scriptContext, uint frameIndex, JsrtDebuggerStackFrame ** debuggerStackFrame)
{
    if (this->stackFrames == nullptr)
    {
        this->GetStackFrames(scriptContext);
    }

    return this->stackFrames->TryGetFrameObjectFromFrameIndex(frameIndex, debuggerStackFrame);
}

Js::DynamicObject* JsrtDebugManager::SetBreakPoint(Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* utf8SourceInfo, UINT lineNumber, UINT columnNumber)
{
    Js::DebugDocument* debugDocument = utf8SourceInfo->GetDebugDocument();
    if (debugDocument != nullptr && SUCCEEDED(utf8SourceInfo->EnsureLineOffsetCacheNoThrow()) && lineNumber < utf8SourceInfo->GetLineCount())
    {
        charcount_t charPosition = 0;
        charcount_t byteOffset = 0;
        utf8SourceInfo->GetCharPositionForLineInfo((charcount_t)lineNumber, &charPosition, &byteOffset);
        long ibos = charPosition + columnNumber + 1;

        Js::StatementLocation statement;
        if (!debugDocument->GetStatementLocation(ibos, &statement))
        {
            return nullptr;
        }

        // Don't see a use case for supporting multiple breakpoints at same location.
        // If a breakpoint already exists, just return that
        Js::BreakpointProbe* probe = debugDocument->FindBreakpoint(statement);
        if (probe == nullptr)
        {
            probe = debugDocument->SetBreakPoint(statement, BREAKPOINT_ENABLED);

            if(probe == nullptr)
            {
                return nullptr;
            }

            this->GetDebugDocumentManager()->AddDocument(probe->GetId(), debugDocument);
        }

        probe->GetStatementLocation(&statement);

        Js::DynamicObject* bpObject = (Js::DynamicObject*)Js::CrossSite::MarshalVar(debugDocument->GetUtf8SourceInfo()->GetScriptContext(), scriptContext->GetLibrary()->CreateObject());

        JsrtDebugUtils::AddPropertyToObject(bpObject, JsrtDebugPropertyId::breakpointId, probe->GetId(), scriptContext);
        JsrtDebugUtils::AddLineColumnToObject(bpObject, statement.function, statement.bytecodeSpan.begin);
        JsrtDebugUtils::AddScriptIdToObject(bpObject, utf8SourceInfo);

        return bpObject;
    }

    return nullptr;
}

void JsrtDebugManager::GetBreakpoints(Js::JavascriptArray** bpsArray, Js::ScriptContext* scriptContext)
{
    Js::ScriptContext* arrayScriptContext = (*bpsArray)->GetScriptContext();
    Js::ProbeContainer* probeContainer = scriptContext->GetDebugContext()->GetProbeContainer();

    probeContainer->MapProbes([&](int i, Js::Probe* pProbe)
    {
        Js::BreakpointProbe* bp = (Js::BreakpointProbe*)pProbe;
        Js::DynamicObject* bpObject = scriptContext->GetLibrary()->CreateObject();

        JsrtDebugUtils::AddPropertyToObject(bpObject, JsrtDebugPropertyId::breakpointId, bp->GetId(), scriptContext);
        JsrtDebugUtils::AddLineColumnToObject(bpObject, bp->GetFunctionBody(), bp->GetBytecodeOffset());

        Js::Utf8SourceInfo* utf8SourceInfo = bp->GetDbugDocument()->GetUtf8SourceInfo();
        JsrtDebugUtils::AddScriptIdToObject(bpObject, utf8SourceInfo);

        Js::Var marshaledObj = Js::CrossSite::MarshalVar(arrayScriptContext, bpObject);
        Js::JavascriptOperators::OP_SetElementI((Js::Var)(*bpsArray), Js::JavascriptNumber::ToVar((*bpsArray)->GetLength(), arrayScriptContext), marshaledObj, arrayScriptContext);
    });
}

#if ENABLE_TTD
Js::BreakpointProbe* JsrtDebugManager::SetBreakpointHelper_TTD(int64 desiredBpId, Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* utf8SourceInfo, UINT lineNumber, UINT columnNumber, BOOL* isNewBP)
{
    *isNewBP = FALSE;
    Js::DebugDocument* debugDocument = utf8SourceInfo->GetDebugDocument();
    if(debugDocument != nullptr && SUCCEEDED(utf8SourceInfo->EnsureLineOffsetCacheNoThrow()) && lineNumber < utf8SourceInfo->GetLineCount())
    {
        charcount_t charPosition = 0;
        charcount_t byteOffset = 0;
        utf8SourceInfo->GetCharPositionForLineInfo((charcount_t)lineNumber, &charPosition, &byteOffset);
        long ibos = charPosition + columnNumber + 1;

        Js::StatementLocation statement;
        if(!debugDocument->GetStatementLocation(ibos, &statement))
        {
            return nullptr;
        }

        // Don't see a use case for supporting multiple breakpoints at same location.
        // If a breakpoint already exists, just return that
        Js::BreakpointProbe* probe = debugDocument->FindBreakpoint(statement);
        TTDAssert(probe == nullptr || desiredBpId == -1, "We shouldn't be resetting this BP unless it was cleared eariler!");

        if(probe == nullptr)
        {
            probe = debugDocument->SetBreakPoint_TTDWbpId(desiredBpId, statement);
            *isNewBP = TRUE;

            this->GetDebugDocumentManager()->AddDocument(probe->GetId(), debugDocument);
        }

        return probe;
    }

    return nullptr;
}
#endif

JsrtDebuggerObjectsManager* JsrtDebugManager::GetDebuggerObjectsManager()
{
    if (this->debuggerObjectsManager == nullptr)
    {
        this->debuggerObjectsManager = Anew(this->GetDebugObjectArena(), JsrtDebuggerObjectsManager, this);
    }
    return this->debuggerObjectsManager;
}

void JsrtDebugManager::ClearDebuggerObjects()
{
    if (this->debuggerObjectsManager != nullptr)
    {
        this->debuggerObjectsManager->ClearAll();
    }
}

ArenaAllocator* JsrtDebugManager::GetDebugObjectArena()
{
    if (this->debugObjectArena == nullptr)
    {
        this->debugObjectArena = HeapNew(ArenaAllocator, _u("DebugObjectArena"), this->threadContext->GetPageAllocator(), Js::Throw::OutOfMemory);

        this->threadContext->GetRecycler()->RegisterExternalGuestArena(this->debugObjectArena);
    }

    return this->debugObjectArena;
}

JsrtDebugDocumentManager* JsrtDebugManager::GetDebugDocumentManager()
{
    if (this->debugDocumentManager == nullptr)
    {
        this->debugDocumentManager = Anew(this->GetDebugObjectArena(), JsrtDebugDocumentManager, this);
    }
    return this->debugDocumentManager;
}

void JsrtDebugManager::ClearDebugDocument(Js::ScriptContext* scriptContext)
{
    if (this->debugDocumentManager != nullptr)
    {
        this->debugDocumentManager->ClearDebugDocument(scriptContext);
    }
}

void JsrtDebugManager::ClearBreakpointDebugDocumentDictionary()
{
    if (this->debugDocumentManager != nullptr)
    {
        this->debugDocumentManager->ClearBreakpointDebugDocumentDictionary();
    }
}

bool JsrtDebugManager::RemoveBreakpoint(UINT breakpointId)
{
    if (this->debugDocumentManager != nullptr)
    {
        return this->GetDebugDocumentManager()->RemoveBreakpoint(breakpointId);
    }

    return false;
}

void JsrtDebugManager::SetBreakOnException(JsDiagBreakOnExceptionAttributes exceptionAttributes)
{
    this->breakOnExceptionAttributes = exceptionAttributes;
}

JsDiagBreakOnExceptionAttributes JsrtDebugManager::GetBreakOnException()
{
    return this->breakOnExceptionAttributes;
}

JsDiagDebugEvent JsrtDebugManager::GetDebugEventFromStopType(Js::StopType stopType)
{
    switch (stopType)
    {
    case Js::STOP_BREAKPOINT: return JsDiagDebugEventBreakpoint;
    case Js::STOP_INLINEBREAKPOINT: return JsDiagDebugEventDebuggerStatement;
    case Js::STOP_STEPCOMPLETE: return JsDiagDebugEventStepComplete;
    case Js::STOP_EXCEPTIONTHROW: return JsDiagDebugEventRuntimeException;
    case Js::STOP_ASYNCBREAK: return JsDiagDebugEventAsyncBreak;

    case Js::STOP_MUTATIONBREAKPOINT:
    default:
        Assert("Unhandled stoptype");
        break;
    }

    return JsDiagDebugEventBreakpoint;
}
#endif