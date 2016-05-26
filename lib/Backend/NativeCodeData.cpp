//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

NativeCodeData::NativeCodeData(DataChunk * chunkList) : chunkList(chunkList)
{
#ifdef PERF_COUNTERS
    this->size = 0;
#endif
}

NativeCodeData::~NativeCodeData()
{
    NativeCodeData::DeleteChunkList(this->chunkList);
    PERF_COUNTER_SUB(Code, DynamicNativeCodeDataSize, this->size);
    PERF_COUNTER_SUB(Code, TotalNativeCodeDataSize, this->size);
}

// dataAddr: target address
// startAddress: current data start address
// addrToFixup: address that currently pointing to dataAddr, which need to be updated
void
NativeCodeData::AddFixupEntry(void* dataAddr, void* addrToFixup, void* startAddress)
{
    DataChunk* chunk = (DataChunk*)((char*)startAddress - offsetof(DataChunk, data));

    NativeDataFixupEntry* entry = (NativeDataFixupEntry*)midl_user_allocate(sizeof(NativeDataFixupEntry));
    entry->addrOffset = (unsigned int)((__int64)addrToFixup - (__int64)startAddress);
    entry->targetTotalOffset = ((DataChunk*)((char*)dataAddr - offsetof(DataChunk, data)))->offset;
    entry->next = chunk->fixupList;
    chunk->fixupList = entry;

}

void
NativeCodeData::DeleteChunkList(DataChunk * chunkList)
{
    DataChunk * next = chunkList;
    while (next != nullptr)
    {
        DataChunk * current = next;
        next = next->next;
        delete current;
    }
}

NativeCodeData::Allocator::Allocator() : chunkList(nullptr), lastChunkList(nullptr)
{
    this->totalSize = 0;
    this->allocCount = 0;
#if DBG
    this->finalized = false;
#endif
#ifdef PERF_COUNTERS
    this->size = 0;
#endif
}

NativeCodeData::Allocator::~Allocator()
{
    Assert(!finalized || this->chunkList == nullptr);
    NativeCodeData::DeleteChunkList(this->chunkList);
    PERF_COUNTER_SUB(Code, DynamicNativeCodeDataSize, this->size);
    PERF_COUNTER_SUB(Code, TotalNativeCodeDataSize, this->size);
}

char *
NativeCodeData::Allocator::Alloc(size_t requestSize)
{
    char * data = nullptr;
    Assert(!finalized);    
    requestSize = Math::Align(requestSize, sizeof(void*));
    DataChunk * newChunk = HeapNewStructPlus(requestSize, DataChunk);


    newChunk->next = nullptr;
    newChunk->allocIndex = this->allocCount++;
    newChunk->len = (unsigned int)requestSize;
    newChunk->fixupList = nullptr;
    newChunk->offset = this->totalSize;
    if (this->chunkList == nullptr)
    {
        this->chunkList = newChunk;
        this->lastChunkList = newChunk;
    }
    else
    {
        this->lastChunkList->next = newChunk;
        this->lastChunkList = newChunk;
    }

    data = newChunk->data;
    this->totalSize += (unsigned int)requestSize;

#ifdef PERF_COUNTERS
    this->size += requestSize;
    PERF_COUNTER_ADD(Code, DynamicNativeCodeDataSize, requestSize);
#endif

    PERF_COUNTER_ADD(Code, TotalNativeCodeDataSize, requestSize);
    return data;
}

char *
NativeCodeData::Allocator::AllocZero(size_t requestSize)
{
    char * data = Alloc(requestSize);
    memset(data, 0, requestSize);
    return data;
}

NativeCodeData *
NativeCodeData::Allocator::Finalize()
{
    NativeCodeData * data = nullptr;
    if (this->chunkList != nullptr)
    {
        data = HeapNew(NativeCodeData, this->chunkList);
        this->chunkList = nullptr;
#ifdef PERF_COUNTERS
        data->size = this->size;
        this->size = 0;
#endif
    }
#if DBG
    this->finalized = true;
#endif
    return data;
}

//////////////////////////////////////////////////////////////////////////
//NativeCodeData::Allocator::Free
//This function should not be called at all because the life time is active during the run time
//This function is added to enable Dictionary(has calls to Free() Method - which will never be called as it will be
//allocated as a NativeAllocator to be allocated with NativeAllocator)
//////////////////////////////////////////////////////////////////////////
void
NativeCodeData::Allocator::Free(void * buffer, size_t byteSize)
{
}
