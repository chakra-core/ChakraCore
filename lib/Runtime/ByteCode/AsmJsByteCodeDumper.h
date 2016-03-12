//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef TEMP_DISABLE_ASMJS
namespace Js {

#if DBG_DUMP
    class AsmJsByteCodeDumper : public ByteCodeDumper
    {
    public:
        static void Dump(AsmJsFunc* func, FunctionBody* body);
        static void DumpConstants(AsmJsFunc* func, FunctionBody* body);
        static void DumpOp(OpCodeAsmJs op, LayoutSize layoutSize, ByteCodeReader& reader, FunctionBody* dumpFunction);

        static void DumpIntReg(RegSlot reg);
        static void DumpDoubleReg(RegSlot reg);
        static void DumpFloatReg(RegSlot reg);
        static void DumpR8Float(float value);
        static void DumpFloat32x4Reg(RegSlot reg);
        static void DumpInt32x4Reg(RegSlot reg);
        
        static void DumpUint32x4Reg(RegSlot reg);
        static void DumpInt16x8Reg(RegSlot reg);
        static void DumpUint16x8Reg(RegSlot reg);
        static void DumpInt8x16Reg(RegSlot reg);
        static void DumpUint8x16Reg(RegSlot reg);
        static void DumpBool32x4Reg(RegSlot reg);
        static void DumpBool16x8Reg(RegSlot reg);
        static void DumpBool8x16Reg(RegSlot reg);

        static void DumpFloat64x2Reg(RegSlot reg);

#define LAYOUT_TYPE(layout) \
    static void Dump##layout(OpCodeAsmJs op, const unaligned OpLayout##layout* data, FunctionBody * dumpFunction, ByteCodeReader& reader);
#define LAYOUT_TYPE_WMS(layout) \
    template <class T> static void Dump##layout(OpCodeAsmJs op, const unaligned T* data, FunctionBody * dumpFunction, ByteCodeReader& reader);
#include "LayoutTypesAsmJs.h"

    };
#endif

}
#endif
