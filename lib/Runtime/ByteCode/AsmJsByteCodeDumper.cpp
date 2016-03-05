//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

#ifndef TEMP_DISABLE_ASMJS
#if DBG_DUMP
#include "Language/AsmJsModule.h"
#include "ByteCode/AsmJsByteCodeDumper.h"

namespace Js
{
    void AsmJsByteCodeDumper::Dump(AsmJsFunc* func, FunctionBody* body)
    {
        ByteCodeReader reader;
        reader.Create(body);
        StatementReader statementReader;
        statementReader.Create(body);
        body->DumpFullFunctionName();
        Output::Print(_u(" Asm.js ("));
        const ArgSlot argCount = func->GetArgCount();
        for (ArgSlot i = 0; i < argCount; i++)
        {
            AsmJsType var = func->GetArgType(i);
            if (i > 0)
            {
                Output::Print(_u(", "));
            }
            if (var.isDouble())
            {
                Output::Print(_u("+In%hu"), i);
            }
            else if (var.isFloat())
            {
                Output::Print(_u("flt(In%hu)"), i);
            }
            else if (var.isInt())
            {
                Output::Print(_u("In%hu|0"), i);
            }
            else if (var.isSIMDType())
            {
                switch (var.GetWhich())
                {
                case AsmJsType::Int32x4:
                    Output::Print(_u("I4(In%hu)"), i);
                    break;
                case AsmJsType::Float32x4:
                    Output::Print(_u("F4(In%hu)"), i);
                    break;
                case AsmJsType::Float64x2:
                    Output::Print(_u("D2(In%hu)"), i);
                    break;
                }
            }
            else
            {
                Assert(UNREACHED);
            }
        }

        Output::Print(_u(") "));
        Output::Print(_u("(size: %d [%d])\n"), body->GetByteCodeCount(), body->GetByteCodeWithoutLDACount());
        const auto& intRegisters = func->GetRegisterSpace<int>();
        const auto& doubleRegisters = func->GetRegisterSpace<double>();
        const auto& floatRegisters = func->GetRegisterSpace<float>();
        Output::Print(
            _u("      Integer : %u locals (%u temps from I%u)\n"),
            intRegisters.GetVarCount(),
            intRegisters.GetTmpCount(),
            intRegisters.GetFirstTmpRegister());
        Output::Print(
            _u("      Doubles : %u locals (%u temps from D%u)\n"),
            doubleRegisters.GetVarCount(),
            doubleRegisters.GetTmpCount(),
            doubleRegisters.GetFirstTmpRegister());

        Output::Print(
            _u("      Floats : %u locals (%u temps from F%u)\n"),
            floatRegisters.GetVarCount(),
            floatRegisters.GetTmpCount(),
            floatRegisters.GetFirstTmpRegister());

        const auto& simdRegisters = func->GetRegisterSpace<AsmJsSIMDValue>();
        Output::Print(
            _u("      SIMDs : %u locals (%u temps from SIMD%u)\n"),
            simdRegisters.GetVarCount(),
            simdRegisters.GetTmpCount(),
            simdRegisters.GetFirstTmpRegister());

        uint32 statementIndex = 0;
        DumpConstants(func, body);

        Output::Print(_u("    Implicit Arg Ins:\n    ======== =====\n    "));
        int iArg = intRegisters.GetConstCount(), dArg = doubleRegisters.GetConstCount(), fArg = floatRegisters.GetConstCount();
        int simdArg = simdRegisters.GetConstCount();
        for (ArgSlot i = 0; i < argCount; i++)
        {
            const AsmJsType& var = func->GetArgType(i);
            if (var.isDouble())
            {
                Output::Print(_u(" D%d  In%d"), dArg++, i);
            }
            else if (var.isFloat())
            {
                Output::Print(_u(" F%d  In%d"), fArg++, i);
            }
            else if (var.isInt())
            {
                Output::Print(_u(" I%d  In%d"), iArg++, i);
            }
            else if (var.isSIMDType())
            {
                Output::Print(_u(" SIMD%d  In%d"), simdArg++, i);
            }
            else
            {
                Assert(UNREACHED);
            }
            Output::Print(_u("\n    "));
        }
        Output::Print(_u("\n"));

        if (func->GetReturnType() == AsmJsRetType::Void)
        {
            Output::Print(_u("    0000   %-20s R0\n"), OpCodeUtilAsmJs::GetOpCodeName(OpCodeAsmJs::LdUndef));
        }

        while (true)
        {
            while (statementReader.AtStatementBoundary(&reader))
            {
                body->PrintStatementSourceLine(statementIndex);
                statementIndex = statementReader.MoveNextStatementBoundary();
            }
            int byteOffset = reader.GetCurrentOffset();
            LayoutSize layoutSize;
            OpCodeAsmJs op = (OpCodeAsmJs)reader.ReadOp(layoutSize);
            if (op == OpCodeAsmJs::EndOfBlock)
            {
                Assert(reader.GetCurrentOffset() == body->GetByteCode()->GetLength());
                break;
            }
            Output::Print(_u("    %04x %2s"), byteOffset, layoutSize == LargeLayout ? _u("L-") : layoutSize == MediumLayout ? _u("M-") : _u(""));
            DumpOp(op, layoutSize, reader, body);
            if (Js::Configuration::Global.flags.Verbose)
            {
                int layoutStart = byteOffset + 2; // Account for the prefix op
                int endByteOffset = reader.GetCurrentOffset();
                Output::SkipToColumn(70);
                if (layoutSize == LargeLayout)
                {
                    Output::Print(_u("%02X "),
                        op > Js::OpCodeAsmJs::MaxByteSizedOpcodes ?
                        Js::OpCodeAsmJs::ExtendedLargeLayoutPrefix : Js::OpCodeAsmJs::LargeLayoutPrefix);
                }
                else if (layoutSize == MediumLayout)
                {
                    Output::Print(_u("%02X "),
                        op > Js::OpCodeAsmJs::MaxByteSizedOpcodes ?
                        Js::OpCodeAsmJs::ExtendedMediumLayoutPrefix : Js::OpCodeAsmJs::MediumLayoutPrefix);
                }
                else
                {
                    Assert(layoutSize == SmallLayout);
                    if (op > Js::OpCodeAsmJs::MaxByteSizedOpcodes)
                    {
                        Output::Print(_u("%02X "), Js::OpCodeAsmJs::ExtendedOpcodePrefix);
                    }
                    else
                    {
                        Output::Print(_u("   "));
                        layoutStart--; // don't have a prefix
                    }
                }

                Output::Print(_u("%02x"), (byte)op);
                for (int i = layoutStart; i < endByteOffset; i++)
                {
                    Output::Print(_u(" %02x"), reader.GetRawByte(i));
                }
            }
            Output::Print(_u("\n"));
        }
        if (statementReader.AtStatementBoundary(&reader))
        {
            body->PrintStatementSourceLine(statementIndex);
            statementIndex = statementReader.MoveNextStatementBoundary();
        }
        Output::Print(_u("\n"));
        Output::Flush();
    }

    void AsmJsByteCodeDumper::DumpConstants(AsmJsFunc* func, FunctionBody* body)
    {
        const auto& intRegisters = func->GetRegisterSpace<int>();
        const auto& doubleRegisters = func->GetRegisterSpace<double>();
        const auto& floatRegisters = func->GetRegisterSpace<float>();

        int nbIntConst = intRegisters.GetConstCount();
        int nbDoubleConst = doubleRegisters.GetConstCount();
        int nbFloatConst = floatRegisters.GetConstCount();

        int* constTable = (int*)((Var*)body->GetConstTable() + (AsmJsFunctionMemory::RequiredVarConstants - 1));
        if (nbIntConst > 0)
        {
            Output::Print(_u("    Constant Integer:\n    ======== =======\n    "));
            for (int i = 0; i < nbIntConst; i++)
            {
                Output::Print(_u(" I%d  %d\n    "), i, *constTable);
                ++constTable;
            }
        }

        float* floatTable = (float*)constTable;
        Output::Print(_u("\n"));
        if (nbFloatConst > 0)
        {

            Output::Print(_u("    Constant Floats:\n    ======== ======\n    "));
            for (int i = 0; i < nbFloatConst; i++)
            {
                Output::Print(_u(" F%d  %.4f\n    "), i, *floatTable);
                ++floatTable;
                ++constTable;
            }
        }

        double* doubleTable = (double*)constTable;
        Output::Print(_u("\n"));
        if (nbDoubleConst > 0)
        {

            Output::Print(_u("    Constant Doubles:\n    ======== ======\n    "));
            for (int i = 0; i < nbDoubleConst; i++)
            {
                Output::Print(_u(" D%d  %.4f\n    "), i, *doubleTable);
                ++doubleTable;
            }
        }
        // SIMD reg space is un-typed.
        // We print each register in its 3 possible types to ease debugging.
        const auto& simdRegisters = func->GetRegisterSpace<AsmJsSIMDValue>();
        int nbSimdConst = simdRegisters.GetConstCount();

        Output::Print(_u("\n"));
        if (nbSimdConst > 0)
        {
            AsmJsSIMDValue* simdTable = (AsmJsSIMDValue*)doubleTable;
            Output::Print(_u("    Constant SIMD values:\n    ======== ======\n    "));
            for (int i = 0; i < nbSimdConst; i++)
            {
                Output::Print(_u("SIMD%d "), i);
                Output::Print(_u("\tI4(%d, %d, %d, %d),"), simdTable->i32[SIMD_X], simdTable->i32[SIMD_Y], simdTable->i32[SIMD_Z], simdTable->i32[SIMD_W]);
                Output::Print(_u("\tF4(%.4f, %.4f, %.4f, %.4f),"), simdTable->f32[SIMD_X], simdTable->f32[SIMD_Y], simdTable->f32[SIMD_Z], simdTable->f32[SIMD_W]);
                Output::Print(_u("\tD2(%.4f, %.4f)\n    "), simdTable->f64[SIMD_X], simdTable->f64[SIMD_Y]);
                ++simdTable;
            }
        }
        Output::Print(_u("\n"));
    }

    void AsmJsByteCodeDumper::DumpOp(OpCodeAsmJs op, LayoutSize layoutSize, ByteCodeReader& reader, FunctionBody* dumpFunction)
    {
        Output::Print(_u("%-20s"), OpCodeUtilAsmJs::GetOpCodeName(op));
        OpLayoutTypeAsmJs nType = OpCodeUtilAsmJs::GetOpCodeLayout(op);
        switch (layoutSize * OpLayoutTypeAsmJs::Count + nType)
        {
#define LAYOUT_TYPE(layout) \
            case OpLayoutTypeAsmJs::layout: \
                Assert(layoutSize == SmallLayout); \
                Dump##layout(op, reader.layout(), dumpFunction, reader); \
                break;
#define LAYOUT_TYPE_WMS(layout) \
            case SmallLayout * OpLayoutTypeAsmJs::Count + OpLayoutTypeAsmJs::layout: \
                Dump##layout(op, reader.layout##_Small(), dumpFunction, reader); \
                break; \
            case MediumLayout * OpLayoutTypeAsmJs::Count + OpLayoutTypeAsmJs::layout: \
                Dump##layout(op, reader.layout##_Medium(), dumpFunction, reader); \
                break; \
            case LargeLayout * OpLayoutTypeAsmJs::Count + OpLayoutTypeAsmJs::layout: \
                Dump##layout(op, reader.layout##_Large(), dumpFunction, reader); \
                break;
#include "LayoutTypesAsmJs.h"

        default:
            AssertMsg(false, "Unknown OpLayout");
            break;
        }
    }

    void AsmJsByteCodeDumper::DumpIntReg(RegSlot reg)
    {
        Output::Print(_u(" I%d "), (int)reg);
    }
    void AsmJsByteCodeDumper::DumpDoubleReg(RegSlot reg)
    {
        Output::Print(_u(" D%d "), (int)reg);
    }

    void AsmJsByteCodeDumper::DumpFloatReg(RegSlot reg)
    {
        Output::Print(_u(" F%d "), (int)reg);
    }
    void AsmJsByteCodeDumper::DumpR8Float(float value)
    {
        Output::Print(_u(" float:%f "), value);
    }

    // Float32x4
    void AsmJsByteCodeDumper::DumpFloat32x4Reg(RegSlot reg)
    {
        Output::Print(_u("F4_%d "), (int)reg);
    }

    // Int32x4
    void AsmJsByteCodeDumper::DumpInt32x4Reg(RegSlot reg)
    {
        Output::Print(_u("I4_%d "), (int)reg);
    }

    // Float64x2
    void AsmJsByteCodeDumper::DumpFloat64x2Reg(RegSlot reg)
    {
        Output::Print(_u("D2_%d "), (int)reg);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpElementSlot(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        switch (op)
        {
        case OpCodeAsmJs::LdSlot:
        case OpCodeAsmJs::LdSlotArr:
            Output::Print(_u(" R%d = R%d[%d] "), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::LdArr_Func:
            Output::Print(_u(" R%d = R%d[I%d] "), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::StSlot_Int:
            Output::Print(_u(" R%d[%d] = I%d "), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::StSlot_Flt:
            Output::Print(_u(" R%d[%d] = F%d "), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::StSlot_Db:
            Output::Print(_u(" R%d[%d] = D%d "), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::LdSlot_Int:
            Output::Print(_u(" I%d = R%d[%d] "), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::LdSlot_Flt:
            Output::Print(_u(" F%d = R%d[%d] "), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::LdSlot_Db:
            Output::Print(_u(" D%d = R%d[%d] "), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_F4:
            Output::Print(_u(" F4_%d = R%d[%d] "), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_I4:
            Output::Print(_u(" I4_%d = R%d[%d] "), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_D2:
            Output::Print(_u(" D2_%d = R%d[%d] "), data->Value, data->Instance, data->SlotIndex);
            break;

        case OpCodeAsmJs::Simd128_StSlot_F4:
            Output::Print(_u(" R%d[%d]  = F4_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_I4:
            Output::Print(_u(" R%d[%d]  = I4_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_D2:
            Output::Print(_u(" R%d[%d]  = D2_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        default:
        {
            AssertMsg(false, "Unknown OpCode for OpLayoutElementSlot");
            break;
        }
        }
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmTypedArr(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        char16* heapTag = nullptr;
        char16 valueTag = 'I';
        switch (data->ViewType)
        {
        case ArrayBufferView::TYPE_INT8:
            heapTag = _u("HEAP8"); valueTag = 'I';  break;
        case ArrayBufferView::TYPE_UINT8:
            heapTag = _u("HEAPU8"); valueTag = 'U'; break;
        case ArrayBufferView::TYPE_INT16:
            heapTag = _u("HEAP16"); valueTag = 'I'; break;
        case ArrayBufferView::TYPE_UINT16:
            heapTag = _u("HEAPU16"); valueTag = 'U'; break;
        case ArrayBufferView::TYPE_INT32:
            heapTag = _u("HEAP32"); valueTag = 'I'; break;
        case ArrayBufferView::TYPE_UINT32:
            heapTag = _u("HEAPU32"); valueTag = 'U'; break;
        case ArrayBufferView::TYPE_FLOAT32:
            heapTag = _u("HEAPF32"); valueTag = 'F'; break;
        case ArrayBufferView::TYPE_FLOAT64:
            heapTag = _u("HEAPF64"); valueTag = 'D'; break;
        default:
            Assert(false);
            __assume(false);
            break;
        }

        switch (op)
        {
        case OpCodeAsmJs::LdArr:
            Output::Print(_u("%c%d = %s[I%d]"), valueTag, data->Value, heapTag, data->SlotIndex); break;
        case OpCodeAsmJs::LdArrConst:
            Output::Print(_u("%c%d = %s[%d]"), valueTag, data->Value, heapTag, data->SlotIndex); break;
        case OpCodeAsmJs::StArr:
            Output::Print(_u("%s[I%d] = %c%d"), heapTag, data->SlotIndex, valueTag, data->Value); break;
        case OpCodeAsmJs::StArrConst:
            Output::Print(_u("%s[%d] = %c%d"), heapTag, data->SlotIndex, valueTag, data->Value); break;
        default:
            Assert(false);
            __assume(false);
            break;
        }
    }

    void AsmJsByteCodeDumper::DumpStartCall(OpCodeAsmJs op, const unaligned OpLayoutStartCall* data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        Assert(op == OpCodeAsmJs::StartCall || op == OpCodeAsmJs::I_StartCall);
        Output::Print(_u(" ArgSize: %d"), data->ArgCount);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmCall(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        if (data->Return != Constants::NoRegister)
        {
            DumpReg((RegSlot)data->Return);
            Output::Print(_u("="));
        }
        Output::Print(_u(" R%d(ArgCount: %d)"), data->Function, data->ArgCount);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmUnsigned1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpU4(data->C1);
    }
    void AsmJsByteCodeDumper::DumpEmpty(OpCodeAsmJs op, const unaligned OpLayoutEmpty* data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        // empty
    }

    void AsmJsByteCodeDumper::DumpAsmBr(OpCodeAsmJs op, const unaligned OpLayoutAsmBr* data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpOffset(data->RelativeJumpOffset, reader);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg3(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg5(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg6(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg7(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
        DumpReg(data->R6);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg2IntConst1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpI4(data->C2);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Double1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpDoubleReg(data->D1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Float1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpFloatReg(data->F1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpDouble1Int1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpDoubleReg(data->D0);
        DumpIntReg(data->I1);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpDouble1Float1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpDoubleReg(data->D0);
        DumpFloatReg(data->F1);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpDouble1Reg1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpDoubleReg(data->D0);
        DumpReg(data->R1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat1Reg1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloatReg(data->F0);
        DumpReg(data->R1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Reg1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpReg(data->R1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpReg1Double1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpDoubleReg(data->D1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpReg1Float1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpFloatReg(data->F1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Double2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpDoubleReg(data->D1);
        DumpDoubleReg(data->D2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Float2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpFloatReg(data->F1);
        DumpFloatReg(data->F2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpReg1Int1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpIntReg(data->I1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpIntReg(data->I1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Const1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpI4(data->C1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt3(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpIntReg(data->I1);
        DumpIntReg(data->I2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpDouble2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpDoubleReg(data->D0);
        DumpDoubleReg(data->D1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloatReg(data->F0);
        DumpFloatReg(data->F1);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpFloat3(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloatReg(data->F0);
        DumpFloatReg(data->F1);
        DumpFloatReg(data->F2);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpFloat1Double1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloatReg(data->F0);
        DumpDoubleReg(data->D1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat1Int1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloatReg(data->F0);
        DumpIntReg(data->I1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpDouble3(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpDoubleReg(data->D0);
        DumpDoubleReg(data->D1);
        DumpDoubleReg(data->D2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpBrInt1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpOffset(data->RelativeJumpOffset, reader);
        DumpIntReg(data->I1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpBrInt2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpOffset(data->RelativeJumpOffset, reader);
        DumpIntReg(data->I1);
        DumpIntReg(data->I2);
    }

    // Float32x4
    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat32x4Reg(data->F4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_3(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat32x4Reg(data->F4_1);
        DumpFloat32x4Reg(data->F4_2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat32x4Reg(data->F4_1);
        DumpFloat32x4Reg(data->F4_2);
        DumpFloat32x4Reg(data->F4_3);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_1Float4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloatReg(data->F1);
        DumpFloatReg(data->F2);
        DumpFloatReg(data->F3);
        DumpFloatReg(data->F4);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_2Int4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat32x4Reg(data->F4_1);
        DumpIntReg(data->I2);
        DumpIntReg(data->I3);
        DumpIntReg(data->I4);
        DumpIntReg(data->I5);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_3Int4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat32x4Reg(data->F4_1);
        DumpFloat32x4Reg(data->F4_2);
        DumpIntReg(data->I3);
        DumpIntReg(data->I4);
        DumpIntReg(data->I5);
        DumpIntReg(data->I6);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_1Float1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloatReg(data->F1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_2Float1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat32x4Reg(data->F4_1);
        DumpFloatReg(data->F2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_1Float64x2_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat64x2Reg(data->D2_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_1Int32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpInt32x4Reg(data->I4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_1Int32x4_1Float32x4_2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpInt32x4Reg(data->I4_1);
        DumpFloat32x4Reg(data->F4_2);
        DumpFloat32x4Reg(data->F4_3);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpReg1Float32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpFloat32x4Reg(data->F4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Float32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpFloat32x4Reg(data->F4_1);
    }

    // Int32x4
    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpInt32x4Reg(data->I4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_3(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpInt32x4Reg(data->I4_1);
        DumpInt32x4Reg(data->I4_2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpInt32x4Reg(data->I4_1);
        DumpInt32x4Reg(data->I4_2);
        DumpInt32x4Reg(data->I4_3);
    }


    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_1Int4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpIntReg(data->I1);
        DumpIntReg(data->I2);
        DumpIntReg(data->I3);
        DumpIntReg(data->I4);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_2Int4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpInt32x4Reg(data->I4_1);
        DumpIntReg(data->I2);
        DumpIntReg(data->I3);
        DumpIntReg(data->I4);
        DumpIntReg(data->I5);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_2Int2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpInt32x4Reg(data->I4_1);
        DumpIntReg(data->I2);
        DumpIntReg(data->I3);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Int32x4_1Int1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpInt32x4Reg(data->I4_1);
        DumpIntReg(data->I2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat32x4_2Int1Float1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat32x4Reg(data->F4_0);
        DumpFloat32x4Reg(data->F4_1);
        DumpIntReg(data->I2);
        DumpFloatReg(data->F3);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat1Float32x4_1Int1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloatReg(data->F0);
        DumpInt32x4Reg(data->F4_1);
        DumpIntReg(data->I2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_3Int4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpInt32x4Reg(data->I4_1);
        DumpInt32x4Reg(data->I4_2);
        DumpIntReg(data->I3);
        DumpIntReg(data->I4);
        DumpIntReg(data->I5);
        DumpIntReg(data->I6);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_1Int1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpIntReg(data->I1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_2Int1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpInt32x4Reg(data->I4_1);
        DumpIntReg(data->I2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpReg1Int32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpInt32x4Reg(data->I4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_1Float32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpFloat32x4Reg(data->F4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt32x4_1Float64x2_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpInt32x4Reg(data->I4_0);
        DumpFloat64x2Reg(data->D2_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Int32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpFloat64x2Reg(data->I4_1);
    }

    // Float64x2
    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpFloat64x2Reg(data->D2_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_3(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpFloat64x2Reg(data->D2_1);
        DumpFloat64x2Reg(data->D2_2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_4(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpFloat64x2Reg(data->D2_1);
        DumpFloat64x2Reg(data->D2_2);
        DumpFloat64x2Reg(data->D2_3);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_1Double2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpDoubleReg(data->D1);
        DumpDoubleReg(data->D2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_1Double1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpDoubleReg(data->D1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_2Double1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpFloat64x2Reg(data->D2_1);
        DumpDoubleReg(data->D2);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_2Int2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpFloat64x2Reg(data->D2_1);
        DumpIntReg(data->I2);
        DumpIntReg(data->I3);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_3Int2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpFloat64x2Reg(data->D2_1);
        DumpFloat64x2Reg(data->D2_2);
        DumpIntReg(data->I3);
        DumpIntReg(data->I4);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_1Float32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpFloat32x4Reg(data->F4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_1Int32x4_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpInt32x4Reg(data->I4_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpFloat64x2_1Int32x4_1Float64x2_2(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpFloat64x2Reg(data->D2_0);
        DumpInt32x4Reg(data->I4_1);
        DumpFloat64x2Reg(data->D2_2);
        DumpFloat64x2Reg(data->D2_3);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpReg1Float64x2_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpFloat64x2Reg(data->D2_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpInt1Float64x2_1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpIntReg(data->I0);
        DumpFloat64x2Reg(data->D2_1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmSimdTypedArr(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        char16* heapTag = nullptr;

        switch (data->ViewType)
        {
        case ArrayBufferView::TYPE_INT8:
            heapTag = _u("HEAP8"); break;
        case ArrayBufferView::TYPE_UINT8:
            heapTag = _u("HEAPU8"); break;
        case ArrayBufferView::TYPE_INT16:
            heapTag = _u("HEAP16"); break;
        case ArrayBufferView::TYPE_UINT16:
            heapTag = _u("HEAPU16"); break;
        case ArrayBufferView::TYPE_INT32:
            heapTag = _u("HEAP32"); break;
        case ArrayBufferView::TYPE_UINT32:
            heapTag = _u("HEAPU32"); break;
        case ArrayBufferView::TYPE_FLOAT32:
            heapTag = _u("HEAPF32"); break;
        case ArrayBufferView::TYPE_FLOAT64:
            heapTag = _u("HEAPF64"); break;
        default:
            Assert(false);
            __assume(false);
            break;
        }

        switch (op)
        {
        case OpCodeAsmJs::Simd128_LdArrConst_I4:
        case OpCodeAsmJs::Simd128_LdArrConst_F4:
        case OpCodeAsmJs::Simd128_LdArrConst_D2:
        case OpCodeAsmJs::Simd128_StArrConst_I4:
        case OpCodeAsmJs::Simd128_StArrConst_F4:
        case OpCodeAsmJs::Simd128_StArrConst_D2:
            Output::Print(_u(" %s[%d] "), heapTag, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdArr_I4:
        case OpCodeAsmJs::Simd128_LdArr_F4:
        case OpCodeAsmJs::Simd128_LdArr_D2:
        case OpCodeAsmJs::Simd128_StArr_I4:
        case OpCodeAsmJs::Simd128_StArr_F4:
        case OpCodeAsmJs::Simd128_StArr_D2:
            Output::Print(_u(" %s[I%d] "), heapTag, data->SlotIndex);
            break;
        default:
            Assert(false);
            __assume(false);
            break;
        }

        switch (op)
        {
        case OpCodeAsmJs::Simd128_LdArr_I4:
        case OpCodeAsmJs::Simd128_LdArrConst_I4:
        case OpCodeAsmJs::Simd128_StArr_I4:
        case OpCodeAsmJs::Simd128_StArrConst_I4:
            DumpInt32x4Reg(data->Value);
            break;
        case OpCodeAsmJs::Simd128_LdArr_F4:
        case OpCodeAsmJs::Simd128_LdArrConst_F4:
        case OpCodeAsmJs::Simd128_StArr_F4:
        case OpCodeAsmJs::Simd128_StArrConst_F4:
            DumpFloat32x4Reg(data->Value);
            break;
        case OpCodeAsmJs::Simd128_LdArr_D2:
        case OpCodeAsmJs::Simd128_LdArrConst_D2:
        case OpCodeAsmJs::Simd128_StArr_D2:
        case OpCodeAsmJs::Simd128_StArrConst_D2:
            DumpFloat64x2Reg(data->Value);
            break;
        default:
            Assert(false);
            __assume(false);
            break;
        }

        // data width
        Output::Print(_u(" %d bytes "), data->DataWidth);
    }
}

#endif
#endif
