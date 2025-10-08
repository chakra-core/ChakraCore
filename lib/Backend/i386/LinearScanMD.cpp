//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "SccLiveness.h"

extern const IRType RegTypes[RegNumCount];

LinearScanMD::LinearScanMD(Func *func)
    : func(func)
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

    __analysis_assume(reg - RegXMM0 >= 0 && reg - RegXMM0 < XMM_REGCOUNT);
    StackSym *sym;
    if (type == TyFloat32)
    {
        sym = this->xmmSymTable32[reg - RegXMM0];
    }
    else if (type == TyFloat64)
    {
        sym = this->xmmSymTable64[reg - RegXMM0];
    }
    else
    {
        Assert(IRType_IsSimd128(type));
        sym = this->xmmSymTable128[reg - RegXMM0];
    }

    if (sym == NULL)
    {
        sym = StackSym::New(type, func);
        func->StackAllocate(sym, TySize[type]);

        __analysis_assume(reg - RegXMM0 < XMM_REGCOUNT);

        if (type == TyFloat32)
        {
            this->xmmSymTable32[reg - RegXMM0] = sym;
        }
        else if (type == TyFloat64)
        {
            this->xmmSymTable64[reg - RegXMM0] = sym;
        }
        else
        {
            Assert(IRType_IsSimd128(type));
            this->xmmSymTable128[reg - RegXMM0] = sym;
        }
    }

    return sym;
}

void
LinearScanMD::GenerateBailOut(IR::Instr * instr, __in_ecount(registerSaveSymsCount) StackSym ** registerSaveSyms, uint registerSaveSymsCount)
{
    Func *const func = instr->m_func;
    BailOutInfo *const bailOutInfo = instr->GetBailOutInfo();
    IR::Instr *firstInstr = instr->m_prev;
    if(bailOutInfo->branchConditionOpnd)
    {
        // Pass in the branch condition
        //     push condition
        IR::Instr *const newInstr = IR::Instr::New(Js::OpCode::PUSH, func);
        newInstr->SetSrc1(bailOutInfo->branchConditionOpnd);
        instr->InsertBefore(newInstr);
        newInstr->CopyNumber(instr);
        linearScan->SetSrcRegs(newInstr);
    }

    // Pass in the bailout record
    //     push bailOutRecord
    {
        if (!func->IsOOPJIT()) // in-proc jit
        {
            IR::Instr *const newInstr = IR::Instr::New(Js::OpCode::PUSH, func);
            newInstr->SetSrc1(IR::AddrOpnd::New(bailOutInfo->bailOutRecord, IR::AddrOpndKindDynamicBailOutRecord, func, true));
            instr->InsertBefore(newInstr);
            newInstr->CopyNumber(instr);
        }
        else // oop jit
        {
            // [esp - 8]: original eax
            // [esp - 4]: bailout record

            IR::RegOpnd* esp = IR::RegOpnd::New(nullptr, RegESP, TyVar, this->func);
            IR::RegOpnd* eax = IR::RegOpnd::New(nullptr, RegEAX, TyVar, this->func);

            // sub esp, 8  ;To prevent recycler collect the var in eax
            Lowerer::InsertSub(false, esp, esp, IR::IntConstOpnd::New(8, TyVar, this->func), instr);

            // save eax
            // mov [esp], eax
            Lowerer::InsertMove(IR::IndirOpnd::New(esp, 0, TyInt32, func), eax, instr);

            // mov eax, dataAddr
            auto nativeDataAddr = func->GetWorkItem()->GetWorkItemData()->nativeDataAddr;
            Lowerer::InsertMove(eax, IR::AddrOpnd::New(nativeDataAddr, IR::AddrOpndKindDynamicNativeCodeDataRef, func), instr);

            // mov eax, [eax]
            Lowerer::InsertMove(eax, IR::IndirOpnd::New(eax, 0, TyMachPtr, func), instr);

            // lea eax, [eax + bailoutRecord_offset]
            unsigned int bailoutRecordOffset = NativeCodeData::GetDataTotalOffset(bailOutInfo->bailOutRecord);
            Lowerer::InsertLea(
                eax,
                IR::IndirOpnd::New(eax, bailoutRecordOffset, TyUint32,
#if DBG
                    NativeCodeData::GetDataDescription(bailOutInfo->bailOutRecord, func->m_alloc),
#endif
                this->func), instr);

            // mov [esp + 4], eax
            Lowerer::InsertMove(IR::IndirOpnd::New(esp, 4, TyMachPtr, func), eax, instr);

            // restore eax            
            // pop eax
            instr->InsertBefore(IR::Instr::New(Js::OpCode::POP, eax, eax, func));
        }
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
}

__declspec(naked) void LinearScanMD::SaveAllRegisters(BailOutRecord *const bailOutRecord)
{
    __asm
    {
        // [esp + 0 * 4] == return address
        // [esp + 1 * 4] == caller's return address
        // [esp + 2 * 4] == bailOutRecord

        push eax
        mov eax, [esp + 3 * 4] // bailOutRecord
        mov eax, [eax] // bailOutRecord->globalBailOutRecordDataTable
        mov eax, [eax] // bailOutRecord->globalBailOutRecordDataTable->registerSaveSpace

        mov [eax + (RegECX - 1) * 4], ecx
        pop ecx // saved eax
        mov [eax + (RegEAX - 1) * 4], ecx
        mov [eax + (RegEDX - 1) * 4], edx
        mov [eax + (RegEBX - 1) * 4], ebx
        // mov [rax + (RegESP - 1) * 4], esp // the stack pointer is not used by bailout, the frame pointer is used instead
        mov [eax + (RegEBP - 1) * 4], ebp
        mov [eax + (RegESI - 1) * 4], esi
        mov [eax + (RegEDI - 1) * 4], edi

        movups[eax + (RegXMM0 - 1) * 4], xmm0
        movups[eax + (RegXMM0 - 1) * 4 + (RegXMM1 - RegXMM0) * 16], xmm1
        movups[eax + (RegXMM0 - 1) * 4 + (RegXMM2 - RegXMM0) * 16], xmm2
        movups[eax + (RegXMM0 - 1) * 4 + (RegXMM3 - RegXMM0) * 16], xmm3
        movups[eax + (RegXMM0 - 1) * 4 + (RegXMM4 - RegXMM0) * 16], xmm4
        movups[eax + (RegXMM0 - 1) * 4 + (RegXMM5 - RegXMM0) * 16], xmm5
        movups[eax + (RegXMM0 - 1) * 4 + (RegXMM6 - RegXMM0) * 16], xmm6
        movups[eax + (RegXMM0 - 1) * 4 + (RegXMM7 - RegXMM0) * 16], xmm7

        // Don't pop parameters, the caller will redirect into another function call
        ret
    }
}

__declspec(naked) void LinearScanMD::SaveAllRegistersNoSse2(BailOutRecord *const bailOutRecord)
{
    __asm
    {
        // [esp + 0 * 4] == return address
        // [esp + 1 * 4] == caller's return address
        // [esp + 2 * 4] == bailOutRecord

        push eax
        mov eax, [esp + 3 * 4] // bailOutRecord
        mov eax, [eax] // bailOutRecord->globalBailOutRecordDataTable
        mov eax, [eax] // bailOutRecord->globalBailOutRecordDataTable->registerSaveSpace

        mov [eax + (RegECX - 1) * 4], ecx
        pop ecx // saved eax
        mov [eax + (RegEAX - 1) * 4], ecx
        mov [eax + (RegEDX - 1) * 4], edx
        mov [eax + (RegEBX - 1) * 4], ebx
        // mov [rax + (RegESP - 1) * 4], esp // the stack pointer is not used by bailout, the frame pointer is used instead
        mov [eax + (RegEBP - 1) * 4], ebp
        mov [eax + (RegESI - 1) * 4], esi
        mov [eax + (RegEDI - 1) * 4], edi

        // Don't pop parameters, the caller will redirect into another function call
        ret
    }
}

__declspec(naked) void LinearScanMD::SaveAllRegistersAndBailOut(BailOutRecord *const bailOutRecord)
{
    __asm
    {
        // [esp + 0 * 4] == return address
        // [esp + 1 * 4] == bailOutRecord

        call SaveAllRegisters
        jmp BailOutRecord::BailOut
    }
}

__declspec(naked) void LinearScanMD::SaveAllRegistersNoSse2AndBailOut(BailOutRecord *const bailOutRecord)
{
    __asm
    {
        // [esp + 0 * 4] == return address
        // [esp + 1 * 4] == bailOutRecord

        call SaveAllRegistersNoSse2
        jmp BailOutRecord::BailOut
    }
}

__declspec(naked) void LinearScanMD::SaveAllRegistersAndBranchBailOut(
    BranchBailOutRecord *const bailOutRecord,
    const BOOL condition)
{
    __asm
    {
        // [esp + 0 * 4] == return address
        // [esp + 1 * 4] == bailOutRecord
        // [esp + 2 * 4] == condition

        call SaveAllRegisters
        jmp BranchBailOutRecord::BailOut
    }
}

__declspec(naked) void LinearScanMD::SaveAllRegistersNoSse2AndBranchBailOut(
    BranchBailOutRecord *const bailOutRecord,
    const BOOL condition)
{
    __asm
    {
        // [esp + 0 * 4] == return address
        // [esp + 1 * 4] == bailOutRecord
        // [esp + 2 * 4] == condition

        call SaveAllRegistersNoSse2
        jmp BranchBailOutRecord::BailOut
    }
}

void
LinearScanMD::InsertOpHelperSpillAndRestores(SList<OpHelperBlock> *opHelperBlockList)
{
    FOREACH_SLIST_ENTRY(OpHelperBlock, opHelperBlock, opHelperBlockList)
    {
        this->InsertOpHelperSpillsAndRestores(opHelperBlock);
    }
    NEXT_SLIST_ENTRY;
}

void
LinearScanMD::InsertOpHelperSpillsAndRestores(OpHelperBlock const& opHelperBlock)
{
    FOREACH_SLIST_ENTRY(OpHelperSpilledLifetime, opHelperSpilledLifetime, &opHelperBlock.spilledLifetime)
    {
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
            IR::RegOpnd * regOpnd = IR::RegOpnd::New(NULL, opHelperSpilledLifetime.reg, type, this->func);
            if (!sym)
            {
                sym = EnsureSpillSymForXmmReg(regOpnd->GetReg(), this->func, type);
            }
            IR::Instr * pushInstr = IR::Instr::New(LowererMDArch::GetAssignOp(type), IR::SymOpnd::New(sym, type, this->func), regOpnd, this->func);
            opHelperBlock.opHelperLabel->InsertAfter(pushInstr);
            pushInstr->CopyNumber(opHelperBlock.opHelperLabel);

            if (opHelperSpilledLifetime.reload)
            {
                IR::Instr * popInstr = IR::Instr::New(LowererMDArch::GetAssignOp(type), regOpnd, IR::SymOpnd::New(sym, type, this->func), this->func);
                opHelperBlock.opHelperEndInstr->InsertBefore(popInstr);
                popInstr->CopyNumber(opHelperBlock.opHelperEndInstr);
            }
        }
        else
        {
            IR::RegOpnd * regOpnd;
            if (!sym)
            {
                regOpnd = IR::RegOpnd::New(NULL, opHelperSpilledLifetime.reg, TyMachPtr, this->func);
                IR::Instr * pushInstr = IR::Instr::New(Js::OpCode::PUSH, this->func);
                pushInstr->SetSrc1(regOpnd);
                opHelperBlock.opHelperLabel->InsertAfter(pushInstr);
                pushInstr->CopyNumber(opHelperBlock.opHelperLabel);
                if (opHelperSpilledLifetime.reload)
                {
                     IR::Instr * popInstr = IR::Instr::New(Js::OpCode::POP, regOpnd, this->func);
                    opHelperBlock.opHelperEndInstr->InsertBefore(popInstr);
                    popInstr->CopyNumber(opHelperBlock.opHelperEndInstr);
                }
            }
            else
            {
                regOpnd = IR::RegOpnd::New(NULL, opHelperSpilledLifetime.reg, sym->GetType(), this->func);
                IR::Instr* instr = Lowerer::InsertMove(IR::SymOpnd::New(sym, sym->GetType(), func), regOpnd, opHelperBlock.opHelperLabel->m_next);
                instr->CopyNumber(opHelperBlock.opHelperLabel);
                if (opHelperSpilledLifetime.reload)
                {
                    instr = Lowerer::InsertMove(regOpnd, IR::SymOpnd::New(sym, sym->GetType(), func), opHelperBlock.opHelperEndInstr);
                    instr->CopyNumber(opHelperBlock.opHelperEndInstr);
                }
            }
        }

    }
    NEXT_SLIST_ENTRY;
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
    return RegNOREG;
}
