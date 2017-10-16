//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

// Build static array of legal instruction forms
#undef MACRO
#define MACRO(name, jnLayout, attrib, byte2, legalforms, opbyte, ...) legalforms,
static const LegalInstrForms _InstrForms[] =
{
#include "MdOpCodes.h"
};

// Local class for tracking the opcodes used by an expanded LDIMM
class LdImmOpcode
{
public:
    void Set(Js::OpCode opcode, IntConstType immed, int shift)
    {
        m_opcode = opcode;
        m_immed = immed << shift;
    }

    Js::OpCode m_opcode;
    IntConstType m_immed;
};


static LegalForms LegalDstForms(IR::Instr * instr)
{
    Assert(instr->IsLowered());
    return _InstrForms[instr->m_opcode - (Js::OpCode::MDStart+1)].dst;
}

static LegalForms LegalSrcForms(IR::Instr * instr, uint opndNum)
{
    Assert(instr->IsLowered());
    return _InstrForms[instr->m_opcode - (Js::OpCode::MDStart+1)].src[opndNum-1];
}

void LegalizeMD::LegalizeInstr(IR::Instr * instr, bool fPostRegAlloc)
{
    if (!instr->IsLowered())
    {
        AssertMsg(UNREACHED, "Unlowered instruction in m.d. legalizer");
        return;
    }

    // Convert CMP/CMN/TST opcodes into more primitive forms
    switch (instr->m_opcode)
    {
    case Js::OpCode::CMP:
        instr->SetDst(IR::RegOpnd::New(NULL, RegZR, instr->GetSrc1()->GetType() , instr->m_func));
        instr->m_opcode = Js::OpCode::SUBS;
        break;

    case Js::OpCode::CMN:
        instr->SetDst(IR::RegOpnd::New(NULL, RegZR, instr->GetSrc1()->GetType(), instr->m_func));
        instr->m_opcode = Js::OpCode::ADDS;
        break;

    case Js::OpCode::TST:
        instr->SetDst(IR::RegOpnd::New(NULL, RegZR, instr->GetSrc1()->GetType(), instr->m_func));
        instr->m_opcode = Js::OpCode::ANDS;
        break;
    }

    LegalizeDst(instr, fPostRegAlloc);
    LegalizeSrc(instr, instr->GetSrc1(), 1, fPostRegAlloc);
    LegalizeSrc(instr, instr->GetSrc2(), 2, fPostRegAlloc);
}

void LegalizeMD::LegalizeRegOpnd(IR::Instr* instr, IR::Opnd* opnd)
{
    // Arm64 does not support 8 or 16 bit register usage, so promote anything smaller than 32 bits up to 32 bits.
    // Arm64 does not support 32 bit float register usage, so promote 32 bit floats to 64 bit.
    IRType ty = opnd->GetType();
    switch(ty)
    {
    case TyInt8:
    case TyInt16:
        ty = TyInt32;
        break;
    
    case TyUint8:
    case TyUint16:
        ty = TyUint32;
        break;

    case TyFloat32:
        ty = TyFloat64;
        break;
    }

    if (ty != opnd->GetType())
    { 
        // UseWithNewType will make a copy if the register is already in use. We know it is in use because it is used in this instruction, and we want to reuse this operand rather than making a copy 
        // so UnUse it before calling UseWithNewType.
        opnd->UnUse();
        opnd->UseWithNewType(ty, instr->m_func);
    }
}

void LegalizeMD::LegalizeDst(IR::Instr * instr, bool fPostRegAlloc)
{
    LegalForms forms = LegalDstForms(instr);

    IR::Opnd * opnd = instr->GetDst();
    if (opnd == NULL)
    {
#ifdef DBG
        // No legalization possible, just report error.
        if (forms != 0)
        {
            IllegalInstr(instr, _u("Expected dst opnd"));
        }
#endif
        return;
    }

    switch (opnd->GetKind())
    {
    case IR::OpndKindReg:
#ifdef DBG
        if (!(forms & L_RegMask))
        {
            IllegalInstr(instr, _u("Unexpected reg dst"));
        }
#endif
        LegalizeRegOpnd(instr, opnd);
        break;

    case IR::OpndKindMemRef:
    {
        // MemRefOpnd is a deference of the memory location.
        // So extract the location, load it to register, replace the MemRefOpnd with an IndirOpnd taking the
        // register as base, and fall through to legalize the IndirOpnd.
        intptr_t memLoc = opnd->AsMemRefOpnd()->GetMemLoc();
        IR::RegOpnd *newReg = IR::RegOpnd::New(TyMachPtr, instr->m_func);
        if (fPostRegAlloc)
        {
            newReg->SetReg(SCRATCH_REG);
        }
        IR::Instr *newInstr = IR::Instr::New(Js::OpCode::LDIMM, newReg,
            IR::AddrOpnd::New(memLoc, opnd->AsMemRefOpnd()->GetAddrKind(), instr->m_func, true), instr->m_func);
        instr->InsertBefore(newInstr);
        LegalizeMD::LegalizeInstr(newInstr, fPostRegAlloc);
        IR::IndirOpnd *indirOpnd = IR::IndirOpnd::New(newReg, 0, opnd->GetType(), instr->m_func);
        opnd = instr->ReplaceDst(indirOpnd);
    }
    // FALL THROUGH
    case IR::OpndKindIndir:
        if (!(forms & L_IndirMask))
        {
            instr = LegalizeStore(instr, forms, fPostRegAlloc);
            forms = LegalDstForms(instr);
        }
        LegalizeIndirOffset(instr, opnd->AsIndirOpnd(), forms, fPostRegAlloc);
        break;

    case IR::OpndKindSym:
        if (!(forms & L_SymMask))
        {
            instr = LegalizeStore(instr, forms, fPostRegAlloc);
            forms = LegalDstForms(instr);
        }

        if (fPostRegAlloc)
        {
            // In order to legalize SymOffset we need to know final argument area, which is only available after lowerer.
            // So, don't legalize sym offset here, but it will be done as part of register allocator.
            LegalizeSymOffset(instr, opnd->AsSymOpnd(), forms, fPostRegAlloc);
        }
        break;

    default:
        AssertMsg(UNREACHED, "Unexpected dst opnd kind");
        break;
    }
}

IR::Instr * LegalizeMD::LegalizeStore(IR::Instr *instr, LegalForms forms, bool fPostRegAlloc)
{
    if (LowererMD::IsAssign(instr) && instr->GetSrc1()->IsRegOpnd())
    {
        // We can just change this to a store in place.
        instr->m_opcode = LowererMD::GetStoreOp(instr->GetSrc1()->GetType());
    }
    else
    {
        // Sink the mem opnd. The caller will verify the offset.
        // We don't expect to hit this point after register allocation, because we
        // can't guarantee that the instruction will be legal.
        Assert(!fPostRegAlloc);
        instr = instr->SinkDst(LowererMD::GetStoreOp(instr->GetDst()->GetType()), RegNOREG);
    }

    return instr;
}

void LegalizeMD::LegalizeSrc(IR::Instr * instr, IR::Opnd * opnd, uint opndNum, bool fPostRegAlloc)
{
    LegalForms forms = LegalSrcForms(instr, opndNum);
    if (opnd == NULL)
    {
#ifdef DBG
        // No legalization possible, just report error.
        if (forms != 0)
        {
            IllegalInstr(instr, _u("Expected src %d opnd"), opndNum);
        }
#endif
        return;
    }

    switch (opnd->GetKind())
    {
    case IR::OpndKindReg:
#ifdef DBG
        if (!(forms & L_RegMask))
        {
            IllegalInstr(instr, _u("Unexpected reg as src%d opnd"), opndNum);
        }
#endif
        LegalizeRegOpnd(instr, opnd);
        break;

    case IR::OpndKindAddr:
    case IR::OpndKindHelperCall:
    case IR::OpndKindIntConst:
        LegalizeImmed(instr, opnd, opndNum, opnd->GetImmediateValue(instr->m_func), forms, fPostRegAlloc);
        break;

    case IR::OpndKindLabel:
        LegalizeLabelOpnd(instr, opnd, opndNum, fPostRegAlloc);
        break;

    case IR::OpndKindMemRef:
    {
        // MemRefOpnd is a deference of the memory location.
        // So extract the location, load it to register, replace the MemRefOpnd with an IndirOpnd taking the
        // register as base, and fall through to legalize the IndirOpnd.
        intptr_t memLoc = opnd->AsMemRefOpnd()->GetMemLoc();
        IR::RegOpnd *newReg = IR::RegOpnd::New(TyMachPtr, instr->m_func);
        if (fPostRegAlloc)
        {
            newReg->SetReg(SCRATCH_REG);
        }
        IR::Instr *newInstr = IR::Instr::New(Js::OpCode::LDIMM, newReg, IR::AddrOpnd::New(memLoc, IR::AddrOpndKindDynamicMisc, instr->m_func), instr->m_func);
        instr->InsertBefore(newInstr);
        LegalizeMD::LegalizeInstr(newInstr, fPostRegAlloc);
        IR::IndirOpnd *indirOpnd = IR::IndirOpnd::New(newReg, 0, opnd->GetType(), instr->m_func);
        if (opndNum == 1)
        {
            opnd = instr->ReplaceSrc1(indirOpnd);
        }
        else
        {
            opnd = instr->ReplaceSrc2(indirOpnd);
        }
    }
    // FALL THROUGH
    case IR::OpndKindIndir:
        if (!(forms & L_IndirMask))
        {
            instr = LegalizeLoad(instr, opndNum, forms, fPostRegAlloc);
            forms = LegalSrcForms(instr, 1);
        }
        LegalizeIndirOffset(instr, opnd->AsIndirOpnd(), forms, fPostRegAlloc);
        break;

    case IR::OpndKindSym:
        if (!(forms & L_SymMask))
        {
            instr = LegalizeLoad(instr, opndNum, forms, fPostRegAlloc);
            forms = LegalSrcForms(instr, 1);
        }

        if (fPostRegAlloc)
        {
            // In order to legalize SymOffset we need to know final argument area, which is only available after lowerer.
            // So, don't legalize sym offset here, but it will be done as part of register allocator.
            LegalizeSymOffset(instr, opnd->AsSymOpnd(), forms, fPostRegAlloc);
        }
        break;

    default:
        AssertMsg(UNREACHED, "Unexpected src opnd kind");
        break;
    }
}

IR::Instr * LegalizeMD::LegalizeLoad(IR::Instr *instr, uint opndNum, LegalForms forms, bool fPostRegAlloc)
{
    IR::Instr* legalized = instr;
    if (LowererMD::IsAssign(instr) && instr->GetDst()->IsRegOpnd())
    {
        // We can just change this to a load in place.
        instr->m_opcode = LowererMD::GetLoadOp(instr->GetDst()->GetType());
    }
    else
    {
        // Hoist the memory opnd. The caller will verify the offset.
        if (opndNum == 1)
        {
            AssertMsg(!fPostRegAlloc || instr->GetSrc1()->GetType() == TyMachReg, "Post RegAlloc other types disallowed");
            legalized = instr->HoistSrc1(LowererMD::GetLoadOp(instr->GetSrc1()->GetType()), fPostRegAlloc ? SCRATCH_REG : RegNOREG);
            LegalizeRegOpnd(instr, instr->GetSrc1());
        }
        else
        {
            AssertMsg(!fPostRegAlloc || instr->GetSrc2()->GetType() == TyMachReg, "Post RegAlloc other types disallowed");
            legalized = instr->HoistSrc2(LowererMD::GetLoadOp(instr->GetSrc2()->GetType()), fPostRegAlloc ? SCRATCH_REG : RegNOREG);
            LegalizeRegOpnd(instr, instr->GetSrc2());
        }
    }

    return legalized;
}

void LegalizeMD::LegalizeIndirOffset(IR::Instr * instr, IR::IndirOpnd * indirOpnd, LegalForms forms, bool fPostRegAlloc)
{
    int32 offset = indirOpnd->GetOffset();

    // Can't have both offset and index, so hoist the offset and try again.
    if (indirOpnd->GetIndexOpnd() != NULL && offset != 0)
    {
        IR::Instr *addInstr = instr->HoistIndirOffset(indirOpnd, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
        LegalizeMD::LegalizeInstr(addInstr, fPostRegAlloc);
        return;
    }

    // Determine scale factor for scaled offsets
    int size = indirOpnd->GetSize();
    int32 scaledOffset = offset / size;

    // Either scaled unsigned 12-bit offset, or unscaled signed 9-bit offset
    if (forms & L_IndirSU12I9)
    {
        if (offset == (scaledOffset * size) && IS_CONST_UINT12(scaledOffset))
        {
            return;
        }
        if (IS_CONST_INT9(offset))
        {
            return;
        }
    }

    // scaled signed 9-bit offset
    if (forms & L_IndirSI7)
    {
        if (IS_CONST_INT7(scaledOffset))
        {
            return;
        }
    }

    // Offset is too large, so hoist it and replace it with an index
    IR::Instr *addInstr = instr->HoistIndirOffset(indirOpnd, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
    LegalizeMD::LegalizeInstr(addInstr, fPostRegAlloc);
}

void LegalizeMD::LegalizeSymOffset(
    IR::Instr * instr,
    IR::SymOpnd * symOpnd,
    LegalForms forms,
    bool fPostRegAlloc)
{
    AssertMsg(fPostRegAlloc, "LegalizeMD::LegalizeSymOffset can (and will) be called as part of register allocation. Can't call it as part of lowerer, as final argument area is not available yet.");

    RegNum baseReg;
    int32 offset;

    if (!symOpnd->m_sym->IsStackSym())
    {
        return;
    }

    EncoderMD::BaseAndOffsetFromSym(symOpnd, &baseReg, &offset, instr->m_func->GetTopFunc());

    // Determine scale factor for scaled offsets
    int scale = (symOpnd->GetType() == TyFloat64) ? 3 : 2;
    int32 scaledOffset = offset >> scale;
    
    // Either scaled unsigned 12-bit offset, or unscaled signed 9-bit offset
    if (forms & L_SymSU12I9)
    {
        if (offset == (scaledOffset << scale) && IS_CONST_UINT12(scaledOffset))
        {
            return;
        }
        if (IS_CONST_INT9(offset))
        {
            return;
        }
    }

    // scaled signed 9-bit offset
    if (forms & L_SymSI7)
    {
        if (IS_CONST_INT7(scaledOffset))
        {
            return;
        }
    }

    IR::Instr * newInstr;
    if (instr->m_opcode == Js::OpCode::LEA)
    {
        instr->m_opcode = Js::OpCode::ADD;
        instr->ReplaceSrc1(IR::RegOpnd::New(NULL, baseReg, TyMachPtr, instr->m_func));
        instr->SetSrc2(IR::IntConstOpnd::New(offset, TyMachReg, instr->m_func));
        newInstr = instr->HoistSrc2(Js::OpCode::LDIMM, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
        LegalizeMD::LegalizeInstr(newInstr, fPostRegAlloc);
        LegalizeMD::LegalizeInstr(instr, fPostRegAlloc);
    }
    else
    {
        newInstr = instr->HoistSymOffset(symOpnd, baseReg, offset, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
        LegalizeMD::LegalizeInstr(newInstr, fPostRegAlloc);
    }
}

void LegalizeMD::LegalizeImmed(
    IR::Instr * instr,
    IR::Opnd * opnd,
    uint opndNum,
    IntConstType immed,
    LegalForms forms,
    bool fPostRegAlloc)
{
    int size = instr->GetDst()->GetSize();
    if (!(((forms & L_ImmLog12) && EncoderMD::CanEncodeLogicalConst(immed, size)) ||
         ((forms & L_ImmU12) && IS_CONST_UINT12(immed)) ||
         ((forms & L_ImmU12Lsl12) && IS_CONST_UINT12LSL12(immed)) ||
         ((forms & L_ImmU6) && IS_CONST_UINT6(immed)) ||
         ((forms & L_ImmU16) && IS_CONST_UINT16(immed)) ||
         ((forms & L_ImmU6U6) && IS_CONST_UINT6UINT6(immed))))
    {
        if (instr->m_opcode != Js::OpCode::LDIMM)
        {
            instr = LegalizeMD::GenerateLDIMM(instr, opndNum, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
        }

        if (fPostRegAlloc)
        {
            LegalizeMD::LegalizeLDIMM(instr, immed);
        }
    }
}

void LegalizeMD::LegalizeLabelOpnd(
    IR::Instr * instr,
    IR::Opnd * opnd,
    uint opndNum,
    bool fPostRegAlloc)
{
    if (instr->m_opcode != Js::OpCode::LDIMM)
    {
        instr = LegalizeMD::GenerateLDIMM(instr, opndNum, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
    }
    if (fPostRegAlloc)
    {
        LegalizeMD::LegalizeLdLabel(instr, opnd);
    }
}

IR::Instr * LegalizeMD::GenerateLDIMM(IR::Instr * instr, uint opndNum, RegNum scratchReg)
{
    if (LowererMD::IsAssign(instr) && instr->GetDst()->IsRegOpnd())
    {
        instr->m_opcode = Js::OpCode::LDIMM;
    }
    else
    {
        if (opndNum == 1)
        {
            instr = instr->HoistSrc1(Js::OpCode::LDIMM, scratchReg);
        }
        else
        {
            instr = instr->HoistSrc2(Js::OpCode::LDIMM, scratchReg);
        }
    }

    return instr;
}

void LegalizeMD::LegalizeLDIMM(IR::Instr * instr, IntConstType immed)
{
    // In case of inlined entry instruction, we don't know the offset till the encoding phase
    if (!instr->isInlineeEntryInstr)
    {
        // Short-circuit simple 16-bit immediates
        if ((immed & 0xffff) == immed || (immed & 0xffff0000) == immed || (immed & 0xffff00000000ll) == immed || (immed & 0xffff000000000000ll) == immed)
        {
            instr->m_opcode = Js::OpCode::MOVZ;
            return;
        }

        // Short-circuit simple inverted 16-bit immediates
        IntConstType invImmed = ~immed;
        if ((invImmed & 0xffff) == invImmed || (invImmed & 0xffff0000) == invImmed || (invImmed & 0xffff00000000ll) == invImmed || (invImmed & 0xffff000000000000ll) == invImmed)
        {
            instr->ReplaceSrc1(IR::IntConstOpnd::New(invImmed, TyInt64, instr->m_func));
            instr->m_opcode = Js::OpCode::MOVN;
            return;
        }

        // Short-circuit simple inverted 16-bit immediates that can be implicitly truncated to 32 bits
        IntConstType invImmed32 = ~immed & 0xffffffffull;
        if ((invImmed32 & 0xffff) == invImmed32 || (invImmed32 & 0xffff0000) == invImmed32)
        {
            instr->GetDst()->SetType(TyInt32);
            IR::IntConstOpnd *src1 = IR::IntConstOpnd::New(invImmed32, TyInt64, instr->m_func);
            instr->ReplaceSrc1(src1);
            instr->m_opcode = Js::OpCode::MOVN;
            return;
        }

        // Short-circuit 32-bit logical constants
        if (EncoderMD::CanEncodeLogicalConst(immed, 4))
        {
            instr->GetDst()->SetType(TyInt32);
            instr->SetSrc2(instr->GetSrc1());
            instr->ReplaceSrc1(IR::RegOpnd::New(NULL, RegZR, TyInt32, instr->m_func));
            instr->m_opcode = Js::OpCode::ORR;
            return;
        }

        // Short-circuit 64-bit logical constants
        if (EncoderMD::CanEncodeLogicalConst(immed, 8))
        {
            instr->SetSrc2(instr->GetSrc1());
            instr->ReplaceSrc1(IR::RegOpnd::New(NULL, RegZR, TyInt64, instr->m_func));
            instr->m_opcode = Js::OpCode::ORR;
            return;
        }

        // Determine how many 16-bit chunks are all-0 versus all-1
        int numZeros = 0;
        int numOnes = 0;
        for (int wordNum = 0; wordNum < 4; wordNum++)
        {
            ULONG curWord = (immed >> (16 * wordNum)) & 0xffff;
            if (curWord == 0)
            {
                numZeros++;
            }
            else if (curWord == 0xffff)
            {
                numOnes++;
            }
        }

        // Determine whether to obfuscate
        bool fDontEncode = Security::DontEncode(instr->GetSrc1());

        // Determine whether the initial opcode will be a MOVZ or a MOVN
        ULONG wordMask = (numOnes > numZeros) ? 0xffff : 0x0000;
        ULONG wordXor = wordMask;
        Js::OpCode curOpcode = (wordMask == 0xffff) ? Js::OpCode::MOVN : Js::OpCode::MOVZ;

        // Build a theoretical list of opcodes
        LdImmOpcode opcodeList[4];
        int opcodeListIndex = 0;
        for (int wordNum = 0; wordNum < 4; wordNum++)
        {
            ULONG curWord = (immed >> (16 * wordNum)) & 0xffff;
            if (curWord != wordMask)
            {
                opcodeList[opcodeListIndex++].Set(curOpcode, curWord ^ wordXor, 16 * wordNum);
                curOpcode = Js::OpCode::MOVK;
                wordXor = 0;
            }
        }

        // Insert extra opcodes as needed
        for (int opNum = 0; opNum < opcodeListIndex - 1; opNum++)
        {
            IR::IntConstOpnd *src1 = IR::IntConstOpnd::New(opcodeList[opNum].m_immed, TyInt64, instr->m_func);
            IR::Instr * instrMov = IR::Instr::New(opcodeList[opNum].m_opcode, instr->GetDst(), src1, instr->m_func);
            instr->InsertBefore(instrMov);
        }

        // Replace the LDIMM with the final opcode
        IR::IntConstOpnd *src1 = IR::IntConstOpnd::New(opcodeList[opcodeListIndex - 1].m_immed, TyInt64, instr->m_func);
        instr->ReplaceSrc1(src1);
        instr->m_opcode = opcodeList[opcodeListIndex - 1].m_opcode;

        if (!fDontEncode)
        {
            // ARM64_WORKITEM: Hook this up
//            LegalizeMD::ObfuscateLDIMM(instrMov, instr);
        }
    }
    else
    {
        // ARM64_WORKITEM: This needs to be understood better
        Assert(Security::DontEncode(instr->GetSrc1()));
        Assert(false);
/*      IR::LabelInstr *label = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func, false);
        instr->InsertBefore(label);
        Assert((immed & 0x0000000F) == immed);
        label->SetOffset(immed);

        IR::LabelOpnd *target = IR::LabelOpnd::New(label, instr->m_func);

        IR::Instr * instrMov = IR::Instr::New(Js::OpCode::MOVZ, instr->GetDst(), target, instr->m_func);
        instr->InsertBefore(instrMov);

        instr->ReplaceSrc1(target);
        instr->m_opcode = Js::OpCode::MOVK64;

        label->isInlineeEntryInstr = true;
        instr->isInlineeEntryInstr = false;*/
    }
}

void LegalizeMD::ObfuscateLDIMM(IR::Instr * instrMov, IR::Instr * instrMovt)
{
    // Are security measures disabled?

    if (CONFIG_ISENABLED(Js::DebugFlag) ||
        CONFIG_ISENABLED(Js::BenchmarkFlag) ||
        PHASE_OFF(Js::EncodeConstantsPhase, instrMov->m_func->GetTopFunc())
        )
    {
        return;
    }

    UINT_PTR rand = Math::Rand();

    // Use this random value as follows:
    // bits 0-3: reg to use in pre-LDIMM instr
    // bits 4: do/don't emit pre-LDIMM instr
    // bits 5-6:  emit and/or/add/mov as pre-LDIMM instr
    // Similarly for bits 7-13 (mid-LDIMM) and 14-20 (post-LDIMM)

    RegNum targetReg = instrMov->GetDst()->AsRegOpnd()->GetReg();

    LegalizeMD::EmitRandomNopBefore(instrMov, rand, targetReg);
    LegalizeMD::EmitRandomNopBefore(instrMovt, rand >> 7, targetReg);
    LegalizeMD::EmitRandomNopBefore(instrMovt->m_next, rand >> 14, targetReg);
}

void LegalizeMD::EmitRandomNopBefore(IR::Instr *insertInstr, UINT_PTR rand, RegNum targetReg)
{
    // bits 0-3: reg to use in pre-LDIMM instr
    // bits 4: do/don't emit pre-LDIMM instr
    // bits 5-6: emit and/or/add/mov as pre-LDIMM instr

    if (!(rand & (1 << 4)) && !PHASE_FORCE(Js::EncodeConstantsPhase, insertInstr->m_func->GetTopFunc()))
    {
        return;
    }

    IR::Instr * instr;
    IR::RegOpnd * opnd1;
    IR::Opnd * opnd2 = NULL;
    Js::OpCode op = Js::OpCode::InvalidOpCode;

    RegNum regNum = (RegNum)((rand & ((1 << 4) - 1)) + RegR0);

    opnd1 = IR::RegOpnd::New(NULL, regNum, TyMachReg, insertInstr->m_func);

    if (regNum == RegSP || regNum == targetReg) //skip sp & the target reg
    {
        // ORR pc,pc,0 has unpredicted behavior.
        // AND sp,sp,sp has unpredicted behavior.
        // We avoid target reg to avoid pipeline stalls.
        // Less likely target reg will be RegR12 as we insert nops only for user defined constants and
        // RegR12 is mostly used for temporary data such as legalizer post regalloc.
        opnd1->SetReg(RegR12);
    }

    switch ((rand >> 5) & 3)
    {
    case 0:
        op = Js::OpCode::AND;
        opnd2 = opnd1;
        break;
    case 1:
        op = Js::OpCode::ORR;
        opnd2 = IR::IntConstOpnd::New(0, TyMachReg, insertInstr->m_func);
        break;
    case 2:
        op = Js::OpCode::ADD;
        opnd2 = IR::IntConstOpnd::New(0, TyMachReg, insertInstr->m_func);
        break;
    case 3:
        op = Js::OpCode::MOV;
        break;
    }

    instr = IR::Instr::New(op, opnd1, opnd1, insertInstr->m_func);
    if (opnd2)
    {
        instr->SetSrc2(opnd2);
    }
    insertInstr->InsertBefore(instr);
}

void LegalizeMD::LegalizeLdLabel(IR::Instr * instr, IR::Opnd * opnd)
{
    // ARM64_WORKITEM
    __debugbreak();
#if 0

    Assert(instr->m_opcode == Js::OpCode::LDIMM);
    Assert(opnd->IsLabelOpnd());

    IR::Instr * instrMov = IR::Instr::New(Js::OpCode::MOVW, instr->GetDst(), opnd, instr->m_func);
    instr->InsertBefore(instrMov);

    instr->m_opcode = Js::OpCode::MOVT;
#endif
}

bool LegalizeMD::LegalizeDirectBranch(IR::BranchInstr *branchInstr, uint32 branchOffset)
{
    Assert(branchInstr->IsBranchInstr());

    uint32 labelOffset = branchInstr->GetTarget()->GetOffset();
    Assert(labelOffset); //Label offset must be set.

    int32 offset = labelOffset - branchOffset;
    //We should never run out of 24 bits which corresponds to +-16MB of code size.
    AssertMsg(IS_CONST_INT24(offset >> 1), "Cannot encode more that 16 MB offset");

    if (LowererMD::IsUnconditionalBranch(branchInstr))
    {
        return false;
    }

    if (IS_CONST_INT21(offset))
    {
        return false;
    }

    // Convert a conditional branch which can only be +-1MB to unconditional branch which is +-16MB
    // Convert beq Label (where Label is long jump) to something like this
    //          bne Fallback
    //          b Label
    // Fallback:

    IR::LabelInstr *doneLabelInstr = IR::LabelInstr::New(Js::OpCode::Label, branchInstr->m_func, false);
    IR::BranchInstr *newBranchInstr = IR::BranchInstr::New(branchInstr->m_opcode, doneLabelInstr, branchInstr->m_func);
    LowererMD::InvertBranch(newBranchInstr);

    branchInstr->InsertBefore(newBranchInstr);
    branchInstr->InsertAfter(doneLabelInstr);
    branchInstr->m_opcode = Js::OpCode::B;
    return true;
}

void LegalizeMD::LegalizeIndirOpndForVFP(IR::Instr* insertInstr, IR::IndirOpnd *indirOpnd, bool fPostRegAlloc)
{
    IR::RegOpnd *baseOpnd = indirOpnd->GetBaseOpnd();
    int32 offset = indirOpnd->GetOffset();

    IR::RegOpnd *indexOpnd = indirOpnd->UnlinkIndexOpnd(); //Clears index operand
    byte scale = indirOpnd->GetScale();
    IR::Instr *instr = NULL;

    if (indexOpnd)
    {
        if (scale > 0)
        {
            // There is no support for ADD instruction with barrel shifter in encoder, hence add an explicit instruction to left shift the index operand
            // Reason is this requires 4 operand in IR and there is no support for this yet.
            // If we encounter more such scenarios, its better to solve the root cause.
            // Also VSTR & VLDR don't take index operand as parameter
            IR::RegOpnd* newIndexOpnd = IR::RegOpnd::New(indexOpnd->GetType(), insertInstr->m_func);
            instr = IR::Instr::New(Js::OpCode::LSL, newIndexOpnd, indexOpnd,
                                   IR::IntConstOpnd::New(scale, TyMachReg, insertInstr->m_func), insertInstr->m_func);
            insertInstr->InsertBefore(instr);
            indirOpnd->SetScale(0); //Clears scale
            indexOpnd = newIndexOpnd;
        }

        insertInstr->HoistIndirIndexOpndAsAdd(indirOpnd, baseOpnd, indexOpnd, fPostRegAlloc? SCRATCH_REG : RegNOREG);
    }

    if (IS_CONST_UINT10((offset < 0? -offset: offset)))
    {
        return;
    }
    IR::Instr* instrAdd = insertInstr->HoistIndirOffsetAsAdd(indirOpnd, indirOpnd->GetBaseOpnd(), offset, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
    LegalizeMD::LegalizeInstr(instrAdd, fPostRegAlloc);
}


#ifdef DBG

void LegalizeMD::IllegalInstr(IR::Instr * instr, const char16 * msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);
    Output::Print(_u("Illegal instruction: "));
    instr->Dump();
    Output::Print(msg, argptr);
    Assert(UNREACHED);
}

#endif
