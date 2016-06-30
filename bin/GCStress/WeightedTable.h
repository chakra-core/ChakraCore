//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

// This is a rather simplistic implementation of a weighted probability table.
// If the weights are large, it will use a lot of memory.

// It would be nice if we could use std::vector here, but leveraging STL with Chakra seems to cause problems.

template <class T>
class WeightedTable
{
public:
    WeightedTable() :
        entries(nullptr), size(0)
    {
    }

    void AddWeightedEntry(T value, unsigned int weight)
    {
        T * newEntries = static_cast<T *>(realloc(entries, (size + weight) * sizeof(T)));
        if (newEntries == nullptr)
        {
            // Should throw something better
            throw "OOM in AddWeightedEntry realloc";
        }

        entries = newEntries;

        for (unsigned int i = 0; i < weight; i++)
        {
            entries[size + i] = value;
        }

        size += weight;
    }

    T GetRandomEntry() 
    {
        if (size == 0)
        {
            // Should throw something better
            throw "Illegal GetRandomEntry on empty WeightedTable";
        }
        
        return entries[GetRandomInteger(size)];
    }

    unsigned int GetSize()
    {
        return size;
    }

    T GetEntry(unsigned int index)
    {
        if (index >= size)
        {
            // Should throw something better
            throw "Illegal GetEntry beyond table end";
        }
        
        return entries[index];
    }
    
private:
    T * entries;
    unsigned int size;
};

