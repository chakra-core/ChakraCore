//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "..\Runtime\Debug\RuntimeDebugPch.h"
#include "JsrtDebuggerObject.h"
#include "JsrtDebugEventObject.h"

class JsrtDebug : public Js::HaltCallback, public Js::DebuggerOptionsCallback, public HostDebugContext
{
public:
    JsrtDebug(ThreadContext* threadContext);
    ~JsrtDebug();

    void SetDebugEventCallback(JsDiagDebugEventCallback debugEventCallback, void* callbackState);
    void ReportScriptCompile(Js::JavascriptFunction * scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException);
    void ReportBreak(Js::InterpreterHaltState * haltState);
    void ReportExceptionBreak(Js::InterpreterHaltState * haltState);
    void HandleResume(Js::InterpreterHaltState * haltState, BREAKRESUMEACTION resumeAction);
    void SetResumeType(BREAKRESUMEACTION resumeAction);

    bool EnableAsyncBreak(Js::ScriptContext* scriptContext);

    void CallDebugEventCallback(JsDiagDebugEvent debugEvent, Js::DynamicObject* eventDataObject, Js::ScriptContext* scriptContext);
    void CallDebugEventCallbackForBreak(JsDiagDebugEvent debugEvent, Js::DynamicObject* eventDataObject, Js::ScriptContext* scriptContext);

    Js::JavascriptArray * GetScripts(Js::ScriptContext* scriptContext);
    Js::DynamicObject* GetScript(Js::Utf8SourceInfo* utf8SourceInfo);
    Js::DynamicObject * GetSource(uint scriptId);

    Js::JavascriptArray * GetStackFrames(Js::ScriptContext* scriptContext);
    bool TryGetFrameObjectFromFrameIndex(Js::ScriptContext *scriptContext, uint frameIndex, DebuggerObjectBase ** debuggerObject);

    Js::DynamicObject* SetBreakPoint(Js::Utf8SourceInfo* utf8SourceInfo, UINT lineNumber, UINT columnNumber);
    void GetBreakpoints(Js::JavascriptArray** bpsArray, Js::ScriptContext* scriptContext);

    DebuggerObjectsManager* GetDebuggerObjectsManager();
    void ClearDebuggerObjects();

    ArenaAllocator* GetDebugObjectArena();

    DebugDocumentManager* GetDebugDocumentManager();
    void ClearDebugDocument(Js::ScriptContext* scriptContext);

    bool RemoveBreakpoint(UINT breakpointId);

    void SetBreakOnException(JsDiagBreakOnExceptionType breakOnExceptionType);
    JsDiagBreakOnExceptionType GetBreakOnException();

    JsDiagDebugEvent GetDebugEventFromStopType(Js::StopType stopType);

private:
    ThreadContext* threadContext;
    JsDiagDebugEventCallback debugEventCallback;
    void* callbackState;
    BREAKRESUMEACTION resumeAction;
    ArenaAllocator* debugObjectArena;
    DebuggerObjectsManager* debuggerObjectsManager;
    uint callBackDepth;
    DebugDocumentManager* debugDocumentManager;
    JsrtDebugStackFrames* stackFrames;
    JsDiagBreakOnExceptionType breakOnExceptionType;

    // Js::HaltCallback overrides
    virtual bool CanHalt(Js::InterpreterHaltState* pHaltState);
    virtual void DispatchHalt(Js::InterpreterHaltState* pHaltState);
    virtual void CleanupHalt() sealed;
    virtual bool CanAllowBreakpoints();
    virtual bool IsInClosedState();

    // Js::DebuggerOptionsCallback overrides
    virtual bool IsExceptionReportingEnabled();
    virtual bool IsFirstChanceExceptionEnabled();

    // HostDebugContext overrides
    virtual void Delete() {}
    DWORD_PTR GetHostSourceContext(Js::Utf8SourceInfo * sourceInfo) { return Js::Constants::NoHostSourceContext; }
    HRESULT SetThreadDescription(__in LPCWSTR url) { return S_OK; }
    HRESULT DbgRegisterFunction(Js::ScriptContext * scriptContext, Js::FunctionBody * functionBody, DWORD_PTR dwDebugSourceContext, LPCWSTR title);
    void ReParentToCaller(Js::Utf8SourceInfo* sourceInfo) {}
};

