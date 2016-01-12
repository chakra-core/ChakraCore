//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "..\Runtime\Debug\RuntimeDebugPch.h"
#include "JsrtDebuggerObject.h"
#include "JsrtDebugEventObject.h"

class JsrtDebug : public Js::HaltCallback, public Js::DebuggerOptionsCallback
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

    void CallDebugEventCallback(JsDiagDebugEvent debugEvent, Js::DynamicObject* eventDataObject);

    Js::JavascriptArray * GetScripts(Js::ScriptContext* scriptContext);
    Js::DynamicObject* GetScript(Js::Utf8SourceInfo* utf8SourceInfo);
    Js::DynamicObject * GetSource(uint scriptId);

    Js::JavascriptArray * GetStackFrames(Js::ScriptContext* scriptContext, uint fromIndex, uint totalFrames);
    Js::DynamicObject * GetStackFrame(Js::DiagStackFrame* stackFrame, uint frameIndex);

    HRESULT SetBreakPoint(Js::Utf8SourceInfo* utf8SourceInfo, UINT lineNumber, UINT columnNumber, UINT *breakpointId);
    void GetBreakpoints(Js::JavascriptArray** bpsArray, Js::ScriptContext* scriptContext);

    DebuggerObjectsManager* GetDebuggerObjectsManager();

    ArenaAllocator* GetDebugObjectArena();

    DebugDocumentManager* GetDebugDocumentManager();
    void ClearDebugDocument(Js::ScriptContext* scriptContext);

    void RemoveBreakpoint(UINT breakpointId);

    void SetBreakOnException(JsDiagBreakOnExceptionType breakOnExceptionType);
    JsDiagBreakOnExceptionType GetBreakOnException();

private:
    ThreadContext* threadContext;
    JsDiagDebugEventCallback debugEventCallback;
    void* callbackState;
    BREAKRESUMEACTION resumeAction;
    ArenaAllocator* debugObjectArena;
    DebuggerObjectsManager* debuggerObjectsManager;
    uint callBackDepth;
    DebugDocumentManager* debugDocumentManager;

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
};
