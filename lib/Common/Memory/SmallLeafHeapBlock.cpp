//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

template <class TBlockAttributes>
SmallLeafHeapBlockT<TBlockAttributes> *
SmallLeafHeapBlockT<TBlockAttributes>::New(HeapBucketT<SmallLeafHeapBlockT<TBlockAttributes>> * bucket)
{
    CompileAssert(TBlockAttributes::MaxObjectSize <= USHRT_MAX);
    Assert(bucket->sizeCat <= TBlockAttributes::MaxObjectSize);
    Assert((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / bucket->sizeCat <= USHRT_MAX);

    ushort objectSize = (ushort)bucket->sizeCat;
    ushort objectCount = (ushort)((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / objectSize);
    return NoMemProtectHeapNewNoThrowPlusPrefixZ(Base::GetAllocPlusSize(objectCount), SmallLeafHeapBlockT<TBlockAttributes>, bucket, objectSize, objectCount);
}

template <>
SmallLeafHeapBlockT<SmallAllocationBlockAttributes>::SmallLeafHeapBlockT(HeapBucketT<SmallLeafHeapBlockT<SmallAllocationBlockAttributes>> * bucket, ushort objectSize, ushort objectCount)
    : Base(bucket, objectSize, objectCount, HeapBlock::HeapBlockType::SmallLeafBlockType)
{
    Assert(objectCount > 1 && objectCount == (this->GetPageCount() * AutoSystemInfo::PageSize)/ objectSize);
}

template <>
SmallLeafHeapBlockT<MediumAllocationBlockAttributes>::SmallLeafHeapBlockT(HeapBucketT<SmallLeafHeapBlockT<MediumAllocationBlockAttributes>> * bucket, ushort objectSize, ushort objectCount)
    : Base(bucket, objectSize, objectCount, HeapBlock::HeapBlockType::MediumLeafBlockType)
{
    Assert(objectCount > 1 && objectCount == (this->GetPageCount() * AutoSystemInfo::PageSize) / objectSize);
}

template <class TBlockAttributes>
void
SmallLeafHeapBlockT<TBlockAttributes>::Delete(SmallLeafHeapBlockT<TBlockAttributes> * heapBlock)
{
    Assert(heapBlock->IsLeafBlock());
    NoMemProtectHeapDeletePlusPrefix(Base::GetAllocPlusSize(heapBlock->objectCount), heapBlock);
}

template <class TBlockAttributes>
void
SmallLeafHeapBlockT<TBlockAttributes>::ScanNewImplicitRoots(Recycler * recycler)
{
    Base::ScanNewImplicitRootsBase([](void * object, size_t objectSize){});
}

#ifdef RECYCLER_SLOW_CHECK_ENABLED
template <class TBlockAttributes>
bool
SmallLeafHeapBlockT<TBlockAttributes>::GetFreeObjectListOnAllocator(FreeObject ** freeObjectList)
{
    return this->template GetFreeObjectListOnAllocatorImpl<SmallLeafHeapBlockT<TBlockAttributes>>(freeObjectList);
}
#endif

namespace Memory
{
    // Declare the class templates
    template class SmallLeafHeapBlockT<SmallAllocationBlockAttributes>;
    template class SmallLeafHeapBlockT<MediumAllocationBlockAttributes>;
}
