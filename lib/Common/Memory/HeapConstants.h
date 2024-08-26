//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class HeapConstants
{
public:
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    static const uint MaxSmallObjectSize = 3584; // We only have small and large objects now.
    static const uint MaxObjectSizeWithMinGranularity = 768;
#else
#if defined(TARGET_32)
    static const uint MaxSmallObjectSize = 512;
#else
    static const uint MaxSmallObjectSize = 768;
#endif

#if SMALLBLOCK_MEDIUM_ALLOC
    static const uint MaxMediumObjectSize = 8 * 1024; // Maximum medium object size is 8K
#else
    static const uint MaxMediumObjectSize = 9216;
#endif
#endif

#if defined(TARGET_32)
    // Only if a pointer points to first 8k region of a large object, it will set the mark bit in the chunk->MarkBits
    // If the pointer points outside of that region, no mark bit will be set
    static const uint MaxLargeObjectMarkOffset = 8 * 1024; 
#endif

    static const uint ObjectAllocationShift = 4;        // 16
    static const uint ObjectGranularity = 1 << ObjectAllocationShift;
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    /*
    Multiplier  Alignment   # of Buckets
    1               16          48
    2               32           8
    4               64           8
    8               128          8
    16              256          4
    Total # of Buckets:         76
    */
    static const ushort TotalAlignmentBucketCount = 76;
    static const uint BucketCount = TotalAlignmentBucketCount;
    static const uint TotalSmallObjectSizeCatCount = MaxSmallObjectSize / ObjectGranularity;
    static const ushort AlignmentBucketStrides = 5;
    static const ushort AlignmentBucketMultipliers[AlignmentBucketStrides];
    static const ushort AlignmentBucketCounts[AlignmentBucketStrides];
#else
    static const uint BucketCount = (MaxSmallObjectSize >> ObjectAllocationShift);
#endif

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    static const uint MediumObjectGranularity = 256;
    static const uint MediumBucketCount = (MaxMediumObjectSize - MaxSmallObjectSize) / MediumObjectGranularity;
#endif
};

///
/// BlockAttributes are used to determine the allocation characteristics of a heap block
/// These include the number of pages to allocate, the object capacity of the block
/// and the shape of the object's bit vectors
/// Since the constants here are used while generating the ValidPointerMap constants
/// please remember to regenerate the ValidPointersMap by first switching to a dynamic VPM
/// as controlled by the USE_STATIC_VPM in HeapInfo.h, and then running GenValidPointers.cmd
///
class SmallAllocationBlockAttributes
{
public:
    static const size_t MinObjectSize = HeapConstants::ObjectGranularity;

    static const size_t PageCount = 1;
    static const size_t BitVectorCount = ((PageCount * AutoSystemInfo::PageSize) / HeapConstants::ObjectGranularity);
    static const ushort MaxAddressBit = BitVectorCount - 1;
    static const uint   BucketCount = HeapConstants::BucketCount;
    static const size_t BucketGranularity = HeapConstants::ObjectGranularity;
    static const uint   MaxObjectSize = HeapConstants::MaxSmallObjectSize;
    static const uint   MaxObjectCount = PageCount * AutoSystemInfo::PageSize / HeapConstants::ObjectGranularity;
    static const uint   MaxSmallObjectCount = MaxObjectCount;
    static const uint   ObjectCountPerPage = AutoSystemInfo::PageSize / HeapConstants::ObjectGranularity;

    // This is there for RecyclerSweep to distinguish which bucket index to use
    static const bool IsSmallBlock = true;
    static const bool IsMediumBlock = false;
    static const bool IsLargeBlock = false;

    static BOOL IsAlignedObjectSize(size_t sizeCat);
};

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
class MediumAllocationBlockAttributes
{
public:
    static const size_t PageCount = 8;
    static const size_t MinObjectSize = HeapConstants::MaxSmallObjectSize;
    static const ushort BitVectorCount = ((PageCount * AutoSystemInfo::PageSize) / HeapConstants::ObjectGranularity);
    static const size_t MaxAddressBit = (BitVectorCount - 1);
    static const uint   MaxObjectSize = HeapConstants::MaxMediumObjectSize;
    static const uint   BucketCount = HeapConstants::MediumBucketCount;
    static const size_t BucketGranularity = HeapConstants::MediumObjectGranularity;
    static const uint   MaxObjectCount = PageCount * AutoSystemInfo::PageSize / (MinObjectSize + BucketGranularity);
    static const uint   MaxSmallObjectCount = PageCount * AutoSystemInfo::PageSize / HeapConstants::ObjectGranularity;
    static const uint   ObjectCountPerPage = AutoSystemInfo::PageSize / MinObjectSize;

    // This is there for RecyclerSweep to distinguish which bucket index to use
    static const bool IsSmallBlock = false;
    static const bool IsMediumBlock = true;
    static const bool IsLargeBlock = false;

    static BOOL IsAlignedObjectSize(size_t sizeCat);
};
#endif

class LargeAllocationBlockAttributes
{
public:
    // This is there for RecyclerSweep to distinguish which bucket index to use
    static const bool IsSmallBlock = false;
    static const bool IsMediumBlock = false;
    static const bool IsLargeBlock = true;
};
