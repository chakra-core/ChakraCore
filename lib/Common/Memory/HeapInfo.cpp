//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"
#include "Memory/PageHeapBlockTypeFilter.h"
#if defined(TARGET_32)
#include "ValidPointersMap/vpm.32b.h"
#elif defined(TARGET_64)
#include "ValidPointersMap/vpm.64b.h"
#else
#error "Platform is not handled"
#endif

template _ALWAYSINLINE char* HeapInfo::RealAlloc<NoBit, false>(Recycler * recycler, size_t sizeCat, size_t size);

// The VS2013 linker treats this as a redefinition of an already
// defined constant and complains. So skip the declaration if we're compiling
// with VS2013 or below.
#if !defined(_MSC_VER) || _MSC_VER >= 1900
const uint SmallAllocationBlockAttributes::MaxSmallObjectCount;
const uint MediumAllocationBlockAttributes::MaxSmallObjectCount;
#endif

HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>  HeapInfo::smallAllocValidPointersMap;
HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes> HeapInfo::mediumAllocValidPointersMap;

template <class TBlockAttributes>
ValidPointers<TBlockAttributes>::ValidPointers(ushort const * validPointers, uint bucketIndex)
#if USE_VPM_TABLE
    : validPointers(validPointers)
#endif
{
#if !USE_VPM_TABLE
    Assert(validPointers == nullptr);
    maxObjectIndex = CalculateBucketInfo(bucketIndex, &indexPerObject);
#endif
#if DBG && USE_VPM_TABLE
    uint localIndexPerObject;
    uint localMaxObjectIndex = CalculateBucketInfo(bucketIndex, &localIndexPerObject);
    for (uint index = 0; index < TBlockAttributes::MaxSmallObjectCount; index++)
    {
        Assert(CalculateAddressIndex(index, localIndexPerObject, localMaxObjectIndex) == validPointers[index]);
        Assert(CalculateInteriorAddressIndex(index, localIndexPerObject, localMaxObjectIndex) == validPointers[index + TBlockAttributes::MaxSmallObjectCount]);
    }
#endif
}

template <class TBlockAttributes>
ushort ValidPointers<TBlockAttributes>::GetAddressIndex(uint index) const
{
    Assert(index < TBlockAttributes::MaxSmallObjectCount);
#if USE_VPM_TABLE    
    return validPointers[index];
#else
    return CalculateAddressIndex(index, indexPerObject, maxObjectIndex);
#endif
    
}

template <class TBlockAttributes>
ushort ValidPointers<TBlockAttributes>::GetInteriorAddressIndex(uint index) const
{
    Assert(index < TBlockAttributes::MaxSmallObjectCount);
#if USE_VPM_TABLE    
    return validPointers[index + TBlockAttributes::MaxSmallObjectCount];
#else
    return CalculateInteriorAddressIndex(index, indexPerObject, maxObjectIndex);
#endif
}

#if !USE_VPM_TABLE || DBG
template <class TBlockAttributes>
uint ValidPointers<TBlockAttributes>::CalculateBucketInfo(uint bucketIndex, uint * indexPerObject)
{
    uint bucketSize;
    if (TBlockAttributes::IsSmallBlock)
    {
        bucketSize = TBlockAttributes::MinObjectSize + HeapConstants::ObjectGranularity * bucketIndex;
    }
    else
    {
        bucketSize = TBlockAttributes::MinObjectSize + HeapConstants::MediumObjectGranularity * (bucketIndex + 1);
    }
    *indexPerObject = bucketSize / HeapConstants::ObjectGranularity;
    return ((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / bucketSize) * bucketSize / HeapConstants::ObjectGranularity;
}

template <class TBlockAttributes>
ushort ValidPointers<TBlockAttributes>::CalculateAddressIndex(uint index, uint indexPerObject, uint maxObjectIndex)
{
    Assert(index < TBlockAttributes::MaxSmallObjectCount);
    if (index >= maxObjectIndex)
    {
        return (ushort)-1;
    }
    uint addressIndex = index / indexPerObject;
    Assert(addressIndex < USHRT_MAX);
    if (addressIndex * indexPerObject != index)
    {
        return (ushort)-1;
    }
    return (ushort)addressIndex;
}

template <class TBlockAttributes>
ushort ValidPointers<TBlockAttributes>::CalculateInteriorAddressIndex(uint index, uint indexPerObject, uint maxObjectIndex)
{
    Assert(index < TBlockAttributes::MaxSmallObjectCount);
    if (index >= maxObjectIndex)
    {
        return (ushort)-1;
    }
    uint addressIndex = index / indexPerObject;
    Assert(addressIndex < USHRT_MAX);
    return (ushort)addressIndex;
}
#endif

#if defined(ENABLE_TEST_HOOKS) || !USE_STATIC_VPM
template <class TBlockAttributes>
void HeapInfo::ValidPointersMap<TBlockAttributes>::GenerateValidPointersMap(ValidPointersMapTable * validTable, InvalidBitsTable& invalidTable, BlockInfoMapTable& blockInfoTable)
{
    // Create the valid pointer map to be shared by the buckets.
    // Also create the invalid objects bit vector.
    ushort * buffer = &((*validTable)[0][0]);
    if (buffer)
    {
        memset(buffer, -1, sizeof(ushort) * 2 * TBlockAttributes::MaxSmallObjectCount * TBlockAttributes::BucketCount);
    }

    for (uint i = 0; i < TBlockAttributes::BucketCount; i++)
    {
        // Non-interior first
        ushort * validPointers = buffer;
        if (buffer)
        {            
            buffer += TBlockAttributes::MaxSmallObjectCount;
        }

        typename SmallHeapBlockT<TBlockAttributes>::SmallHeapBlockBitVector * invalidBitVector = &invalidTable[i];
        invalidBitVector->SetAll();

        uint bucketSize;

        if (TBlockAttributes::IsSmallBlock)
        {
            bucketSize = TBlockAttributes::MinObjectSize + HeapConstants::ObjectGranularity * i;
        }
        else
        {
            bucketSize = TBlockAttributes::MinObjectSize + HeapConstants::MediumObjectGranularity * (i + 1);
        }

        uint stride = bucketSize / HeapConstants::ObjectGranularity;
        uint maxObjectCountForBucket = ((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / bucketSize);

        BlockInfoMapRow* blockInfoRow = &blockInfoTable[i];

        memset(blockInfoRow, 0, sizeof(BlockInfoMapRow));

        for (ushort j = 0; j < maxObjectCountForBucket; j++)
        {
            if (validPointers)
            {
                validPointers[j * stride] = j;
            }

            uintptr_t objectAddress = j * bucketSize;
            Assert(objectAddress / AutoSystemInfo::PageSize < USHRT_MAX);
            ushort pageIndex = (ushort)(objectAddress / AutoSystemInfo::PageSize);

            (*blockInfoRow)[pageIndex].pageObjectCount++;
            (*blockInfoRow)[pageIndex].lastObjectIndexOnPage = max(j, (*blockInfoRow)[pageIndex].lastObjectIndexOnPage);

            invalidBitVector->Clear(j * stride);
        }

        if (buffer)
        {
            // interior pointer
            ushort * validInteriorPointers = buffer;
            buffer += TBlockAttributes::MaxSmallObjectCount;
            for (ushort j = 0; j < maxObjectCountForBucket; j++)
            {
                uint start = j * stride;
                uint end = min(start + stride, TBlockAttributes::MaxSmallObjectCount);
                for (uint k = start; k < end; k++)
                {
                    validInteriorPointers[k] = j;
                }
            }
        }
    }
}
#endif

#ifdef ENABLE_TEST_HOOKS
template <>
HRESULT HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::GenerateValidPointersMapForBlockType(FILE* file)
{
#define IfErrorGotoCleanup(result) if ((result) < 0) { hr = E_FAIL; goto cleanup; }

    Assert(file != nullptr);
    HRESULT hr = S_OK;

    // Use heap to allocate the table so we don't bloat the stack (~64k). We only use this function
    // to generate headers as part of testing.
    ValidPointersMapTable *valid = (ValidPointersMapTable *)malloc(sizeof(ValidPointersMapTable));
    InvalidBitsTable *invalid = (InvalidBitsTable *)malloc(sizeof(InvalidBitsTable));
    BlockInfoMapTable *blockMap = (BlockInfoMapTable*)malloc(sizeof(BlockInfoMapTable));

    if (valid == nullptr || invalid == nullptr || blockMap == nullptr)
    {
        hr = E_FAIL;
        goto cleanup;
    }
    GenerateValidPointersMap(valid, *invalid, *blockMap);

    IfErrorGotoCleanup(fwprintf(file, _u("#if USE_VPM_TABLE\n")))
    IfErrorGotoCleanup(fwprintf(file, _u("const ushort HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::validPointersBuffer[HeapConstants::BucketCount][HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::rowSize] = \n{\n")));
    // Generate the full buffer.
    for (unsigned i = 0; i < HeapConstants::BucketCount; ++i)
    {
        IfErrorGotoCleanup(fwprintf(file, _u("    {\n        ")));
        for (unsigned j = 0; j < rowSize; ++j)
        {
            IfErrorGotoCleanup(fwprintf(
                file,
                (j < rowSize - 1) ? _u("0x%04hX, ") : _u("0x%04hX"),
                (*valid)[i][j]));
        }
        IfErrorGotoCleanup(fwprintf(file, (i < HeapConstants::BucketCount - 1 ? _u("\n    },\n") : _u("\n    }\n"))));
    }
    IfErrorGotoCleanup(fwprintf(file, _u("};\n")));
    IfErrorGotoCleanup(fwprintf(file, _u("#endif // USE_VPM_TABLE\n\n")))

    // Generate the invalid bitvectors.
    IfErrorGotoCleanup(fwprintf(
        file,
        _u("const BVUnit HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::invalidBitsData[HeapConstants::BucketCount][SmallHeapBlockT<SmallAllocationBlockAttributes>::SmallHeapBlockBitVector::wordCount] = {\n")));
    for (unsigned i = 0; i < HeapConstants::BucketCount; ++i)
    {
        IfErrorGotoCleanup(fwprintf(file, _u("    {\n        ")));

        for (unsigned j = 0; j < (*invalid)[i].wordCount; ++j)
        {
            const char16 *format = (j < (*invalid)[i].wordCount - 1) ?
#if defined(TARGET_32)
                _u("0x%08X, ") : _u("0x%08X")
#elif defined(TARGET_64)
                _u("0x%016I64X, ") : _u("0x%016I64X")
#else
#error "Platform is not handled"
#endif
                ;
            IfErrorGotoCleanup(fwprintf(file, format, (*invalid)[i].GetRawData()[j]));
        }
        IfErrorGotoCleanup(fwprintf(file, (i < HeapConstants::BucketCount - 1 ? _u("\n    },\n") : _u("\n    }\n"))));
    }

    IfErrorGotoCleanup(fwprintf(
        file,
        _u("};\n")
        _u("// The following is used to construct the InvalidBitsTable statically without forcing BVStatic to be an aggregate\n")
        _u("const HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::InvalidBitsTable * const HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::invalidBitsBuffers =\n")
        _u("    reinterpret_cast<const HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::InvalidBitsTable *>(&HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::invalidBitsData);\n")));

    // Generate the block map table
    IfErrorGotoCleanup(fwprintf(
        file,
        _u("const SmallHeapBlockT<SmallAllocationBlockAttributes>::BlockInfo  HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>::blockInfoBuffer[SmallAllocationBlockAttributes::BucketCount][SmallAllocationBlockAttributes::PageCount] = {\n")));
    for (unsigned i = 0; i < HeapConstants::BucketCount; ++i)
    {
        IfErrorGotoCleanup(fwprintf(file, _u("    // Bucket: %u, Size: %d\n"), i, (int) (HeapConstants::ObjectGranularity + (i * SmallAllocationBlockAttributes::BucketGranularity))));
        IfErrorGotoCleanup(fwprintf(file, _u("    {\n")));

        for (unsigned j = 0; j < SmallAllocationBlockAttributes::PageCount; ++j)
        {
            IfErrorGotoCleanup(fwprintf(file, _u("        { ")));

            const char16 *format = _u("0x%04hX, 0x%04hX");
            IfErrorGotoCleanup(fwprintf(file, format, (*blockMap)[i][j].lastObjectIndexOnPage, (*blockMap)[i][j].pageObjectCount));
#if USE_FEWER_PAGES_PER_BLOCK
            IfErrorGotoCleanup(fwprintf(file, _u(" }\n")));
#else
            IfErrorGotoCleanup(fwprintf(file, (j < SmallAllocationBlockAttributes::PageCount - 1 ? _u(" },\n") : _u(" }\n"))));
#endif
        }
        IfErrorGotoCleanup(fwprintf(file, (i < HeapConstants::BucketCount - 1 ? _u("    },\n") : _u("    }\n"))));
    }

    IfErrorGotoCleanup(fwprintf(file, _u("};\n")));

cleanup:
#undef IfErrorGotoCleanup
    free(valid);
    free(invalid);
    return hr;
}

template <>
HRESULT HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::GenerateValidPointersMapForBlockType(FILE* file)
{
#define IfErrorGotoCleanup(result) if ((result) < 0) { hr = E_FAIL; goto cleanup; }

    Assert(file != nullptr);
    HRESULT hr = S_OK;

    // Use heap to allocate the table so we don't bloat the stack (~64k). We only use this function
    // to generate headers as part of testing.
    ValidPointersMapTable *valid = (ValidPointersMapTable *)malloc(sizeof(ValidPointersMapTable));
    InvalidBitsTable *invalid = (InvalidBitsTable *)malloc(sizeof(InvalidBitsTable));
    BlockInfoMapTable *blockMap = (BlockInfoMapTable *)malloc(sizeof(BlockInfoMapTable));

    if (valid == nullptr || invalid == nullptr || blockMap == nullptr)
    {
        hr = E_FAIL;
        goto cleanup;
    }
    GenerateValidPointersMap(valid, *invalid, *blockMap);

    IfErrorGotoCleanup(fwprintf(file, _u("#if USE_VPM_TABLE\n")))
    IfErrorGotoCleanup(fwprintf(file, _u("const ushort HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::validPointersBuffer[MediumAllocationBlockAttributes::BucketCount][HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::rowSize] = \n{\n")));
    // Generate the full buffer.
    for (unsigned i = 0; i < HeapConstants::MediumBucketCount; ++i)
    {
        IfErrorGotoCleanup(fwprintf(file, _u("    {\n        ")));
        for (unsigned j = 0; j < rowSize; ++j)
        {
            IfErrorGotoCleanup(fwprintf(
                file,
                (j < rowSize - 1) ? _u("0x%04hX, ") : _u("0x%04hX"),
                (*valid)[i][j]));
        }
        IfErrorGotoCleanup(fwprintf(file, (i < HeapConstants::MediumBucketCount - 1 ? _u("\n    },\n") : _u("\n    }\n"))));
    }
    IfErrorGotoCleanup(fwprintf(file, _u("};\n")));
    IfErrorGotoCleanup(fwprintf(file, _u("#endif // USE_VPM_TABLE\n\n")))

    // Generate the invalid bitvectors.
    IfErrorGotoCleanup(fwprintf(
        file,
        _u("const BVUnit HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::invalidBitsData[MediumAllocationBlockAttributes::BucketCount][SmallHeapBlockT<MediumAllocationBlockAttributes>::SmallHeapBlockBitVector::wordCount] = {\n")));
    for (unsigned i = 0; i < HeapConstants::MediumBucketCount; ++i)
    {
        IfErrorGotoCleanup(fwprintf(file, _u("    {\n        ")));

        for (unsigned j = 0; j < (*invalid)[i].wordCount; ++j)
        {
            const char16 *format = (j < (*invalid)[i].wordCount - 1) ?
#if defined(TARGET_32)
                _u("0x%08X, ") : _u("0x%08X")
#elif defined(TARGET_64)
                _u("0x%016I64X, ") : _u("0x%016I64X")
#else
#error "Platform is not handled"
#endif
                ;
            IfErrorGotoCleanup(fwprintf(file, format, (*invalid)[i].GetRawData()[j]));
        }
        IfErrorGotoCleanup(fwprintf(file, (i < HeapConstants::MediumBucketCount - 1 ? _u("\n    },\n") : _u("\n    }\n"))));
    }
    IfErrorGotoCleanup(fwprintf(
        file,
        _u("};\n")
        _u("// The following is used to construct the InvalidBitsTable statically without forcing BVStatic to be an aggregate\n")
        _u("const HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::InvalidBitsTable * const HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::invalidBitsBuffers =\n")
        _u("    reinterpret_cast<const HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::InvalidBitsTable *>(&HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::invalidBitsData);\n")));

    // Generate the block map table
    IfErrorGotoCleanup(fwprintf(
        file,
        _u("const SmallHeapBlockT<MediumAllocationBlockAttributes>::BlockInfo  HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>::blockInfoBuffer[MediumAllocationBlockAttributes::BucketCount][MediumAllocationBlockAttributes::PageCount] = {\n")));

    for (unsigned i = 0; i < HeapConstants::MediumBucketCount; ++i)
    {
        IfErrorGotoCleanup(fwprintf(file, _u("    // Bucket: %u, Size: %d\n"), i, (int)(HeapConstants::MaxSmallObjectSize + ((i + 1) * MediumAllocationBlockAttributes::BucketGranularity))));
        IfErrorGotoCleanup(fwprintf(file, _u("    {\n")));

        for (unsigned j = 0; j < MediumAllocationBlockAttributes::PageCount; ++j)
        {
            IfErrorGotoCleanup(fwprintf(file, _u("        { ")));

            const char16 *format = _u("0x%04hX, 0x%04hX");
            IfErrorGotoCleanup(fwprintf(file, format, (*blockMap)[i][j].lastObjectIndexOnPage, (*blockMap)[i][j].pageObjectCount));
            IfErrorGotoCleanup(fwprintf(file, (j < MediumAllocationBlockAttributes::PageCount - 1 ? _u(" },\n") : _u(" }\n"))));
        }
        IfErrorGotoCleanup(fwprintf(file, (i < HeapConstants::MediumBucketCount - 1 ? _u("    },\n") : _u("    }\n"))));
    }

    IfErrorGotoCleanup(fwprintf(file, _u("};\n")));

cleanup:
#undef IfErrorGotoCleanup
    free(valid);
    free(invalid);
    return hr;
}

template <class TBlockAttributes>
HRESULT HeapInfo::ValidPointersMap<TBlockAttributes>::GenerateValidPointersMapHeader(LPCWSTR vpmFullPath)
{
    Assert(vpmFullPath != nullptr);
    HRESULT hr = E_FAIL;
    FILE * file = nullptr;

    if (_wfopen_s(&file, vpmFullPath, _u("w")) == 0 && file != nullptr)
    {
        const char16 * header =
            _u("//-------------------------------------------------------------------------------------------------------\n")
            _u("// Copyright (C) Microsoft. All rights reserved.\n")
            _u("// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.\n")
            _u("//-------------------------------------------------------------------------------------------------------\n")
            _u("// Generated via jshost -GenerateValidPointersMapHeader\n")
#if defined(TARGET_32)
            _u("// Target platforms: 32bit - x86 & arm\n")
#elif defined(TARGET_64)
            _u("// Target platform: 64bit - amd64 & arm64\n")
#else
#error "Platform is not handled"
#endif
            _u("#if USE_STATIC_VPM\n")
            _u("\n");
        if (fwprintf(file, header) >= 0)
        {
            hr = ValidPointersMap<SmallAllocationBlockAttributes>::GenerateValidPointersMapForBlockType(file);
            if (SUCCEEDED(hr))
            {
                hr = ValidPointersMap<MediumAllocationBlockAttributes>::GenerateValidPointersMapForBlockType(file);
            }

            fwprintf(file, _u("#endif // USE_STATIC_VPM\n"));
        }

        fclose(file);
    }

    return hr;
}
#endif

HeapInfo::HeapInfo(AllocationPolicyManager * policyManager, Js::ConfigFlagsTable& configFlagsTable, IdleDecommitPageAllocator * leafPageAllocator) :
    recycler(nullptr),
    recyclerLeafPageAllocator(leafPageAllocator),
    recyclerPageAllocator(this, policyManager, configFlagsTable, RecyclerHeuristic::Instance.DefaultMaxFreePageCount, RecyclerHeuristic::Instance.DefaultMaxAllocPageCount),
    recyclerLargeBlockPageAllocator(this, policyManager, configFlagsTable, RecyclerHeuristic::Instance.DefaultMaxFreePageCount),
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
    recyclerWithBarrierPageAllocator(this, policyManager, configFlagsTable, RecyclerHeuristic::Instance.DefaultMaxFreePageCount, PageAllocator::DefaultMaxAllocPageCount, true),
#endif
#if ENABLE_CONCURRENT_GC
    newLeafHeapBlockList(nullptr),
    newNormalHeapBlockList(nullptr),
#ifdef RECYCLER_WRITE_BARRIER
    newNormalWithBarrierHeapBlockList(nullptr),
    newFinalizableWithBarrierHeapBlockList(nullptr),
#endif
    newFinalizableHeapBlockList(nullptr),
#ifdef RECYCLER_VISITED_HOST
    newRecyclerVisitedHostHeapBlockList(nullptr),
#endif
    newMediumLeafHeapBlockList(nullptr),
    newMediumNormalHeapBlockList(nullptr),
#ifdef RECYCLER_WRITE_BARRIER
    newMediumNormalWithBarrierHeapBlockList(nullptr),
    newMediumFinalizableWithBarrierHeapBlockList(nullptr),
#endif
#ifdef RECYCLER_VISITED_HOST
    newMediumRecyclerVisitedHostHeapBlockList(nullptr),
#endif
    newMediumFinalizableHeapBlockList(nullptr),
#endif
#ifdef RECYCLER_FINALIZE_CHECK
    liveFinalizableObjectCount(0),
    pendingDisposableObjectCount(0),
    newFinalizableObjectCount(0),
#endif
#ifdef RECYCLER_PAGE_HEAP
    pageHeapMode(PageHeapMode::PageHeapModeOff),
    isPageHeapEnabled(false),
    pageHeapBlockType(PageHeapBlockTypeFilter::PageHeapBlockTypeFilterAll),
    captureAllocCallStack(false),
    captureFreeCallStack(false),
#endif
    hasPendingTransferDisposedObjects(false)
{
#if DBG_DUMP
    recyclerPageAllocator.debugName = _u("Recycler");
    recyclerLargeBlockPageAllocator.debugName = _u("RecyclerLargeBlock");
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
    recyclerWithBarrierPageAllocator.debugName = _u("RecyclerWithBarrier");
#endif
#endif
}

HeapInfo::~HeapInfo()
{
    RECYCLER_SLOW_CHECK(this->VerifySmallHeapBlockCount());

    // Finalize all finalizable object first
    for (uint i=0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].FinalizeAllObjects();
    }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i=0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].FinalizeAllObjects();
    }
#endif

    largeObjectBucket.FinalizeAllObjects();

#if ENABLE_CONCURRENT_GC
    SmallFinalizableHeapBucket::FinalizeHeapBlockList(this->newFinalizableHeapBlockList);
    MediumFinalizableHeapBucket::FinalizeHeapBlockList(this->newMediumFinalizableHeapBlockList);
#ifdef RECYCLER_VISITED_HOST
    SmallRecyclerVisitedHostHeapBucket::FinalizeHeapBlockList(this->newRecyclerVisitedHostHeapBlockList);
    MediumRecyclerVisitedHostHeapBucket::FinalizeHeapBlockList(this->newMediumRecyclerVisitedHostHeapBlockList);
#endif
#ifdef RECYCLER_WRITE_BARRIER
    SmallFinalizableWithBarrierHeapBucket::FinalizeHeapBlockList(this->newFinalizableWithBarrierHeapBlockList);
    MediumFinalizableWithBarrierHeapBucket::FinalizeHeapBlockList(this->newMediumFinalizableWithBarrierHeapBlockList);
#endif
#endif

#ifdef RECYCLER_FINALIZE_CHECK
    Assert(liveFinalizableObjectCount == 0);
    Assert(pendingDisposableObjectCount == 0);
#endif
    // Delete the heap blocks
    Recycler * recycler = this->recycler;

#ifdef RECYCLER_SLOW_CHECK_ENABLED
    size_t largeBlockCount = this->largeObjectBucket.GetLargeHeapBlockCount(false);

    uint mediumBlockCount = 0;
#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && !SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumBlockCount += mediumHeapBuckets[i].GetLargeHeapBlockCount(false);
    }
#endif
#endif

    RECYCLER_SLOW_CHECK(Assert(this->heapBlockCount[HeapBlock::HeapBlockType::LargeBlockType] - largeBlockCount - mediumBlockCount == 0));

#if ENABLE_CONCURRENT_GC
    SmallLeafHeapBucket::DeleteHeapBlockList(this->newLeafHeapBlockList, recycler);
    SmallNormalHeapBucket::DeleteHeapBlockList(this->newNormalHeapBlockList, recycler);
#ifdef RECYCLER_WRITE_BARRIER
    SmallNormalWithBarrierHeapBucket::DeleteHeapBlockList(this->newNormalWithBarrierHeapBlockList, recycler);
    SmallFinalizableWithBarrierHeapBucket::DeleteHeapBlockList(this->newFinalizableWithBarrierHeapBlockList, recycler);
#endif
    SmallFinalizableHeapBucket::DeleteHeapBlockList(this->newFinalizableHeapBlockList, recycler);

    MediumLeafHeapBucket::DeleteHeapBlockList(this->newMediumLeafHeapBlockList, recycler);
    MediumNormalHeapBucket::DeleteHeapBlockList(this->newMediumNormalHeapBlockList, recycler);
#ifdef RECYCLER_WRITE_BARRIER
    MediumNormalWithBarrierHeapBucket::DeleteHeapBlockList(this->newMediumNormalWithBarrierHeapBlockList, recycler);
    MediumFinalizableWithBarrierHeapBucket::DeleteHeapBlockList(this->newMediumFinalizableWithBarrierHeapBlockList, recycler);
#endif
    MediumFinalizableHeapBucket::DeleteHeapBlockList(this->newMediumFinalizableHeapBlockList, recycler);

#ifdef RECYCLER_VISITED_HOST
    SmallFinalizableHeapBucket::DeleteHeapBlockList(this->newRecyclerVisitedHostHeapBlockList, recycler);
    MediumFinalizableHeapBucket::DeleteHeapBlockList(this->newMediumRecyclerVisitedHostHeapBlockList, recycler);
#endif

#endif

    // We do this here, instead of in the Recycler destructor, because the above stuff may
    // generate additional tracking events, particularly ReportUnallocated.
    // Arguably we shouldn't report these things as ReportUnallocated...
    RecyclerMemoryTracking::ReportRecyclerDestroy(recycler);
}

void
HeapInfo::Initialize(Recycler * recycler
#ifdef RECYCLER_PAGE_HEAP
    , PageHeapMode pageheapmode
    , bool captureAllocCallStack
    , bool captureFreeCallStack
#endif
)
{
    this->recycler = recycler;
#ifdef DUMP_FRAGMENTATION_STATS
    if (recycler->GetRecyclerFlagsTable().DumpFragmentationStats)
    {
        Output::Print(_u("[FRAG %d] Start"), ::GetTickCount());
    }
#endif

#ifdef RECYCLER_PAGE_HEAP
    isPageHeapEnabled = false;
    PageHeapBlockTypeFilter blockTypeFilter = PageHeapBlockTypeFilter::PageHeapBlockTypeFilterAll;
    Js::NumberRange bucketNumberRange;
    Js::NumberRange* pBucketNumberRange = &bucketNumberRange;
    if (pageheapmode == PageHeapMode::PageHeapModeOff)
    {
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        isPageHeapEnabled = recycler->GetRecyclerFlagsTable().PageHeap != PageHeapMode::PageHeapModeOff;
        pageheapmode = (PageHeapMode)recycler->GetRecyclerFlagsTable().PageHeap;
        blockTypeFilter = (PageHeapBlockTypeFilter)recycler->GetRecyclerFlagsTable().PageHeapBlockType;
        pBucketNumberRange = &recycler->GetRecyclerFlagsTable().PageHeapBucketNumber;

#else
        // @TODO in free build, use environment var or other way to enable page heap
        // currently page heap build is enable in free build but has not implemented a way to input the page heap flags.
        // if we only need page heap in free test build, just move RECYCLER_PAGE_HEAP definition into ENABLE_DEBUG_CONFIG_OPTIONS
        // in CommonDefines.h it should work
#endif
    }
    else
    {
        isPageHeapEnabled = true;
    }

#ifdef RECYCLER_PAGE_HEAP
    if (isPageHeapEnabled)
    {
        this->captureAllocCallStack = captureAllocCallStack;
        this->captureFreeCallStack = captureFreeCallStack;

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        this->captureAllocCallStack = captureAllocCallStack || recycler->GetRecyclerFlagsTable().PageHeapAllocStack;
        this->captureFreeCallStack = captureFreeCallStack || recycler->GetRecyclerFlagsTable().PageHeapFreeStack;
#endif
    }
#endif

    if (IsPageHeapEnabled())
    {
        this->pageHeapMode = pageheapmode;

        // Use one of the two modes with -PageHeap flag
        Assert(this->pageHeapMode == PageHeapMode::PageHeapModeBlockStart || this->pageHeapMode == PageHeapMode::PageHeapModeBlockEnd);

        this->pageHeapBlockType = blockTypeFilter;

        for (int i = 0; i < HeapConstants::BucketCount + HeapConstants::MediumBucketCount; i++)
        {
            if (pBucketNumberRange->InRange(i))
            {
                if (i < HeapConstants::BucketCount)
                {
                    this->smallBlockPageHeapBucketFilter.Set(i);
                }
                else
                {
                    this->mediumBlockPageHeapBucketFilter.Set(i - HeapConstants::BucketCount);
                }
            }
        }
    }
    else
    {
        // These should not be set if we're not in page heap mode
        Assert(!(captureAllocCallStack || captureFreeCallStack));
    }
#endif

    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].Initialize(this, (i + 1) << HeapConstants::ObjectAllocationShift);
    }
    RECYCLER_SLOW_CHECK(memset((void*)this->heapBlockCount, 0, sizeof(this->heapBlockCount)));

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
#if SMALLBLOCK_MEDIUM_ALLOC
        mediumHeapBuckets[i].Initialize(this, HeapConstants::MaxSmallObjectSize + ((i + 1) * HeapConstants::MediumObjectGranularity));
#else
        mediumHeapBuckets[i].Initialize(this, HeapConstants::MaxSmallObjectSize + ((i + 1) * HeapConstants::MediumObjectGranularity), true);
#endif
    }
#endif

    largeObjectBucket.Initialize(this, HeapConstants::MaxMediumObjectSize);
}

#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY) || defined(MEMSPECT_TRACKING) || defined(ETW_MEMORY_TRACKING)
void
HeapInfo::Initialize(Recycler * recycler, void(*trackNativeAllocCallBack)(Recycler *, void *, size_t)
#ifdef RECYCLER_PAGE_HEAP
, PageHeapMode pageheapmode
, bool captureAllocCallStack
, bool captureFreeCallStack
#endif
)
{
    Initialize(recycler
#ifdef RECYCLER_PAGE_HEAP
        , pageheapmode
        , captureAllocCallStack
        , captureFreeCallStack
#endif
        );

    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].GetBucket<NoBit>().GetAllocator()->SetTrackNativeAllocatedObjectCallBack(trackNativeAllocCallBack);
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].GetBucket<NoBit>().GetAllocator()->SetTrackNativeAllocatedObjectCallBack(trackNativeAllocCallBack);
    }
#endif
}
#endif

#ifdef RECYCLER_PAGE_HEAP
template bool HeapInfo::IsPageHeapEnabledForBlock<MediumAllocationBlockAttributes>(const size_t objectSize);
template bool HeapInfo::IsPageHeapEnabledForBlock<SmallAllocationBlockAttributes>(const size_t objectSize);
template bool HeapInfo::IsPageHeapEnabledForBlock<LargeAllocationBlockAttributes>(const size_t objectSize);

template <typename TBlockAttributes>
bool HeapInfo::IsPageHeapEnabledForBlock(const size_t objectSize)
{
    if (IsPageHeapEnabled())
    {
        if (TBlockAttributes::IsSmallBlock)
        {
            return smallBlockPageHeapBucketFilter.Test(GetBucketIndex(objectSize)) != 0;
        }
        else if (TBlockAttributes::IsMediumBlock)
        {
            return mediumBlockPageHeapBucketFilter.Test(GetMediumBucketIndex(objectSize)) != 0;
        }
        else
        {
            return ((byte)this->pageHeapBlockType & (byte)PageHeapBlockTypeFilter::PageHeapBlockTypeFilterLarge) != 0;
        }
    }
    return false;
}
#endif

void
HeapInfo::ResetMarks(ResetMarkFlags flags)
{
    for (uint i=0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].ResetMarks(flags);
    }
#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i=0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].ResetMarks(flags);
    }
#endif

    largeObjectBucket.ResetMarks(flags);

#if ENABLE_CONCURRENT_GC
    if ((flags & ResetMarkFlags_ScanImplicitRoot) != 0)
    {
        HeapBlockList::ForEach(newLeafHeapBlockList, [flags](SmallLeafHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });

        HeapBlockList::ForEach(newNormalHeapBlockList, [flags](SmallNormalHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });

#ifdef RECYCLER_WRITE_BARRIER
        HeapBlockList::ForEach(newNormalWithBarrierHeapBlockList, [flags](SmallNormalWithBarrierHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });

        HeapBlockList::ForEach(newFinalizableWithBarrierHeapBlockList, [flags](SmallFinalizableWithBarrierHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });
#endif

        HeapBlockList::ForEach(newFinalizableHeapBlockList, [flags](SmallNormalHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });

        HeapBlockList::ForEach(newMediumLeafHeapBlockList, [flags](MediumLeafHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });

        HeapBlockList::ForEach(newMediumNormalHeapBlockList, [flags](MediumNormalHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });

#ifdef RECYCLER_WRITE_BARRIER
        HeapBlockList::ForEach(newMediumNormalWithBarrierHeapBlockList, [flags](MediumNormalWithBarrierHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });

        HeapBlockList::ForEach(newMediumFinalizableWithBarrierHeapBlockList, [flags](MediumFinalizableWithBarrierHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });
#endif
        HeapBlockList::ForEach(newMediumFinalizableHeapBlockList, [flags](MediumNormalHeapBlock * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });
    }
#endif
}

void
HeapInfo::ScanInitialImplicitRoots()
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].ScanInitialImplicitRoots(recycler);
    }
#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].ScanInitialImplicitRoots(recycler);
    }
#endif

    largeObjectBucket.ScanInitialImplicitRoots(recycler);

#if ENABLE_CONCURRENT_GC
    // NOTE: Don't need to do newLeafHeapBlockList

    HeapBlockList::ForEach(newNormalHeapBlockList, [this](SmallNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newNormalWithBarrierHeapBlockList, [this](SmallNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

    HeapBlockList::ForEach(newFinalizableWithBarrierHeapBlockList, [this](SmallFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });
#endif

    HeapBlockList::ForEach(newFinalizableHeapBlockList, [this](SmallNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

#endif

#if ENABLE_CONCURRENT_GC
    // NOTE: Don't need to do newLeafHeapBlockList

    HeapBlockList::ForEach(newMediumNormalHeapBlockList, [this](MediumNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newMediumNormalWithBarrierHeapBlockList, [this](MediumNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

    HeapBlockList::ForEach(newMediumFinalizableWithBarrierHeapBlockList, [this](MediumFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });
#endif

    HeapBlockList::ForEach(newMediumFinalizableHeapBlockList, [this](MediumNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

#endif

}


void
HeapInfo::ScanNewImplicitRoots()
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].ScanNewImplicitRoots(recycler);
    }
#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].ScanNewImplicitRoots(recycler);
    }
#endif

    largeObjectBucket.ScanNewImplicitRoots(recycler);

#if ENABLE_CONCURRENT_GC

    // NOTE: need to do newLeafHeapBlockList to find new memory
    HeapBlockList::ForEach(newLeafHeapBlockList, [this](SmallLeafHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

    HeapBlockList::ForEach(newNormalHeapBlockList, [this](SmallNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newNormalWithBarrierHeapBlockList, [this](SmallNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

    HeapBlockList::ForEach(newFinalizableWithBarrierHeapBlockList, [this](SmallFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });
#endif

    HeapBlockList::ForEach(newFinalizableHeapBlockList, [this](SmallNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

    // NOTE: need to do newLeafHeapBlockList to find new memory
    HeapBlockList::ForEach(newMediumLeafHeapBlockList, [this](MediumLeafHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

    HeapBlockList::ForEach(newMediumNormalHeapBlockList, [this](MediumNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newMediumNormalWithBarrierHeapBlockList, [this](MediumNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

    HeapBlockList::ForEach(newMediumFinalizableWithBarrierHeapBlockList, [this](MediumFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });
#endif

    HeapBlockList::ForEach(newMediumFinalizableHeapBlockList, [this](MediumNormalHeapBlock * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

#endif
}

LargeHeapBlock *
HeapInfo::AddLargeHeapBlock(size_t size)
{
    // Do a no-throwing allocation here
    return largeObjectBucket.AddLargeHeapBlock(size, /* nothrow = */ true);
}

void
HeapInfo::Finalize(RecyclerSweep& recyclerSweep)
{
    Recycler * recycler = recyclerSweep.GetRecycler();
#ifdef RECYCLER_STATS
    memset((void*)&recycler->collectionStats.numEmptySmallBlocks, 0, sizeof(recycler->collectionStats.numEmptySmallBlocks));
    recycler->collectionStats.numZeroedOutSmallBlocks = 0;
#endif
#ifdef RECYCLER_FINALIZE_CHECK
    this->newFinalizableObjectCount = 0;
#endif
    RECYCLER_SLOW_CHECK(VerifySmallHeapBlockCount());

#if ENABLE_CONCURRENT_GC
    // Merge the new blocks before we sweep the finalizable object in thread
    recyclerSweep.MergePendingNewHeapBlockList<SmallFinalizableHeapBlock>();
    recyclerSweep.MergePendingNewMediumHeapBlockList<MediumFinalizableHeapBlock>();

#ifdef RECYCLER_WRITE_BARRIER
    recyclerSweep.MergePendingNewHeapBlockList<SmallFinalizableWithBarrierHeapBlock>();
    recyclerSweep.MergePendingNewMediumHeapBlockList<MediumFinalizableWithBarrierHeapBlock>();
#endif
#ifdef RECYCLER_VISITED_HOST
    recyclerSweep.MergePendingNewHeapBlockList<SmallRecyclerVisitedHostHeapBlock>();
    recyclerSweep.MergePendingNewMediumHeapBlockList<MediumRecyclerVisitedHostHeapBlock>();
#endif
#endif

    RECYCLER_PROFILE_EXEC_BEGIN(recycler, Js::FinalizePhase);    

    largeObjectBucket.Finalize();

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && !(SMALLBLOCK_MEDIUM_ALLOC)
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].Finalize();
    }
#endif

    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].SweepFinalizableObjects(recyclerSweep);
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    // CONCURRENT-TODO: Allow this in the background as well
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].SweepFinalizableObjects(recyclerSweep);
    }
#endif

    RECYCLER_PROFILE_EXEC_END(recycler, Js::FinalizePhase);
}

void
HeapInfo::Sweep(RecyclerSweep& recyclerSweep, bool concurrent)
{
    RECYCLER_PROFILE_EXEC_BEGIN(recyclerSweep.GetRecycler(), Js::SweepSmallPhase);

#if ENABLE_CONCURRENT_GC
    if (concurrent)
    {
        RECYCLER_SLOW_CHECK(VerifySmallHeapBlockCount());
        RECYCLER_SLOW_CHECK(VerifyLargeHeapBlockCount());
    }

    if (concurrent)
    {
        this->SetupBackgroundSweep(recyclerSweep);
    }
    else
#endif
    {
        this->SweepSmallNonFinalizable(recyclerSweep);
    }

    RECYCLER_PROFILE_EXEC_CHANGE(recyclerSweep.GetRecycler(), Js::SweepSmallPhase, Js::SweepLargePhase);

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && !(SMALLBLOCK_MEDIUM_ALLOC)
    // CONCURRENT-TODO: Allow this in the background as well
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].Sweep(recyclerSweep);
    }
#endif
    largeObjectBucket.Sweep(recyclerSweep);

    RECYCLER_PROFILE_EXEC_END(recyclerSweep.GetRecycler(), Js::SweepLargePhase);

    RECYCLER_SLOW_CHECK(VerifyLargeHeapBlockCount());
    RECYCLER_SLOW_CHECK(Assert(this->newFinalizableObjectCount == 0));
}

#if ENABLE_CONCURRENT_GC
void
HeapInfo::SetupBackgroundSweep(RecyclerSweep& recyclerSweep)
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        this->heapBuckets[i].SetupBackgroundSweep(recyclerSweep);
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        this->mediumHeapBuckets[i].SetupBackgroundSweep(recyclerSweep);
    }
#endif
}
#endif

void
HeapInfo::SweepSmallNonFinalizable(RecyclerSweep& recyclerSweep)
{
#if ENABLE_CONCURRENT_GC
    recyclerSweep.MergePendingNewHeapBlockList<SmallLeafHeapBlock>();
    recyclerSweep.MergePendingNewHeapBlockList<SmallNormalHeapBlock>();
    recyclerSweep.MergePendingNewMediumHeapBlockList<MediumLeafHeapBlock>();
    recyclerSweep.MergePendingNewMediumHeapBlockList<MediumNormalHeapBlock>();
#ifdef RECYCLER_WRITE_BARRIER
    recyclerSweep.MergePendingNewHeapBlockList<SmallNormalWithBarrierHeapBlock>();
    recyclerSweep.MergePendingNewMediumHeapBlockList<MediumNormalWithBarrierHeapBlock>();
#endif

    // Finalizable are already merge before in SweepHeap
    Assert(!recyclerSweep.HasPendingNewHeapBlocks());
#endif
    if (!recyclerSweep.IsBackground())
    {
        // finalizer may trigger arena allocations, do don't suspend the leaf (thread) page allocator
        // until  we are going to sweep leaf pages.
        this->GetRecyclerLeafPageAllocator()->SuspendIdleDecommit();
    }
    for (uint i=0; i<HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].Sweep(recyclerSweep);
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].Sweep(recyclerSweep);
    }
#endif

    if (!recyclerSweep.IsBackground())
    {
        // large block don't use the leaf page allocator, we can resume idle decommit now
        this->GetRecyclerLeafPageAllocator()->ResumeIdleDecommit();

        RECYCLER_SLOW_CHECK(VerifySmallHeapBlockCount());
        RECYCLER_SLOW_CHECK(VerifyLargeHeapBlockCount());
    }
}

size_t
HeapInfo::Rescan(RescanFlags flags)
{
    size_t scannedPageCount = 0;

    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        scannedPageCount += heapBuckets[i].Rescan(recycler, flags);
    }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
#if SMALLBLOCK_MEDIUM_ALLOC
        scannedPageCount += mediumHeapBuckets[i].Rescan(recycler, flags);
#else
        scannedPageCount += mediumHeapBuckets[i].Rescan(flags);
#endif
    }
#endif

    scannedPageCount += largeObjectBucket.Rescan(flags);

    return scannedPageCount;
}


#if ENABLE_MEM_STATS
#include "BucketStatsReporter.h"
void
HeapInfo::GetBucketStats(BucketStatsReporter& report)
{
#if ENABLE_CONCURRENT_GC
    // Pre aggregate pass on all the heap blocks that are not merged into bucket's lists yet
    report.PreAggregateBucketStats(this->newNormalHeapBlockList);
    report.PreAggregateBucketStats(this->newLeafHeapBlockList);
    report.PreAggregateBucketStats(this->newFinalizableHeapBlockList);
#ifdef RECYCLER_WRITE_BARRIER
    report.PreAggregateBucketStats(this->newNormalWithBarrierHeapBlockList);
    report.PreAggregateBucketStats(this->newFinalizableWithBarrierHeapBlockList);
#endif
#ifdef RECYCLER_VISITED_HOST
    report.PreAggregateBucketStats(this->newRecyclerVisitedHostHeapBlockList);
#endif

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    report.PreAggregateBucketStats(this->newMediumNormalHeapBlockList);
    report.PreAggregateBucketStats(this->newMediumLeafHeapBlockList);
    report.PreAggregateBucketStats(this->newMediumFinalizableHeapBlockList);
#ifdef RECYCLER_WRITE_BARRIER
    report.PreAggregateBucketStats(this->newMediumNormalWithBarrierHeapBlockList);
    report.PreAggregateBucketStats(this->newMediumFinalizableWithBarrierHeapBlockList);
#endif
#ifdef RECYCLER_VISITED_HOST
    report.PreAggregateBucketStats(this->newMediumRecyclerVisitedHostHeapBlockList);
#endif
#endif
#endif  // ENABLE_CONCURRENT_GC

    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        report.GetBucketStats<SmallAllocationBlockAttributes, NoBit>(heapBuckets[i]);
        report.GetBucketStats<SmallAllocationBlockAttributes, LeafBit>(heapBuckets[i]);
        report.GetBucketStats<SmallAllocationBlockAttributes, FinalizeBit>(heapBuckets[i]);
#ifdef RECYCLER_WRITE_BARRIER
        report.GetBucketStats<SmallAllocationBlockAttributes, WithBarrierBit>(heapBuckets[i]);
        report.GetBucketStats<SmallAllocationBlockAttributes, FinalizableWithBarrierBit>(heapBuckets[i]);
#endif
#ifdef RECYCLER_VISITED_HOST
        report.GetBucketStats<SmallAllocationBlockAttributes, RecyclerVisitedHostBit>(heapBuckets[i]);
#endif
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        report.GetBucketStats<MediumAllocationBlockAttributes, NoBit>(mediumHeapBuckets[i]);
        report.GetBucketStats<MediumAllocationBlockAttributes, LeafBit>(mediumHeapBuckets[i]);
        report.GetBucketStats<MediumAllocationBlockAttributes, FinalizeBit>(mediumHeapBuckets[i]);
#ifdef RECYCLER_WRITE_BARRIER
        report.GetBucketStats<MediumAllocationBlockAttributes, WithBarrierBit>(mediumHeapBuckets[i]);
        report.GetBucketStats<MediumAllocationBlockAttributes, FinalizableWithBarrierBit>(mediumHeapBuckets[i]);
#endif
#ifdef RECYCLER_VISITED_HOST
        report.GetBucketStats<MediumAllocationBlockAttributes, RecyclerVisitedHostBit>(mediumHeapBuckets[i]);
#endif
    }
#endif

    report.GetBucketStats(largeObjectBucket);
}

#endif  // ENABLE_MEM_STATS


#if ENABLE_PARTIAL_GC
void
HeapInfo::SweepPartialReusePages(RecyclerSweep& recyclerSweep)
{
    RECYCLER_PROFILE_EXEC_THREAD_BEGIN(recyclerSweep.IsBackground(), recyclerSweep.GetRecycler(), Js::SweepPartialReusePhase);

    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].SweepPartialReusePages(recyclerSweep);
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].SweepPartialReusePages(recyclerSweep);
    }
#endif

    RECYCLER_PROFILE_EXEC_THREAD_END(recyclerSweep.IsBackground(), recyclerSweep.GetRecycler(), Js::SweepPartialReusePhase);

    // GC-TODO: LargeHeapBlock don't reuse object, so we don't need to keep
    // pages with low free space from being reused.

    // Only count the byte that we would have freed but we are not reusing it if we are doing a partial GC
    // This will increase the GC pressure and make partial less and less likely.
    if (recyclerSweep.GetManager()->InPartialCollect())
    {
        recyclerSweep.GetRecycler()->autoHeap.unusedPartialCollectFreeBytes += recyclerSweep.GetManager()->GetPartialUnusedFreeByteCount();
    }
}

void HeapInfo::FinishPartialCollect(RecyclerSweep * recyclerSweep)
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].FinishPartialCollect(recyclerSweep);
    }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].FinishPartialCollect(recyclerSweep);
    }
#endif

    largeObjectBucket.FinishPartialCollect(recyclerSweep);
}
#endif

#if ENABLE_CONCURRENT_GC
void
HeapInfo::PrepareSweep()
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].PrepareSweep();
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].PrepareSweep();
    }
#endif
}
#endif

#if ENABLE_PARTIAL_GC || ENABLE_CONCURRENT_GC
#ifdef RECYCLER_WRITE_WATCH 
void HeapInfo::EnableWriteWatch()
{
    recyclerPageAllocator.EnableWriteWatch(); 
    recyclerLargeBlockPageAllocator.EnableWriteWatch();
}

bool HeapInfo::ResetWriteWatch()
{
    return recyclerPageAllocator.ResetWriteWatch() && recyclerLargeBlockPageAllocator.ResetWriteWatch();
}

#if DBG
size_t HeapInfo::GetWriteWatchPageCount()
{
    return this->recyclerPageAllocator.GetWriteWatchPageCount() + this->recyclerLargeBlockPageAllocator.GetWriteWatchPageCount();
}
#endif
#endif

void
HeapInfo::SweepPendingObjects(RecyclerSweep& recyclerSweep)
{
    if (recyclerSweep.HasPendingSweepSmallHeapBlocks())
    {
        for (uint i = 0; i < HeapConstants::BucketCount; i++)
        {
            heapBuckets[i].SweepPendingObjects(recyclerSweep);
        }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
        for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
        {
            mediumHeapBuckets[i].SweepPendingObjects(recyclerSweep);
        }
#endif
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && !SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].SweepPendingObjects(recyclerSweep);
    }
#endif

    largeObjectBucket.SweepPendingObjects(recyclerSweep);
}
#endif

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
void HeapInfo::StartAllocationsDuringConcurrentSweep()
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].StartAllocationDuringConcurrentSweep();
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].StartAllocationDuringConcurrentSweep();
    }
#endif
}

bool
HeapInfo::DoTwoPassConcurrentSweepPreCheck()
{
    bool enableTwoPassSweep = false;
    // We will continue to do the check for all the buckets so we can enable/disable the feature 
    // per bucket.
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        if (heapBuckets[i].DoTwoPassConcurrentSweepPreCheck())
        {
            enableTwoPassSweep = true;
        }
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        if (mediumHeapBuckets[i].DoTwoPassConcurrentSweepPreCheck())
        {
            enableTwoPassSweep = true;
        }
    }
#endif

    return enableTwoPassSweep;
}

void
HeapInfo::FinishConcurrentSweepPass1(RecyclerSweep& recyclerSweep)
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].FinishConcurrentSweepPass1(recyclerSweep);
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].FinishConcurrentSweepPass1(recyclerSweep);
    }
#endif
}

void
HeapInfo::FinishSweepPrep(RecyclerSweep& recyclerSweep)
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].FinishSweepPrep(recyclerSweep);
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].FinishSweepPrep(recyclerSweep);
    }
#endif
}

void
HeapInfo::FinishConcurrentSweep()
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].FinishConcurrentSweep();
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].FinishConcurrentSweep();
    }
#endif
}
#endif

#if ENABLE_CONCURRENT_GC
void
HeapInfo::TransferPendingHeapBlocks(RecyclerSweep& recyclerSweep)
{
    Assert(!recyclerSweep.IsBackground());
    RECYCLER_SLOW_CHECK(VerifySmallHeapBlockCount());
    if (recyclerSweep.HasPendingEmptyBlocks())
    {
        for (uint i = 0; i < HeapConstants::BucketCount; i++)
        {
            heapBuckets[i].TransferPendingEmptyHeapBlocks(recyclerSweep);
        }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
        for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
        {
            mediumHeapBuckets[i].TransferPendingEmptyHeapBlocks(recyclerSweep);
        }
#endif

        RECYCLER_SLOW_CHECK(VerifySmallHeapBlockCount());
    }

    // We might still have block that has been disposed but not made allocable
    // which happens if we finish disposing object during concurrent sweep
    // and can't modify the block lists
    recyclerSweep.FlushPendingTransferDisposedObjects();
}

void
HeapInfo::ConcurrentTransferSweptObjects(RecyclerSweep& recyclerSweep)
{
#if ENABLE_PARTIAL_GC
    Assert(!recyclerSweep.InPartialCollectMode());
#endif
    Assert(!recyclerSweep.IsBackground());
    TransferPendingHeapBlocks(recyclerSweep);

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && !SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].ConcurrentTransferSweptObjects(recyclerSweep);
    }
#endif

    largeObjectBucket.ConcurrentTransferSweptObjects(recyclerSweep);
}

#if ENABLE_PARTIAL_GC
void
HeapInfo::ConcurrentPartialTransferSweptObjects(RecyclerSweep& recyclerSweep)
{
    Assert(recyclerSweep.InPartialCollectMode());
    Assert(!recyclerSweep.IsBackground());
    TransferPendingHeapBlocks(recyclerSweep);

    RECYCLER_SLOW_CHECK(this->VerifyLargeHeapBlockCount());

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && !SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].ConcurrentPartialTransferSweptObjects(recyclerSweep);
    }
#endif

    largeObjectBucket.ConcurrentPartialTransferSweptObjects(recyclerSweep);

    RECYCLER_SLOW_CHECK(this->VerifyLargeHeapBlockCount());
}
#endif
#endif

void
HeapInfo::DisposeObjects()
{
    Recycler * recycler = this->recycler;
    do
    {
        recycler->hasDisposableObject = false;

        // finalizing the objects
        for (uint i = 0; i < HeapConstants::BucketCount; i++)
        {
            heapBuckets[i].DisposeObjects();
        }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
        for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
        {
            mediumHeapBuckets[i].DisposeObjects();
        }
#endif

        largeObjectBucket.DisposeObjects();
    }
    // Calling dispose may enter the GC again and dispose more objects, loop until we don't have any more
    while (recycler->hasDisposableObject);

    this->hasPendingTransferDisposedObjects = true;
#if ENABLE_CONCURRENT_GC
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP 
    // As during concurrent sweep we start/stop allocations it is safer to prevent transferring disposed objects altogether.
    if (!recycler->IsConcurrentExecutingState() && !recycler->IsConcurrentSweepState())
#else
    if (!recycler->IsConcurrentExecutingState())
#endif
#endif
    {
        // Can't transfer disposed object when the background thread is walking the heap block list
        // That includes reset mark, background rescan and concurrent sweep. Delay the transfer later.
        // NOTE1: During concurrent sweep,  we can't do this only if the bucket has "stopped" allocation
        // After it resume allocation, we don't walk the list in the background thread any more
        // (except for checking heap block count). But this is easier to detect via the collection state
        // without walking all buckets.
        // NOTE2: During transitive closure mark, we don't walk the heap block list, but we can continue
        // to do background rescan.  Since we don't have synchronization for that, we can't really enable
        // able this just for the transitive closure,  so just do all the background executing state.
        TransferDisposedObjects();
    }
}

void
HeapInfo::TransferDisposedObjects()
{
    Assert(this->hasPendingTransferDisposedObjects);
#if ENABLE_CONCURRENT_GC
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP 
    Assert(!this->recycler->IsConcurrentExecutingState() && !this->recycler->IsConcurrentSweepState());
#else
    Assert(!this->recycler->IsConcurrentExecutingState());
#endif
#endif
    this->hasPendingTransferDisposedObjects = false;

    // move the disposed object back to the free lists
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].TransferDisposedObjects();
    }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].TransferDisposedObjects();
    }
#endif

    largeObjectBucket.TransferDisposedObjects();
}
void
HeapInfo::EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size))
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].EnumerateObjects(infoBits, CallBackFunction);
    }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].EnumerateObjects(infoBits, CallBackFunction);
    }
#endif

    largeObjectBucket.EnumerateObjects(infoBits, CallBackFunction);

#if ENABLE_CONCURRENT_GC
    HeapBucket::EnumerateObjects(newLeafHeapBlockList, infoBits, CallBackFunction);
    HeapBucket::EnumerateObjects(newNormalHeapBlockList, infoBits, CallBackFunction);
#ifdef RECYCLER_WRITE_BARRIER
    HeapBucket::EnumerateObjects(newNormalWithBarrierHeapBlockList, infoBits, CallBackFunction);
    HeapBucket::EnumerateObjects(newFinalizableWithBarrierHeapBlockList, infoBits, CallBackFunction);
#endif

#ifdef RECYCLER_VISITED_HOST
    HeapBucket::EnumerateObjects(newRecyclerVisitedHostHeapBlockList, infoBits, CallBackFunction);
#endif
    HeapBucket::EnumerateObjects(newFinalizableHeapBlockList, infoBits, CallBackFunction);

    HeapBucket::EnumerateObjects(newMediumLeafHeapBlockList, infoBits, CallBackFunction);
    HeapBucket::EnumerateObjects(newMediumNormalHeapBlockList, infoBits, CallBackFunction);
#ifdef RECYCLER_WRITE_BARRIER
    HeapBucket::EnumerateObjects(newMediumNormalWithBarrierHeapBlockList, infoBits, CallBackFunction);
    HeapBucket::EnumerateObjects(newMediumFinalizableWithBarrierHeapBlockList, infoBits, CallBackFunction);
#endif

#ifdef RECYCLER_VISITED_HOST
    HeapBucket::EnumerateObjects(newMediumRecyclerVisitedHostHeapBlockList, infoBits, CallBackFunction);
#endif
    HeapBucket::EnumerateObjects(newMediumFinalizableHeapBlockList, infoBits, CallBackFunction);
#endif
}

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
size_t
HeapInfo::GetSmallHeapBlockCount(bool checkCount) const
{
    size_t currentSmallHeapBlockCount = 0;
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        currentSmallHeapBlockCount += heapBuckets[i].GetNonEmptyHeapBlockCount(checkCount);
        currentSmallHeapBlockCount += heapBuckets[i].GetEmptyHeapBlockCount();
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        currentSmallHeapBlockCount += mediumHeapBuckets[i].GetNonEmptyHeapBlockCount(checkCount);
        currentSmallHeapBlockCount += mediumHeapBuckets[i].GetEmptyHeapBlockCount();
    }
#endif

#if ENABLE_CONCURRENT_GC
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newLeafHeapBlockList);
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newNormalHeapBlockList);
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newFinalizableHeapBlockList);
#ifdef RECYCLER_VISITED_HOST
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newRecyclerVisitedHostHeapBlockList);
#endif
#ifdef RECYCLER_WRITE_BARRIER
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newNormalWithBarrierHeapBlockList);
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newFinalizableWithBarrierHeapBlockList);
#endif

    currentSmallHeapBlockCount += HeapBlockList::Count(this->newMediumLeafHeapBlockList);
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newMediumNormalHeapBlockList);
#ifdef RECYCLER_VISITED_HOST
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newMediumRecyclerVisitedHostHeapBlockList);
#endif
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newMediumFinalizableHeapBlockList);
#ifdef RECYCLER_WRITE_BARRIER
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newMediumNormalWithBarrierHeapBlockList);
    currentSmallHeapBlockCount += HeapBlockList::Count(this->newMediumFinalizableWithBarrierHeapBlockList);
#endif

    // TODO: Update recycler sweep
    // Recycler can be null if we have OOM in the ctor
    if (this->recycler && this->recycler->recyclerSweepManager != nullptr)
    {
        // This function can't be called in the background
        Assert(!this->recycler->recyclerSweepManager->IsBackground());
        currentSmallHeapBlockCount += this->recycler->recyclerSweepManager->GetPendingMergeNewHeapBlockCount(this);
    }
#endif
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    size_t expectedHeapBlockCount =
        this->heapBlockCount[HeapBlock::HeapBlockType::SmallNormalBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallLeafBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallFinalizableBlockType]
#ifdef RECYCLER_VISITED_HOST
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallRecyclerVisitedHostBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumRecyclerVisitedHostBlockType]
#endif
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumNormalBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumLeafBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumFinalizableBlockType];


#ifdef RECYCLER_WRITE_BARRIER
    expectedHeapBlockCount +=
        this->heapBlockCount[HeapBlock::HeapBlockType::SmallNormalBlockWithBarrierType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallFinalizableBlockWithBarrierType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumNormalBlockWithBarrierType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumFinalizableBlockWithBarrierType];

#endif
    Assert(!checkCount || currentSmallHeapBlockCount == expectedHeapBlockCount);
#endif

    return currentSmallHeapBlockCount;
}

size_t
HeapInfo::GetLargeHeapBlockCount(bool checkCount) const
{
    size_t currentLargeHeapBlockCount = 0;

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && !SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        currentLargeHeapBlockCount += mediumHeapBuckets[i].GetLargeHeapBlockCount(checkCount);
    }
#endif

    currentLargeHeapBlockCount += largeObjectBucket.GetLargeHeapBlockCount(checkCount);

    RECYCLER_SLOW_CHECK(Assert(!checkCount || currentLargeHeapBlockCount == this->heapBlockCount[HeapBlock::HeapBlockType::LargeBlockType]));
    return currentLargeHeapBlockCount;
}

#endif

#ifdef RECYCLER_SLOW_CHECK_ENABLED
void
HeapInfo::Check()
{
    Assert(!this->recycler->CollectionInProgress());

    size_t currentSmallHeapBlockCount = 0;
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        currentSmallHeapBlockCount += heapBuckets[i].Check();
        currentSmallHeapBlockCount += heapBuckets[i].GetEmptyHeapBlockCount();
    }

    size_t currentLargeHeapBlockCount = 0;
#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
#if SMALLBLOCK_MEDIUM_ALLOC
        currentSmallHeapBlockCount += mediumHeapBuckets[i].Check();
        currentSmallHeapBlockCount += mediumHeapBuckets[i].GetEmptyHeapBlockCount();
#else
        currentLargeHeapBlockCount += mediumHeapBuckets[i].Check();
#endif
    }
#endif

#if ENABLE_CONCURRENT_GC
    currentSmallHeapBlockCount += Check(true, false, this->newLeafHeapBlockList);
    currentSmallHeapBlockCount += Check(true, false, this->newNormalHeapBlockList);
#ifdef RECYCLER_WRITE_BARRIER
    currentSmallHeapBlockCount += Check(true, false, this->newNormalWithBarrierHeapBlockList);
    currentSmallHeapBlockCount += Check(true, false, this->newFinalizableWithBarrierHeapBlockList);
#endif
    currentSmallHeapBlockCount += Check(true, false, this->newFinalizableHeapBlockList);
#ifdef RECYCLER_VISITED_HOST
    currentSmallHeapBlockCount += Check(true, false, this->newRecyclerVisitedHostHeapBlockList);
    currentSmallHeapBlockCount += Check(true, false, this->newMediumRecyclerVisitedHostHeapBlockList);
#endif
#endif

#if ENABLE_CONCURRENT_GC
    currentSmallHeapBlockCount += Check(true, false, this->newMediumLeafHeapBlockList);
    currentSmallHeapBlockCount += Check(true, false, this->newMediumNormalHeapBlockList);
#ifdef RECYCLER_WRITE_BARRIER
    currentSmallHeapBlockCount += Check(true, false, this->newMediumNormalWithBarrierHeapBlockList);
    currentSmallHeapBlockCount += Check(true, false, this->newMediumFinalizableWithBarrierHeapBlockList);
#endif
    currentSmallHeapBlockCount += Check(true, false, this->newMediumFinalizableHeapBlockList);
#endif

    size_t expectedHeapBlockCount =
        this->heapBlockCount[HeapBlock::HeapBlockType::SmallNormalBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallLeafBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallFinalizableBlockType]
#ifdef RECYCLER_VISITED_HOST
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallRecyclerVisitedHostBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumRecyclerVisitedHostBlockType]
#endif
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumNormalBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumLeafBlockType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumFinalizableBlockType];

#ifdef RECYCLER_WRITE_BARRIER
    expectedHeapBlockCount +=
        this->heapBlockCount[HeapBlock::HeapBlockType::SmallNormalBlockWithBarrierType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::SmallFinalizableBlockWithBarrierType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumNormalBlockWithBarrierType]
        + this->heapBlockCount[HeapBlock::HeapBlockType::MediumFinalizableBlockWithBarrierType];

#endif

    Assert(currentSmallHeapBlockCount == expectedHeapBlockCount);

    currentLargeHeapBlockCount += largeObjectBucket.Check();

    Assert(currentLargeHeapBlockCount == this->heapBlockCount[HeapBlock::HeapBlockType::LargeBlockType]);
}

template <typename TBlockType>
size_t
HeapInfo::Check(bool expectFull, bool expectPending, TBlockType * list, TBlockType * tail)
{
    size_t heapBlockCount = 0;
    HeapBlockList::ForEach(list, tail, [&heapBlockCount, expectFull, expectPending](TBlockType * heapBlock)
    {
        heapBlock->Check(expectFull, expectPending);
        heapBlockCount++;
    });
    return heapBlockCount;
}

template size_t HeapInfo::Check<SmallNormalHeapBlock>(bool expectFull, bool expectPending, SmallNormalHeapBlock * list, SmallNormalHeapBlock * tail);
template size_t HeapInfo::Check<SmallLeafHeapBlock>(bool expectFull, bool expectPending, SmallLeafHeapBlock * list, SmallLeafHeapBlock * tail);
template size_t HeapInfo::Check<SmallFinalizableHeapBlock>(bool expectFull, bool expectPending, SmallFinalizableHeapBlock * list, SmallFinalizableHeapBlock * tail);
#ifdef RECYCLER_VISITED_HOST
template size_t HeapInfo::Check<SmallRecyclerVisitedHostHeapBlock>(bool expectFull, bool expectPending, SmallRecyclerVisitedHostHeapBlock * list, SmallRecyclerVisitedHostHeapBlock * tail);
template size_t HeapInfo::Check<MediumRecyclerVisitedHostHeapBlock>(bool expectFull, bool expectPending, MediumRecyclerVisitedHostHeapBlock * list, MediumRecyclerVisitedHostHeapBlock * tail);
#endif
template size_t HeapInfo::Check<LargeHeapBlock>(bool expectFull, bool expectPending, LargeHeapBlock * list, LargeHeapBlock * tail);
#ifdef RECYCLER_WRITE_BARRIER
template size_t HeapInfo::Check<SmallNormalWithBarrierHeapBlock>(bool expectFull, bool expectPending, SmallNormalWithBarrierHeapBlock * list, SmallNormalWithBarrierHeapBlock * tail);
template size_t HeapInfo::Check<SmallFinalizableWithBarrierHeapBlock>(bool expectFull, bool expectPending, SmallFinalizableWithBarrierHeapBlock * list, SmallFinalizableWithBarrierHeapBlock * tail);
#endif

template size_t HeapInfo::Check<MediumNormalHeapBlock>(bool expectFull, bool expectPending, MediumNormalHeapBlock * list, MediumNormalHeapBlock * tail);
template size_t HeapInfo::Check<MediumLeafHeapBlock>(bool expectFull, bool expectPending, MediumLeafHeapBlock * list, MediumLeafHeapBlock * tail);
template size_t HeapInfo::Check<MediumFinalizableHeapBlock>(bool expectFull, bool expectPending, MediumFinalizableHeapBlock * list, MediumFinalizableHeapBlock * tail);
template size_t HeapInfo::Check<LargeHeapBlock>(bool expectFull, bool expectPending, LargeHeapBlock * list, LargeHeapBlock * tail);
#ifdef RECYCLER_WRITE_BARRIER
template size_t HeapInfo::Check<MediumNormalWithBarrierHeapBlock>(bool expectFull, bool expectPending, MediumNormalWithBarrierHeapBlock * list, MediumNormalWithBarrierHeapBlock * tail);
template size_t HeapInfo::Check<MediumFinalizableWithBarrierHeapBlock>(bool expectFull, bool expectPending, MediumFinalizableWithBarrierHeapBlock * list, MediumFinalizableWithBarrierHeapBlock * tail);
#endif

void
HeapInfo::VerifySmallHeapBlockCount()
{
    GetSmallHeapBlockCount(true);
}
void
HeapInfo::VerifyLargeHeapBlockCount()
{
    GetLargeHeapBlockCount(true);
}
#endif

#ifdef RECYCLER_MEMORY_VERIFY
void
HeapInfo::Verify()
{
    Assert(!this->recycler->CollectionInProgress());
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].Verify();
    }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].Verify();
    }
#endif

    largeObjectBucket.Verify();

#if ENABLE_CONCURRENT_GC
    HeapBlockList::ForEach(newLeafHeapBlockList, [](SmallLeafHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
    HeapBlockList::ForEach(newNormalHeapBlockList, [](SmallNormalHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newNormalWithBarrierHeapBlockList, [](SmallNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
    HeapBlockList::ForEach(newFinalizableWithBarrierHeapBlockList, [](SmallFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#endif
#ifdef RECYCLER_VISITED_HOST
    HeapBlockList::ForEach(newRecyclerVisitedHostHeapBlockList, [](SmallFinalizableHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#endif
    HeapBlockList::ForEach(newFinalizableHeapBlockList, [](SmallFinalizableHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#endif

#if ENABLE_CONCURRENT_GC
    HeapBlockList::ForEach(newMediumLeafHeapBlockList, [](MediumLeafHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
    HeapBlockList::ForEach(newMediumNormalHeapBlockList, [](MediumNormalHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newMediumNormalWithBarrierHeapBlockList, [](MediumNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
    HeapBlockList::ForEach(newMediumFinalizableWithBarrierHeapBlockList, [](MediumFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#endif
#ifdef RECYCLER_VISITED_HOST
    HeapBlockList::ForEach(newMediumRecyclerVisitedHostHeapBlockList, [](MediumFinalizableHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#endif
    HeapBlockList::ForEach(newMediumFinalizableHeapBlockList, [](MediumFinalizableHeapBlock * heapBlock)
    {
        heapBlock->Verify();
    });
#endif
}
#endif

#ifdef RECYCLER_VERIFY_MARK
void
HeapInfo::VerifyMark()
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        heapBuckets[i].VerifyMark();
    }

#ifdef BUCKETIZE_MEDIUM_ALLOCATIONS
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        mediumHeapBuckets[i].VerifyMark();
    }
#endif

    largeObjectBucket.VerifyMark();

#if ENABLE_CONCURRENT_GC
    HeapBlockList::ForEach(newLeafHeapBlockList, [](SmallLeafHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
    HeapBlockList::ForEach(newNormalHeapBlockList, [](SmallNormalHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newNormalWithBarrierHeapBlockList, [](SmallNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
    HeapBlockList::ForEach(newFinalizableWithBarrierHeapBlockList, [](SmallFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#endif
#ifdef RECYCLER_VISITED_HOST
    HeapBlockList::ForEach(newRecyclerVisitedHostHeapBlockList, [](SmallFinalizableHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#endif
    HeapBlockList::ForEach(newFinalizableHeapBlockList, [](SmallFinalizableHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#endif

#if ENABLE_CONCURRENT_GC
    HeapBlockList::ForEach(newMediumLeafHeapBlockList, [](MediumLeafHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
    HeapBlockList::ForEach(newMediumNormalHeapBlockList, [](MediumNormalHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#ifdef RECYCLER_WRITE_BARRIER
    HeapBlockList::ForEach(newMediumNormalWithBarrierHeapBlockList, [](MediumNormalWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
    HeapBlockList::ForEach(newMediumFinalizableWithBarrierHeapBlockList, [](MediumFinalizableWithBarrierHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#endif
#ifdef RECYCLER_VISITED_HOST
    HeapBlockList::ForEach(newMediumRecyclerVisitedHostHeapBlockList, [](MediumFinalizableHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#endif
    HeapBlockList::ForEach(newMediumFinalizableHeapBlockList, [](MediumFinalizableHeapBlock * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#endif
}
#endif

#ifdef RECYCLER_FINALIZE_CHECK
size_t
HeapInfo::GetFinalizeCount()
{
    return this->liveFinalizableObjectCount - this->newFinalizableObjectCount - this->pendingDisposableObjectCount;
}
#endif

#ifdef RECYCLER_MEMORY_VERIFY
void
HeapInfo::EnableVerify()
{
    ForEachPageAllocator([](IdleDecommitPageAllocator* pageAlloc)
    {
        pageAlloc->EnableVerify();
    });
}
#endif

#ifdef RECYCLER_NO_PAGE_REUSE
void
HeapInfo::DisablePageReuse()
{
    ForEachPageAllocator([](IdleDecommitPageAllocator* pageAlloc)
    {
        pageAlloc->DisablePageReuse();
    });
}
#endif
#if DBG
bool
HeapInfo::AllocatorsAreEmpty()
{
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        if (!heapBuckets[i].AllocatorsAreEmpty())
        {
            return false;
        }
    }

#if defined(BUCKETIZE_MEDIUM_ALLOCATIONS) && SMALLBLOCK_MEDIUM_ALLOC
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        if (!mediumHeapBuckets[i].AllocatorsAreEmpty())
        {
            return false;
        }
    }

#endif

    return true;
}
#endif

// ==============================================================
// Page allocator APIs
// ==============================================================
void
HeapInfo::Prime()
{
    ForEachPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->Prime(RecyclerPageAllocator::DefaultPrimePageCount);
    });
}

void
HeapInfo::CloseNonLeaf()
{
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->Close();
    });
}

void
HeapInfo::SuspendIdleDecommitNonLeaf()
{
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->SuspendIdleDecommit();
    });
}

void HeapInfo::ResumeIdleDecommitNonLeaf()
{
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->ResumeIdleDecommit();
    });
}

void
HeapInfo::EnterIdleDecommit()
{
    ForEachPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->EnterIdleDecommit();
    });
}

IdleDecommitSignal
HeapInfo::LeaveIdleDecommit(bool allowTimer)
{
    IdleDecommitSignal idleDecommitSignal = IdleDecommitSignal_None;
    ForEachPageAllocator([&idleDecommitSignal, allowTimer](IdleDecommitPageAllocator * pageAlloc)
    {
        IdleDecommitSignal signal = pageAlloc->LeaveIdleDecommit(allowTimer);
        idleDecommitSignal = max(idleDecommitSignal, signal);
    });
    return idleDecommitSignal;
}

#ifdef IDLE_DECOMMIT_ENABLED
DWORD HeapInfo::IdleDecommit()
{
    DWORD waitTime = INFINITE;
    ForEachPageAllocator([&](IdleDecommitPageAllocator * pageAlloc)
    {
        DWORD pageAllocatorWaitTime = pageAlloc->IdleDecommit();
        waitTime = min(waitTime, pageAllocatorWaitTime);
    });
    return waitTime;
}
#endif

#if DBG
void
HeapInfo::ShutdownIdleDecommit()
{
    ForEachPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->ShutdownIdleDecommit();
    });
}
#endif

void
HeapInfo::DecommitNow(bool all)
{
    ForEachPageAllocator([=](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->DecommitNow(all);
    });
}

size_t
HeapInfo::GetUsedBytes()
{
#if GLOBAL_ENABLE_WRITE_BARRIER
    if (CONFIG_FLAG(ForceSoftwareWriteBarrier))
    {
        Assert(recyclerPageAllocator.usedBytes == 0);
    }
#endif

    size_t usedBytes = 0;
    ForEachPageAllocator([&](IdleDecommitPageAllocator * pageAlloc)
    {
        usedBytes += pageAlloc->usedBytes;
    });
    return usedBytes;
}

size_t
HeapInfo::GetReservedBytes()
{
    size_t reservedBytes = 0;
    ForEachPageAllocator([&](IdleDecommitPageAllocator * pageAlloc)
    {
        reservedBytes += pageAlloc->reservedBytes;
    });
    return reservedBytes;
}

size_t
HeapInfo::GetCommittedBytes()
{
    size_t committedBytes = 0;
    ForEachPageAllocator([&](IdleDecommitPageAllocator * pageAlloc)
    {
        committedBytes += pageAlloc->committedBytes;
    });
    return committedBytes;
}

size_t
HeapInfo::GetNumberOfSegments()
{
    size_t numberOfSegments = 0;
    ForEachPageAllocator([&](IdleDecommitPageAllocator * pageAlloc)
    {
        numberOfSegments += pageAlloc->numberOfSegments;
    });
    return numberOfSegments;
}

IdleDecommitPageAllocator*
HeapInfo::GetRecyclerPageAllocator()
{
    // TODO: SWB this is for Finalizable leaf allocation, which we didn't implement leaf bucket for it
    // remove this after the finalizable leaf bucket is implemented
#if GLOBAL_ENABLE_WRITE_BARRIER
    if (CONFIG_FLAG(ForceSoftwareWriteBarrier))
    {
        return &this->recyclerWithBarrierPageAllocator;
    }
    else
#endif
    {
#if defined(RECYCLER_WRITE_WATCH) || !defined(RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE)
        return &this->recyclerPageAllocator;
#else
        return &this->recyclerWithBarrierPageAllocator;
#endif
    }
}

IdleDecommitPageAllocator*
HeapInfo::GetRecyclerLargeBlockPageAllocator()
{
    return &this->recyclerLargeBlockPageAllocator;
}

IdleDecommitPageAllocator*
HeapInfo::GetRecyclerLeafPageAllocator()
{
    return this->recyclerLeafPageAllocator;
}

#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
IdleDecommitPageAllocator*
HeapInfo::GetRecyclerWithBarrierPageAllocator()
{
    return &this->recyclerWithBarrierPageAllocator;
}
#endif

#if ENABLE_BACKGROUND_PAGE_ZEROING
void
HeapInfo::StartQueueZeroPage()
{
    // Only queue up non-leaf pages- leaf pages don't need to be zeroed out
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->StartQueueZeroPage();
    });
}

void
HeapInfo::StopQueueZeroPage()
{
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->StopQueueZeroPage();
    });
}

void
HeapInfo::BackgroundZeroQueuedPages()
{
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->BackgroundZeroQueuedPages();
    });
}

void
HeapInfo::ZeroQueuedPages()
{
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->ZeroQueuedPages();
    });
}

void
HeapInfo::FlushBackgroundPages()
{
    ForEachPageAllocator([](IdleDecommitPageAllocator * pageAlloc)
    {
        pageAlloc->SuspendIdleDecommit();
        pageAlloc->FlushBackgroundPages();
        pageAlloc->ResumeIdleDecommit();
    });
}

#if DBG
bool HeapInfo::HasZeroQueuedPages()
{
    bool hasZeroQueuedPage = false;
    ForEachNonLeafPageAllocator([&](IdleDecommitPageAllocator * pageAlloc)
    {
        hasZeroQueuedPage = hasZeroQueuedPage || pageAlloc->HasZeroQueuedPages();
    });
    return hasZeroQueuedPage;
}
#endif
#endif

#if DBG
void
HeapInfo::ResetThreadId()
{
    ForEachPageAllocator([](IdleDecommitPageAllocator* pageAlloc)
    {
        pageAlloc->ClearConcurrentThreadId();
    });
}

void
HeapInfo::SetDisableThreadAccessCheck()
{
    ForEachNonLeafPageAllocator([](IdleDecommitPageAllocator* pageAlloc)
    {
        pageAlloc->SetDisableThreadAccessCheck();
    });
}
#endif


// Block attribute functions
/* static */
BOOL SmallAllocationBlockAttributes::IsAlignedObjectSize(size_t sizeCat)
{
    return HeapInfo::IsAlignedSmallObjectSize(sizeCat);
}

/* static */
BOOL MediumAllocationBlockAttributes::IsAlignedObjectSize(size_t sizeCat)
{
    return HeapInfo::IsAlignedMediumObjectSize(sizeCat);
}

namespace Memory
{
template class HeapInfo::ValidPointersMap<SmallAllocationBlockAttributes>;
template class ValidPointers<SmallAllocationBlockAttributes>;

template class HeapInfo::ValidPointersMap<MediumAllocationBlockAttributes>;
template class ValidPointers<MediumAllocationBlockAttributes>;
};
