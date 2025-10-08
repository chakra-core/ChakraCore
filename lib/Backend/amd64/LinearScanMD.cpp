//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "SccLiveness.h"

extern const IRType RegTypes[RegNumCount];

LinearScanMD::LinearScanMD(Func *func)
    : helperSpillSlots(nullptr),
      maxOpHelperSpilledLiveranges(0),
      func(func)
{
    this->byteableRegsBv.ClearAll();

    FOREACH_REG(reg)
    {
        if (LinearScan::GetRegAttribs(reg) & RA_BYTEABLE)
        {
            this->byteableRegsBv.Set(reg);
        }
    } NEXT_REG;

    memset((void*)this->xmmSymTable128, 0, sizeof(this->xmmSymTable128));
    memset((void*)this->xmmSymTable64, 0, sizeof(this->xmmSymTable64));
    memset((void*)this->xmmSymTable32, 0, sizeof(this->xmmSymTable32));
}

BitVector
LinearScanMD::FilterRegIntSizeConstraints(BitVector regsBv, BitVector sizeUsageBv) const
{
    // Requires byte-able reg?
    if (sizeUsageBv.Test(1))
    {
        regsBv.And(this->byteableRegsBv);
    }

    return regsBv;
}

bool
LinearScanMD::FitRegIntSizeConstraints(RegNum reg, BitVector sizeUsageBv) const
{
    // Requires byte-able reg?
    return !sizeUsageBv.Test(1) || this->byteableRegsBv.Test(reg);
}

bool
LinearScanMD::FitRegIntSizeConstraints(RegNum reg, IRType type) const
{
    // Requires byte-able reg?
    return TySize[type] != 1 || this->byteableRegsBv.Test(reg);
}

StackSym *
LinearScanMD::EnsureSpillSymForXmmReg(RegNum reg, Func *func, IRType type)
{
    Assert(REGNUM_ISXMMXREG(reg));

    __analysis_assume(reg - FIRST_XMM_REG < XMM_REGCOUNT);
    StackSym *sym;
    if (type == TyFloat32)
    {
        sym = this->xmmSymTable32[reg - FIRST_XMM_REG];
    }
    else if (type == TyFloat64)
    {
        sym = this->xmmSymTable64[reg - FIRST_XMM_REG];
    }
    else
    {
        Assert(IRType_IsSimd128(type));
        sym = this->xmmSymTable128[reg - FIRST_XMM_REG];
    }

    if (sym == nullptr)
    {
        sym = StackSym::New(type, func);
        func->StackAllocate(sym, TySize[type]);

        __analysis_assume(reg - FIRST_XMM_REG < XMM_REGCOUNT);

        if (type == TyFloat32)
        {
            this->xmmSymTable32[reg - FIRST_XMM_REG] = sym;
        }
        else if (type == TyFloat64)
        {
            this->xmmSymTable64[reg - FIRST_XMM_REG] = sym;
        }
        else
        {
            Assert(IRType_IsSimd128(type));
            this->xmmSymTable128[reg - FIRST_XMM_REG] = sym;
        }
    }

    return sym;
}

void
LinearScanMD::LegalizeConstantUse(IR::Instr * instr, IR::Opnd * opnd)
{
    Assert(opnd->IsAddrOpnd() || opnd->IsIntConstOpnd());
    intptr_t value = opnd->IsAddrOpnd() ? (intptr_t)opnd->AsAddrOpnd()->m_address : opnd->AsIntConstOpnd()->GetValue();
    if (value == 0
        && instr->m_opcode == Js::OpCode::MOV
        && !instr->GetDst()->IsRegOpnd()
        && TySize[opnd->GetType()] >= 4)
    {
        Assert(this->linearScan->instrUseRegs.IsEmpty());

        // MOV doesn't have an imm8 encoding for 32-bit/64-bit assignment, so if we have a register available,
        // we should hoist it and generate xor reg, reg and MOV dst, reg
        BitVector regsBv;
        regsBv.Copy(this->linearScan->activeRegs);
        regsBv.Or(this->linearScan->callSetupRegs);

        regsBv.ComplimentAll();
        regsBv.And(this->linearScan->int32Regs);
        regsBv.Minus(this->linearScan->tempRegs);       // Avoid tempRegs
        BVIndex regIndex = regsBv.GetNextBit();
        if (regIndex != BVInvalidIndex)
        {
            instr->HoistSrc1(Js::OpCode::MOV, (RegNum)regIndex);
            this->linearScan->instrUseRegs.Set(regIndex);
            this->func->m_regsUsed.Set(regIndex);

            // If we are in a loop, we need to mark the register being used by the loop so that
            // reload to that register will not be hoisted out of the loop
            this->linearScan->RecordLoopUse(nullptr, (RegNum)regIndex);
        }
    }
}

void
LinearScanMD::InsertOpHelperSpillAndRestores(SList<OpHelperBlock> *opHelperBlockList)
{
    if (maxOpHelperSpilledLiveranges)
    {
        Assert(!helperSpillSlots);
        helperSpillSlots = AnewArrayZ(linearScan->GetTempAlloc(), StackSym *, maxOpHelperSpilledLiveranges);
    }

    FOREACH_SLIST_ENTRY(OpHelperBlock, opHelperBlock, opHelperBlockList)
    {
        InsertOpHelperSpillsAndRestores(opHelperBlock);
    }
    NEXT_SLIST_ENTRY;
}

void
LinearScanMD::InsertOpHelperSpillsAndRestores(const OpHelperBlock& opHelperBlock)
{
    uint32 index = 0;

    FOREACH_SLIST_ENTRY(OpHelperSpilledLifetime, opHelperSpilledLifetime, &opHelperBlock.spilledLifetime)
    {
        // Use the original sym as spill slot if this is an inlinee arg
        StackSym* sym = nullptr;
        if (opHelperSpilledLifetime.spillAsArg)
        {
            sym = opHelperSpilledLifetime.lifetime->sym;
            AnalysisAssert(sym);
            Assert(sym->IsAllocated());
        }

        if (RegTypes[opHelperSpilledLifetime.reg] == TyFloat64)
        {
            IRType type = opHelperSpilledLifetime.lifetime->sym->GetType();
            IR::RegOpnd *regOpnd = IR::RegOpnd::New(nullptr, opHelperSpilledLifetime.reg, type, this->func);

            if (!sym)
            {
                sym = EnsureSpillSymForXmmReg(regOpnd->GetReg(), this->func, type);
            }

            IR::Instr   *pushInstr = IR::Instr::New(LowererMDArch::GetAssignOp(type), IR::SymOpnd::New(sym, type, this->func), regOpnd, this->func);
            opHelperBlock.opHelperLabel->InsertAfter(pushInstr);
            pushInstr->CopyNumber(opHelperBlock.opHelperLabel);
            if (opHelperSpilledLifetime.reload)
            {
                IR::Instr   *popInstr = IR::Instr::New(LowererMDArch::GetAssignOp(type), regOpnd, IR::SymOpnd::New(sym, type, this->func), this->func);
                opHelperBlock.opHelperEndInstr->InsertBefore(popInstr);
                popInstr->CopyNumber(opHelperBlock.opHelperEndInstr);
            }
        }
        else
        {
            Assert(helperSpillSlots);
            Assert(index < maxOpHelperSpilledLiveranges);

            if (!sym)
            {
                // Lazily allocate only as many slots as we really need.
                if (!helperSpillSlots[index])
                {
                    helperSpillSlots[index] = StackSym::New(TyMachReg, func);
                }

                sym = helperSpillSlots[index];
                index++;

                Assert(sym);
                func->StackAllocate(sym, MachRegInt);
            }
            IR::RegOpnd * regOpnd = IR::RegOpnd::New(nullptr, opHelperSpilledLifetime.reg, sym->GetType(), func);
            Lowerer::InsertMove(IR::SymOpnd::New(sym, sym->GetType(), func), regOpnd, opHelperBlock.opHelperLabel->m_next);
            if (opHelperSpilledLifetime.reload)
            {
                Lowerer::InsertMove(regOpnd, IR::SymOpnd::New(sym, sym->GetType(), func), opHelperBlock.opHelperEndInstr);
            }
        }
    }
    NEXT_SLIST_ENTRY;
}

void
LinearScanMD::EndOfHelperBlock(uint32 helperSpilledLiveranges)
{
    if (helperSpilledLiveranges > maxOpHelperSpilledLiveranges)
    {
        maxOpHelperSpilledLiveranges = helperSpilledLiveranges;
    }
}

void
LinearScanMD::GenerateBailOut(IR::Instr * instr, __in_ecount(registerSaveSymsCount) StackSym ** registerSaveSyms, uint registerSaveSymsCount)
{
    Func *const func = instr->m_func;
    BailOutInfo *const bailOutInfo = instr->GetBailOutInfo();
    IR::Instr *firstInstr = instr->m_prev;

    // Code analysis doesn't do inter-procesure analysis and cannot infer the value of registerSaveSymsCount,
    // but the passed in registerSaveSymsCount is static value RegNumCount-1, so reg-1 in below loop is always a valid index.
    __analysis_assume(static_cast<int>(registerSaveSymsCount) == static_cast<int>(RegNumCount-1));
    Assert(static_cast<int>(registerSaveSymsCount) == static_cast<int>(RegNumCount-1));

    // Save registers used for parameters, and rax, if necessary, into the shadow space allocated for register parameters:
    //     mov  [rsp + 16], RegArg1     (if branchConditionOpnd)
    //     mov  [rsp + 8], RegArg0
    //     mov  [rsp], rax
    const RegNum regs[3] = { RegRAX, RegArg0, RegArg1 };
    for (int i = (bailOutInfo->branchConditionOpnd ? 2 : 1); i >= 0; i--)
    {
        RegNum reg = regs[i];
        StackSym *const stackSym = registerSaveSyms[reg - 1];
        if(!stackSym)
        {
            continue;
        }

        const IRType regType = RegTypes[reg];
        Lowerer::InsertMove(
            IR::SymOpnd::New(func->m_symTable->GetArgSlotSym(static_cast<Js::ArgSlot>(i + 1)), regType, func),
            IR::RegOpnd::New(stackSym, reg, regType, func),
            instr);
    }

    if(bailOutInfo->branchConditionOpnd)
    {
        // Pass in the branch condition
        //     mov  RegArg1, condition
        IR::Instr *const newInstr =
            Lowerer::InsertMove(
                IR::RegOpnd::New(nullptr, RegArg1, bailOutInfo->branchConditionOpnd->GetType(), func),
                bailOutInfo->branchConditionOpnd,
                instr);
        linearScan->SetSrcRegs(newInstr);
    }

    if (!func->IsOOPJIT())
    {
        // Pass in the bailout record
        //     mov  RegArg0, bailOutRecord
        Lowerer::InsertMove(
            IR::RegOpnd::New(nullptr, RegArg0, TyMachPtr, func),
            IR::AddrOpnd::New(bailOutInfo->bailOutRecord, IR::AddrOpndKindDynamicBailOutRecord, func, true),
            instr);
    }
    else
    {
        // move RegArg0, dataAddr
        Lowerer::InsertMove(
            IR::RegOpnd::New(nullptr, RegArg0, TyMachPtr, func),
            IR::AddrOpnd::New(func->GetWorkItem()->GetWorkItemData()->nativeDataAddr, IR::AddrOpndKindDynamicNativeCodeDataRef, func),
            instr);

        // mov RegArg0, [RegArg0]
        Lowerer::InsertMove(
            IR::RegOpnd::New(nullptr, RegArg0, TyMachPtr, func),
            IR::IndirOpnd::New(IR::RegOpnd::New(nullptr, RegArg0, TyVar, this->func), 0, TyMachPtr, func),
            instr);

        // lea RegArg0, [RegArg0 + bailoutRecord_offset]
        int bailoutRecordOffset = NativeCodeData::GetDataTotalOffset(bailOutInfo->bailOutRecord);
        Lowerer::InsertLea(IR::RegOpnd::New(nullptr, RegArg0, TyVar, this->func),
            IR::IndirOpnd::New(IR::RegOpnd::New(nullptr, RegArg0, TyVar, this->func), bailoutRecordOffset, TyMachPtr,
#if DBG
            NativeCodeData::GetDataDescription(bailOutInfo->bailOutRecord, func->m_alloc),
#endif
            this->func), instr);

    }

    firstInstr = firstInstr->m_next;
    for(uint i = 0; i < registerSaveSymsCount; i++)
    {
        StackSym *const stackSym = registerSaveSyms[i];
        if(!stackSym)
        {
            continue;
        }

        // Record the use on the lifetime in case it spilled afterwards. Spill loads will be inserted before 'firstInstr', that
        // is, before the register saves are done.
        this->linearScan->RecordUse(stackSym->scratch.linearScan.lifetime, firstInstr, nullptr, true);
    }

    // Load the bailout target into rax
    //     mov  rax, BailOut
    //     call rax

    // TODO: Before lazy bailout, this is done unconditionally.
    //       Need to verify if this is the right check in order
    //       to keep the same behaviour as before.
    if (!instr->HasLazyBailOut())
    {
        Assert(instr->GetSrc1()->IsHelperCallOpnd());
        Lowerer::InsertMove(IR::RegOpnd::New(nullptr, RegRAX, TyMachPtr, func), instr->GetSrc1(), instr);
        instr->ReplaceSrc1(IR::RegOpnd::New(nullptr, RegRAX, TyMachPtr, func));
    }
}

uint LinearScanMD::GetRegisterSaveIndex(RegNum reg)
{
    if (RegTypes[reg] == TyFloat64)
    {
        // make room for maximum XMM reg size
        Assert(reg >= RegXMM0);
        return (reg - RegXMM0) * (sizeof(SIMDValue) / sizeof(Js::Var)) + RegXMM0;
    }
    else
    {
        return reg;
    }
}

RegNum LinearScanMD::GetRegisterFromSaveIndex(uint offset)
{
    return (RegNum)(offset >= RegXMM0 ? (offset - RegXMM0) / (sizeof(SIMDValue) / sizeof(Js::Var)) + RegXMM0 : offset);
}

RegNum LinearScanMD::GetParamReg(IR::SymOpnd *symOpnd, Func *func)
{
    RegNum reg = RegNOREG;
    StackSym *paramSym = symOpnd->m_sym->AsStackSym();

    if (func->GetJITFunctionBody()->IsAsmJsMode() && !func->IsLoopBody())
    {
        // Asm.js function only have 1 implicit param as they have no CallInfo, and they have float/SIMD params.
        // Asm.js loop bodies however are called like normal JS functions.
        if (IRType_IsFloat(symOpnd->GetType()) || IRType_IsSimd(symOpnd->GetType()))
        {
            switch (paramSym->GetParamSlotNum())
            {
            case 1:
                reg = RegXMM1;
                break;
            case 2:
                reg = RegXMM2;
                break;
            case 3:
                reg = RegXMM3;
                break;
            }
        }
        else
        {
            if (paramSym->IsImplicitParamSym())
            {
                switch (paramSym->GetParamSlotNum())
                {
                case 1:
                    reg = RegArg0;
                    break;
                default:
                    Assert(UNREACHED);
                }
            }
            else
            {
                switch (paramSym->GetParamSlotNum())
                {
                case 1:
                    reg = RegArg1;
                    break;
                case 2:
                    reg = RegArg2;
                    break;
                case 3:
                    reg = RegArg3;
                    break;
                }
            }
        }
    }
    else // Non-Asm.js
    {
        Assert(symOpnd->GetType() == TyVar || IRType_IsNativeInt(symOpnd->GetType()));

        if (paramSym->IsImplicitParamSym())
        {
            switch (paramSym->GetParamSlotNum())
            {
            case 1:
                reg = RegArg0;
                break;
            case 2:
                reg = RegArg1;
                break;
            }
        }
        else
        {
            switch (paramSym->GetParamSlotNum())
            {
            case 1:
                reg = RegArg2;
                break;
            case 2:
                reg = RegArg3;
                break;
            }
        }
    }

    return reg;
}
