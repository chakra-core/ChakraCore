//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#ifdef ENABLE_SIMDJS

#define GET_SIMDOPCODE(irOpcode) m_simd128OpCodesMap[(uint32)(irOpcode - Js::OpCode::Simd128_Start)]

#define SET_SIMDOPCODE(irOpcode, mdOpcode) \
    Assert((uint32)m_simd128OpCodesMap[(uint32)(Js::OpCode::irOpcode - Js::OpCode::Simd128_Start)] == 0);\
    Assert(Js::OpCode::mdOpcode > Js::OpCode::MDStart);\
    m_simd128OpCodesMap[(uint32)(Js::OpCode::irOpcode - Js::OpCode::Simd128_Start)] = Js::OpCode::mdOpcode;

IR::Instr* LowererMD::Simd128Instruction(IR::Instr *instr)
{
    // Currently only handles type-specialized/asm.js opcodes

    if (!instr->GetDst())
    {
        // SIMD ops always have DST in asmjs
        Assert(!instr->m_func->GetJITFunctionBody()->IsAsmJsMode());
        // unused result. Do nothing.
        IR::Instr * pInstr = instr->m_prev;
        instr->Remove();
        return pInstr;
    }

    if (Simd128TryLowerMappedInstruction(instr))
    {
        return instr->m_prev;
    }
    return Simd128LowerUnMappedInstruction(instr);
}

bool LowererMD::Simd128TryLowerMappedInstruction(IR::Instr *instr)
{
    bool legalize = true;
    Js::OpCode opcode = GET_SIMDOPCODE(instr->m_opcode);

    if ((uint32)opcode == 0)
        return false;

    Assert(instr->GetDst() && instr->GetDst()->IsRegOpnd() && instr->GetDst()->IsSimd128() || instr->GetDst()->GetType() == TyInt32);
    Assert(instr->GetSrc1() && instr->GetSrc1()->IsRegOpnd() && instr->GetSrc1()->IsSimd128());
    Assert(!instr->GetSrc2() || (((instr->GetSrc2()->IsRegOpnd() && instr->GetSrc2()->IsSimd128()) || (instr->GetSrc2()->IsIntConstOpnd() && instr->GetSrc2()->GetType() == TyInt8))));

    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_Abs_F4:
        Assert(opcode == Js::OpCode::ANDPS);
        instr->SetSrc2(IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AbsMaskF4Addr(), instr->GetSrc1()->GetType(), m_func));
        break;
#if 0
    case Js::OpCode::Simd128_Abs_D2:
        Assert(opcode == Js::OpCode::ANDPD);
        instr->SetSrc2(IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AbsMaskD2Addr(), instr->GetSrc1()->GetType(), m_func));
        break;
#endif // 0

    case Js::OpCode::Simd128_Neg_F4:
        Assert(opcode == Js::OpCode::XORPS);
        instr->SetSrc2(IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86NegMaskF4Addr(), instr->GetSrc1()->GetType(), m_func));
        break;
#if 0
    case Js::OpCode::Simd128_Neg_D2:
        Assert(opcode == Js::OpCode::XORPS);
        instr->SetSrc2(IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86NegMaskD2Addr(), instr->GetSrc1()->GetType(), m_func));
        break;
#endif // 0

    case Js::OpCode::Simd128_Not_I4:
    case Js::OpCode::Simd128_Not_I16:
    case Js::OpCode::Simd128_Not_I8:
    case Js::OpCode::Simd128_Not_U4:
    case Js::OpCode::Simd128_Not_U8:
    case Js::OpCode::Simd128_Not_U16:
    case Js::OpCode::Simd128_Not_B4:
    case Js::OpCode::Simd128_Not_B8:
    case Js::OpCode::Simd128_Not_B16:
        Assert(opcode == Js::OpCode::XORPS);
        instr->SetSrc2(IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), instr->GetSrc1()->GetType(), m_func));
        break;
    case Js::OpCode::Simd128_Gt_F4:
    //case Js::OpCode::Simd128_Gt_D2:
    case Js::OpCode::Simd128_GtEq_F4:
    //case Js::OpCode::Simd128_GtEq_D2:
    case Js::OpCode::Simd128_Lt_I4:
    case Js::OpCode::Simd128_Lt_I8:
    case Js::OpCode::Simd128_Lt_I16:
    {
        Assert(opcode == Js::OpCode::CMPLTPS || opcode == Js::OpCode::CMPLTPD || opcode == Js::OpCode::CMPLEPS
            || opcode == Js::OpCode::CMPLEPD || opcode == Js::OpCode::PCMPGTD || opcode == Js::OpCode::PCMPGTB
            || opcode == Js::OpCode::PCMPGTW );
        // swap operands
        auto *src1 = instr->UnlinkSrc1();
        auto *src2 = instr->UnlinkSrc2();
        instr->SetSrc1(src2);
        instr->SetSrc2(src1);
        break;
    }

    }
    instr->m_opcode = opcode;
    if (legalize)
    {
        //MakeDstEquSrc1(instr);
        Legalize(instr);
    }

    return true;
}

IR::Instr* LowererMD::Simd128LowerUnMappedInstruction(IR::Instr *instr)
{
    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_LdC:
        return Simd128LoadConst(instr);

    case Js::OpCode::Simd128_FloatsToF4:
    case Js::OpCode::Simd128_IntsToI4:
    case Js::OpCode::Simd128_IntsToU4:
    case Js::OpCode::Simd128_IntsToB4:
        return Simd128LowerConstructor_4(instr);
    case Js::OpCode::Simd128_IntsToI8:
    case Js::OpCode::Simd128_IntsToU8:
    case Js::OpCode::Simd128_IntsToB8:
        return Simd128LowerConstructor_8(instr);
    case Js::OpCode::Simd128_IntsToI16:
    case Js::OpCode::Simd128_IntsToU16:
    case Js::OpCode::Simd128_IntsToB16:
        return Simd128LowerConstructor_16(instr);

#if 0
    case Js::OpCode::Simd128_DoublesToD2:
        return Simd128LowerConstructor_2(instr);
#endif // 0

    case Js::OpCode::Simd128_ExtractLane_I4:
    case Js::OpCode::Simd128_ExtractLane_I8:
    case Js::OpCode::Simd128_ExtractLane_I16:
    case Js::OpCode::Simd128_ExtractLane_U4:
    case Js::OpCode::Simd128_ExtractLane_U8:
    case Js::OpCode::Simd128_ExtractLane_U16:
    case Js::OpCode::Simd128_ExtractLane_B4:
    case Js::OpCode::Simd128_ExtractLane_B8:
    case Js::OpCode::Simd128_ExtractLane_B16:
    case Js::OpCode::Simd128_ExtractLane_F4:
        return Simd128LowerLdLane(instr);

    case Js::OpCode::Simd128_ReplaceLane_I4:
    case Js::OpCode::Simd128_ReplaceLane_F4:
    case Js::OpCode::Simd128_ReplaceLane_U4:
    case Js::OpCode::Simd128_ReplaceLane_B4:
        return SIMD128LowerReplaceLane_4(instr);

    case Js::OpCode::Simd128_ReplaceLane_I8:
    case Js::OpCode::Simd128_ReplaceLane_U8:
    case Js::OpCode::Simd128_ReplaceLane_B8:
        return SIMD128LowerReplaceLane_8(instr);

    case Js::OpCode::Simd128_ReplaceLane_I16:
    case Js::OpCode::Simd128_ReplaceLane_U16:
    case Js::OpCode::Simd128_ReplaceLane_B16:
        return SIMD128LowerReplaceLane_16(instr);

    case Js::OpCode::Simd128_Splat_F4:
    case Js::OpCode::Simd128_Splat_I4:
    //case Js::OpCode::Simd128_Splat_D2:
    case Js::OpCode::Simd128_Splat_I8:
    case Js::OpCode::Simd128_Splat_I16:
    case Js::OpCode::Simd128_Splat_U4:
    case Js::OpCode::Simd128_Splat_U8:
    case Js::OpCode::Simd128_Splat_U16:
    case Js::OpCode::Simd128_Splat_B4:
    case Js::OpCode::Simd128_Splat_B8:
    case Js::OpCode::Simd128_Splat_B16:
        return Simd128LowerSplat(instr);

    case Js::OpCode::Simd128_Rcp_F4:
    //case Js::OpCode::Simd128_Rcp_D2:
        return Simd128LowerRcp(instr);

    case Js::OpCode::Simd128_Sqrt_F4:
    //case Js::OpCode::Simd128_Sqrt_D2:
        return Simd128LowerSqrt(instr);

    case Js::OpCode::Simd128_RcpSqrt_F4:
    //case Js::OpCode::Simd128_RcpSqrt_D2:
        return Simd128LowerRcpSqrt(instr);

    case Js::OpCode::Simd128_Select_F4:
    case Js::OpCode::Simd128_Select_I4:
    //case Js::OpCode::Simd128_Select_D2:
    case Js::OpCode::Simd128_Select_I8:
    case Js::OpCode::Simd128_Select_I16:
    case Js::OpCode::Simd128_Select_U4:
    case Js::OpCode::Simd128_Select_U8:
    case Js::OpCode::Simd128_Select_U16:
        return Simd128LowerSelect(instr);

    case Js::OpCode::Simd128_Neg_I4:
    case Js::OpCode::Simd128_Neg_I8:
    case Js::OpCode::Simd128_Neg_I16:
    case Js::OpCode::Simd128_Neg_U4:
    case Js::OpCode::Simd128_Neg_U8:
    case Js::OpCode::Simd128_Neg_U16:
        return Simd128LowerNeg(instr);

    case Js::OpCode::Simd128_Mul_I4:
    case Js::OpCode::Simd128_Mul_U4:
        return Simd128LowerMulI4(instr);
    case Js::OpCode::Simd128_Mul_I16:
    case Js::OpCode::Simd128_Mul_U16:
        return Simd128LowerMulI16(instr);

    case Js::OpCode::Simd128_ShRtByScalar_I4:
    case Js::OpCode::Simd128_ShLtByScalar_I4:
    case Js::OpCode::Simd128_ShRtByScalar_I8:
    case Js::OpCode::Simd128_ShLtByScalar_I8:
    case Js::OpCode::Simd128_ShLtByScalar_I16:
    case Js::OpCode::Simd128_ShRtByScalar_I16:
    case Js::OpCode::Simd128_ShRtByScalar_U4:
    case Js::OpCode::Simd128_ShLtByScalar_U4:
    case Js::OpCode::Simd128_ShRtByScalar_U8:
    case Js::OpCode::Simd128_ShLtByScalar_U8:
    case Js::OpCode::Simd128_ShRtByScalar_U16:
    case Js::OpCode::Simd128_ShLtByScalar_U16:
        return Simd128LowerShift(instr);

    case Js::OpCode::Simd128_LdArr_I4:
    case Js::OpCode::Simd128_LdArr_I8:
    case Js::OpCode::Simd128_LdArr_I16:
    case Js::OpCode::Simd128_LdArr_U4:
    case Js::OpCode::Simd128_LdArr_U8:
    case Js::OpCode::Simd128_LdArr_U16:
    case Js::OpCode::Simd128_LdArr_F4:
    //case Js::OpCode::Simd128_LdArr_D2:
    case Js::OpCode::Simd128_LdArrConst_I4:
    case Js::OpCode::Simd128_LdArrConst_I8:
    case Js::OpCode::Simd128_LdArrConst_I16:
    case Js::OpCode::Simd128_LdArrConst_U4:
    case Js::OpCode::Simd128_LdArrConst_U8:
    case Js::OpCode::Simd128_LdArrConst_U16:
    case Js::OpCode::Simd128_LdArrConst_F4:
    //case Js::OpCode::Simd128_LdArrConst_D2:
        if (m_func->GetJITFunctionBody()->IsAsmJsMode())
        {
            // with bound checks
            return Simd128AsmJsLowerLoadElem(instr);
        }
        else
        {
            // non-AsmJs, boundChecks are extracted from instr
            return Simd128LowerLoadElem(instr);
        }

    case Js::OpCode::Simd128_StArr_I4:
    case Js::OpCode::Simd128_StArr_I8:
    case Js::OpCode::Simd128_StArr_I16:
    case Js::OpCode::Simd128_StArr_U4:
    case Js::OpCode::Simd128_StArr_U8:
    case Js::OpCode::Simd128_StArr_U16:
    case Js::OpCode::Simd128_StArr_F4:
    //case Js::OpCode::Simd128_StArr_D2:
    case Js::OpCode::Simd128_StArrConst_I4:
    case Js::OpCode::Simd128_StArrConst_I8:
    case Js::OpCode::Simd128_StArrConst_I16:
    case Js::OpCode::Simd128_StArrConst_U4:
    case Js::OpCode::Simd128_StArrConst_U8:
    case Js::OpCode::Simd128_StArrConst_U16:
    case Js::OpCode::Simd128_StArrConst_F4:
    //case Js::OpCode::Simd128_StArrConst_D2:
        if (m_func->GetJITFunctionBody()->IsAsmJsMode())
        {
            return Simd128AsmJsLowerStoreElem(instr);
        }
        else
        {
            return Simd128LowerStoreElem(instr);
        }

    case Js::OpCode::Simd128_Swizzle_U4:
    case Js::OpCode::Simd128_Swizzle_I4:
    case Js::OpCode::Simd128_Swizzle_F4:
    //case Js::OpCode::Simd128_Swizzle_D2:
        return Simd128LowerSwizzle_4(instr);

    case Js::OpCode::Simd128_Shuffle_U4:
    case Js::OpCode::Simd128_Shuffle_I4:
    case Js::OpCode::Simd128_Shuffle_F4:
    //case Js::OpCode::Simd128_Shuffle_D2:
        return Simd128LowerShuffle_4(instr);
    case Js::OpCode::Simd128_Swizzle_I8:
    case Js::OpCode::Simd128_Swizzle_I16:
    case Js::OpCode::Simd128_Swizzle_U8:
    case Js::OpCode::Simd128_Swizzle_U16:
    case Js::OpCode::Simd128_Shuffle_I8:
    case Js::OpCode::Simd128_Shuffle_I16:
    case Js::OpCode::Simd128_Shuffle_U8:
    case Js::OpCode::Simd128_Shuffle_U16:
        return Simd128LowerShuffle(instr);

    case Js::OpCode::Simd128_FromUint32x4_F4:
        return Simd128LowerFloat32x4FromUint32x4(instr);

    case Js::OpCode::Simd128_FromFloat32x4_I4:
        return Simd128LowerInt32x4FromFloat32x4(instr);

    case Js::OpCode::Simd128_FromFloat32x4_U4:
        return Simd128LowerUint32x4FromFloat32x4(instr);

    case Js::OpCode::Simd128_Neq_I4:
    case Js::OpCode::Simd128_Neq_I8:
    case Js::OpCode::Simd128_Neq_I16:
    case Js::OpCode::Simd128_Neq_U4:
    case Js::OpCode::Simd128_Neq_U8:
    case Js::OpCode::Simd128_Neq_U16:
        return Simd128LowerNotEqual(instr);

    case Js::OpCode::Simd128_Lt_U4:
    case Js::OpCode::Simd128_Lt_U8:
    case Js::OpCode::Simd128_Lt_U16:
    case Js::OpCode::Simd128_GtEq_U4:
    case Js::OpCode::Simd128_GtEq_U8:
    case Js::OpCode::Simd128_GtEq_U16:
        return Simd128LowerLessThan(instr);
    case Js::OpCode::Simd128_LtEq_I4:
    case Js::OpCode::Simd128_LtEq_I8:
    case Js::OpCode::Simd128_LtEq_I16:
    case Js::OpCode::Simd128_LtEq_U4:
    case Js::OpCode::Simd128_LtEq_U8:
    case Js::OpCode::Simd128_LtEq_U16:
    case Js::OpCode::Simd128_Gt_U4:
    case Js::OpCode::Simd128_Gt_U8:
    case Js::OpCode::Simd128_Gt_U16:
        return Simd128LowerLessThanOrEqual(instr);

    case Js::OpCode::Simd128_GtEq_I4:
    case Js::OpCode::Simd128_GtEq_I8:
    case Js::OpCode::Simd128_GtEq_I16:
        return Simd128LowerGreaterThanOrEqual(instr);

    case Js::OpCode::Simd128_Min_F4:
    case Js::OpCode::Simd128_Max_F4:
        return Simd128LowerMinMax_F4(instr);

    case Js::OpCode::Simd128_AnyTrue_B4:
    case Js::OpCode::Simd128_AnyTrue_B8:
    case Js::OpCode::Simd128_AnyTrue_B16:
        return Simd128LowerAnyTrue(instr);

    case Js::OpCode::Simd128_AllTrue_B4:
    case Js::OpCode::Simd128_AllTrue_B8:
    case Js::OpCode::Simd128_AllTrue_B16:
        return Simd128LowerAllTrue(instr);

    default:
        AssertMsg(UNREACHED, "Unsupported Simd128 instruction");
    }
    return nullptr;
}

IR::Instr* LowererMD::Simd128LoadConst(IR::Instr* instr)
{
    Assert(instr->GetDst() && instr->m_opcode == Js::OpCode::Simd128_LdC);
    Assert(instr->GetDst()->IsSimd128());
    Assert(instr->GetSrc1()->IsSimd128());
    Assert(instr->GetSrc1()->IsSimd128ConstOpnd());
    Assert(instr->GetSrc2() == nullptr);

    AsmJsSIMDValue value = instr->GetSrc1()->AsSimd128ConstOpnd()->m_value;

    // MOVUPS dst, [const]
    
    void *pValue = NativeCodeDataNewNoFixup(this->m_func->GetNativeCodeDataAllocator(), SIMDType<DataDesc_LowererMD_Simd128LoadConst>, value);
    IR::Opnd * simdRef;
    if (!m_func->IsOOPJIT())
    {
        simdRef = IR::MemRefOpnd::New((void *)pValue, instr->GetDst()->GetType(), instr->m_func);
    }
    else
    {
        int offset = NativeCodeData::GetDataTotalOffset(pValue);

        simdRef = IR::IndirOpnd::New(IR::RegOpnd::New(m_func->GetTopFunc()->GetNativeCodeDataSym(), TyVar, m_func), offset, instr->GetDst()->GetType(),
#if DBG
            NativeCodeData::GetDataDescription(pValue, m_func->m_alloc),
#endif
            m_func, true);

        GetLowerer()->addToLiveOnBackEdgeSyms->Set(m_func->GetTopFunc()->GetNativeCodeDataSym()->m_id);
    }

    instr->ReplaceSrc1(simdRef);
    instr->m_opcode = LowererMDArch::GetAssignOp(instr->GetDst()->GetType());
    Legalize(instr);

    return instr->m_prev;
}

IR::Instr* LowererMD::Simd128CanonicalizeToBools(IR::Instr* instr, const Js::OpCode &cmpOpcode, IR::Opnd& dstOpnd)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_IntsToB4 || instr->m_opcode == Js::OpCode::Simd128_IntsToB8 || instr->m_opcode == Js::OpCode::Simd128_IntsToB16 ||
           instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B4 || instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B8 || instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B16);
    IR::Instr *pInstr;
    //dst = cmpOpcode dst, X86_ALL_ZEROS
    pInstr = IR::Instr::New(cmpOpcode, &dstOpnd, &dstOpnd, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllZerosAddr(), TySimd128I4, m_func), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    // dst = PANDN dst, X86_ALL_NEG_ONES
    pInstr = IR::Instr::New(Js::OpCode::PANDN, &dstOpnd, &dstOpnd, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), TySimd128I4, m_func), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    return instr;
}

IR::Instr* LowererMD::Simd128LowerConstructor_8(IR::Instr *instr)
{
    IR::Opnd* dst = nullptr;
    IR::Opnd* srcs[8];

    //Simd128_IntsToI8/U8/B8
    Assert(instr->m_opcode == Js::OpCode::Simd128_IntsToI8 || instr->m_opcode == Js::OpCode::Simd128_IntsToU8 || instr->m_opcode == Js::OpCode::Simd128_IntsToB8);
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);

    Assert(args->Count() == 9);
    dst = args->Pop();

    uint i = 0;
    while (!args->Empty() && i < 8)
    {
        srcs[i] = args->Pop();
        // src's might have been constant prop'ed. Enregister them if so.
        srcs[i] = EnregisterIntConst(instr, srcs[i], TyInt16);
        Assert(srcs[i]->GetType() == TyInt16 && srcs[i]->IsRegOpnd());
        // PINSRW dst, srcs[i], i
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PINSRW, dst, srcs[i], IR::IntConstOpnd::New(i, TyInt8, m_func, true), m_func));
        i++;
    }
    if (instr->m_opcode == Js::OpCode::Simd128_IntsToB8)
    {
        instr = Simd128CanonicalizeToBools(instr, Js::OpCode::PCMPEQW, *dst);
    }
    IR::Instr* prevInstr;
    prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

IR::Instr* LowererMD::Simd128LowerConstructor_16(IR::Instr *instr)
{
    IR::Opnd* dst = nullptr;
    IR::Opnd* srcs[16];
    //Simd128_IntsToI16/U16/B16
    Assert(instr->m_opcode == Js::OpCode::Simd128_IntsToU16 || instr->m_opcode == Js::OpCode::Simd128_IntsToI16 || instr->m_opcode == Js::OpCode::Simd128_IntsToB16);
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);
    intptr_t tempSIMD = m_func->GetThreadContextInfo()->GetSimdTempAreaAddr(0);
#if DBG
    // using only one SIMD temp
    intptr_t endAddrSIMD = tempSIMD + sizeof(X86SIMDValue);
#endif
    intptr_t address;
    IR::Instr * newInstr;

    Assert(args->Count() == 17);
    dst = args->Pop();

    uint i = 0;
    while (!args->Empty() && i < 16)
    {
        srcs[i] = args->Pop();
        // src's might have been constant prop'ed. Enregister them if so.
        srcs[i] = EnregisterIntConst(instr, srcs[i], TyInt8);
        Assert(srcs[i]->GetType() == TyInt8 && srcs[i]->IsRegOpnd());

        address = tempSIMD + i;
        // check for buffer overrun
        Assert((intptr_t)address < endAddrSIMD);
        // MOV [temp + i], src[i] (TyInt8)
        newInstr = IR::Instr::New(Js::OpCode::MOV, IR::MemRefOpnd::New(tempSIMD + i, TyInt8, m_func), srcs[i], m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        i++;
    }
    // MOVUPS dst, [temp]
    newInstr = IR::Instr::New(Js::OpCode::MOVUPS, dst, IR::MemRefOpnd::New(tempSIMD, TySimd128U16, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    if (instr->m_opcode == Js::OpCode::Simd128_IntsToB16)
    {
        instr = Simd128CanonicalizeToBools(instr, Js::OpCode::PCMPEQB, *dst);
    }

    IR::Instr* prevInstr;
    prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

IR::Instr* LowererMD::Simd128LowerConstructor_4(IR::Instr *instr)
{
    IR::Opnd* dst  = nullptr;
    IR::Opnd* src1 = nullptr;
    IR::Opnd* src2 = nullptr;
    IR::Opnd* src3 = nullptr;
    IR::Opnd* src4 = nullptr;
    IR::Instr* newInstr = nullptr;

    Assert(instr->m_opcode == Js::OpCode::Simd128_FloatsToF4 ||
           instr->m_opcode == Js::OpCode::Simd128_IntsToB4   ||
           instr->m_opcode == Js::OpCode::Simd128_IntsToI4   ||
           instr->m_opcode == Js::OpCode::Simd128_IntsToU4);

    // use MOVSS for both int32x4 and float32x4. MOVD zeroes upper bits.
    Js::OpCode movOpcode = Js::OpCode::MOVSS;
    Js::OpCode shiftOpcode = Js::OpCode::PSLLDQ;
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);

    // The number of src opnds should be exact. If opnds are missing, they should be filled in by globopt during type-spec.
    Assert(args->Count() == 5);

    dst = args->Pop();
    src1 = args->Pop();
    src2 = args->Pop();
    src3 = args->Pop();
    src4 = args->Pop();

    if (instr->m_opcode == Js::OpCode::Simd128_FloatsToF4)
    {
        // We don't have f32 type-spec, so we type-spec to f64 and convert to f32 before use.
        if (src1->IsFloat64())
        {
            IR::RegOpnd *regOpnd32 = IR::RegOpnd::New(TyFloat32, this->m_func);
            // CVTSD2SS regOpnd32.f32, src.f64    -- Convert regOpnd from f64 to f32
            newInstr = IR::Instr::New(Js::OpCode::CVTSD2SS, regOpnd32, src1, this->m_func);
            instr->InsertBefore(newInstr);
            src1 = regOpnd32;
        }
        if (src2->IsFloat64())
        {
            IR::RegOpnd *regOpnd32 = IR::RegOpnd::New(TyFloat32, this->m_func);
            // CVTSD2SS regOpnd32.f32, src.f64    -- Convert regOpnd from f64 to f32
            newInstr = IR::Instr::New(Js::OpCode::CVTSD2SS, regOpnd32, src2, this->m_func);
            instr->InsertBefore(newInstr);
            src2 = regOpnd32;
        }
        if (src3->IsFloat64())
        {
            IR::RegOpnd *regOpnd32 = IR::RegOpnd::New(TyFloat32, this->m_func);
            // CVTSD2SS regOpnd32.f32, src.f64    -- Convert regOpnd from f64 to f32
            newInstr = IR::Instr::New(Js::OpCode::CVTSD2SS, regOpnd32, src3, this->m_func);
            instr->InsertBefore(newInstr);
            src3 = regOpnd32;
        }
        if (src4->IsFloat64())
        {
            IR::RegOpnd *regOpnd32 = IR::RegOpnd::New(TyFloat32, this->m_func);
            // CVTSD2SS regOpnd32.f32, src.f64    -- Convert regOpnd from f64 to f32
            newInstr = IR::Instr::New(Js::OpCode::CVTSD2SS, regOpnd32, src4, this->m_func);
            instr->InsertBefore(newInstr);
            src4 = regOpnd32;
        }

        Assert(src1->IsRegOpnd() && src1->GetType() == TyFloat32);
        Assert(src2->IsRegOpnd() && src2->GetType() == TyFloat32);
        Assert(src3->IsRegOpnd() && src3->GetType() == TyFloat32);
        Assert(src4->IsRegOpnd() && src4->GetType() == TyFloat32);

        // MOVSS dst, src4
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, src4, m_func));
        // PSLLDQ dst, dst, 4
        instr->InsertBefore(IR::Instr::New(shiftOpcode, dst, dst, IR::IntConstOpnd::New(4, TyInt8, m_func, true), m_func));
        // MOVSS dst, src3
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, src3, m_func));
        // PSLLDQ dst, 4
        instr->InsertBefore(IR::Instr::New(shiftOpcode, dst, dst, IR::IntConstOpnd::New(4, TyInt8, m_func, true), m_func));
        // MOVSS dst, src2
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, src2, m_func));
        // PSLLDQ dst, 4
        instr->InsertBefore(IR::Instr::New(shiftOpcode, dst, dst, IR::IntConstOpnd::New(4, TyInt8, m_func, true), m_func));
        // MOVSS dst, src1
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, src1, m_func));
    }
    else
    {
        //Simd128_IntsToI4/U4
        IR::RegOpnd *temp = IR::RegOpnd::New(TyFloat32, m_func);

        // src's might have been constant prop'ed. Enregister them if so.
        src4 = EnregisterIntConst(instr, src4);
        src3 = EnregisterIntConst(instr, src3);
        src2 = EnregisterIntConst(instr, src2);
        src1 = EnregisterIntConst(instr, src1);

        Assert(src1->GetType() == TyInt32 && src1->IsRegOpnd());
        Assert(src2->GetType() == TyInt32 && src2->IsRegOpnd());
        Assert(src3->GetType() == TyInt32 && src3->IsRegOpnd());
        Assert(src4->GetType() == TyInt32 && src4->IsRegOpnd());

        // MOVD t(TyFloat32), src4(TyInt32)
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, temp, src4, m_func));

        // MOVSS dst, t
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, temp, m_func));
        // PSLLDQ dst, dst, 4
        instr->InsertBefore(IR::Instr::New(shiftOpcode, dst, dst, IR::IntConstOpnd::New(TySize[TyInt32], TyInt8, m_func, true), m_func));

        // MOVD t(TyFloat32), sr34(TyInt32)
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, temp, src3, m_func));
        // MOVSS dst, t
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, temp, m_func));
        // PSLLDQ dst, dst, 4
        instr->InsertBefore(IR::Instr::New(shiftOpcode, dst, dst, IR::IntConstOpnd::New(TySize[TyInt32], TyInt8, m_func, true), m_func));

        // MOVD t(TyFloat32), src2(TyInt32)
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, temp, src2, m_func));
        // MOVSS dst, t
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, temp, m_func));
        // PSLLDQ dst, dst, 4
        instr->InsertBefore(IR::Instr::New(shiftOpcode, dst, dst, IR::IntConstOpnd::New(TySize[TyInt32], TyInt8, m_func, true), m_func));

        // MOVD t(TyFloat32), src1(TyInt32)
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, temp, src1, m_func));
        // MOVSS dst, t
        instr->InsertBefore(IR::Instr::New(movOpcode, dst, temp, m_func));

        if (instr->m_opcode == Js::OpCode::Simd128_IntsToB4)
        {
            instr = Simd128CanonicalizeToBools(instr, Js::OpCode::PCMPEQD, *dst);
        }
    }

    IR::Instr* prevInstr;
    prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}
#if 0
IR::Instr *LowererMD::Simd128LowerConstructor_2(IR::Instr *instr)
{
    IR::Opnd* dst = nullptr;
    IR::Opnd* src1 = nullptr;
    IR::Opnd* src2 = nullptr;

    Assert(instr->m_opcode == Js::OpCode::Simd128_DoublesToD2);
    dst = instr->GetDst();

    src1 = instr->GetSrc1();
    src2 = instr->GetSrc2();

    Assert(src1->IsRegOpnd() && src1->GetType() == TyFloat64);
    Assert(src2->IsRegOpnd() && src2->GetType() == TyFloat64);
    // MOVSD dst, src2
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSD, dst, src2, m_func));
    // PSLLDQ dst, dst, 8
    instr->InsertBefore(IR::Instr::New(Js::OpCode::PSLLDQ, dst, dst, IR::IntConstOpnd::New(TySize[TyFloat64], TyInt8, m_func, true), m_func));
    // MOVSD dst, src1
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSD, dst, src1, m_func));
    Assert(dst->IsRegOpnd() && dst->IsSimd128());
    IR::Instr* prevInstr;
    prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}
#endif

IR::Instr* LowererMD::Simd128LowerLdLane(IR::Instr *instr)
{
    IR::Opnd* dst, *src1, *src2;
    Js::OpCode movOpcode = Js::OpCode::MOVSS;
    uint laneWidth = 0, laneIndex = 0, shamt = 0, mask = 0;
    IRType laneType = TyInt32;
    dst = instr->GetDst();
    src1 = instr->GetSrc1();
    src2 = instr->GetSrc2();

    Assert(dst && dst->IsRegOpnd() && (dst->GetType() == TyFloat32 || dst->GetType() == TyInt32 || dst->GetType() == TyUint32 || dst->GetType() == TyFloat64));
    Assert(src1 && src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2 && src2->IsIntConstOpnd());

    laneIndex = (uint)src2->AsIntConstOpnd()->AsUint32();
    laneWidth = 4;
    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_ExtractLane_F4:
        movOpcode = Js::OpCode::MOVSS;
        Assert(laneIndex < 4);
        break;
    case Js::OpCode::Simd128_ExtractLane_I8:
    case Js::OpCode::Simd128_ExtractLane_U8:
    case Js::OpCode::Simd128_ExtractLane_B8:
        movOpcode = Js::OpCode::MOVD;
        Assert(laneIndex < 8);
        shamt = (laneIndex % 2) * 16;
        laneIndex = laneIndex / 2;
        laneType = TyInt16;
        mask = 0x0000ffff;
        break;
    case Js::OpCode::Simd128_ExtractLane_I16:
    case Js::OpCode::Simd128_ExtractLane_U16:
    case Js::OpCode::Simd128_ExtractLane_B16:
        movOpcode = Js::OpCode::MOVD;
        Assert(laneIndex < 16);
        shamt = (laneIndex % 4) * 8;
        laneIndex = laneIndex / 4;
        laneType = TyInt8;
        mask = 0x000000ff;
        break;
    case Js::OpCode::Simd128_ExtractLane_U4:
    case Js::OpCode::Simd128_ExtractLane_I4:
    case Js::OpCode::Simd128_ExtractLane_B4:
        movOpcode = Js::OpCode::MOVD;
        Assert(laneIndex < 4);
        break;
    default:
        Assert(UNREACHED);
    }

    {
        IR::Opnd* tmp = src1;
        if (laneIndex != 0)
        {
            // tmp = PSRLDQ src1, shamt
            tmp = IR::RegOpnd::New(src1->GetType(), m_func);
            IR::Instr *shiftInstr = IR::Instr::New(Js::OpCode::PSRLDQ, tmp, src1, IR::IntConstOpnd::New(laneWidth * laneIndex, TyInt8, m_func, true), m_func);
            instr->InsertBefore(shiftInstr);
            Legalize(shiftInstr);
        }
        // MOVSS/MOVSD/MOVD dst, tmp
        instr->InsertBefore(IR::Instr::New(movOpcode, movOpcode == Js::OpCode::MOVD ? dst : dst->UseWithNewType(tmp->GetType(), m_func), tmp, m_func));
    }

    // dst has the 4-byte lane
    if (instr->m_opcode == Js::OpCode::Simd128_ExtractLane_I8 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_U8 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B8 ||
        instr->m_opcode == Js::OpCode::Simd128_ExtractLane_U16|| instr->m_opcode == Js::OpCode::Simd128_ExtractLane_I16|| instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B16)
    {
        // extract the 1/2 bytes sublane
        IR::Instr *newInstr = nullptr;
        if (shamt != 0)
        {
            // SHR dst, dst, shamt
            newInstr = IR::Instr::New(Js::OpCode::SHR, dst, dst, IR::IntConstOpnd::New((IntConstType)shamt, TyInt8, m_func), m_func);
            instr->InsertBefore(newInstr);
            Legalize(newInstr);
        }

        Assert(laneType == TyInt8 || laneType == TyInt16);
        // zero or sign-extend upper bits
        if (instr->m_opcode == Js::OpCode::Simd128_ExtractLane_I8 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_I16)
        {
            if (laneType == TyInt8)
            {
                IR::RegOpnd * tmp = IR::RegOpnd::New(TyInt8, m_func);
                newInstr = IR::Instr::New(Js::OpCode::MOV, tmp, dst, m_func);
                instr->InsertBefore(newInstr);
                Legalize(newInstr);
                newInstr = IR::Instr::New(Js::OpCode::MOVSX, dst, tmp, m_func);
            }
            else
            {
                newInstr = IR::Instr::New(Js::OpCode::MOVSXW, dst, dst->UseWithNewType(laneType, m_func), m_func);
            }
        }
        else
        {
            newInstr = IR::Instr::New(Js::OpCode::AND, dst, dst, IR::IntConstOpnd::New(mask, TyInt32, m_func), m_func);
        }

        instr->InsertBefore(newInstr);
        Legalize(newInstr);
    }
    if (instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B4 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B8 ||
        instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B16)
    {
        IR::Instr* pInstr    = nullptr;
        IR::RegOpnd* tmp = IR::RegOpnd::New(TyInt8, m_func);

        // cmp      dst, -1
        pInstr = IR::Instr::New(Js::OpCode::CMP, m_func);
        pInstr->SetSrc1(dst->UseWithNewType(laneType, m_func));
        pInstr->SetSrc2(IR::IntConstOpnd::New(-1, laneType, m_func, true));
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // mov     tmp(TyInt8), dst
        pInstr = IR::Instr::New(Js::OpCode::MOV, tmp, dst, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // sete     tmp(TyInt8)
        pInstr = IR::Instr::New(Js::OpCode::SETE, tmp, tmp, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // movsx      dst, tmp(TyInt8)
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSX, dst, tmp, m_func));
    }

    IR::Instr* prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

IR::Instr* LowererMD::Simd128LowerSplat(IR::Instr *instr)
{
    Js::OpCode shufOpCode = Js::OpCode::SHUFPS, movOpCode = Js::OpCode::MOVSS;
    IR::Opnd *dst, *src1;
    IR::Instr *pInstr = nullptr;
    dst = instr->GetDst();
    src1 = instr->GetSrc1();

    Assert(dst && dst->IsRegOpnd() && dst->IsSimd128());

    Assert(src1 && src1->IsRegOpnd() && (src1->GetType() == TyFloat32 || src1->GetType() == TyInt32 || src1->GetType() == TyFloat64 ||
        src1->GetType() == TyInt16 || src1->GetType() == TyInt8 || src1->GetType() == TyUint16 ||
        src1->GetType() == TyUint8 || src1->GetType() == TyUint32));

    Assert(!instr->GetSrc2());

    IR::Opnd* tempTruncate = nullptr;
    bool bSkip = false;
    IR::LabelInstr *labelZero = IR::LabelInstr::New(Js::OpCode::Label, m_func);
    IR::LabelInstr *labelDone = IR::LabelInstr::New(Js::OpCode::Label, m_func);

    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_Splat_F4:
        shufOpCode = Js::OpCode::SHUFPS;
        movOpCode = Js::OpCode::MOVSS;
        break;
    case Js::OpCode::Simd128_Splat_I4:
    case Js::OpCode::Simd128_Splat_U4:
        shufOpCode = Js::OpCode::PSHUFD;
        movOpCode = Js::OpCode::MOVD;
        break;
#if 0
    case Js::OpCode::Simd128_Splat_D2:
        shufOpCode = Js::OpCode::SHUFPD;
        movOpCode = Js::OpCode::MOVSD;
        break;
#endif // 0

    case Js::OpCode::Simd128_Splat_I8:
    case Js::OpCode::Simd128_Splat_U8:
        // MOV          tempTruncate(bx),  src1: truncate the value to 16bit int
        // MOVD         dst, tempTruncate(bx)
        // PUNPCKLWD    dst, dst
        // PSHUFD       dst, dst, 0
        tempTruncate = EnregisterIntConst(instr, src1, TyInt16);
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, dst, tempTruncate, m_func));
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PUNPCKLWD, dst, dst, dst, m_func));
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSHUFD, dst, dst, IR::IntConstOpnd::New(0, TyInt8, m_func, true), m_func));
        bSkip = true;
        break;
    case Js::OpCode::Simd128_Splat_I16:
    case Js::OpCode::Simd128_Splat_U16:
        // MOV          tempTruncate(bx),  src1: truncate the value to 8bit int
        // MOVD         dst, tempTruncate(bx)
        // PUNPCKLBW    dst, dst
        // PUNPCKLWD    dst, dst
        // PSHUFD       dst, dst, 0
        tempTruncate = EnregisterIntConst(instr, src1, TyInt8);
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, dst, tempTruncate, m_func));
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PUNPCKLBW, dst, dst, dst, m_func));
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PUNPCKLWD, dst, dst, dst, m_func));
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSHUFD, dst, dst, IR::IntConstOpnd::New(0, TyInt8, m_func, true), m_func));
        bSkip = true;
        break;
    case Js::OpCode::Simd128_Splat_B4:
    case Js::OpCode::Simd128_Splat_B8:
    case Js::OpCode::Simd128_Splat_B16:
        // CMP      src1, 0
        // JEQ      $labelZero
        // MOVAPS   dst, xmmword ptr[X86_ALL_NEG_ONES]
        // JMP      $labelDone
        // $labelZero:
        // XORPS    dst, dst
        // $labelDone:
        //pInstr = IR::Instr::New(Js::OpCode::CMP, src1, IR::IntConstOpnd::New(0, TyInt8, m_func, true), m_func);
        //instr->InsertBefore(pInstr);
        //Legalize(pInstr);

        // cmp      src1, 0000h
        pInstr = IR::Instr::New(Js::OpCode::CMP, m_func);
        pInstr->SetSrc1(src1);
        pInstr->SetSrc2(IR::IntConstOpnd::New(0x0000, TyInt32, m_func, true));
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        //JEQ       $labelZero
        instr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JEQ, labelZero, m_func));
        // MOVAPS   dst, xmmword ptr[X86_ALL_NEG_ONES]
        pInstr = IR::Instr::New(Js::OpCode::MOVAPS, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // JMP      $labelDone
        instr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JMP, labelDone, m_func));
        // $labelZero:
        instr->InsertBefore(labelZero);
        // XORPS    dst, dst
        instr->InsertBefore(IR::Instr::New(Js::OpCode::XORPS, dst, dst, dst, m_func));  // make dst to be 0
        // $labelDone:
        instr->InsertBefore(labelDone);
        bSkip = true;
        break;
    default:
        Assert(UNREACHED);
    }

    if (instr->m_opcode == Js::OpCode::Simd128_Splat_F4 && instr->GetSrc1()->IsFloat64())
    {
        IR::RegOpnd *regOpnd32 = IR::RegOpnd::New(TyFloat32, this->m_func);
        // CVTSD2SS regOpnd32.f32, src.f64    -- Convert regOpnd from f64 to f32
        instr->InsertBefore(IR::Instr::New(Js::OpCode::CVTSD2SS, regOpnd32, src1, this->m_func));
        src1 = regOpnd32;
    }

    if (!bSkip)
    {
        instr->InsertBefore(IR::Instr::New(movOpCode, dst, src1, m_func));
        instr->InsertBefore(IR::Instr::New(shufOpCode, dst, dst, IR::IntConstOpnd::New(0, TyInt8, m_func, true), m_func));
    }

    IR::Instr* prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

IR::Instr* LowererMD::Simd128LowerRcp(IR::Instr *instr, bool removeInstr)
{
    Js::OpCode opcode = Js::OpCode::DIVPS;
    IR::Opnd *dst, *src1;
    dst = instr->GetDst();
    src1 = instr->GetSrc1();

    Assert(dst && dst->IsRegOpnd());
    Assert(src1 && src1->IsRegOpnd());
    Assert(instr->GetSrc2() == nullptr);
    Assert(src1->IsSimd128F4() || src1->IsSimd128I4());
    opcode = Js::OpCode::DIVPS;

#if 0
    {
        Assert(instr->m_opcode == Js::OpCode::Simd128_Rcp_D2 || instr->m_opcode == Js::OpCode::Simd128_RcpSqrt_D2);
        Assert(src1->IsSimd128D2());
        opcode = Js::OpCode::DIVPD;
        x86_allones_mask = (void*)(&X86_ALL_ONES_D2);
    }
#endif // 0

    IR::RegOpnd* tmp = IR::RegOpnd::New(src1->GetType(), m_func);
    IR::Instr* movInstr = IR::Instr::New(Js::OpCode::MOVAPS, tmp, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllOnesF4Addr(), src1->GetType(), m_func), m_func);
    instr->InsertBefore(movInstr);
    Legalize(movInstr);

    instr->InsertBefore(IR::Instr::New(opcode, tmp, tmp, src1, m_func));
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, dst, tmp, m_func));
    if (removeInstr)
    {
        IR::Instr* prevInstr = instr->m_prev;
        instr->Remove();
        return prevInstr;
    }
    return instr;
}

IR::Instr* LowererMD::Simd128LowerSqrt(IR::Instr *instr)
{
    Js::OpCode opcode = Js::OpCode::SQRTPS;

    IR::Opnd *dst, *src1;
    dst = instr->GetDst();
    src1 = instr->GetSrc1();

    Assert(dst && dst->IsRegOpnd());
    Assert(src1 && src1->IsRegOpnd());
    Assert(instr->GetSrc2() == nullptr);
    opcode = Js::OpCode::SQRTPS;
#if 0
    {
        Assert(instr->m_opcode == Js::OpCode::Simd128_Sqrt_D2);
        opcode = Js::OpCode::SQRTPD;
    }
#endif // 0

    instr->InsertBefore(IR::Instr::New(opcode, dst, src1, m_func));
    IR::Instr* prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

IR::Instr* LowererMD::Simd128LowerRcpSqrt(IR::Instr *instr)
{
    Js::OpCode opcode = Js::OpCode::SQRTPS;
    Simd128LowerRcp(instr, false);

    opcode = Js::OpCode::SQRTPS;

#if 0
    else
    {
        Assert(instr->m_opcode == Js::OpCode::Simd128_RcpSqrt_D2);
        opcode = Js::OpCode::SQRTPD;
    }
#endif // 0

    instr->InsertBefore(IR::Instr::New(opcode, instr->GetDst(), instr->GetDst(), m_func));
    IR::Instr* prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

IR::Instr* LowererMD::Simd128LowerSelect(IR::Instr *instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_Select_F4 || instr->m_opcode == Js::OpCode::Simd128_Select_I4 /*|| instr->m_opcode == Js::OpCode::Simd128_Select_D2 */||
        instr->m_opcode == Js::OpCode::Simd128_Select_I8 || instr->m_opcode == Js::OpCode::Simd128_Select_I16 || instr->m_opcode == Js::OpCode::Simd128_Select_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_Select_U8 || instr->m_opcode == Js::OpCode::Simd128_Select_U16 );

    IR::Opnd* dst = nullptr;
    IR::Opnd* src1 = nullptr;
    IR::Opnd* src2 = nullptr;
    IR::Opnd* src3 = nullptr;
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);
    // The number of src opnds should be exact. Missing opnds means type-error, and we should generate an exception throw instead (or globopt does).
    Assert(args->Count() == 4);
    dst = args->Pop();
    src1 = args->Pop(); // mask
    src2 = args->Pop(); // trueValue
    src3 = args->Pop(); // falseValue

    Assert(dst->IsRegOpnd() && dst->IsSimd128());
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsRegOpnd() && src2->IsSimd128());
    Assert(src3->IsRegOpnd() && src3->IsSimd128());

    IR::RegOpnd *tmp = IR::RegOpnd::New(src1->GetType(), m_func);
    IR::Instr *pInstr = nullptr;
    // ANDPS tmp1, mask, tvalue
    pInstr = IR::Instr::New(Js::OpCode::ANDPS, tmp, src1, src2, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    // ANDPS dst, mask, fvalue
    pInstr = IR::Instr::New(Js::OpCode::ANDNPS, dst, src1, src3, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    // ORPS dst, dst, tmp1
    pInstr = IR::Instr::New(Js::OpCode::ORPS, dst, dst, tmp, m_func);
    instr->InsertBefore(pInstr);

    pInstr = instr->m_prev;
    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerNeg(IR::Instr *instr)
{

    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    Js::OpCode addOpcode = Js::OpCode::PADDD;
    void * allOnes = (void*)&X86_ALL_ONES_I4;

    Assert(dst->IsRegOpnd() && dst->IsSimd128());
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(instr->GetSrc2() == nullptr);

    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_Neg_I4:
    case Js::OpCode::Simd128_Neg_U4:
        break;
    case Js::OpCode::Simd128_Neg_I8:
    case Js::OpCode::Simd128_Neg_U8:
        addOpcode = Js::OpCode::PADDW;
        allOnes = (void*)&X86_ALL_ONES_I8;
        break;
    case Js::OpCode::Simd128_Neg_I16:
    case Js::OpCode::Simd128_Neg_U16:
        addOpcode = Js::OpCode::PADDB;
        allOnes = (void*)&X86_ALL_ONES_I16;
        break;
    default:
        Assert(UNREACHED);
    }

    // MOVAPS dst, src1
    IR::Instr *pInstr = IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func);
    instr->InsertBefore(pInstr);

    // PANDN dst, dst, 0xfff...f
    pInstr = IR::Instr::New(Js::OpCode::PANDN, dst, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), src1->GetType(), m_func), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // addOpCode dst, dst, {allOnes}
    pInstr = IR::Instr::New(addOpcode, dst, dst, IR::MemRefOpnd::New(allOnes, src1->GetType(), m_func), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    pInstr = instr->m_prev;
    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerMulI4(IR::Instr *instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_Mul_I4 || instr->m_opcode == Js::OpCode::Simd128_Mul_U4);
    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    IR::Opnd* temp1, *temp2, *temp3;
    Assert(dst->IsRegOpnd() && dst->IsSimd128());
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsRegOpnd() && src2->IsSimd128());

    temp1 = IR::RegOpnd::New(src1->GetType(), m_func);
    temp2 = IR::RegOpnd::New(src1->GetType(), m_func);
    temp3 = IR::RegOpnd::New(src1->GetType(), m_func);

    // temp1 = PMULUDQ src1, src2
    pInstr = IR::Instr::New(Js::OpCode::PMULUDQ, temp1, src1, src2, m_func);
    instr->InsertBefore(pInstr);
    //MakeDstEquSrc1(pInstr);
    Legalize(pInstr);

    // temp2 = PSLRD src1, 0x4
    pInstr = IR::Instr::New(Js::OpCode::PSRLDQ, temp2, src1, IR::IntConstOpnd::New(TySize[TyInt32], TyInt8, m_func, true), m_func);
    instr->InsertBefore(pInstr);
    //MakeDstEquSrc1(pInstr);
    Legalize(pInstr);

    // temp3 = PSLRD src2, 0x4
    pInstr = IR::Instr::New(Js::OpCode::PSRLDQ, temp3, src2, IR::IntConstOpnd::New(TySize[TyInt32], TyInt8, m_func, true), m_func);
    instr->InsertBefore(pInstr);
    //MakeDstEquSrc1(pInstr);
    Legalize(pInstr);

    // temp2 = PMULUDQ temp2, temp3
    pInstr = IR::Instr::New(Js::OpCode::PMULUDQ, temp2, temp2, temp3, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    //PSHUFD temp1, temp1, 0x8
    instr->InsertBefore(IR::Instr::New(Js::OpCode::PSHUFD, temp1, temp1, IR::IntConstOpnd::New( 8 /*b00001000*/, TyInt8, m_func, true), m_func));

    //PSHUFD temp2, temp2, 0x8
    instr->InsertBefore(IR::Instr::New(Js::OpCode::PSHUFD, temp2, temp2, IR::IntConstOpnd::New(8 /*b00001000*/, TyInt8, m_func, true), m_func));

    // PUNPCKLDQ dst, temp1, temp2
    pInstr = IR::Instr::New(Js::OpCode::PUNPCKLDQ, dst, temp1, temp2, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    pInstr = instr->m_prev;
    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerMulI16(IR::Instr *instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_Mul_I16 || instr->m_opcode == Js::OpCode::Simd128_Mul_U16);
    IR::Instr *pInstr = nullptr;
    IR::Opnd* dst  = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    IR::Opnd* temp1, *temp2, *temp3;
    IRType simdType, laneType;
    if (instr->m_opcode == Js::OpCode::Simd128_Mul_I16)
    {
        simdType = TySimd128I16;
        laneType = TyInt8;
    }
    else
    {
        simdType = TySimd128U16;
        laneType = TyUint8;
    }
    Assert(dst->IsRegOpnd() && dst->GetType() == simdType);
    Assert(src1->IsRegOpnd() && src1->GetType() == simdType);
    Assert(src2->IsRegOpnd() && src2->GetType() == simdType);

    temp1 = IR::RegOpnd::New(simdType, m_func);
    temp2 = IR::RegOpnd::New(simdType, m_func);
    temp3 = IR::RegOpnd::New(simdType, m_func);

    // MOVAPS temp1, src1
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, temp1, src1, m_func));
    //PMULLW temp1, src2
    pInstr = IR::Instr::New(Js::OpCode::PMULLW, temp1, temp1, src2, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    //PAND temp1 {0x00ff00ff00ff00ff00ff00ff00ff00ff}  :To zero out bytes 1,3,5...
    pInstr = IR::Instr::New(Js::OpCode::PAND, temp1, temp1, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86LowBytesMaskAddr(), simdType, m_func), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    //PSRLW src1, 8
    pInstr = IR::Instr::New(Js::OpCode::PSRLW, temp2, src2, IR::IntConstOpnd::New(8, laneType, m_func, true), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    //PSRLW src2, 8  :upper 8 bits of each word
    pInstr = IR::Instr::New(Js::OpCode::PSRLW, temp3, src1, IR::IntConstOpnd::New(8, laneType, m_func, true), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    //PMULLW src1, src2
    pInstr = IR::Instr::New(Js::OpCode::PMULLW, temp2, temp2, temp3, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    //PSLLW src1, 8  :sets the results bytes 1,3,5..
    pInstr = IR::Instr::New(Js::OpCode::PSLLW, temp2, temp2, IR::IntConstOpnd::New(8, laneType, m_func, true), m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);
    //POR temp1, src1 :OR bytes 0,2,4.. to final result
    pInstr = IR::Instr::New(Js::OpCode::POR, dst, temp1, temp2, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    pInstr = instr->m_prev;
    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerShift(IR::Instr *instr)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd() && dst->IsSimd128());
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsInt32());

    Js::OpCode opcode = Js::OpCode::PSLLD;
    int elementSizeInBytes = 0;

    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_ShLtByScalar_I4:
    case Js::OpCode::Simd128_ShLtByScalar_U4:    // same as int32x4.ShiftLeftScalar
        opcode = Js::OpCode::PSLLD;
        elementSizeInBytes = 4;
        break;
    case Js::OpCode::Simd128_ShRtByScalar_I4:
        opcode = Js::OpCode::PSRAD;
        elementSizeInBytes = 4;
        break;
    case Js::OpCode::Simd128_ShLtByScalar_I8:
    case Js::OpCode::Simd128_ShLtByScalar_U8:    // same as int16x8.ShiftLeftScalar
        opcode = Js::OpCode::PSLLW;
        elementSizeInBytes = 2;
        break;
    case Js::OpCode::Simd128_ShRtByScalar_I8:
        opcode = Js::OpCode::PSRAW;
        elementSizeInBytes = 2;
        break;
    case Js::OpCode::Simd128_ShRtByScalar_U4:
        opcode = Js::OpCode::PSRLD;
        elementSizeInBytes = 4;
        break;
    case Js::OpCode::Simd128_ShRtByScalar_U8:
        opcode = Js::OpCode::PSRLW;
        elementSizeInBytes = 2;
        break;
    case Js::OpCode::Simd128_ShLtByScalar_I16:   // composite, int8x16.ShiftLeftScalar
    case Js::OpCode::Simd128_ShRtByScalar_I16:   // composite, int8x16.ShiftRightScalar
    case Js::OpCode::Simd128_ShLtByScalar_U16:   // same as int8x16.ShiftLeftScalar
    case Js::OpCode::Simd128_ShRtByScalar_U16:   // composite, uint8x16.ShiftRightScalar
        elementSizeInBytes = 1;
        break;
    default:
        Assert(UNREACHED);
    }

    IR::Instr *pInstr = nullptr;
    IR::RegOpnd *reg  = IR::RegOpnd::New(TyInt32, m_func);
    IR::RegOpnd *reg2 = IR::RegOpnd::New(TyInt32, m_func);
    IR::RegOpnd *tmp0 = IR::RegOpnd::New(src1->GetType(), m_func);
    IR::RegOpnd *tmp1 = IR::RegOpnd::New(src1->GetType(), m_func);
    IR::RegOpnd *tmp2 = IR::RegOpnd::New(src1->GetType(), m_func);

    //Shift amount: The shift amout is masked by [ElementSize] * 8
    //The masked Shift amount is moved to xmm register
    //AND  shamt, shmask, shamt
    //MOVD tmp0, shamt 

    IR::RegOpnd *shamt = IR::RegOpnd::New(src2->GetType(), m_func);
    // en-register
    IR::Opnd *origShamt = EnregisterIntConst(instr, src2); //unnormalized shift amount
    pInstr = IR::Instr::New(Js::OpCode::AND, shamt, origShamt, IR::IntConstOpnd::New(Js::SIMDUtils::SIMDGetShiftAmountMask(elementSizeInBytes), TyInt32, m_func), m_func); // normalizing by elm width (i.e. shamt % elm_width)
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    pInstr = IR::Instr::New(Js::OpCode::MOVD, tmp0, shamt, m_func);
    instr->InsertBefore(pInstr);


    if (instr->m_opcode == Js::OpCode::Simd128_ShLtByScalar_I4 || instr->m_opcode == Js::OpCode::Simd128_ShRtByScalar_I4 ||
        instr->m_opcode == Js::OpCode::Simd128_ShLtByScalar_U4 || instr->m_opcode == Js::OpCode::Simd128_ShRtByScalar_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_ShLtByScalar_I8 || instr->m_opcode == Js::OpCode::Simd128_ShRtByScalar_I8 ||
        instr->m_opcode == Js::OpCode::Simd128_ShLtByScalar_U8 || instr->m_opcode == Js::OpCode::Simd128_ShRtByScalar_U8)
    {
        // shiftOpCode dst, src1, tmp0
        pInstr = IR::Instr::New(opcode, dst, src1, tmp0, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_ShLtByScalar_I16 || instr->m_opcode == Js::OpCode::Simd128_ShLtByScalar_U16)
    {
        // MOVAPS tmp1, src1
        pInstr = IR::Instr::New(Js::OpCode::MOVAPS, tmp1, src1, m_func);
        instr->InsertBefore(pInstr);
        // MOVAPS dst, src1
        pInstr = IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func);
        instr->InsertBefore(pInstr);
        // PAND     tmp1, [X86_HIGHBYTES_MASK]
        pInstr = IR::Instr::New(Js::OpCode::PAND, tmp1, tmp1, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86HighBytesMaskAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // PSLLW  tmp1, tmp0
        pInstr = IR::Instr::New(Js::OpCode::PSLLW, tmp1, tmp1, tmp0, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // PSLLW  dst, tmp0
        pInstr = IR::Instr::New(Js::OpCode::PSLLW, dst, dst, tmp0, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // PAND   dst, [X86_LOWBYTES_MASK]
        pInstr = IR::Instr::New(Js::OpCode::PAND, dst, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86LowBytesMaskAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // POR    dst,  tmp1
        pInstr = IR::Instr::New(Js::OpCode::POR, dst, dst, tmp1, m_func);
        instr->InsertBefore(pInstr);
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_ShRtByScalar_I16)
    {
        // MOVAPS   tmp1, src1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, tmp1, src1, m_func));
        // MOVAPS   dst, src1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func));
        // PSLLW    dst, 8
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSLLW, dst, dst, IR::IntConstOpnd::New(8, TyInt8, m_func), m_func));
        // LEA      reg, [shamt + 8]
        IR::IndirOpnd *indirOpnd = IR::IndirOpnd::New(shamt->AsRegOpnd(), +8, TyInt32, m_func);
        instr->InsertBefore(IR::Instr::New(Js::OpCode::LEA, reg, indirOpnd, m_func));
        // MOVD   tmp0, reg
        pInstr = IR::Instr::New(Js::OpCode::MOVD, tmp2, reg, m_func);
        instr->InsertBefore(pInstr);
        // PSRAW    dst, tmp0
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSRAW, dst, dst, tmp2, m_func));
        // PAND     dst, [X86_LOWBYTES_MASK]
        pInstr = IR::Instr::New(Js::OpCode::PAND, dst, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86LowBytesMaskAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // PSRAW    tmp1, tmp0
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSRAW, tmp1, tmp1, tmp0, m_func));
        // PAND     tmp1, [X86_HIGHBYTES_MASK]
        pInstr = IR::Instr::New(Js::OpCode::PAND, tmp1, tmp1, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86HighBytesMaskAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // POR      dst, tmp1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::POR, dst, dst, tmp1, m_func));
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_ShRtByScalar_U16)
    {
        IR::RegOpnd * shamtReg = IR::RegOpnd::New(TyInt8, m_func);
        shamtReg->SetReg(LowererMDArch::GetRegShiftCount());
        IR::RegOpnd * tmp = IR::RegOpnd::New(TyInt8, m_func);

        // MOVAPS   dst, src1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func));
        // MOV      reg2, 0FFh
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOV, reg2, IR::IntConstOpnd::New(0xFF, TyInt32, m_func), m_func));
        // MOV      shamtReg, shamt
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOV, shamtReg, shamt, m_func));
        // SHR      reg2, shamtReg (lower 8 bit)
        instr->InsertBefore(IR::Instr::New(Js::OpCode::SHR, reg2, reg2, shamtReg, m_func));

        // MOV      tmp, reg2
        // MOVSX    reg2, tmp(TyInt8)

        pInstr = IR::Instr::New(Js::OpCode::MOV, tmp, reg2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSX, reg2, tmp, m_func));
        IR::RegOpnd *mask = IR::RegOpnd::New(TySimd128I4, m_func);
        // PSRLW dst, mask
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSRLW, dst, dst, tmp0, m_func));
        // splat (0xFF >> shamt) into mask
        // MOVD  mask, reg2
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, mask, reg2, m_func));
        // PUNPCKLBW   mask, mask
        pInstr = IR::Instr::New(Js::OpCode::PUNPCKLBW, mask, mask, mask, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // PUNPCKLWD   mask, mask
        pInstr = IR::Instr::New(Js::OpCode::PUNPCKLWD, mask, mask, mask, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // PSHUFD      mask, mask, 0
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSHUFD, mask, mask, IR::IntConstOpnd::New(0, TyInt8, m_func, true), m_func));
        // PAND dst, mask
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PAND, dst, dst, mask, m_func));
    }
    else
    {
        Assert(UNREACHED);
    }

    pInstr = instr->m_prev;
    instr->Remove();

    return pInstr;
}

IR::Instr* LowererMD::SIMD128LowerReplaceLane_8(IR::Instr* instr)
{
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);

    int lane = 0;

    IR::Opnd *dst  = args->Pop();
    IR::Opnd *src1 = args->Pop();
    IR::Opnd *src2 = args->Pop();
    IR::Opnd *src3 = args->Pop();
    IR::Instr * newInstr = nullptr;

    Assert(dst->IsSimd128() && src1->IsSimd128());

    lane = src2->AsIntConstOpnd()->AsInt32();

    IR::Opnd* laneValue = EnregisterIntConst(instr, src3, TyInt16);

    Assert(instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_I8 || instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_U8 || instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B8);

    // MOVAPS dst, src1
    newInstr = IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // PINSRW dst, value, index
    newInstr = IR::Instr::New(Js::OpCode::PINSRW, dst, laneValue, IR::IntConstOpnd::New(lane, TyInt8, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    if (instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B8)  //canonicalizing lanes
    {
        instr = Simd128CanonicalizeToBools(instr, Js::OpCode::PCMPEQW, *dst);
    }

    IR::Instr* prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}


IR::Instr* LowererMD::SIMD128LowerReplaceLane_16(IR::Instr* instr)
{
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);
    int lane = 0;
    IR::Opnd *dst = args->Pop();
    IR::Opnd *src1 = args->Pop();
    IR::Opnd *src2 = args->Pop();
    IR::Opnd *src3 = args->Pop();
    IR::Instr * newInstr = nullptr;

    Assert(dst->IsSimd128() && src1->IsSimd128());

    lane = src2->AsIntConstOpnd()->AsInt32();
    Assert(lane >= 0 && lane < 16);

    IR::Opnd* laneValue = EnregisterIntConst(instr, src3, TyInt8);
    intptr_t tempSIMD = m_func->GetThreadContextInfo()->GetSimdTempAreaAddr(0);
#if DBG
    // using only one SIMD temp
    intptr_t endAddrSIMD = tempSIMD + sizeof(X86SIMDValue);
#endif

    Assert(instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_I16 || instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_U16 || instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B16);
    // MOVUPS [temp], src1
    intptr_t address = tempSIMD;
    newInstr = IR::Instr::New(Js::OpCode::MOVUPS, IR::MemRefOpnd::New(address, TySimd128I16, m_func), src1, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // MOV [temp+offset], laneValue
    address = tempSIMD + lane;
    // check for buffer overrun
    Assert((intptr_t)address < endAddrSIMD);
    newInstr = IR::Instr::New(Js::OpCode::MOV, IR::MemRefOpnd::New(address, TyInt8, m_func), laneValue, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // MOVUPS dst, [temp]
    address = tempSIMD;
    newInstr = IR::Instr::New(Js::OpCode::MOVUPS, dst, IR::MemRefOpnd::New(address, TySimd128I16, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    if (instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B16)  //canonicalizing lanes.
    {
        instr = Simd128CanonicalizeToBools(instr, Js::OpCode::PCMPEQB, *dst);
    }

    IR::Instr* prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

IR::Instr* LowererMD::SIMD128LowerReplaceLane_4(IR::Instr* instr)
{
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);

    int lane = 0, byteWidth = 0;

    IR::Opnd *dst = args->Pop();
    IR::Opnd *src1 = args->Pop();
    IR::Opnd *src2 = args->Pop();
    IR::Opnd *src3 = args->Pop();

    Assert(dst->IsSimd128() && src1->IsSimd128());
    IRType type = dst->GetType();
    lane = src2->AsIntConstOpnd()->AsInt32();

    IR::Opnd* laneValue = EnregisterIntConst(instr, src3);

    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_ReplaceLane_I4:
    case Js::OpCode::Simd128_ReplaceLane_U4:
    case Js::OpCode::Simd128_ReplaceLane_B4:
        byteWidth = TySize[TyInt32];
        break;
    case Js::OpCode::Simd128_ReplaceLane_F4:
        byteWidth = TySize[TyFloat32];
        break;
    default:
        Assert(UNREACHED);
    }

    // MOVAPS dst, src1
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func));

    if (laneValue->GetType() == TyInt32 || laneValue->GetType() == TyUint32)
    {
        IR::RegOpnd *tempReg = IR::RegOpnd::New(TyFloat32, m_func);//mov intval to xmm
                                                                    //MOVD
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVD, tempReg, laneValue, m_func));
        laneValue = tempReg;
    }
    Assert(laneValue->GetType() == TyFloat32);
    if (lane == 0)
    {
        // MOVSS for both TyFloat32 and TyInt32. MOVD zeroes upper bits.
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSS, dst, laneValue, m_func));
    }
    else if (lane == 2)
    {
        IR::RegOpnd *tmp = IR::RegOpnd::New(type, m_func);
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVHLPS, tmp, dst, m_func));
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSS, tmp, laneValue, m_func));
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVLHPS, dst, tmp, m_func));
    }
    else
    {
        Assert(lane == 1 || lane == 3);
        uint8 shufMask = 0xE4; // 11 10 01 00
        shufMask |= lane;      // 11 10 01 id
        shufMask &= ~(0x03 << (lane << 1)); // set 2 bits corresponding to lane index to 00

                                            // SHUFPS dst, dst, shufMask
        instr->InsertBefore(IR::Instr::New(Js::OpCode::SHUFPS, dst, dst, IR::IntConstOpnd::New(shufMask, TyInt8, m_func, true), m_func));

        // MOVSS dst, value
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSS, dst, laneValue, m_func));

        // SHUFPS dst, dst, shufMask
        instr->InsertBefore(IR::Instr::New(Js::OpCode::SHUFPS, dst, dst, IR::IntConstOpnd::New(shufMask, TyInt8, m_func, true), m_func));
    }
    if (instr->m_opcode == Js::OpCode::Simd128_ReplaceLane_B4) //Canonicalizing lanes
    {
        instr = Simd128CanonicalizeToBools(instr, Js::OpCode::PCMPEQD, *dst);
    }
    IR::Instr* prevInstr = instr->m_prev;
    instr->Remove();
    return prevInstr;
}

/*
4 and 2 lane Swizzle.
*/
IR::Instr* LowererMD::Simd128LowerSwizzle_4(IR::Instr* instr)
{
    Js::OpCode shufOpcode = Js::OpCode::SHUFPS;
    Js::OpCode irOpcode = instr->m_opcode;

    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);

    IR::Opnd *dst = args->Pop();
    IR::Opnd *srcs[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

    int i = 0;
    while (!args->Empty() && i < 6)
    {
        srcs[i++] = args->Pop();
    }

    int8 shufMask = 0;
    int lane0 = 0, lane1 = 0, lane2 = 0, lane3 = 0;
    IR::Instr *pInstr = instr->m_prev;

    Assert(dst->IsSimd128() && srcs[0] && srcs[0]->IsSimd128());

    // globOpt will type-spec if all lane indices are constants, and within range constraints to match a single SSE instruction
    Assert(irOpcode == Js::OpCode::Simd128_Swizzle_I4 || irOpcode == Js::OpCode::Simd128_Swizzle_U4 || irOpcode == Js::OpCode::Simd128_Swizzle_F4 /*|| irOpcode == Js::OpCode::Simd128_Swizzle_D2*/);
    AssertMsg(srcs[1] && srcs[1]->IsIntConstOpnd() &&
              srcs[2] && srcs[2]->IsIntConstOpnd() &&
              (/*irOpcode == Js::OpCode::Simd128_Swizzle_D2 || */(srcs[3] && srcs[3]->IsIntConstOpnd())) &&
              (/*irOpcode == Js::OpCode::Simd128_Swizzle_D2 || */(srcs[4] && srcs[4]->IsIntConstOpnd())), "Type-specialized swizzle is supported only with constant lane indices");

#if 0
    if (irOpcode == Js::OpCode::Simd128_Swizzle_D2)
    {
        lane0 = srcs[1]->AsIntConstOpnd()->AsInt32();
        lane1 = srcs[2]->AsIntConstOpnd()->AsInt32();
        Assert(lane0 >= 0 && lane0 < 2);
        Assert(lane1 >= 0 && lane1 < 2);
        shufMask = (int8)((lane1 << 1) | lane0);
        shufOpcode = Js::OpCode::SHUFPD;
    }
#endif // 0

    if (irOpcode == Js::OpCode::Simd128_Swizzle_I4 || irOpcode == Js::OpCode::Simd128_Swizzle_U4)
    {
        shufOpcode = Js::OpCode::PSHUFD;
    }
    AnalysisAssert(srcs[3] != nullptr && srcs[4] != nullptr);
    lane0 = srcs[1]->AsIntConstOpnd()->AsInt32();
    lane1 = srcs[2]->AsIntConstOpnd()->AsInt32();
    lane2 = srcs[3]->AsIntConstOpnd()->AsInt32();
    lane3 = srcs[4]->AsIntConstOpnd()->AsInt32();
    Assert(lane1 >= 0 && lane1 < 4);
    Assert(lane2 >= 0 && lane2 < 4);
    Assert(lane2 >= 0 && lane2 < 4);
    Assert(lane3 >= 0 && lane3 < 4);
    shufMask = (int8)((lane3 << 6) | (lane2 << 4) | (lane1 << 2) | lane0);

    instr->m_opcode = shufOpcode;
    instr->SetDst(dst);

    // MOVAPS dst, src1
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, dst, srcs[0], m_func));
    // SHUF dst, dst, imm8
    instr->SetSrc1(dst);
    instr->SetSrc2(IR::IntConstOpnd::New((IntConstType)shufMask, TyInt8, m_func, true));
    return pInstr;
}

/*
4 lane shuffle. Handles arbitrary lane values.
*/

IR::Instr* LowererMD::Simd128LowerShuffle_4(IR::Instr* instr)
{
    Js::OpCode irOpcode = instr->m_opcode;
    SList<IR::Opnd*> *args = Simd128GetExtendedArgs(instr);
    IR::Opnd *dst = args->Pop();
    IR::Opnd *srcs[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

    int j = 0;
    while (!args->Empty() && j < 6)
    {
        srcs[j++] = args->Pop();
    }

    uint8 lanes[4], lanesSrc[4];
    uint fromSrc1, fromSrc2;
    IR::Instr *pInstr = instr->m_prev;

    Assert(dst->IsSimd128() && srcs[0] && srcs[0]->IsSimd128() && srcs[1] && srcs[1]->IsSimd128());
    Assert(irOpcode == Js::OpCode::Simd128_Shuffle_I4 || irOpcode == Js::OpCode::Simd128_Shuffle_U4 || irOpcode == Js::OpCode::Simd128_Shuffle_F4);

    // globOpt will type-spec if all lane indices are constants, and within range constraints to match a single SSE instruction
    AssertMsg(srcs[2] && srcs[2]->IsIntConstOpnd() &&
              srcs[3] && srcs[3]->IsIntConstOpnd() &&
              srcs[4] && srcs[4]->IsIntConstOpnd() &&
              srcs[5] && srcs[5]->IsIntConstOpnd(), "Type-specialized shuffle is supported only with constant lane indices");

    lanes[0] = (uint8) srcs[2]->AsIntConstOpnd()->AsInt32();
    lanes[1] = (uint8) srcs[3]->AsIntConstOpnd()->AsInt32();
    lanes[2] = (uint8) srcs[4]->AsIntConstOpnd()->AsInt32();
    lanes[3] = (uint8) srcs[5]->AsIntConstOpnd()->AsInt32();
    Assert(lanes[0] >= 0 && lanes[0] < 8);
    Assert(lanes[1] >= 0 && lanes[1] < 8);
    Assert(lanes[2] >= 0 && lanes[2] < 8);
    Assert(lanes[3] >= 0 && lanes[3] < 8);

    CheckShuffleLanes_4(lanes, lanesSrc, &fromSrc1, &fromSrc2);
    Assert(fromSrc1 + fromSrc2 == 4);

    if (fromSrc1 == 4 || fromSrc2 == 4)
    {
        // can be done with a swizzle
        IR::Opnd *srcOpnd = fromSrc1 == 4 ? srcs[0] : srcs[1];
        InsertShufps(lanes, dst, srcOpnd, srcOpnd, instr);
    }
    else if (fromSrc1 == 2)
    {
        if (lanes[0] < 4 && lanes[1] < 4)
        {
            // x86 friendly shuffle
            Assert(lanes[2] >= 4 && lanes[3] >= 4);
            InsertShufps(lanes, dst, srcs[0], srcs[1], instr);
        }
        else
        {
            // arbitrary shuffle with 2 lanes from each src
            uint8 ordLanes[4], reArrLanes[4];

            // order lanes based on which src they come from
            // compute re-arrangement mask
            for (uint8 i = 0, j1 = 0, j2 = 2; i < 4; i++)
            {
                if (lanesSrc[i] == 1 && j1 < 4)
                {
                    ordLanes[j1] = lanes[i];
                    reArrLanes[i] = j1;
                    j1++;
                }
                else if(j2 < 4)
                {
                    Assert(lanesSrc[i] == 2);
                    ordLanes[j2] = lanes[i];
                    reArrLanes[i] = j2;
                    j2++;
                }
            }
            IR::RegOpnd *temp = IR::RegOpnd::New(dst->GetType(), m_func);
            InsertShufps(ordLanes, temp, srcs[0], srcs[1], instr);
            InsertShufps(reArrLanes, dst, temp, temp, instr);
        }
    }
    else if (fromSrc1 == 3 || fromSrc2 == 3)
    {
        // shuffle with 3 lanes from one src, one from another

        IR::Instr *newInstr;
        IR::Opnd * majSrc, *minSrc;
        IR::RegOpnd *temp1 = IR::RegOpnd::New(dst->GetType(), m_func);
        IR::RegOpnd *temp2 = IR::RegOpnd::New(dst->GetType(), m_func);
        IR::RegOpnd *temp3 = IR::RegOpnd::New(dst->GetType(), m_func);
        uint8 minorityLane = 0, maxLaneValue;
        majSrc = fromSrc1 == 3 ? srcs[0] : srcs[1];
        minSrc = fromSrc1 == 3 ? srcs[1] : srcs[0];
        Assert(majSrc != minSrc);

        // Algorithm:
        // SHUFPS temp1, majSrc, lanes
        // SHUFPS temp2, minSrc, lanes
        // MOVUPS temp3, [minorityLane mask]
        // ANDPS  temp2, temp3          // mask all lanes but minorityLane
        // ANDNPS temp3, temp1          // zero minorityLane
        // ORPS   dst, temp2, temp3

        // find minorityLane to mask
        maxLaneValue = minSrc == srcs[0] ? 4 : 8;
        for (uint8 i = 0; i < 4; i++)
        {
            if (lanes[i] >= (maxLaneValue - 4) && lanes[i] < maxLaneValue)
            {
                minorityLane = i;
                break;
            }
        }
        IR::MemRefOpnd * laneMask = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86FourLanesMaskAddr(minorityLane), dst->GetType(), m_func);

        InsertShufps(lanes, temp1, majSrc, majSrc, instr);
        InsertShufps(lanes, temp2, minSrc, minSrc, instr);
        newInstr = IR::Instr::New(Js::OpCode::MOVUPS, temp3, laneMask, m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        newInstr = IR::Instr::New(Js::OpCode::ANDPS, temp2, temp2, temp3, m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        newInstr = IR::Instr::New(Js::OpCode::ANDNPS, temp3, temp3, temp1, m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        newInstr = IR::Instr::New(Js::OpCode::ORPS, dst, temp2, temp3, m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
    }

    instr->Remove();
    return pInstr;
}

// 8 and 16 lane shuffle with memory temps
IR::Instr* LowererMD::Simd128LowerShuffle(IR::Instr* instr)
{
    Js::OpCode irOpcode = instr->m_opcode;
    IR::Instr *pInstr = instr->m_prev, *newInstr = nullptr;
    SList<IR::Opnd*> *args = nullptr;
    IR::Opnd *dst = nullptr;
    IR::Opnd *src1 = nullptr, *src2 = nullptr;
    uint8 lanes[16], laneCount = 0, scale = 1;
    bool isShuffle = false;
    IRType laneType = TyInt16;
    intptr_t temp1SIMD = m_func->GetThreadContextInfo()->GetSimdTempAreaAddr(0);
    intptr_t temp2SIMD = m_func->GetThreadContextInfo()->GetSimdTempAreaAddr(1);
    intptr_t dstSIMD   = m_func->GetThreadContextInfo()->GetSimdTempAreaAddr(2);
#if DBG
    intptr_t endAddrSIMD = (intptr_t)(temp1SIMD + sizeof(X86SIMDValue) * SIMD_TEMP_SIZE);
#endif
    void *address = nullptr;
    args = Simd128GetExtendedArgs(instr);

    switch (irOpcode)
    {
    case Js::OpCode::Simd128_Swizzle_I8:
    case Js::OpCode::Simd128_Swizzle_U8:
        Assert(args->Count() == 10);
        laneCount = 8;
        laneType = TyInt16;
        isShuffle = false;
        scale = 2;
        break;
    case Js::OpCode::Simd128_Swizzle_I16:
    case Js::OpCode::Simd128_Swizzle_U16:
        Assert(args->Count() == 18);
        laneCount = 16;
        laneType = TyInt8;
        isShuffle = false;
        scale = 1;
        break;
    case Js::OpCode::Simd128_Shuffle_I8:
    case Js::OpCode::Simd128_Shuffle_U8:
        Assert(args->Count() == 11);
        laneCount = 8;
        isShuffle = true;
        laneType = TyUint16;
        scale = 2;
        break;
        case Js::OpCode::Simd128_Shuffle_I16:
    case Js::OpCode::Simd128_Shuffle_U16:
        Assert(args->Count() == 19);
        laneCount = 16;
        isShuffle = true;
        laneType = TyUint8;
        scale = 1;
        break;
    default:
        Assert(UNREACHED);
    }

    dst = args->Pop();
    src1 = args->Pop();
    if (isShuffle)
    {
        src2 = args->Pop();
    }

    Assert(dst->IsSimd128() && src1 && src1->IsSimd128() && (!isShuffle|| src2->IsSimd128()));

    for (uint i = 0; i < laneCount; i++)
    {
        IR::Opnd * laneOpnd = args->Pop();
        Assert(laneOpnd->IsIntConstOpnd());
        lanes[i] = (uint8)laneOpnd->AsIntConstOpnd()->AsInt32();
    }

    // MOVUPS [temp], src1
    newInstr = IR::Instr::New(Js::OpCode::MOVUPS, IR::MemRefOpnd::New((void*)temp1SIMD, TySimd128I16, m_func), src1, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);
    if (isShuffle)
    {
        // MOVUPS [temp+16], src2
        newInstr = IR::Instr::New(Js::OpCode::MOVUPS, IR::MemRefOpnd::New((void*)(temp2SIMD), TySimd128I16, m_func), src2, m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
    }
    for (uint i = 0; i < laneCount; i++)
    {
        //. MOV tmp, [temp1SIMD + laneValue*scale]
        IR::RegOpnd *tmp = IR::RegOpnd::New(laneType, m_func);
        address = (void*)(temp1SIMD + lanes[i] * scale);
        Assert((intptr_t)address + (intptr_t)scale <= (intptr_t)dstSIMD);
        newInstr = IR::Instr::New(Js::OpCode::MOV, tmp, IR::MemRefOpnd::New(address, laneType, m_func), m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);

        //. MOV [dstSIMD + i*scale], tmp
        address = (void*)(dstSIMD + i * scale);
        Assert((intptr_t)address + (intptr_t) scale <= endAddrSIMD);
        newInstr = IR::Instr::New(Js::OpCode::MOV,IR::MemRefOpnd::New(address, laneType, m_func), tmp, m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
    }

    // MOVUPS dst, [dstSIMD]
    newInstr = IR::Instr::New(Js::OpCode::MOVUPS, dst, IR::MemRefOpnd::New((void*)dstSIMD, TySimd128I16, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerNotEqual(IR::Instr* instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_Neq_I4 || instr->m_opcode == Js::OpCode::Simd128_Neq_I8 ||
        instr->m_opcode == Js::OpCode::Simd128_Neq_I16 || instr->m_opcode == Js::OpCode::Simd128_Neq_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_Neq_U8 || instr->m_opcode == Js::OpCode::Simd128_Neq_U16);

    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd() && (dst->IsSimd128B4() || dst->IsSimd128B8() || dst->IsSimd128B16()));
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsRegOpnd() && src2->IsSimd128());

    Js::OpCode cmpOpcode = Js::OpCode::PCMPEQD;
    if (instr->m_opcode == Js::OpCode::Simd128_Neq_I8 || instr->m_opcode == Js::OpCode::Simd128_Neq_U8)
    {
        cmpOpcode = Js::OpCode::PCMPEQW;
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_Neq_I16 || instr->m_opcode == Js::OpCode::Simd128_Neq_U16)
    {
        cmpOpcode = Js::OpCode::PCMPEQB;
    }
    // dst = PCMPEQD src1, src2
    pInstr = IR::Instr::New(cmpOpcode, dst, src1, src2, m_func);
    instr->InsertBefore(pInstr);
    //MakeDstEquSrc1(pInstr);
    Legalize(pInstr);

    // dst = PANDN dst, X86_ALL_NEG_ONES
    pInstr = IR::Instr::New(Js::OpCode::PANDN, dst, dst,  IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), TySimd128I4, m_func), m_func);
    instr->InsertBefore(pInstr);
    //MakeDstEquSrc1(pInstr);
    Legalize(pInstr);

    pInstr = instr->m_prev;
    instr->Remove();

    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerLessThan(IR::Instr* instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_Lt_U4 || instr->m_opcode == Js::OpCode::Simd128_Lt_U8 || instr->m_opcode == Js::OpCode::Simd128_Lt_U16 ||
        instr->m_opcode == Js::OpCode::Simd128_GtEq_U4 || instr->m_opcode == Js::OpCode::Simd128_GtEq_U8 || instr->m_opcode == Js::OpCode::Simd128_GtEq_U16);

    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd() && (dst->IsSimd128B4() || dst->IsSimd128B8() || dst->IsSimd128B16()));
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsRegOpnd() && src2->IsSimd128());

    IR::RegOpnd* tmpa = IR::RegOpnd::New(src1->GetType(), m_func);
    IR::RegOpnd* tmpb = IR::RegOpnd::New(src1->GetType(), m_func);

    IR::MemRefOpnd* signBits = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86DoubleWordSignBitsAddr(), TySimd128I4, m_func);
    IR::RegOpnd * mask = IR::RegOpnd::New(TySimd128I4, m_func);

    Js::OpCode cmpOpcode = Js::OpCode::PCMPGTD;
    if (instr->m_opcode == Js::OpCode::Simd128_Lt_U8 || instr->m_opcode == Js::OpCode::Simd128_GtEq_U8)
    {
        cmpOpcode = Js::OpCode::PCMPGTW;
        signBits = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86WordSignBitsAddr(), TySimd128I4, m_func);
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_Lt_U16 || instr->m_opcode == Js::OpCode::Simd128_GtEq_U16)
    {
        cmpOpcode = Js::OpCode::PCMPGTB;
        signBits = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86ByteSignBitsAddr(), TySimd128I4, m_func);
    }

    // MOVUPS mask, [signBits]
    pInstr = IR::Instr::New(Js::OpCode::MOVUPS, mask, signBits, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // tmpa = PXOR src1, signBits
    pInstr = IR::Instr::New(Js::OpCode::PXOR, tmpa, src1, mask, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // tmpb = PXOR src2, signBits
    pInstr = IR::Instr::New(Js::OpCode::PXOR, tmpb, src2, mask, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // dst = cmpOpCode tmpb, tmpa  (Less than, swapped opnds)
    pInstr = IR::Instr::New(cmpOpcode, dst, tmpb, tmpa, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    if (instr->m_opcode == Js::OpCode::Simd128_GtEq_U4 || instr->m_opcode == Js::OpCode::Simd128_GtEq_U8 || instr->m_opcode == Js::OpCode::Simd128_GtEq_U16)
    {
        // for SIMD unsigned int, greaterThanOrEqual == lessThan + Not
        // dst = PANDN dst, X86_ALL_NEG_ONES
        // MOVUPS mask, [allNegOnes]
        pInstr = IR::Instr::New(Js::OpCode::PANDN, dst, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }

    pInstr = instr->m_prev;
    instr->Remove();

    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerLessThanOrEqual(IR::Instr* instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_LtEq_I4 || instr->m_opcode == Js::OpCode::Simd128_LtEq_I8 || instr->m_opcode == Js::OpCode::Simd128_LtEq_I16 ||
        instr->m_opcode == Js::OpCode::Simd128_LtEq_U4 || instr->m_opcode == Js::OpCode::Simd128_LtEq_U8 || instr->m_opcode == Js::OpCode::Simd128_LtEq_U16 ||
        instr->m_opcode == Js::OpCode::Simd128_Gt_U4 || instr->m_opcode == Js::OpCode::Simd128_Gt_U8 || instr->m_opcode == Js::OpCode::Simd128_Gt_U16);

    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd() && (dst->IsSimd128B4() || dst->IsSimd128B8() || dst->IsSimd128B16()));
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsRegOpnd() && src2->IsSimd128());

    IR::RegOpnd* tmpa = IR::RegOpnd::New(src1->GetType(), m_func);
    IR::RegOpnd* tmpb = IR::RegOpnd::New(src1->GetType(), m_func);

    Js::OpCode cmpOpcode = Js::OpCode::PCMPGTD;
    Js::OpCode eqpOpcode = Js::OpCode::PCMPEQD;
    if (instr->m_opcode == Js::OpCode::Simd128_LtEq_I8 || instr->m_opcode == Js::OpCode::Simd128_LtEq_U8 || instr->m_opcode == Js::OpCode::Simd128_Gt_U8)
    {
        cmpOpcode = Js::OpCode::PCMPGTW;
        eqpOpcode = Js::OpCode::PCMPEQW;
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_LtEq_I16 || instr->m_opcode == Js::OpCode::Simd128_LtEq_U16 || instr->m_opcode == Js::OpCode::Simd128_Gt_U16)
    {
        cmpOpcode = Js::OpCode::PCMPGTB;
        eqpOpcode = Js::OpCode::PCMPEQB;
    }

    if (instr->m_opcode == Js::OpCode::Simd128_LtEq_I4)
    {
        // dst = pcmpgtd src1, src2
        pInstr = IR::Instr::New(Js::OpCode::PCMPGTD, dst, src1, src2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // dst = pandn dst, xmmword ptr[X86_ALL_NEG_ONES]
        pInstr = IR::Instr::New(Js::OpCode::PANDN, dst, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_LtEq_I8 || instr->m_opcode == Js::OpCode::Simd128_LtEq_I16)
    {
        // tmpa = pcmpgtw src2, src1 (src1 < src2?) [pcmpgtb]
        pInstr = IR::Instr::New(cmpOpcode, tmpa, src2, src1, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // tmpb = pcmpeqw src1, src2 [pcmpeqb]
        pInstr = IR::Instr::New(eqpOpcode, tmpb, src1, src2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // dst =  por tmpa, tmpb
        pInstr = IR::Instr::New(Js::OpCode::POR, dst, tmpa, tmpb, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_LtEq_U4 || instr->m_opcode == Js::OpCode::Simd128_LtEq_U8 || instr->m_opcode == Js::OpCode::Simd128_LtEq_U16 ||
        instr->m_opcode == Js::OpCode::Simd128_Gt_U4 || instr->m_opcode == Js::OpCode::Simd128_Gt_U8 || instr->m_opcode == Js::OpCode::Simd128_Gt_U16)
    {
        IR::MemRefOpnd* signBits = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86DoubleWordSignBitsAddr(), TySimd128I4, m_func);
        IR::RegOpnd * mask = IR::RegOpnd::New(TySimd128I4, m_func);
        if (instr->m_opcode == Js::OpCode::Simd128_LtEq_U8 || instr->m_opcode == Js::OpCode::Simd128_Gt_U8)
        {
            signBits = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86WordSignBitsAddr(), TySimd128I4, m_func);
        }
        else if (instr->m_opcode == Js::OpCode::Simd128_LtEq_U16 || instr->m_opcode == Js::OpCode::Simd128_Gt_U16)
        {
            signBits = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86ByteSignBitsAddr(), TySimd128I4, m_func);
        }
        // MOVUPS mask, [signBits]
        pInstr = IR::Instr::New(Js::OpCode::MOVUPS, mask, signBits, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        // tmpa = PXOR src1, mask
        pInstr = IR::Instr::New(Js::OpCode::PXOR, tmpa, src1, mask, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // tmpb = PXOR src2, signBits
        pInstr = IR::Instr::New(Js::OpCode::PXOR, tmpb, src2, mask, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // dst = cmpOpCode tmpb, tmpa
        pInstr = IR::Instr::New(cmpOpcode, dst, tmpb, tmpa, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // tmpa = pcmpeqd tmpa, tmpb
        pInstr = IR::Instr::New(eqpOpcode, tmpa, tmpa, tmpb, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // dst = por dst, tmpa
        pInstr = IR::Instr::New(Js::OpCode::POR, dst, dst, tmpa, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        if (instr->m_opcode == Js::OpCode::Simd128_Gt_U4 || instr->m_opcode == Js::OpCode::Simd128_Gt_U8 || instr->m_opcode == Js::OpCode::Simd128_Gt_U16)
        {   // for SIMD unsigned int, greaterThan == lessThanOrEqual + Not
            // dst = PANDN dst, X86_ALL_NEG_ONES
            pInstr = IR::Instr::New(Js::OpCode::PANDN, dst, dst,  IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), TySimd128I4, m_func), m_func);
            instr->InsertBefore(pInstr);

            Legalize(pInstr);
        }
    }

    pInstr = instr->m_prev;
    instr->Remove();

    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerGreaterThanOrEqual(IR::Instr* instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_GtEq_I4 || instr->m_opcode == Js::OpCode::Simd128_GtEq_I8 || instr->m_opcode == Js::OpCode::Simd128_GtEq_I16);

    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd() && (dst->IsSimd128B4() || dst->IsSimd128B8() || dst->IsSimd128B16()));
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsRegOpnd() && src2->IsSimd128());

    if (instr->m_opcode == Js::OpCode::Simd128_GtEq_I4)
    {
        // dst = pcmpgtd src2, src1
        pInstr = IR::Instr::New(Js::OpCode::PCMPGTD, dst, src2, src1, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // dst = pandn dst, xmmword ptr[X86_ALL_NEG_ONES]
        pInstr = IR::Instr::New(Js::OpCode::PANDN, dst, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesAddr(), TySimd128I4, m_func), m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }
    else if (instr->m_opcode == Js::OpCode::Simd128_GtEq_I8 || instr->m_opcode == Js::OpCode::Simd128_GtEq_I16)
    {
        IR::RegOpnd* tmp1 = IR::RegOpnd::New(src1->GetType(), m_func);
        IR::RegOpnd* tmp2 = IR::RegOpnd::New(src1->GetType(), m_func);

        Js::OpCode cmpOpcode = Js::OpCode::PCMPGTW;
        Js::OpCode eqpOpcode = Js::OpCode::PCMPEQW;
        if (instr->m_opcode == Js::OpCode::Simd128_GtEq_I16)
        {
            cmpOpcode = Js::OpCode::PCMPGTB;
            eqpOpcode = Js::OpCode::PCMPEQB;
        }

        // tmp1 = pcmpgtw src1, src2 [pcmpgtb]
        pInstr = IR::Instr::New(cmpOpcode, tmp1, src1, src2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // tmp2 = pcmpeqw src1, src2 [pcmpeqw]
        pInstr = IR::Instr::New(eqpOpcode, tmp2, src1, src2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);

        // dst  = por     tmp1, tmp2
        pInstr = IR::Instr::New(Js::OpCode::POR, dst, tmp1, tmp2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }

    pInstr = instr->m_prev;
    instr->Remove();

    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerMinMax_F4(IR::Instr* instr)
{
    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd() && dst->IsSimd128());
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    Assert(src2->IsRegOpnd() && src2->IsSimd128());
    Assert(instr->m_opcode == Js::OpCode::Simd128_Min_F4 || instr->m_opcode == Js::OpCode::Simd128_Max_F4);
    IR::RegOpnd* tmp1 = IR::RegOpnd::New(src1->GetType(), m_func);
    IR::RegOpnd* tmp2 = IR::RegOpnd::New(src2->GetType(), m_func);

    if (instr->m_opcode == Js::OpCode::Simd128_Min_F4)
    {
        pInstr = IR::Instr::New(Js::OpCode::MINPS, tmp1, src1, src2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        //
        pInstr = IR::Instr::New(Js::OpCode::MINPS, tmp2, src2, src1, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        //
        pInstr = IR::Instr::New(Js::OpCode::ORPS, dst, tmp1, tmp2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }
    else 
    {

        //This sequence closely mirrors SIMDFloat32x4Operation::OpMax except for
        //the fact that tmp2 (tmpbValue) is reused to reduce the number of registers
        //needed for this sequence. 

        pInstr = IR::Instr::New(Js::OpCode::MAXPS, tmp1, src1, src2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        //
        pInstr = IR::Instr::New(Js::OpCode::MAXPS, tmp2, src2, src1, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        //
        pInstr = IR::Instr::New(Js::OpCode::ANDPS, tmp1, tmp1, tmp2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        //
        pInstr = IR::Instr::New(Js::OpCode::CMPUNORDPS, tmp2, src1, src2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
        //
        pInstr = IR::Instr::New(Js::OpCode::ORPS, dst, tmp1, tmp2, m_func);
        instr->InsertBefore(pInstr);
        Legalize(pInstr);
    }

    pInstr = instr->m_prev;
    instr->Remove();
    return pInstr;

}

IR::Instr* LowererMD::Simd128LowerAnyTrue(IR::Instr* instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_AnyTrue_B4 || instr->m_opcode == Js::OpCode::Simd128_AnyTrue_B8 ||
        instr->m_opcode == Js::OpCode::Simd128_AnyTrue_B16);

    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    Assert(dst->IsRegOpnd() && dst->IsInt32());
    Assert(src1->IsRegOpnd() && src1->IsSimd128());
    // pmovmskb dst, src1
    // neg      dst
    // sbb      dst, dst
    // neg      dst

    // pmovmskb dst, src1
    pInstr = IR::Instr::New(Js::OpCode::PMOVMSKB, dst, src1, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // neg      dst
    pInstr = IR::Instr::New(Js::OpCode::NEG, dst, dst, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // sbb      dst, dst
    pInstr = IR::Instr::New(Js::OpCode::SBB, dst, dst, dst, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // neg      dst
    pInstr = IR::Instr::New(Js::OpCode::NEG, dst, dst, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    pInstr = instr->m_prev;
    instr->Remove();

    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerAllTrue(IR::Instr* instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_AllTrue_B4 || instr->m_opcode == Js::OpCode::Simd128_AllTrue_B8 ||
        instr->m_opcode == Js::OpCode::Simd128_AllTrue_B16);

    IR::Instr *pInstr;
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();

    Assert(dst->IsRegOpnd() && dst->IsInt32());
    Assert(src1->IsRegOpnd() && src1->IsSimd128());

    IR::RegOpnd * tmp = IR::RegOpnd::New(TyInt8, m_func);

    // pmovmskb dst, src1
    pInstr = IR::Instr::New(Js::OpCode::PMOVMSKB, dst, src1, m_func);
    instr->InsertBefore(pInstr);

    // cmp      dst, 0FFFFh
    pInstr = IR::Instr::New(Js::OpCode::CMP, m_func);
    pInstr->SetSrc1(dst);
    pInstr->SetSrc2(IR::IntConstOpnd::New(0x0FFFF, TyInt32, m_func, true));
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // mov     tmp(TyInt8), dst
    pInstr = IR::Instr::New(Js::OpCode::MOV, tmp, dst, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // sete    tmp(TyInt8)
    pInstr = IR::Instr::New(Js::OpCode::SETE, tmp, tmp, m_func);
    instr->InsertBefore(pInstr);
    Legalize(pInstr);

    // movsx dst, dst(TyInt8)
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSX, dst, tmp, m_func));

    pInstr = instr->m_prev;
    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerInt32x4FromFloat32x4(IR::Instr *instr)
{
    IR::Opnd *dst, *src, *tmp, *tmp2, *mask1, *mask2;
    IR::Instr *insertInstr, *pInstr, *newInstr;
    IR::LabelInstr *doneLabel;
    dst = instr->GetDst();
    src = instr->GetSrc1();
    Assert(dst != nullptr && src != nullptr && dst->IsSimd128() && src->IsSimd128());

    // CVTTPS2DQ dst, src
    instr->m_opcode = Js::OpCode::CVTTPS2DQ;
    insertInstr = instr->m_next;
    pInstr = instr->m_prev;
    doneLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    mask1 = IR::RegOpnd::New(TyInt32, m_func);
    mask2 = IR::RegOpnd::New(TyInt32, m_func);

    // bound checks
    // check if any value is potentially out of range (0x80000000 in output)
    // PCMPEQD tmp, dst, X86_NEG_MASK (0x80000000)
    // MOVMSKPS mask1, tmp
    // CMP mask1, 0
    // JNE $doneLabel
    tmp = IR::RegOpnd::New(TySimd128I4, m_func);
    tmp2 = IR::RegOpnd::New(TySimd128I4, m_func);
    newInstr = IR::Instr::New(Js::OpCode::MOVAPS, tmp2, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86NegMaskF4Addr(), TySimd128I4, m_func), m_func);
    insertInstr->InsertBefore(newInstr);
    Legalize(newInstr);
    newInstr = IR::Instr::New(Js::OpCode::PCMPEQD, tmp, dst, tmp2, m_func);
    insertInstr->InsertBefore(newInstr);
    Legalize(newInstr);
    insertInstr->InsertBefore(IR::Instr::New(Js::OpCode::MOVMSKPS, mask1, tmp, m_func));
    newInstr = IR::Instr::New(Js::OpCode::CMP, m_func);
    newInstr->SetSrc1(mask1);
    newInstr->SetSrc2(IR::IntConstOpnd::New(0, TyInt32, m_func));
    insertInstr->InsertBefore(newInstr);
    insertInstr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JEQ, doneLabel, m_func));

    // we have potential out of bound. check bounds
    // MOVAPS tmp2, X86_TWO_31_F4 (0x4f000000)
    // CMPLEPS tmp, tmp2, src
    // MOVMSKPS mask1, tmp
    // MOVAPS tmp2, X86_NEG_TWO_31_F4 (0xcf000000)
    // CMPLTPS tmp, src, tmp2
    // MOVMSKPS mask2, tmp
    // OR mask1, mask1, mask2
    // CMP mask1, 0
    // JNE $doneLabel
    newInstr = IR::Instr::New(Js::OpCode::MOVAPS, tmp2, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86TwoPower31F4Addr(), TySimd128I4, m_func), m_func);
    insertInstr->InsertBefore(newInstr);
    Legalize(newInstr);
    newInstr = IR::Instr::New(Js::OpCode::CMPLEPS, tmp, tmp2, src, m_func);
    insertInstr->InsertBefore(newInstr);
    Legalize(newInstr);
    insertInstr->InsertBefore(IR::Instr::New(Js::OpCode::MOVMSKPS, mask1, tmp, m_func));

    newInstr = IR::Instr::New(Js::OpCode::MOVAPS, tmp2, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86NegTwoPower31F4Addr(), TySimd128I4, m_func), m_func);
    insertInstr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::CMPLTPS, tmp, src, tmp2, m_func);
    insertInstr->InsertBefore(newInstr);
    Legalize(newInstr);

    insertInstr->InsertBefore(IR::Instr::New(Js::OpCode::MOVMSKPS, mask2, tmp, m_func));

    insertInstr->InsertBefore(IR::Instr::New(Js::OpCode::OR, mask1, mask1, mask2, m_func));
    newInstr = IR::Instr::New(Js::OpCode::CMP, m_func);
    newInstr->SetSrc1(mask1);
    newInstr->SetSrc2(IR::IntConstOpnd::New(0, TyInt32, m_func));
    insertInstr->InsertBefore(newInstr);
    insertInstr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JEQ, doneLabel, m_func));

    // throw range error
    m_lowerer->GenerateRuntimeError(insertInstr, JSERR_ArgumentOutOfRange, IR::HelperOp_RuntimeRangeError);

    insertInstr->InsertBefore(doneLabel);

    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerUint32x4FromFloat32x4(IR::Instr *instr)
{
    IR::Opnd *dst, *src, *tmp, *tmp2, *two_31_f4_mask, *two_31_i4_mask, *mask;
    IR::Instr *pInstr, *newInstr;
    IR::LabelInstr *doneLabel, *throwLabel;
    dst = instr->GetDst();
    src = instr->GetSrc1();
    Assert(dst != nullptr && src != nullptr && dst->IsSimd128() && src->IsSimd128());

    doneLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    throwLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
    pInstr = instr->m_prev;

    mask = IR::RegOpnd::New(TyInt32, m_func);
    two_31_f4_mask = IR::RegOpnd::New(TySimd128F4, m_func);
    two_31_i4_mask = IR::RegOpnd::New(TySimd128I4, m_func);
    tmp = IR::RegOpnd::New(TySimd128F4, m_func);
    tmp2 = IR::RegOpnd::New(TySimd128F4, m_func);
    // any lanes <= -1.0 ?
    // CMPLEPS tmp, src, [X86_ALL_FLOAT32_NEG_ONES]
    // MOVMSKPS mask, tmp
    // CMP mask, 0
    // JNE $throwLabel
    newInstr = IR::Instr::New(Js::OpCode::CMPLEPS, tmp, src, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllNegOnesF4Addr(), TySimd128I4, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::MOVMSKPS, mask, tmp, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::CMP, m_func);
    newInstr->SetSrc1(mask);
    newInstr->SetSrc2(IR::IntConstOpnd::New(0, TyInt32, m_func));
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    instr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JNE, throwLabel, m_func));

    // CVTTPS2DQ does a range check over signed range [-2^31, 2^31-1], so will fail to convert values >= 2^31.
    // To fix this, subtract 2^31 from values >= 2^31, do CVTTPS2DQ, then add 2^31 back.
    // MOVAPS       two_31_f4_mask, [X86_TWO_31]
    // CMPLEPS      tmp2, two_31_mask, src
    // ANDPS        two_31_f4_mask, tmp2          // tmp has f32(2^31) for lanes >= 2^31, 0 otherwise
    // SUBPS        tmp2, two_31_f4_mask          // subtract 2^31 from lanes >= 2^31, unchanged otherwise.
    // CVTTPS2DQ    dst, tmp2
    newInstr = IR::Instr::New(Js::OpCode::MOVAPS, two_31_f4_mask, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86TwoPower31F4Addr(), TySimd128F4, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::CMPLEPS, tmp2, two_31_f4_mask, src, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::ANDPS, two_31_f4_mask, two_31_f4_mask, tmp2, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::SUBPS, tmp2, src, two_31_f4_mask, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::CVTTPS2DQ, dst, tmp2, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // check if any value is out of range (i.e. >= 2^31, meaning originally >= 2^32 before value adjustment)
    // PCMPEQD      tmp, dst, [X86_NEG_MASK]
    // MOVMSKPS     mask, tmp
    // CMP          mask, 0
    // JNE          $throwLabel
    newInstr = IR::Instr::New(Js::OpCode::PCMPEQD, tmp, dst, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86NegMaskF4Addr(), TySimd128I4, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::MOVMSKPS, mask, tmp, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::CMP, m_func);
    newInstr->SetSrc1(mask);
    newInstr->SetSrc2(IR::IntConstOpnd::New(0, TyInt32, m_func));
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    instr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JNE, throwLabel, m_func));

    // we pass range checks
    // add i4(2^31) values back to adjusted values.
    // Use first bit from the 2^31 float mask (0x4f000...0 << 1)
    // and AND with 2^31 int mask (0x8000..0) setting first bit to zero if lane hasn't been adjusted
    // MOVAPS       two_31_i4_mask, [X86_TWO_31_I4]
    // PSLLD        two_31_f4_mask, 1
    // ANDPS        two_31_i4_mask, two_31_f4_mask
    // PADDD        dst, dst, two_31_i4_mask
    // JMP          $doneLabel
    newInstr = IR::Instr::New(Js::OpCode::MOVAPS, two_31_i4_mask, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86TwoPower31I4Addr(), TySimd128I4, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::PSLLD, two_31_f4_mask, two_31_f4_mask, IR::IntConstOpnd::New(1, TyInt8, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::ANDPS, two_31_i4_mask, two_31_i4_mask, two_31_f4_mask, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    newInstr = IR::Instr::New(Js::OpCode::PADDD, dst, dst, two_31_i4_mask, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);
    instr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JMP, doneLabel, m_func));

    // throwLabel:
    //      Throw Range Error
    instr->InsertBefore(throwLabel);
    m_lowerer->GenerateRuntimeError(instr, JSERR_ArgumentOutOfRange, IR::HelperOp_RuntimeRangeError);
    // doneLabe:
    instr->InsertBefore(doneLabel);

    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128LowerFloat32x4FromUint32x4(IR::Instr *instr)
{
    IR::Opnd *dst, *src, *tmp, *zero;
    IR::Instr *pInstr, *newInstr;

    dst = instr->GetDst();
    src = instr->GetSrc1();
    Assert(dst != nullptr && src != nullptr && dst->IsSimd128() && src->IsSimd128());

    pInstr = instr->m_prev;

    zero = IR::RegOpnd::New(TySimd128I4, m_func);
    tmp = IR::RegOpnd::New(TySimd128I4, m_func);

    // find unsigned values above 2^31-1. Comparison is signed, so look for values < 0
    // MOVAPS zero, [X86_ALL_ZEROS]
    newInstr = IR::Instr::New(Js::OpCode::MOVAPS, zero, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86AllZerosAddr(), TySimd128I4, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // tmp = PCMPGTD zero, src
    newInstr = IR::Instr::New(Js::OpCode::PCMPGTD, tmp, zero, src, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // temp1 has f32(2^32) for unsigned values above 2^31, 0 otherwise
    // ANDPS tmp, tmp, [X86_TWO_32_F4]
    newInstr = IR::Instr::New(Js::OpCode::ANDPS, tmp, tmp, IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetX86TwoPower32F4Addr(), TySimd128F4, m_func), m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // convert
    // dst = CVTDQ2PS src
    newInstr = IR::Instr::New(Js::OpCode::CVTDQ2PS, dst, src, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    // Add f32(2^32) to negative values
    // ADDPS dst, dst, tmp
    newInstr = IR::Instr::New(Js::OpCode::ADDPS, dst, dst, tmp, m_func);
    instr->InsertBefore(newInstr);
    Legalize(newInstr);

    instr->Remove();
    return pInstr;
}

IR::Instr* LowererMD::Simd128AsmJsLowerLoadElem(IR::Instr *instr)
{
    Assert(instr->m_opcode == Js::OpCode::Simd128_LdArr_I4 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArr_I8 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArr_I16 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArr_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArr_U8 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArr_U16 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArr_F4 ||
        //instr->m_opcode == Js::OpCode::Simd128_LdArr_D2 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArrConst_I4 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArrConst_I8 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArrConst_I16 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArrConst_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArrConst_U8 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArrConst_U16 ||
        instr->m_opcode == Js::OpCode::Simd128_LdArrConst_F4
        //instr->m_opcode == Js::OpCode::Simd128_LdArrConst_D2
        );

    IR::Instr * instrPrev = instr->m_prev;
    IR::RegOpnd * indexOpnd = instr->GetSrc1()->AsIndirOpnd()->GetIndexOpnd();
    IR::RegOpnd * baseOpnd = instr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd();
    IR::Opnd * dst = instr->GetDst();
    IR::Opnd * src1 = instr->GetSrc1();
    IR::Opnd * src2 = instr->GetSrc2();
    ValueType arrType = baseOpnd->GetValueType();
    uint8 dataWidth = instr->dataWidth;

    // Type-specialized.
    Assert(dst->IsSimd128() && src1->IsSimd128() && src2->GetType() == TyUint32);

    IR::Instr * done;
    if (indexOpnd ||  (((uint32)src1->AsIndirOpnd()->GetOffset() + dataWidth) > 0x1000000 /* 16 MB */))
    {
        uint32 bpe = Simd128GetTypedArrBytesPerElem(arrType);
        // bound check and helper
        done = this->lowererMDArch.LowerAsmJsLdElemHelper(instr, true, bpe != dataWidth);
    }
    else
    {
        // Reaching here means:
        // We have a constant index, and either
        // (1) constant heap or (2) variable heap with constant index < 16MB.
        // Case (1) requires static bound check. Case (2) means we are always in bound.

        // this can happen in cases where globopt props a constant access which was not known at bytecodegen time or when heap is non-constant

        if (src2->IsIntConstOpnd() && ((uint32)src1->AsIndirOpnd()->GetOffset() + dataWidth > src2->AsIntConstOpnd()->AsUint32()))
        {
            m_lowerer->GenerateRuntimeError(instr, JSERR_ArgumentOutOfRange, IR::HelperOp_RuntimeRangeError);
            instr->Remove();
            return instrPrev;
        }
        done = instr;
    }

    return Simd128ConvertToLoad(dst, src1, dataWidth, instr);
}

IR::Instr* LowererMD::Simd128LowerLoadElem(IR::Instr *instr)
{
    Assert(!m_func->GetJITFunctionBody()->IsAsmJsMode());

    Assert(
           instr->m_opcode == Js::OpCode::Simd128_LdArr_I4 ||
           instr->m_opcode == Js::OpCode::Simd128_LdArr_I8 ||
           instr->m_opcode == Js::OpCode::Simd128_LdArr_I16 ||
           instr->m_opcode == Js::OpCode::Simd128_LdArr_U4 ||
           instr->m_opcode == Js::OpCode::Simd128_LdArr_U8 ||
           instr->m_opcode == Js::OpCode::Simd128_LdArr_U16 ||
           instr->m_opcode == Js::OpCode::Simd128_LdArr_F4
          );

    IR::Opnd * src = instr->GetSrc1();
    IR::RegOpnd * indexOpnd =src->AsIndirOpnd()->GetIndexOpnd();
    IR::Opnd * dst = instr->GetDst();
    ValueType arrType = src->AsIndirOpnd()->GetBaseOpnd()->GetValueType();

    // If we type-specialized, then array is a definite typed-array.
    Assert(arrType.IsObject() && arrType.IsTypedArray());

    Simd128GenerateUpperBoundCheck(indexOpnd, src->AsIndirOpnd(), arrType, instr);
    Simd128LoadHeadSegment(src->AsIndirOpnd(), arrType, instr);
    return Simd128ConvertToLoad(dst, src, instr->dataWidth, instr, m_lowerer->GetArrayIndirScale(arrType) /* scale factor */);
}

IR::Instr *
LowererMD::Simd128ConvertToLoad(IR::Opnd *dst, IR::Opnd *src, uint8 dataWidth, IR::Instr* instr, byte scaleFactor /* = 0*/)
{
    IR::Instr *newInstr = nullptr;
    IR::Instr * instrPrev = instr->m_prev;

    // Type-specialized.
    Assert(dst && dst->IsSimd128());
    Assert(src->IsIndirOpnd());
    if (scaleFactor > 0)
    {
        // needed only for non-Asmjs code
        Assert(!m_func->GetJITFunctionBody()->IsAsmJsMode());
        src->AsIndirOpnd()->SetScale(scaleFactor);
    }

    switch (dataWidth)
    {
    case 16:
        // MOVUPS dst, src1([arrayBuffer + indexOpnd])
        newInstr = IR::Instr::New(LowererMDArch::GetAssignOp(src->GetType()), dst, src, instr->m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        break;
    case 12:
    {
        IR::RegOpnd *temp = IR::RegOpnd::New(src->GetType(), instr->m_func);

        // MOVSD dst, src1([arrayBuffer + indexOpnd])
        newInstr = IR::Instr::New(Js::OpCode::MOVSD, dst, src, instr->m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);

        // MOVSS temp, src1([arrayBuffer + indexOpnd + 8])
        newInstr = IR::Instr::New(Js::OpCode::MOVSS, temp, src, instr->m_func);
        instr->InsertBefore(newInstr);
        newInstr->GetSrc1()->AsIndirOpnd()->SetOffset(src->AsIndirOpnd()->GetOffset() + 8, true);
        Legalize(newInstr);

        // PSLLDQ temp, 0x08
        instr->InsertBefore(IR::Instr::New(Js::OpCode::PSLLDQ, temp, temp, IR::IntConstOpnd::New(8, TyInt8, instr->m_func, true), instr->m_func));

        // ORPS dst, temp
        newInstr = IR::Instr::New(Js::OpCode::ORPS, dst, dst, temp, instr->m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        break;
    }
    case 8:
        // MOVSD dst, src1([arrayBuffer + indexOpnd])
        newInstr = IR::Instr::New(Js::OpCode::MOVSD, dst, src, instr->m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        break;
    case 4:
        // MOVSS dst, src1([arrayBuffer + indexOpnd])
        newInstr = IR::Instr::New(Js::OpCode::MOVSS, dst, src, instr->m_func);
        instr->InsertBefore(newInstr);
        Legalize(newInstr);
        break;
    default:
        Assume(UNREACHED);
    }

    instr->Remove();
    return instrPrev;
}

IR::Instr*
LowererMD::Simd128AsmJsLowerStoreElem(IR::Instr *instr)
{
    Assert(
        instr->m_opcode == Js::OpCode::Simd128_StArr_I4 ||
        instr->m_opcode == Js::OpCode::Simd128_StArr_I8 ||
        instr->m_opcode == Js::OpCode::Simd128_StArr_I16 ||
        instr->m_opcode == Js::OpCode::Simd128_StArr_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_StArr_U8 ||
        instr->m_opcode == Js::OpCode::Simd128_StArr_U16 ||
        instr->m_opcode == Js::OpCode::Simd128_StArr_F4 ||
        //instr->m_opcode == Js::OpCode::Simd128_StArr_D2 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_I4 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_I8 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_I16 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_U8 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_U16 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_U4 ||
        instr->m_opcode == Js::OpCode::Simd128_StArrConst_F4
        //instr->m_opcode == Js::OpCode::Simd128_StArrConst_D2
        );

    IR::Instr * instrPrev = instr->m_prev;
    IR::RegOpnd * indexOpnd = instr->GetDst()->AsIndirOpnd()->GetIndexOpnd();
    IR::RegOpnd * baseOpnd = instr->GetDst()->AsIndirOpnd()->GetBaseOpnd();
    IR::Opnd * dst = instr->GetDst();
    IR::Opnd * src1 = instr->GetSrc1();
    IR::Opnd * src2 = instr->GetSrc2();
    ValueType arrType = baseOpnd->GetValueType();
    uint8 dataWidth = instr->dataWidth;

    // Type-specialized.
    Assert(dst->IsSimd128() && src1->IsSimd128() && src2->GetType() == TyUint32);

    IR::Instr * done;

    if (indexOpnd || ((uint32)dst->AsIndirOpnd()->GetOffset() + dataWidth > 0x1000000))
    {
        // CMP indexOpnd, src2(arrSize)
        // JA $helper
        // JMP $store
        // $helper:
        // Throw RangeError
        // JMP $done
        // $store:
        // MOV dst([arrayBuffer + indexOpnd]), src1
        // $done:
        uint32 bpe = Simd128GetTypedArrBytesPerElem(arrType);
        done = this->lowererMDArch.LowerAsmJsStElemHelper(instr, true, bpe != dataWidth);
    }
    else
    {
        // we might have a constant index if globopt propped a constant store. we can ahead of time check if it is in-bounds
        if (src2->IsIntConstOpnd() && ((uint32)dst->AsIndirOpnd()->GetOffset() + dataWidth > src2->AsIntConstOpnd()->AsUint32()))
        {
            m_lowerer->GenerateRuntimeError(instr, JSERR_ArgumentOutOfRange, IR::HelperOp_RuntimeRangeError);
            instr->Remove();
            return instrPrev;
        }
        done = instr;
    }

    return Simd128ConvertToStore(dst, src1, dataWidth, instr);
}

IR::Instr*
LowererMD::Simd128LowerStoreElem(IR::Instr *instr)
{
    Assert(!m_func->GetJITFunctionBody()->IsAsmJsMode());
    Assert(
           instr->m_opcode == Js::OpCode::Simd128_StArr_I4 ||
           instr->m_opcode == Js::OpCode::Simd128_StArr_I8 ||
           instr->m_opcode == Js::OpCode::Simd128_StArr_I16 ||
           instr->m_opcode == Js::OpCode::Simd128_StArr_U4 ||
           instr->m_opcode == Js::OpCode::Simd128_StArr_U8 ||
           instr->m_opcode == Js::OpCode::Simd128_StArr_U16 ||
           instr->m_opcode == Js::OpCode::Simd128_StArr_F4
        );

    IR::Opnd * dst = instr->GetDst();
    IR::RegOpnd * indexOpnd = dst->AsIndirOpnd()->GetIndexOpnd();
    IR::Opnd * src1 = instr->GetSrc1();
    uint8 dataWidth = instr->dataWidth;
    ValueType arrType = dst->AsIndirOpnd()->GetBaseOpnd()->GetValueType();

    // If we type-specialized, then array is a definite type-array.
    Assert(arrType.IsObject() && arrType.IsTypedArray());

    Simd128GenerateUpperBoundCheck(indexOpnd, dst->AsIndirOpnd(), arrType, instr);
    Simd128LoadHeadSegment(dst->AsIndirOpnd(), arrType, instr);
    return Simd128ConvertToStore(dst, src1, dataWidth, instr, m_lowerer->GetArrayIndirScale(arrType) /*scale factor*/);
}

IR::Instr *
LowererMD::Simd128ConvertToStore(IR::Opnd *dst, IR::Opnd *src1, uint8 dataWidth, IR::Instr* instr, byte scaleFactor /* = 0 */)
{
    IR::Instr * instrPrev = instr->m_prev;

    Assert(src1 && src1->IsSimd128());
    Assert(dst->IsIndirOpnd());

    if (scaleFactor > 0)
    {
        // needed only for non-Asmjs code
        Assert(!m_func->GetJITFunctionBody()->IsAsmJsMode());
        dst->AsIndirOpnd()->SetScale(scaleFactor);
    }

    switch (dataWidth)
    {
    case 16:
        // MOVUPS dst([arrayBuffer + indexOpnd]), src1
        instr->InsertBefore(IR::Instr::New(LowererMDArch::GetAssignOp(src1->GetType()), dst, src1, instr->m_func));
        break;
    case 12:
    {
               IR::RegOpnd *temp = IR::RegOpnd::New(src1->GetType(), instr->m_func);
               IR::Instr *movss;
               // MOVAPS temp, src
               instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, temp, src1, instr->m_func));
               // MOVSD dst([arrayBuffer + indexOpnd]), temp
               instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSD, dst, temp, instr->m_func));
               // PSRLDQ temp, 0x08
               instr->InsertBefore(IR::Instr::New(Js::OpCode::PSRLDQ, temp, temp, IR::IntConstOpnd::New(8, TyInt8, m_func, true), instr->m_func));
               // MOVSS dst([arrayBuffer + indexOpnd + 8]), temp
               movss = IR::Instr::New(Js::OpCode::MOVSS, dst, temp, instr->m_func);
               instr->InsertBefore(movss);
               movss->GetDst()->AsIndirOpnd()->SetOffset(dst->AsIndirOpnd()->GetOffset() + 8, true);
               break;
    }
    case 8:
        // MOVSD dst([arrayBuffer + indexOpnd]), src1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSD, dst, src1, instr->m_func));
        break;
    case 4:
        // MOVSS dst([arrayBuffer + indexOpnd]), src1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVSS, dst, src1, instr->m_func));
        break;
    default:;
        Assume(UNREACHED);
    }
    instr->Remove();
    return instrPrev;
}

void
LowererMD::Simd128GenerateUpperBoundCheck(IR::RegOpnd *indexOpnd, IR::IndirOpnd *indirOpnd, ValueType arrType, IR::Instr *instr)
{
    Assert(!m_func->GetJITFunctionBody()->IsAsmJsMode());

    IR::ArrayRegOpnd *arrayRegOpnd = indirOpnd->GetBaseOpnd()->AsArrayRegOpnd();
    IR::Opnd* headSegmentLengthOpnd;

    if (arrayRegOpnd->EliminatedUpperBoundCheck())
    {
        // already eliminated or extracted by globOpt (OptArraySrc). Nothing to do.
        return;
    }

    if (arrayRegOpnd->HeadSegmentLengthSym())
    {
        headSegmentLengthOpnd = IR::RegOpnd::New(arrayRegOpnd->HeadSegmentLengthSym(), TyUint32, m_func);
    }
    else
    {
        // (headSegmentLength = [base + offset(length)])
        int lengthOffset;
        lengthOffset = m_lowerer->GetArrayOffsetOfLength(arrType);
        headSegmentLengthOpnd = IR::IndirOpnd::New(arrayRegOpnd, lengthOffset, TyUint32, m_func);
    }

    IR::LabelInstr * skipLabel = Lowerer::InsertLabel(false, instr);
    int32 elemCount = Lowerer::SimdGetElementCountFromBytes(arrayRegOpnd->GetValueType(), instr->dataWidth);
    if (indexOpnd)
    {
        //  MOV tmp, elemCount
        //  ADD tmp, index
        //  CMP tmp, Length  -- upper bound check
        //  JBE  $storeLabel
        //  Throw RuntimeError
        //  skipLabel:
        IR::RegOpnd *tmp = IR::RegOpnd::New(indexOpnd->GetType(), m_func);
        IR::IntConstOpnd *elemCountOpnd = IR::IntConstOpnd::New(elemCount, TyInt8, m_func, true);
        m_lowerer->InsertMove(tmp, elemCountOpnd, skipLabel);
        Lowerer::InsertAdd(false, tmp, tmp, indexOpnd, skipLabel);
        m_lowerer->InsertCompareBranch(tmp, headSegmentLengthOpnd, Js::OpCode::BrLe_A, true, skipLabel, skipLabel);
    }
    else
    {
        // CMP Length, (offset + elemCount)
        // JA $storeLabel
        int32 offset = indirOpnd->GetOffset();
        int32 index = offset + elemCount;
        m_lowerer->InsertCompareBranch(headSegmentLengthOpnd, IR::IntConstOpnd::New(index, TyInt32, m_func, true), Js::OpCode::BrLe_A, true, skipLabel, skipLabel);
    }
    m_lowerer->GenerateRuntimeError(skipLabel, JSERR_ArgumentOutOfRange, IR::HelperOp_RuntimeRangeError);
    return;
}

void
LowererMD::Simd128LoadHeadSegment(IR::IndirOpnd *indirOpnd, ValueType arrType, IR::Instr *instr)
{

    // For non-asm.js we check if headSeg symbol exists, else load it.
    IR::ArrayRegOpnd *arrayRegOpnd = indirOpnd->GetBaseOpnd()->AsArrayRegOpnd();
    IR::RegOpnd *headSegmentOpnd;

    if (arrayRegOpnd->HeadSegmentSym())
    {
        headSegmentOpnd = IR::RegOpnd::New(arrayRegOpnd->HeadSegmentSym(), TyMachPtr, m_func);
    }
    else
    {
        //  MOV headSegment, [base + offset(head)]
        int32 headOffset = m_lowerer->GetArrayOffsetOfHeadSegment(arrType);
        IR::IndirOpnd * newIndirOpnd = IR::IndirOpnd::New(arrayRegOpnd, headOffset, TyMachPtr, this->m_func);
        headSegmentOpnd = IR::RegOpnd::New(TyMachPtr, this->m_func);
        m_lowerer->InsertMove(headSegmentOpnd, newIndirOpnd, instr);
    }

    // change base to be the head segment instead of the array object
    indirOpnd->SetBaseOpnd(headSegmentOpnd);
}

// Builds args list <dst, src1, src2, src3 ..>
SList<IR::Opnd*> * LowererMD::Simd128GetExtendedArgs(IR::Instr *instr)
{
    SList<IR::Opnd*> * args = JitAnew(m_lowerer->m_alloc, SList<IR::Opnd*>, m_lowerer->m_alloc);
    IR::Instr *pInstr = instr;
    IR::Opnd *dst, *src1, *src2;

    dst = src1 = src2 = nullptr;

    if (pInstr->GetDst())
    {
        dst = pInstr->UnlinkDst();
    }

    src1 = pInstr->UnlinkSrc1();
    Assert(src1->GetStackSym()->IsSingleDef());

    pInstr = src1->GetStackSym()->GetInstrDef();

    while (pInstr && pInstr->m_opcode == Js::OpCode::ExtendArg_A)
    {
        Assert(pInstr->GetSrc1());
        src1 = pInstr->GetSrc1()->Copy(this->m_func);
        if (src1->IsRegOpnd())
        {
            this->m_lowerer->addToLiveOnBackEdgeSyms->Set(src1->AsRegOpnd()->m_sym->m_id);
        }
        args->Push(src1);

        if (pInstr->GetSrc2())
        {
            src2 = pInstr->GetSrc2();
            Assert(src2->GetStackSym()->IsSingleDef());
            pInstr = src2->GetStackSym()->GetInstrDef();
        }
        else
        {
            pInstr = nullptr;
        }

    }
    args->Push(dst);
    Assert(args->Count() > 3);
    return args;
}

IR::Opnd*
LowererMD::EnregisterIntConst(IR::Instr* instr, IR::Opnd *constOpnd, IRType type /* = TyInt32*/)
{
    IRType constType = constOpnd->GetType();
    if (!IRType_IsNativeInt(constType))
    {
        // not int opnd, nothing to do
        return constOpnd;
    }

    Assert(type == TyInt32 || type == TyInt16 || type == TyInt8);
    Assert(constType == TyInt32 || constType == TyInt16 || constType == TyInt8);
    if (constOpnd->IsRegOpnd())
    {
        // already a register, just cast
        constOpnd->SetType(type);
        return constOpnd;
    }

    // en-register
    IR::RegOpnd *tempReg = IR::RegOpnd::New(type, m_func);

    // MOV tempReg, constOpnd
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOV, tempReg, constOpnd, m_func));
    return tempReg;
}

void LowererMD::Simd128InitOpcodeMap()
{
    m_simd128OpCodesMap = JitAnewArrayZ(m_lowerer->m_alloc, Js::OpCode, Js::Simd128OpcodeCount());

    // All simd ops should be contiguous for this mapping to work
    Assert(Js::OpCode::Simd128_End + (Js::OpCode) 1 == Js::OpCode::Simd128_Start_Extend);
    //SET_SIMDOPCODE(Simd128_FromFloat64x2_I4     , CVTTPD2DQ);
    //SET_SIMDOPCODE(Simd128_FromFloat64x2Bits_I4 , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromFloat32x4Bits_I4 , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt16x8Bits_I4   , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt8x16Bits_I4   , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint32x4Bits_I4  , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint16x8Bits_I4  , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint8x16Bits_I4  , MOVAPS);
    SET_SIMDOPCODE(Simd128_Add_I4               , PADDD);
    SET_SIMDOPCODE(Simd128_Sub_I4               , PSUBD);
    SET_SIMDOPCODE(Simd128_Lt_I4                , PCMPGTD);
    SET_SIMDOPCODE(Simd128_Gt_I4                , PCMPGTD);
    SET_SIMDOPCODE(Simd128_Eq_I4                , PCMPEQD);
    SET_SIMDOPCODE(Simd128_And_I4               , PAND);
    SET_SIMDOPCODE(Simd128_Or_I4                , POR);
    SET_SIMDOPCODE(Simd128_Xor_I4               , PXOR);
    SET_SIMDOPCODE(Simd128_Not_I4               , XORPS);

    SET_SIMDOPCODE(Simd128_FromFloat32x4Bits_I8    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt32x4Bits_I8      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt8x16Bits_I8      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint32x4Bits_I8     , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint16x8Bits_I8     , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint8x16Bits_I8     , MOVAPS);
    SET_SIMDOPCODE(Simd128_Or_I16               , POR);
    SET_SIMDOPCODE(Simd128_Xor_I16              , PXOR);
    SET_SIMDOPCODE(Simd128_Not_I16              , XORPS);
    SET_SIMDOPCODE(Simd128_And_I16              , PAND);
    SET_SIMDOPCODE(Simd128_Add_I16              , PADDB);
    SET_SIMDOPCODE(Simd128_Sub_I16              , PSUBB);
    SET_SIMDOPCODE(Simd128_Lt_I16               , PCMPGTB);
    SET_SIMDOPCODE(Simd128_Gt_I16               , PCMPGTB);
    SET_SIMDOPCODE(Simd128_Eq_I16               , PCMPEQB);
    SET_SIMDOPCODE(Simd128_FromFloat32x4Bits_I16, MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt32x4Bits_I16  , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt16x8Bits_I16      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint32x4Bits_I16     , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint16x8Bits_I16     , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint8x16Bits_I16     , MOVAPS);

    SET_SIMDOPCODE(Simd128_FromFloat32x4Bits_U4    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt32x4Bits_U4      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt16x8Bits_U4      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt8x16Bits_U4      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint16x8Bits_U4     , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint8x16Bits_U4     , MOVAPS);

    SET_SIMDOPCODE(Simd128_FromFloat32x4Bits_U8    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt32x4Bits_U8      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt16x8Bits_U8      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt8x16Bits_U8      , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint32x4Bits_U8     , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint8x16Bits_U8     , MOVAPS);

    SET_SIMDOPCODE(Simd128_FromFloat32x4Bits_U16  , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt32x4Bits_U16    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt16x8Bits_U16    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt8x16Bits_U16    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint32x4Bits_U16   , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint16x8Bits_U16   , MOVAPS);

    //SET_SIMDOPCODE(Simd128_FromFloat64x2_F4      , CVTPD2PS);
    //SET_SIMDOPCODE(Simd128_FromFloat64x2Bits_F4  , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt32x4_F4        , CVTDQ2PS);
    SET_SIMDOPCODE(Simd128_FromInt32x4Bits_F4    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt16x8Bits_F4    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt8x16Bits_F4    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint32x4Bits_F4    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint16x8Bits_F4    , MOVAPS);
    SET_SIMDOPCODE(Simd128_FromUint8x16Bits_F4    , MOVAPS);
    SET_SIMDOPCODE(Simd128_Abs_F4                , ANDPS);
    SET_SIMDOPCODE(Simd128_Neg_F4                , XORPS);
    SET_SIMDOPCODE(Simd128_Add_F4                , ADDPS);
    SET_SIMDOPCODE(Simd128_Sub_F4                , SUBPS);
    SET_SIMDOPCODE(Simd128_Mul_F4                , MULPS);
    SET_SIMDOPCODE(Simd128_Div_F4                , DIVPS);
    SET_SIMDOPCODE(Simd128_Sqrt_F4               , SQRTPS);
    SET_SIMDOPCODE(Simd128_Lt_F4                 , CMPLTPS); // CMPLTPS
    SET_SIMDOPCODE(Simd128_LtEq_F4               , CMPLEPS); // CMPLEPS
    SET_SIMDOPCODE(Simd128_Eq_F4                 , CMPEQPS); // CMPEQPS
    SET_SIMDOPCODE(Simd128_Neq_F4                , CMPNEQPS); // CMPNEQPS
    SET_SIMDOPCODE(Simd128_Gt_F4                 , CMPLTPS); // CMPLTPS (swap srcs)
    SET_SIMDOPCODE(Simd128_GtEq_F4               , CMPLEPS); // CMPLEPS (swap srcs)


#if 0
    SET_SIMDOPCODE(Simd128_FromFloat32x4_D2, CVTPS2PD);
    SET_SIMDOPCODE(Simd128_FromFloat32x4Bits_D2, MOVAPS);
    SET_SIMDOPCODE(Simd128_FromInt32x4_D2, CVTDQ2PD);
    SET_SIMDOPCODE(Simd128_FromInt32x4Bits_D2, MOVAPS);
    SET_SIMDOPCODE(Simd128_Neg_D2, XORPS);
    SET_SIMDOPCODE(Simd128_Add_D2, ADDPD);
    SET_SIMDOPCODE(Simd128_Abs_D2, ANDPD);
    SET_SIMDOPCODE(Simd128_Sub_D2, SUBPD);
    SET_SIMDOPCODE(Simd128_Mul_D2, MULPD);
    SET_SIMDOPCODE(Simd128_Div_D2, DIVPD);
    SET_SIMDOPCODE(Simd128_Min_D2, MINPD);
    SET_SIMDOPCODE(Simd128_Max_D2, MAXPD);
    SET_SIMDOPCODE(Simd128_Sqrt_D2, SQRTPD);
    SET_SIMDOPCODE(Simd128_Lt_D2, CMPLTPD); // CMPLTPD
    SET_SIMDOPCODE(Simd128_LtEq_D2, CMPLEPD); // CMPLEPD
    SET_SIMDOPCODE(Simd128_Eq_D2, CMPEQPD); // CMPEQPD
    SET_SIMDOPCODE(Simd128_Neq_D2, CMPNEQPD); // CMPNEQPD
    SET_SIMDOPCODE(Simd128_Gt_D2, CMPLTPD); // CMPLTPD (swap srcs)
    SET_SIMDOPCODE(Simd128_GtEq_D2, CMPLEPD); // CMPLEPD (swap srcs)
#endif // 0

    SET_SIMDOPCODE(Simd128_And_I8               , PAND);
    SET_SIMDOPCODE(Simd128_Or_I8                , POR);
    SET_SIMDOPCODE(Simd128_Xor_I8               , XORPS);
    SET_SIMDOPCODE(Simd128_Not_I8               , XORPS);
    SET_SIMDOPCODE(Simd128_Add_I8               , PADDW);
    SET_SIMDOPCODE(Simd128_Sub_I8               , PSUBW);
    SET_SIMDOPCODE(Simd128_Mul_I8               , PMULLW);
    SET_SIMDOPCODE(Simd128_Eq_I8                , PCMPEQW);
    SET_SIMDOPCODE(Simd128_Lt_I8                , PCMPGTW); // (swap srcs)
    SET_SIMDOPCODE(Simd128_Gt_I8                , PCMPGTW);
    SET_SIMDOPCODE(Simd128_AddSaturate_I8       , PADDSW);
    SET_SIMDOPCODE(Simd128_SubSaturate_I8       , PSUBSW);

    SET_SIMDOPCODE(Simd128_AddSaturate_I16      , PADDSB);
    SET_SIMDOPCODE(Simd128_SubSaturate_I16      , PSUBSB);

    SET_SIMDOPCODE(Simd128_And_U4               , PAND);
    SET_SIMDOPCODE(Simd128_Or_U4                , POR);
    SET_SIMDOPCODE(Simd128_Xor_U4               , XORPS);
    SET_SIMDOPCODE(Simd128_Not_U4               , XORPS);
    SET_SIMDOPCODE(Simd128_Add_U4               , PADDD);
    SET_SIMDOPCODE(Simd128_Sub_U4               , PSUBD);
    SET_SIMDOPCODE(Simd128_Eq_U4                , PCMPEQD); // same as int32x4.equal

    SET_SIMDOPCODE(Simd128_And_U8               , PAND);
    SET_SIMDOPCODE(Simd128_Or_U8                , POR);
    SET_SIMDOPCODE(Simd128_Xor_U8               , XORPS);
    SET_SIMDOPCODE(Simd128_Not_U8               , XORPS);
    SET_SIMDOPCODE(Simd128_Add_U8               , PADDW);
    SET_SIMDOPCODE(Simd128_Sub_U8               , PSUBW);
    SET_SIMDOPCODE(Simd128_Mul_U8               , PMULLW);
    SET_SIMDOPCODE(Simd128_Eq_U8                , PCMPEQW); // same as int16X8.equal
    SET_SIMDOPCODE(Simd128_AddSaturate_U8       , PADDUSW);
    SET_SIMDOPCODE(Simd128_SubSaturate_U8       , PSUBUSW);

    SET_SIMDOPCODE(Simd128_And_U16              , PAND);
    SET_SIMDOPCODE(Simd128_Or_U16               , POR);
    SET_SIMDOPCODE(Simd128_Xor_U16              , XORPS);
    SET_SIMDOPCODE(Simd128_Not_U16              , XORPS);
    SET_SIMDOPCODE(Simd128_Add_U16              , PADDB);
    SET_SIMDOPCODE(Simd128_Sub_U16              , PSUBB);

    SET_SIMDOPCODE(Simd128_Eq_U16               , PCMPEQB); // same as int8x16.equal
    SET_SIMDOPCODE(Simd128_AddSaturate_U16      , PADDUSB);
    SET_SIMDOPCODE(Simd128_SubSaturate_U16      , PSUBUSB);

    SET_SIMDOPCODE(Simd128_And_B4               , PAND);
    SET_SIMDOPCODE(Simd128_Or_B4                , POR);
    SET_SIMDOPCODE(Simd128_Xor_B4               , XORPS);
    SET_SIMDOPCODE(Simd128_Not_B4               , XORPS);

    SET_SIMDOPCODE(Simd128_And_B8               , PAND);
    SET_SIMDOPCODE(Simd128_Or_B8                , POR);
    SET_SIMDOPCODE(Simd128_Xor_B8               , XORPS);
    SET_SIMDOPCODE(Simd128_Not_B8               , XORPS);

    SET_SIMDOPCODE(Simd128_And_B16              , PAND);
    SET_SIMDOPCODE(Simd128_Or_B16               , POR);
    SET_SIMDOPCODE(Simd128_Xor_B16              , XORPS);
    SET_SIMDOPCODE(Simd128_Not_B16              , XORPS);
}
#undef SIMD_SETOPCODE
#undef SIMD_GETOPCODE

// FromVar
void
LowererMD::GenerateCheckedSimdLoad(IR::Instr * instr)
{
    Assert(instr->m_opcode == Js::OpCode::FromVar);
    Assert(instr->GetSrc1()->GetType() == TyVar);
    Assert(IRType_IsSimd128(instr->GetDst()->GetType()));

    bool checkRequired = instr->HasBailOutInfo();
    IR::LabelInstr * labelHelper = nullptr, * labelDone = nullptr;
    IR::Instr * insertInstr = instr, * newInstr;
    IR::RegOpnd * src = instr->GetSrc1()->AsRegOpnd(), * dst = instr->GetDst()->AsRegOpnd();
    Assert(!checkRequired || instr->GetBailOutKind() == IR::BailOutSimd128F4Only || instr->GetBailOutKind() == IR::BailOutSimd128I4Only);

    if (checkRequired)
    {
        labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
        labelDone = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
        instr->InsertBefore(labelHelper);
        instr->InsertAfter(labelDone);
        insertInstr = labelHelper;

        GenerateObjectTest(instr->GetSrc1(), insertInstr, labelHelper);

        newInstr = IR::Instr::New(Js::OpCode::CMP, instr->m_func);
        newInstr->SetSrc1(IR::IndirOpnd::New(instr->GetSrc1()->AsRegOpnd(), 0, TyMachPtr, instr->m_func));
        newInstr->SetSrc2(m_lowerer->LoadVTableValueOpnd(instr, dst->GetType() == TySimd128F4 ? VTableValue::VtableSimd128F4 : VTableValue::VtableSimd128I4));
        insertInstr->InsertBefore(newInstr);
        Legalize(newInstr);
        insertInstr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JNE, labelHelper, this->m_func));
        instr->UnlinkSrc1();
        instr->UnlinkDst();
        this->m_lowerer->GenerateBailOut(instr);

    }
    size_t valueOffset = dst->GetType() == TySimd128F4 ? Js::JavascriptSIMDFloat32x4::GetOffsetOfValue() : Js::JavascriptSIMDInt32x4::GetOffsetOfValue();
    Assert(valueOffset < INT_MAX);
    newInstr = IR::Instr::New(Js::OpCode::MOVUPS, dst, IR::IndirOpnd::New(src, static_cast<int>(valueOffset), dst->GetType(), this->m_func), this->m_func);
    insertInstr->InsertBefore(newInstr);

    insertInstr->InsertBefore(IR::BranchInstr::New(Js::OpCode::JMP, labelDone, this->m_func));
    // FromVar is converted to BailOut call. Don't remove.
}

// ToVar
void LowererMD::GenerateSimdStore(IR::Instr * instr)
{
    IR::RegOpnd *dst, *src;
    IRType type;
    dst = instr->GetDst()->AsRegOpnd();
    src = instr->GetSrc1()->AsRegOpnd();
    type = src->GetType();

    this->m_lowerer->LoadScriptContext(instr);
    IR::Instr * instrCall = IR::Instr::New(Js::OpCode::CALL, instr->GetDst(),
        IR::HelperCallOpnd::New(type == TySimd128F4 ? IR::HelperAllocUninitializedSimdF4 : IR::HelperAllocUninitializedSimdI4, this->m_func), this->m_func);
    instr->InsertBefore(instrCall);
    this->lowererMDArch.LowerCall(instrCall, 0);

    IR::Opnd * valDst;
    if (type == TySimd128F4)
    {
        valDst = IR::IndirOpnd::New(dst, (int32)Js::JavascriptSIMDFloat32x4::GetOffsetOfValue(), TySimd128F4, this->m_func);
    }
    else
    {
        valDst = IR::IndirOpnd::New(dst, (int32)Js::JavascriptSIMDInt32x4::GetOffsetOfValue(), TySimd128I4, this->m_func);
    }

    instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVUPS, valDst, src, this->m_func));
    instr->Remove();
}

void LowererMD::CheckShuffleLanes_4(uint8 lanes[], uint8 lanesSrc[], uint *fromSrc1, uint *fromSrc2)
{
    Assert(lanes);
    Assert(lanesSrc);
    Assert(fromSrc1 && fromSrc2);
    *fromSrc1 = 0;
    *fromSrc2 = 0;
    for (uint i = 0; i < 4; i++)
    {
        if (lanes[i] >= 0 && lanes[i] < 4)
        {
            (*fromSrc1)++;
            lanesSrc[i] = 1;
        }
        else if (lanes[i] >= 4 && lanes[i] < 8)
        {
            (*fromSrc2)++;
            lanesSrc[i] = 2;
        }
        else
        {
            Assert(UNREACHED);
        }
    }
}

void LowererMD::InsertShufps(uint8 lanes[], IR::Opnd *dst, IR::Opnd *src1, IR::Opnd *src2, IR::Instr *instr)
{
    int8 shufMask;
    uint8 normLanes[4];
    IR::RegOpnd * tmp = IR::RegOpnd::New(TySimd128I4, m_func);
    for (uint i = 0; i < 4; i++)
    {
        normLanes[i] = (lanes[i] >= 4) ? (lanes[i] - 4) : lanes[i];
    }
    shufMask = (int8)((normLanes[3] << 6) | (normLanes[2] << 4) | (normLanes[1] << 2) | normLanes[0]);
    // ToDo: Move this to legalization code
    if (dst->IsEqual(src1))
    {
        // instruction already legal
        instr->InsertBefore(IR::Instr::New(Js::OpCode::SHUFPS, dst, src2, IR::IntConstOpnd::New((IntConstType)shufMask, TyInt8, m_func, true), m_func));
    }
    else if (dst->IsEqual(src2))
    {

        // MOVAPS tmp, dst
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, tmp, dst, m_func));
        // MOVAPS dst, src1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func));
        // SHUF dst, tmp, imm8
        instr->InsertBefore(IR::Instr::New(Js::OpCode::SHUFPS, dst, tmp, IR::IntConstOpnd::New((IntConstType)shufMask, TyInt8, m_func, true), m_func));
    }
    else
    {
        // MOVAPS dst, src1
        instr->InsertBefore(IR::Instr::New(Js::OpCode::MOVAPS, dst, src1, m_func));
        // SHUF dst, src2, imm8
        instr->InsertBefore(IR::Instr::New(Js::OpCode::SHUFPS, dst, src2, IR::IntConstOpnd::New((IntConstType)shufMask, TyInt8, m_func, true), m_func));
    }
}

BYTE LowererMD::Simd128GetTypedArrBytesPerElem(ValueType arrType)
{
    return  (1 << Lowerer::GetArrayIndirScale(arrType));
}

#endif