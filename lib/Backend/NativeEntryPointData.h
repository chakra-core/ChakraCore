//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_NATIVE_CODEGEN
class NativeCodeData;

namespace Js
{
    class FunctionBody;
};

typedef JsUtil::List<NativeOffsetInlineeFramePair, HeapAllocator> InlineeFrameMap;
typedef JsUtil::List<LazyBailOutRecord, HeapAllocator> BailOutRecordMap;

class JitTransferData;

class NativeEntryPointData
{
public:
    NativeEntryPointData();
    
    JitTransferData* EnsureJitTransferData(Recycler* recycler);
    JitTransferData* GetJitTransferData() { return this->jitTransferData; }    
    void FreeJitTransferData();

    void RecordNativeCode(Js::JavascriptMethod thunkAddress, Js::JavascriptMethod nativeAddress, ptrdiff_t codeSize, void * validationCookie);
    Js::JavascriptMethod GetNativeAddress() { return this->nativeAddress; }
    Js::JavascriptMethod GetThunkAddress() { return this->thunkAddress; }
    ptrdiff_t GetCodeSize() { return this->codeSize; }

    void SetTJNativeAddress(Js::JavascriptMethod nativeAddress, void * validationCookie);
    void SetTJCodeSize(ptrdiff_t codeSize);

    void AddWeakFuncRef(RecyclerWeakReference<Js::FunctionBody> *weakFuncRef, Recycler *recycler);

    Js::EntryPointPolymorphicInlineCacheInfo * EnsurePolymorphicInlineCacheInfo(Recycler * recycler, Js::FunctionBody * functionBody);
    Js::EntryPointPolymorphicInlineCacheInfo * GetPolymorphicInlineCacheInfo() { return polymorphicInlineCacheInfo; }

    void RegisterConstructorCache(Js::ConstructorCache* constructorCache, Recycler* recycler);
#if DBG
    uint GetConstructorCacheCount() const { return this->constructorCaches != nullptr ? this->constructorCaches->Count() : 0; }
#endif

    void PinTypeRefs(Recycler * recycler, size_t count, void ** typeRefs);

    Js::PropertyGuard* RegisterSharedPropertyGuard(Js::PropertyId propertyId, Js::ScriptContext* scriptContext);
    Js::PropertyId* GetSharedPropertyGuards(Recycler * recycler, _Out_ unsigned int& count);
    bool TryGetSharedPropertyGuard(Js::PropertyId propertyId, Js::PropertyGuard*& guard);

    Js::EquivalentTypeCache * EnsureEquivalentTypeCache(int guardCount, Js::ScriptContext * scriptContext, Js::EntryPointInfo * entryPointInfo);
    bool ClearEquivalentTypeCaches(Recycler * recycler);

    Field(Js::FakePropertyGuardWeakReference*) * EnsurePropertyGuardWeakRefs(int guardCount, Recycler * recycler);

    Js::SmallSpanSequence * GetNativeThrowSpanSequence() { return this->nativeThrowSpanSequence; }
    void SetNativeThrowSpanSequence(Js::SmallSpanSequence * seq) { this->nativeThrowSpanSequence = seq; }
    uint GetFrameHeight() { return frameHeight; }
    void SetFrameHeight(uint frameHeight) { this->frameHeight = frameHeight; }
    uint32 GetPendingPolymorphicCacheState() const { return this->pendingPolymorphicCacheState; }
    void SetPendingPolymorphicCacheState(uint32 state) { this->pendingPolymorphicCacheState = state; }
    BYTE GetPendingInlinerVersion() const { return this->pendingInlinerVersion; }
    void SetPendingInlinerVersion(BYTE version) { this->pendingInlinerVersion = version; }
    Js::ImplicitCallFlags GetPendingImplicitCallFlags() const { return this->pendingImplicitCallFlags; }
    void SetPendingImplicitCallFlags(Js::ImplicitCallFlags flags) { this->pendingImplicitCallFlags = flags; }
   
    void Cleanup(Js::ScriptContext * scriptContext, bool isShutdown, bool reset);
    void ClearTypeRefsAndGuards(Js::ScriptContext * scriptContext);   

#if PDATA_ENABLED
    XDataAllocation* GetXDataInfo() { return this->xdataInfo; }
    void CleanupXDataInfo();
    void SetXDataInfo(XDataAllocation* xdataInfo) { this->xdataInfo = xdataInfo; }
#endif
private:
    void RegisterEquivalentTypeCaches(Js::ScriptContext * scriptContext, Js::EntryPointInfo * entryPointInfo);
    void UnregisterEquivalentTypeCaches(Js::ScriptContext * scriptContext);
    void FreePropertyGuards();

    void FreeNativeCode(Js::ScriptContext * scriptContext, bool isShutdown);

    FieldNoBarrier(Js::JavascriptMethod) nativeAddress;
    FieldNoBarrier(Js::JavascriptMethod) thunkAddress;
    Field(ptrdiff_t) codeSize;
    Field(void*) validationCookie;

    // This field holds any recycler allocated references that must be kept alive until
    // we install the entry point.  It is freed at that point, so anything that must survive
    // until the EntryPointInfo itself goes away, must be copied somewhere else.
    Field(JitTransferData*) jitTransferData;

    typedef JsUtil::BaseHashSet<RecyclerWeakReference<Js::FunctionBody>*, Recycler, PowerOf2SizePolicy> WeakFuncRefSet;
    Field(WeakFuncRefSet *) weakFuncRefSet;

    // Need to keep strong references to the guards here so they don't get collected while the entry point is alive.
    typedef JsUtil::BaseDictionary<Js::PropertyId, Js::PropertyGuard*, Recycler, PowerOf2SizePolicy> SharedPropertyGuardDictionary;
    Field(SharedPropertyGuardDictionary*) sharedPropertyGuards;

    typedef SListCounted<Js::ConstructorCache*, Recycler> ConstructorCacheList;
    Field(ConstructorCacheList*) constructorCaches;

    Field(Js::EntryPointPolymorphicInlineCacheInfo *) polymorphicInlineCacheInfo;

    // If we pin types this array contains strong references to types, otherwise it holds weak references.
    Field(Field(void*)*) runtimeTypeRefs;

    // This array holds fake weak references to type property guards. We need it to zero out the weak references when the
    // entry point is finalized and the guards are about to be freed. Otherwise, if one of the guards was to be invalidated
    // from the thread context, we would AV trying to access freed memory. Note that the guards themselves are allocated by
    // NativeCodeData::Allocator and are kept alive by the data field. The weak references are recycler allocated, and so
    // the array must be recycler allocated also, so that the recycler doesn't collect the weak references.
    Field(Field(Js::FakePropertyGuardWeakReference*)*) propertyGuardWeakRefs;
    Field(Js::EquivalentTypeCache*) equivalentTypeCaches;
    Field(Js::EntryPointInfo **) registeredEquivalentTypeCacheRef;

    FieldNoBarrier(Js::SmallSpanSequence *) nativeThrowSpanSequence;

#if PDATA_ENABLED
    Field(XDataAllocation *) xdataInfo;
#endif

    Field(int) propertyGuardCount;
    Field(int) equivalentTypeCacheCount;

    Field(uint) frameHeight;

    // TODO: these only applies to FunctionEntryPointInfo
    Field(BYTE)                pendingInlinerVersion;
    Field(Js::ImplicitCallFlags) pendingImplicitCallFlags;
    Field(uint32)              pendingPolymorphicCacheState;

#if DBG_DUMP || defined(VTUNE_PROFILING)    
public:
    // NativeOffsetMap is public for DBG_DUMP, private for VTUNE_PROFILING
    struct NativeOffsetMap
    {
        uint32 statementIndex;
        regex::Interval nativeOffsetSpan;
    };
    typedef JsUtil::List<NativeOffsetMap, HeapAllocator> NativeOffsetMapListType;
    NativeOffsetMapListType& GetNativeOffsetMaps() { return nativeOffsetMaps; }
private:    
    Field(NativeOffsetMapListType) nativeOffsetMaps;
#endif
};

class InProcNativeEntryPointData : public NativeEntryPointData
{
public:
    InProcNativeEntryPointData();

    void SetNativeCodeData(NativeCodeData * nativeCodeData);

    InlineeFrameMap * GetInlineeFrameMap();
    void RecordInlineeFrameMap(JsUtil::List<NativeOffsetInlineeFramePair, ArenaAllocator>* tempInlineeFrameMap);

    BailOutRecordMap * GetBailOutRecordMap();
    void RecordBailOutMap(JsUtil::List<LazyBailOutRecord, ArenaAllocator>* bailoutMap);

#if !FLOATVAR
    void SetNumberChunks(CodeGenNumberChunk* chunks)
    {
        numberChunks = chunks;
    }
#endif
    void OnCleanup();
private:
    FieldNoBarrier(NativeCodeData *) nativeCodeData;
    FieldNoBarrier(InlineeFrameMap*) inlineeFrameMap;
    FieldNoBarrier(BailOutRecordMap*) bailoutRecordMap;
#if !FLOATVAR
    Field(CodeGenNumberChunk*) numberChunks;
#endif
};

class OOPNativeEntryPointData : public NativeEntryPointData
{
public:
    OOPNativeEntryPointData();

    static uint32 GetOffsetOfNativeDataBuffer();
    static void DeleteNativeDataBuffer(char * naitveDataBuffer);

    char* GetNativeDataBuffer();
    char** GetNativeDataBufferRef();
    void SetNativeDataBuffer(char *);

    uint GetInlineeFrameOffsetArrayOffset();
    uint GetInlineeFrameOffsetArrayCount();
    void RecordInlineeFrameOffsetsInfo(unsigned int offsetsArrayOffset, unsigned int offsetsArrayCount);

#if !FLOATVAR
    void ProcessNumberPageSegments(Js::ScriptContext * scriptContext);
    void SetNumberPageSegment(XProcNumberPageSegment * segments)
    {
        Assert(numberPageSegments == nullptr);
        numberPageSegments = segments;
    }
#endif

    void OnCleanup();
private:
    Field(uint) inlineeFrameOffsetArrayOffset;
    Field(uint) inlineeFrameOffsetArrayCount;
    FieldNoBarrier(char *) nativeDataBuffer;

#if !FLOATVAR
    Field(Field(Js::JavascriptNumber*)*) numberArray;
    Field(XProcNumberPageSegment*) numberPageSegments;
#endif
};

#endif