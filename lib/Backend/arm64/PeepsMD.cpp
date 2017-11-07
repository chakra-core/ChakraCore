//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

static const uint8 RegAttribs[RegNumCount] =
{
#define REGDAT(Name, ListName, Encode, Type, Attribs) Attribs,
#include "RegList.h"
#undef REGDAT
};


// PeepsMD::Init
void
PeepsMD::Init(Peeps *peeps)
{
    this->peeps = peeps;
}

// PeepsMD::ProcessImplicitRegs
void
PeepsMD::ProcessImplicitRegs(IR::Instr *instr)
{
    if (LowererMD::IsCall(instr))
    {
        FOREACH_REG(reg)
        {
            // not DONTALLOCATE, not CALLEESAVED
            if (RegAttribs[reg] == 0)
            {
                this->peeps->ClearReg(reg);
            }
        } NEXT_REG

        // The SCRATCH_REG (RegR17) is marked DONTALLOCATE, so it will be missed above.
        Assert(RegAttribs[SCRATCH_REG] & RA_DONTALLOCATE);
        this->peeps->ClearReg(SCRATCH_REG);
    }
    else if (instr->m_opcode == Js::OpCode::SMULL ||
             instr->m_opcode == Js::OpCode::SMADDL)
    {
        // As we don't currently have support for 4 operand instrs, we use R17 as 4th operand,
        // Notify the peeps that we use r17: SMULL, dst, r12, src1, src2.
        this->peeps->ClearReg(SCRATCH_REG);
    }
}

void
PeepsMD::PeepAssign(IR::Instr *instr)
{
    return;
}
