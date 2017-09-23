//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeMathPch.h"

namespace Js
{
    // These implementations need to be compiled with /arch:sse2
#if _M_IX86
#define DB_NOINLINE _NOINLINE
#else
#define DB_NOINLINE
#endif

    // Double
    template<> DB_NOINLINE double AsmJsMath::DivChecked<double>(double aLeft, double aRight) { return aLeft / aRight; }
    template<> DB_NOINLINE double AsmJsMath::DivUnsafe<double>(double aLeft, double aRight) { return aLeft / aRight; }
    template<> DB_NOINLINE double AsmJsMath::Mul<double>(double aLeft, double aRight) { return aLeft * aRight; }
    template<> double AsmJsMath::RemChecked<double>(double aLeft, double aRight) { return NumberUtilities::Modulus(aLeft, aRight); }
    template<> double AsmJsMath::RemUnsafe<double>(double aLeft, double aRight) { return NumberUtilities::Modulus(aLeft, aRight); }

    // Float
    template<> float AsmJsMath::DivChecked<float>(float aLeft, float aRight) { return aLeft / aRight; }
    template<> float AsmJsMath::DivUnsafe<float>(float aLeft, float aRight) { return aLeft / aRight; }
    template<> float AsmJsMath::Mul<float>(float aLeft, float aRight) { return aLeft * aRight; }

    // Int32
    template<> int32 AsmJsMath::Mul<int32>(int32 aLeft, int32 aRight) { return aLeft * aRight; }
    template<> int32 AsmJsMath::DivUnsafe<int32>(int32 aLeft, int32 aRight) { return aLeft / aRight; }
    template<>
    int32 AsmJsMath::DivChecked<int32>(int32 aLeft, int32 aRight)
    {
        return aRight == 0 ? 0 : (aLeft == INT_MIN && aRight == -1) ? INT_MIN : aLeft / aRight;
    }
    template<> bool AsmJsMath::DivWouldTrap(int32 aLeft, int32 aRight) { return aRight == 0 || (aLeft == INT_MIN && aRight == -1); }
    template<> int32 AsmJsMath::RemUnsafe<int32>(int32 aLeft, int32 aRight) { return aLeft % aRight; }
    template<>
    int32 AsmJsMath::RemChecked<int32>(int32 aLeft, int32 aRight)
    {
        return ((aRight == 0) || (aLeft == INT_MIN && aRight == -1)) ? 0 : aLeft % aRight;
    }
    template<> bool AsmJsMath::RemWouldTrap(int32 aLeft, int32 aRight) { return aRight == 0 || (aLeft == INT_MIN && aRight == -1); }

    // Unsigned Int32
    template<> uint32 AsmJsMath::Mul<uint32>(uint32 aLeft, uint32 aRight) { return aLeft * aRight; }
    template<> uint32 AsmJsMath::DivUnsafe<uint32>(uint32 aLeft, uint32 aRight) { return aLeft / aRight; }
    template<>
    uint32 AsmJsMath::DivChecked<uint32>(uint32 aLeft, uint32 aRight)
    {
        return aRight == 0 ? 0 : aLeft / aRight;
    }
    template<> bool AsmJsMath::DivWouldTrap(uint32 aLeft, uint32 aRight) { return aRight == 0; }
    template<> uint32 AsmJsMath::RemUnsafe<uint32>(uint32 aLeft, uint32 aRight) { return aLeft % aRight; }
    template<>
    uint32 AsmJsMath::RemChecked<uint32>(uint32 aLeft, uint32 aRight)
    {
        return aRight == 0 ? 0 : (aLeft == INT_MIN && aRight == -1) ? INT_MIN : aLeft % aRight;
    }
    template<> bool AsmJsMath::RemWouldTrap(uint32 aLeft, uint32 aRight) { return aRight == 0 || (aLeft == INT_MIN && aRight == -1); }

    // Int64
    template<> int64 AsmJsMath::Mul<int64>(int64 aLeft, int64 aRight) { return aLeft * aRight; }
    template<> int64 AsmJsMath::DivUnsafe<int64>(int64 aLeft, int64 aRight) { return aLeft / aRight; }
    template<>
    int64 AsmJsMath::DivChecked<int64>(int64 aLeft, int64 aRight)
    {
        return aRight == 0 ? 0 : (aLeft == LONGLONG_MIN && aRight == -1) ? LONGLONG_MIN : aLeft / aRight;
    }
    template<> bool AsmJsMath::DivWouldTrap(int64 aLeft, int64 aRight) { return aRight == 0 || (aLeft == LONGLONG_MIN && aRight == -1); }
    template<> int64 AsmJsMath::RemUnsafe<int64>(int64 aLeft, int64 aRight) { return aLeft % aRight; }
    template<>
    int64 AsmJsMath::RemChecked<int64>(int64 aLeft, int64 aRight)
    {
        return ((aRight == 0) || (aLeft == LONGLONG_MIN && aRight == -1)) ? 0 : aLeft % aRight;
    }
    template<> bool AsmJsMath::RemWouldTrap(int64 aLeft, int64 aRight) { return aRight == 0 || (aLeft == LONGLONG_MIN && aRight == -1); }

    // Unsigned Int64
    template<> uint64 AsmJsMath::Mul<uint64>(uint64 aLeft, uint64 aRight) { return aLeft * aRight; }
    template<> uint64 AsmJsMath::DivUnsafe<uint64>(uint64 aLeft, uint64 aRight) { return aLeft / aRight; }
    template<>
    uint64 AsmJsMath::DivChecked<uint64>(uint64 aLeft, uint64 aRight)
    {
        return aRight == 0 ? 0 : aLeft / aRight;
    }
    template<> bool AsmJsMath::DivWouldTrap(uint64 aLeft, uint64 aRight) { return aRight == 0; }
    template<> uint64 AsmJsMath::RemUnsafe<uint64>(uint64 aLeft, uint64 aRight) { return aLeft % aRight; }
    template<>
    uint64 AsmJsMath::RemChecked<uint64>(uint64 aLeft, uint64 aRight)
    {
        return aRight == 0 ? 0 : (aLeft == LONGLONG_MIN && aRight == -1) ? LONGLONG_MIN : aLeft % aRight;
    }
    template<> bool AsmJsMath::RemWouldTrap(uint64 aLeft, uint64 aRight) { return aRight == 0 || (aLeft == LONGLONG_MIN && aRight == -1); }
}
