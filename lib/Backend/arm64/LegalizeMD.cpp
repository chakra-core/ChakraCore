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
        m_immed = immed;
        m_shift = shift;
    }

    Js::OpCode m_opcode;
    int m_shift;
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
        IR::Instr * newInstr = instr->SinkDst(LowererMD::GetStoreOp(instr->GetDst()->GetType()), RegNOREG);
        LegalizeMD::LegalizeRegOpnd(instr, instr->GetDst());
        instr = newInstr;
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
    if (LowererMD::IsAssign(instr) && instr->GetDst()->IsRegOpnd())
    {
        // We can just change this to a load in place.
        instr->m_opcode = LowererMD::GetLoadOp(instr->GetDst()->GetType());
    }
    else
    {
        // Hoist the memory opnd. The caller will verify the offset.
        IR::Opnd* src = (opndNum == 1) ? instr->GetSrc1() : instr->GetSrc2();
        AssertMsg(!fPostRegAlloc || src->GetType() == TyMachReg, "Post RegAlloc other types disallowed");
        instr = GenerateHoistSrc(instr, opndNum, LowererMD::GetLoadOp(src->GetType()), fPostRegAlloc ? SCRATCH_REG : RegNOREG, fPostRegAlloc);
    }

    if (instr->m_opcode == Js::OpCode::LDR && instr->GetSrc1()->IsSigned())
    {
        instr->m_opcode = Js::OpCode::LDRS;
    }

    return instr;
}

void LegalizeMD::LegalizeIndirOffset(IR::Instr * instr, IR::IndirOpnd * indirOpnd, LegalForms forms, bool fPostRegAlloc)
{
    // For LEA, we have special handling of indiropnds
    auto correctSize = [](IR::Instr* instr, IR::IndirOpnd* indirOpnd)
    {
        // If the base and index operands on an LEA are different sizes, grow the smaller one,
        // matching the larger size but keeping the correct signedness for the type
        IR::RegOpnd* baseOpnd = indirOpnd->GetBaseOpnd();
        IR::RegOpnd* indexOpnd = indirOpnd->GetIndexOpnd();
        if (   baseOpnd != nullptr
            && indexOpnd != nullptr
            && baseOpnd->GetSize() != indexOpnd->GetSize()
            )
        {
            if (baseOpnd->GetSize() < indexOpnd->GetSize())
            {
                IRType largerType = indexOpnd->GetType();
                Assert(IRType_IsSignedInt(largerType) || IRType_IsUnsignedInt(largerType));
                IRType sourceType = baseOpnd->GetType();
                IRType targetType = IRType_IsSignedInt(sourceType) ? IRType_EnsureSigned(largerType) : IRType_EnsureUnsigned(largerType);
                IR::Instr* movInstr = Lowerer::InsertMove(baseOpnd->UseWithNewType(targetType, instr->m_func), baseOpnd, instr, false);
                indirOpnd->SetBaseOpnd(movInstr->GetDst()->AsRegOpnd());
            }
            else
            {
                Assert(indexOpnd->GetSize() < baseOpnd->GetSize());
                IRType largerType = baseOpnd->GetType();
                Assert(IRType_IsSignedInt(largerType) || IRType_IsUnsignedInt(largerType));
                IRType sourceType = indexOpnd->GetType();
                IRType targetType = IRType_IsSignedInt(sourceType) ? IRType_EnsureSigned(largerType) : IRType_EnsureUnsigned(largerType);
                IR::Instr* movInstr = Lowerer::InsertMove(indexOpnd->UseWithNewType(targetType, instr->m_func), indexOpnd, instr, false);
                indirOpnd->SetIndexOpnd(movInstr->GetDst()->AsRegOpnd());
            }
        }
    };

    int32 offset = indirOpnd->GetOffset();

    if (indirOpnd->IsFloat())
    {
        IR::RegOpnd *baseOpnd = indirOpnd->GetBaseOpnd();
        IR::RegOpnd *indexOpnd = indirOpnd->UnlinkIndexOpnd(); //Clears index operand
        byte scale = indirOpnd->GetScale();

        if (indexOpnd)
        {
            if (scale > 0)
            {
                // There is no support for ADD instruction with barrel shifter in encoder, hence add an explicit instruction to left shift the index operand
                // Reason is this requires 4 operand in IR and there is no support for this yet.
                // If we encounter more such scenarios, its better to solve the root cause.
                // Also VSTR & VLDR don't take index operand as parameter
                IR::RegOpnd* newIndexOpnd = IR::RegOpnd::New(indexOpnd->GetType(), instr->m_func);
                IR::Instr* newInstr = IR::Instr::New(Js::OpCode::LSL, newIndexOpnd, indexOpnd,
                    IR::IntConstOpnd::New(scale, TyMachReg, instr->m_func), instr->m_func);
                instr->InsertBefore(newInstr);
                indirOpnd->SetScale(0); //Clears scale
                indexOpnd = newIndexOpnd;
            }

            IR::Instr * instrAdd = Lowerer::HoistIndirIndexOpndAsAdd(instr, indirOpnd, baseOpnd, indexOpnd, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
            LegalizeMD::LegalizeInstr(instrAdd, fPostRegAlloc);
        }
    }
    else if (indirOpnd->GetIndexOpnd() != nullptr && offset != 0)
    {
        // Can't have both offset and index, so hoist the offset and try again.
        IR::Instr *addInstr = Lowerer::HoistIndirOffset(instr, indirOpnd, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
        LegalizeMD::LegalizeInstr(addInstr, fPostRegAlloc);
        if (instr->m_opcode == Js::OpCode::LEA)
        {
            AssertOrFailFastMsg(indirOpnd->GetBaseOpnd() != nullptr && indirOpnd->GetBaseOpnd()->GetSize() == TySize[TyMachPtr], "Base operand of LEA must have pointer-width!");
            correctSize(instr, indirOpnd);
        }
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
    if (forms & L_IndirU12Lsl12)
    {
        // Only one opcode with this form right now
        Assert(instr->m_opcode == Js::OpCode::LEA);
        AssertOrFailFastMsg(indirOpnd->GetBaseOpnd() != nullptr && indirOpnd->GetBaseOpnd()->GetSize() == TySize[TyMachPtr], "Base operand of LEA must have pointer-width!");
        if (offset != 0)
        {
            // Should have already handled this case
            Assert(indirOpnd->GetIndexOpnd() == nullptr);
            if (IS_CONST_UINT12(offset) || IS_CONST_UINT12LSL12(offset))
            {
                return;
            }
        }
        else
        {
            if (indirOpnd->GetIndexOpnd() != nullptr)
            {
                correctSize(instr, indirOpnd);
            }
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
    IR::Instr *addInstr = Lowerer::HoistIndirOffset(instr, indirOpnd, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
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

    // This is so far used for LEA, where the offset ends up needing to be a valid ADD immediate
    if (forms & L_SymU12Lsl12)
    {
        if (IS_CONST_00000FFF(offset) || IS_CONST_00FFF000(offset))
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
        newInstr = Lowerer::HoistSymOffset(instr, symOpnd, baseReg, offset, fPostRegAlloc ? SCRATCH_REG : RegNOREG);
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
            instr = LegalizeMD::GenerateLDIMM(instr, opndNum, fPostRegAlloc ? SCRATCH_REG : RegNOREG, fPostRegAlloc);
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
        instr = LegalizeMD::GenerateLDIMM(instr, opndNum, fPostRegAlloc ? SCRATCH_REG : RegNOREG, fPostRegAlloc);
    }
    if (fPostRegAlloc)
    {
        LegalizeMD::LegalizeLdLabel(instr, opnd);
    }
}

IR::Instr * LegalizeMD::GenerateHoistSrc(IR::Instr * instr, uint opndNum, Js::OpCode op, RegNum scratchReg, bool fPostRegAlloc)
{
    IR::Instr * newInstr;
    if (opndNum == 1)
    {
        newInstr = instr->HoistSrc1(op, scratchReg);
        LegalizeMD::LegalizeRegOpnd(instr, instr->GetSrc1());
    }
    else
    {
        newInstr = instr->HoistSrc2(op, scratchReg);
        LegalizeMD::LegalizeRegOpnd(instr, instr->GetSrc2());
    }
    return newInstr;
}

IR::Instr * LegalizeMD::GenerateLDIMM(IR::Instr * instr, uint opndNum, RegNum scratchReg, bool fPostRegAlloc)
{
    if (LowererMD::IsAssign(instr) && instr->GetDst()->IsRegOpnd())
    {
        instr->m_opcode = Js::OpCode::LDIMM;
    }
    else
    {
        instr = GenerateHoistSrc(instr, opndNum, Js::OpCode::LDIMM, scratchReg, fPostRegAlloc);
    }

    return instr;
}

void LegalizeMD::LegalizeLDIMM(IR::Instr * instr, IntConstType immed)
{
    // In case of inlined entry instruction, we don't know the offset till the encoding phase
    if (!instr->isInlineeEntryInstr)
    {

        // If the source is a tagged int, and the dest is int32 or uint32, untag the value so that it fits in 32 bits.
        if (instr->GetDst()->IsIntegral32() && instr->GetSrc1()->IsTaggedInt())
        {
            immed = (int32)immed;
            instr->ReplaceSrc1(IR::IntConstOpnd::New(immed, instr->GetDst()->GetType(), instr->m_func));
        }

        // Short-circuit simple 16-bit immediates
        if ((immed & 0xffff) == immed || (immed & 0xffff0000) == immed || (immed & 0xffff00000000ll) == immed || (immed & 0xffff000000000000ll) == immed)
        {
            instr->m_opcode = Js::OpCode::MOVZ;
            uint32 shift = ShiftTo16((UIntConstType*)&immed);
            instr->ReplaceSrc1(IR::IntConstOpnd::New(immed, TyUint16, instr->m_func));
            instr->SetSrc2(IR::IntConstOpnd::New(shift, TyUint8, instr->m_func));
            return;
        }

        // Short-circuit simple inverted 16-bit immediates
        IntConstType invImmed = ~immed;
        if ((invImmed & 0xffff) == invImmed || (invImmed & 0xffff0000) == invImmed || (invImmed & 0xffff00000000ll) == invImmed || (invImmed & 0xffff000000000000ll) == invImmed)
        {
            instr->m_opcode = Js::OpCode::MOVN;
            immed = invImmed;
            uint32 shift = ShiftTo16((UIntConstType*)&immed);
            instr->ReplaceSrc1(IR::IntConstOpnd::New(immed, TyUint16, instr->m_func));
            instr->SetSrc2(IR::IntConstOpnd::New(shift, TyUint8, instr->m_func));
            return;
        }

        // Short-circuit simple inverted 16-bit immediates that can be implicitly truncated to 32 bits
        if (immed == (uint32)immed)
        {
            IntConstType invImmed32 = ~immed & 0xffffffffull;
            if ((invImmed32 & 0xffff) == invImmed32 || (invImmed32 & 0xffff0000) == invImmed32)
            {
                instr->GetDst()->SetType(TyInt32);
                uint32 shift = ShiftTo16((UIntConstType*)&invImmed32);
                IR::IntConstOpnd *src1 = IR::IntConstOpnd::New(invImmed32 & 0xFFFF, TyInt16, instr->m_func);
                IR::IntConstOpnd *src2 = IR::IntConstOpnd::New(shift, TyUint8, instr->m_func);
                instr->ReplaceSrc1(src1);
                instr->SetSrc2(src2);
                instr->m_opcode = Js::OpCode::MOVN;
                return;
            }
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
        if (instr->GetDst()->GetSize() == 8 && EncoderMD::CanEncodeLogicalConst(immed, 8))
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
            IR::IntConstOpnd *src1 = IR::IntConstOpnd::New(opcodeList[opNum].m_immed, TyInt16, instr->m_func);
            IR::IntConstOpnd *src2 = IR::IntConstOpnd::New(opcodeList[opNum].m_shift, TyUint8, instr->m_func);
            IR::Instr * instrMov = IR::Instr::New(opcodeList[opNum].m_opcode, instr->GetDst(), src1, src2, instr->m_func);
            instr->InsertBefore(instrMov);
        }

        // Replace the LDIMM with the final opcode
        IR::IntConstOpnd *src1 = IR::IntConstOpnd::New(opcodeList[opcodeListIndex - 1].m_immed, TyInt16, instr->m_func);
        IR::IntConstOpnd *src2 = IR::IntConstOpnd::New(opcodeList[opcodeListIndex - 1].m_shift, TyUint8, instr->m_func);
        instr->ReplaceSrc1(src1);
        instr->SetSrc2(src2);
        instr->m_opcode = opcodeList[opcodeListIndex - 1].m_opcode;

        if (!fDontEncode)
        {
            // ARM64_WORKITEM: Hook this up
//            LegalizeMD::ObfuscateLDIMM(instrMov, instr);
        }
    }
    else
    {
        // Since we don't know the value yet, we're going to handle it when we do
        // This is done by having the load be from a label operand, which is later
        // changed such that its offset is the correct value to ldimm

        // InlineeCallInfo is encoded as ((offset into function) << 4) | (argCount & 0xF).
        // This will fit into 32 bits as long as the function has less than 2^26 instructions, which should be always.

        // The assembly generated becomes something like
        // Label (offset:fake)
        // MOVZ DST, Label
        // MOVK DST, Label <- was the LDIMM

        Assert(Security::DontEncode(instr->GetSrc1()));

        // The label with the special offset value, used for reloc
        IR::LabelInstr *label = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func, false);
        instr->InsertBefore(label);
        Assert((immed & 0x0000000F) == immed);
        label->SetOffset((uint32)immed);
        label->isInlineeEntryInstr = true;

        IR::LabelOpnd *target = IR::LabelOpnd::New(label, instr->m_func);

        // We'll handle splitting this up to properly load the immediates now
        // Typically (and worst case) we'll need to load 64 bits.
        IR::Instr* bits0_15 = IR::Instr::New(Js::OpCode::MOVZ, instr->GetDst(), target, IR::IntConstOpnd::New(0, IRType::TyUint8, instr->m_func, true), instr->m_func);
        instr->InsertBefore(bits0_15);

        instr->ReplaceSrc1(target);
        instr->SetSrc2(IR::IntConstOpnd::New(16, IRType::TyUint8, instr->m_func, true));
        instr->m_opcode = Js::OpCode::MOVK;

        instr->isInlineeEntryInstr = false;
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
        // Less likely target reg will be RegR17 as we insert nops only for user defined constants and
        // RegR12 is mostly used for temporary data such as legalizer post regalloc.
        opnd1->SetReg(SCRATCH_REG);
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
    Assert(instr->m_opcode == Js::OpCode::LDIMM);
    Assert(opnd->IsLabelOpnd());

    if (opnd->AsLabelOpnd()->GetLabel()->isInlineeEntryInstr)
    {
        // We want to leave it as LDIMMs so that we can easily disambiguate later
        return;
    }
    else
    {
        instr->m_opcode = Js::OpCode::ADR;
    }
}

namespace
{
    IR::LabelInstr* TryInsertBranchIsland(IR::Instr* instr, IR::LabelInstr* target, int limit, bool forward)
    {
        int instrCount = 1;
        IR::Instr* branchInstr = nullptr;

        if (forward)
        {
            for (IR::Instr* next = instr->m_next; instrCount < limit && next != nullptr; next = next->m_next)
            {
                if (next->IsBranchInstr() && next->AsBranchInstr()->IsUnconditional())
                {
                    branchInstr = next;
                    break;
                }
                ++instrCount;
            }
        }
        else
        {
            for (IR::Instr* prev = instr->m_prev; instrCount < limit && prev != nullptr; prev = prev->m_prev)
            {
                if (prev->IsBranchInstr() && prev->AsBranchInstr()->IsUnconditional())
                {
                    branchInstr = prev;
                    break;
                }
                ++instrCount;
            }
        }

        if (branchInstr == nullptr)
        {
            return nullptr;
        }

        IR::LabelInstr* islandLabel = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func, false);
        branchInstr->InsertAfter(islandLabel);

        IR::BranchInstr* targetBranch = IR::BranchInstr::New(Js::OpCode::B, target, branchInstr->m_func);
        islandLabel->InsertAfter(targetBranch);

        return islandLabel;
    }
}

bool LegalizeMD::LegalizeDirectBranch(IR::BranchInstr *branchInstr, uintptr_t branchOffset)
{
    Assert(branchInstr->IsBranchInstr());

    uintptr_t labelOffset = branchInstr->GetTarget()->GetOffset();
    Assert(labelOffset); //Label offset must be set.

    intptr_t wordOffset = intptr_t(labelOffset - branchOffset) / 4;
    //We should never run out of 26 bits which corresponds to +-64MB of code size.
    AssertMsg(IS_CONST_INT26(wordOffset), "Cannot encode more that 64 MB offset");

    Assert(!LowererMD::IsUnconditionalBranch(branchInstr));

    int limit = 0;
    if (branchInstr->m_opcode == Js::OpCode::TBZ || branchInstr->m_opcode == Js::OpCode::TBNZ)
    {
        // TBZ and TBNZ are limited to 14 bit offsets.
        if (IS_CONST_INT14(wordOffset))
        {
            return false;
        }
        limit = 0x00001fff;
    }
    else
    {
        // Other conditional branches are limited to 19 bit offsets.
        if (IS_CONST_INT19(wordOffset))
        {
            return false;
        }
        limit = 0x0003ffff;
    }

    // Attempt to find an unconditional branch within the range and insert a branch island below it:
    //  CB. $island
    //  ...
    //  B
    //  $island:
    //  B $target
    
    IR::LabelInstr* islandLabel = TryInsertBranchIsland(branchInstr, branchInstr->GetTarget(), limit, wordOffset < 0);
    if (islandLabel != nullptr)
    {
        branchInstr->SetTarget(islandLabel);
        return true;
    }

    // Convert a conditional branch which can only be +-256kb(+-8kb for TBZ/TBNZ) to unconditional branch which is +-64MB
    // Convert beq Label (where Label is long jump) to something like this
    //          bne Fallback
    //          b Label
    // Fallback:

    IR::LabelInstr *fallbackLabel = IR::LabelInstr::New(Js::OpCode::Label, branchInstr->m_func, false);
    IR::BranchInstr *fallbackBranch = IR::BranchInstr::New(branchInstr->m_opcode, fallbackLabel, branchInstr->m_func);

    // CBZ | CBNZ | TBZ | TBNZ
    if (branchInstr->GetSrc1() != nullptr)
    {
        fallbackBranch->SetSrc1(branchInstr->UnlinkSrc1());
    }

    // TBZ | TBNZ
    if (branchInstr->GetSrc2() != nullptr)
    {
        fallbackBranch->SetSrc2(branchInstr->UnlinkSrc2());
    }

    LowererMD::InvertBranch(fallbackBranch);

    branchInstr->InsertBefore(fallbackBranch);
    branchInstr->InsertAfter(fallbackLabel);
    branchInstr->m_opcode = Js::OpCode::B;
    return true;
}

bool LegalizeMD::LegalizeAdrOffset(IR::Instr *instr, uintptr_t instrOffset)
{
    Assert(instr->m_opcode == Js::OpCode::ADR);

    IR::LabelOpnd* labelOpnd = instr->GetSrc1()->AsLabelOpnd();
    IR::LabelInstr* label = labelOpnd->GetLabel();

    uintptr_t labelOffset = label->GetOffset();
    Assert(labelOffset); //Label offset must be set.

    intptr_t wordOffset = intptr_t(labelOffset - instrOffset) / 4;

    if (IS_CONST_INT19(wordOffset))
    {
        return false;
    }

    //We should never run out of 26 bits which corresponds to +-64MB of code size.
    AssertMsg(IS_CONST_INT26(wordOffset), "Cannot encode more that 64 MB offset");

    // Attempt to find an unconditional branch within the range and insert a branch island below it:
    //  ADR $island
    //  ...
    //  B
    //  $island:
    //  B $target

    int limit = 0x0003ffff;
    IR::LabelInstr* islandLabel = TryInsertBranchIsland(instr, label, limit, wordOffset < 0);
    if (islandLabel != nullptr)
    {
        labelOpnd->SetLabel(islandLabel);
        return true;
    }

    // Convert an Adr instruction which can only be +-1mb to branch which is +-64MB
    //  Adr $adrLabel
    //  b continue
    //  $adrLabel:
    //  b label
    //  $continue:

    IR::LabelInstr* continueLabel = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func, false);
    instr->InsertAfter(continueLabel);

    IR::BranchInstr* continueBranch = IR::BranchInstr::New(Js::OpCode::B, continueLabel, instr->m_func);
    continueLabel->InsertBefore(continueBranch);

    IR::LabelInstr* adrLabel = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func, false);
    continueLabel->InsertBefore(adrLabel);

    IR::BranchInstr* labelBranch = IR::BranchInstr::New(Js::OpCode::B, label, instr->m_func);
    continueLabel->InsertBefore(labelBranch);

    labelOpnd->SetLabel(adrLabel);
    return true;
}

bool LegalizeMD::LegalizeDataAdr(IR::Instr *instr, uintptr_t dataOffset)
{
    Assert(instr->m_opcode == Js::OpCode::ADR);

    IR::LabelOpnd* labelOpnd = instr->GetSrc1()->AsLabelOpnd();

    Assert(labelOpnd->GetLabel()->m_isDataLabel);


    // dataOffset provides an upper bound on the distance between instr and the label.
    if (IS_CONST_INT19(dataOffset >> 2))
    {
        return false;
    }

    // The distance is too large to encode as an ADR isntruction so it must be handled as a 3 instruction load immediate. 
    // The label address won't be known until after encoding. Assign the label opnd as src1 to let the encoder know to handle them as relocs.

    IR::Instr* bits0_15 = IR::Instr::New(Js::OpCode::MOVZ, instr->GetDst(), labelOpnd, IR::IntConstOpnd::New(0, IRType::TyUint8, instr->m_func, true), instr->m_func);
    instr->InsertBefore(bits0_15);

    IR::Instr* bits16_31 = IR::Instr::New(Js::OpCode::MOVK, instr->GetDst(), labelOpnd, IR::IntConstOpnd::New(16, IRType::TyUint8, instr->m_func, true), instr->m_func);
    instr->InsertBefore(bits16_31);

    instr->SetSrc2(IR::IntConstOpnd::New(32, IRType::TyUint8, instr->m_func, true));
    instr->m_opcode = Js::OpCode::MOVK;

    return true;
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
