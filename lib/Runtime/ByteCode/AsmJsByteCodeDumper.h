//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ASMJS_PLAT
namespace Js {

#if DBG_DUMP
    class AsmJsByteCodeDumper : public ByteCodeDumper
    {
    public:
        static void Dump(FunctionBody* body, const WAsmJs::TypedRegisterAllocator* typedRegister, AsmJsFunc* asmFunc);
        static void DumpConstants(AsmJsFunc* func, FunctionBody* body);
        static void DumpOp(OpCodeAsmJs op, LayoutSize layoutSize, ByteCodeReader& reader, FunctionBody* dumpFunction);

        static void DumpIntReg(RegSlot reg);
        static void DumpLongReg(RegSlot reg);
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

        static void DumpRegReg(RegSlot reg) { DumpReg(reg); }
        static void DumpIntConstReg(int val) { DumpI4(val); }
        static void DumpLongConstReg(int64 val) { DumpI8(val); }
        static void DumpFloatConstReg(float val) { DumpR4(val); }
        static void DumpDoubleConstReg(double val) { DumpR8(val); }

#define LAYOUT_TYPE(layout) \
    static void Dump##layout(OpCodeAsmJs op, const unaligned OpLayout##layout* data, FunctionBody * dumpFunction, ByteCodeReader& reader);
#define LAYOUT_TYPE_WMS(layout) \
    template <class T> static void Dump##layout(OpCodeAsmJs op, const unaligned T* data, FunctionBody * dumpFunction, ByteCodeReader& reader);
#include "LayoutTypesAsmJs.h"

    private:
        struct WAsmJsMemTag
        {
            char16 valueTag;
            const char16 * heapTag;
        };
        static void InitializeWAsmJsMemTag(ArrayBufferView::ViewType type, _Out_ WAsmJsMemTag * tag);
    };
#endif

}
#endif
