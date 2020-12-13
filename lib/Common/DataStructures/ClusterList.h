//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "../Core/Assertions.h"
#include "../Memory/Allocator.h"

/*
 * ClusterLists are intended to solve the problem of determining what connected set
 * groups are present in a graph while iterating over a list of connections.
 */

template<typename indexType, class TAllocator>
class ClusterList
{
private:
    // The maximum index enabled; basically an array bounds for the set.
    indexType maxIndex;
    // The low-level data store backing the structure. This is a list initialized as the
    // identity mapping (list[i] = i), and is then updated as links are added so that an
    // entry in the list is the index of an entry of equal or lower number which is part
    // of the same set. This is later updated to be the index of the element with lowest
    // index in the set for all entries, avoiding situations where lookups would need to
    // follow a chain.
    indexType* list;
#if DBG
    // During the merge process, we may end up with some situations where set membership
    // is chained; that is, that node 7 is in set 5, but 5 is in set 4. Following all of
    // the merge operations we have a step to reconcile all of this information and then
    // update each element of the list to point to the least member of its set. This can
    // be checked in debug builds to see if we've done this step or not.
    bool consolidated = false;
#endif
    TAllocator* alloc;
public:
    // Create a new ClusterList with a specified maximum node number
    ClusterList(TAllocator* alloc, indexType maxCount)
        : maxIndex(maxCount)
        , list(nullptr)
        , alloc(alloc)
#if DBG
        , consolidated(true)
#endif
    {
        list = AllocatorNewArrayLeaf(TAllocator, this->alloc, indexType, maxIndex);
        for (indexType i = 0; i < maxIndex; i++)
        {
            list[i] = i;
        }
    }
    // Merge the set containing node a and the set containing node b
    void Merge(indexType a, indexType b)
    {
        indexType aVal = GetSet<false>(a);
        indexType bVal = GetSet<false>(b);
        if (aVal == bVal)
            return;
        indexType min = (aVal < bVal ? aVal : bVal);
        list[aVal] = min;
        list[bVal] = min;
#if DBG
        consolidated = false;
#endif
    }
    ~ClusterList()
    {
        if (this->list != nullptr)
        {
            AllocatorDeleteArrayLeaf(TAllocator, this->alloc, maxIndex, this->list);
            this->list = nullptr;
        }
    }
    // Do a pass to update each set membership reference to the minimum for the set
    void Consolidate()
    {
        for (indexType i = 0; i < maxIndex; i++)
        {
            list[i] = list[list[i]];
        }
#if DBG
        consolidated = true;
#endif
    }
    // Reset the list; useful if we're re-using the data structure
    void Reset()
    {
        for (indexType i = 0; i < maxIndex; i++)
        {
            list[i] = i;
        }
#if DBG
        consolidated = true;
#endif
    }
    // Get the index of the least element in the set, which serves as a unique set identifier
    // (note that if further merges happen, you may end up in a set with a different number)
    template<bool assumeConsolidated>
    indexType GetSet(indexType in)
    {
        if (assumeConsolidated)
        {
            Assert(consolidated);
            return list[in];
        }
        else
        {
            if (list[in] == in)
            {
                return in;
            }
            else
            {
                indexType actualSet = GetSet<false>(list[in]);
                list[in] = actualSet;
                return actualSet;
            }
        }
    }
    // Map a function on index, setnumber for all nodes in the set
    // note that it'll run on nodes that were never involved in any merges and are thus in solo sets
    template<typename MapAccessoryType>
    inline void Map(void(*callBack)(indexType, indexType, MapAccessoryType), MapAccessoryType accessory)
    {
        Assert(consolidated);
        for (indexType i = 0; i < maxIndex; i++)
        {
            callBack(i, list[i], accessory);
        }
    }
};

/*
 * SegmentClusterList differs from a normal ClusterList in that we allocate regions
 * of the list instead of the entire list. This should provide good performance due
 * to the multi-pass nature of the globopt; regions of code will generally generate
 * stacksyms in one region per pass (since they're allocated incrementally), so the
 * numbers used will normally be somewhat grouped. This saves us allocating most of
 * the array in cases where we have large functions (which is more common than code
 * input may indicate due to inlining), assuming that the data structure would need
 * to be allocated on a per-block basis otherwise.
 */

template<typename indexType, class TAllocator, unsigned int numPerSegment = 16>
class SegmentClusterList
{
private:
    // The maximum index enabled; basically an array bounds for the set.
    indexType maxIndex;
    // The backing store for this ClusterList type is an array of pointers to subsegment
    // structures, each of which stores a fixed region. Elements outside this region are
    // assumed to have an identity mapping before first use.
    indexType **backingStore;
#if DBG
    // During the merge process, we may end up with some situations where set membership
    // is chained; that is, that node 7 is in set 5, but 5 is in set 4. Following all of
    // the merge operations we have a step to reconcile all of this information and then
    // update each element of the list to point to the least member of its set. This can
    // be checked in debug builds to see if we've done this step or not.
    bool consolidated = false;
#endif
    TAllocator* alloc;
    inline void EnsureBaseSize(indexType inputMax)
    {
        // We want to ensure that we can store to inputMax, which means we need to grow to inputMax+1
        indexType targetSize = inputMax + 1;
        // round up to the next numPerSegment multiple
        indexType max = (targetSize % numPerSegment == 0) ? targetSize : (targetSize + numPerSegment - (targetSize % numPerSegment));
        // get segment counts for allocation
        indexType newNumSegments = max / numPerSegment;
        indexType curNumSegments = maxIndex / numPerSegment;
        if (newNumSegments > curNumSegments)
        {
            // Note that if you want to use this with something other than an ArenaAllocator, then
            // you'll have to add clean-up mechanics
            indexType **tempList = AllocatorNewArray(TAllocator, alloc, indexType*, newNumSegments);
            for (indexType i = 0; i < curNumSegments; i++)
            {
                tempList[i] = backingStore[i];
            }
            for (indexType i = curNumSegments; i < newNumSegments; i++)
            {
                tempList[i] = nullptr;
            }
            // Set the max index to the number of supported indicies
            maxIndex = max;
            // replace the backingStore with the new backing Store
            indexType **oldBackingStore = backingStore;
            backingStore = tempList;
            AllocatorDeleteArray(TAllocator, alloc, curNumSegments, oldBackingStore);
        }
    }
    inline void CreateBacking(indexType index)
    {
        Assert(index >= maxIndex || backingStore[index / numPerSegment] == nullptr);
        // grow the size of the pointer array if needed
        EnsureBaseSize(index);
        if (backingStore[index / numPerSegment] == nullptr)
        {
            // allocate a new segment
            backingStore[index / numPerSegment] = AllocatorNewArrayLeaf(TAllocator, alloc, indexType, numPerSegment);
            indexType baseForSegment = (index / numPerSegment) * numPerSegment;
            for (indexType i = 0; i < numPerSegment; i++)
            {
                backingStore[index / numPerSegment][i] = i + baseForSegment;
            }
        }
    }
    template<bool createBacking = false, bool assumeConsolidated = false>
    inline indexType Lookup(indexType index)
    {
        if (index >= maxIndex || backingStore[index / numPerSegment] == nullptr)
        {
            // Not being there simply means that it's still an identity mapping
            if(createBacking)
            {
                CreateBacking(index);
            }
            return index;
        }
        if (assumeConsolidated)
        {
            // If the list is consolidated, then we can return whatever's there
            return backingStore[index / numPerSegment][index % numPerSegment];
        }
        else
        {
            // If it's not consolidated, we need to follow the chain down, and update each entry to the root
            indexType currentValue = backingStore[index / numPerSegment][index % numPerSegment];
            if (currentValue == index)
            {
                return index;
            }
            indexType trueRoot = Lookup<false, assumeConsolidated>(currentValue);
            backingStore[index / numPerSegment][index % numPerSegment] = trueRoot;
            return trueRoot;
        }
    }

    template<bool assumeExists = false>
    inline void Assign(indexType index, indexType value)
    {
        if (!assumeExists)
        {
            if (index >= maxIndex || backingStore[index / numPerSegment] == nullptr)
            {
                CreateBacking(index);
            }
        }
        backingStore[index / numPerSegment][index % numPerSegment] = value;
    }
public:
    // Create a new ClusterList with a specified maximum node number
    SegmentClusterList(TAllocator* allocator, indexType maxCount)
        : maxIndex(maxCount % numPerSegment == 0 ? maxCount : maxCount + (numPerSegment - (maxCount % numPerSegment)))
        , backingStore(nullptr)
        , alloc(allocator)
#if DBG
        , consolidated(true)
#endif
    {
        backingStore = AllocatorNewArrayZ(TAllocator, alloc, indexType*, maxIndex / numPerSegment);
    }
    ~SegmentClusterList()
    {
        if (this->backingStore != nullptr)
        {
            // Reset is just a delete + nullptr on all segments
            this->Reset();
            AllocatorDeleteArray(TAllocator, alloc, maxIndex / numPerSegment, this->backingStore);
            this->backingStore = nullptr;
        }
    }
    // Merge the set containing node a and the set containing node b
    void Merge(indexType a, indexType b)
    {
        if (a == b)
            return;
        indexType aVal = Lookup<true>(a);
        indexType bVal = Lookup<true>(b);
        indexType min = (aVal < bVal ? aVal : bVal);
        // We need to update the roots of both branches to point to the min
        Assign<false>(aVal, min);
        Assign<false>(bVal, min);
#if DBG
        if(aVal != min || bVal != min)
            consolidated = false;
#endif
    }
    // Do a pass to update each set membership reference to the minimum for the set
    void Consolidate()
    {
        for (indexType i = 0; i < maxIndex / numPerSegment; i++)
        {
            if (backingStore[i] != nullptr)
            {
                for (indexType j = 0; j < numPerSegment; j++)
                {
                    // We can assumeConsolidated here because everything less than the current index is consolidated
                    backingStore[i][j] = Lookup<false, true>(backingStore[i][j]);
                }
            }
        }
#if DBG
        consolidated = true;
#endif
    }
    // Reset the list; useful if we're re-using the data structure
    void Reset()
    {
        for (indexType i = 0; i < maxIndex / numPerSegment; i++)
        {
            if (backingStore[i] != nullptr)
            {
                AllocatorDeleteArrayLeaf(TAllocator, alloc, numPerSegment, backingStore[i]);
                backingStore[i] = nullptr;
            }
        }
#if DBG
        consolidated = true;
#endif
    }
    // Get the index of the least element in the set, which serves as a unique set identifier
    // (note that if further merges happen, you may end up in a set with a different number)
    indexType GetSet(indexType in)
    {
        Assert(consolidated);
        return Lookup<false>(in);
    }
    // Map a function on index, setnumber, accessory for all nodes in the set (or just non-identities)
    // note that it'll run on nodes that were never involved in any merges and are thus in solo sets
    template<typename MapAccessoryType, bool onlyNonIdentity = false>
    inline void Map(void(*callBack)(indexType, indexType, MapAccessoryType), MapAccessoryType accessory)
    {
        Assert(consolidated);
        if (onlyNonIdentity)
        {
            for (indexType i = 0; i < maxIndex / numPerSegment; i++)
            {
                if (backingStore[i] == nullptr)
                {
                    continue;
                }
                for (indexType j = 0; j < numPerSegment; j++)
                {
                    indexType index = i * numPerSegment + j;
                    indexType local = Lookup<false, true>(index);
                    if (index != local)
                    {
                        callBack(index, local, accessory);
                    }
                }
            }
        }
        else
        {
            for (indexType i = 0; i < maxIndex; i++)
            {
                callBack(i, Lookup<false, true>(i), accessory);
            }
        }
    }

    // Map a function across the set for a particular index
    template<typename MapAccessoryType, bool onlyNonIdentity = false>
    inline void MapSet(indexType baseSetMember, void(*callBack)(indexType, MapAccessoryType), MapAccessoryType accessory)
    {
        Assert(consolidated);
        indexType baseSet = Lookup<false, true>(baseSetMember);
        if (!onlyNonIdentity)
        {
            callBack(baseSet, accessory);
        }
        // We now only need to look at the stuff greater than baseSet
        for (indexType i = baseSet + 1; i < maxIndex; i++)
        {
            if (backingStore[i / numPerSegment] == nullptr)
            {
                // advance to the next block if this one is an identity block
                i += (numPerSegment - (i % numPerSegment)) - 1;
                continue;
            }
            if (backingStore[i / numPerSegment][i % numPerSegment] == baseSet)
            {
                callBack(i, accessory);
            }
        }
    }

#if DBG_DUMP
    void Dump()
    {
        bool printed = false;
        Output::Print(_u("["));
        for (indexType i = 0; i < maxIndex / numPerSegment; i++)
        {
            if (backingStore[i] == nullptr)
            {
                continue;
            }
            for (indexType j = 0; j < numPerSegment; j++)
            {
                indexType index = i * numPerSegment + j;
                indexType local = Lookup<false, true>(index);
                if (index != local)
                {
                    if (printed)
                    {
                        Output::Print(_u(", "));
                    }
                    Output::Print(_u("%u <= %u"), local, index);
                    printed = true;
                }
            }
        }
        Output::Print(_u("]\n"));
    }
#endif
};
