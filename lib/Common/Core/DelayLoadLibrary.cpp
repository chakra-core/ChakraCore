//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"
#include "Core/DelayLoadLibrary.h"

DelayLoadLibrary::DelayLoadLibrary()
{
    m_hModule = nullptr;
    m_isInit = false;
}

DelayLoadLibrary::~DelayLoadLibrary()
{
    if (m_hModule)
    {
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
    }
}

void DelayLoadLibrary::Ensure(DWORD dwFlags)
{
    if (!m_isInit)
    {
        m_hModule = LoadLibraryEx(GetLibraryName(), nullptr, dwFlags);
        m_isInit = true;
    }
}

void DelayLoadLibrary::EnsureFromSystemDirOnly()
{
    Ensure(LOAD_LIBRARY_SEARCH_SYSTEM32);
}


FARPROC DelayLoadLibrary::GetFunction(_In_ LPCSTR lpFunctionName)
{
    if (m_hModule)
    {
        return GetProcAddress(m_hModule, lpFunctionName);
    }

    return nullptr;
}

bool DelayLoadLibrary::IsAvailable()
{
    return m_hModule != nullptr;
}

#if _WIN32

static Kernel32Library Kernel32LibraryObject;
Kernel32Library* Kernel32Library::Instance = &Kernel32LibraryObject;

LPCTSTR Kernel32Library::GetLibraryName() const
{
    return _u("kernel32.dll");
}

HRESULT Kernel32Library::SetThreadDescription(
    _In_ HANDLE hThread,
    _In_ PCWSTR lpThreadDescription
)
{
    if (m_hModule)
    {
        if (setThreadDescription == nullptr)
        {
            setThreadDescription = (PFnSetThreadDescription)GetFunction("SetThreadDescription");
            if (setThreadDescription == nullptr)
            {
                return S_FALSE;
            }
        }
        return setThreadDescription(hThread, lpThreadDescription);
    }

  return S_FALSE;
}

static NtdllLibrary NtdllLibraryObject;
NtdllLibrary* NtdllLibrary::Instance = &NtdllLibraryObject;

LPCTSTR NtdllLibrary::GetLibraryName() const
{
    return _u("ntdll.dll");
}

#if PDATA_ENABLED

_Success_(return == 0)
NtdllLibrary::NTSTATUS NtdllLibrary::AddGrowableFunctionTable( _Out_ PVOID * DynamicTable,
    _In_reads_(MaximumEntryCount) PRUNTIME_FUNCTION FunctionTable,
    _In_ DWORD EntryCount,
    _In_ DWORD MaximumEntryCount,
    _In_ ULONG_PTR RangeBase,
    _In_ ULONG_PTR RangeEnd )
{
    Assert(AutoSystemInfo::Data.IsWin8OrLater());
    if(m_hModule)
    {
        if(addGrowableFunctionTable == NULL)
        {
            addGrowableFunctionTable = (PFnRtlAddGrowableFunctionTable)GetFunction("RtlAddGrowableFunctionTable");
            if(addGrowableFunctionTable == NULL)
            {
                Assert(false);
                return 1;
            }
        }

#if DBG
        // Validate the PDATA was not registered or already unregistered
        ULONG_PTR            imageBase = 0;
        RUNTIME_FUNCTION  *runtimeFunction = RtlLookupFunctionEntry((DWORD64)RangeBase, &imageBase, nullptr);
        Assert(runtimeFunction == NULL);
#endif

        *DynamicTable = nullptr;
        NTSTATUS status = addGrowableFunctionTable(DynamicTable,
            FunctionTable,
            EntryCount,
            MaximumEntryCount,
            RangeBase,
            RangeEnd);
#if _M_X64
        PHASE_PRINT_TESTTRACE1(Js::XDataPhase, _u("[%d]Register: Begin: %llx, End: %x, Unwind: %llx, RangeBase: %llx, RangeEnd: %llx, table: %llx, Status: %x\n"),
           GetCurrentThreadId(), FunctionTable->BeginAddress, FunctionTable->EndAddress, FunctionTable->UnwindInfoAddress, RangeBase, RangeEnd, *DynamicTable, status);
#endif
        Assert((status >= 0 && *DynamicTable != nullptr) || status == 0xC000009A /*STATUS_INSUFFICIENT_RESOURCES*/);
        return status;
    }
    return 1;
}

VOID NtdllLibrary::DeleteGrowableFunctionTable( _In_ PVOID DynamicTable )
{
    Assert(AutoSystemInfo::Data.IsWin8OrLater());
    if(m_hModule)
    {
        if(deleteGrowableFunctionTable == NULL)
        {
            deleteGrowableFunctionTable = (PFnRtlDeleteGrowableFunctionTable)GetFunction("RtlDeleteGrowableFunctionTable");
            if(deleteGrowableFunctionTable == NULL)
            {
                Assert(false);
                return;
            }
        }
        deleteGrowableFunctionTable(DynamicTable);

        PHASE_PRINT_TESTTRACE1(Js::XDataPhase, _u("[%d]UnRegister: table: %llx\n"), GetCurrentThreadId(), DynamicTable);
    }
}

VOID NtdllLibrary::GrowFunctionTable(_Inout_ PVOID DynamicTable, _In_ ULONG NewEntryCount)
{
    if (m_hModule)
    {
        if (growFunctionTable == nullptr)
        {
            growFunctionTable = (PFnRtlGrowFunctionTable)GetFunction("RtlGrowFunctionTable");
            if (growFunctionTable == nullptr)
            {
                Assert(false);
                return;
            }
        }

        growFunctionTable(DynamicTable, NewEntryCount);
    }
}
#endif // PDATA_ENABLED

VOID NtdllLibrary::InitializeObjectAttributes(
    POBJECT_ATTRIBUTES   InitializedAttributes,
    PUNICODE_STRING      ObjectName,
    ULONG                Attributes,
    HANDLE               RootDirectory,
    PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    InitializedAttributes->Length = sizeof(OBJECT_ATTRIBUTES);
    InitializedAttributes->RootDirectory = RootDirectory;
    InitializedAttributes->Attributes = Attributes;
    InitializedAttributes->ObjectName = ObjectName;
    InitializedAttributes->SecurityDescriptor = SecurityDescriptor;
    InitializedAttributes->SecurityQualityOfService = NULL;
}

#ifndef DELAYLOAD_SECTIONAPI
extern "C"
WINBASEAPI
NtdllLibrary::NTSTATUS
WINAPI
NtCreateSection(
    _Out_    PHANDLE            SectionHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ NtdllLibrary::POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PLARGE_INTEGER     MaximumSize,
    _In_     ULONG              SectionPageProtection,
    _In_     ULONG              AllocationAttributes,
    _In_opt_ HANDLE             FileHandle
);
#endif

NtdllLibrary::NTSTATUS NtdllLibrary::CreateSection(
    _Out_    PHANDLE            SectionHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PLARGE_INTEGER     MaximumSize,
    _In_     ULONG              SectionPageProtection,
    _In_     ULONG              AllocationAttributes,
    _In_opt_ HANDLE             FileHandle)
{
#ifdef DELAYLOAD_SECTIONAPI
    if (m_hModule)
    {
        if (createSection == nullptr)
        {
            createSection = (PFnNtCreateSection)GetFunction("NtCreateSection");
            if (createSection == nullptr)
            {
                Assert(false);
                SectionHandle = nullptr;
                return -1;
            }
        }
        return createSection(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    }
    SectionHandle = nullptr;
    return -1;
#else
    return NtCreateSection(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
#endif
}

#ifndef DELAYLOAD_SECTIONAPI
extern "C"
WINBASEAPI
NtdllLibrary::NTSTATUS
WINAPI
NtMapViewOfSection(
    _In_        HANDLE          SectionHandle,
    _In_        HANDLE          ProcessHandle,
    _Inout_     PVOID           *BaseAddress,
    _In_        ULONG_PTR       ZeroBits,
    _In_        SIZE_T          CommitSize,
    _Inout_opt_ PLARGE_INTEGER  SectionOffset,
    _Inout_     PSIZE_T         ViewSize,
    _In_        NtdllLibrary::SECTION_INHERIT InheritDisposition,
    _In_        ULONG           AllocationType,
    _In_        ULONG           Win32Protect
);
#endif

NtdllLibrary::NTSTATUS NtdllLibrary::MapViewOfSection(
    _In_        HANDLE          SectionHandle,
    _In_        HANDLE          ProcessHandle,
    _Inout_     PVOID           *BaseAddress,
    _In_        ULONG_PTR       ZeroBits,
    _In_        SIZE_T          CommitSize,
    _Inout_opt_ PLARGE_INTEGER  SectionOffset,
    _Inout_     PSIZE_T         ViewSize,
    _In_        SECTION_INHERIT InheritDisposition,
    _In_        ULONG           AllocationType,
    _In_        ULONG           Win32Protect)
{
#ifdef DELAYLOAD_SECTIONAPI
    if (m_hModule)
    {
        if (mapViewOfSection == nullptr)
        {
            mapViewOfSection = (PFnNtMapViewOfSection)GetFunction("NtMapViewOfSection");
            if (mapViewOfSection == nullptr)
            {
                Assert(false);
                return -1;
            }
        }
        return mapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    }
    return -1;
#else
    return NtMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
#endif
}

#ifndef DELAYLOAD_SECTIONAPI
extern "C"
WINBASEAPI
NtdllLibrary::NTSTATUS
WINAPI
NtUnmapViewOfSection(
    _In_     HANDLE ProcessHandle,
    _In_opt_ PVOID  BaseAddress
);
#endif

NtdllLibrary::NTSTATUS NtdllLibrary::UnmapViewOfSection(
    _In_     HANDLE ProcessHandle,
    _In_opt_ PVOID  BaseAddress)
{
#ifdef DELAYLOAD_SECTIONAPI
    if (m_hModule)
    {
        if (unmapViewOfSection == nullptr)
        {
            unmapViewOfSection = (PFnNtUnmapViewOfSection)GetFunction("NtUnmapViewOfSection");
            if (unmapViewOfSection == nullptr)
            {
                Assert(false);
                return -1;
            }
        }
        return unmapViewOfSection(ProcessHandle, BaseAddress);
    }
    return -1;
#else
    return NtUnmapViewOfSection(ProcessHandle, BaseAddress);
#endif
}

#ifndef DELAYLOAD_SECTIONAPI
extern "C"
WINBASEAPI
NtdllLibrary::NTSTATUS
WINAPI
NtClose(_In_ HANDLE Handle);
#endif

NtdllLibrary::NTSTATUS NtdllLibrary::Close(_In_ HANDLE Handle)
{
#ifdef DELAYLOAD_SECTIONAPI
    if (m_hModule)
    {
        if (close == nullptr)
        {
            close = (PFnNtClose)GetFunction("NtClose");
            if (close == nullptr)
            {
                Assert(false);
                return -1;
            }
        }
        return close(Handle);
    }
    return -1;
#else
    return NtClose(Handle);
#endif
}

#ifndef DELAYLOAD_UNLOCKMEMORY
extern "C"
WINBASEAPI
NtdllLibrary::NTSTATUS
WINAPI
NtUnlockVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG MapType
);
#endif

NtdllLibrary::NTSTATUS NtdllLibrary::UnlockVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG MapType)
{
#ifdef DELAYLOAD_UNLOCKMEMORY
    if (m_hModule)
    {
        if (unlock == nullptr)
        {
            unlock = (PFnNtUnlockVirtualMemory)GetFunction("NtUnlockVirtualMemory");
            if (unlock == nullptr)
            {
                Assert(false);
                return -1;
            }
        }
        return unlock(ProcessHandle, BaseAddress, RegionSize, MapType);
    }
    return -1;
#else
    return NtUnlockVirtualMemory(ProcessHandle, BaseAddress, RegionSize, MapType);
#endif
}

static RPCLibrary RPCLibraryObject;
RPCLibrary* RPCLibrary::Instance = &RPCLibraryObject;

LPCTSTR RPCLibrary::GetLibraryName() const
{
    return _u("rpcrt4.dll");
}

RPC_STATUS RPCLibrary::RpcServerRegisterIf3(
    _In_ RPC_IF_HANDLE IfSpec,
    _In_opt_ UUID* MgrTypeUuid,
    _In_opt_ RPC_MGR_EPV* MgrEpv,
    _In_ unsigned int Flags,
    _In_ unsigned int MaxCalls,
    _In_ unsigned int MaxRpcSize,
    _In_opt_ RPC_IF_CALLBACK_FN* IfCallback,
    _In_opt_ void* SecurityDescriptor)
{
#if !(NTDDI_VERSION >= NTDDI_WIN8)
    if (m_hModule)
    {
        if (serverRegister == nullptr)
        {
            serverRegister = (PFnRpcServerRegisterIf3)GetFunction("RpcServerRegisterIf3");
            if (serverRegister == nullptr)
            {
                Assert(false);
                return -1;
            }
        }
        return serverRegister(IfSpec, MgrTypeUuid, MgrEpv, Flags, MaxCalls, MaxRpcSize, IfCallback, SecurityDescriptor);
    }
    return -1;
#else
    return ::RpcServerRegisterIf3(IfSpec, MgrTypeUuid, MgrEpv, Flags, MaxCalls, MaxRpcSize, IfCallback, SecurityDescriptor);
#endif
}
#endif // _WIN32
