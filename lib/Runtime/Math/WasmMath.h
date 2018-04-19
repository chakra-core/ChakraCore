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
    template <
        typename SourceType,
        typename DstType,
        typename ReinterpretType,
        ReinterpretType Max,
        ReinterpretType NegZero,
        ReinterpretType NegOne,
        CmpPtr<ReinterpretType> MaxCmp,
        CmpPtr<ReinterpretType> NegOneCmp,
        bool Saturate,
        DstType MinResult,
        DstType MaxResult>
    static DstType ConvertFloatToInt(SourceType srcVal, _In_ Js::ScriptContext* scriptContext);
    template <typename STYPE> static bool IsNaN(STYPE src);

    template<typename To, typename From> static To SignExtend(To value);

    template <bool Saturate>
    static int64 F32ToI64(float src, _In_ Js::ScriptContext* scriptContext);
    template <bool Saturate>
    static uint64 F32ToU64(float src, _In_ Js::ScriptContext* scriptContext);
    template <bool Saturate>
    static int64 F64ToI64(double src, _In_ Js::ScriptContext* scriptContext);
    template <bool Saturate>
    static uint64 F64ToU64(double src, _In_ Js::ScriptContext* scriptContext);
    template <bool Saturate>
    static int32 F32ToI32(float src, _In_ Js::ScriptContext* scriptContext);
    template <bool Saturate>
    static uint32 F32ToU32(float src, _In_ Js::ScriptContext* scriptContext);
    template <bool Saturate>
    static int32 F64ToI32(double src, _In_ Js::ScriptContext* scriptContext);
    template <bool Saturate>
    static uint32 F64ToU32(double src, _In_ Js::ScriptContext* scriptContext);
};

} //namespace Wasm

#include "WasmMath.inl"
