//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

template <typename TBlockAttributes>
void
SmallLeafHeapBucketT<TBlockAttributes>::Sweep(RecyclerSweep& recyclerSweep)
{
    BaseT::SweepBucket(recyclerSweep, [](RecyclerSweep& recyclerSweep){});
}

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)

template <typename TBlockAttributes>
size_t
SmallLeafHeapBucketT<TBlockAttributes>::GetNonEmptyHeapBlockCount(bool checkCount) const
{
    return BaseT::GetNonEmptyHeapBlockCount(checkCount);
}
#endif

#ifdef RECYCLER_SLOW_CHECK_ENABLED
template <typename TBlockAttributes>
size_t
SmallLeafHeapBucketT<TBlockAttributes>::Check()
{
    return BaseT::Check(true);
}
#endif

#ifdef RECYCLER_MEMORY_VERIFY
template <typename TBlockAttributes>
void
SmallLeafHeapBucketT<TBlockAttributes>::Verify()
{
    BaseT::Verify();
}
#endif

#ifdef RECYCLER_VERIFY_MARK
template <typename TBlockAttributes>
void
SmallLeafHeapBucketT<TBlockAttributes>::VerifyMark()
{
    __super::VerifyMark();
}
#endif

namespace Memory
{
    template class SmallLeafHeapBucketT<SmallAllocationBlockAttributes>;
    template class SmallLeafHeapBucketT<MediumAllocationBlockAttributes>;
}
