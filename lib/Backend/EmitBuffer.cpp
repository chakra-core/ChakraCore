//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

//----------------------------------------------------------------------------
// EmitBufferManager::EmitBufferManager
//      Constructor
//----------------------------------------------------------------------------
template <typename TAlloc, typename TPreReservedAlloc, typename SyncObject>
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::EmitBufferManager(ArenaAllocator * allocator, CustomHeap::CodePageAllocators<TAlloc, TPreReservedAlloc> * codePageAllocators,
    Js::ScriptContext * scriptContext, LPCWSTR name, HANDLE processHandle) :
    allocationHeap(allocator, codePageAllocators, processHandle),
    allocator(allocator),
    allocations(nullptr),
    scriptContext(scriptContext),
    processHandle(processHandle)
{
#if DBG_DUMP
    this->totalBytesCode = 0;
    this->totalBytesLoopBody = 0;
    this->totalBytesAlignment = 0;
    this->totalBytesCommitted = 0;
    this->totalBytesReserved = 0;
    this->name = name;
#endif
}

//----------------------------------------------------------------------------
// EmitBufferManager::~EmitBufferManager()
//      Free up all the VirtualAlloced memory
//----------------------------------------------------------------------------
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::~EmitBufferManager()
{
    Clear();
}

template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
void
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::Decommit()
{
    FreeAllocations(false);
}
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
void
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::Clear()
{
    FreeAllocations(true);
}

template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
void
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::FreeAllocations(bool release)
{
    AutoRealOrFakeCriticalSection<SyncObject> autoCs(&this->criticalSection);

#if DBG_DUMP
    if (!release && PHASE_STATS1(Js::EmitterPhase))
    {
        this->DumpAndResetStats(Js::Configuration::Global.flags.Filename);
    }
#endif

    TEmitBufferAllocation * allocation = this->allocations;
    while (allocation != nullptr)
    {
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if(CONFIG_FLAG(CheckEmitBufferPermissions))
        {
            CheckBufferPermissions(allocation);
        }
#endif
        if (release)
        {
            this->allocationHeap.Free(allocation->allocation);
        }
        else if ((scriptContext != nullptr) && allocation->recorded)
        {
            // In case of ThunkEmitter the script context would be null and we don't want to track that as code size.
            this->scriptContext->GetThreadContext()->SubCodeSize(allocation->bytesCommitted);
            allocation->recorded = false;
        }

        allocation = allocation->nextAllocation;
    }
    if (release)
    {
        this->allocations = nullptr;
    }
    else
    {
        this->allocationHeap.DecommitAll();
    }
}

template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::IsInHeap(__in void* address)
{
    AutoRealOrFakeCriticalSection<SyncObject> autocs(&this->criticalSection);
    return this->allocationHeap.IsInHeap(address);
}

template <typename TAlloc, typename TPreReservedAlloc>
class AutoCustomHeapPointer
{
public:
    AutoCustomHeapPointer(CustomHeap::Heap<TAlloc, TPreReservedAlloc> * allocationHeap, CustomHeap::Allocation* heapAllocation) :
        _allocationHeap(allocationHeap),
        _heapAllocation(heapAllocation)
    {
    }

    ~AutoCustomHeapPointer()
    {
        if (_heapAllocation)
        {
            _allocationHeap->Free(_heapAllocation);
        }
    }

    CustomHeap::Allocation* Detach()
    {
        CustomHeap::Allocation* allocation = _heapAllocation;
        Assert(allocation != nullptr);
        _heapAllocation = nullptr;
        return allocation;
    }

private:
    CustomHeap::Allocation* _heapAllocation;
    CustomHeap::Heap<TAlloc, TPreReservedAlloc>* _allocationHeap;
};

//----------------------------------------------------------------------------
// EmitBufferManager::NewAllocation
//      Create a new allocation
//----------------------------------------------------------------------------
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
EmitBufferAllocation<TAlloc, TPreReservedAlloc> *
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::NewAllocation(size_t bytes, ushort pdataCount, ushort xdataSize, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode)
{
    FAULTINJECT_MEMORY_THROW(_u("JIT"), bytes);

    Assert(this->criticalSection.IsLocked());

    bool isAllJITCodeInPreReservedRegion = true;
    CustomHeap::Allocation* heapAllocation = this->allocationHeap.Alloc(bytes, pdataCount, xdataSize, canAllocInPreReservedHeapPageSegment, isAnyJittedCode, &isAllJITCodeInPreReservedRegion);

    if (heapAllocation  == nullptr)
    {
        if (!JITManager::GetJITManager()->IsJITServer())
        {
            // This is used in interpreter scenario, thus we need to try to recover memory, if possible.
            // Can't simply throw as in JIT scenario, for which throw is what we want in order to give more mem to interpreter.
            JsUtil::ExternalApi::RecoverUnusedMemory();
            heapAllocation = this->allocationHeap.Alloc(bytes, pdataCount, xdataSize, canAllocInPreReservedHeapPageSegment, isAnyJittedCode, &isAllJITCodeInPreReservedRegion);
        }
    }

    if (heapAllocation  == nullptr)
    {
        Js::Throw::OutOfMemory();
    }

#if DBG
    heapAllocation->isAllocationUsed = true;
#endif

    AutoCustomHeapPointer<TAlloc, TPreReservedAlloc> allocatedMemory(&this->allocationHeap, heapAllocation);
    VerboseHeapTrace(_u("New allocation: 0x%p, size: %p\n"), heapAllocation->address, heapAllocation->size);
    TEmitBufferAllocation * allocation = AnewStruct(this->allocator, TEmitBufferAllocation);

    allocation->bytesCommitted = heapAllocation->size;
    allocation->allocation = allocatedMemory.Detach();
    allocation->bytesUsed = 0;
    allocation->nextAllocation = this->allocations;
    allocation->recorded = false;
    allocation->inPrereservedRegion = isAllJITCodeInPreReservedRegion;

    this->allocations = allocation;

#if DBG_DUMP
    this->totalBytesCommitted += heapAllocation->size;
#endif

    return allocation;
}

template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::FreeAllocation(void* address)
{
    AutoRealOrFakeCriticalSection<SyncObject> autoCs(&this->criticalSection);

    TEmitBufferAllocation* previous = nullptr;
    TEmitBufferAllocation* allocation = allocations;
    while(allocation != nullptr)
    {
        if (address >= allocation->allocation->address && address < (allocation->allocation->address + allocation->bytesUsed))
        {
            if (previous == nullptr)
            {
                this->allocations = allocation->nextAllocation;
            }
            else
            {
                previous->nextAllocation = allocation->nextAllocation;
            }

            if ((scriptContext != nullptr) && allocation->recorded)
            {
                this->scriptContext->GetThreadContext()->SubCodeSize(allocation->bytesCommitted);
            }

            VerboseHeapTrace(_u("Freeing 0x%p, allocation: 0x%p\n"), address, allocation->allocation->address);

            this->allocationHeap.Free(allocation->allocation);
            this->allocator->Free(allocation, sizeof(TEmitBufferAllocation));

            return true;
        }
        previous = allocation;
        allocation = allocation->nextAllocation;
    }
    return false;
}

//----------------------------------------------------------------------------
// EmitBufferManager::FinalizeAllocation
//      Fill the rest of the page with debugger breakpoint.
//----------------------------------------------------------------------------
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::FinalizeAllocation(TEmitBufferAllocation *allocation, BYTE * dstBuffer)
{
    Assert(this->criticalSection.IsLocked());

    DWORD bytes = allocation->BytesFree();
    if(bytes > 0)
    {
        BYTE* buffer = nullptr;
        this->GetBuffer(allocation, bytes, &buffer);
        if (!this->CommitBuffer(allocation, dstBuffer, 0, /*sourceBuffer=*/ nullptr, /*alignPad=*/ bytes))
        {
            return false;
        }

#if DBG_DUMP
        this->totalBytesCode -= bytes;
#endif
    }

    return true;
}

template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
EmitBufferAllocation<TAlloc, TPreReservedAlloc>*
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::GetBuffer(TEmitBufferAllocation *allocation, __in size_t bytes, __deref_bcount(bytes) BYTE** ppBuffer)
{
    Assert(this->criticalSection.IsLocked());

    Assert(allocation->BytesFree() >= bytes);

    // In case of ThunkEmitter the script context would be null and we don't want to track that as code size.
    if (scriptContext && !allocation->recorded)
    {
        this->scriptContext->GetThreadContext()->AddCodeSize(allocation->bytesCommitted);
        allocation->recorded = true;
    }

    // The codegen buffer is beyond the alignment section - hence, we pass this pointer.
    *ppBuffer = allocation->GetUnused();
    return allocation;
}

//----------------------------------------------------------------------------
// EmitBufferManager::Allocate
//      Allocates an executable buffer with a certain alignment
//      NOTE: This buffer is not readable or writable. Use CommitBuffer
//      to modify this buffer one page at a time.
//----------------------------------------------------------------------------
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
EmitBufferAllocation<TAlloc, TPreReservedAlloc>*
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::AllocateBuffer(__in size_t bytes, __deref_bcount(bytes) BYTE** ppBuffer, ushort pdataCount /*=0*/, ushort xdataSize  /*=0*/, bool canAllocInPreReservedHeapPageSegment /*=false*/,
    bool isAnyJittedCode /* = false*/)
{
    AutoRealOrFakeCriticalSection<SyncObject> autoCs(&this->criticalSection);

    Assert(ppBuffer != nullptr);

    TEmitBufferAllocation * allocation = this->NewAllocation(bytes, pdataCount, xdataSize, canAllocInPreReservedHeapPageSegment, isAnyJittedCode);

    GetBuffer(allocation, bytes, ppBuffer);

#if DBG
    MEMORY_BASIC_INFORMATION memBasicInfo;
    size_t resultBytes = VirtualQueryEx(this->processHandle, allocation->allocation->address, &memBasicInfo, sizeof(memBasicInfo));
    Assert(resultBytes == 0 || memBasicInfo.Protect == PAGE_EXECUTE_READ);
#endif

    return allocation;
}

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::CheckCommitFaultInjection()
{
    if (Js::Configuration::Global.flags.ForceOOMOnEBCommit == 0)
    {
        return false;
    }

    commitCount++;

    if (Js::Configuration::Global.flags.ForceOOMOnEBCommit == -1)
    {
        Output::Print(_u("Commit count: %d\n"), commitCount);
    }
    else if (commitCount == Js::Configuration::Global.flags.ForceOOMOnEBCommit)
    {
        return true;
    }

    return false;
}

#endif

#if DBG
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::IsBufferExecuteReadOnly(TEmitBufferAllocation * allocation)
{
    AutoRealOrFakeCriticalSection<SyncObject> autoCs(&this->criticalSection);
    MEMORY_BASIC_INFORMATION memBasicInfo;
    size_t resultBytes = VirtualQuery(allocation->allocation->address, &memBasicInfo, sizeof(memBasicInfo));
    return resultBytes != 0 && memBasicInfo.Protect == PAGE_EXECUTE_READ;
}
#endif

template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::ProtectBufferWithExecuteReadWriteForInterpreter(TEmitBufferAllocation* allocation)
{
    Assert(this->criticalSection.IsLocked());
    Assert(allocation != nullptr);
    return (this->allocationHeap.ProtectAllocationWithExecuteReadWrite(allocation->allocation) == TRUE);
}

// Returns true if we successfully commit the buffer
// Returns false if we OOM
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::CommitBufferForInterpreter(TEmitBufferAllocation* allocation, _In_reads_bytes_(bufferSize) BYTE* pBuffer, _In_ size_t bufferSize)
{
    Assert(this->criticalSection.IsLocked());

    Assert(allocation != nullptr);
    allocation->bytesUsed += bufferSize;
#ifdef DEBUG
    this->totalBytesCode += bufferSize;
#endif

    VerboseHeapTrace(_u("Setting execute permissions on 0x%p, allocation: 0x%p\n"), pBuffer, allocation->allocation->address);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (CheckCommitFaultInjection())
    {
        return false;
    }
#endif

    if (!JITManager::GetJITManager()->IsJITServer() && !this->allocationHeap.ProtectAllocationWithExecuteReadOnly(allocation->allocation))
    {
        return false;
    }

    FlushInstructionCache(this->processHandle, pBuffer, bufferSize);

    return true;
}

//----------------------------------------------------------------------------
// EmitBufferManager::CommitBuffer
//      Aligns the buffer with DEBUG instructions.
//      Copies contents of source buffer to the destination buffer - at max of one page at a time.
//      This ensures that only 1 page is writable at any point of time.
//      Commit a buffer from the last AllocateBuffer call that is filled.
//----------------------------------------------------------------------------
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
bool
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::CommitBuffer(TEmitBufferAllocation* allocation, __out_bcount(bytes) BYTE* destBuffer, __in size_t bytes, __in_bcount(bytes) const BYTE* sourceBuffer, __in DWORD alignPad)
{
    AutoRealOrFakeCriticalSection<SyncObject> autoCs(&this->criticalSection);

    Assert(destBuffer != nullptr);
    Assert(allocation != nullptr);

    BYTE *currentDestBuffer = destBuffer + allocation->GetBytesUsed();
    char *bufferToFlush = allocation->allocation->address + allocation->GetBytesUsed();
    Assert(allocation->BytesFree() >= bytes + alignPad);

    size_t bytesLeft = bytes + alignPad;
    size_t sizeToFlush = bytesLeft;

    // Copy the contents and set the alignment pad
    while(bytesLeft != 0)
    {
        DWORD spaceInCurrentPage = AutoSystemInfo::PageSize - ((size_t)currentDestBuffer & (AutoSystemInfo::PageSize - 1));
        size_t bytesToChange = bytesLeft > spaceInCurrentPage ? spaceInCurrentPage : bytesLeft;


        // Buffer and the bytes that are marked RWX - these will eventually be marked as 'EXCEUTE' only.
        BYTE* readWriteBuffer = currentDestBuffer;
        size_t readWriteBytes = bytesToChange;

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (CheckCommitFaultInjection())
        {
            return false;
        }
#endif
        if (!JITManager::GetJITManager()->IsJITServer() && !this->allocationHeap.ProtectAllocationWithExecuteReadWrite(allocation->allocation, (char*)readWriteBuffer))
        {
            return false;
        }

        if (alignPad != 0)
        {
            DWORD alignBytes = alignPad < spaceInCurrentPage ? alignPad : spaceInCurrentPage;
            CustomHeap::FillDebugBreak(currentDestBuffer, alignBytes);

            alignPad -= alignBytes;
            currentDestBuffer += alignBytes;
            allocation->bytesUsed += alignBytes;
            bytesLeft -= alignBytes;
            bytesToChange -= alignBytes;

#if DBG_DUMP
            this->totalBytesAlignment += alignBytes;
#endif
        }

        // If there are bytes still left to be copied then we should do the copy.
        if(bytesToChange > 0)
        {
            AssertMsg(alignPad == 0, "If we are copying right now - we should be done with setting alignment.");

            memcpy_s(currentDestBuffer, allocation->BytesFree(), sourceBuffer, bytesToChange);

            currentDestBuffer += bytesToChange;
            sourceBuffer += bytesToChange;
            allocation->bytesUsed += bytesToChange;
            bytesLeft -= bytesToChange;
        }

        Assert(readWriteBuffer + readWriteBytes == currentDestBuffer);

        if (!JITManager::GetJITManager()->IsJITServer() && !this->allocationHeap.ProtectAllocationWithExecuteReadOnly(allocation->allocation, (char*)readWriteBuffer))
        {
            return false;
        }
    }

    FlushInstructionCache(this->processHandle, bufferToFlush, sizeToFlush);
#if DBG_DUMP
    this->totalBytesCode += bytes;
#endif

    //Finish the current EmitBufferAllocation
    return FinalizeAllocation(allocation, destBuffer);
}

template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
void
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::CompletePreviousAllocation(TEmitBufferAllocation* allocation)
{
    AutoRealOrFakeCriticalSection<SyncObject> autoCs(&this->criticalSection);
    if (allocation != nullptr)
    {
        allocation->bytesUsed = allocation->bytesCommitted;
    }
}

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
void
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::CheckBufferPermissions(TEmitBufferAllocation *allocation)
{
    AutoRealOrFakeCriticalSection<SyncObject> autoCs(&this->criticalSection);

    if(allocation->bytesCommitted == 0)
        return;

    MEMORY_BASIC_INFORMATION memInfo;

    BYTE *buffer = (BYTE*) allocation->allocation->address;
    SIZE_T size = allocation->bytesCommitted;

    while(1)
    {
        SIZE_T result = VirtualQuery(buffer, &memInfo, sizeof(memInfo));
        if(result == 0)
        {
            // VirtualQuery failed.  This is not an expected condition, but it would be benign for the purposes of this check.  Seems
            // to occur occasionally on process shutdown.
            break;
        }
        else if(memInfo.Protect == PAGE_EXECUTE_READWRITE)
        {
            Output::Print(_u("ERROR: Found PAGE_EXECUTE_READWRITE page!\n"));
#ifdef DEBUG
            AssertMsg(FALSE, "Page was marked PAGE_EXECUTE_READWRITE");
#else
            Fatal();
#endif
        }

        // Figure out if we need to continue the query.  The returned size might be larger than the size we requested,
        // for instance if more pages were allocated directly afterward, with the same permissions.
        if(memInfo.RegionSize >= size)
        {
            break;
        }

        // recalculate size for next iteration
        buffer += memInfo.RegionSize;
        size -= memInfo.RegionSize;

        if(size <= 0)
        {
            AssertMsg(FALSE, "Last VirtualQuery left us with unmatched regions");
            break;
        }
    }
}
#endif

#if DBG_DUMP
template <typename TAlloc, typename TPreReservedAlloc, class SyncObject>
void
EmitBufferManager<TAlloc, TPreReservedAlloc, SyncObject>::DumpAndResetStats(char16 const * filename)
{
    if (this->totalBytesCommitted != 0)
    {
        size_t wasted = this->totalBytesCommitted - this->totalBytesCode - this->totalBytesAlignment;
        Output::Print(_u("Stats for %s: %s \n"), name, filename);
        Output::Print(_u("  Total code size      : %10d (%6.2f%% of committed)\n"), this->totalBytesCode,
            (float)this->totalBytesCode * 100 / this->totalBytesCommitted);
        Output::Print(_u("  Total LoopBody code  : %10d\n"), this->totalBytesLoopBody);
        Output::Print(_u("  Total alignment size : %10d (%6.2f%% of committed)\n"), this->totalBytesAlignment,
            (float)this->totalBytesAlignment * 100 / this->totalBytesCommitted);
        Output::Print(_u("  Total wasted size    : %10d (%6.2f%% of committed)\n"), wasted,
            (float)wasted * 100 / this->totalBytesCommitted);
        Output::Print(_u("  Total committed size : %10d (%6.2f%% of reserved)\n"), this->totalBytesCommitted,
            (float)this->totalBytesCommitted * 100 / this->totalBytesReserved);
        Output::Print(_u("  Total reserved size  : %10d\n"), this->totalBytesReserved);
    }
    this->totalBytesCode = 0;
    this->totalBytesLoopBody = 0;
    this->totalBytesAlignment = 0;
    this->totalBytesCommitted = 0;
    this->totalBytesReserved = 0;
}
#endif

template class EmitBufferManager<VirtualAllocWrapper, PreReservedVirtualAllocWrapper, FakeCriticalSection>;
template class EmitBufferManager<VirtualAllocWrapper, PreReservedVirtualAllocWrapper, CriticalSection>;
#if ENABLE_OOP_NATIVE_CODEGEN
template class EmitBufferManager<SectionAllocWrapper, PreReservedSectionAllocWrapper, FakeCriticalSection>;
template class EmitBufferManager<SectionAllocWrapper, PreReservedSectionAllocWrapper, CriticalSection>;
#endif
