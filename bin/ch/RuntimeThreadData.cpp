//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#ifndef _WIN32
HANDLE CreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName)
{
    // xplat-todo: implement this in PAL
    Assert(false);
    return INVALID_HANDLE_VALUE;
}
BOOL ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount)
{
    // xplat-todo: implement this in PAL
    Assert(false);
    return FALSE;
}

#endif

void RuntimeThreadLocalData::Initialize(RuntimeThreadData* threadData)
{
    this->threadData = threadData;
}

void RuntimeThreadLocalData::Uninitialize()
{
}


THREAD_LOCAL RuntimeThreadLocalData threadLocalData;

RuntimeThreadLocalData& GetRuntimeThreadLocalData()
{
    return threadLocalData;
}

RuntimeThreadData::RuntimeThreadData()
{
    this->hevntInitialScriptCompleted = CreateEvent(NULL, TRUE, FALSE, NULL);
    this->hevntReceivedBroadcast = CreateEvent(NULL, FALSE, FALSE, NULL);
    this->hevntShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);

    this->sharedContent = nullptr;
    this->receiveBroadcastCallbackFunc = nullptr;

    this->leaving = false;

    InitializeCriticalSection(&csReportQ);

}

RuntimeThreadData::~RuntimeThreadData()
{
    CloseHandle(this->hevntInitialScriptCompleted);
    CloseHandle(this->hevntReceivedBroadcast);
    CloseHandle(this->hevntShutdown);
    CloseHandle(this->hThread);
    DeleteCriticalSection(&csReportQ);
}

DWORD RuntimeThreadData::ThreadProc()
{
    JsValueRef scriptSource;
    JsValueRef fname;
    const char* fullPath = "agent source";
    HRESULT hr = S_OK;

    threadLocalData.Initialize(this);

    IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime));
    IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
    IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));


    if (!WScriptJsrt::Initialize())
    {
        IfFailGo(E_FAIL);
    }


    IfJsErrorFailLog(ChakraRTInterface::JsCreateExternalArrayBuffer((void*)this->initialSource.c_str(),
        (unsigned int)this->initialSource.size(), nullptr, nullptr, &scriptSource));


    ChakraRTInterface::JsCreateString(fullPath, strlen(fullPath), &fname);

    ChakraRTInterface::JsRun(scriptSource, WScriptJsrt::GetNextSourceContext(), fname, JsParseScriptAttributeNone, nullptr);

    SetEvent(this->parent->hevntInitialScriptCompleted);

    // loop waiting for work;

    while (true)
    {
        HANDLE handles[] = { this->hevntReceivedBroadcast, this->hevntShutdown };
        DWORD waitRet = WaitForMultipleObjects(_countof(handles), handles, false, INFINITE);

        if (waitRet == WAIT_OBJECT_0)
        {
            JsValueRef args[3];
            ChakraRTInterface::JsGetGlobalObject(&args[0]);
            ChakraRTInterface::JsCreateSharedArrayBufferWithSharedContent(this->parent->sharedContent, &args[1]);
            ChakraRTInterface::JsDoubleToNumber(1, &args[2]);

            // notify the parent we received the data
            ReleaseSemaphore(this->parent->hSemaphore, 1, NULL);

            if (this->receiveBroadcastCallbackFunc)
            {
                ChakraRTInterface::JsCallFunction(this->receiveBroadcastCallbackFunc, args, 3, nullptr);
            }
        }

        if (waitRet == WAIT_OBJECT_0 + 1 || this->leaving)
        {
            WScriptJsrt::Uninitialize();

            if (this->receiveBroadcastCallbackFunc)
            {
                ChakraRTInterface::JsRelease(this->receiveBroadcastCallbackFunc, nullptr);
            }
            ChakraRTInterface::JsSetCurrentContext(nullptr);
            ChakraRTInterface::JsDisposeRuntime(runtime);

            threadLocalData.Uninitialize();
            return 0;
        }
        else if (waitRet != WAIT_OBJECT_0)
        {
            Assert(false);
            break;
        }
    }

Error:

    ChakraRTInterface::JsSetCurrentContext(nullptr);
    ChakraRTInterface::JsDisposeRuntime(runtime);
    threadLocalData.Uninitialize();
    return 0;
}
