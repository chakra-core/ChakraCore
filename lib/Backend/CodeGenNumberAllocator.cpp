//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

#if !FLOATVAR
CodeGenNumberThreadAllocator::CodeGenNumberThreadAllocator(Recycler * recycler)
    : recycler(recycler), currentNumberSegment(nullptr), currentChunkSegment(nullptr),
    numberSegmentEnd(nullptr), currentNumberBlockEnd(nullptr), nextNumber(nullptr), chunkSegmentEnd(nullptr),
    currentChunkBlockEnd(nullptr), nextChunk(nullptr), hasNewNumberBlock(nullptr), hasNewChunkBlock(nullptr),
    pendingIntegrationNumberSegmentCount(0), pendingIntegrationChunkSegmentCount(0),
    pendingIntegrationNumberSegmentPageCount(0), pendingIntegrationChunkSegmentPageCount(0)
{
}

CodeGenNumberThreadAllocator::~CodeGenNumberThreadAllocator()
{
    pendingIntegrationNumberSegment.Clear(&NoThrowNoMemProtectHeapAllocator::Instance);
    pendingIntegrationChunkSegment.Clear(&NoThrowNoMemProtectHeapAllocator::Instance);
    pendingIntegrationNumberBlock.Clear(&NoThrowHeapAllocator::Instance);
    pendingIntegrationChunkBlock.Clear(&NoThrowHeapAllocator::Instance);
    pendingFlushNumberBlock.Clear(&NoThrowHeapAllocator::Instance);
    pendingFlushChunkBlock.Clear(&NoThrowHeapAllocator::Instance);
    pendingReferenceNumberBlock.Clear(&NoThrowHeapAllocator::Instance);
}

size_t
CodeGenNumberThreadAllocator::GetNumberAllocSize()
{
#ifdef RECYCLER_MEMORY_VERIFY
    if (recycler->VerifyEnabled())
    {
        return HeapInfo::GetAlignedSize(AllocSizeMath::Add(sizeof(Js::JavascriptNumber) + sizeof(size_t), recycler->verifyPad));
    }
#endif
    return HeapInfo::GetAlignedSizeNoCheck(sizeof(Js::JavascriptNumber));
}


size_t
CodeGenNumberThreadAllocator::GetChunkAllocSize()
{
#ifdef RECYCLER_MEMORY_VERIFY
    if (recycler->VerifyEnabled())
    {
        return HeapInfo::GetAlignedSize(AllocSizeMath::Add(sizeof(CodeGenNumberChunk) + sizeof(size_t), recycler->verifyPad));
    }
#endif
    return HeapInfo::GetAlignedSizeNoCheck(sizeof(CodeGenNumberChunk));
}

Js::JavascriptNumber *
CodeGenNumberThreadAllocator::AllocNumber()
{
    AutoCriticalSection autocs(&cs);
    size_t sizeCat = GetNumberAllocSize();
    if (nextNumber + sizeCat > currentNumberBlockEnd)
    {
        AllocNewNumberBlock();
    }
    Js::JavascriptNumber * newNumber = (Js::JavascriptNumber *)nextNumber;
#ifdef RECYCLER_MEMORY_VERIFY
    recycler->FillCheckPad(newNumber, sizeof(Js::JavascriptNumber), sizeCat);
#endif

    nextNumber += sizeCat;
    return newNumber;
}

CodeGenNumberChunk *
CodeGenNumberThreadAllocator::AllocChunk()
{
    AutoCriticalSection autocs(&cs);
    size_t sizeCat = GetChunkAllocSize();
    if (nextChunk + sizeCat > currentChunkBlockEnd)
    {
        AllocNewChunkBlock();
    }
    CodeGenNumberChunk * newChunk = (CodeGenNumberChunk *)nextChunk;
#ifdef RECYCLER_MEMORY_VERIFY
    recycler->FillCheckPad(nextChunk, sizeof(CodeGenNumberChunk), sizeCat);
#endif

    memset(newChunk, 0, sizeof(CodeGenNumberChunk));
    nextChunk += sizeCat;
    return newChunk;
}

void
CodeGenNumberThreadAllocator::AllocNewNumberBlock()
{
    Assert(cs.IsLocked());
    Assert(nextNumber + GetNumberAllocSize() > currentNumberBlockEnd);
    if (hasNewNumberBlock)
    {
        if (!pendingReferenceNumberBlock.PrependNode(&NoThrowHeapAllocator::Instance,
            currentNumberBlockEnd - BlockSize, currentNumberSegment))
        {
            Js::Throw::OutOfMemory();
        }
        hasNewNumberBlock = false;
    }

    if (currentNumberBlockEnd == numberSegmentEnd)
    {
        Assert(cs.IsLocked());
        // Reserve the segment, but not committing it
        currentNumberSegment = PageAllocator::AllocPageSegment(pendingIntegrationNumberSegment, this->recycler->GetRecyclerLeafPageAllocator(), false, true, false);
        if (currentNumberSegment == nullptr)
        {
            currentNumberBlockEnd = nullptr;
            numberSegmentEnd = nullptr;
            nextNumber = nullptr;
            Js::Throw::OutOfMemory();
        }
        pendingIntegrationNumberSegmentCount++;
        pendingIntegrationNumberSegmentPageCount += currentNumberSegment->GetPageCount();
        currentNumberBlockEnd = currentNumberSegment->GetAddress();
        numberSegmentEnd = currentNumberSegment->GetEndAddress();
    }

    // Commit the page.
    if (!::VirtualAlloc(currentNumberBlockEnd, BlockSize, MEM_COMMIT, PAGE_READWRITE))
    {
        Js::Throw::OutOfMemory();
    }
    nextNumber = currentNumberBlockEnd;
    currentNumberBlockEnd += BlockSize;
    hasNewNumberBlock = true;
    this->recycler->GetRecyclerLeafPageAllocator()->FillAllocPages(nextNumber, 1);
}

void
CodeGenNumberThreadAllocator::AllocNewChunkBlock()
{
    Assert(cs.IsLocked());
    Assert(nextChunk + GetChunkAllocSize() > currentChunkBlockEnd);
    if (hasNewChunkBlock)
    {
        if (!pendingFlushChunkBlock.PrependNode(&NoThrowHeapAllocator::Instance,
            currentChunkBlockEnd - BlockSize, currentChunkSegment))
        {
            Js::Throw::OutOfMemory();
        }
        // All integrated pages' object are all live initially, so don't need to rescan them
        // todo: SWB: need to allocate number with write barrier pages
        ::ResetWriteWatch(currentChunkBlockEnd - BlockSize, BlockSize);
        pendingReferenceNumberBlock.MoveTo(&pendingFlushNumberBlock);
        hasNewChunkBlock = false;
    }

    if (currentChunkBlockEnd == chunkSegmentEnd)
    {
        Assert(cs.IsLocked());
        // Reserve the segment, but not committing it
        currentChunkSegment = PageAllocator::AllocPageSegment(pendingIntegrationChunkSegment, this->recycler->GetRecyclerPageAllocator(), false, true, false);
        if (currentChunkSegment == nullptr)
        {
            currentChunkBlockEnd = nullptr;
            chunkSegmentEnd = nullptr;
            nextChunk = nullptr;
            Js::Throw::OutOfMemory();
        }
        pendingIntegrationChunkSegmentCount++;
        pendingIntegrationChunkSegmentPageCount += currentChunkSegment->GetPageCount();
        currentChunkBlockEnd = currentChunkSegment->GetAddress();
        chunkSegmentEnd = currentChunkSegment->GetEndAddress();
    }

    // Commit the page.
    if (!::VirtualAlloc(currentChunkBlockEnd, BlockSize, MEM_COMMIT, PAGE_READWRITE))
    {
        Js::Throw::OutOfMemory();
    }

    nextChunk = currentChunkBlockEnd;
    currentChunkBlockEnd += BlockSize;
    hasNewChunkBlock = true;
    this->recycler->GetRecyclerLeafPageAllocator()->FillAllocPages(nextChunk, 1);
}

void
CodeGenNumberThreadAllocator::Integrate()
{
    AutoCriticalSection autocs(&cs);
    PageAllocator * leafPageAllocator = this->recycler->GetRecyclerLeafPageAllocator();
    leafPageAllocator->IntegrateSegments(pendingIntegrationNumberSegment, pendingIntegrationNumberSegmentCount, pendingIntegrationNumberSegmentPageCount);
    PageAllocator * recyclerPageAllocator = this->recycler->GetRecyclerPageAllocator();
    recyclerPageAllocator->IntegrateSegments(pendingIntegrationChunkSegment, pendingIntegrationChunkSegmentCount, pendingIntegrationChunkSegmentPageCount);
    pendingIntegrationNumberSegmentCount = 0;
    pendingIntegrationChunkSegmentCount = 0;
    pendingIntegrationNumberSegmentPageCount = 0;
    pendingIntegrationChunkSegmentPageCount = 0;

#ifdef TRACK_ALLOC
    TrackAllocData oldAllocData = recycler->nextAllocData;
    recycler->nextAllocData.Clear();
#endif
    while (!pendingIntegrationNumberBlock.Empty())
    {
        TRACK_ALLOC_INFO(recycler, Js::JavascriptNumber, Recycler, 0, (size_t)-1);

        BlockRecord& record = pendingIntegrationNumberBlock.Head();
        if (!recycler->IntegrateBlock<LeafBit>(record.blockAddress, record.segment, GetNumberAllocSize(), sizeof(Js::JavascriptNumber)))
        {
            Js::Throw::OutOfMemory();
        }
        pendingIntegrationNumberBlock.RemoveHead(&NoThrowHeapAllocator::Instance);
    }


    while (!pendingIntegrationChunkBlock.Empty())
    {
        // REVIEW: the above number block integration can be moved into this loop

        TRACK_ALLOC_INFO(recycler, CodeGenNumberChunk, Recycler, 0, (size_t)-1);

        BlockRecord& record = pendingIntegrationChunkBlock.Head();
        if (!recycler->IntegrateBlock<NoBit>(record.blockAddress, record.segment, GetChunkAllocSize(), sizeof(CodeGenNumberChunk)))
        {
            Js::Throw::OutOfMemory();
        }
#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
        if (CONFIG_FLAG(ForceSoftwareWriteBarrier) && CONFIG_FLAG(RecyclerVerifyMark))
        {
            Recycler::WBSetBitRange(record.blockAddress, BlockSize / sizeof(void*));
        }
#endif
        pendingIntegrationChunkBlock.RemoveHead(&NoThrowHeapAllocator::Instance);
    }
#ifdef TRACK_ALLOC
    Assert(recycler->nextAllocData.IsEmpty());
    recycler->nextAllocData = oldAllocData;
#endif
}

void
CodeGenNumberThreadAllocator::FlushAllocations()
{
    AutoCriticalSection autocs(&cs);
    pendingFlushNumberBlock.MoveTo(&pendingIntegrationNumberBlock);
    pendingFlushChunkBlock.MoveTo(&pendingIntegrationChunkBlock);
}

CodeGenNumberAllocator::CodeGenNumberAllocator(CodeGenNumberThreadAllocator * threadAlloc, Recycler * recycler) :
    threadAlloc(threadAlloc), recycler(recycler), chunk(nullptr), chunkTail(nullptr), currentChunkNumberCount(CodeGenNumberChunk::MaxNumberCount)
{
#if DBG
    finalized = false;
#endif
}

// We should never call this function if we are using tagged float
Js::JavascriptNumber *
CodeGenNumberAllocator::Alloc()
{
    Assert(!finalized);
    if (currentChunkNumberCount == CodeGenNumberChunk::MaxNumberCount)
    {
        CodeGenNumberChunk * newChunk = threadAlloc? threadAlloc->AllocChunk()
            : RecyclerNewStructZ(recycler, CodeGenNumberChunk);
        // Need to always put the new chunk last, as when we flush
        // pages, new chunk's page might not be full yet, and won't
        // be flushed, and we will have a broken link in the link list.
        newChunk->next = nullptr;
        if (this->chunkTail != nullptr)
        {
            this->chunkTail->next = newChunk;
        }
        else
        {
            this->chunk = newChunk;
        }
        this->chunkTail = newChunk;
        this->currentChunkNumberCount = 0;
    }
    Js::JavascriptNumber * newNumber = threadAlloc? threadAlloc->AllocNumber()
        : Js::JavascriptNumber::NewUninitialized(recycler);
    this->chunkTail->numbers[this->currentChunkNumberCount++] = newNumber;
    return newNumber;
}


CodeGenNumberChunk *
CodeGenNumberAllocator::Finalize()
{
    Assert(!finalized);
#if DBG
    finalized = true;
#endif
    CodeGenNumberChunk * finalizedChunk = this->chunk;
    this->chunk = nullptr;
    this->chunkTail = nullptr;
    this->currentChunkNumberCount = 0;
    return finalizedChunk;
}


uint XProcNumberPageSegmentImpl::sizeCat = sizeof(Js::JavascriptNumber);
Js::JavascriptNumber* XProcNumberPageSegmentImpl::AllocateNumber(Func* func, double value)
{
    HANDLE hProcess = func->GetThreadContextInfo()->GetProcessHandle();

    XProcNumberPageSegmentImpl* tail = this;

    if (this->pageAddress != 0)
    {
        while (tail->nextSegment)
        {
            tail = (XProcNumberPageSegmentImpl*)tail->nextSegment;
        }

        if (tail->pageAddress + tail->committedEnd - tail->allocEndAddress >= sizeCat)
        {
            auto number = tail->allocEndAddress;
            tail->allocEndAddress += sizeCat;

#if DBG
            Js::JavascriptNumber localNumber(value, (Js::StaticType*)func->GetScriptContextInfo()->GetNumberTypeStaticAddr(), true);
#else
            Js::JavascriptNumber localNumber(value, (Js::StaticType*)func->GetScriptContextInfo()->GetNumberTypeStaticAddr());
#endif
            Js::JavascriptNumber* pLocalNumber = &localNumber;

#ifdef RECYCLER_MEMORY_VERIFY
            if (func->GetScriptContextInfo()->IsRecyclerVerifyEnabled())
            {
                pLocalNumber = (Js::JavascriptNumber*)alloca(sizeCat);
                memset(pLocalNumber, Recycler::VerifyMemFill, sizeCat);
                Recycler::FillPadNoCheck(pLocalNumber, sizeof(Js::JavascriptNumber), sizeCat, false);
                pLocalNumber = new (pLocalNumber) Js::JavascriptNumber(localNumber);
            }
#else
            Assert(sizeCat == sizeof(Js::JavascriptNumber));
            __analysis_assume(sizeCat == sizeof(Js::JavascriptNumber));
#endif
            // change vtable to the remote one
            *(void**)pLocalNumber = (void*)func->GetScriptContextInfo()->GetVTableAddress(VTableValue::VtableJavascriptNumber);

            // initialize number by WriteProcessMemory
            if (!WriteProcessMemory(hProcess, (void*)number, pLocalNumber, sizeCat, NULL))
            {
                MemoryOperationLastError::RecordLastError();
                Js::Throw::OutOfMemory();
            }

            return (Js::JavascriptNumber*) number;
        }

        // alloc blocks
        if (tail->GetCommitEndAddress() < tail->GetEndAddress())
        {
            Assert((unsigned int)((char*)tail->GetEndAddress() - (char*)tail->GetCommitEndAddress()) >= BlockSize);
            // TODO: implement guard pages (still necessary for OOP JIT?)
            LPVOID addr = ::VirtualAllocEx(hProcess, tail->GetCommitEndAddress(), BlockSize, MEM_COMMIT, PAGE_READWRITE);
            if (addr == nullptr)
            {
                MemoryOperationLastError::RecordLastError();
                Js::Throw::OutOfMemory();
            }
            tail->committedEnd += BlockSize;
            return AllocateNumber(func, value);
        }
    }

    // alloc new segment
    void* pages = ::VirtualAllocEx(hProcess, nullptr, PageCount * AutoSystemInfo::PageSize, MEM_RESERVE, PAGE_READWRITE);
    if (pages == nullptr)
    {
        MemoryOperationLastError::RecordLastError();
        Js::Throw::OutOfMemory();
    }

    if (tail->pageAddress == 0)
    {
        tail->pageAddress = (intptr_t)pages;
        tail->allocStartAddress = this->pageAddress;
        tail->allocEndAddress = this->pageAddress;
        tail->nextSegment = nullptr;
        return AllocateNumber(func, value);
    }
    else
    {
        XProcNumberPageSegmentImpl* seg = (XProcNumberPageSegmentImpl*)midl_user_allocate(sizeof(XProcNumberPageSegment));
        if (seg == nullptr)
        {
            Js::Throw::OutOfMemory();
        }
        seg = new (seg) XProcNumberPageSegmentImpl();
        tail->nextSegment = seg;
        return seg->AllocateNumber(func, value);
    }
}


XProcNumberPageSegmentImpl::XProcNumberPageSegmentImpl()
{
    memset(this, 0, sizeof(XProcNumberPageSegment));
}

void XProcNumberPageSegmentImpl::Initialize(bool recyclerVerifyEnabled, uint recyclerVerifyPad)
{
    uint allocSize = (uint)sizeof(Js::JavascriptNumber);
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    allocSize += Js::Configuration::Global.flags.NumberAllocPlusSize;
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    // TODO: share same pad size with main process
    if (recyclerVerifyEnabled)
    {
        uint padAllocSize = (uint)AllocSizeMath::Add(sizeof(Js::JavascriptNumber) + sizeof(size_t), recyclerVerifyPad);
        allocSize = padAllocSize < allocSize ? allocSize : padAllocSize;
    }
#endif

    allocSize = (uint)HeapInfo::GetAlignedSizeNoCheck(allocSize);

    if (BlockSize%allocSize != 0)
    {
        // align allocation sizeCat to be 2^n to make integration easier
        allocSize = BlockSize / (1 << (Math::Log2((size_t)BlockSize / allocSize)));
    }

    sizeCat = allocSize;
}

Field(Js::JavascriptNumber*)* ::XProcNumberPageSegmentManager::RegisterSegments(XProcNumberPageSegment* segments)
{
    Assert(segments->pageAddress && segments->allocStartAddress && segments->allocEndAddress);
    XProcNumberPageSegmentImpl* segmentImpl = (XProcNumberPageSegmentImpl*)segments;

    XProcNumberPageSegmentImpl* temp = segmentImpl;
    size_t totalCount = 0;
    while (temp)
    {
        totalCount += (temp->allocEndAddress - temp->allocStartAddress) / XProcNumberPageSegmentImpl::sizeCat;
        temp = (XProcNumberPageSegmentImpl*)temp->nextSegment;
    }

    Field(Js::JavascriptNumber*)* numbers = RecyclerNewArray(this->recycler, Field(Js::JavascriptNumber*), totalCount);

    temp = segmentImpl;
    int count = 0;
    while (temp)
    {
        while (temp->allocStartAddress < temp->allocEndAddress)
        {
            numbers[count] = (Js::JavascriptNumber*)temp->allocStartAddress;
            count++;
            temp->allocStartAddress += XProcNumberPageSegmentImpl::sizeCat;
        }
        temp = (XProcNumberPageSegmentImpl*)temp->nextSegment;
    }

    AutoCriticalSection autoCS(&cs);
    if (this->segmentsList == nullptr)
    {
        this->segmentsList = segmentImpl;
    }
    else
    {
        temp = segmentsList;
        while (temp->nextSegment)
        {
            temp = (XProcNumberPageSegmentImpl*)temp->nextSegment;
        }
        temp->nextSegment = segmentImpl;
    }

    return numbers;
}

XProcNumberPageSegment * XProcNumberPageSegmentManager::GetFreeSegment(Memory::ArenaAllocator* alloc)
{
    AutoCriticalSection autoCS(&cs);

    auto temp = segmentsList;
    auto prev = &segmentsList;
    while (temp)
    {
        if (temp->allocEndAddress != temp->pageAddress + (int)(XProcNumberPageSegmentImpl::PageCount*AutoSystemInfo::PageSize)) // not full
        {
            *prev = (XProcNumberPageSegmentImpl*)temp->nextSegment;

            // remove from the list
            XProcNumberPageSegment * seg = (XProcNumberPageSegment *)AnewStructZ(alloc, XProcNumberPageSegmentImpl);
            temp->nextSegment = 0;
            memcpy(seg, temp, sizeof(XProcNumberPageSegment));
            midl_user_free(temp);
            return seg;
        }
        prev = (XProcNumberPageSegmentImpl**)&temp->nextSegment;
        temp = (XProcNumberPageSegmentImpl*)temp->nextSegment;
    }

    return nullptr;
}

void XProcNumberPageSegmentManager::Integrate()
{
    AutoCriticalSection autoCS(&cs);

    auto temp = this->segmentsList;
    auto prev = &this->segmentsList;
    while (temp)
    {
        if((uintptr_t)temp->allocEndAddress - (uintptr_t)temp->pageAddress > temp->blockIntegratedSize + XProcNumberPageSegmentImpl::BlockSize)
        {
            if (temp->pageSegment == 0)
            {
                auto leafPageAllocator = recycler->GetRecyclerLeafPageAllocator();
                DListBase<PageSegment> segmentList;
                temp->pageSegment = (intptr_t)leafPageAllocator->AllocPageSegment(segmentList, leafPageAllocator,
                    (void*)temp->pageAddress, XProcNumberPageSegmentImpl::PageCount, temp->committedEnd / AutoSystemInfo::PageSize, false);

                if (temp->pageSegment)
                {
                    leafPageAllocator->IntegrateSegments(segmentList, 1, XProcNumberPageSegmentImpl::PageCount);
                    this->integratedSegmentCount++;
                }
            }

            if (temp->pageSegment)
            {
                unsigned int minIntegrateSize = XProcNumberPageSegmentImpl::BlockSize;
                for (; temp->pageAddress + temp->blockIntegratedSize + minIntegrateSize < (unsigned int)temp->allocEndAddress;
                    temp->blockIntegratedSize += minIntegrateSize)
                {
                    TRACK_ALLOC_INFO(recycler, Js::JavascriptNumber, Recycler, 0, (size_t)-1);

                    if (!recycler->IntegrateBlock<LeafBit>((char*)temp->pageAddress + temp->blockIntegratedSize,
                        (PageSegment*)temp->pageSegment, XProcNumberPageSegmentImpl::sizeCat, sizeof(Js::JavascriptNumber)))
                    {
                        Js::Throw::OutOfMemory();
                    }
                }

                if ((uintptr_t)temp->allocEndAddress + XProcNumberPageSegmentImpl::sizeCat
                    > (uintptr_t)temp->pageAddress + XProcNumberPageSegmentImpl::PageCount*AutoSystemInfo::PageSize)
                {
                    *prev = (XProcNumberPageSegmentImpl*)temp->nextSegment;
                    midl_user_free(temp);
                    temp = *prev;
                    continue;
                }
            }
        }

        temp = (XProcNumberPageSegmentImpl*)temp->nextSegment;
    }
}

XProcNumberPageSegmentManager::XProcNumberPageSegmentManager(Recycler* recycler)
    :segmentsList(nullptr), recycler(recycler), integratedSegmentCount(0)
{
#ifdef RECYCLER_MEMORY_VERIFY
    XProcNumberPageSegmentImpl::Initialize(recycler->VerifyEnabled() == TRUE, recycler->GetVerifyPad());
#else
    XProcNumberPageSegmentImpl::Initialize(false, 0);
#endif
}

XProcNumberPageSegmentManager::~XProcNumberPageSegmentManager()
{
    auto temp = segmentsList;
    while (temp)
    {
        auto next = temp->nextSegment;
        midl_user_free(temp);
        temp = (XProcNumberPageSegmentImpl*)next;
    }
}
#endif