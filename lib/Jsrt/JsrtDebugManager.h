//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "../Runtime/Debug/RuntimeDebugPch.h"
#include "JsrtDebuggerObject.h"
#include "JsrtDebugEventObject.h"

class JsrtDebugManager : public Js::HaltCallback, public Js::DebuggerOptionsCallback, public HostDebugContext
{
public:
    JsrtDebugManager(ThreadContext* threadContext);
    ~JsrtDebugManager();

    void SetDebugEventCallback(JsDiagDebugEventCallback debugEventCallback, void* callbackState);
    void* GetAndClearCallbackState();
    bool IsDebugEventCallbackSet() const;

#if ENABLE_TTD
    void ReportScriptCompile_TTD(Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException, bool notify);
#endif

    void ReportScriptCompile(Js::JavascriptFunction* scriptFunction, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException);
    void ReportBreak(Js::InterpreterHaltState* haltState);
    void ReportExceptionBreak(Js::InterpreterHaltState* haltState);
    void HandleResume(Js::InterpreterHaltState* haltState, BREAKRESUMEACTION resumeAction);
    void SetResumeType(BREAKRESUMEACTION resumeAction);

    bool EnableAsyncBreak(Js::ScriptContext* scriptContext);

    void CallDebugEventCallback(JsDiagDebugEvent debugEvent, Js::DynamicObject* eventDataObject, Js::ScriptContext* scriptContext, bool isBreak);
    void CallDebugEventCallbackForBreak(JsDiagDebugEvent debugEvent, Js::DynamicObject* eventDataObject, Js::ScriptContext* scriptContext);

    Js::JavascriptArray* GetScripts(Js::ScriptContext* scriptContext);
    Js::DynamicObject* GetScript(Js::Utf8SourceInfo* utf8SourceInfo);
    Js::DynamicObject* GetSource(Js::ScriptContext* scriptContext, uint scriptId);

    Js::JavascriptArray* GetStackFrames(Js::ScriptContext* scriptContext);
    bool TryGetFrameObjectFromFrameIndex(Js::ScriptContext *scriptContext, uint frameIndex, JsrtDebuggerStackFrame ** debuggerStackFrame);

    Js::DynamicObject* SetBreakPoint(Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* utf8SourceInfo, UINT lineNumber, UINT columnNumber);
    void GetBreakpoints(Js::JavascriptArray** bpsArray, Js::ScriptContext* scriptContext);

#if ENABLE_TTD
    Js::BreakpointProbe* SetBreakpointHelper_TTD(int64 desiredBpId, Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* utf8SourceInfo, UINT lineNumber, UINT columnNumber, BOOL* isNewBP);
#endif

    JsrtDebuggerObjectsManager* GetDebuggerObjectsManager();
    void ClearDebuggerObjects();

    ArenaAllocator* GetDebugObjectArena();

    JsrtDebugDocumentManager* GetDebugDocumentManager();
    void ClearDebugDocument(Js::ScriptContext* scriptContext);
    void ClearBreakpointDebugDocumentDictionary();

    bool RemoveBreakpoint(UINT breakpointId);

    void SetBreakOnException(JsDiagBreakOnExceptionAttributes exceptionAttributes);
    JsDiagBreakOnExceptionAttributes GetBreakOnException();

    JsDiagDebugEvent GetDebugEventFromStopType(Js::StopType stopType);
    ThreadContext* GetThreadContext() const { return this->threadContext; }
private:
    ThreadContext* threadContext;
    JsDiagDebugEventCallback debugEventCallback;
    void* callbackState;
    BREAKRESUMEACTION resumeAction;
    ArenaAllocator* debugObjectArena;
    JsrtDebuggerObjectsManager* debuggerObjectsManager;
    JsrtDebugDocumentManager* debugDocumentManager;
    JsrtDebugStackFrames* stackFrames;
    JsDiagBreakOnExceptionAttributes breakOnExceptionAttributes;

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
    DWORD_PTR GetHostSourceContext(Js::Utf8SourceInfo* sourceInfo) { return Js::Constants::NoHostSourceContext; }
    HRESULT SetThreadDescription(__in LPCWSTR url) { return S_OK; }
    HRESULT DbgRegisterFunction(Js::ScriptContext* scriptContext, Js::FunctionBody* functionBody, DWORD_PTR dwDebugSourceContext, LPCWSTR title);
    void ReParentToCaller(Js::Utf8SourceInfo* sourceInfo) {}
};
