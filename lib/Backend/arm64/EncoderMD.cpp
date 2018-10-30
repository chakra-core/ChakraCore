//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "ARM64Encoder.h"
#include "ARM64NeonEncoder.h"
#include "Language/JavascriptFunctionArgIndex.h"

static const uint32 Opdope[] =
{
#define MACRO(name, jnLayout, attrib, byte2, form, opbyte, dope, ...) dope,
#include "MdOpCodes.h"
#undef MACRO
};

DWORD
EncoderMD::BranchOffset_26(int64 x)
{
    Assert(IS_CONST_INT26(x >> 1));
    Assert((x & 0x3) == 0);
    x = x >> 2;
    return (DWORD) x;
}

///----------------------------------------------------------------------------
///
/// EncoderMD::Init
///
///----------------------------------------------------------------------------

void
EncoderMD::Init(Encoder *encoder)
{
    m_encoder = encoder;
    m_relocList = nullptr;
}

///----------------------------------------------------------------------------
///
/// EncoderMD::GetRegEncode
///
///     Get the encoding of a given register.
///
///----------------------------------------------------------------------------

BYTE
EncoderMD::GetRegEncode(IR::RegOpnd *regOpnd)
{
    return GetRegEncode(regOpnd->GetReg());
}

BYTE
EncoderMD::GetRegEncode(RegNum reg)
{
    return RegEncode[reg];
}

BYTE
EncoderMD::GetFloatRegEncode(IR::RegOpnd *regOpnd)
{
    BYTE regEncode = GetRegEncode(regOpnd->GetReg());
    AssertMsg(regEncode <= LAST_FLOAT_REG_ENCODE, "Impossible to allocate higher registers on VFP");
    return regEncode;
}

///----------------------------------------------------------------------------
///
/// EncoderMD::GetOpdope
///
///     Get the dope vector of a particular instr.  The dope vector describes
///     certain properties of an instr.
///
///----------------------------------------------------------------------------

uint32
EncoderMD::GetOpdope(IR::Instr *instr)
{
    return GetOpdope(instr->m_opcode);
}

uint32
EncoderMD::GetOpdope(Js::OpCode op)
{
    return Opdope[op - (Js::OpCode::MDStart+1)];
}

//
// EncoderMD::CanonicalizeInstr :
//     Put the instruction in its final form for encoding. This may involve
// expanding a pseudo-op such as LEA or changing an opcode to indicate the
// op bits the encoder should use.
//
//     Return the size of the final instruction's encoding.
//

bool EncoderMD::CanonicalizeInstr(IR::Instr* instr)
{
    if (!instr->IsLowered())
    {
        return false;
    }

    if (instr->m_opcode == Js::OpCode::LEA)
    {
        this->CanonicalizeLea(instr);
    }

    return true;
}

void EncoderMD::CanonicalizeLea(IR::Instr * instr)
{
    int32 offset;

    IR::Opnd* src1 = instr->UnlinkSrc1();

    if (src1->IsSymOpnd())
    {
        RegNum baseReg;
        // We may as well turn this LEA into the equivalent ADD instruction and let the common ADD
        // logic handle it.
        IR::SymOpnd *symOpnd = src1->AsSymOpnd();

        this->BaseAndOffsetFromSym(symOpnd, &baseReg, &offset, this->m_func);
        symOpnd->Free(this->m_func);
        instr->SetSrc1(IR::RegOpnd::New(nullptr, baseReg, TyMachReg, this->m_func));
        Assert(IS_CONST_00000FFF(offset) || IS_CONST_00FFF000(offset));
        instr->SetSrc2(IR::IntConstOpnd::New(offset, TyMachReg, this->m_func));
    }
    else
    {
        IR::IndirOpnd *indirOpnd = src1->AsIndirOpnd();
        IR::RegOpnd *baseOpnd = indirOpnd->GetBaseOpnd();
        IR::RegOpnd *indexOpnd = indirOpnd->GetIndexOpnd();
        offset = indirOpnd->GetOffset();

        Assert(offset == 0 || indexOpnd == nullptr);
        instr->SetSrc1(baseOpnd);

        if (indexOpnd)
        {
            AssertMsg(indirOpnd->GetScale() == 0, "NYI Needs shifted register support for ADD");
            instr->SetSrc2(indexOpnd);
        }
        else
        {
            // We want to emit a legal instruction
            Assert(IS_CONST_00000FFF(offset) || IS_CONST_00FFF000(offset));
            instr->SetSrc2(IR::IntConstOpnd::New(offset, TyMachReg, this->m_func));
        }
        indirOpnd->Free(this->m_func);
    }
    instr->m_opcode = Js::OpCode::ADD;
    Assert(instr->GetSrc1()->GetSize() == instr->GetSrc2()->GetSize());
}

bool
EncoderMD::DecodeMemoryOpnd(IR::Opnd* opnd, ARM64_REGISTER &baseRegResult, ARM64_REGISTER &indexRegResult, BYTE &indexScale, int32 &offset)
{
    RegNum baseReg;

    if (opnd->IsSymOpnd())
    {
        IR::SymOpnd *symOpnd = opnd->AsSymOpnd();

        this->BaseAndOffsetFromSym(symOpnd, &baseReg, &offset, this->m_func);
        baseRegResult = this->GetRegEncode(baseReg);
        return false;
    }
    else
    {
        IR::IndirOpnd *indirOpnd = opnd->AsIndirOpnd();
        IR::RegOpnd *baseOpnd = indirOpnd->GetBaseOpnd();
        IR::RegOpnd *indexOpnd = indirOpnd->GetIndexOpnd();
        offset = indirOpnd->GetOffset();

        Assert(offset == 0 || indexOpnd == nullptr);
        baseRegResult = this->GetRegEncode(baseOpnd);

        if (indexOpnd)
        {
            indexRegResult = this->GetRegEncode(indexOpnd);
            indexScale = indirOpnd->GetScale();
            return true;
        }
        else
        {
            return false;
        }
    }
}

template<typename _RegFunc64> 
int EncoderMD::EmitOp1Register64(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc64 reg64)
{
    IR::Opnd* src1 = instr->GetSrc1();
    Assert(src1->IsRegOpnd());
    Assert(src1->GetSize() == 8);

    return reg64(Emitter, this->GetRegEncode(src1->AsRegOpnd()));
}

template<typename _RegFunc32, typename _RegFunc64>
int EncoderMD::EmitOp2Register(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();

    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());

    if (size == 8)
    {
        return reg64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
    }
    else
    {
        return reg32(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
    }
}

template<typename _RegFunc32, typename _RegFunc64>
int EncoderMD::EmitOp3Register(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();

    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());
    Assert(src2->IsRegOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());
    Assert(size == src2->GetSize());

    if (size == 8)
    {
        return reg64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
    }
    else
    {
        return reg32(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
    }
}

template<typename _RegFunc32, typename _RegFunc64>
int EncoderMD::EmitOp3RegisterShifted(Arm64CodeEmitter &Emitter, IR::Instr* instr, SHIFT_EXTEND_TYPE shiftType, int shiftAmount, _RegFunc32 reg32, _RegFunc64 reg64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();

    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());
    Assert(src2->IsRegOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());
    Assert(size == src2->GetSize());

    if (size == 8)
    {
        return reg64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), Arm64RegisterParam(this->GetRegEncode(src2->AsRegOpnd()), shiftType, shiftAmount & 63));
    }
    else
    {
        return reg32(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), Arm64RegisterParam(this->GetRegEncode(src2->AsRegOpnd()), shiftType, shiftAmount & 31));
    }
}

template<typename _ImmFunc32, typename _ImmFunc64>
int EncoderMD::EmitOp3Immediate(Arm64CodeEmitter &Emitter, IR::Instr* instr, _ImmFunc32 imm32, _ImmFunc64 imm64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();

    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());
    Assert(src2->IsImmediateOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());

    int64 immediate = src2->GetImmediateValue(instr->m_func);
    if (size == 8)
    {
        return imm64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG64(immediate));
    }
    else
    {
        return imm32(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
    }
}

template<typename _RegFunc32, typename _RegFunc64, typename _ImmFunc32, typename _ImmFunc64>
int EncoderMD::EmitOp3RegisterOrImmediate(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32,  _RegFunc64 reg64, _ImmFunc32 imm32, _ImmFunc64 imm64)
{
    if (instr->GetSrc2()->IsImmediateOpnd())
    {
        return this->EmitOp3Immediate(Emitter, instr, imm32, imm64);
    }
    else if (instr->GetSrc2()->IsRegOpnd())
    {
        return this->EmitOp3Register(Emitter, instr, reg32, reg64);
    }
    else
    {
        AssertMsg(false, "EmitOp3RegisterOrImmediate failed to encode");
        return 0;
    }
}

template<typename _RegFunc32, typename _RegFunc64, typename _ImmFunc32, typename _ImmFunc64>
int EncoderMD::EmitOp3RegisterOrImmediateExtendSPReg(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64, _ImmFunc32 imm32, _ImmFunc64 imm64)
{
    // if we have two regopnds as sources, then we need to be careful
    if (instr->GetSrc1()->IsRegOpnd() && instr->GetSrc2()->IsRegOpnd())
    {
        IR::RegOpnd* src1 = instr->GetSrc1()->AsRegOpnd();
        IR::RegOpnd* src2 = instr->GetSrc2()->AsRegOpnd();
        IR::RegOpnd* dst  = instr->GetDst()->AsRegOpnd();
        AssertMsg(!(src1->GetReg() == RegSP && src2->GetReg() == RegSP), "Tried to encode an add or sub that used RegSP as both sources - tighten legalization restrictions!");

        // We need to swap the parameters if
        // 1. src2 is RegSP
        // 2. src1 is RegZR and dst is RegSP
        // This is because the valid instruction forms are
        // add Rd, Rs, Rw_SFT
        // add Rd|SP, Rs|SP, Rw_EXT
        // and the encoding for SP is the same as the encoding for ZR
        if (src2->GetReg() == RegSP || (src1->GetReg() == RegZR && dst->GetReg() == RegSP))
        {
            // We can only really do this for addition, so we failfast if it's a sub
            AssertOrFailFastMsg(instr->m_opcode != Js::OpCode::SUB && instr->m_opcode != Js::OpCode::SUBS, "Tried to encode a SUB/SUBS with RegSP as the second operand or as the dest with RegZR in first operand - tighten legalization restrictions!");
            // We need to swap the arguments
            instr->UnlinkSrc1();
            instr->UnlinkSrc2();
            instr->SetSrc1(src2);
            instr->SetSrc2(src1);
            IR::RegOpnd* temp = src1;
            src1 = src2;
            src2 = temp;
        }

        // The extended form of the instruction takes RegSP for dst and src1 and RegZR for src2
        if (src1->GetReg() == RegSP || instr->GetDst()->AsRegOpnd()->GetReg() == RegSP)
        {
            // EXTEND_UXTX effectively means LSL here, just that LSL is a shift, not an extend, operation
            // Regardless, we do it by 0, so it should just be directly using the register
            return this->EmitOp3RegisterShifted(Emitter, instr, EXTEND_UXTX, 0, reg32, reg64);
        }
    }

    return EmitOp3RegisterOrImmediate(Emitter, instr, reg32, reg64, imm32, imm64);
}

int EncoderMD::EmitPrefetch(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd)
{
    Assert(memOpnd->IsIndirOpnd() || memOpnd->IsSymOpnd());

    ARM64_REGISTER indexReg;
    ARM64_REGISTER baseReg;
    BYTE indexScale;
    int32 offset;
    if (DecodeMemoryOpnd(memOpnd, baseReg, indexReg, indexScale, offset))
    {
        return EmitPrfmRegister(Emitter, baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
    }
    else
    {
        return EmitPrfmOffset(Emitter, baseReg, offset);
    }
}

template<typename _RegFunc8, typename _RegFunc16, typename _RegFunc32, typename _RegFunc64, typename _OffFunc8, typename _OffFunc16, typename _OffFunc32, typename _OffFunc64>
int EncoderMD::EmitLoadStore(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd,  IR::Opnd* srcDstOpnd, _RegFunc8 reg8, _RegFunc16 reg16, _RegFunc32 reg32, _RegFunc64 reg64, _OffFunc8 off8, _OffFunc16 off16, _OffFunc32 off32, _OffFunc64 off64)
{
    Assert(srcDstOpnd->IsRegOpnd());
    Assert(memOpnd->IsIndirOpnd() || memOpnd->IsSymOpnd());

    int size = memOpnd->GetSize();
    Assert(size == 1 || size == 2 || size == 4 || size == 8);

    ARM64_REGISTER indexReg;
    ARM64_REGISTER baseReg;
    BYTE indexScale;
    int32 offset;
    if (DecodeMemoryOpnd(memOpnd, baseReg, indexReg, indexScale, offset))
    {
        if (size == 8)
        {
            return reg64(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else if (size == 4)
        {
            return reg32(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else if (size == 2)
        {
            return reg16(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else
        {
            return reg8(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
    }
    else
    {
        if (size == 8)
        {
            return off64(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, offset);
        }
        else if (size == 4)
        {
            return off32(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, offset);
        }
        else if (size == 2)
        {
            return off16(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, offset);
        }
        else
        {
            return off8(Emitter, this->GetRegEncode(srcDstOpnd->AsRegOpnd()), baseReg, offset);
        }
    }
}

template<typename _OffFunc32, typename _OffFunc64>
int EncoderMD::EmitLoadStorePair(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd, IR::Opnd* srcDst1Opnd, IR::Opnd* srcDst2Opnd, _OffFunc32 off32, _OffFunc64 off64)
{
    Assert(memOpnd->IsIndirOpnd() || memOpnd->IsSymOpnd());

    int size = memOpnd->GetSize();
    Assert(size == 4 || size == 8);

    ARM64_REGISTER indexReg;
    ARM64_REGISTER baseReg;
    BYTE indexScale;
    int32 offset;
    if (DecodeMemoryOpnd(memOpnd, baseReg, indexReg, indexScale, offset))
    {
        // Should never get here
        AssertMsg(false, "EmitLoadStorePair failed to encode");
        return 0;
    }
    else
    {
        if (size == 8)
        {
            return off64(Emitter, this->GetRegEncode(srcDst1Opnd->AsRegOpnd()),  this->GetRegEncode(srcDst2Opnd->AsRegOpnd()), baseReg, offset);
        }
        else
        {
            return off32(Emitter, this->GetRegEncode(srcDst1Opnd->AsRegOpnd()),  this->GetRegEncode(srcDst2Opnd->AsRegOpnd()), baseReg, offset);
        }
    }
}

template<typename _Emitter>
int EncoderMD::EmitUnconditionalBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Emitter emitter)
{
    ArmBranchLinker Linker;
    EncodeReloc::New(&m_relocList, RelocTypeBranch26, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
    Linker.SetTarget(Emitter);
    return emitter(Emitter, Linker);
}

int EncoderMD::EmitConditionalBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, int condition)
{
    ArmBranchLinker Linker;
    EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
    Linker.SetTarget(Emitter);
    return EmitBranch(Emitter, Linker, condition);
}

template<typename _Emitter, typename _Emitter64>
int EncoderMD::EmitCompareAndBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Emitter emitter, _Emitter64 emitter64)
{
    IR::Opnd* src1 = instr->GetSrc1();
    Assert(src1->IsRegOpnd());

    int size = src1->GetSize();
    Assert(size == 4 || size == 8);

    ArmBranchLinker Linker;
    EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
    Linker.SetTarget(Emitter);

    if (size == 8)
    {
        return emitter64(Emitter, this->GetRegEncode(src1->AsRegOpnd()), Linker);
    }
    else
    {
        return emitter(Emitter, this->GetRegEncode(src1->AsRegOpnd()), Linker);
    }
}

template<typename _Emitter>
int EncoderMD::EmitTestAndBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Emitter emitter)
{
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(src1->IsRegOpnd());
    Assert(src2->IsImmediateOpnd());

    ArmBranchLinker Linker;
    EncodeReloc::New(&m_relocList, RelocTypeBranch14, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
    Linker.SetTarget(Emitter);

    int64 immediate = src2->GetImmediateValue(instr->m_func);
    Assert(immediate >= 0 && immediate < 64);
    return emitter(Emitter, this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate), Linker);
}

template<typename _Emitter, typename _Emitter64>
int EncoderMD::EmitMovConstant(Arm64CodeEmitter &Emitter, IR::Instr *instr, _Emitter emitter, _Emitter64 emitter64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd());
    Assert(src1->IsImmediateOpnd() || src1->IsLabelOpnd());
    Assert(src2->IsIntConstOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);

    uint32 shift = src2->AsIntConstOpnd()->AsUint32();
    Assert(shift < 32 || size == 8);
    Assert(shift == 0 || shift == 16 || (size == 8 && (shift == 32 || shift == 48)));

    IntConstType immediate = 0;
    if (src1->IsImmediateOpnd())
    {
        immediate = src1->GetImmediateValue(instr->m_func);
    }
    else
    {
        Assert(src1->IsLabelOpnd());
        IR::LabelInstr* labelInstr = src1->AsLabelOpnd()->GetLabel();

        if (labelInstr->m_isDataLabel)
        {
            // If the label is a data label, we don't know the label address yet so handle it as a reloc.
            EncodeReloc::New(&m_relocList, RelocTypeLabelImmed, m_pc, labelInstr, m_encoder->m_tempAlloc);
            immediate = 0;
        }
        else
        {
            // Here the LabelOpnd's offset is a post-lower immediate value; we need
            // to mask in just the part indicated by our own shift amount, and send
            // that along as our immediate load value.
            uintptr_t fullvalue = labelInstr->GetOffset();
            immediate = (fullvalue >> shift) & 0xffff;
        }
    }

    Assert((immediate & 0xFFFF) == immediate);

    if (size == 8)
    {
        return emitter64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate), shift);
    }
    else
    {
        return emitter(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate), shift);
    }
}

template<typename _Emitter, typename _Emitter64>
int EncoderMD::EmitBitfield(Arm64CodeEmitter &Emitter, IR::Instr *instr, _Emitter emitter, _Emitter64 emitter64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());
    Assert(src2->IsImmediateOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());

    IntConstType immediate = src2->GetImmediateValue(instr->m_func);
    int start = immediate & 0x3f;
    int length = (immediate >> 16) & 0x3f;
    Assert(start >= 0 && start < 8 * size);
    Assert(length >= 0 && length < 8 * size);

    if (size == 8)
    {
        return emitter64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), start, length);
    }
    else
    {
        return emitter(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), start, length);
    }
}

template<typename _Emitter, typename _Emitter64>
int EncoderMD::EmitConditionalSelect(Arm64CodeEmitter &Emitter, IR::Instr *instr, int condition, _Emitter emitter, _Emitter64 emitter64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());
    Assert(src2->IsRegOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());
    Assert(size == src2->GetSize());

    if (size == 8)
    {
        return emitter64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), condition);
    }
    else
    {
        return emitter(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), condition);
    }
}

template<typename _Emitter>
int EncoderMD::EmitOp2FpRegister(Arm64CodeEmitter &Emitter, IR::Instr *instr, _Emitter emitter)
{
    return EmitOp2FpRegister(Emitter, instr->GetDst(), instr->GetSrc1(), emitter);
}

template<typename _Emitter>
int EncoderMD::EmitOp2FpRegister(Arm64CodeEmitter &Emitter, IR::Opnd* opnd1, IR::Opnd* opnd2, _Emitter emitter)
{
    Assert(opnd1->IsRegOpnd());
    Assert(opnd2->IsRegOpnd());

    int size = opnd1->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == opnd2->GetSize());

    NEON_SIZE neonSize = (size == 8) ? SIZE_1D : SIZE_1S;

    return emitter(Emitter, this->GetFloatRegEncode(opnd1->AsRegOpnd()), this->GetFloatRegEncode(opnd2->AsRegOpnd()), neonSize);
}

template<typename _Emitter>
int EncoderMD::EmitOp3FpRegister(Arm64CodeEmitter &Emitter, IR::Instr *instr, _Emitter emitter)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();

    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());
    Assert(src2->IsRegOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());
    Assert(size == src2->GetSize());

    NEON_SIZE neonSize = (size == 8) ? SIZE_1D : SIZE_1S;

    return emitter(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), this->GetFloatRegEncode(src1->AsRegOpnd()), this->GetFloatRegEncode(src2->AsRegOpnd()), neonSize);
}

template<typename _LoadStoreFunc>
int EncoderMD::EmitLoadStoreFp(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd, IR::Opnd* srcDstOpnd, _LoadStoreFunc loadStore)
{
    Assert(srcDstOpnd->IsRegOpnd());
    Assert(memOpnd->IsIndirOpnd() || memOpnd->IsSymOpnd());

    int size = memOpnd->GetSize();
    Assert(size == 4 || size == 8);

    ARM64_REGISTER indexReg;
    ARM64_REGISTER baseReg;
    BYTE indexScale;
    int32 offset;
    if (DecodeMemoryOpnd(memOpnd, baseReg, indexReg, indexScale, offset))
    {
        // Should never get here
        AssertMsg(false, "EmitLoadStoreFp failed to encode");
        return 0;
    }
    else
    {
        return loadStore(Emitter, this->GetFloatRegEncode(srcDstOpnd->AsRegOpnd()), (size == 8) ? SIZE_1D : SIZE_1S, baseReg, offset);
    }
}

template<typename _LoadStoreFunc>
int EncoderMD::EmitLoadStoreFpPair(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd, IR::Opnd* srcDst1Opnd, IR::Opnd* srcDst2Opnd, _LoadStoreFunc loadStore)
{
    Assert(memOpnd->IsIndirOpnd() || memOpnd->IsSymOpnd());

    int size = memOpnd->GetSize();
    Assert(size == 4 || size == 8);

    ARM64_REGISTER indexReg;
    ARM64_REGISTER baseReg;
    BYTE indexScale;
    int32 offset;
    if (DecodeMemoryOpnd(memOpnd, baseReg, indexReg, indexScale, offset))
    {
        // Should never get here
        AssertMsg(false, "EmitLoadStoreFpPair failed to encode");
        return 0;
    }
    else
    {
        return loadStore(Emitter, this->GetFloatRegEncode(srcDst1Opnd->AsRegOpnd()), this->GetFloatRegEncode(srcDst2Opnd->AsRegOpnd()), (size == 8) ? SIZE_1D : SIZE_1S, baseReg, offset);
    }
}

template<typename _Int32Func, typename _Uint32Func, typename _Int64Func, typename _Uint64Func>
int EncoderMD::EmitConvertToInt(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Int32Func toInt32, _Uint32Func toUint32, _Int64Func toInt64, _Uint64Func toUint64)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    Assert(dst->IsRegOpnd());
    Assert(!dst->IsFloat());
    Assert(src1->IsRegOpnd());
    Assert(src1->IsFloat());

    DebugOnly(int size = dst->GetSize());
    Assert(size == 4 || size == 8);
    int srcSize = src1->GetSize();
    Assert(srcSize == 4 || srcSize == 8);

    if (dst->GetType() == TyInt32)
    {
        return toInt32(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetFloatRegEncode(src1->AsRegOpnd()), (srcSize == 8) ? SIZE_1D : SIZE_1S);
    }
    else if (dst->GetType() == TyUint32)
    {
        return toUint32(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetFloatRegEncode(src1->AsRegOpnd()), (srcSize == 8) ? SIZE_1D : SIZE_1S);
    }
    else if (dst->GetType() == TyInt64)
    {
        return toInt64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetFloatRegEncode(src1->AsRegOpnd()), (srcSize == 8) ? SIZE_1D : SIZE_1S);
    }
    else if (dst->GetType() == TyUint64)
    {
        return toUint64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetFloatRegEncode(src1->AsRegOpnd()), (srcSize == 8) ? SIZE_1D : SIZE_1S);
    }
    
    // Shouldn't get here
    AssertMsg(false, "EmitConvertToInt failed to encode");
    return 0;
}

template<typename _Emitter>
int EncoderMD::EmitConditionalSelectFp(Arm64CodeEmitter &Emitter, IR::Instr *instr, int condition, _Emitter emitter)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd());
    Assert(src1->IsRegOpnd());
    Assert(src2->IsRegOpnd());

    int size = dst->GetSize();
    Assert(size == 4 || size == 8);
    Assert(size == src1->GetSize());
    Assert(size == src2->GetSize());

    return emitter(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), this->GetFloatRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), condition, (size == 8) ? SIZE_1D : SIZE_1S);
}

//---------------------------------------------------------------------------
//
// GenerateEncoding()
//
// generates the encoding for the specified tuple/form by applying the
// associated encoding steps
//
//---------------------------------------------------------------------------
ULONG
EncoderMD::GenerateEncoding(IR::Instr* instr, BYTE *pc)
{
    Arm64LocalCodeEmitter<1> Emitter;
    IR::Opnd* dst = 0;
    IR::Opnd* src1 = 0;
    IR::Opnd* src2 = 0;
    int bytes = 0;
    int size;

    switch (instr->m_opcode)
    {
    case Js::OpCode::ADD:
        bytes = this->EmitOp3RegisterOrImmediateExtendSPReg(Emitter, instr, EmitAddRegister, EmitAddRegister64, EmitAddImmediate, EmitAddImmediate64);
        break;

    case Js::OpCode::ADDS:
        // ADDS and SUBS have no valid encoding where dst == RegSP
        Assert(instr->GetDst()->AsRegOpnd()->GetReg() != RegSP);
        bytes = this->EmitOp3RegisterOrImmediateExtendSPReg(Emitter, instr, EmitAddsRegister, EmitAddsRegister64, EmitAddsImmediate, EmitAddsImmediate64);
        break;

    case Js::OpCode::ADR:
        dst = instr->GetDst();
        src1 = instr->GetSrc1();
        Assert(dst->IsRegOpnd());
        Assert(src1->IsLabelOpnd());

        Assert(dst->GetSize() == 8);
        Assert(!src1->AsLabelOpnd()->GetLabel()->isInlineeEntryInstr);
        EncodeReloc::New(&m_relocList, RelocTypeLabelAdr, m_pc, src1->AsLabelOpnd()->GetLabel(), m_encoder->m_tempAlloc);
        bytes = EmitAdr(Emitter, this->GetRegEncode(dst->AsRegOpnd()), 0);
        break;

    case Js::OpCode::AND:
        bytes = this->EmitOp3RegisterOrImmediate(Emitter, instr, EmitAndRegister, EmitAndRegister64, EmitAndImmediate, EmitAndImmediate64);
        break;

    case Js::OpCode::ANDS:
        bytes = this->EmitOp3RegisterOrImmediate(Emitter, instr, EmitAndsRegister, EmitAndsRegister64, EmitAndsImmediate, EmitAndsImmediate64);
        break;

    case Js::OpCode::ASR:
        bytes = this->EmitOp3RegisterOrImmediate(Emitter, instr, EmitAsrRegister, EmitAsrRegister64, EmitAsrImmediate, EmitAsrImmediate64);
        break;
    
    case Js::OpCode::B:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_AL);
        break;

    case Js::OpCode::BFI:
        bytes = this->EmitBitfield(Emitter, instr, EmitBfi, EmitBfi64);
        break;

    case Js::OpCode::BFXIL:
        bytes = this->EmitBitfield(Emitter, instr, EmitBfxil, EmitBfxil64);
        break;

    // ARM64_WORKITEM: Legalizer needs to convert BIC with immediate to AND with inverted immediate
    case Js::OpCode::BIC:
        bytes = this->EmitOp3Register(Emitter, instr, EmitBicRegister, EmitBicRegister64);
        break;

    case Js::OpCode::BL:
        bytes = this->EmitUnconditionalBranch(Emitter, instr, EmitBl);
        break;
    
    case Js::OpCode::BR:
        bytes = this->EmitOp1Register64(Emitter, instr, EmitBr);
        break;
    
    case Js::OpCode::BLR:
        bytes = this->EmitOp1Register64(Emitter, instr, EmitBlr);
        break;
    
    case Js::OpCode::BEQ:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_EQ);
        break;
    
    case Js::OpCode::BNE:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_NE);
        break;
    
    case Js::OpCode::BLT:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_LT);
        break;
    
    case Js::OpCode::BLE:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_LE);
        break;

    case Js::OpCode::BGT:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_GT);
        break;
    
    case Js::OpCode::BGE:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_GE);
        break;

    case Js::OpCode::BCS:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_CS);
        break;
    
    case Js::OpCode::BCC:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_CC);
        break;
    
    case Js::OpCode::BHI:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_HI);
        break;
    
    case Js::OpCode::BLS:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_LS);
        break;
    
    case Js::OpCode::BMI:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_MI);
        break;
    
    case Js::OpCode::BPL:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_PL);
        break;

    case Js::OpCode::BVS:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_VS);
        break;
    
    case Js::OpCode::BVC:
        bytes = this->EmitConditionalBranch(Emitter, instr, COND_VC);
        break;
    
    case Js::OpCode::CBZ:
        bytes = this->EmitCompareAndBranch(Emitter, instr, EmitCbz, EmitCbz64);
        break;

    case Js::OpCode::CBNZ:
        bytes = this->EmitCompareAndBranch(Emitter, instr, EmitCbnz, EmitCbnz64);
        break;

    case Js::OpCode::CLZ:
        bytes = this->EmitOp2Register(Emitter, instr, EmitClz, EmitClz64);
        break;

    // Legalizer should convert this to SUBS before getting here
    case Js::OpCode::CMP:
        Assert(false);
        break;

    // Legalizer should convert this to ADDS before getting here
    case Js::OpCode::CMN:
        Assert(false);
        break;

    case Js::OpCode::CSELEQ:
        bytes = this->EmitConditionalSelect(Emitter, instr, COND_EQ, EmitCsel, EmitCsel64);
        break;

    case Js::OpCode::CSELNE:
        bytes = this->EmitConditionalSelect(Emitter, instr, COND_NE, EmitCsel, EmitCsel64);
        break;

    case Js::OpCode::CSELLT:
        bytes = this->EmitConditionalSelect(Emitter, instr, COND_LT, EmitCsel, EmitCsel64);
        break;

    case Js::OpCode::CSNEGPL:
        bytes = this->EmitConditionalSelect(Emitter, instr, COND_PL, EmitCsneg, EmitCsneg64);
        break;

    case Js::OpCode::CMP_SXTW:
        src1 = instr->GetSrc1();
        src2 = instr->GetSrc2();
        Assert(instr->GetDst() == nullptr);
        Assert(src1->IsRegOpnd());
        Assert(src2->IsRegOpnd());

        size = src1->GetSize();
        Assert(size == 8);
        Assert(size == src2->GetSize());

        bytes = EmitSubsRegister64(Emitter, ARMREG_ZR, this->GetRegEncode(src1->AsRegOpnd()), Arm64RegisterParam(this->GetRegEncode(src2->AsRegOpnd()), EXTEND_SXTW, 0));
        break;

    case Js::OpCode::DEBUGBREAK:
        bytes = EmitDebugBreak(Emitter);
        break;

    case Js::OpCode::EOR:
        bytes = this->EmitOp3RegisterOrImmediate(Emitter, instr, EmitEorRegister, EmitEorRegister64, EmitEorImmediate, EmitEorImmediate64);
        break;

    case Js::OpCode::EOR_ASR31:
        bytes = this->EmitOp3RegisterShifted(Emitter, instr, SHIFT_ASR, 63, EmitEorRegister, EmitEorRegister64);
        break;

    // Legalizer should convert these into MOVZ/MOVN/MOVK
    case Js::OpCode::LDIMM:
        Assert(false);
        break;

    case Js::OpCode::LDR:
        Assert(instr->GetDst()->GetSize() <= instr->GetSrc1()->GetSize() || instr->GetSrc1()->IsUnsigned());
        bytes = this->EmitLoadStore(Emitter, instr, instr->GetSrc1(), instr->GetDst(), EmitLdrbRegister, EmitLdrhRegister, EmitLdrRegister, EmitLdrRegister64, EmitLdrbOffset, EmitLdrhOffset, EmitLdrOffset, EmitLdrOffset64);
        break;

    case Js::OpCode::LDRS:
        Assert(instr->GetDst()->GetSize() <= instr->GetSrc1()->GetSize() || instr->GetSrc1()->IsSigned());
        bytes = this->EmitLoadStore(Emitter, instr, instr->GetSrc1(), instr->GetDst(), EmitLdrsbRegister, EmitLdrshRegister, EmitLdrswRegister64, EmitLdrRegister64, EmitLdrsbOffset, EmitLdrshOffset, EmitLdrswOffset64, EmitLdrOffset64);
        break;

    // Note: src2 is really the second destination register, due to limitations of IR::Instr
    case Js::OpCode::LDP:
        bytes = this->EmitLoadStorePair(Emitter, instr, instr->GetSrc1(), instr->GetDst(), instr->GetSrc2(), EmitLdpOffset, EmitLdpOffset64);
        break;

    // Note: src2 is really the second destination register, due to limitations of IR::Instr
    case Js::OpCode::LDP_POST:
        bytes = this->EmitLoadStorePair(Emitter, instr, instr->GetSrc1(), instr->GetDst(), instr->GetSrc2(), EmitLdpOffsetPostIndex, EmitLdpOffsetPostIndex64);
        break;

    // Legalizer should convert this to MOV/ADD before getting here
    case Js::OpCode::LEA:
        Assert(false);
        break;
    
    case Js::OpCode::LSL:
        bytes = this->EmitOp3RegisterOrImmediate(Emitter, instr, EmitLslRegister, EmitLslRegister64, EmitLslImmediate, EmitLslImmediate64);
        break;
        
    case Js::OpCode::LSR:
        bytes = this->EmitOp3RegisterOrImmediate(Emitter, instr, EmitLsrRegister, EmitLsrRegister64, EmitLsrImmediate, EmitLsrImmediate64);
        break;

    case Js::OpCode::MOV_TRUNC:
        Assert(instr->GetDst()->GetSize() == 4);
        Assert(instr->GetSrc1()->GetSize() == 4);
        // fall through.

    case Js::OpCode::MOV:
        bytes = this->EmitOp2Register(Emitter, instr, EmitMovRegister, EmitMovRegister64);
        break;

    case Js::OpCode::MOVK:
        bytes = this->EmitMovConstant(Emitter, instr, EmitMovk, EmitMovk64);
        break;
    
    case Js::OpCode::MOVN:
        bytes = this->EmitMovConstant(Emitter, instr, EmitMovn, EmitMovn64);
        break;

    case Js::OpCode::MOVZ:
        bytes = this->EmitMovConstant(Emitter, instr, EmitMovz, EmitMovz64);
        break;

    case Js::OpCode::MRS_FPCR:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        Assert(dst->GetSize() == 4);
        bytes = EmitMrs(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ARM64_FPCR);
        break;

    case Js::OpCode::MRS_FPSR:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        Assert(dst->GetSize() == 4);
        bytes = EmitMrs(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ARM64_FPSR);
        break;

    case Js::OpCode::MSR_FPCR:
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        Assert(src1->GetSize() == 4);
        bytes = EmitMsr(Emitter, this->GetRegEncode(src1->AsRegOpnd()), ARM64_FPCR);
        break;

    case Js::OpCode::MSR_FPSR:
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        Assert(src1->GetSize() == 4);
        bytes = EmitMsr(Emitter, this->GetRegEncode(src1->AsRegOpnd()), ARM64_FPSR);
        break;

    case Js::OpCode::MUL:
        bytes = this->EmitOp3Register(Emitter, instr, EmitMul, EmitMul64);
        break;

    case Js::OpCode::MVN:
        bytes = this->EmitOp2Register(Emitter, instr, EmitMvnRegister, EmitMvnRegister64);
        break;

    // SMULL dst, src1, src2. src1 and src2 are 32-bit. dst is 64-bit.
    case Js::OpCode::SMULL:
        dst = instr->GetDst();
        src1 = instr->GetSrc1();
        src2 = instr->GetSrc2();
        Assert(dst->IsRegOpnd());
        Assert(src1->IsRegOpnd());
        Assert(src2->IsRegOpnd());

        Assert(dst->GetSize() == 8);
        Assert(src1->GetSize() == 4);
        Assert(src2->GetSize() == 4);
        bytes = EmitSmull(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        break;

    // SMADDL (SMLAL from ARM32) dst, dst, src1, src2. src1 and src2 are 32-bit. dst is 64-bit.
    case Js::OpCode::SMADDL:
        dst = instr->GetDst();
        src1 = instr->GetSrc1();
        src2 = instr->GetSrc2();
        Assert(dst->IsRegOpnd());
        Assert(src1->IsRegOpnd());
        Assert(src2->IsRegOpnd());
        bytes = EmitSmaddl(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        break;

    // MSUB (MLS from ARM32) dst, src1, src2: Multiply and Subtract. We use 3 registers: dst = src1 - src2 * dst
    case Js::OpCode::MSUB:
        dst = instr->GetDst();
        src1 = instr->GetSrc1();
        src2 = instr->GetSrc2();
        Assert(dst->IsRegOpnd());
        Assert(src1->IsRegOpnd());
        Assert(src2->IsRegOpnd());
        bytes = EmitMsub(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
        break;

    case Js::OpCode::NOP:
        bytes = EmitNop(Emitter);
        break;

    case Js::OpCode::ORR:
        bytes = this->EmitOp3RegisterOrImmediate(Emitter, instr, EmitOrrRegister, EmitOrrRegister64, EmitOrrImmediate, EmitOrrImmediate64);
        break;

    case Js::OpCode::PLD:
        bytes = this->EmitPrefetch(Emitter, instr, instr->GetSrc1());
        break;

    case Js::OpCode::RET:
        bytes = this->EmitOp1Register64(Emitter, instr, EmitRet);
        break;

    // Legalizer should convert this to SDIV/MSUB before getting here
    case Js::OpCode::REM:
        Assert(false);
        break;

    case Js::OpCode::SBFX:
        bytes = this->EmitBitfield(Emitter, instr, EmitSbfx, EmitSbfx64);
        break;

    case Js::OpCode::SDIV:
        bytes = this->EmitOp3Register(Emitter, instr, EmitSdiv, EmitSdiv64);
        break;

    case Js::OpCode::STR:
        bytes = this->EmitLoadStore(Emitter, instr, instr->GetDst(), instr->GetSrc1(), EmitStrbRegister, EmitStrhRegister, EmitStrRegister, EmitStrRegister64, EmitStrbOffset, EmitStrhOffset, EmitStrOffset, EmitStrOffset64);
        break;

    case Js::OpCode::STP:
        bytes = this->EmitLoadStorePair(Emitter, instr, instr->GetDst(), instr->GetSrc1(), instr->GetSrc2(), EmitStpOffset, EmitStpOffset64);
        break;

    case Js::OpCode::STP_PRE:
        bytes = this->EmitLoadStorePair(Emitter, instr, instr->GetDst(), instr->GetSrc1(), instr->GetSrc2(), EmitStpOffsetPreIndex, EmitStpOffsetPreIndex64);
        break;

    case Js::OpCode::SUB:
        bytes = this->EmitOp3RegisterOrImmediateExtendSPReg(Emitter, instr, EmitSubRegister, EmitSubRegister64, EmitSubImmediate, EmitSubImmediate64);
        break;

    case Js::OpCode::SUBS:
        // ADDS and SUBS have no valid encoding where dst == RegSP
        Assert(instr->GetDst()->AsRegOpnd()->GetReg() != RegSP);
        bytes = this->EmitOp3RegisterOrImmediateExtendSPReg(Emitter, instr, EmitSubsRegister, EmitSubsRegister64, EmitSubsImmediate, EmitSubsImmediate64);
        break;

    case Js::OpCode::SUB_LSL4:
        bytes = this->EmitOp3RegisterShifted(Emitter, instr, EXTEND_UXTX, 4, EmitSubRegister, EmitSubRegister64);
        break;

    case Js::OpCode::TBZ:
        bytes = this->EmitTestAndBranch(Emitter, instr, EmitTbz);
        break;

    case Js::OpCode::TBNZ:
        bytes = this->EmitTestAndBranch(Emitter, instr, EmitTbnz);
        break;

    // Legalizer should convert this to ANDS before getting here
    case Js::OpCode::TST:
        Assert(false);
        break;

    case Js::OpCode::UBFX:
        bytes = this->EmitBitfield(Emitter, instr, EmitUbfx, EmitUbfx64);
        break;

    case Js::OpCode::FABS:
        bytes = this->EmitOp2FpRegister(Emitter, instr, EmitNeonFabs);
        break;

    case Js::OpCode::FADD:
        bytes = this->EmitOp3FpRegister(Emitter, instr, EmitNeonFadd);
        break;

    case Js::OpCode::FCMP:
        bytes = this->EmitOp2FpRegister(Emitter, instr->GetSrc1(), instr->GetSrc2(), EmitNeonFcmp);
        break;

    case Js::OpCode::FCSELEQ:
        bytes = this->EmitConditionalSelectFp(Emitter, instr, COND_EQ, EmitNeonFcsel);
        break;

    case Js::OpCode::FCSELNE:
        bytes = this->EmitConditionalSelectFp(Emitter, instr, COND_NE, EmitNeonFcsel);
        break;

    case Js::OpCode::FCVT:
        dst = instr->GetDst();
        src1 = instr->GetSrc1();
        Assert(dst->IsRegOpnd());
        Assert(src1->IsRegOpnd());
        Assert(dst->IsFloat());

        size = dst->GetSize();
        Assert(size == 4 || size == 8);

        if (src1->IsFloat())
        {
            bytes = EmitNeonFcvt(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), (size == 8) ? SIZE_1D : SIZE_1S, this->GetFloatRegEncode(src1->AsRegOpnd()), (src1->GetSize() == 8) ? SIZE_1D : SIZE_1S);
        }
        else if (src1->GetType() == TyInt32)
        {
            bytes = EmitNeonScvtf(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), Arm64SimpleRegisterParam(this->GetRegEncode(src1->AsRegOpnd())), (size == 8) ? SIZE_1D : SIZE_1S);
        }
        else if (src1->GetType() == TyUint32)
        {
            bytes = EmitNeonUcvtf(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), Arm64SimpleRegisterParam(this->GetRegEncode(src1->AsRegOpnd())), (size == 8) ? SIZE_1D : SIZE_1S);
        }
        else if (src1->GetType() == TyInt64)
        {
            bytes = EmitNeonScvtf64(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), Arm64SimpleRegisterParam(this->GetRegEncode(src1->AsRegOpnd())), (size == 8) ? SIZE_1D : SIZE_1S);
        }
        else if (src1->GetType() == TyUint64)
        {
            bytes = EmitNeonUcvtf64(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), Arm64SimpleRegisterParam(this->GetRegEncode(src1->AsRegOpnd())), (size == 8) ? SIZE_1D : SIZE_1S);
        }
        break;

    case Js::OpCode::FCVTM:
        bytes = this->EmitConvertToInt(Emitter, instr, EmitNeonFcvtmsGen, EmitNeonFcvtmuGen, EmitNeonFcvtmsGen64, EmitNeonFcvtmuGen64);
        break;

    case Js::OpCode::FCVTN:
        bytes = this->EmitConvertToInt(Emitter, instr, EmitNeonFcvtnsGen, EmitNeonFcvtnuGen, EmitNeonFcvtnsGen64, EmitNeonFcvtnuGen64);
        break;

    case Js::OpCode::FCVTP:
        bytes = this->EmitConvertToInt(Emitter, instr, EmitNeonFcvtpsGen, EmitNeonFcvtpuGen, EmitNeonFcvtpsGen64, EmitNeonFcvtpuGen64);
        break;

    case Js::OpCode::FCVTZ:
        bytes = this->EmitConvertToInt(Emitter, instr, EmitNeonFcvtzsGen, EmitNeonFcvtzuGen, EmitNeonFcvtzsGen64, EmitNeonFcvtzuGen64);
        break;

    case Js::OpCode::FDIV:
        bytes = this->EmitOp3FpRegister(Emitter, instr, EmitNeonFdiv);
        break;

    case Js::OpCode::FLDR:
        bytes = this->EmitLoadStoreFp(Emitter, instr, instr->GetSrc1(), instr->GetDst(), EmitNeonLdrOffset);
        break;

    // Note: src2 is really the second destination register, due to limitations of IR::Instr
    case Js::OpCode::FLDP:
        bytes = this->EmitLoadStoreFpPair(Emitter, instr, instr->GetSrc1(), instr->GetDst(), instr->GetSrc2(), EmitNeonLdpOffset);
        break;

    case Js::OpCode::FMIN:
        bytes = this->EmitOp3FpRegister(Emitter, instr, EmitNeonFmin);
        break;

    case Js::OpCode::FMAX:
        bytes = this->EmitOp3FpRegister(Emitter, instr, EmitNeonFmax);
        break;

    case Js::OpCode::FMOV:
        bytes = this->EmitOp2FpRegister(Emitter, instr, EmitNeonFmov);
        break;

    case Js::OpCode::FMOV_GEN:
        dst = instr->GetDst();
        src1 = instr->GetSrc1();
        Assert(dst->IsRegOpnd());
        Assert(src1->IsRegOpnd());

        size = dst->GetSize();
        Assert(size == 4 || size == 8);
        Assert(size == src1->GetSize());

        Assert(dst->IsFloat() != src1->IsFloat());
        if (dst->IsFloat())
        {
            bytes = EmitNeonIns(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), 0, this->GetRegEncode(src1->AsRegOpnd()), (size == 8) ? SIZE_1D : SIZE_1S);
        }
        else
        {
            if (size == 8)
            { 
                bytes = EmitNeonUmov64(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), 0, (size == 8) ? SIZE_1D : SIZE_1S);
            }
            else
            {
                bytes = EmitNeonUmov(Emitter, this->GetFloatRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), 0, (size == 8) ? SIZE_1D : SIZE_1S);
            }
        }
        break;

    case Js::OpCode::FMUL:
        bytes = this->EmitOp3FpRegister(Emitter, instr, EmitNeonFmul);
        break;

    case Js::OpCode::FNEG:
        bytes = this->EmitOp2FpRegister(Emitter, instr, EmitNeonFneg);
        break;

    case Js::OpCode::FRINTM:
        bytes = this->EmitOp2FpRegister(Emitter, instr, EmitNeonFrintm);
        break;

    case Js::OpCode::FRINTP:
        bytes = this->EmitOp2FpRegister(Emitter, instr, EmitNeonFrintp);
        break;

    case Js::OpCode::FSUB:
        bytes = this->EmitOp3FpRegister(Emitter, instr, EmitNeonFsub);
        break;

    case Js::OpCode::FSQRT:
        bytes = this->EmitOp2FpRegister(Emitter, instr, EmitNeonFsqrt);
        break;

    case Js::OpCode::FSTR:
        bytes = this->EmitLoadStoreFp(Emitter, instr, instr->GetDst(), instr->GetSrc1(), EmitNeonStrOffset);
        break;

    case Js::OpCode::FSTP:
        bytes = this->EmitLoadStoreFpPair(Emitter, instr, instr->GetDst(), instr->GetSrc1(), instr->GetSrc2(), EmitNeonStpOffset);
        break;

    // Opcode not yet implemented
    default:
#if DBG_DUMP
        instr->Dump();
        Output::Flush();
#endif
        AssertMsg(UNREACHED, "Unsupported Instruction Form");
        break;

    }

    Assert(bytes != 0);

    return Emitter.Opcode();
}

#ifdef INSERT_NOPS
ptrdiff_t insertNops(BYTE *pc, DWORD outInstr, uint count, uint size)
{
        //Insert count nops in the beginning
        for(int i = 0; i < count;i++)
        {
            *(DWORD *)(pc + i * sizeof(DWORD)) = 0x8000F3AF;
        }

        if (size == sizeof(ENCODE_16))
        {
            *(ENCODE_16 *)(pc + count * sizeof(DWORD)) = (ENCODE_16)(outInstr & 0x0000ffff);
            *(ENCODE_16 *)(pc + sizeof(ENCODE_16) + count * sizeof(DWORD)) = (ENCODE_16)(0xBF00);
        }
        else
        {
            Assert(size == sizeof(DWORD));
            *(DWORD *)(pc + count * sizeof(DWORD)) = outInstr;
        }

        //Insert count nops at the end;
        for(int i = count + 1; i < (2 *count + 1); i++)
        {
            *(DWORD *)(pc + i * sizeof(DWORD)) = 0x8000F3AF;
        }

        return MachInt*(2*count + 1);
}
#endif //INSERT_NOPS

///----------------------------------------------------------------------------
///
/// EncoderMD::Encode
///
///     Emit the ARM encoding for the given instruction in the passed in
///     buffer ptr.
///
///----------------------------------------------------------------------------

ptrdiff_t
EncoderMD::Encode(IR::Instr *instr, BYTE *pc, BYTE* beginCodeAddress)
{
    m_pc = pc;

    DWORD  outInstr;

    // Instructions must be lowered, we don't handle non-MD opcodes here.
    Assert(instr != nullptr);

    if (instr->IsLowered() == false)
    {
        if (instr->IsLabelInstr())
        {
            if (instr->isInlineeEntryInstr)
            {
                size_t inlineeOffset = m_pc - m_encoder->m_encodeBuffer;
                size_t argCount = instr->AsLabelInstr()->GetOffset();
                Assert(inlineeOffset == (inlineeOffset & 0x0FFFFFFF));

                intptr_t inlineeCallInfo = 0;
                const bool encodeResult = Js::InlineeCallInfo::Encode(inlineeCallInfo, argCount, inlineeOffset);
                Assert(encodeResult);
                //We are re-using offset to save the inlineeCallInfo which will be patched in ApplyRelocs
                //This is a cleaner way to patch MOVW\MOVT pair with the right inlineeCallInfo
                instr->AsLabelInstr()->ResetOffset((uintptr_t)inlineeCallInfo);
            }
            else
            {
                instr->AsLabelInstr()->SetPC(m_pc);
                m_func->m_unwindInfo.SetLabelOffset(instr->AsLabelInstr()->m_id, DWORD(m_pc - m_encoder->m_encodeBuffer));
            }
        }
    #if DBG_DUMP
        if (instr->IsEntryInstr() && Js::Configuration::Global.flags.DebugBreak.Contains(m_func->GetFunctionNumber()))
        {
            IR::Instr *int3 = IR::Instr::New(Js::OpCode::DEBUGBREAK, m_func);
            return this->Encode(int3, m_pc);
        }
    #endif
        return 0;
    }

    this->CanonicalizeInstr(instr);

    outInstr = GenerateEncoding(instr, m_pc);

    if (outInstr == 0)
    {
        return 0;
    }

    // TODO: Check if VFP/Neon instructions in Thumb-2 mode we need to swap the instruction halfwords
#ifdef INSERT_NOPS
    return insertNops(m_pc, outInstr, CountNops, sizeof(DWORD));
#else
    *(DWORD *)m_pc = outInstr ;
    return MachInt;
#endif
}

bool
EncoderMD::EncodeLogicalConst(IntConstType constant, DWORD * result, int size = 4)
{
    *result = FindArm64LogicalImmediateEncoding(constant, size);
    return (*result != ARM64_LOGICAL_IMMEDIATE_NO_ENCODING);
}

bool
EncoderMD::CanEncodeLogicalConst(IntConstType constant, int size)
{
    DWORD encode;
    return EncodeLogicalConst(constant, &encode, size);
}

///----------------------------------------------------------------------------
///
/// EncodeReloc::New
///
///----------------------------------------------------------------------------

void
EncodeReloc::New(EncodeReloc **pHead, RelocType relocType, BYTE *offset, IR::Instr *relocInstr, ArenaAllocator *alloc)
{
    EncodeReloc *newReloc      = AnewStruct(alloc, EncodeReloc);
    newReloc->m_relocType      = relocType;
    newReloc->m_consumerOffset = offset;
    newReloc->m_next           = *pHead;
    newReloc->m_relocInstr     = relocInstr;
    *pHead                     = newReloc;
}

void
EncoderMD::BaseAndOffsetFromSym(IR::SymOpnd *symOpnd, RegNum *pBaseReg, int32 *pOffset, Func * func)
{
    StackSym *stackSym = symOpnd->m_sym->AsStackSym();

    RegNum baseReg = func->GetLocalsPointer();
    int32 offset = stackSym->m_offset + symOpnd->m_offset;
    if (baseReg == RegSP)
    {
        // SP points to the base of the argument area. Non-reg SP points directly to the locals.
        offset += (func->m_argSlotsForFunctionsCalled * MachRegInt);
    }

    if (func->HasInlinee())
    {
        Assert(func->HasInlinee());
        if ((!stackSym->IsArgSlotSym() || stackSym->m_isOrphanedArg) && !stackSym->IsParamSlotSym())
        {
            offset += func->GetInlineeArgumentStackSize();
        }
    }

    if (stackSym->IsParamSlotSym())
    {
        offset += func->m_localStackHeight + func->m_ArgumentsOffset;
        if (!EncoderMD::CanEncodeLoadStoreOffset(offset))
        {
            // Use the frame pointer. No need to hoist an offset for a param.
            baseReg = FRAME_REG;
            offset = stackSym->m_offset + symOpnd->m_offset - (Js::JavascriptFunctionArgIndex_Frame * MachRegInt);
            Assert(EncoderMD::CanEncodeLoadStoreOffset(offset));
        }
    }
#ifdef DBG
    else
    {
        // Locals are offset by the size of the area allocated for stack args.
        Assert(offset >= 0);
        Assert(baseReg != RegSP || (uint)offset >= (func->m_argSlotsForFunctionsCalled * MachRegInt));

        if (func->HasInlinee())
        {
            // TODO (megupta): BaseReg will be a pre-reserved non SP register when we start supporting try
            Assert(baseReg == RegSP || baseReg == ALT_LOCALS_PTR);
            if (stackSym->IsArgSlotSym() && !stackSym->m_isOrphanedArg)
            {
                Assert(stackSym->m_isInlinedArgSlot);
                //Assert((uint)offset <= ((func->m_argSlotsForFunctionsCalled + func->GetMaxInlineeArgOutCount()) * MachRegInt));
            }
            else
            {
                AssertMsg(stackSym->IsAllocated(), "StackSym offset should be set");
                //Assert((uint)offset > ((func->m_argSlotsForFunctionsCalled + func->GetMaxInlineeArgOutCount()) * MachRegInt));
                //Assert(offset > (func->HasTry() ? (int32)func->GetMaxInlineeArgOutSize() : (int32)(func->m_argSlotsForFunctionsCalled * MachRegInt + func->GetMaxInlineeArgOutSize())));
            }
        }
        // TODO: restore the following assert (very useful) once we have a way to tell whether prolog/epilog
        // gen is complete.
        //Assert(offset < func->m_localStackHeight);
    }
#endif
    *pBaseReg = baseReg;
    *pOffset = offset;
}

///----------------------------------------------------------------------------
///
/// EncoderMD::ApplyRelocs
/// We apply relocations to the temporary buffer using the target buffer's address
/// before we copy the contents of the temporary buffer to the target buffer.
///----------------------------------------------------------------------------
void
EncoderMD::ApplyRelocs(size_t codeBufferAddress, size_t codeSize, uint* bufferCRC, BOOL isBrShorteningSucceeded, bool isFinalBufferValidation)
{
    for (EncodeReloc *reloc = m_relocList; reloc; reloc = reloc->m_next)
    {
        PULONG relocAddress = PULONG(reloc->m_consumerOffset);
        PULONG targetAddress = PULONG(reloc->m_relocInstr->AsLabelInstr()->GetPC());
        ULONG_PTR immediate;

        switch (reloc->m_relocType)
        {
        case RelocTypeBranch14:
        case RelocTypeBranch19:
        case RelocTypeBranch26:
            ArmBranchLinker::LinkRaw(relocAddress, targetAddress);
            break;

        case RelocTypeLabelAdr:
            Assert(!reloc->m_relocInstr->isInlineeEntryInstr);
            immediate = ULONG_PTR(targetAddress) - ULONG_PTR(relocAddress);
            Assert(IS_CONST_INT21(immediate));
            *relocAddress = (*relocAddress & ~(3 << 29)) | ULONG((immediate & 3) << 29);
            *relocAddress = (*relocAddress & ~(0x7ffff << 5)) | ULONG(((immediate >> 2) & 0x7ffff) << 5);
            break;

        case RelocTypeLabelImmed:
        {
            // read the shift from the encoded instruction.
            uint32 shift = ((*relocAddress & (0x3 << 21)) >> 21) * 16;
            uintptr_t fullvalue = ULONG_PTR(targetAddress) - ULONG_PTR(m_encoder->m_encodeBuffer) + ULONG_PTR(codeBufferAddress);
            immediate = (fullvalue >> shift) & 0xffff;

            // replace the immediate value in the encoded instruction.
            *relocAddress = (*relocAddress & ~(0xffff << 5)) | ULONG((immediate & 0xffff) << 5);
            break;
        }

        case RelocTypeLabel:
            *(ULONG_PTR*)relocAddress = ULONG_PTR(targetAddress) - ULONG_PTR(m_encoder->m_encodeBuffer) + ULONG_PTR(codeBufferAddress);
            break;

        default:
            // unexpected/unimplemented type
            Assert(UNREACHED);
        }
    }
}

void
EncoderMD::EncodeInlineeCallInfo(IR::Instr *instr, uint32 codeOffset)
{
     DebugOnly(IR::LabelInstr* inlineeStart = instr->AsLabelInstr());
     Assert((inlineeStart->GetOffset() & 0x0F) == inlineeStart->GetOffset());
     return;
}

bool EncoderMD::TryConstFold(IR::Instr *instr, IR::RegOpnd *regOpnd)
{
    Assert(regOpnd->m_sym->IsConst());

    if (instr->m_opcode == Js::OpCode::MOV)
    {
        if (instr->GetSrc1() != regOpnd)
        {
            return false;
        }
        if (!instr->GetDst()->IsRegOpnd())
        {
            return false;
        }

        IR::Opnd* constOpnd = regOpnd->m_sym->GetConstOpnd();
        if (constOpnd->GetSize() > regOpnd->GetSize())
        {
            return false;
        }

        instr->ReplaceSrc(regOpnd, constOpnd);
        LegalizeMD::LegalizeInstr(instr);

        return true;
    }
    else
    {
        return false;
    }
}

bool EncoderMD::TryFold(IR::Instr *instr, IR::RegOpnd *regOpnd)
{
    if (LowererMD::IsAssign(instr))
    {
        if (!instr->GetDst()->IsRegOpnd() || regOpnd != instr->GetSrc1())
        {
            return false;
        }
        IR::SymOpnd *symOpnd = IR::SymOpnd::New(regOpnd->m_sym, regOpnd->GetType(), instr->m_func);
        instr->ReplaceSrc(regOpnd, symOpnd);
        LegalizeMD::LegalizeInstr(instr);

        return true;
    }
    else
    {
        return false;
    }
}

void EncoderMD::AddLabelReloc(BYTE* relocAddress)
{
    Assert(relocAddress != nullptr);
    EncodeReloc::New(&m_relocList, RelocTypeLabel, relocAddress, *(IR::Instr**)relocAddress, m_encoder->m_tempAlloc);
}

