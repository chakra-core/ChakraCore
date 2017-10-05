//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "ARM64Encoder.h"
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

const BYTE
EncoderMD::GetRegEncode(IR::RegOpnd *regOpnd)
{
    return GetRegEncode(regOpnd->GetReg());
}

const BYTE
EncoderMD::GetRegEncode(RegNum reg)
{
    return RegEncode[reg];
}

const BYTE
EncoderMD::GetFloatRegEncode(IR::RegOpnd *regOpnd)
{
    BYTE regEncode = GetRegEncode(regOpnd->GetReg());
    AssertMsg(regEncode <= LAST_FLOAT_REG_NUM, "Impossible to allocate higher registers on VFP");
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
    RegNum baseReg;
    int32 offset;

    IR::Opnd* src1 = instr->UnlinkSrc1();

    if (src1->IsSymOpnd())
    {
        // We may as well turn this LEA into the equivalent ADD instruction and let the common ADD
        // logic handle it.
        IR::SymOpnd *symOpnd = src1->AsSymOpnd();

        this->BaseAndOffsetFromSym(symOpnd, &baseReg, &offset, this->m_func);
        symOpnd->Free(this->m_func);
        instr->SetSrc1(IR::RegOpnd::New(nullptr, baseReg, TyMachReg, this->m_func));
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
            instr->SetSrc2(IR::IntConstOpnd::New(offset, TyMachReg, this->m_func));
        }
        indirOpnd->Free(this->m_func);
    }
    instr->m_opcode = Js::OpCode::ADD;
}

bool
EncoderMD::DecodeMemoryOpnd(IR::Opnd* opnd, ARM64_REGISTER &baseRegResult, ARM64_REGISTER &indexRegResult, BYTE &indexScale, int32 &offset)
{
    RegNum baseReg;

    if (opnd->IsSymOpnd())
    {
        IR::SymOpnd *symOpnd = opnd->AsSymOpnd();

        this->BaseAndOffsetFromSym(symOpnd, &baseReg, &offset, this->m_func);
        baseRegResult = baseReg;
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

//---------------------------------------------------------------------------
//
// GenerateEncoding()
//
// generates the encoding for the specified tuple/form by applying the
// associated encoding steps
//
//---------------------------------------------------------------------------
ULONG
EncoderMD::GenerateEncoding(IR::Instr* instr, BYTE *pc, int32 size)
{
/*
    dst  = instr->GetDst();

    if(opcode == Js::OpCode::MLS)
    {
        Assert(instr->m_prev->GetDst()->IsRegOpnd() && (instr->m_prev->GetDst()->AsRegOpnd()->GetReg() == RegR12));
    }

    if (dst == nullptr || LowererMD::IsCall(instr))
    {
        opn = instr->GetSrc1();
        reg = opn;
    }
    else if (opcode == Js::OpCode::POP || opcode == Js::OpCode::VPOP)
    {
        opn = instr->GetSrc1();
        reg = dst;
    }
    else
    {
        opn = dst;
        reg = opn;
    }
*/



    Arm64LocalCodeEmitter<1> Emitter;
    ARM64_REGISTER indexReg;
    ARM64_REGISTER baseReg;
    ArmBranchLinker Linker;
    IR::Opnd* dst = 0;
    IR::Opnd* src1 = 0;
    IR::Opnd* src2 = 0;
    BYTE indexScale;
    int64 immediate;
    int32 offset;
    int shift;
    int bytes;

    switch (instr->m_opcode)
    {
    case Js::OpCode::ADD:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitAddImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitAddRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::ADD64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitAddImmediate64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), immediate);
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitAddRegister64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::ADDS:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitAddsImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitAddsRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::ADDS64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitAddsImmediate64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), immediate);
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitAddsRegister64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::AND:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitAndImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitAndRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::ASR:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitAsrImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitAsrRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;
    
    case Js::OpCode::B:
        EncodeReloc::New(&m_relocList, RelocTypeBranch26, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker);
        break;
    
    case Js::OpCode::BL:
        EncodeReloc::New(&m_relocList, RelocTypeBranch26, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBl(Emitter, Linker);
        break;
    
    case Js::OpCode::BR:
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        bytes = EmitBr(Emitter, this->GetRegEncode(src1->AsRegOpnd()));
        break;
    
    case Js::OpCode::BLR:
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        bytes = EmitBlr(Emitter, this->GetRegEncode(src1->AsRegOpnd()));
        break;
    
    // ARM64_WORKITEM: Legalizer needs to convert BIC with immediate to AND with inverted immediate
    case Js::OpCode::BIC:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        bytes = EmitBicRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        break;
    
    case Js::OpCode::BEQ:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_EQ);
        break;
    
    case Js::OpCode::BNE:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_NE);
        break;
    
    case Js::OpCode::BLT:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_LT);
        break;
    
    case Js::OpCode::BLE:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_LE);
        break;
    
    case Js::OpCode::BGT:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_GT);
        break;
    
    case Js::OpCode::BGE:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_GE);
        break;

    case Js::OpCode::BCS:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_CS);
        break;
    
    case Js::OpCode::BCC:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_CC);
        break;
    
    case Js::OpCode::BHI:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_HI);
        break;
    
    case Js::OpCode::BLS:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_LS);
        break;
    
    case Js::OpCode::BMI:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_MI);
        break;
    
    case Js::OpCode::BPL:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_PL);
        break;

    case Js::OpCode::BVS:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_VS);
        break;
    
    case Js::OpCode::BVC:
        EncodeReloc::New(&m_relocList, RelocTypeBranch19, m_pc, instr->AsBranchInstr()->GetTarget(), m_encoder->m_tempAlloc);
        Linker.SetTarget(Emitter);
        bytes = EmitBranch(Emitter, Linker, COND_VC);
        break;
    
    case Js::OpCode::DEBUGBREAK:
        bytes = EmitDebugBreak(Emitter);
        break;

    case Js::OpCode::CLZ:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        bytes = EmitClz(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
        break;

    // ARM64_WORKITEM: Should legalizer change this to SUBS before the encoder?
    case Js::OpCode::CMP:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        if (src1->IsImmediateOpnd())
        {
            immediate = src1->GetImmediateValue(instr->m_func);
            bytes = EmitSubsImmediate(Emitter, ARMREG_ZR, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src1->IsRegOpnd());
            bytes = EmitSubsRegister(Emitter, ARMREG_ZR, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
        }
        break;

    // ARM64_WORKITEM: Should legalizer change this to ADDS before the encoder?
    case Js::OpCode::CMN:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        if (src1->IsImmediateOpnd())
        {
            immediate = src1->GetImmediateValue(instr->m_func);
            bytes = EmitAddsImmediate(Emitter, ARMREG_ZR, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src1->IsRegOpnd());
            bytes = EmitAddsRegister(Emitter, ARMREG_ZR, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
        }
        break;

    case Js::OpCode::CMP_ASR31:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        bytes = EmitSubsRegister(Emitter, ARMREG_ZR, this->GetRegEncode(dst->AsRegOpnd()), Arm64RegisterParam(this->GetRegEncode(src1->AsRegOpnd()), SHIFT_ASR, 31));
        break;

    case Js::OpCode::EOR:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitEorImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitEorRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::EOR_ASR31:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        bytes = EmitEorRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), Arm64RegisterParam(this->GetRegEncode(src2->AsRegOpnd()), SHIFT_ASR, 31));
        break;

    // Legalizer should convert these into MOVZ/MOVN/MOVK
    case Js::OpCode::LDIMM:
        Assert(false);
        break;

    case Js::OpCode::LDR:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsIndirOpnd() || src1->IsSymOpnd());
        if (DecodeMemoryOpnd(src1, baseReg, indexReg, indexScale, offset))
        {
            bytes = EmitLdrRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else
        {
            bytes = EmitLdrOffset(Emitter, this->GetRegEncode(dst->AsRegOpnd()), baseReg, offset);
        }
        break;

    case Js::OpCode::LDR64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsIndirOpnd() || src1->IsSymOpnd());
        if (DecodeMemoryOpnd(src1, baseReg, indexReg, indexScale, offset))
        {
            bytes = EmitLdrRegister64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else
        {
            bytes = EmitLdrOffset64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), baseReg, offset);
        }
        break;

    // Note: src2 is really the second destination
    case Js::OpCode::LDP:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsIndirOpnd() || src1->IsSymOpnd());
        if (DecodeMemoryOpnd(src1, baseReg, indexReg, indexScale, offset))
        {
            bytes = 0;
        }
        else
        {
            bytes = EmitLdpOffset(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), baseReg, offset);
        }
        break;

    case Js::OpCode::LDP64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsIndirOpnd() || src1->IsSymOpnd());
        if (DecodeMemoryOpnd(src1, baseReg, indexReg, indexScale, offset))
        {
            bytes = 0;
        }
        else
        {
            bytes = EmitLdpOffset64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), baseReg, offset);
        }
        break;

    // Legalizer should convert these into MOV/ADD
    case Js::OpCode::LEA:
        Assert(false);
        break;
    
    case Js::OpCode::LSL:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitLslImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitLslRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;
        
    case Js::OpCode::LSR:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitLsrImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitLsrRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::MOV:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        bytes = EmitMovRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
        break;

    case Js::OpCode::MOVK64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsImmediateOpnd());
        immediate = src1->GetImmediateValue(instr->m_func);
        shift = 0;
        while ((immediate & 0xFFFF) != immediate)
        {
            immediate = ULONG64(immediate) >> 16;
            shift += 16;
        }
        bytes = EmitMovk64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate), shift);
        break;
    
    case Js::OpCode::MOVN:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsImmediateOpnd());
        immediate = src1->GetImmediateValue(instr->m_func);
        shift = 0;
        while ((immediate & 0xFFFF) != immediate)
        {
            immediate = ULONG64(immediate) >> 16;
            shift += 16;
        }
        bytes = EmitMovn(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate), shift);
        break;

    case Js::OpCode::MOVN64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsImmediateOpnd());
        immediate = src1->GetImmediateValue(instr->m_func);
        shift = 0;
        while ((immediate & 0xFFFF) != immediate)
        {
            immediate = ULONG64(immediate) >> 16;
            shift += 16;
        }
        bytes = EmitMovn64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate), shift);
        break;

    case Js::OpCode::MOVZ:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsImmediateOpnd());
        immediate = src1->GetImmediateValue(instr->m_func);
        shift = 0;
        while ((immediate & 0xFFFF) != immediate)
        {
            immediate = ULONG64(immediate) >> 16;
            shift += 16;
        }
        bytes = EmitMovz(Emitter, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate), shift);
        break;
    
    case Js::OpCode::MUL:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        bytes = EmitMul(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        break;

    // SMULL dst, src1, src2. src1 and src2 are 32-bit. dst is 64-bit.
    case Js::OpCode::SMULL:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        bytes = EmitSmull(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        break;

    // SMADDL (SMLAL from ARM32) dst, dst, src1, src2. src1 and src2 are 32-bit. dst is 64-bit.
    case Js::OpCode::SMADDL:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        bytes = EmitSmaddl(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        break;

    // MSUB (MLS from ARM32) dst, src1, src2: Multiply and Subtract. We use 3 registers: dst = src1 - src2 * dst
    case Js::OpCode::MSUB:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        bytes = EmitMsub(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), this->GetRegEncode(dst->AsRegOpnd()));
        break;

    case Js::OpCode::NOP:
        bytes = EmitNop(Emitter);
        break;

    case Js::OpCode::ORR:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitOrrImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitOrrRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::ORR64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitOrrImmediate64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitOrrRegister64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::PLD:
        src1 = instr->GetSrc1();
        Assert(src1->IsIndirOpnd() || src1->IsSymOpnd());
        if (DecodeMemoryOpnd(src1, baseReg, indexReg, indexScale, offset))
        {
            bytes = EmitPrfmRegister(Emitter, baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else
        {
            bytes = EmitPrfmOffset(Emitter, baseReg, offset);
        }
        break;

    case Js::OpCode::RET:
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        bytes = EmitRet(Emitter, this->GetRegEncode(src1->AsRegOpnd()));
        break;

    // Legalizer should convert these into SDIV/MSUB
    case Js::OpCode::REM:
        Assert(false);
        break;

    case Js::OpCode::SDIV:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        bytes = EmitSdiv(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        break;

    case Js::OpCode::STR:
        dst = instr->GetDst();
        Assert(dst->IsIndirOpnd() || dst->IsSymOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        if (DecodeMemoryOpnd(dst, baseReg, indexReg, indexScale, offset))
        {
            bytes = EmitStrRegister(Emitter, this->GetRegEncode(src1->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else
        {
            bytes = EmitStrOffset(Emitter, this->GetRegEncode(src1->AsRegOpnd()), baseReg, offset);
        }
        break;

    case Js::OpCode::STR64:
        dst = instr->GetDst();
        Assert(dst->IsIndirOpnd() || dst->IsSymOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        if (DecodeMemoryOpnd(dst, baseReg, indexReg, indexScale, offset))
        {
            bytes = EmitStrRegister64(Emitter, this->GetRegEncode(src1->AsRegOpnd()), baseReg, Arm64RegisterParam(indexReg, SHIFT_LSL, indexScale));
        }
        else
        {
            bytes = EmitStrOffset64(Emitter, this->GetRegEncode(src1->AsRegOpnd()), baseReg, offset);
        }
        break;

    // Note: src2 is really the second destination
    case Js::OpCode::STP:
        dst = instr->GetDst();
        Assert(dst->IsIndirOpnd() || dst->IsSymOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        if (DecodeMemoryOpnd(dst, baseReg, indexReg, indexScale, offset))
        {
            bytes = 0;
        }
        else
        {
            bytes = EmitStpOffset(Emitter, this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), baseReg, offset);
        }
        break;

    case Js::OpCode::STP64:
        dst = instr->GetDst();
        Assert(dst->IsIndirOpnd() || dst->IsSymOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        Assert(src2->IsRegOpnd());
        if (DecodeMemoryOpnd(dst, baseReg, indexReg, indexScale, offset))
        {
            bytes = 0;
        }
        else
        {
            bytes = EmitStpOffset64(Emitter, this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()), baseReg, offset);
        }
        break;

    case Js::OpCode::SUB:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitSubImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitSubRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::SUB64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitSubImmediate64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), immediate);
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitSubRegister64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::SUBS:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitSubsImmediate(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitSubsRegister(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    case Js::OpCode::SUBS64:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        Assert(src1->IsRegOpnd());
        src2 = instr->GetSrc2();
        if (src2->IsImmediateOpnd())
        {
            immediate = src2->GetImmediateValue(instr->m_func);
            bytes = EmitSubsImmediate64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), immediate);
        }
        else
        {
            Assert(src2->IsRegOpnd());
            bytes = EmitSubsRegister64(Emitter, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()), this->GetRegEncode(src2->AsRegOpnd()));
        }
        break;

    // ARM64_WORKITEM: Should legalizer change this to ANDS before the encoder?
    case Js::OpCode::TST:
        dst = instr->GetDst();
        Assert(dst->IsRegOpnd());
        src1 = instr->GetSrc1();
        if (src1->IsImmediateOpnd())
        {
            immediate = src1->GetImmediateValue(instr->m_func);
            bytes = EmitAndsImmediate(Emitter, ARMREG_ZR, this->GetRegEncode(dst->AsRegOpnd()), ULONG(immediate));
        }
        else
        {
            Assert(src1->IsRegOpnd());
            bytes = EmitAndsRegister(Emitter, ARMREG_ZR, this->GetRegEncode(dst->AsRegOpnd()), this->GetRegEncode(src1->AsRegOpnd()));
        }
        break;

    }

/*

MACRO(TIOFLW,  Reg1,       OpSideEffect,   0,  LEGAL_CMP1,     INSTR_TYPE(Forms_TIOFLW), D___)

// Pseudo-op that loads the size of the arg out area. A special op with no src is used so that the
// actual arg out size can be fixed up by the encoder.
MACRO(LDARGOUTSZ,Reg1,     0,              0,  LEGAL_REG1,     INSTR_TYPE(Forms_LDIMM), D___)

// Pseudo-op: dst = EOR src, src ASR #31
MACRO(CLRSIGN, Reg2,       0,              0,  LEGAL_REG2,     INSTR_TYPE(Forms_CLRSIGN), D___)

// Pseudo-op: dst = SUB src1, src2 ASR #31
MACRO(SBCMPLNT, Reg3,      0,              0,  LEGAL_REG3,     INSTR_TYPE(Forms_SBCMPLNT), D___)


//VFP instructions:
MACRO(VABS,     Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VABS),   D___)
MACRO(VADDF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VADDF64),   D___)
MACRO(VCMPF64,  Reg1,      OpSideEffect,   0,  LEGAL_CMP_SH,    INSTR_TYPE(Forms_VCMPF64),   D___)
MACRO(VCVTF64F32, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF64F32),   D___)
MACRO(VCVTF32F64, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF32F64),   D___)
MACRO(VCVTF64S32, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF64S32),   D___)
MACRO(VCVTF64U32, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF64U32),   D___)
MACRO(VCVTS32F64, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTS32F64),   D___)
MACRO(VCVTRS32F64, Reg2,   0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTRS32F64),   D___)
MACRO(VDIVF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VDIVF64),   D___)
MACRO(VLDR,     Reg2,      0,              0,  LEGAL_VLOAD,     INSTR_TYPE(Forms_VLDR),   DL__)
MACRO(VLDR32,   Reg2,      0,              0,  LEGAL_VLOAD,     INSTR_TYPE(Forms_VLDR32), DL__) //single precision float load
MACRO(VMOV,     Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VMOV),   DM__)
MACRO(VMOVARMVFP, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VMOVARMVFP),   DM__)
MACRO(VMRS,     Empty,     OpSideEffect,   0,  LEGAL_NONE,      INSTR_TYPE(Forms_VMRS),   D___)
MACRO(VMRSR,    Reg1,      OpSideEffect,   0,  LEGAL_NONE,      INSTR_TYPE(Forms_VMRSR),   D___)
MACRO(VMSR,     Reg1,      OpSideEffect,   0,  LEGAL_NONE,      INSTR_TYPE(Forms_VMSR),   D___)
MACRO(VMULF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VMULF64),   D___)
MACRO(VNEGF64,  Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VNEGF64),   D___)
MACRO(VPUSH,    Reg2,      OpSideEffect,   0,  LEGAL_VPUSHPOP,  INSTR_TYPE(Forms_VPUSH), DSUS)
MACRO(VPOP,     Reg2,      OpSideEffect,   0,  LEGAL_VPUSHPOP,  INSTR_TYPE(Forms_VPOP), DLUP)
MACRO(VSUBF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VSUBF64),   D___)
MACRO(VSQRT,    Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VSQRT),   D___)
MACRO(VSTR,     Reg2,      0,              0,  LEGAL_VSTORE,    INSTR_TYPE(Forms_VSTR),   DS__)
MACRO(VSTR32,   Reg2,      0,              0,  LEGAL_VSTORE,    INSTR_TYPE(Forms_VSTR32), DS__) //single precision float store
    }

*/

// ToDo (SaAgarwa) - Commented to compile debug build
/*
#if DBG
    if (!done)
    {
        instr->Dump();
        Output::Flush();
        AssertMsg(UNREACHED, "Unsupported Instruction Form");
    }
#endif
*/
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
    int    size = 0;

    // Instructions must be lowered, we don't handle non-MD opcodes here.
    Assert(instr != nullptr);

    if (instr->IsLowered() == false)
    {
        if (instr->IsLabelInstr())
        {
            if (instr->isInlineeEntryInstr)
            {
                intptr_t inlineeCallInfo = 0;
                const bool encodeResult = Js::InlineeCallInfo::Encode(inlineeCallInfo, instr->AsLabelInstr()->GetOffset(), m_pc - m_encoder->m_encodeBuffer);
                Assert(encodeResult);
                //We are re-using offset to save the inlineeCallInfo which will be patched in ApplyRelocs
                //This is a cleaner way to patch MOVW\MOVT pair with the right inlineeCallInfo
                instr->AsLabelInstr()->ResetOffset((uint32)inlineeCallInfo);
            }
            else
            {
                instr->AsLabelInstr()->SetPC(m_pc);
                if (instr->AsLabelInstr()->m_id == m_func->m_unwindInfo.GetPrologStartLabel())
                {
                    m_func->m_unwindInfo.SetPrologOffset(DWORD(m_pc - m_encoder->m_encodeBuffer));
                }
                else if (instr->AsLabelInstr()->m_id == m_func->m_unwindInfo.GetEpilogEndLabel())
                {
                    // This is the last instruction in the epilog. Any instructions that follow
                    // are separated code, so the unwind info will have to represent them as a function
                    // fragment. (If there's no separated code, then this offset will equal the total
                    // code size.)
                    m_func->m_unwindInfo.SetEpilogEndOffset(DWORD(m_pc - m_encoder->m_encodeBuffer - m_func->m_unwindInfo.GetPrologOffset()));
                }
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
    size = 4;

    outInstr = GenerateEncoding(instr, m_pc, size);

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
        if (func->HasInlinee())
        {
            Assert(func->HasInlinee());
            if ((!stackSym->IsArgSlotSym() || stackSym->m_isOrphanedArg) && !stackSym->IsParamSlotSym())
            {
                offset += func->GetInlineeArgumentStackSize();
            }
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
            Assert(baseReg == RegSP);
            if (stackSym->IsArgSlotSym() && !stackSym->m_isOrphanedArg)
            {
                Assert(stackSym->m_isInlinedArgSlot);
                //Assert((uint)offset <= ((func->m_argSlotsForFunctionsCalled + func->GetMaxInlineeArgOutCount()) * MachRegInt));
            }
            else
            {
                AssertMsg(stackSym->IsAllocated(), "StackSym offset should be set");
                //Assert((uint)offset > ((func->m_argSlotsForFunctionsCalled + func->GetMaxInlineeArgOutCount()) * MachRegInt));
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
#if 0

    // ARM64_WORKITEM

    for (EncodeReloc *reloc = m_relocList; reloc; reloc = reloc->m_next)
    {
        BYTE * relocAddress = reloc->m_consumerOffset;
        int64 pcrel;
        DWORD encode = *(DWORD*)relocAddress;
        switch (reloc->m_relocType)
        {
        case RelocTypeBranch20:
            {
                IR::LabelInstr * labelInstr = reloc->m_relocInstr->AsLabelInstr();
                Assert(!labelInstr->isInlineeEntryInstr);
                AssertMsg(labelInstr->GetPC() != nullptr, "Branch to unemitted label?");
                pcrel = (uint32)(labelInstr->GetPC() - reloc->m_consumerOffset);
                encode |= BranchOffset_T2_20(pcrel);
                *(uint32 *)relocAddress = encode;
                break;
            }

        case RelocTypeBranch24:
            {
                IR::LabelInstr * labelInstr = reloc->m_relocInstr->AsLabelInstr();
                Assert(!labelInstr->isInlineeEntryInstr);
                AssertMsg(labelInstr->GetPC() != nullptr, "Branch to unemitted label?");
                pcrel = (uint32)(labelInstr->GetPC() - reloc->m_consumerOffset);
                encode |= BranchOffset_T2_24(pcrel);
                *(DWORD *)relocAddress = encode;
                break;
            }

        case RelocTypeDataLabelLow:
            {
                IR::LabelInstr * labelInstr = reloc->m_relocInstr->AsLabelInstr();
                Assert(!labelInstr->isInlineeEntryInstr && labelInstr->m_isDataLabel);

                AssertMsg(labelInstr->GetPC() != nullptr, "Branch to unemitted label?");

                pcrel = ((labelInstr->GetPC() - m_encoder->m_encodeBuffer + codeBufferAddress) & 0xFFFF);

                if (!EncodeImmediate16(pcrel, (DWORD*) &encode))
                {
                    Assert(UNREACHED);
                }
                *(DWORD *) relocAddress = encode;
                break;
            }

        case RelocTypeLabelLow:
            {
                // Absolute (not relative) label address (lower 16 bits)
                IR::LabelInstr * labelInstr = reloc->m_relocInstr->AsLabelInstr();
                if (!labelInstr->isInlineeEntryInstr)
                {
                    AssertMsg(labelInstr->GetPC() != nullptr, "Branch to unemitted label?");
                    // Note that the bottom bit must be set, since this is a Thumb code address.
                    pcrel = ((labelInstr->GetPC() - m_encoder->m_encodeBuffer + codeBufferAddress) & 0xFFFF) | 1;
                }
                else
                {
                    //This is a encoded low 16 bits.
                    pcrel = labelInstr->GetOffset() & 0xFFFF;
                }
                if (!EncodeImmediate16(pcrel, (DWORD*) &encode))
                {
                    Assert(UNREACHED);
                }
                *(DWORD *) relocAddress = encode;
                break;
            }

        case RelocTypeLabelHigh:
            {
                // Absolute (not relative) label address (upper 16 bits)
                IR::LabelInstr * labelInstr = reloc->m_relocInstr->AsLabelInstr();
                if (!labelInstr->isInlineeEntryInstr)
                {
                    AssertMsg(labelInstr->GetPC() != nullptr, "Branch to unemitted label?");
                    pcrel = (labelInstr->GetPC() - m_encoder->m_encodeBuffer + codeBufferAddress) >> 16;
                    // We only record the relocation on the low byte of the pair
                }
                else
                {
                    //This is a encoded high 16 bits.
                    pcrel = labelInstr->GetOffset() >> 16;
                }
                if (!EncodeImmediate16(pcrel, (DWORD*) &encode))
                {
                    Assert(UNREACHED);
                }
                *(DWORD *) relocAddress = encode;
                break;
            }

        case RelocTypeLabel:
            {
                IR::LabelInstr * labelInstr = reloc->m_relocInstr->AsLabelInstr();
                AssertMsg(labelInstr->GetPC() != nullptr, "Branch to unemitted label?");
                /* For Thumb instruction set -> OR 1 with the address*/
                *(uint32 *)relocAddress = (uint32)(labelInstr->GetPC() - m_encoder->m_encodeBuffer + codeBufferAddress)  | 1;
                break;
            }
        default:
            AssertMsg(UNREACHED, "Unknown reloc type");
        }
    }

#endif
}

void
EncoderMD::EncodeInlineeCallInfo(IR::Instr *instr, uint32 codeOffset)
{
     IR::LabelInstr* inlineeStart = instr->AsLabelInstr();
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

        instr->ReplaceSrc(regOpnd, regOpnd->m_sym->GetConstOpnd());
        LegalizeMD::LegalizeInstr(instr, false);

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
        LegalizeMD::LegalizeInstr(instr, false);

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

