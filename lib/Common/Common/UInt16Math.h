//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class UInt16Math
{
public:
    template< class Func >
    static uint16 Add(uint16 lhs, uint16 rhs, __inout Func& overflowFn)
    {
        uint16 result = lhs + rhs;

        // If the result is smaller than the LHS, then we overflowed
        if( result < lhs )
        {
            overflowFn();
        }

        return result;
    }

    template< class Func >
    static void Inc(uint16& lhs, __inout Func& overflowFn)
    {
        ++lhs;

        // If lhs becomes 0, then we overflowed
        if(!lhs)
        {
            overflowFn();
        }
    }

    // Convenience function which uses DefaultOverflowPolicy (throws OOM upon overflow)
    static uint16 Add(uint16 lhs, uint16 rhs)
    {
        return Add(lhs, rhs, ::Math::DefaultOverflowPolicy);
    }

    // Convenience function which returns a bool indicating overflow
    static bool Add(uint16 lhs, uint16 rhs, __out uint16* result)
    {
        ::Math::RecordOverflowPolicy overflowGuard;
        *result = Add(lhs, rhs, overflowGuard);
        return overflowGuard.HasOverflowed();
    }

    // Convenience function which uses DefaultOverflowPolicy (throws OOM upon overflow)
    static void Inc(uint16& lhs)
    {
        Inc(lhs, ::Math::DefaultOverflowPolicy);
    }

    template<typename Func>
    static uint16 Mul(uint16 lhs, uint16 rhs, __in Func& overflowFn)
    {
        // Do the multiplication using 32-bit unsigned math.
        uint32 result = static_cast<uint32>(lhs) * static_cast<uint32>(rhs);

        // Does the result fit in 16-bits?
        if(result > UINT16_MAX)
        {
            overflowFn();
        }

        return static_cast<uint16>(result);
    }

    static uint16 Mul(uint16 lhs, uint16 rhs)
    {
        return Mul(lhs, rhs, ::Math::DefaultOverflowPolicy);
    }

    static bool Mul(uint16 lhs, uint16 rhs, __out uint16* result)
    {
        ::Math::RecordOverflowPolicy overflowGuard;
        *result = Mul(lhs, rhs, overflowGuard);
        return overflowGuard.HasOverflowed();
    }

};
using ArgSlotMath = UInt16Math;

template <>
inline bool Math::IncImpl<uint16>(uint16 val, uint16 *pResult)
{
    return UInt16Math::Add(val, 1, pResult);
}

template <>
inline bool Math::AddImpl<uint16>(uint16 left, uint16 right, uint16 *pResult)
{
    return UInt16Math::Add(left, right, pResult);
}
