//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "JitTransferData.h"

using namespace Js;

void JitTransferData::AddJitTimeTypeRef(void* typeRef, Recycler* recycler)
{
    Assert(typeRef != nullptr);
    EnsureJitTimeTypeRefs(recycler);
    this->jitTimeTypeRefs->AddNew(typeRef);
}

void JitTransferData::EnsureJitTimeTypeRefs(Recycler* recycler)
{
    if (this->jitTimeTypeRefs == nullptr)
    {
        this->jitTimeTypeRefs = RecyclerNew(recycler, TypeRefSet, recycler);
    }
}

void JitTransferData::RecordTypeGuards(int typeGuardCount, TypeGuardTransferEntry* typeGuardTransferRecord, size_t typeGuardTransferPlusSize)
{
    this->propertyGuardCount = typeGuardCount;
    this->propertyGuardsByPropertyId = typeGuardTransferRecord;
    this->propertyGuardsByPropertyIdPlusSize = typeGuardTransferPlusSize;
}

void JitTransferData::RecordCtorCacheGuards(CtorCacheGuardTransferEntry* ctorCacheTransferRecord, size_t ctorCacheTransferPlusSize)
{
    this->ctorCacheGuardsByPropertyId = ctorCacheTransferRecord;
    this->ctorCacheGuardsByPropertyIdPlusSize = ctorCacheTransferPlusSize;
}


void JitTransferData::Cleanup()
{
    // This dictionary is recycler allocated so it doesn't need to be explicitly freed.
    this->jitTimeTypeRefs = nullptr;

    if (this->lazyBailoutProperties != nullptr)
    {
        HeapDeleteArray(this->lazyBailoutPropertyCount, this->lazyBailoutProperties);
        this->lazyBailoutProperties = nullptr;
    }

    // All structures below are heap allocated and need to be freed explicitly.
    if (this->runtimeTypeRefs != nullptr)
    {
        if (this->runtimeTypeRefs->isOOPJIT)
        {
            midl_user_free(this->runtimeTypeRefs);
        }
        else
        {
            HeapDeletePlus(offsetof(PinnedTypeRefsIDL, typeRefs) + sizeof(void*)*this->runtimeTypeRefs->count - sizeof(PinnedTypeRefsIDL),
                PointerValue(this->runtimeTypeRefs));
        }
        this->runtimeTypeRefs = nullptr;
    }

    if (this->propertyGuardsByPropertyId != nullptr)
    {
        HeapDeletePlus(this->propertyGuardsByPropertyIdPlusSize, this->propertyGuardsByPropertyId);
        this->propertyGuardsByPropertyId = nullptr;
    }
    this->propertyGuardCount = 0;
    this->propertyGuardsByPropertyIdPlusSize = 0;

    if (this->ctorCacheGuardsByPropertyId != nullptr)
    {
        HeapDeletePlus(this->ctorCacheGuardsByPropertyIdPlusSize, this->ctorCacheGuardsByPropertyId);
        this->ctorCacheGuardsByPropertyId = nullptr;
    }
    this->ctorCacheGuardsByPropertyIdPlusSize = 0;

    if (this->equivalentTypeGuards != nullptr)
    {
        HeapDeleteArray(this->equivalentTypeGuardCount, this->equivalentTypeGuards);
        this->equivalentTypeGuards = nullptr;
    }
    this->equivalentTypeGuardCount = 0;

    if (this->jitTransferRawData != nullptr)
    {
        HeapDelete(this->jitTransferRawData);
        this->jitTransferRawData = nullptr;
    }

    if (this->equivalentTypeGuardOffsets)
    {
        midl_user_free(this->equivalentTypeGuardOffsets);
    }

    if (this->typeGuardTransferData.entries != nullptr)
    {
        auto next = &this->typeGuardTransferData.entries;
        while (*next)
        {
            auto current = (*next);
            *next = (*next)->next;
            midl_user_free(current);
        }
    }

    if (this->ctorCacheTransferData.entries != nullptr)
    {
        CtorCacheTransferEntryIDL ** entries = this->ctorCacheTransferData.entries;
        for (uint i = 0; i < this->ctorCacheTransferData.ctorCachesCount; ++i)
        {
            midl_user_free(entries[i]);
        }
        midl_user_free(entries);
    }
}
