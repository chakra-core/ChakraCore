//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if ENABLE_NATIVE_CODEGEN
namespace Js
{

EquivalentTypeSet::EquivalentTypeSet(RecyclerJITTypeHolder * types, uint16 count)
    : types(types), count(count), sortedAndDuplicatesRemoved(false)
{
}

JITTypeHolder EquivalentTypeSet::GetType(uint16 index) const
{
    Assert(this->types != nullptr && this->count > 0 && index < this->count);
    return this->types[index];
}

JITTypeHolder EquivalentTypeSet::GetFirstType() const
{
    return GetType(0);
}

bool EquivalentTypeSet::Contains(const JITTypeHolder type, uint16* pIndex)
{
    if (!this->GetSortedAndDuplicatesRemoved())
    {
        this->SortAndRemoveDuplicates();
    }
    for (uint16 ti = 0; ti < this->count; ti++)
    {
        if (this->GetType(ti) == type)
        {
            if (pIndex)
            {
                *pIndex = ti;
            }
            return true;
        }
    }
    return false;
}

bool EquivalentTypeSet::AreIdentical(EquivalentTypeSet * left, EquivalentTypeSet * right)
{
    if (!left->GetSortedAndDuplicatesRemoved())
    {
        left->SortAndRemoveDuplicates();
    }
    if (!right->GetSortedAndDuplicatesRemoved())
    {
        right->SortAndRemoveDuplicates();
    }

    Assert(left->GetSortedAndDuplicatesRemoved() && right->GetSortedAndDuplicatesRemoved());

    if (left->count != right->count)
    {
        return false;
    }

    // TODO: OOP JIT, optimize this (previously we had memcmp)
    for (uint i = 0; i < left->count; ++i)
    {
        if (left->types[i] != right->types[i])
        {
            return false;
        }
    }
    return true;
}

bool EquivalentTypeSet::IsSubsetOf(EquivalentTypeSet * left, EquivalentTypeSet * right)
{
    if (!left->GetSortedAndDuplicatesRemoved())
    {
        left->SortAndRemoveDuplicates();
    }
    if (!right->GetSortedAndDuplicatesRemoved())
    {
        right->SortAndRemoveDuplicates();
    }

    if (left->count > right->count)
    {
        return false;
    }

    // Try to find each left type in the right set.
    int j = 0;
    for (int i = 0; i < left->count; i++)
    {
        bool found = false;
        for (; j < right->count; j++)
        {
            if (left->types[i] < right->types[j])
            {
                // Didn't find the left type. Fail.
                return false;
            }
            if (left->types[i] == right->types[j])
            {
                // Found the left type. Continue to the next left/right pair.
                found = true;
                j++;
                break;
            }
        }
        Assert(j <= right->count);
        if (j == right->count && !found)
        {
            // Exhausted the right set without finding the current left type.
            return false;
        }
    }
    return true;
}

void EquivalentTypeSet::SortAndRemoveDuplicates()
{
    uint16 oldCount = this->count;
    uint16 i;

    // sorting
    for (i = 1; i < oldCount; i++)
    {
        uint16 j = i;
        while (j > 0 && (this->types[j - 1] > this->types[j]))
        {
            JITTypeHolder tmp = this->types[j];
            this->types[j] = this->types[j - 1];
            this->types[j - 1] = tmp;
            j--;
        }
    }

    // removing duplicate types from the sorted set
    i = 0;
    for (uint16 j = 1; j < oldCount; j++)
    {
        if (this->types[i] != this->types[j])
        {
            this->types[++i] = this->types[j];
        }
    }
    this->count = ++i;
    for (i; i < oldCount; i++)
    {
        this->types[i] = JITTypeHolder(nullptr);
    }

    this->sortedAndDuplicatesRemoved = true;
}
}
#endif