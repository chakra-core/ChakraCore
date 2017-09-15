//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

///---------------------------------------------------------------------------
///
/// class Math
///
///---------------------------------------------------------------------------

class Math
{
public:

    static uint32 PopCnt32(uint32 x)
    {
        // sum set bits in every bit pair
        x -= (x >> 1) & 0x55555555u;
        // sum pairs into quads
        x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
        // sum quads into octets
        x = (x + (x >> 4)) & 0x0f0f0f0fu;
        // sum octets into topmost octet
        x *= 0x01010101u;
        return x >> 24;
    }

    // Explicit cast to integral (may truncate).  Avoids warning C4302 'type cast': truncation
    template <typename T>
    static T PointerCastToIntegralTruncate(void * pointer)
    {
        return (T)(uintptr_t)pointer;
    }

    // Explicit cast to integral. Assert that it doesn't truncate.  Avoids warning C4302 'type cast': truncation
    template <typename T>
    static T PointerCastToIntegral(void * pointer)
    {
        T value = PointerCastToIntegralTruncate<T>(pointer);
        Assert((uintptr_t)value == (uintptr_t)pointer);
        return value;
    }

    static bool     FitsInDWord(int32 value) { return true; }
    static bool     FitsInDWord(size_t value) { return ((size_t)(signed int)(value & 0xFFFFFFFF) == value); }
    static bool     FitsInDWord(int64 value) { return ((int64)(signed int)(value & 0xFFFFFFFF) == value); }

    static bool     FitsInWord(int32 value) { return ((int32)(int16)(value & 0xFFFF) == value); }

    static UINT_PTR Rand();
    static bool     IsPow2(int32 val) { return (val > 0 && ((val-1) & val) == 0); }
    static uint32   NextPowerOf2(uint32 n);

    // Use for compile-time evaluation of powers of 2
    template<uint32 val> struct Is
    {
        static const bool Pow2 = ((val-1) & val) == 0;
    };

    // Defined in the header so that the Recycler static lib doesn't
    // need to pull in jscript.common.common.lib
    static uint32 Log2(uint32 value)
    {
        int i;

        for (i = 0; value >>= 1; i++);
        return i;
    }

    // Define a couple of overflow policies for the UInt32Math routines.

    // The default policy for overflow is to throw an OutOfMemory exception
    __declspec(noreturn) static void DefaultOverflowPolicy();

    // A functor (class with operator()) which records whether a the calculation
    // encountered an overflow condition.
    class RecordOverflowPolicy
    {
        bool fOverflow;
    public:
        RecordOverflowPolicy() : fOverflow(false)
        {
        }

        // Called when an overflow is detected
        void operator()()
        {
            fOverflow = true;
        }

        bool HasOverflowed() const
        {
            return fOverflow;
        }
    };

    template <typename T>
    static T Align(T size, T alignment)
    {
        return ((size + (alignment-1)) & ~(alignment-1));
    }

    template <typename T>
    static bool IsAligned(T size, T alignment)
    {
        return (size & (alignment - 1)) == 0;
    }

    template <typename T, class Func>
    static T AlignOverflowCheck(T size, T alignment, __inout Func& overflowFn)
    {
        Assert(size >= 0);
        T alignSize = Align(size, alignment);
        if (alignSize < size)
        {
            overflowFn();
        }
        return alignSize;
    }

    template <typename T>
    static T AlignOverflowCheck(T size, T alignment)
    {
        return AlignOverflowCheck(size, alignment, DefaultOverflowPolicy);
    }

    // Postfix increment "val++", call overflowFn() first if overflow
    template <typename T, class Func>
    static T PostInc(T& val, const Func& overflowFn)
    {
        T tmp = val;
        if (IncImpl(val, &tmp))
        {
            overflowFn();  // call before changing val
        }

        T old = val;
        val = tmp;
        return old;
    }

    // Postfix increment "val++", call DefaultOverflowPolicy() first if overflow
    template <typename T>
    static T PostInc(T& val)
    {
        return PostInc(val, DefaultOverflowPolicy);
    }

    template <typename T>
    static bool IncImpl(T val, T *pResult)
    {
        CompileAssert(false);  // must implement template specialization on type T
    }

    template <typename T, class Func>
    static T Add(T left, T right, const Func& overflowFn)
    {
        T result;
        if (AddImpl(left, right, &result))
        {
            overflowFn();
        }
        return result;
    }

    template <typename T>
    static T Add(T left, T right)
    {
        return Add(left, right, DefaultOverflowPolicy);
    }

    template <typename T>
    static bool AddImpl(T left, T right, T *pResult)
    {
        CompileAssert(false);  // must implement template specialization on type T
    }
};
