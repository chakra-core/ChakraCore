//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Common.h"
#include "CommonInl.h" // TODO: Try to remove this later

#include "Core/ConfigParser.h"

bool ConfigParserAPI::FillConsoleTitle(__ecount(cchBufferSize) LPWSTR buffer, size_t cchBufferSize, __in LPWSTR moduleName)
{
    swprintf_s(buffer, cchBufferSize, _u("Chakra GC: %d - %s"), GetCurrentProcessId(), moduleName);

    return true;
}

void ConfigParserAPI::DisplayInitialOutput(__in LPWSTR moduleName)
{
    Output::Print(_u("Chakra GC\n"));
    Output::Print(_u("INIT: PID        : %d\n"), GetCurrentProcessId());
    Output::Print(_u("INIT: DLL Path   : %s\n"), moduleName);
}

#ifdef ENABLE_JS_ETW
void EtwCallbackApi::OnSessionChange(ULONG /* controlCode */, PVOID /* callbackContext */)
{
    // Does nothing
}
#endif

// Include this file got get the default behavior for JsUtil::ExternalApi functions.
void JsUtil::ExternalApi::RecoverUnusedMemory()
{
}

bool JsUtil::ExternalApi::RaiseOutOfMemoryIfScriptActive()
{
    return false;
}

bool JsUtil::ExternalApi::RaiseStackOverflowIfScriptActive(Js::ScriptContext * scriptContext, PVOID returnAddress)
{
    return false;
}

ThreadContextId JsUtil::ExternalApi::GetCurrentThreadContextId()
{
    return (ThreadContextId)::GetCurrentThreadId();
}

bool JsUtil::ExternalApi::RaiseOnIntOverflow()
{
    return false;
}

LPCWSTR JsUtil::ExternalApi::GetFeatureKeyName()
{
    return  _u("Software\\Microsoft\\Internet Explorer\\ChakraRecycler");
}

#if DBG || defined(EXCEPTION_CHECK)
BOOL JsUtil::ExternalApi::IsScriptActiveOnCurrentThreadContext()
{
    return false;
}
#endif


extern "C"
{
    bool IsMessageBoxWPresent()
    {
        return true;
    }
}

bool GetDeviceFamilyInfo(
    _Out_opt_ ULONGLONG* /*pullUAPInfo*/,
    _Out_opt_ ULONG* /*pulDeviceFamily*/,
    _Out_opt_ ULONG* /*pulDeviceForm*/);

void
ChakraBinaryAutoSystemInfoInit(AutoSystemInfo * autoSystemInfo)
{
    ULONGLONG UAPInfo;
    ULONG DeviceFamily;
    ULONG DeviceForm;
    if (GetDeviceFamilyInfo(&UAPInfo, &DeviceFamily, &DeviceForm))
    {
        bool isMobile = (DeviceFamily == 0x00000004 /*DEVICEFAMILYINFOENUM_MOBILE*/);
        autoSystemInfo->shouldQCMoreFrequently = isMobile;
        autoSystemInfo->supportsOnlyMultiThreadedCOM = isMobile;  //TODO: pick some other platform to the list
        autoSystemInfo->isLowMemoryDevice = isMobile;  //TODO: pick some other platform to the list
    }
}

enum MemProtectHeapCollectFlags {};
enum MemProtectHeapCreateFlags {};

HRESULT MemProtectHeapCreate(void ** heapHandle, MemProtectHeapCreateFlags flags) { return E_NOTIMPL; };
void * MemProtectHeapRootAlloc(void * heapHandle, size_t size) { return nullptr; };
void * MemProtectHeapRootAllocLeaf(void * heapHandle, size_t size) { return nullptr; };
HRESULT MemProtectHeapUnrootAndZero(void * heapHandle, void * memory) { return E_NOTIMPL; };
HRESULT MemProtectHeapMemSize(void * heapHandle, void * memory, size_t * outSize) { return E_NOTIMPL; };
HRESULT MemProtectHeapDestroy(void * heapHandle) { return E_NOTIMPL; };
HRESULT MemProtectHeapCollect(void * heapHandle, MemProtectHeapCollectFlags flags) { return E_NOTIMPL; };
HRESULT MemProtectHeapProtectCurrentThread(void * heapHandle, void(__stdcall* threadWake)(void* threadWakeArgument), void* threadWakeArgument) { return E_NOTIMPL; };
HRESULT MemProtectHeapUnprotectCurrentThread(void * heapHandle) { return E_NOTIMPL; };
HRESULT MemProtectHeapSynchronizeWithCollector(void * heapHandle) { return E_NOTIMPL; };

#if DBG && defined(INTERNAL_MEM_PROTECT_HEAP_ALLOC)
void MemProtectHeapSetDisableConcurrentThreadExitedCheck(void * heapHandle) {};
#endif

#if DBG && defined(RECYCLER_VERIFY_MARK)
bool IsLikelyRuntimeFalseReference(char* objectStartAddress, size_t offset,
    const char* typeName)
{
    return false;
}
#endif
