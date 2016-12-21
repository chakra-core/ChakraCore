//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
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


FARPROC DelayLoadLibrary::GetFunction(__in LPCSTR lpFunctionName)
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

static NtdllLibrary NtdllLibraryObject;
NtdllLibrary* NtdllLibrary::Instance = &NtdllLibraryObject;

LPCTSTR NtdllLibrary::GetLibraryName() const
{
    return _u("ntdll.dll");
}

#if PDATA_ENABLED

_Success_(return == 0)
DWORD NtdllLibrary::AddGrowableFunctionTable( _Out_ PVOID * DynamicTable,
    _In_reads_(MaximumEntryCount) PRUNTIME_FUNCTION FunctionTable,
    _In_ DWORD EntryCount,
    _In_ DWORD MaximumEntryCount,
    _In_ ULONG_PTR RangeBase,
    _In_ ULONG_PTR RangeEnd )
{
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
        return addGrowableFunctionTable(DynamicTable,
            FunctionTable,
            EntryCount,
            MaximumEntryCount,
            RangeBase,
            RangeEnd);
    }
    return 1;
}

VOID NtdllLibrary::DeleteGrowableFunctionTable( _In_ PVOID DynamicTable )
{
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

#endif // _WIN32
