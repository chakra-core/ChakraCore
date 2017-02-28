//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ASMJS_PLAT
namespace Js
{
    ///----------------------------------------------------------------------------
    ///
    /// enum OpCodeAsmJs
    ///
    /// OpCodeAsmJs defines the set of p-code instructions available for byte-code in Asm.Js.
    ///
    ///----------------------------------------------------------------------------
    enum class OpCodeAsmJs : ushort {
#define DEF_OP(x, y, ...) x,
#include "OpCodeListAsmJs.h"
        MaxByteSizedOpcodes = 255,
#include "ExtendedOpCodeListAsmJs.h"
        ByteCodeLast,
#undef DEF_OP
        Count  // Number of operations
    };

    inline OpCodeAsmJs operator+(OpCodeAsmJs o1, OpCodeAsmJs o2) { return (OpCodeAsmJs)((uint)o1 + (uint)o2); }
    inline uint operator+(OpCodeAsmJs o1, uint i) { return ((uint)o1 + i); }
    inline uint operator+(uint i, OpCodeAsmJs &o2) { return (i + (uint)o2); }
    inline OpCodeAsmJs operator++(OpCodeAsmJs &o) { return o = (OpCodeAsmJs)(o + 1U); }
    inline OpCodeAsmJs operator++(OpCodeAsmJs &o, int) { OpCodeAsmJs prev_o = o;  o = (OpCodeAsmJs)(o + 1U); return prev_o; }
    inline OpCodeAsmJs operator-(OpCodeAsmJs o1, OpCodeAsmJs o2) { return (OpCodeAsmJs)((uint)o1 - (uint)o2); }
    inline uint operator-(OpCodeAsmJs o1, uint i) { return ((uint)o1 - i); }
    inline uint operator-(uint i, OpCodeAsmJs &o2) { return (i - (uint)o2); }
    inline OpCodeAsmJs operator--(OpCodeAsmJs &o) { return o = (OpCodeAsmJs)(o - 1U); }
    inline OpCodeAsmJs operator--(OpCodeAsmJs &o, int) { return o = (OpCodeAsmJs)(o - 1U); }
    inline uint operator<<(OpCodeAsmJs o1, uint i) { return ((uint)o1 << i); }
    inline OpCodeAsmJs& operator+=(OpCodeAsmJs &o, uint i) { return (o = (OpCodeAsmJs)(o + i)); }
    inline OpCodeAsmJs& operator-=(OpCodeAsmJs &o, uint i) { return (o = (OpCodeAsmJs)(o - i)); }
    inline bool operator==(OpCodeAsmJs &o, uint i) { return ((uint)(o) == i); }
    inline bool operator==(uint i, OpCodeAsmJs &o) { return (i == (uint)(o)); }
    inline bool operator!=(OpCodeAsmJs &o, uint i) { return ((uint)(o) != i); }
    inline bool operator!=(uint i, OpCodeAsmJs &o) { return (i != (uint)(o)); }
    inline bool operator<(OpCodeAsmJs &o, uint i) { return ((uint)(o) < i); }
    inline bool operator<(uint i, OpCodeAsmJs &o) { return (i < (uint)(o)); }
    inline bool operator<=(OpCodeAsmJs &o, uint i) { return ((uint)(o) <= i); }
    inline bool operator<=(uint i, OpCodeAsmJs &o) { return (i <= (uint)(o)); }
    inline bool operator<=(OpCodeAsmJs o1, OpCode o2) { return ((OpCode)o1 <= (o2)); }
    inline bool operator>(OpCodeAsmJs &o, uint i) { return ((uint)(o) > i); }
    inline bool operator>(uint i, OpCodeAsmJs &o) { return (i > (uint)(o)); }
    inline bool operator>=(OpCodeAsmJs &o, uint i) { return ((uint)(o) >= i); }
    inline bool operator>=(uint i, OpCodeAsmJs &o) { return (i >= (uint)(o)); }

    inline BOOL IsSimd128AsmJsOpcode(OpCodeAsmJs o)
    {
        return (o > Js::OpCodeAsmJs::Simd128_Start && o < Js::OpCodeAsmJs::Simd128_End) || (o > Js::OpCodeAsmJs::Simd128_Start_Extend && o < Js::OpCodeAsmJs::Simd128_End_Extend);
    }
    inline uint Simd128AsmJsOpcodeCount()
    {
        return (uint)(Js::OpCodeAsmJs::Simd128_End - Js::OpCodeAsmJs::Simd128_Start) + 1 + (uint)(Js::OpCodeAsmJs::Simd128_End_Extend - Js::OpCodeAsmJs::Simd128_Start_Extend) + 1;
    }

    ///----------------------------------------------------------------------------
    ///
    /// enum OpLayoutTypeAsmJs
    ///
    /// OpLayoutTypeAsmJs defines a set of layouts available for OpCodes.  These layouts
    /// correspond to "OpLayout" structs defined below, such as "OpLayoutReg1".
    ///
    ///----------------------------------------------------------------------------

    BEGIN_ENUM_UINT( OpLayoutTypeAsmJs )
        // This define only one enum for each layout type, but not for each layout variant
#define LAYOUT_TYPE(x) x,
#define LAYOUT_TYPE_WMS LAYOUT_TYPE
#include "LayoutTypesAsmJs.h"
        Count,
    END_ENUM_UINT()

#pragma pack(push, 1)
    /// Asm.js Layout

    template <typename SizePolicy>
    struct OpLayoutT_AsmTypedArr
    {
        // force encode 4 bytes because it can be a value
        uint32                               SlotIndex;
        typename SizePolicy::RegSlotType     Value;
        Js::ArrayBufferView::ViewType        ViewType;
    };

    template <typename SizePolicy>
    struct OpLayoutT_WasmMemAccess
    {
        uint32                               Offset;
        typename SizePolicy::RegSlotType     SlotIndex;
        typename SizePolicy::RegSlotType     Value;
        Js::ArrayBufferView::ViewType        ViewType;
    };

    template <typename SizePolicy>
    struct OpLayoutT_AsmCall
    {
        typename SizePolicy::ArgSlotType     ArgCount;
        typename SizePolicy::RegSlotSType    Return;
        typename SizePolicy::RegSlotType     Function;
        int8                                 ReturnType;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg1
    {
        typename SizePolicy::RegSlotType     R0;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg2
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg3
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg4
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg5
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
    };

    template <typename SizePolicy>
    struct OpLayoutT_AsmReg6
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg7
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
        typename SizePolicy::RegSlotType     R6;
    };

    template <typename SizePolicy>
    struct OpLayoutT_AsmReg9
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
        typename SizePolicy::RegSlotType     R6;
        typename SizePolicy::RegSlotType     R7;
        typename SizePolicy::RegSlotType     R8;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg10
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
        typename SizePolicy::RegSlotType     R6;
        typename SizePolicy::RegSlotType     R7;
        typename SizePolicy::RegSlotType     R8;
        typename SizePolicy::RegSlotType     R9;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg11
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
        typename SizePolicy::RegSlotType     R6;
        typename SizePolicy::RegSlotType     R7;
        typename SizePolicy::RegSlotType     R8;
        typename SizePolicy::RegSlotType     R9;
        typename SizePolicy::RegSlotType     R10;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg17
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
        typename SizePolicy::RegSlotType     R6;
        typename SizePolicy::RegSlotType     R7;
        typename SizePolicy::RegSlotType     R8;
        typename SizePolicy::RegSlotType     R9;
        typename SizePolicy::RegSlotType     R10;
        typename SizePolicy::RegSlotType     R11;
        typename SizePolicy::RegSlotType     R12;
        typename SizePolicy::RegSlotType     R13;
        typename SizePolicy::RegSlotType     R14;
        typename SizePolicy::RegSlotType     R15;
        typename SizePolicy::RegSlotType     R16;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg18
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
        typename SizePolicy::RegSlotType     R6;
        typename SizePolicy::RegSlotType     R7;
        typename SizePolicy::RegSlotType     R8;
        typename SizePolicy::RegSlotType     R9;
        typename SizePolicy::RegSlotType     R10;
        typename SizePolicy::RegSlotType     R11;
        typename SizePolicy::RegSlotType     R12;
        typename SizePolicy::RegSlotType     R13;
        typename SizePolicy::RegSlotType     R14;
        typename SizePolicy::RegSlotType     R15;
        typename SizePolicy::RegSlotType     R16;
        typename SizePolicy::RegSlotType     R17;
    };
    template <typename SizePolicy>
    struct OpLayoutT_AsmReg19
    {
        typename SizePolicy::RegSlotType     R0;
        typename SizePolicy::RegSlotType     R1;
        typename SizePolicy::RegSlotType     R2;
        typename SizePolicy::RegSlotType     R3;
        typename SizePolicy::RegSlotType     R4;
        typename SizePolicy::RegSlotType     R5;
        typename SizePolicy::RegSlotType     R6;
        typename SizePolicy::RegSlotType     R7;
        typename SizePolicy::RegSlotType     R8;
        typename SizePolicy::RegSlotType     R9;
        typename SizePolicy::RegSlotType     R10;
        typename SizePolicy::RegSlotType     R11;
        typename SizePolicy::RegSlotType     R12;
        typename SizePolicy::RegSlotType     R13;
        typename SizePolicy::RegSlotType     R14;
        typename SizePolicy::RegSlotType     R15;
        typename SizePolicy::RegSlotType     R16;
        typename SizePolicy::RegSlotType     R17;
        typename SizePolicy::RegSlotType     R18;
    };

#define RegLayoutType typename SizePolicy::RegSlotType
#define IntLayoutType typename SizePolicy::RegSlotType
#define LongLayoutType typename SizePolicy::RegSlotType
#define FloatLayoutType typename SizePolicy::RegSlotType
#define DoubleLayoutType typename SizePolicy::RegSlotType
#define IntConstLayoutType int
#define LongConstLayoutType int64
#define FloatConstLayoutType float
#define DoubleConstLayoutType double
#define Float32x4LayoutType typename SizePolicy::RegSlotType
#define Bool32x4LayoutType typename SizePolicy::RegSlotType
#define Int32x4LayoutType typename SizePolicy::RegSlotType
#define Float64x2LayoutType typename SizePolicy::RegSlotType
#define Int16x8LayoutType typename SizePolicy::RegSlotType
#define Bool16x8LayoutType typename SizePolicy::RegSlotType
#define Int8x16LayoutType typename SizePolicy::RegSlotType
#define Bool8x16LayoutType typename SizePolicy::RegSlotType
#define Uint32x4LayoutType typename SizePolicy::RegSlotType
#define Uint16x8LayoutType typename SizePolicy::RegSlotType
#define Uint8x16LayoutType typename SizePolicy::RegSlotType
#define LAYOUT_FIELDS_HELPER(x, y) x ## y
#define LAYOUT_FIELDS_DEF(x, y) LAYOUT_FIELDS_HELPER(x, y)
#define LAYOUT_TYPE_WMS_REG2(layout, t0, t1) \
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
    };
#define LAYOUT_TYPE_WMS_REG3(layout, t0, t1, t2) \
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
    };

#define LAYOUT_TYPE_WMS_REG4(layout, t0, t1, t2, t3)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
    };
#define LAYOUT_TYPE_WMS_REG5(layout, t0, t1, t2, t3, t4)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
    };
#define LAYOUT_TYPE_WMS_REG6(layout, t0, t1, t2, t3, t4, t5)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
    };
#define LAYOUT_TYPE_WMS_REG7(layout, t0, t1, t2, t3, t4, t5, t6)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
        t6##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6);\
    };
#define LAYOUT_TYPE_WMS_REG9(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
        t6##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6);\
        t7##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7);\
        t8##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8);\
    };
#define LAYOUT_TYPE_WMS_REG10(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
        t6##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6);\
        t7##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7);\
        t8##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8);\
        t9##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9);\
    };
#define LAYOUT_TYPE_WMS_REG11(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
        t6##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6);\
        t7##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7);\
        t8##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8);\
        t9##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9);\
        t10##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10);\
    };
#define LAYOUT_TYPE_WMS_REG17(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
        t6##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6);\
        t7##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7);\
        t8##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8);\
        t9##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9);\
        t10##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10);\
        t11##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t11(), 11);\
        t12##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t12(), 12);\
        t13##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t13(), 13);\
        t14##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t14(), 14);\
        t15##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t15(), 15);\
        t16##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t16(), 16);\
    };
#define LAYOUT_TYPE_WMS_REG18(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
        t6##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6);\
        t7##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7);\
        t8##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8);\
        t9##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9);\
        t10##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10);\
        t11##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t11(), 11);\
        t12##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t12(), 12);\
        t13##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t13(), 13);\
        t14##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t14(), 14);\
        t15##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t15(), 15);\
        t16##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t16(), 16);\
        t17##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t17(), 17);\
    };
#define LAYOUT_TYPE_WMS_REG19(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18)\
    template <typename SizePolicy> struct OpLayoutT_##layout\
    {\
        t0##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0);\
        t1##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1);\
        t2##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2);\
        t3##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3);\
        t4##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4);\
        t5##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5);\
        t6##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6);\
        t7##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7);\
        t8##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8);\
        t9##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9);\
        t10##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10);\
        t11##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t11(), 11);\
        t12##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t12(), 12);\
        t13##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t13(), 13);\
        t14##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t14(), 14);\
        t15##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t15(), 15);\
        t16##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t16(), 16);\
        t17##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t17(), 17);\
        t18##LayoutType LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t18(), 18);\
    };
#include "LayoutTypesAsmJs.h"
#undef RegLayoutType
#undef IntLayoutType
#undef LongLayoutType
#undef FloatLayoutType
#undef DoubleLayoutType
#undef IntConstLayoutType
#undef LongConstLayoutType
#undef FloatConstLayoutType
#undef DoubleConstLayoutType
#undef Float32x4LayoutType
#undef Bool32x4LayoutType
#undef Int32x4LayoutType
#undef Float64x2LayoutType
#undef Int16x8LayoutType
#undef Bool16x8LayoutType
#undef Int8x16LayoutType
#undef Bool8x16LayoutType
#undef Uint32x4LayoutType
#undef Uint16x8LayoutType
#undef Uint8x16LayoutType

    template <typename SizePolicy>
    struct OpLayoutT_AsmUnsigned1
    {
        typename SizePolicy::UnsignedType C1;
    };

    struct OpLayoutAsmBr
    {
        int32  RelativeJumpOffset;
    };

    template <typename SizePolicy>
    struct OpLayoutT_BrInt1
    {
        int32  RelativeJumpOffset;
        typename SizePolicy::RegSlotType     I1;
    };

    template <typename SizePolicy>
    struct OpLayoutT_BrInt2
    {
        int32  RelativeJumpOffset;
        typename SizePolicy::RegSlotType     I1;
        typename SizePolicy::RegSlotType     I2;
    };

    template <typename SizePolicy>
    struct OpLayoutT_BrInt1Const1
    {
        int32  RelativeJumpOffset;
        typename SizePolicy::RegSlotType     I1;
        int32     C1;
    };

    template <typename SizePolicy>
    struct OpLayoutT_AsmSimdTypedArr
    {
        // force encode 4 bytes because it can be a value
        uint32                               SlotIndex;
        typename SizePolicy::RegSlotType     Value;
        ArrayBufferView::ViewType            ViewType;
        int8                                 DataWidth; // # of bytes to load/store
    };

    // Generate the multi size layout type defs
#define LAYOUT_TYPE_WMS(layout) \
    typedef OpLayoutT_##layout<LargeLayoutSizePolicy> OpLayout##layout##_Large; \
    typedef OpLayoutT_##layout<MediumLayoutSizePolicy> OpLayout##layout##_Medium; \
    typedef OpLayoutT_##layout<SmallLayoutSizePolicy> OpLayout##layout##_Small;
#include "LayoutTypesAsmJs.h"

#pragma pack(pop)

    // Generate structure to automatically map layout to its info
    template <OpLayoutTypeAsmJs::_E layout> struct OpLayoutInfoAsmJs;

#define LAYOUT_TYPE(layout) \
    CompileAssert(sizeof(OpLayout##layout) <= MaxLayoutSize); \
    template <> struct OpLayoutInfoAsmJs<OpLayoutTypeAsmJs::layout> \
        {  \
        static const bool HasMultiSizeLayout = false; \
        };

#define LAYOUT_TYPE_WMS(layout) \
    CompileAssert(sizeof(OpLayout##layout##_Large) <= MaxLayoutSize); \
    template <> struct OpLayoutInfoAsmJs<OpLayoutTypeAsmJs::layout> \
        {  \
        static const bool HasMultiSizeLayout = true; \
        };
#include "LayoutTypesAsmJs.h"

    // Generate structure to automatically map opcode to its info
    // Also generate assert to make sure the layout and opcode use the same macro with and without multiple size layout
    template <OpCodeAsmJs opcode> struct OpCodeInfoAsmJs;

#define DEFINE_OPCODEINFO(op, layout, extended) \
    CompileAssert(!OpLayoutInfoAsmJs<OpLayoutTypeAsmJs::layout>::HasMultiSizeLayout); \
    template <> struct OpCodeInfoAsmJs<OpCodeAsmJs::op> \
        { \
        static const OpLayoutTypeAsmJs::_E Layout = OpLayoutTypeAsmJs::layout; \
        static const bool HasMultiSizeLayout = false; \
        static const bool IsExtendedOpcode = extended; \
        typedef OpLayout##layout LayoutType; \
        };
#define DEFINE_OPCODEINFO_WMS(op, layout, extended) \
    CompileAssert(OpLayoutInfoAsmJs<OpLayoutTypeAsmJs::layout>::HasMultiSizeLayout); \
    template <> struct OpCodeInfoAsmJs<OpCodeAsmJs::op> \
        { \
        static const OpLayoutTypeAsmJs::_E Layout = OpLayoutTypeAsmJs::layout; \
        static const bool HasMultiSizeLayout = true; \
        static const bool IsExtendedOpcode = extended; \
        typedef OpLayout##layout##_Large LayoutType_Large; \
        typedef OpLayout##layout##_Medium LayoutType_Medium; \
        typedef OpLayout##layout##_Small LayoutType_Small; \
        };
#define MACRO(op, layout, ...) DEFINE_OPCODEINFO(op, layout, false)
#define MACRO_WMS(op, layout, ...) DEFINE_OPCODEINFO_WMS(op, layout, false)
#define MACRO_EXTEND(op, layout, ...) DEFINE_OPCODEINFO(op, layout, true)
#define MACRO_EXTEND_WMS(op, layout, ...) DEFINE_OPCODEINFO_WMS(op, layout, true)
#include "OpCodesAsmJs.h"
#undef DEFINE_OPCODEINFO
#undef DEFINE_OPCODEINFO_WMS
}
#endif
