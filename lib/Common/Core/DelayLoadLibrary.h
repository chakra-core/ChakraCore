//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class DelayLoadLibrary
{
protected:
    HMODULE m_hModule;
    bool m_isInit;

public:
    DelayLoadLibrary();
    virtual ~DelayLoadLibrary();

    virtual LPCTSTR GetLibraryName() const = 0;

    FARPROC GetFunction(_In_ LPCSTR lpFunctionName);

    void EnsureFromSystemDirOnly();
    bool IsAvailable();
private:
    void Ensure(DWORD dwFlags = 0);

};

#if _WIN32

// This needs to be delay loaded because SetThreadDescription is available only
// on Win10 1607+
class Kernel32Library : protected DelayLoadLibrary
{
private:
    typedef HRESULT (WINAPI *PFnSetThreadDescription)(
      _In_ HANDLE hThread,
      _In_ PCWSTR lpThreadDescription
    );

    PFnSetThreadDescription setThreadDescription;

  public:
    static Kernel32Library* Instance;

    Kernel32Library() : DelayLoadLibrary(),
      setThreadDescription(NULL)
    {
        this->EnsureFromSystemDirOnly();
    }

    LPCTSTR GetLibraryName() const;

    HRESULT WINAPI SetThreadDescription(
        _In_ HANDLE hThread,
        _In_ PCWSTR lpThreadDescription
        );
};

// This needs to be delay loaded because it is available on
// Win8 only
class NtdllLibrary : protected DelayLoadLibrary
{
public:
    // needed for InitializeObjectAttributes
    static const ULONG OBJ_KERNEL_HANDLE = 0x00000200;
    static const ULONG MAP_PROCESS = 1;

    typedef struct _UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;

    typedef struct _OBJECT_ATTRIBUTES {
        ULONG           Length;
        HANDLE          RootDirectory;
        PUNICODE_STRING ObjectName;
        ULONG           Attributes;
        PVOID           SecurityDescriptor;
        PVOID           SecurityQualityOfService;
    }  OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

    typedef enum _SECTION_INHERIT {
        ViewShare = 1,
        ViewUnmap = 2
    } SECTION_INHERIT, *PSECTION_INHERIT;

    typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

private:
#if PDATA_ENABLED
    typedef _Success_(return == 0) DWORD (NTAPI *PFnRtlAddGrowableFunctionTable)(_Out_ PVOID * DynamicTable,
        _In_reads_(MaximumEntryCount) PRUNTIME_FUNCTION FunctionTable,
        _In_ DWORD EntryCount,
        _In_ DWORD MaximumEntryCount,
        _In_ ULONG_PTR RangeBase,
        _In_ ULONG_PTR RangeEnd);
    PFnRtlAddGrowableFunctionTable addGrowableFunctionTable;

    typedef VOID (NTAPI *PFnRtlDeleteGrowableFunctionTable)(_In_ PVOID DynamicTable);
    PFnRtlDeleteGrowableFunctionTable deleteGrowableFunctionTable;

    typedef VOID (NTAPI *PFnRtlGrowFunctionTable)(_Inout_ PVOID DynamicTable, _In_ ULONG NewEntryCount);
    PFnRtlGrowFunctionTable growFunctionTable;
#endif

    typedef NTSTATUS(NTAPI *PFnNtCreateSection)(
        _Out_    PHANDLE            SectionHandle,
        _In_     ACCESS_MASK        DesiredAccess,
        _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
        _In_opt_ PLARGE_INTEGER     MaximumSize,
        _In_     ULONG              SectionPageProtection,
        _In_     ULONG              AllocationAttributes,
        _In_opt_ HANDLE             FileHandle);
    PFnNtCreateSection createSection;

    typedef NTSTATUS(NTAPI *PFnNtMapViewOfSection)(
        _In_        HANDLE          SectionHandle,
        _In_        HANDLE          ProcessHandle,
        _Inout_     PVOID           *BaseAddress,
        _In_        ULONG_PTR       ZeroBits,
        _In_        SIZE_T          CommitSize,
        _Inout_opt_ PLARGE_INTEGER  SectionOffset,
        _Inout_     PSIZE_T         ViewSize,
        _In_        SECTION_INHERIT InheritDisposition,
        _In_        ULONG           AllocationType,
        _In_        ULONG           Win32Protect);
    PFnNtMapViewOfSection mapViewOfSection;

    typedef NTSTATUS(NTAPI *PFnNtUnmapViewOfSection)(
        _In_     HANDLE ProcessHandle,
        _In_opt_ PVOID  BaseAddress);
    PFnNtUnmapViewOfSection unmapViewOfSection;

    typedef NTSTATUS(NTAPI *PFnNtClose)(_In_ HANDLE Handle);
    PFnNtClose close;

    typedef NTSTATUS(NTAPI *PFnNtUnlockVirtualMemory)(
        _In_ HANDLE ProcessHandle,
        _Inout_ PVOID *BaseAddress,
        _Inout_ PSIZE_T RegionSize,
        _In_ ULONG MapType);
    PFnNtUnlockVirtualMemory unlock;

public:
    static NtdllLibrary* Instance;

    NtdllLibrary() : DelayLoadLibrary(),
#if PDATA_ENABLED
        addGrowableFunctionTable(NULL),
        deleteGrowableFunctionTable(NULL),
        growFunctionTable(NULL),
#endif
        createSection(NULL),
        mapViewOfSection(NULL),
        unmapViewOfSection(NULL),
        close(NULL),
        unlock(nullptr)
    {
        this->EnsureFromSystemDirOnly();
    }

    LPCTSTR GetLibraryName() const;

#if PDATA_ENABLED
    _Success_(return == 0)
    NTSTATUS AddGrowableFunctionTable(_Out_ PVOID * DynamicTable,
        _In_reads_(MaximumEntryCount) PRUNTIME_FUNCTION FunctionTable,
        _In_ DWORD EntryCount,
        _In_ DWORD MaximumEntryCount,
        _In_ ULONG_PTR RangeBase,
        _In_ ULONG_PTR RangeEnd);
    VOID DeleteGrowableFunctionTable(_In_ PVOID DynamicTable);
    VOID GrowFunctionTable(__inout PVOID DynamicTable, _In_ ULONG NewEntryCount);
#endif

    // we do not have the header where this macro is defined, so implement ourselves
    VOID InitializeObjectAttributes(
        POBJECT_ATTRIBUTES   InitializedAttributes,
        PUNICODE_STRING      ObjectName,
        ULONG                Attributes,
        HANDLE               RootDirectory,
        PSECURITY_DESCRIPTOR SecurityDescriptor
    );

    NTSTATUS CreateSection(
        _Out_    PHANDLE            SectionHandle,
        _In_     ACCESS_MASK        DesiredAccess,
        _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
        _In_opt_ PLARGE_INTEGER     MaximumSize,
        _In_     ULONG              SectionPageProtection,
        _In_     ULONG              AllocationAttributes,
        _In_opt_ HANDLE             FileHandle
    );

    NTSTATUS MapViewOfSection(
        _In_        HANDLE          SectionHandle,
        _In_        HANDLE          ProcessHandle,
        _Inout_     PVOID           *BaseAddress,
        _In_        ULONG_PTR       ZeroBits,
        _In_        SIZE_T          CommitSize,
        _Inout_opt_ PLARGE_INTEGER  SectionOffset,
        _Inout_     PSIZE_T         ViewSize,
        _In_        SECTION_INHERIT InheritDisposition,
        _In_        ULONG           AllocationType,
        _In_        ULONG           Win32Protect
    );

    NTSTATUS UnmapViewOfSection(
        _In_     HANDLE ProcessHandle,
        _In_opt_ PVOID  BaseAddress
    );

    NTSTATUS Close(
        _In_ HANDLE Handle
    );

    NTSTATUS UnlockVirtualMemory(
        _In_ HANDLE ProcessHandle,
        _Inout_ PVOID *BaseAddress,
        _Inout_ PSIZE_T RegionSize,
        _In_ ULONG MapType
    );
};

// This needs to be delay loaded because RpcServerRegisterIf3 is available only
// on Win8+
class RPCLibrary : protected DelayLoadLibrary
{
private:
    typedef RPC_STATUS(NTAPI* PFnRpcServerRegisterIf3)(
        _In_ RPC_IF_HANDLE IfSpec,
        _In_opt_ UUID* MgrTypeUuid,
        _In_opt_ RPC_MGR_EPV* MgrEpv,
        _In_ unsigned int Flags,
        _In_ unsigned int MaxCalls,
        _In_ unsigned int MaxRpcSize,
        _In_opt_ RPC_IF_CALLBACK_FN* IfCallback,
        _In_opt_ void* SecurityDescriptor);

    PFnRpcServerRegisterIf3 serverRegister;

public:
    static RPCLibrary* Instance;

    RPCLibrary() : DelayLoadLibrary(),
        serverRegister(nullptr)
    {
        this->EnsureFromSystemDirOnly();
    }

    LPCTSTR GetLibraryName() const;

    RPC_STATUS RpcServerRegisterIf3(
        _In_ RPC_IF_HANDLE IfSpec,
        _In_opt_ UUID* MgrTypeUuid,
        _In_opt_ RPC_MGR_EPV* MgrEpv,
        _In_ unsigned int Flags,
        _In_ unsigned int MaxCalls,
        _In_ unsigned int MaxRpcSize,
        _In_opt_ RPC_IF_CALLBACK_FN* IfCallback,
        _In_opt_ void* SecurityDescriptor
    );
};

#endif
