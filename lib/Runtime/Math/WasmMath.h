//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
class WasmMath
{
public:
    template<typename T> static int Eqz(T value);
    template<typename T> static T Shl( T aLeft, T aRight );
    template<typename T> static T Shr( T aLeft, T aRight );
    template<typename T> static T ShrU( T aLeft, T aRight );
    template<typename T> static T VECTORCALL Copysign(T aLeft, T aRight);
    template<typename T> static T Trunc(T aLeft);
    template<typename T> static T Nearest(T aLeft);
    template<typename T> static T PopCnt(T value);
    template<typename T> static T Ctz(T value);
    template<typename T> static T Clz(T value);
    template<typename T> static T Rol(T aLeft, T aRight);
    template<typename T> static T Ror(T aLeft, T aRight);
    template <typename T> bool static LessThan(T aLeft, T aRight);
    template <typename T> bool static LessOrEqual(T aLeft, T aRight);
    template <typename T> using CmpPtr = bool(*)(T a, T b);
    template <typename STYPE, typename UTYPE, UTYPE MAX, UTYPE NEG_ZERO, UTYPE NEG_ONE, CmpPtr<UTYPE> CMP1, CmpPtr<UTYPE> CMP2> static bool isInRange(STYPE srcVal);
    template <typename STYPE> static bool isNaN(STYPE src);
};

} //namespace Wasm

#include "WasmMath.inl"
