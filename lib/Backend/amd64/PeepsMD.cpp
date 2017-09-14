//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

// PeepsMD::Init
void
PeepsMD::Init(Peeps *peeps)
{
    this->peeps = peeps;
}

// PeepsMD::ProcessImplicitRegs
// Note: only do calls for now
void
PeepsMD::ProcessImplicitRegs(IR::Instr *instr)
{
    if (LowererMD::IsCall(instr))
    {
#define REGDAT(Name, Listing, Encode, Type, BitVec) \
        if (!((BitVec) & (RA_CALLEESAVE | RA_DONTALLOCATE))) \
        { \
            this->peeps->ClearReg(Reg ## Name); \
        }
#include "RegList.h"
    }
    else if (instr->m_opcode == Js::OpCode::IMUL)
    {
        this->peeps->ClearReg(RegRDX);
    }
    else if (instr->m_opcode == Js::OpCode::IDIV)
    {
        if (instr->GetDst()->AsRegOpnd()->GetReg() == RegRDX)
        {
            this->peeps->ClearReg(RegRAX);
        }
        else
        {
            Assert(instr->GetDst()->AsRegOpnd()->GetReg() == RegRAX);
            this->peeps->ClearReg(RegRDX);
        }
    }
    else if (instr->m_opcode == Js::OpCode::XCHG)
    {
        // At time of writing, I believe that src1 is always identical to dst, but clear both for robustness.

        // Either of XCHG's operands (but not both) can be a memory address, so only clear registers.
        if (instr->GetSrc1()->IsRegOpnd())
        {
            this->peeps->ClearReg(instr->GetSrc1()->AsRegOpnd()->GetReg());
        }
        if (instr->GetSrc2()->IsRegOpnd())
        {
            this->peeps->ClearReg(instr->GetSrc2()->AsRegOpnd()->GetReg());
        }
    }
}

void
PeepsMD::PeepAssign(IR::Instr *instr)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src = instr->GetSrc1();
    if(dst->IsRegOpnd() && instr->m_opcode == Js::OpCode::MOV)
    {
        if (src->IsImmediateOpnd() && src->GetImmediateValue(instr->m_func) == 0)
        {
            Assert(instr->GetSrc2() == NULL);

            // 32-bit XOR has a smaller encoding
            if (TySize[dst->GetType()] == MachPtr)
            {
                dst->SetType(TyInt32);
            }

            instr->m_opcode = Js::OpCode::XOR;
            instr->ReplaceSrc1(dst);
            instr->SetSrc2(dst);
        }
        else if (!instr->isInlineeEntryInstr)
        {
            if(src->IsIntConstOpnd() && src->GetSize() <= TySize[TyUint32])
            {
                dst->SetType(TyUint32);
                src->SetType(TyUint32);
            }
            else if(src->IsAddrOpnd() && (((size_t)src->AsAddrOpnd()->m_address >> 32) == 0 ))
            {
                instr->ReplaceSrc1(IR::IntConstOpnd::New(::Math::PointerCastToIntegral<UIntConstType>(src->AsAddrOpnd()->m_address), TyUint32, instr->m_func));
                dst->SetType(TyUint32);
            }
        }
    }
    else if (((instr->m_opcode == Js::OpCode::MOVSD || instr->m_opcode == Js::OpCode::MOVSS)
                && src->IsRegOpnd()
                && dst->IsRegOpnd()
                && (TySize[src->GetType()] == TySize[dst->GetType()]))
        || ((instr->m_opcode == Js::OpCode::MOVUPS)
                && src->IsRegOpnd()
                && dst->IsRegOpnd())
        || (instr->m_opcode == Js::OpCode::MOVAPD))
    {
        // MOVAPS has 1 byte shorter encoding
        instr->m_opcode = Js::OpCode::MOVAPS;
    }
    else if (instr->m_opcode == Js::OpCode::MOVSD_ZERO)
    {
        instr->m_opcode = Js::OpCode::XORPS;
        instr->SetSrc1(dst);
        instr->SetSrc2(dst);
    }
}


