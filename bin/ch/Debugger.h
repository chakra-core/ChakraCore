//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "stdafx.h"

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

#include <queue>

#define DGB_ENSURE_OK(R) if(R != JsNoError) { AssertMsg(false, "Internal failure aborting!!!");  exit(1);}

asdf;
static const WCHAR controllerScript[] = L"XXXX";
//{
//#include "chakra_debug.js"
//    L'\0'
//};

class DebuggerCh
{
private:
    wchar_t m_ipAddr[20];
    unsigned short m_port;
    SOCKET m_dbgConnectSocket;
    SOCKET m_dbgSocket;

    JsContextRef m_context;
    JsValueRef m_chakraDebugObject;
    JsValueRef m_processDebugProtocolJSON;
    JsValueRef m_processJsrtEventData;

    DebuggerCh(LPCWSTR ipAddr, unsigned short port);
    ~DebuggerCh();

    size_t m_buflen;
    char* m_buf;
    std::queue<wchar_t*> m_msgQueue;
    bool m_waitingForMessage;
    bool m_isProcessingDebuggerMsg;

    wchar_t* PopMessage();
    void SendMsg(const wchar_t* msg, size_t msgLen);
    bool IsEmpty();
    bool ShouldContinue();
    void WaitForMessage();

    void ProcessDebuggerMessage();
    bool ProcessJsrtDebugEvent(JsDiagDebugEvent debugEvent, JsValueRef eventData);

public:
    bool Initialize(JsRuntimeHandle runtime);
    bool InstallDebugCallbacks(JsValueRef chakraDebugObject);
    bool InstallHostCallback(JsValueRef chakraDebugObject, const wchar_t *name, JsNativeFunction nativeFunction);
    bool CallFunction(wchar_t const * functionName, JsValueRef* arguments, unsigned short argumentCount, JsValueRef *result);

    static void CALLBACK JsDiagDebugEventHandler(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState);

    static JsValueRef CALLBACK Log(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetScripts(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsGetSource(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagResume(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsSetBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetFunctionPosition(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsGetStacktrace(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetStackProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagLookupHandles(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsEvaluateScript(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagEvaluate(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetBreakpoints(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagRemoveBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagSetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK SendDelayedRespose(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);

    static void StartDebugging(JsRuntimeHandle runtime, LPCWSTR ipAddr, unsigned short port);
    static DebuggerCh* GetDebugger();
    static void CloseDebugger();

    bool HandleDebugEvent(JsDiagDebugEvent debugEvent, JsValueRef eventData);

    static DebuggerCh* debugger;
};
