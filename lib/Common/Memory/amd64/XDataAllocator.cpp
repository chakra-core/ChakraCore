//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

// This one works only for x64
#if !defined(_M_X64)
CompileAssert(false)
#endif

#include "XDataAllocator.h"
#include "Core/DelayLoadLibrary.h"

#ifndef _WIN32
#include "PlatformAgnostic/AssemblyCommon.h" // __REGISTER_FRAME / __DEREGISTER_FRAME
#endif

XDataAllocator::XDataAllocator(BYTE* address, uint size) :
    freeList(nullptr),
    start(address),
    current(address),
    size(size)
{
    Assert(size > 0);
    Assert(address != nullptr);
}

bool XDataAllocator::Initialize(void* segmentStart, void* segmentEnd)
{
    Assert(segmentEnd > segmentStart);
    return true;
}

XDataAllocator::~XDataAllocator()
{
    current = nullptr;
    ClearFreeList();
}

void XDataAllocator::Delete()
{
    HeapDelete(this);
}

bool XDataAllocator::Alloc(ULONG_PTR functionStart, DWORD functionSize,
    ushort pdataCount, ushort xdataSize, SecondaryAllocation* allocation)
{
    XDataAllocation* xdata = static_cast<XDataAllocation*>(allocation);
    Assert(start != nullptr);
    Assert(current != nullptr);
    Assert(current >= start);
    Assert(xdataSize <= XDATA_SIZE);
    Assert(pdataCount == 1);

    // Allocate a new xdata entry
    if((End() - current) >= XDATA_SIZE)
    {
        xdata->address = current;
        current += XDATA_SIZE;
    } // try allocating from the free list
    else if(freeList)
    {
        auto entry = freeList;
        xdata->address = entry->address;
        this->freeList = entry->next;
        HeapDelete(entry);
    }
    else
    {
        xdata->address = nullptr;
        OUTPUT_TRACE(Js::XDataAllocatorPhase, _u("No space for XDATA.\n"));
    }

#ifndef _WIN32
    if (xdata->address)
    {
        ClearHead(xdata->address);  // mark empty .eh_frame
    }
#endif

    return xdata->address != nullptr;
}

void XDataAllocator::Release(const SecondaryAllocation& allocation)
{
    const XDataAllocation& xdata = static_cast<const XDataAllocation&>(allocation);
    Assert(allocation.address);
    // Add it to free list
    auto freed = HeapNewNoThrowStruct(XDataAllocationEntry);
    if(freed)
    {
        freed->address = xdata.address;
        freed->next = this->freeList;
        this->freeList = freed;
    }
}

bool XDataAllocator::CanAllocate()
{
    return ((End() - current) >= XDATA_SIZE) || this->freeList;
}

void XDataAllocator::ClearFreeList()
{
    XDataAllocationEntry* next = this->freeList;
    XDataAllocationEntry* entry;
    while(next)
    {
        entry = next;
        next = entry->next;
        entry->address = nullptr;
        HeapDelete(entry);
    }
    this->freeList = NULL;
}

/* static */
void XDataAllocator::Register(XDataAllocation * xdataInfo, ULONG_PTR functionStart, DWORD functionSize)
{
#ifdef _WIN32
    ULONG_PTR baseAddress = functionStart;
    xdataInfo->pdata.BeginAddress = (DWORD)(functionStart - baseAddress);
    xdataInfo->pdata.EndAddress = (DWORD)(xdataInfo->pdata.BeginAddress + functionSize);
    xdataInfo->pdata.UnwindInfoAddress = (DWORD)((intptr_t)xdataInfo->address - baseAddress);

    BOOLEAN success = FALSE;
    if (AutoSystemInfo::Data.IsWin8OrLater())
    {
        DWORD status = NtdllLibrary::Instance->AddGrowableFunctionTable(&xdataInfo->functionTable,
            &xdataInfo->pdata,
            /*MaxEntryCount*/ 1,
            /*Valid entry count*/ 1,
            /*RangeBase*/ functionStart,
            /*RangeEnd*/ functionStart + functionSize);
        success = NT_SUCCESS(status);
        if (success)
        {
            Assert(xdataInfo->functionTable != nullptr);
        }
    }
    else
    {
        success = RtlAddFunctionTable(&xdataInfo->pdata, 1, functionStart);
    }
    Js::Throw::CheckAndThrowOutOfMemory(success);

#if DBG
    // Validate that the PDATA registration succeeded
    ULONG64            imageBase = 0;
    RUNTIME_FUNCTION  *runtimeFunction = RtlLookupFunctionEntry((DWORD64)functionStart, &imageBase, nullptr);
    Assert(runtimeFunction != NULL);
#endif

#else  // !_WIN32
    Assert(ReadHead(xdataInfo->address));  // should be non-empty .eh_frame
    __REGISTER_FRAME(xdataInfo->address);
#endif
}

/* static */
void XDataAllocator::Unregister(XDataAllocation * xdataInfo)
{
#ifdef _WIN32
    // Delete the table
    if (AutoSystemInfo::Data.IsWin8OrLater())
    {
        NtdllLibrary::Instance->DeleteGrowableFunctionTable(xdataInfo->functionTable);
    }
    else
    {
        BOOLEAN success = RtlDeleteFunctionTable(&xdataInfo->pdata);
        Assert(success);
    }

#else  // !_WIN32
    Assert(ReadHead(xdataInfo->address));  // should be non-empty .eh_frame
    __DEREGISTER_FRAME(xdataInfo->address);
#endif
}
