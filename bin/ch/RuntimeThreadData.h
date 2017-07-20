//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include <list>

class RuntimeThreadData
{
public:
    RuntimeThreadData();
    ~RuntimeThreadData();
    HANDLE hevntInitialScriptCompleted;
    HANDLE hevntReceivedBroadcast;
    HANDLE hevntShutdown;
    HANDLE hSemaphore;
    HANDLE hThread;

    JsSharedArrayBufferContentHandle sharedContent;
    JsValueRef receiveBroadcastCallbackFunc;


    JsRuntimeHandle runtime;
    JsContextRef context;


    std::string initialSource;

    RuntimeThreadData* parent;
    
    std::list<RuntimeThreadData*> children;

    CRITICAL_SECTION csReportQ;
    std::list<std::string> reportQ;

    bool leaving;


    DWORD ThreadProc();

};

struct RuntimeThreadLocalData
{
    // can't use ctor/dtor because it's not supported in VS2012
    // error C2483: 'threadLocalData' : object with constructor or destructor cannot be declared 'thread' 
    void Initialize(RuntimeThreadData* threadData);
    void Uninitialize();
    RuntimeThreadData* threadData;
};

RuntimeThreadLocalData& GetRuntimeThreadLocalData();
