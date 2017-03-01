//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeByteCodePch.h"

#ifdef ASMJS_PLAT
#if DBG_DUMP
#include "Language/AsmJsModule.h"
#include "ByteCode/AsmJsByteCodeDumper.h"

namespace Js
{
    void AsmJsByteCodeDumper::Dump(FunctionBody* body, const WAsmJs::TypedRegisterAllocator* typedRegister, AsmJsFunc* asmFunc)
    {
        ByteCodeReader reader;
        reader.Create(body);
        StatementReader<FunctionBody::StatementMapList> statementReader;
        statementReader.Create(body);
        body->DumpFullFunctionName();
        Output::Print(_u(" Asm.js ("));
        AsmJsFunctionInfo* funcInfo = body->GetAsmJsFunctionInfo();
        const ArgSlot argCount = funcInfo->GetArgCount();
        for (ArgSlot i = 0; i < argCount; i++)
        {
            AsmJsVarType var = funcInfo->GetArgType(i);
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
            else if (var.isInt64())
            {
                Output::Print(_u("int64(In%hu)"), i);
            }
            else if (var.isSIMD())
            {
                switch (var.which())
                {
                case AsmJsType::Int32x4:
                    Output::Print(_u("I4(In%hu)"), i);
                    break;
                case AsmJsType::Int8x16:
                    Output::Print(_u("I16(In%hu)"), i);
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

        if (!typedRegister && asmFunc)
        {
            typedRegister = &asmFunc->GetTypedRegisterAllocator();
        }

        if (typedRegister)
        {
            typedRegister->DumpLocalsInfo();
        }

        if (asmFunc)
        {
            DumpConstants(asmFunc, body);
        }

        if (typedRegister)
        {
            Output::Print(_u("    Implicit Arg Ins:\n    ======== =====\n    "));
            uint32 iArgs[WAsmJs::LIMIT];
            typedRegister->GetArgumentStartIndex(iArgs);
            uint32 iArg = iArgs[WAsmJs::INT32];
            uint32 lArg = iArgs[WAsmJs::INT64];
            uint32 dArg = iArgs[WAsmJs::FLOAT64];
            uint32 fArg = iArgs[WAsmJs::FLOAT32];
            uint32 simdArg = iArgs[WAsmJs::SIMD];
            for (ArgSlot i = 0; i < argCount; i++)
            {
                AsmJsVarType var = funcInfo->GetArgType(i);
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
                else if (var.isInt64())
                {
                    Output::Print(_u(" L%d  In%d"), lArg++, i);
                }
                else if (var.isSIMD())
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
        }

        if (funcInfo->GetReturnType() == AsmJsRetType::Void)
        {
            Output::Print(_u("    0000   %-20s R0\n"), OpCodeUtilAsmJs::GetOpCodeName(OpCodeAsmJs::LdUndef));
        }

        uint32 statementIndex = 0;
        while (true)
        {
            while (statementReader.AtStatementBoundary(&reader))
            {
                body->PrintStatementSourceLine(statementIndex);
                statementIndex = statementReader.MoveNextStatementBoundary();
            }
            int byteOffset = reader.GetCurrentOffset();
            LayoutSize layoutSize;
            OpCodeAsmJs op = reader.ReadAsmJsOp(layoutSize);
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

    template<typename T, typename Func>
    void PrintTypedConstants(byte* table, WAsmJs::Types type, uint nConsts, Func printValFunc)
    {
        T* constTable = (T*)table;
        if (nConsts > 0)
        {
            char16 buf[32];
            WAsmJs::RegisterSpace::GetTypeDebugName(type, buf, 32);
            Output::Print(_u("    Constant %s:\n    ======== =======\n    "), buf);
            WAsmJs::RegisterSpace::GetTypeDebugName(type, buf, 32, true);
            for (uint i = 0; i < nConsts; i++)
            {
                Output::Print(_u(" %s%d  "), buf, i);
                printValFunc(*constTable);
                Output::Print(_u("\n    "));
                ++constTable;
            }
        }
    }

    void AsmJsByteCodeDumper::DumpConstants(AsmJsFunc* func, FunctionBody* body)
    {
        byte* table = (byte*)((Var*)body->GetConstTable());
        auto constSrcInfos = func->GetTypedRegisterAllocator().GetConstSourceInfos();
        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            WAsmJs::Types type = (WAsmJs::Types)i;
            if (func->GetTypedRegisterAllocator().IsTypeExcluded(type))
            {
                continue;
            }
            uint constCount = func->GetTypedRegisterAllocator().GetRegisterSpace(type)->GetConstCount();
            if (constCount > 0)
            {
                uint offset = constSrcInfos.srcByteOffsets[i];
                byte* tableOffseted = table + offset;
                switch (type)
                {
                case WAsmJs::INT32:   PrintTypedConstants<int>(tableOffseted, type, constCount, [](int v) {Output::Print(_u("%d"), v);}); break;
                case WAsmJs::INT64:   PrintTypedConstants<int64>(tableOffseted, type, constCount, [](int64 v) {Output::Print(_u("%lld"), v);}); break;
                case WAsmJs::FLOAT32: PrintTypedConstants<float>(tableOffseted, type, constCount, [](float v) {Output::Print(_u("%.4f"), v);}); break;
                case WAsmJs::FLOAT64: PrintTypedConstants<double>(tableOffseted, type, constCount, [](double v) {Output::Print(_u("%.4f"), v);}); break;
                case WAsmJs::SIMD:    PrintTypedConstants<AsmJsSIMDValue>(tableOffseted, type, constCount, [](AsmJsSIMDValue v) {
                    Output::Print(_u("\n       I32(%d, %d, %d, %d),"), v.i32[SIMD_X], v.i32[SIMD_Y], v.i32[SIMD_Z], v.i32[SIMD_W]);
                    Output::Print(_u("\n       F32(%.4f, %.4f, %.4f, %.4f),"), v.f32[SIMD_X], v.f32[SIMD_Y], v.f32[SIMD_Z], v.f32[SIMD_W]);
                    Output::Print(_u("\n       D64(%.4f, %.4f),"), v.f64[SIMD_X], v.f64[SIMD_Y]);
                    Output::Print(_u("\n       I8(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)"),
                                  v.i8[0], v.i8[1], v.i8[2], v.i8[3], v.i8[4], v.i8[5], v.i8[6], v.i8[7],
                                  v.i8[8], v.i8[9], v.i8[10], v.i8[11], v.i8[12], v.i8[13], v.i8[14], v.i8[15]);
                    });
                    break;
                default:
                    Assert(false);
                    break;
                }
            }
            Output::Print(_u("\n"));
        }
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

    void AsmJsByteCodeDumper::DumpLongReg(RegSlot reg)
    {
        Output::Print(_u(" L%d "), (int)reg);
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
        Output::Print(_u(" F4_%d "), (int)reg);
    }

    // Int32x4
    void AsmJsByteCodeDumper::DumpInt32x4Reg(RegSlot reg)
    {
        Output::Print(_u(" I4_%d "), (int)reg);
    }

    void AsmJsByteCodeDumper::DumpUint32x4Reg(RegSlot reg)
    {
        Output::Print(_u(" U4_%d "), (int)reg);
    }

    void AsmJsByteCodeDumper::DumpInt16x8Reg(RegSlot reg)
    {
        Output::Print(_u(" I8_%d "), (int)reg);
    }

    // Int8x16
    void AsmJsByteCodeDumper::DumpInt8x16Reg(RegSlot reg)
    {
        Output::Print(_u(" I16_%d "), (int)reg);
    }

    void AsmJsByteCodeDumper::DumpUint16x8Reg(RegSlot reg)
    {
        Output::Print(_u(" U8_%d "), (int)reg);
    }

    void AsmJsByteCodeDumper::DumpUint8x16Reg(RegSlot reg)
    {
        Output::Print(_u(" U16_%d "), (int)reg);
    }
    // Bool32x4
    void AsmJsByteCodeDumper::DumpBool32x4Reg(RegSlot reg)
    {
        Output::Print(_u(" B4_%d "), (int)reg);
    }

    // Bool16x8
    void AsmJsByteCodeDumper::DumpBool16x8Reg(RegSlot reg)
    {
        Output::Print(_u(" B8_%d "), (int)reg);
    }

    // Bool32x4
    void AsmJsByteCodeDumper::DumpBool8x16Reg(RegSlot reg)
    {
        Output::Print(_u(" B16_%d "), (int)reg);
    }

    // Float64x2
    void AsmJsByteCodeDumper::DumpFloat64x2Reg(RegSlot reg)
    {
        Output::Print(_u(" D2_%d "), (int)reg);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpElementSlot(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        switch (op)
        {
        case OpCodeAsmJs::LdSlot:
        case OpCodeAsmJs::LdSlotArr:
        {
#ifdef ENABLE_WASM
            auto asmInfo = dumpFunction->GetAsmJsFunctionInfo();
            auto wasmInfo = asmInfo->GetWasmReaderInfo();
            if (wasmInfo)
            {
                uint index = (uint)data->SlotIndex;
                if (index - wasmInfo->m_module->GetFuncOffset() < wasmInfo->m_module->GetWasmFunctionCount())
                {
                    uint funcIndex = data->SlotIndex - wasmInfo->m_module->GetFuncOffset();
                    auto loadedFunc = wasmInfo->m_module->GetWasmFunctionInfo(funcIndex);
                    Output::Print(_u(" R%d = %s"), data->Value, loadedFunc->GetBody()->GetDisplayName());
                    break;
                }
            }
#endif
            Output::Print(_u(" R%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        }
        case OpCodeAsmJs::LdArr_Func:
        case OpCodeAsmJs::LdArr_WasmFunc:
            Output::Print(_u(" R%d = R%d[I%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::StSlot_Int:
            Output::Print(_u(" R%d[%d] = I%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::StSlot_Long:
            Output::Print(_u(" R%d[%d] = L%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::StSlot_Flt:
            Output::Print(_u(" R%d[%d] = F%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::StSlot_Db:
            Output::Print(_u(" R%d[%d] = D%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::LdSlot_Int:
            Output::Print(_u(" I%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::LdSlot_Long:
            Output::Print(_u(" L%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::LdSlot_Flt:
            Output::Print(_u(" F%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::LdSlot_Db:
            Output::Print(_u(" D%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_F4:
            Output::Print(_u(" F4_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_I4:
            Output::Print(_u(" I4_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_I8:
            Output::Print(_u(" I8_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_I16:
            Output::Print(_u(" I16_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_U4:
            Output::Print(_u(" U4_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_U8:
            Output::Print(_u(" U8_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_U16:
            Output::Print(_u(" U16_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_B4:
            Output::Print(_u(" B4_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_B8:
            Output::Print(_u(" B8_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
        case OpCodeAsmJs::Simd128_LdSlot_B16:
            Output::Print(_u(" B16_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;
#if 0
        case OpCodeAsmJs::Simd128_LdSlot_D2:
            Output::Print(_u(" D2_%d = R%d[%d]"), data->Value, data->Instance, data->SlotIndex);
            break;

#endif // 0

        case OpCodeAsmJs::Simd128_StSlot_F4:
            Output::Print(_u(" R%d[%d]  = F4_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_I4:
            Output::Print(_u(" R%d[%d]  = I4_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_I8:
            Output::Print(_u(" R%d[%d]  = I8_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_I16:
            Output::Print(_u(" R%d[%d]  = I16_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_U4:
            Output::Print(_u(" R%d[%d]  = U4_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_U8:
            Output::Print(_u(" R%d[%d]  = U8_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_U16:
            Output::Print(_u(" R%d[%d]  = U16_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_B4:
            Output::Print(_u(" R%d[%d]  = B4_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_B8:
            Output::Print(_u(" R%d[%d]  = B8_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
        case OpCodeAsmJs::Simd128_StSlot_B16:
            Output::Print(_u(" R%d[%d]  = B16_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
#if 0
        case OpCodeAsmJs::Simd128_StSlot_D2:
            Output::Print(_u(" R%d[%d]  = D2_%d"), data->Instance, data->SlotIndex, data->Value);
            break;
#endif // 0

        default:
        {
            AssertMsg(false, "Unknown OpCode for OpLayoutElementSlot");
            break;
        }
        }
    }

    void AsmJsByteCodeDumper::InitializeWAsmJsMemTag(ArrayBufferView::ViewType type, _Out_ WAsmJsMemTag * tag)
    {
        switch (type)
        {
        case ArrayBufferView::TYPE_INT8:
            tag->heapTag = _u("HEAP8"); tag->valueTag = 'I';  break;
        case ArrayBufferView::TYPE_UINT8:
            tag->heapTag = _u("HEAPU8"); tag->valueTag = 'U'; break;
        case ArrayBufferView::TYPE_INT16:
            tag->heapTag = _u("HEAP16"); tag->valueTag = 'I'; break;
        case ArrayBufferView::TYPE_UINT16:
            tag->heapTag = _u("HEAPU16"); tag->valueTag = 'U'; break;
        case ArrayBufferView::TYPE_INT32:
            tag->heapTag = _u("HEAP32"); tag->valueTag = 'I'; break;
        case ArrayBufferView::TYPE_UINT32:
            tag->heapTag = _u("HEAPU32"); tag->valueTag = 'U'; break;
        case ArrayBufferView::TYPE_FLOAT32:
            tag->heapTag = _u("HEAPF32"); tag->valueTag = 'F'; break;
        case ArrayBufferView::TYPE_FLOAT64:
            tag->heapTag = _u("HEAPF64"); tag->valueTag = 'D'; break;
        case ArrayBufferView::TYPE_INT64:
            tag->heapTag = _u("HEAPI64"); tag->valueTag = 'L'; break;
        case ArrayBufferView::TYPE_INT8_TO_INT64:
            tag->heapTag = _u("HEAP8"); tag->valueTag = 'L'; break;
        case ArrayBufferView::TYPE_UINT8_TO_INT64:
            tag->heapTag = _u("HEAPU8"); tag->valueTag = 'L'; break;
        case ArrayBufferView::TYPE_INT16_TO_INT64:
            tag->heapTag = _u("HEAP16"); tag->valueTag = 'L'; break;
        case ArrayBufferView::TYPE_UINT16_TO_INT64:
            tag->heapTag = _u("HEAPU16"); tag->valueTag = 'L'; break;
        case ArrayBufferView::TYPE_INT32_TO_INT64:
            tag->heapTag = _u("HEAP32"); tag->valueTag = 'L'; break;
        case ArrayBufferView::TYPE_UINT32_TO_INT64:
            tag->heapTag = _u("HEAPU32"); tag->valueTag = 'L'; break;
        default:
            Assume(UNREACHED);
        }
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmTypedArr(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        WAsmJsMemTag tag;
        InitializeWAsmJsMemTag(data->ViewType, &tag);
        switch (op)
        {
        case OpCodeAsmJs::LdArr:
            Output::Print(_u(" %c%d = %s[I%d]"), tag.valueTag, data->Value, tag.heapTag, data->SlotIndex); break;
        case OpCodeAsmJs::LdArrConst:
            Output::Print(_u(" %c%d = %s[%d]"), tag.valueTag, data->Value, tag.heapTag, data->SlotIndex); break;
        case OpCodeAsmJs::StArr:
            Output::Print(_u(" %s[I%d] = %c%d"), tag.heapTag, data->SlotIndex, tag.valueTag, data->Value); break;
        case OpCodeAsmJs::StArrConst:
            Output::Print(_u(" %s[%d] = %c%d"), tag.heapTag, data->SlotIndex, tag.valueTag, data->Value); break;
        default:
            Assume(UNREACHED);
        }
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpWasmMemAccess(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        WAsmJsMemTag tag;
        InitializeWAsmJsMemTag(data->ViewType, &tag);
        switch (op)
        {
        case OpCodeAsmJs::LdArrWasm:
            Output::Print(_u(" %c%d = %s[L%d + %d]"), tag.valueTag, data->Value, tag.heapTag, data->SlotIndex, data->Offset); break;
        case OpCodeAsmJs::StArrWasm:
            Output::Print(_u(" %s[L%d + %d] = %c%d"), tag.heapTag, data->SlotIndex, data->Offset, tag.valueTag, data->Value); break;
        default:
            Assume(UNREACHED);
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
    void AsmJsByteCodeDumper::DumpAsmReg9(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
        DumpReg(data->R6);
        DumpReg(data->R7);
        DumpReg(data->R8);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg10(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
        DumpReg(data->R6);
        DumpReg(data->R7);
        DumpReg(data->R8);
        DumpReg(data->R9);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg11(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
        DumpReg(data->R6);
        DumpReg(data->R7);
        DumpReg(data->R8);
        DumpReg(data->R9);
        DumpReg(data->R10);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg17(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
        DumpReg(data->R6);
        DumpReg(data->R7);
        DumpReg(data->R8);
        DumpReg(data->R9);
        DumpReg(data->R10);
        DumpReg(data->R11);
        DumpReg(data->R12);
        DumpReg(data->R13);
        DumpReg(data->R14);
        DumpReg(data->R15);
        DumpReg(data->R16);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg18(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
        DumpReg(data->R6);
        DumpReg(data->R7);
        DumpReg(data->R8);
        DumpReg(data->R9);
        DumpReg(data->R10);
        DumpReg(data->R11);
        DumpReg(data->R12);
        DumpReg(data->R13);
        DumpReg(data->R14);
        DumpReg(data->R15);
        DumpReg(data->R16);
        DumpReg(data->R17);
    }
    template <class T>
    void AsmJsByteCodeDumper::DumpAsmReg19(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpReg(data->R0);
        DumpReg(data->R1);
        DumpReg(data->R2);
        DumpReg(data->R3);
        DumpReg(data->R4);
        DumpReg(data->R5);
        DumpReg(data->R6);
        DumpReg(data->R7);
        DumpReg(data->R8);
        DumpReg(data->R9);
        DumpReg(data->R10);
        DumpReg(data->R11);
        DumpReg(data->R12);
        DumpReg(data->R13);
        DumpReg(data->R14);
        DumpReg(data->R15);
        DumpReg(data->R16);
        DumpReg(data->R17);
        DumpReg(data->R18);
    }
#define LAYOUT_TYPE_WMS_REG2(layout, t0, t1) \
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
    }
#define LAYOUT_TYPE_WMS_REG3(layout, t0, t1, t2) \
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
    }
#define LAYOUT_TYPE_WMS_REG4(layout, t0, t1, t2, t3)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
    };
#define LAYOUT_TYPE_WMS_REG5(layout, t0, t1, t2, t3, t4)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
    };
#define LAYOUT_TYPE_WMS_REG6(layout, t0, t1, t2, t3, t4, t5)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
    };
#define LAYOUT_TYPE_WMS_REG7(layout, t0, t1, t2, t3, t4, t5, t6)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
        Dump##t6##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6));\
    };
#define LAYOUT_TYPE_WMS_REG9(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
        Dump##t6##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6));\
        Dump##t7##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7));\
        Dump##t8##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8));\
    };
#define LAYOUT_TYPE_WMS_REG10(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
        Dump##t6##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6));\
        Dump##t7##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7));\
        Dump##t8##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8));\
        Dump##t9##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9));\
    };
#define LAYOUT_TYPE_WMS_REG11(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
        Dump##t6##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6));\
        Dump##t7##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7));\
        Dump##t8##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8));\
        Dump##t9##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9));\
        Dump##t10##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10));\
    };
#define LAYOUT_TYPE_WMS_REG17(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
        Dump##t6##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6));\
        Dump##t7##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7));\
        Dump##t8##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8));\
        Dump##t9##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9));\
        Dump##t10##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10));\
        Dump##t11##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t11(), 11));\
        Dump##t12##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t12(), 12));\
        Dump##t13##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t13(), 13));\
        Dump##t14##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t14(), 14));\
        Dump##t15##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t15(), 15));\
        Dump##t16##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t16(), 16));\
    };
#define LAYOUT_TYPE_WMS_REG18(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
        Dump##t6##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6));\
        Dump##t7##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7));\
        Dump##t8##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8));\
        Dump##t9##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9));\
        Dump##t10##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10));\
        Dump##t11##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t11(), 11));\
        Dump##t12##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t12(), 12));\
        Dump##t13##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t13(), 13));\
        Dump##t14##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t14(), 14));\
        Dump##t15##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t15(), 15));\
        Dump##t16##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t16(), 16));\
        Dump##t17##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t17(), 17));\
    };
#define LAYOUT_TYPE_WMS_REG19(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18)\
    template <class T> void AsmJsByteCodeDumper::Dump##layout(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)\
    {\
        Dump##t0##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t0(), 0));\
        Dump##t1##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t1(), 1));\
        Dump##t2##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t2(), 2));\
        Dump##t3##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t3(), 3));\
        Dump##t4##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t4(), 4));\
        Dump##t5##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t5(), 5));\
        Dump##t6##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t6(), 6));\
        Dump##t7##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t7(), 7));\
        Dump##t8##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t8(), 8));\
        Dump##t9##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t9(), 9));\
        Dump##t10##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t10(), 10));\
        Dump##t11##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t11(), 11));\
        Dump##t12##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t12(), 12));\
        Dump##t13##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t13(), 13));\
        Dump##t14##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t14(), 14));\
        Dump##t15##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t15(), 15));\
        Dump##t16##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t16(), 16));\
        Dump##t17##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t17(), 17));\
        Dump##t18##Reg(data->LAYOUT_FIELDS_DEF(LAYOUT_PREFIX_##t18(), 18));\
    };

#include "LayoutTypesAsmJs.h"


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

    template <class T>
    void AsmJsByteCodeDumper::DumpBrInt1Const1(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        DumpOffset(data->RelativeJumpOffset, reader);
        DumpIntReg(data->I1);
        DumpI4(data->C1);
    }

    template <class T>
    void AsmJsByteCodeDumper::DumpAsmSimdTypedArr(OpCodeAsmJs op, const unaligned T * data, FunctionBody * dumpFunction, ByteCodeReader& reader)
    {
        const char16* heapTag = nullptr;

        switch (data->ViewType)
        {
        case ArrayBufferView::TYPE_INT8:
        case ArrayBufferView::TYPE_INT8_TO_INT64:
            heapTag = _u("HEAP8"); break;
        case ArrayBufferView::TYPE_UINT8:
        case ArrayBufferView::TYPE_UINT8_TO_INT64:
            heapTag = _u("HEAPU8"); break;
        case ArrayBufferView::TYPE_INT16:
        case ArrayBufferView::TYPE_INT16_TO_INT64:
            heapTag = _u("HEAP16"); break;
        case ArrayBufferView::TYPE_UINT16:
        case ArrayBufferView::TYPE_UINT16_TO_INT64:
            heapTag = _u("HEAPU16"); break;
        case ArrayBufferView::TYPE_INT32:
        case ArrayBufferView::TYPE_INT32_TO_INT64:
            heapTag = _u("HEAP32"); break;
        case ArrayBufferView::TYPE_UINT32:
        case ArrayBufferView::TYPE_UINT32_TO_INT64:
            heapTag = _u("HEAPU32"); break;
        case ArrayBufferView::TYPE_FLOAT32:
            heapTag = _u("HEAPF32"); break;
        case ArrayBufferView::TYPE_FLOAT64:
            heapTag = _u("HEAPF64"); break;
        case ArrayBufferView::TYPE_INT64:
            heapTag = _u("HEAPI64"); break;
        default:
            Assert(false);
            __assume(false);
            break;
        }

#define SIMD_DUMP_ARR_I4 DumpInt32x4Reg
#define SIMD_DUMP_ARR_I8 DumpInt16x8Reg
#define SIMD_DUMP_ARR_I16 DumpInt8x16Reg
#define SIMD_DUMP_ARR_U4 DumpUint32x4Reg
#define SIMD_DUMP_ARR_U8 DumpUint16x8Reg
#define SIMD_DUMP_ARR_U16 DumpUint8x16Reg
#define SIMD_DUMP_ARR_F4 DumpFloat32x4Reg
#define SIMD_DUMP_ARR_D2 DumpFloat64x2Reg
#define SIMD_DUMP_REG(type) SIMD_DUMP_ARR_##type(data->Value)
#define SIMD_DUMP_ARR_VALUE(type) \
        case OpCodeAsmJs::Simd128_LdArr_##type:\
            SIMD_DUMP_REG(type); Output::Print(_u("= %s[I%d] "), heapTag, data->SlotIndex); break;\
        case OpCodeAsmJs::Simd128_LdArrConst_##type:\
            SIMD_DUMP_REG(type); Output::Print(_u("= %s[%d] "), heapTag, data->SlotIndex); break;\
        case OpCodeAsmJs::Simd128_StArr_##type:\
            Output::Print(_u("%s[I%d] = "), heapTag, data->SlotIndex); SIMD_DUMP_REG(type); break;\
        case OpCodeAsmJs::Simd128_StArrConst_##type:\
            Output::Print(_u("%s[%d] = "), heapTag, data->SlotIndex); SIMD_DUMP_REG(type); break;\

        switch (op)
        {
            SIMD_DUMP_ARR_VALUE(I4)
            SIMD_DUMP_ARR_VALUE(I8)
            SIMD_DUMP_ARR_VALUE(I16)
            SIMD_DUMP_ARR_VALUE(U4)
            SIMD_DUMP_ARR_VALUE(U8)
            SIMD_DUMP_ARR_VALUE(U16)
            SIMD_DUMP_ARR_VALUE(F4)
            //SIMD_DUMP_ARR_VALUE(D2)
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
