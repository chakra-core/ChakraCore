//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class EntryPointInfo;
    class JitEquivalentTypeGuard;
    struct CtorCacheGuardTransferEntry;
    struct TypeGuardTransferEntry;
};

struct TypeGuardTransferData
{
    Field(unsigned int) propertyGuardCount;
    FieldNoBarrier(TypeGuardTransferEntryIDL*) entries;
};

struct CtorCacheTransferData
{
    Field(unsigned int) ctorCachesCount;
    FieldNoBarrier(CtorCacheTransferEntryIDL **) entries;
};

class JitTransferData
{
    friend Js::EntryPointInfo;

private:
    typedef JsUtil::BaseHashSet<void*, Recycler, PowerOf2SizePolicy> TypeRefSet;
    Field(TypeRefSet*) jitTimeTypeRefs;

    Field(PinnedTypeRefsIDL*) runtimeTypeRefs;

    Field(int) propertyGuardCount;
    // This is a dynamically sized array of dynamically sized TypeGuardTransferEntries.  It's heap allocated by the JIT
    // thread and lives until entry point is installed, at which point it is explicitly freed.
    FieldNoBarrier(Js::TypeGuardTransferEntry*) propertyGuardsByPropertyId;
    Field(size_t) propertyGuardsByPropertyIdPlusSize;

    // This is a dynamically sized array of dynamically sized CtorCacheGuardTransferEntry.  It's heap allocated by the JIT
    // thread and lives until entry point is installed, at which point it is explicitly freed.
    FieldNoBarrier(Js::CtorCacheGuardTransferEntry*) ctorCacheGuardsByPropertyId;
    Field(size_t) ctorCacheGuardsByPropertyIdPlusSize;

    Field(int) equivalentTypeGuardCount;
    Field(int) lazyBailoutPropertyCount;
    // This is a dynamically sized array of JitEquivalentTypeGuards. It's heap allocated by the JIT thread and lives
    // until entry point is installed, at which point it is explicitly freed. We need it during installation so as to
    // swap the cache associated with each guard from the heap to the recycler (so the types in the cache are kept alive).
    FieldNoBarrier(Js::JitEquivalentTypeGuard**) equivalentTypeGuards;
    FieldNoBarrier(Js::PropertyId*) lazyBailoutProperties;
    FieldNoBarrier(NativeCodeData*) jitTransferRawData;
    FieldNoBarrier(EquivalentTypeGuardOffsets*) equivalentTypeGuardOffsets;
    Field(TypeGuardTransferData) typeGuardTransferData;
    Field(CtorCacheTransferData) ctorCacheTransferData;

    Field(bool) falseReferencePreventionBit;
    Field(bool) isReady;

public:
    JitTransferData() :
        jitTimeTypeRefs(nullptr), runtimeTypeRefs(nullptr),
        propertyGuardCount(0), propertyGuardsByPropertyId(nullptr), propertyGuardsByPropertyIdPlusSize(0),
        ctorCacheGuardsByPropertyId(nullptr), ctorCacheGuardsByPropertyIdPlusSize(0),
        equivalentTypeGuardCount(0), equivalentTypeGuards(nullptr), jitTransferRawData(nullptr),
        falseReferencePreventionBit(true), isReady(false), lazyBailoutProperties(nullptr), lazyBailoutPropertyCount(0) {}

    void SetRawData(NativeCodeData* rawData) { jitTransferRawData = rawData; }

    void AddJitTimeTypeRef(void* typeRef, Recycler* recycler);

    int GetRuntimeTypeRefCount() { return this->runtimeTypeRefs ? this->runtimeTypeRefs->count : 0; }
    void** GetRuntimeTypeRefs() { return this->runtimeTypeRefs ? (void**)this->runtimeTypeRefs->typeRefs : nullptr; }
    void SetRuntimeTypeRefs(PinnedTypeRefsIDL* pinnedTypeRefs) { this->runtimeTypeRefs = pinnedTypeRefs; }

    Js::JitEquivalentTypeGuard** GetEquivalentTypeGuards() const { return this->equivalentTypeGuards; }
    void SetEquivalentTypeGuards(Js::JitEquivalentTypeGuard** guards, int count)
    {
        this->equivalentTypeGuardCount = count;
        this->equivalentTypeGuards = guards;
    }
    void SetLazyBailoutProperties(Js::PropertyId* properties, int count)
    {
        this->lazyBailoutProperties = properties;
        this->lazyBailoutPropertyCount = count;
    }
    void SetEquivalentTypeGuardOffsets(EquivalentTypeGuardOffsets* offsets)
    {
        equivalentTypeGuardOffsets = offsets;
    }
    void SetTypeGuardTransferData(JITOutputIDL* data)
    {
        typeGuardTransferData.entries = data->typeGuardEntries;
        typeGuardTransferData.propertyGuardCount = data->propertyGuardCount;
    }
    void SetCtorCacheTransferData(JITOutputIDL * data)
    {
        ctorCacheTransferData.entries = data->ctorCacheEntries;
        ctorCacheTransferData.ctorCachesCount = data->ctorCachesCount;
    }

    bool GetIsReady() { return this->isReady; }
    void SetIsReady() { this->isReady = true; }

    void RecordTypeGuards(int propertyGuardCount, Js::TypeGuardTransferEntry* typeGuardTransferRecord, size_t typeGuardTransferPlusSize);
    void RecordCtorCacheGuards(Js::CtorCacheGuardTransferEntry* ctorCacheTransferRecord, size_t ctorCacheTransferPlusSize);

    void Cleanup();
private:
    void EnsureJitTimeTypeRefs(Recycler* recycler);
};
