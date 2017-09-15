//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once


#define UNKNOWN_MOD_INDEX 75 // count of the known mod array (under SizePolicy.cpp)


struct PrimePolicy
{
    inline static hash_t GetBucket(hash_t hashCode, uint size, int modFunctionIndex)
    {
        hash_t targetBucket = ModPrime(hashCode, size, modFunctionIndex);
        return targetBucket;
    }

    inline static uint GetSize(uint capacity, int *modFunctionIndex)
    {
        return GetPrime(capacity, modFunctionIndex);
    }

private:
    static bool IsPrime(uint candidate);
    static uint GetPrime(uint min, int *modFunctionIndex);
    static hash_t ModPrime(hash_t key, uint prime, int modFunctionIndex);
};

struct PowerOf2Policy
{
    inline static hash_t GetBucket(hash_t hashCode, uint size, int modFunctionIndex)
    {
        AssertMsg(Math::IsPow2(size) && size > 1, "Size is not a power of 2.");
        hash_t targetBucket = hashCode & (size-1);
        return targetBucket;
    }

    /// Returns a size that is power of 2 and
    /// greater than specified capacity.
    inline static uint GetSize(size_t minCapacity_t, int *modFunctionIndex)
    {
        AssertMsg(minCapacity_t <= MAXINT32, "the next higher power of 2  must fit in uint32");
        uint minCapacity = static_cast<uint>(minCapacity_t);
        *modFunctionIndex = UNKNOWN_MOD_INDEX;

        if(minCapacity <= 0)
        {
            return 4;
        }

        if (Math::IsPow2(minCapacity))
        {
            return minCapacity;
        }
        else
        {
            return 1 << (Math::Log2(minCapacity) + 1);
        }
    }
};

template <class SizePolicy, uint averageChainLength = 2, uint growthRateNumerator = 2, uint growthRateDenominator = 1, uint minBucket = 4>
struct DictionarySizePolicy
{
    CompileAssert(growthRateNumerator > growthRateDenominator);
    CompileAssert(growthRateDenominator != 0);
    inline static hash_t GetBucket(hash_t hashCode, uint bucketCount, int modFunctionIndex)
    {
        return SizePolicy::GetBucket(hashCode, bucketCount, modFunctionIndex);
    }
    inline static uint GetNextSize(uint minCapacity)
    {
        uint nextSize = minCapacity * growthRateNumerator / growthRateDenominator;
        return (growthRateDenominator != 1 && nextSize <= minCapacity)? minCapacity + 1 : nextSize;
    }
    inline static uint GetBucketSize(uint size, int *modFunctionIndex)
    {
        if (minBucket * averageChainLength >= size)
        {
            return SizePolicy::GetSize(minBucket, modFunctionIndex);
        }
        return SizePolicy::GetSize((size + (averageChainLength - 1)) / averageChainLength, modFunctionIndex);
    }
};

typedef DictionarySizePolicy<PrimePolicy> PrimeSizePolicy;
typedef DictionarySizePolicy<PowerOf2Policy> PowerOf2SizePolicy;
