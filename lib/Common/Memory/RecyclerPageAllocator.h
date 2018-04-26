//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Memory
{

class HeapInfo;
class RecyclerPageAllocator : public IdleDecommitPageAllocator
{
public:
    RecyclerPageAllocator(HeapInfo * heapInfo, AllocationPolicyManager * policyManager,
        Js::ConfigFlagsTable& flagTable, uint maxFreePageCount, uint maxAllocPageCount = PageAllocator::DefaultMaxAllocPageCount, bool enableWriteBarrier = false);
#if ENABLE_CONCURRENT_GC
#ifdef RECYCLER_WRITE_WATCH
    void EnableWriteWatch();
    bool ResetWriteWatch();
#endif
#endif

    static uint const DefaultPrimePageCount = 0x1000; // 16MB

#if ENABLE_CONCURRENT_GC
#ifdef RECYCLER_WRITE_WATCH
#if DBG
    size_t GetWriteWatchPageCount();
#endif
private:
    static bool ResetWriteWatch(DListBase<PageSegment> * segmentList);
    template <typename T>
    static bool ResetAllWriteWatch(DListBase<T> * segmentList);
#if DBG
    static size_t GetWriteWatchPageCount(DListBase<PageSegment> * segmentList);
    template <typename T>
    static size_t GetAllWriteWatchPageCount(DListBase<T> * segmentList);
#endif
#endif
#endif

private:
#if ENABLE_BACKGROUND_PAGE_ZEROING
    ZeroPageQueue zeroPageQueue;
#endif

    HeapInfo * heapInfo;

    bool IsMemProtectMode();
};
}
