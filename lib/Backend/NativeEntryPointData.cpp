//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

#if ENABLE_NATIVE_CODEGEN
#include "NativeEntryPointData.h"
#include "JitTransferData.h"

using namespace Js;

NativeEntryPointData::NativeEntryPointData() :
    runtimeTypeRefs(nullptr), propertyGuardCount(0), propertyGuardWeakRefs(nullptr), polymorphicInlineCacheInfo(nullptr),
    equivalentTypeCacheCount(0), equivalentTypeCaches(nullptr), registeredEquivalentTypeCacheRef(nullptr),
    sharedPropertyGuards(nullptr), constructorCaches(nullptr), weakFuncRefSet(nullptr), jitTransferData(nullptr),
    nativeThrowSpanSequence(nullptr), codeSize(0), nativeAddress(nullptr), thunkAddress(nullptr), validationCookie(nullptr)
#if PDATA_ENABLED
    , xdataInfo(nullptr)
#endif
#if DBG_DUMP | defined(VTUNE_PROFILING)
    , nativeOffsetMaps(&HeapAllocator::Instance)
#endif
{
}

JitTransferData* 
NativeEntryPointData::EnsureJitTransferData(Recycler* recycler)
{
    if (this->jitTransferData == nullptr)
    {
        this->jitTransferData = RecyclerNew(recycler, JitTransferData);
    }
    return this->jitTransferData;
}

void
NativeEntryPointData::FreeJitTransferData()
{
    JitTransferData * data = this->jitTransferData;
    if (data != nullptr)
    {
        this->jitTransferData = nullptr;
        data->Cleanup();
    }
}

void
NativeEntryPointData::RecordNativeCode(Js::JavascriptMethod thunkAddress, Js::JavascriptMethod nativeAddress, ptrdiff_t codeSize, void * validationCookie)
{
    Assert(this->thunkAddress == nullptr);
    Assert(this->nativeAddress == nullptr);
    Assert(this->codeSize == 0);
    Assert(this->validationCookie == nullptr);

    this->nativeAddress = nativeAddress;
    this->thunkAddress = thunkAddress;
    this->codeSize = codeSize;
    this->validationCookie = validationCookie;
}

void
NativeEntryPointData::FreeNativeCode(ScriptContext * scriptContext, bool isShutdown)
{
#if PDATA_ENABLED
    this->CleanupXDataInfo();
#endif

    // if scriptContext is shutting down, no need to free that native code
    if (!isShutdown && this->nativeAddress != nullptr)
    {
        // In the debugger case, we might call cleanup after the native code gen that
        // allocated this entry point has already shutdown. In that case, the validation
        // check below should fail and we should not try to free this entry point
        // since it's already been freed
        Assert(this->validationCookie != nullptr);
        if (this->validationCookie == scriptContext->GetNativeCodeGenerator())
        {
            FreeNativeCodeGenAllocation(scriptContext, this->nativeAddress, this->thunkAddress);
        }
    }

#ifdef PERF_COUNTERS
    PERF_COUNTER_SUB(Code, TotalNativeCodeSize, codeSize);
    PERF_COUNTER_SUB(Code, FunctionNativeCodeSize,codeSize);
    PERF_COUNTER_SUB(Code, DynamicNativeCodeSize, codeSize);
#endif

    this->nativeAddress = nullptr;
    this->thunkAddress = nullptr;
    this->codeSize = 0;
}

void
NativeEntryPointData::SetTJNativeAddress(Js::JavascriptMethod nativeAddress, void * validationCookie)
{
    Assert(this->nativeAddress == nullptr);
    Assert(this->validationCookie == nullptr);
    this->nativeAddress = nativeAddress;
    this->validationCookie = validationCookie;
}

void 
NativeEntryPointData::SetTJCodeSize(ptrdiff_t codeSize)
{
    Assert(this->codeSize == 0);
    this->codeSize = codeSize;
}

void 
NativeEntryPointData::AddWeakFuncRef(RecyclerWeakReference<FunctionBody> *weakFuncRef, Recycler *recycler)
{
    NativeEntryPointData::WeakFuncRefSet * weakFuncRefSet = this->weakFuncRefSet;
    if (weakFuncRefSet == nullptr)
    {
        weakFuncRefSet = RecyclerNew(recycler, WeakFuncRefSet, recycler);
        this->weakFuncRefSet = weakFuncRefSet;
    }
    weakFuncRefSet->AddNew(weakFuncRef);
}

EntryPointPolymorphicInlineCacheInfo * 
NativeEntryPointData::EnsurePolymorphicInlineCacheInfo(Recycler * recycler, FunctionBody * functionBody)
{
    if (!polymorphicInlineCacheInfo)
    {
        polymorphicInlineCacheInfo = RecyclerNew(recycler, EntryPointPolymorphicInlineCacheInfo, functionBody);
    }
    return polymorphicInlineCacheInfo;
}

void 
NativeEntryPointData::RegisterConstructorCache(Js::ConstructorCache* constructorCache, Recycler* recycler)
{
    Assert(constructorCache != nullptr);

    if (!this->constructorCaches)
    {
        this->constructorCaches = RecyclerNew(recycler, ConstructorCacheList, recycler);
    }

    this->constructorCaches->Prepend(constructorCache);
}

void
NativeEntryPointData::PinTypeRefs(Recycler * recycler, size_t jitPinnedTypeRefCount, void ** jitPinnedTypeRefs)
{
    this->runtimeTypeRefs = RecyclerNewArray(recycler, Field(void*), jitPinnedTypeRefCount + 1);
    // Can't use memcpy here because we need write barrier triggers.
    // TODO: optimize this by using Memory::CopyArray instead.
    for (size_t i = 0; i < jitPinnedTypeRefCount; i++)
    {
        this->runtimeTypeRefs[i] = jitPinnedTypeRefs[i];
    }
    this->runtimeTypeRefs[jitPinnedTypeRefCount] = nullptr;
}

PropertyGuard* NativeEntryPointData::RegisterSharedPropertyGuard(Js::PropertyId propertyId, ScriptContext* scriptContext)
{
    if (this->sharedPropertyGuards == nullptr)
    {
        Recycler* recycler = scriptContext->GetRecycler();
        this->sharedPropertyGuards = RecyclerNew(recycler, SharedPropertyGuardDictionary, recycler);
    }

    PropertyGuard* guard = nullptr;
    if (!this->sharedPropertyGuards->TryGetValue(propertyId, &guard))
    {
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        guard = threadContext->RegisterSharedPropertyGuard(propertyId);
        this->sharedPropertyGuards->Add(propertyId, guard);
    }
    return guard;
}

Js::PropertyId* NativeEntryPointData::GetSharedPropertyGuards(Recycler * recycler, _Out_ unsigned int& count)
{
    Js::PropertyId* sharedPropertyGuards = nullptr;
    unsigned int guardCount = 0;

    if (this->sharedPropertyGuards != nullptr)
    {
        const unsigned int sharedPropertyGuardsCount = (unsigned int)this->sharedPropertyGuards->Count();
        Js::PropertyId* guards = RecyclerNewArray(recycler, Js::PropertyId, sharedPropertyGuardsCount);
        auto sharedGuardIter = this->sharedPropertyGuards->GetIterator();

        while (sharedGuardIter.IsValid())
        {
            AnalysisAssert(guardCount < sharedPropertyGuardsCount);
            guards[guardCount] = sharedGuardIter.CurrentKey();
            sharedGuardIter.MoveNext();
            ++guardCount;
        }
        AnalysisAssert(guardCount == sharedPropertyGuardsCount);

        sharedPropertyGuards = guards;
    }

    count = guardCount;
    return sharedPropertyGuards;
}

bool NativeEntryPointData::TryGetSharedPropertyGuard(Js::PropertyId propertyId, Js::PropertyGuard*& guard)
{
    return this->sharedPropertyGuards != nullptr ? this->sharedPropertyGuards->TryGetValue(propertyId, &guard) : false;
}

EquivalentTypeCache *
NativeEntryPointData::EnsureEquivalentTypeCache(int guardCount, ScriptContext * scriptContext, EntryPointInfo * entryPointInfo)
{
    Assert(this->equivalentTypeCacheCount == 0);
    Assert(this->equivalentTypeCaches == nullptr);

    // Create an array of equivalent type caches on the entry point info to ensure they are kept
    // alive for the lifetime of the entry point.

    // No need to zero-initialize, since we will populate all data slots.
    // We used to let the recycler scan the types in the cache, but we no longer do. See
    // ThreadContext::ClearEquivalentTypeCaches for an explanation.
    this->equivalentTypeCaches = RecyclerNewArrayLeafZ(scriptContext->GetRecycler(), EquivalentTypeCache, guardCount);
    this->equivalentTypeCacheCount = guardCount;
    this->RegisterEquivalentTypeCaches(scriptContext, entryPointInfo);
    return this->equivalentTypeCaches;
}

bool
NativeEntryPointData::ClearEquivalentTypeCaches(Recycler * recycler)
{
    Assert(this->equivalentTypeCaches != nullptr);
    Assert(this->equivalentTypeCacheCount > 0);

    bool isAnyCacheLive = false;
    for (EquivalentTypeCache *cache = this->equivalentTypeCaches;
        cache < this->equivalentTypeCaches + this->equivalentTypeCacheCount;
        cache++)
    {
        bool isCacheLive = cache->ClearUnusedTypes(recycler);
        if (isCacheLive)
        {
            isAnyCacheLive = true;
        }
    }

    if (!isAnyCacheLive)
    {
        // The caller must take care of unregistering this entry point. We may be in the middle of
        // walking the list of registered entry points.
        this->equivalentTypeCaches = nullptr;
        this->equivalentTypeCacheCount = 0;
        this->registeredEquivalentTypeCacheRef = nullptr;
    }

    return isAnyCacheLive;
}

Field(FakePropertyGuardWeakReference*) *
NativeEntryPointData::EnsurePropertyGuardWeakRefs(int guardCount, Recycler * recycler)
{
    Assert(this->propertyGuardCount == 0);
    Assert(this->propertyGuardWeakRefs == nullptr);
    this->propertyGuardWeakRefs = RecyclerNewArrayZ(recycler, Field(FakePropertyGuardWeakReference*), guardCount);
    this->propertyGuardCount = guardCount;

    return this->propertyGuardWeakRefs;
}
void
NativeEntryPointData::RegisterEquivalentTypeCaches(ScriptContext * scriptContext, EntryPointInfo * entryPointInfo)
{
    Assert(this->registeredEquivalentTypeCacheRef == nullptr);
    this->registeredEquivalentTypeCacheRef = scriptContext->GetThreadContext()->RegisterEquivalentTypeCacheEntryPoint(entryPointInfo);
}

void
NativeEntryPointData::UnregisterEquivalentTypeCaches(ScriptContext * scriptContext)
{
    EntryPointInfo ** registeredEquivalentTypeCacheRef = this->registeredEquivalentTypeCacheRef;

    if (registeredEquivalentTypeCacheRef != nullptr)
    {
        if (scriptContext != nullptr)
        {
            scriptContext->GetThreadContext()->UnregisterEquivalentTypeCacheEntryPoint(
                registeredEquivalentTypeCacheRef);
        }
        this->registeredEquivalentTypeCacheRef = nullptr;
    }
}

void
NativeEntryPointData::FreePropertyGuards()
{
    // While typePropertyGuardWeakRefs are allocated via NativeCodeData::Allocator and will be automatically freed to the heap,
    // we must zero out the fake weak references so that property guard invalidation doesn't access freed memory.
    if (this->propertyGuardWeakRefs != nullptr)
    {
        for (int i = 0; i < this->propertyGuardCount; i++)
        {
            if (this->propertyGuardWeakRefs[i] != nullptr)
            {
                this->propertyGuardWeakRefs[i]->Zero();
            }
        }
        this->propertyGuardCount = 0;
        this->propertyGuardWeakRefs = nullptr;
    }
}

void 
NativeEntryPointData::ClearTypeRefsAndGuards(ScriptContext * scriptContext)
{
    this->runtimeTypeRefs = nullptr;
    this->FreePropertyGuards();
    this->equivalentTypeCacheCount = 0;
    this->equivalentTypeCaches = nullptr;
    this->UnregisterEquivalentTypeCaches(scriptContext);
}

#if PDATA_ENABLED
void
NativeEntryPointData::CleanupXDataInfo()
{
    if (this->xdataInfo != nullptr)
    {
#ifdef _WIN32
        if (this->xdataInfo->functionTable
            && !DelayDeletingFunctionTable::AddEntry(this->xdataInfo->functionTable))
        {
            DelayDeletingFunctionTable::DeleteFunctionTable(this->xdataInfo->functionTable);
        }
#endif
        XDataAllocator::Unregister(this->xdataInfo);
#if defined(_M_ARM)
        if (JITManager::GetJITManager()->IsOOPJITEnabled())
#endif
        {
            HeapDelete(this->xdataInfo);
        }
        this->xdataInfo = nullptr;
    }
}
#endif //PDATA_ENABLED

void
NativeEntryPointData::Cleanup(ScriptContext * scriptContext, bool isShutdown, bool reset)
{    
    this->FreeJitTransferData();
    this->FreeNativeCode(scriptContext, isShutdown);

    if (JITManager::GetJITManager()->IsOOPJITEnabled())
    {
        ((OOPNativeEntryPointData *)this)->OnCleanup();
    }
    else
    {
        ((InProcNativeEntryPointData *)this)->OnCleanup();
    }

    this->ClearTypeRefsAndGuards(scriptContext);

    if (this->sharedPropertyGuards != nullptr)
    {
        // Reset can be called on the background thread, don't clear it explicitly
        if (!reset)
        {
            sharedPropertyGuards->Clear();
        }
        sharedPropertyGuards = nullptr;
    }

    if (this->constructorCaches != nullptr)
    {
        // Reset can be called on the background thread, don't clear it explicitly
        if (!reset)
        {
            this->constructorCaches->Clear();
        }
        this->constructorCaches = nullptr;
    }

    this->polymorphicInlineCacheInfo = nullptr;
    this->weakFuncRefSet = nullptr;

    if (this->nativeThrowSpanSequence)
    {
        HeapDelete(this->nativeThrowSpanSequence);
        this->nativeThrowSpanSequence = nullptr;
    }

#if DBG_DUMP | defined(VTUNE_PROFILING)
    this->nativeOffsetMaps.Reset();
#endif
}

InProcNativeEntryPointData::InProcNativeEntryPointData() :
    nativeCodeData(nullptr), inlineeFrameMap(nullptr), bailoutRecordMap(nullptr)
#if !FLOATVAR
    , numberChunks(nullptr)
#endif
{
    Assert(!JITManager::GetJITManager()->IsOOPJITEnabled());
}

void
InProcNativeEntryPointData::SetNativeCodeData(NativeCodeData * data)
{
    Assert(!JITManager::GetJITManager()->IsOOPJITEnabled());
    Assert(this->nativeCodeData == nullptr);
    this->nativeCodeData = data;
}

InlineeFrameMap *
InProcNativeEntryPointData::GetInlineeFrameMap()
{
    Assert(!JITManager::GetJITManager()->IsOOPJITEnabled());
    return inlineeFrameMap;
}

void
InProcNativeEntryPointData::RecordInlineeFrameMap(JsUtil::List<NativeOffsetInlineeFramePair, ArenaAllocator>* tempInlineeFrameMap)
{
    Assert(!JITManager::GetJITManager()->IsOOPJITEnabled());
    Assert(this->inlineeFrameMap == nullptr);
    if (tempInlineeFrameMap->Count() > 0)
    {
        this->inlineeFrameMap = HeapNew(InlineeFrameMap, &HeapAllocator::Instance);
        this->inlineeFrameMap->Copy(tempInlineeFrameMap);
    }
}

BailOutRecordMap *
InProcNativeEntryPointData::GetBailOutRecordMap()
{
    Assert(!JITManager::GetJITManager()->IsOOPJITEnabled());
    return this->bailoutRecordMap;
}

void 
InProcNativeEntryPointData::RecordBailOutMap(JsUtil::List<LazyBailOutRecord, ArenaAllocator>* bailoutMap)
{
    Assert(!JITManager::GetJITManager()->IsOOPJITEnabled());
    Assert(this->bailoutRecordMap == nullptr);
    this->bailoutRecordMap = HeapNew(BailOutRecordMap, &HeapAllocator::Instance);
    this->bailoutRecordMap->Copy(bailoutMap);
}

void
InProcNativeEntryPointData::OnCleanup()
{
    Assert(!JITManager::GetJITManager()->IsOOPJITEnabled());
    if (this->nativeCodeData)
    {
        DeleteNativeCodeData(this->nativeCodeData);
        this->nativeCodeData = nullptr;
    }

    if (this->inlineeFrameMap)
    {
        HeapDelete(this->inlineeFrameMap);
        this->inlineeFrameMap = nullptr;
    }

    if (this->bailoutRecordMap)
    {
        HeapDelete(this->bailoutRecordMap);
        this->bailoutRecordMap = nullptr;
    }

#if !FLOATVAR
    this->numberChunks = nullptr;
#endif
}

OOPNativeEntryPointData::OOPNativeEntryPointData() :
    nativeDataBuffer(nullptr)
#if !FLOATVAR
    , numberArray(nullptr), numberPageSegments(nullptr)
#endif
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
}

char*
OOPNativeEntryPointData::GetNativeDataBuffer()
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    return nativeDataBuffer;
}

char**
OOPNativeEntryPointData::GetNativeDataBufferRef()
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    return &nativeDataBuffer;
}

void
OOPNativeEntryPointData::SetNativeDataBuffer(char * buffer)
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    Assert(this->nativeDataBuffer == nullptr);
    this->nativeDataBuffer = buffer;
}

uint32
OOPNativeEntryPointData::GetOffsetOfNativeDataBuffer()
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    return offsetof(OOPNativeEntryPointData, nativeDataBuffer);
}

uint
OOPNativeEntryPointData::GetInlineeFrameOffsetArrayOffset() 
{ 
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    return this->inlineeFrameOffsetArrayOffset; 
}

uint
OOPNativeEntryPointData::GetInlineeFrameOffsetArrayCount() 
{ 
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    return this->inlineeFrameOffsetArrayCount; 
}

void
OOPNativeEntryPointData::RecordInlineeFrameOffsetsInfo(unsigned int offsetsArrayOffset, unsigned int offsetsArrayCount)
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    this->inlineeFrameOffsetArrayOffset = offsetsArrayOffset;
    this->inlineeFrameOffsetArrayCount = offsetsArrayCount;
}

#if !FLOATVAR
void
OOPNativeEntryPointData::ProcessNumberPageSegments(ScriptContext * scriptContext)
{
    if (this->numberPageSegments)
    {
        this->numberArray = scriptContext->GetThreadContext()
            ->GetXProcNumberPageSegmentManager()->RegisterSegments(this->numberPageSegments);
        this->numberPageSegments = nullptr;
    }
}
#endif

void
OOPNativeEntryPointData::OnCleanup()
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    if (this->nativeDataBuffer)
    {
        DeleteNativeDataBuffer(this->nativeDataBuffer);
        this->nativeDataBuffer = nullptr;
    }

#if !FLOATVAR
    this->numberArray = nullptr;
#endif
}

void
OOPNativeEntryPointData::DeleteNativeDataBuffer(char * nativeDataBuffer)
{
    Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
    NativeDataBuffer* buffer = (NativeDataBuffer*)(nativeDataBuffer - offsetof(NativeDataBuffer, data));
    midl_user_free(buffer);
}

#endif
