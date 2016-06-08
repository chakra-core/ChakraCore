//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define NativeCodeDataNew(alloc, T, ...) AllocatorNew(NativeCodeData::AllocatorT<T>, alloc, T, __VA_ARGS__)
#define NativeCodeDataNewZ(alloc, T, ...) AllocatorNewZ(NativeCodeData::AllocatorT<T>, alloc, T, __VA_ARGS__)
#define NativeCodeDataNewArray(alloc, T, count) AllocatorNewArray(NativeCodeData::AllocatorT<T>, alloc, T, count)
#define NativeCodeDataNewArrayZ(alloc, T, count) AllocatorNewArrayZ(NativeCodeData::AllocatorT<T>, alloc, T, count)
#define NativeCodeDataNewPlusZ(size, alloc, T, ...) AllocatorNewPlusZ(NativeCodeData::AllocatorT<T>, alloc, size, T, __VA_ARGS__)


#define NativeCodeDataNewNoFixup(alloc, T, ...) AllocatorNew(NativeCodeData::Allocator, alloc, T, __VA_ARGS__)
#define NativeCodeDataNewZNoFixup(alloc, T, ...) AllocatorNewZ(NativeCodeData::Allocator, alloc, T, __VA_ARGS__)
#define NativeCodeDataNewArrayNoFixup(alloc, T, count) AllocatorNewArray(NativeCodeData::Allocator, alloc, T, count)
#define NativeCodeDataNewArrayZNoFixup(alloc, T, count) AllocatorNewArrayZ(NativeCodeData::Allocator, alloc, T, count)

#define FixupNativeDataPointer(field, chunkList) NativeCodeData::AddFixupEntry(this->field, &this->field, this, chunkList)

struct CodeGenAllocators;

class NativeCodeData
{

public:
    
    struct DataChunk
    {
        unsigned int len;
        unsigned int allocIndex;
        unsigned int offset; // offset to the aggregated buffer
#if DBG
        const char* dataType;
#endif
        
        // todo: use union?
        void(*fixupFunc)(void* _this, NativeCodeData::DataChunk*);
        NativeDataFixupEntry *fixupList;

        DataChunk * next;
        char data[0];
    };


    NativeCodeData(DataChunk * chunkList);
    DataChunk * chunkList;

#ifdef PERF_COUNTERS
    size_t size;
#endif
public:

    static void VerifyExistFixupEntry(void* targetAddr, void* addrToFixup, void* startAddress);
    static void AddFixupEntry(void* targetAddr, void* addrToFixup, void* startAddress, DataChunk * chunkList);
    static void AddFixupEntryForPointerArray(void* startAddress, DataChunk * chunkList);
    static void DeleteChunkList(DataChunk * chunkList);
public:
    class Allocator
    {
    public:
        static const bool FakeZeroLengthArray = false;

        Allocator();
        ~Allocator();

        char * Alloc(size_t requestedBytes);
        char * AllocZero(size_t requestedBytes);
        NativeCodeData * Finalize();
        void Free(void * buffer, size_t byteSize);

        DataChunk * chunkList;
        DataChunk * lastChunkList;
        unsigned int totalSize;
        unsigned int allocCount;

#ifdef TRACK_ALLOC
        // Doesn't support tracking information, dummy implementation
        Allocator * TrackAllocInfo(TrackAllocData const& data) { return this; }
        void ClearTrackAllocInfo(TrackAllocData* data = NULL) {}
#endif
    private:

#if DBG
        bool finalized;
#endif
#ifdef PERF_COUNTERS
        size_t size;
#endif
    };

    template<typename T>
    class AllocatorT :public Allocator
    {
        char* AddFixup(char* dataBlock)
        {
            DataChunk* chunk = (DataChunk*)(dataBlock - offsetof(DataChunk, data));
            chunk->fixupFunc = &Fixup;
#if DBG
            chunk->dataType = typeid(T).name();
#endif     
            return dataBlock;
        }

    public:
        char * Alloc(size_t requestedBytes)
        {
            return AddFixup(__super::Alloc(requestedBytes));
        }
        char * AllocZero(size_t requestedBytes)
        {
            return AddFixup(__super::AllocZero(requestedBytes));
        }

        static void Fixup(void* pThis, NativeCodeData::DataChunk* chunkList)
        {
            ((T*)pThis)->Fixup(chunkList);
        }
    };

    ~NativeCodeData();
};

#if DBG
template<> void NativeCodeData::AllocatorT<double>::Fixup(void* pThis, NativeCodeData::DataChunk* chunkList){}
template<> void NativeCodeData::AllocatorT<float>::Fixup(void* pThis, NativeCodeData::DataChunk* chunkList){}
template<> void NativeCodeData::AllocatorT<AsmJsSIMDValue>::Fixup(void* pThis, NativeCodeData::DataChunk* chunkList){}
template<> void NativeCodeData::AllocatorT<int>::Fixup(void* pThis, NativeCodeData::DataChunk* chunkList) {}
template<> void NativeCodeData::AllocatorT<uint>::Fixup(void* pThis, NativeCodeData::DataChunk* chunkList) {}
#else
template<>
char*
NativeCodeData::AllocatorT<double>::Alloc(size_t requestedBytes)
{
    return __super::Alloc(requestedBytes);
}
template<>
char*
NativeCodeData::AllocatorT<double>::AllocZero(size_t requestedBytes)
{
    return __super::AllocZero(requestedBytes);
}
template<>
char*
NativeCodeData::AllocatorT<float>::Alloc(size_t requestedBytes)
{
    return __super::Alloc(requestedBytes);
}
template<>
char*
NativeCodeData::AllocatorT<float>::AllocZero(size_t requestedBytes)
{
    return __super::AllocZero(requestedBytes);
}
template<>
char*
NativeCodeData::AllocatorT<AsmJsSIMDValue>::Alloc(size_t requestedBytes)
{
    return __super::Alloc(requestedBytes);
}
template<>
char*
NativeCodeData::AllocatorT<AsmJsSIMDValue>::AllocZero(size_t requestedBytes)
{
    return __super::AllocZero(requestedBytes);
}

template<>
char*
NativeCodeData::AllocatorT<int>::Alloc(size_t requestedBytes)
{
    return __super::Alloc(requestedBytes);
}
template<>
char*
NativeCodeData::AllocatorT<int>::AllocZero(size_t requestedBytes)
{
    return __super::AllocZero(requestedBytes);
}

template<>
char*
NativeCodeData::AllocatorT<uint>::Alloc(size_t requestedBytes)
{
    return __super::Alloc(requestedBytes);
}
template<>
char*
NativeCodeData::AllocatorT<uint>::AllocZero(size_t requestedBytes)
{
    return __super::AllocZero(requestedBytes);
}
#endif


template<> void NativeCodeData::AllocatorT<Js::Var>::Fixup(void* pThis, NativeCodeData::DataChunk* chunkList)
{
    NativeCodeData::AddFixupEntryForPointerArray(pThis, chunkList);
}
