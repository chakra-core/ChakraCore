//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Memory
{


class RecyclerPageAllocator : public IdleDecommitPageAllocator
{
public:
    RecyclerPageAllocator(Recycler* recycler, AllocationPolicyManager * policyManager,
#ifndef JD_PRIVATE
        Js::ConfigFlagsTable& flagTable,
#endif
        uint maxFreePageCount, uint maxAllocPageCount = PageAllocator::DefaultMaxAllocPageCount);
#if ENABLE_CONCURRENT_GC
    void EnableWriteWatch();
    bool ResetWriteWatch();
#endif

    static uint const DefaultPrimePageCount = 0x1000; // 16MB

#if ENABLE_CONCURRENT_GC
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
#if ENABLE_BACKGROUND_PAGE_ZEROING
    ZeroPageQueue zeroPageQueue;
#endif
    
    Recycler* recycler;

    bool IsMemProtectMode();
};
}
