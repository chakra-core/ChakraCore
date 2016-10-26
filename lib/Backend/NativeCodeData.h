//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define NativeCodeDataNew(func, T, ...) \
    func->IsOOPJIT() ? \
        AllocatorNew(NativeCodeData::AllocatorT<T>, func->GetNativeCodeDataAllocator(), T, __VA_ARGS__) \
        : AllocatorNew(NativeCodeDataNoFixup::Allocator, func->GetNativeCodeDataNoFixupAllocator(), T, __VA_ARGS__)
#define NativeCodeDataNewZ(func, T, ...) \
    func->IsOOPJIT() ? \
        AllocatorNewZ(NativeCodeData::AllocatorT<T>, func->GetNativeCodeDataAllocator(), T, __VA_ARGS__) \
        : AllocatorNewZ(NativeCodeDataNoFixup::Allocator, func->GetNativeCodeDataNoFixupAllocator(), T, __VA_ARGS__)
#define NativeCodeDataNewArrayZ(func, T, count) \
    func->IsOOPJIT() ? \
        AllocatorNewArrayZ(NativeCodeData::AllocatorT<NativeCodeData::Array<T>>, func->GetNativeCodeDataAllocator(), T, count) \
        : AllocatorNewArrayZ(NativeCodeDataNoFixup::Allocator, func->GetNativeCodeDataNoFixupAllocator(), T, count)
#define NativeCodeDataNewNoFixup(func, T, ...) \
    func->IsOOPJIT() ? \
        AllocatorNew(NativeCodeData::AllocatorNoFixup<T>, func->GetNativeCodeDataAllocator(), T, __VA_ARGS__) \
        : AllocatorNew(NativeCodeDataNoFixup::Allocator, func->GetNativeCodeDataNoFixupAllocator(), T, __VA_ARGS__)
#define NativeCodeDataNewArrayNoFixup(func, T, count) \
    func->IsOOPJIT() ? \
        AllocatorNewArray(NativeCodeData::AllocatorNoFixup<NativeCodeData::Array<T>>, func->GetNativeCodeDataAllocator(), T, count) \
        : AllocatorNewArray(NativeCodeDataNoFixup::Allocator, func->GetNativeCodeDataNoFixupAllocator(), T, count)
#define NativeCodeDataNewArrayZNoFixup(func, T, count) \
    func->IsOOPJIT() ? \
        AllocatorNewArrayZ(NativeCodeData::AllocatorNoFixup<NativeCodeData::Array<T>>, func->GetNativeCodeDataAllocator(), T, count) \
        : AllocatorNewArrayZ(NativeCodeDataNoFixup::Allocator, func->GetNativeCodeDataNoFixupAllocator(), T, count)

#define TransferDataNewZ(alloc, T, ...) AllocatorNewZ(NativeCodeDataNoFixup::Allocator, alloc, T, __VA_ARGS__)

#define FixupNativeDataPointer(field, chunkList) NativeCodeData::AddFixupEntry(this->field, &this->field, this, chunkList)

class CodeGenAllocators;

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

    static DataChunk* GetDataChunk(void* data)
    {
        Assert(JITManager::GetJITManager()->IsJITServer());
        return (NativeCodeData::DataChunk*)((char*)data - offsetof(NativeCodeData::DataChunk, data));
    }

    static char16* GetDataDescription(void* data, JitArenaAllocator * alloc);

    static unsigned int GetDataTotalOffset(void* data)
    {
        Assert(JITManager::GetJITManager()->IsJITServer());
        return GetDataChunk(data)->offset;
    }

    NativeCodeData(DataChunk * chunkList);
    DataChunk * chunkList;

#ifdef PERF_COUNTERS
    size_t size;
#endif
public:

    static void VerifyExistFixupEntry(void* targetAddr, void* addrToFixup, void* startAddress);
    static void AddFixupEntry(void* targetAddr, void* addrToFixup, void* startAddress, DataChunk * chunkList);
    static void AddFixupEntry(void* targetAddr, void* targetStartAddr, void* addrToFixup, void* startAddress, DataChunk * chunkList);
    static void AddFixupEntryForPointerArray(void* startAddress, DataChunk * chunkList);
    static void DeleteChunkList(DataChunk * chunkList);
public:
    class Allocator
    {
    public:
        static const bool FakeZeroLengthArray = false;

        Allocator();
        ~Allocator();

        char * Alloc(DECLSPEC_GUARD_OVERFLOW size_t requestedBytes);
        char * AllocZero(DECLSPEC_GUARD_OVERFLOW size_t requestedBytes);
        char * AllocLeaf(DECLSPEC_GUARD_OVERFLOW size_t requestedBytes);

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
    class Array
    {
    public:
        void Fixup(NativeCodeData::DataChunk* chunkList)
        {
            int count = NativeCodeData::GetDataChunk(this)->len / sizeof(T);
            while (count-- > 0)
            {
                (((T*)this) + count)->Fixup(chunkList);
            }
        }
    };

    template<typename T>
    class AllocatorNoFixup : public Allocator
    {
    public:
        char * Alloc(size_t requestedBytes)
        {
            char* dataBlock = __super::Alloc(requestedBytes);
#if DBG
            DataChunk* chunk = NativeCodeData::GetDataChunk(dataBlock);
            chunk->dataType = typeid(T).name();
            if (PHASE_TRACE1(Js::NativeCodeDataPhase))
            {
                Output::Print(_u("NativeCodeData AllocNoFix: chunk: %p, data: %p, index: %d, len: %x, totalOffset: %x, type: %S\n"),
                    chunk, (void*)dataBlock, chunk->allocIndex, chunk->len, chunk->offset, chunk->dataType);
            }
#endif

            return dataBlock;
        }
        char * AllocZero(size_t requestedBytes)
        {
            char* dataBlock = __super::AllocZero(requestedBytes);

#if DBG
            DataChunk* chunk = NativeCodeData::GetDataChunk(dataBlock);
            chunk->dataType = typeid(T).name();
            if (PHASE_TRACE1(Js::NativeCodeDataPhase))
            {
                Output::Print(_u("NativeCodeData AllocNoFix: chunk: %p, data: %p, index: %d, len: %x, totalOffset: %x, type: %S\n"),
                    chunk, (void*)dataBlock, chunk->allocIndex, chunk->len, chunk->offset, chunk->dataType);
            }
#endif

            return dataBlock;
        }
        char * AllocLeaf(size_t requestedBytes)
        {
            return Alloc(requestedBytes);
        }
    };

    template<typename T>
    class AllocatorT : public Allocator
    {
#if DBG
        __declspec(noinline) // compiler inline this function even in chk build... maybe because it's in .h file?
#endif
        char* AddFixup(char* dataBlock)
        {
            DataChunk* chunk = NativeCodeData::GetDataChunk(dataBlock);
            chunk->fixupFunc = &Fixup;
#if DBG
            chunk->dataType = typeid(T).name();
            if (PHASE_TRACE1(Js::NativeCodeDataPhase))
            {
                Output::Print(_u("NativeCodeData Alloc: chunk: %p, data: %p, index: %d, len: %x, totalOffset: %x, type: %S\n"),
                    chunk, (void*)dataBlock, chunk->allocIndex, chunk->len, chunk->offset, chunk->dataType);
            }
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

enum DataDesc
{
    DataDesc_None,
    DataDesc_InlineeFrameRecord_ArgOffsets,
    DataDesc_InlineeFrameRecord_Constants,
    DataDesc_BailoutInfo_CotalOutParamCount,
    DataDesc_ArgOutOffsetInfo_StartCallOutParamCounts,
    DataDesc_ArgOutOffsetInfo_StartCallArgRestoreAdjustCounts,
    DataDesc_LowererMD_LoadFloatValue_Float,
    DataDesc_LowererMD_LoadFloatValue_Double,
    DataDesc_LowererMD_EmitLoadFloatCommon_Double,
    DataDesc_LowererMD_Simd128LoadConst,
};

template<DataDesc desc = DataDesc_None>
struct IntType
{
    int data;
};

template<DataDesc desc = DataDesc_None>
struct UIntType
{
    uint data;
};

template<DataDesc desc = DataDesc_None>
struct FloatType
{
    FloatType(float val) :data(val) {}
    float data;
};

template<DataDesc desc = DataDesc_None>
struct DoubleType
{
    DoubleType() {}
    DoubleType(double val) :data(val) {}
    double data;
};

template<DataDesc desc = DataDesc_None>
struct SIMDType
{
    SIMDType() {}
    SIMDType(AsmJsSIMDValue val) :data(val) {}
    AsmJsSIMDValue data;
};

template<DataDesc desc = DataDesc_None>
struct VarType
{
    Js::Var data;
    void Fixup(NativeCodeData::DataChunk* chunkList)
    {
        AssertMsg(false, "Please specialize Fixup method for this Var type or use no-fixup allocator");
    }
};

template<>
inline void VarType<DataDesc_InlineeFrameRecord_Constants>::Fixup(NativeCodeData::DataChunk* chunkList)
{
    AssertMsg(false, "InlineeFrameRecord::constants contains Var from main process, should not fixup");
}

struct GlobalBailOutRecordDataTable;
template<>
inline void NativeCodeData::Array<GlobalBailOutRecordDataTable *>::Fixup(NativeCodeData::DataChunk* chunkList)
{
    NativeCodeData::AddFixupEntryForPointerArray(this, chunkList);
}


class NativeCodeDataNoFixup
{

private:
    struct DataChunk
    {
        DataChunk * next;
        char data[0];
    };
    NativeCodeDataNoFixup(DataChunk * chunkList);
    DataChunk * chunkList;

#ifdef PERF_COUNTERS
    size_t size;
#endif
    static void DeleteChunkList(DataChunk * chunkList);
public:
    class Allocator
    {
    public:
        static const bool FakeZeroLengthArray = false;

        Allocator();
        ~Allocator();

        char * Alloc(DECLSPEC_GUARD_OVERFLOW size_t requestedBytes);
        char * AllocZero(DECLSPEC_GUARD_OVERFLOW size_t requestedBytes);
        char * AllocLeaf(DECLSPEC_GUARD_OVERFLOW size_t requestedBytes) { return Alloc(requestedBytes); }
        NativeCodeDataNoFixup * Finalize();
        void Free(void * buffer, size_t byteSize);

#ifdef TRACK_ALLOC
        // Doesn't support tracking information, dummy implementation
        Allocator * TrackAllocInfo(TrackAllocData const& data) { return this; }
        void ClearTrackAllocInfo(TrackAllocData* data = NULL) {}
#endif
    private:
        DataChunk * chunkList;
#if DBG
        bool finalized;
#endif
#ifdef PERF_COUNTERS
        size_t size;
#endif
    };

    ~NativeCodeDataNoFixup();

};