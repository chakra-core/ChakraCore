//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

IR::Instr *Lowerer::PreLowerPeepInstr(IR::Instr *instr, IR::Instr **pInstrPrev)
{
    if (PHASE_OFF(Js::PreLowererPeepsPhase, this->m_func))
    {
        return instr;
    }

    switch (instr->m_opcode)
    {
#if defined(_M_IX86) || defined(_M_X64)
        // this sort of addressing mode magic only applies to x86 platforms
    case Js::OpCode::Add_I4:
        instr = this->PeepShiftAdd(instr);
        *pInstrPrev = instr->m_prev;
        break;
#endif
    case Js::OpCode::Shl_I4:
        instr = this->PeepShl(instr);
        *pInstrPrev = instr->m_prev;
        break;

    case Js::OpCode::BrTrue_I4:
    case Js::OpCode::BrFalse_I4:
        instr = this->PeepBrBool(instr);
        *pInstrPrev = instr->m_prev;
        break;
    }

    return instr;
}

IR::Instr *
Lowerer::TryShiftAdd(IR::Instr *instrAdd, IR::Opnd * opndFold, IR::Opnd * opndAdd)
{
    Assert(instrAdd->m_opcode == Js::OpCode::Add_I4);
    if (!opndFold->GetIsDead())
    {
        return instrAdd;
    }

    if (!(opndAdd->IsRegOpnd() || (opndAdd->IsInt32() && opndAdd->IsIntConstOpnd())))
    {
        return instrAdd;
    }

    if (!opndFold->IsRegOpnd() || !opndFold->AsRegOpnd()->m_sym->IsSingleDef())
    {
        return instrAdd;
    }

    IR::Instr * instrDef = opndFold->AsRegOpnd()->m_sym->GetInstrDef();

    if ((instrDef->m_opcode != Js::OpCode::Shl_I4 && instrDef->m_opcode != Js::OpCode::Mul_I4) || !instrDef->GetSrc1()->IsRegOpnd()  || !instrDef->GetSrc2()->IsIntConstOpnd())
    {
        return instrAdd;
    }

    if (instrDef->HasBailOutInfo())
    {
        return instrAdd;
    }

    byte scale = 0;
    IntConstType constVal = instrDef->GetSrc2()->AsIntConstOpnd()->GetValue();
    if (instrDef->m_opcode == Js::OpCode::Shl_I4)
    {
        if (constVal < 0 || constVal > 3)
        {
            return instrAdd;
        }
        scale = (byte)constVal;
    }
    else
    {
        Assert(instrDef->m_opcode == Js::OpCode::Mul_I4);
        switch (constVal)
        {
        case 1:
            scale = 0;
            break;
        case 2:
            scale = 1;
            break;
        case 4:
            scale = 2;
            break;
        case 8:
            scale = 3;
            break;
        default:
            return instrAdd;
        }
    }

    StackSym * defSrc1Sym = instrDef->GetSrc1()->AsRegOpnd()->m_sym;
    StackSym * foldSym = opndFold->AsRegOpnd()->m_sym;
    FOREACH_INSTR_IN_RANGE(instrIter, instrDef->m_next, instrAdd->m_prev)
    {
        // if any branch between def-use, don't do peeps on it because branch target might depend on the def
        if (instrIter->IsBranchInstr())
        {
            return instrAdd;
        }
        if (instrIter->HasBailOutInfo())
        {
            return instrAdd;
        }
        if (instrIter->FindRegDef(defSrc1Sym))
        {
            return instrAdd;
        }
        if (instrIter->FindRegUse(foldSym))
        {
            return instrAdd;
        }
    } NEXT_INSTR_IN_RANGE;

#if DBG_DUMP
    if (PHASE_TRACE(Js::PreLowererPeepsPhase, instrAdd->m_func))
    {
        Output::Print(_u("PeepShiftAdd : %s (%d) : folding:\n"), instrAdd->m_func->GetJITFunctionBody()->GetDisplayName(), instrAdd->m_func->GetFunctionNumber());
        instrDef->Dump();
        instrAdd->Dump();
    }
#endif

    IR::IndirOpnd * leaOpnd = nullptr;
    if (opndAdd->IsRegOpnd())
    {
        leaOpnd = IR::IndirOpnd::New(opndAdd->AsRegOpnd(), instrDef->UnlinkSrc1()->AsRegOpnd(), scale, opndFold->GetType(), instrAdd->m_func);
    }
    else
    {
        Assert(opndAdd->IsInt32() && opndAdd->IsIntConstOpnd());
        leaOpnd = IR::IndirOpnd::New(instrDef->UnlinkSrc1()->AsRegOpnd(), opndAdd->AsIntConstOpnd()->AsInt32(), scale, opndFold->GetType(), instrAdd->m_func);
    }

    IR::Instr * leaInstr = InsertLea(instrAdd->UnlinkDst()->AsRegOpnd(), leaOpnd, instrAdd);

#if DBG_DUMP
    if (PHASE_TRACE(Js::PreLowererPeepsPhase, instrAdd->m_func))
    {
        Output::Print(_u("into:\n"), instrAdd->m_func->GetJITFunctionBody()->GetDisplayName(), instrAdd->m_func->GetFunctionNumber());
        leaInstr->Dump();
    }
#endif

    instrAdd->Remove();
    instrDef->Remove();

    return leaInstr;
}

IR::Instr *
Lowerer::PeepShiftAdd(IR::Instr *instrAdd)
{
    Assert(instrAdd->m_opcode == Js::OpCode::Add_I4);

    // Peep:
    //    t1 = SHL X, 0|1|2|3    / t1 = MUL X, 1|2|4|8
    //    t2 = ADD t1, Y
    //
    // Into:
    //    t2 = LEA [X * scale + Y]

    if (instrAdd->HasBailOutInfo())
    {
        return instrAdd;
    }

    if (!instrAdd->GetDst()->IsRegOpnd())
    {
        return instrAdd;
    }

    IR::Opnd * src2 = instrAdd->GetSrc2();
    IR::Opnd * src1 = instrAdd->GetSrc1();

    // we can't remove t1 in case both srcs are uses of t1
    if (src1->IsEqual(src2))
    {
        return instrAdd;
    }
    IR::Instr * resultInstr = TryShiftAdd(instrAdd, src1, src2);
    if (resultInstr->m_opcode == Js::OpCode::Add_I4)
    {
        resultInstr = TryShiftAdd(instrAdd, src2, src1);
    }
    return resultInstr;

}

IR::Instr *Lowerer::PeepShl(IR::Instr *instrShl)
{
    IR::Opnd *src1;
    IR::Opnd *src2;
    IR::Instr *instrDef;

    src1 = instrShl->GetSrc1();
    src2 = instrShl->GetSrc2();

    // Peep:
    //    t1 = SHR X, cst
    //    t2 = SHL t1, cst
    //
    // Into:
    //    t2 = AND X, mask

    if (!src1->IsRegOpnd() || !src2->IsIntConstOpnd())
    {
        return instrShl;
    }
    if (!src1->AsRegOpnd()->m_sym->IsSingleDef())
    {
        return instrShl;
    }
    if (instrShl->HasBailOutInfo())
    {
        return instrShl;
    }
    instrDef = src1->AsRegOpnd()->m_sym->GetInstrDef();
    if (instrDef->m_opcode != Js::OpCode::Shr_I4 || !instrDef->GetSrc2()->IsIntConstOpnd()
        || instrDef->GetSrc2()->AsIntConstOpnd()->GetValue() != src2->AsIntConstOpnd()->GetValue()
        || !instrDef->GetSrc1()->IsRegOpnd())
    {
        return instrShl;
    }
    if (!src1->GetIsDead())
    {
        return instrShl;
    }
    if (instrDef->HasBailOutInfo())
    {
        return instrShl;
    }

    FOREACH_INSTR_IN_RANGE(instrIter, instrDef->m_next, instrShl->m_prev)
    {
        if (instrIter->HasBailOutInfo())
        {
            return instrShl;
        }
        if (instrIter->FindRegDef(instrDef->GetSrc1()->AsRegOpnd()->m_sym))
        {
            return instrShl;
        }
        if (instrIter->FindRegUse(src1->AsRegOpnd()->m_sym))
        {
            return instrShl;
        }
        // if any branch between def-use, don't do peeps on it because branch target might depend on the def
        if (instrIter->IsBranchInstr())
        {
            return instrShl;
        }
    } NEXT_INSTR_IN_RANGE;

    instrShl->FreeSrc1();
    instrShl->SetSrc1(instrDef->UnlinkSrc1());
    instrDef->Remove();

    IntConstType oldValue = src2->AsIntConstOpnd()->GetValue();

    // Left shift operator (<<) on arm32 is implemented by LSL which doesn't discard bits beyond lowerest 5-bit.
    // Need to discard such bits to conform to << in JavaScript. This is not a problem for x86 and x64 because
    // behavior of SHL is consistent with JavaScript but keep the below line for clarity.
    oldValue %= sizeof(int32) * 8;

    oldValue = ~((1 << oldValue) - 1);
    src2->AsIntConstOpnd()->SetValue(oldValue);

    instrShl->m_opcode = Js::OpCode::And_I4;

    return instrShl;
}

IR::Instr *Lowerer::PeepBrBool(IR::Instr *instrBr)
{
    IR::Opnd *src1;
    IR::Instr *instrBinOp, *instrCm1, *instrCm2;

    // Peep:
    //    t1 = CmCC_I4 a, b
    //    t2 = CmCC_i4 c, d
    //    t3 = AND/OR t1, t2
    //    BrTrue/False t3, $L_true
    //
    // Into:
    //    BrCC a, b,  $L_true/false
    //    BrCC c, d,  $L_true
    //$L_false:

    src1 = instrBr->GetSrc1();
    if (!src1->IsRegOpnd())
    {
        return instrBr;
    }
    Assert(!instrBr->HasBailOutInfo());

    instrBinOp = instrBr->GetPrevRealInstrOrLabel();
    if (instrBinOp->m_opcode != Js::OpCode::And_I4 && instrBinOp->m_opcode != Js::OpCode::Or_I4)
    {
        return instrBr;
    }
    if (!instrBinOp->GetDst()->IsEqual(src1))
    {
        return instrBr;
    }
    IR::RegOpnd *src1Reg = src1->AsRegOpnd();
    if (!src1Reg->m_sym->IsSingleDef() || !src1Reg->GetIsDead())
    {
        return instrBr;
    }
    Assert(!instrBinOp->HasBailOutInfo());

    instrCm2 = instrBinOp->GetPrevRealInstrOrLabel();
    if (!instrCm2->IsCmCC_I4())
    {
        return instrBr;
    }
    IR::RegOpnd *cm2DstReg = instrCm2->GetDst()->AsRegOpnd();
    if (!cm2DstReg->m_sym->IsSingleDef())
    {
        return instrBr;
    }
    if (cm2DstReg->IsEqual(instrBinOp->GetSrc1()))
    {
        if (!instrBinOp->GetSrc1()->AsRegOpnd()->GetIsDead())
        {
            return instrBr;
        }
    }
    else if (cm2DstReg->IsEqual(instrBinOp->GetSrc2()))
    {
        if (!instrBinOp->GetSrc2()->AsRegOpnd()->GetIsDead())
        {
            return instrBr;
        }
    }
    else
    {
        return instrBr;
    }

    Assert(!instrCm2->HasBailOutInfo());

    instrCm1 = instrCm2->GetPrevRealInstrOrLabel();
    if (!instrCm1->IsCmCC_I4())
    {
        return instrBr;
    }
    Assert(!instrCm1->GetDst()->IsEqual(instrCm2->GetDst()));

    IR::RegOpnd *cm1DstReg = instrCm1->GetDst()->AsRegOpnd();
    if (!cm1DstReg->m_sym->IsSingleDef())
    {
        return instrBr;
    }
    if (cm1DstReg->IsEqual(instrCm2->GetSrc1()) || cm1DstReg->IsEqual(instrCm2->GetSrc2()))
    {
        return instrBr;
    }
    if (cm1DstReg->IsEqual(instrBinOp->GetSrc1()))
    {
        if (!instrBinOp->GetSrc1()->AsRegOpnd()->GetIsDead())
        {
            return instrBr;
        }
    }
    else if (cm1DstReg->IsEqual(instrBinOp->GetSrc2()))
    {
        if (!instrBinOp->GetSrc2()->AsRegOpnd()->GetIsDead())
        {
            return instrBr;
        }
    }
    else
    {
        return instrBr;
    }

    Assert(!instrCm1->HasBailOutInfo());

    IR::LabelInstr *falseLabel = instrBr->AsBranchInstr()->GetTarget();
    IR::LabelInstr *trueLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    instrBr->InsertAfter(trueLabel);
    IR::BranchInstr *instrBr1;
    IR::BranchInstr *instrBr2;

    if (instrBinOp->m_opcode == Js::OpCode::And_I4)
    {
        instrBr1 = instrCm1->ChangeCmCCToBranchInstr(instrBr->m_opcode == Js::OpCode::BrFalse_I4 ? falseLabel : trueLabel);
        instrBr1->Invert();
        instrBr2 = instrCm2->ChangeCmCCToBranchInstr(falseLabel);
        if (instrBr->m_opcode == Js::OpCode::BrFalse_I4)
        {
            instrBr2->Invert();
        }
    }
    else
    {
        Assert(instrBinOp->m_opcode == Js::OpCode::Or_I4);

        instrBr1 = instrCm1->ChangeCmCCToBranchInstr(instrBr->m_opcode == Js::OpCode::BrTrue_I4 ? falseLabel : trueLabel);
        instrBr2 = instrCm2->ChangeCmCCToBranchInstr(falseLabel);
        if (instrBr->m_opcode == Js::OpCode::BrFalse_I4)
        {
            instrBr2->Invert();
        }
    }

    instrBinOp->Remove();
    instrBr->Remove();

    return instrBr2;
}
