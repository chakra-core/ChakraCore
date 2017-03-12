//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

template <class T>
struct SignedTypeTraits
{
    typedef void UnsignedType;
};

template <>
struct SignedTypeTraits<int16>
{
    typedef uint16 UnsignedType;
    static const int16 MinValue = INT16_MIN;
};
template <>
struct SignedTypeTraits<int32>
{
    typedef uint32 UnsignedType;
    static const int32 MinValue = INT32_MIN;
};
template <>
struct SignedTypeTraits<int64>
{
    typedef uint64 UnsignedType;
    static const int64 MinValue = INT64_MIN;
};

template <class T>
class IntMathCommon
{
public:
    typedef typename SignedTypeTraits<T>::UnsignedType UnsignedType;
    static const T MinValue = SignedTypeTraits<T>::MinValue;

    static bool Add(T left, T right, T *pResult);
    static bool Sub(T left, T right, T *pResult);
    static bool Mul(T left, T right, T *pResult);
    static bool Div(T left, T right, T *pResult);
    static bool Mod(T left, T right, T *pResult);
    static bool And(T left, T right, T *pResult);
    static bool  Or(T left, T right, T *pResult);
    static bool Xor(T left, T right, T *pResult);

    static bool Neg(T val, T *pResult);
    static bool Not(T val, T *pResult);
    static bool Inc(T val, T *pResult);
    static bool Dec(T val, T *pResult);

    static T NearestInRangeTo(const T value, const T minimum, const T maximum); // inclusive
};


template <class T>
bool IntMathCommon<T>::Add(T left, T right, T *pResult)
{
#if __has_builtin(__builtin_add_overflow)
    return __builtin_add_overflow(left, right, pResult);
#else
    // Overflow occurs when the result has a different sign from both the
    // left and right operands
    *pResult = static_cast<T>(
        static_cast<UnsignedType>(left) + static_cast<UnsignedType>(right));
    return ((left ^ *pResult) & (right ^ *pResult)) < 0;
#endif
}

template <class T>
bool IntMathCommon<T>::Sub(T left, T right, T *pResult)
{
#if __has_builtin(__builtin_sub_overflow)
    return __builtin_sub_overflow(left, right, pResult);
#else
    // Overflow occurs when the result has a different sign from the left
    // operand, and the result has the same sign as the right operand
    *pResult = static_cast<T>(
        static_cast<UnsignedType>(left) - static_cast<UnsignedType>(right));
    return ((left ^ *pResult) & ~(right ^ *pResult)) < 0;
#endif
}

#if __has_builtin(__builtin_mul_overflow) && !(defined(_ARM_) && defined(__clang__))
template <class T>
bool IntMathCommon<T>::Mul(T left, T right, T *pResult)
{
    return __builtin_mul_overflow(left, right, pResult);
}
#endif

template <class T>
bool IntMathCommon<T>::Div(T left, T right, T *pResult)
{
    AssertMsg(right != 0, "Divide by zero...");

    if (right == -1 && left == MinValue)
    {
        // Special check for MinValue/-1
        return true;
    }

    *pResult = left / right;
    return false;
}

template <class T>
bool IntMathCommon<T>::Mod(T left, T right, T *pResult)
{
    AssertMsg(right != 0, "Mod by zero...");
    if (right == -1 && left == MinValue)
    {
        //Special check for MinValue/-1
        return true;
    }
    *pResult = left % right;
    return false;
}

template <class T>
bool IntMathCommon<T>::And(T left, T right, T *pResult)
{
    *pResult = left & right;
    return false;
}

template <class T>
bool IntMathCommon<T>::Or(T left, T right, T *pResult)
{
    *pResult = left | right;
    return false;
}

template <class T>
bool IntMathCommon<T>::Xor(T left, T right, T *pResult)
{
    *pResult = left ^ right;
    return false;
}

template <class T>
bool IntMathCommon<T>::Neg(T val, T *pResult)
{
    if (val == MinValue)
    {
        *pResult = val;
        return true;
    }
    *pResult = - val;
    return false;
}

template <class T>
bool IntMathCommon<T>::Not(T val, T *pResult)
{
    *pResult = ~val;
    return false;
}

template <class T>
bool IntMathCommon<T>::Inc(T val, T *pResult)
{
#if __has_builtin(__builtin_add_overflow)
    return __builtin_add_overflow(val, 1, pResult);
#else
    *pResult = static_cast<T>(
        static_cast<UnsignedType>(val) + static_cast<UnsignedType>(1));
    // Overflow if result ends up less than input
    return *pResult <= val;
#endif
}

template <class T>
bool IntMathCommon<T>::Dec(T val, T *pResult)
{
#if __has_builtin(__builtin_sub_overflow)
    return __builtin_sub_overflow(val, 1, pResult);
#else
    *pResult = static_cast<T>(
        static_cast<UnsignedType>(val) - static_cast<UnsignedType>(1));
    // Overflow if result ends up greater than input
    return *pResult >= val;
#endif
}

template <class T>
T IntMathCommon<T>::NearestInRangeTo(const T value, const T minimum, const T maximum) // inclusive
{
    Assert(minimum <= maximum);
    return minimum >= value ? minimum : maximum <= value ? maximum : value;
}
