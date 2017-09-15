//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
namespace Js
{
    class EquivalentTypeSet
    {
    private:
        Field(bool) sortedAndDuplicatesRemoved;
        Field(uint16) count;
        Field(RecyclerJITTypeHolder *) types;

    public:
        EquivalentTypeSet(RecyclerJITTypeHolder * types, uint16 count);

        uint16 GetCount() const
        {
            return this->count;
        }

        JITTypeHolder GetFirstType() const;

        JITTypeHolder GetType(uint16 index) const;

        bool GetSortedAndDuplicatesRemoved() const
        {
            return this->sortedAndDuplicatesRemoved;
        }
        bool Contains(const JITTypeHolder type, uint16 * pIndex = nullptr);

        static bool AreIdentical(EquivalentTypeSet * left, EquivalentTypeSet * right);
        static bool IsSubsetOf(EquivalentTypeSet * left, EquivalentTypeSet * right);
        void SortAndRemoveDuplicates();
    };
};
