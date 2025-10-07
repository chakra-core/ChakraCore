//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"
#include "Language/JavascriptFunctionArgIndex.h"

const Js::OpCode LowererMD::MDUncondBranchOpcode = Js::OpCode::B;
const Js::OpCode LowererMD::MDMultiBranchOpcode = Js::OpCode::BR;
const Js::OpCode LowererMD::MDTestOpcode = Js::OpCode::TST;
const Js::OpCode LowererMD::MDOrOpcode = Js::OpCode::ORR;
const Js::OpCode LowererMD::MDXorOpcode = Js::OpCode::EOR;
const Js::OpCode LowererMD::MDOverflowBranchOpcode = Js::OpCode::BVS;
const Js::OpCode LowererMD::MDNotOverflowBranchOpcode = Js::OpCode::BVC;
const Js::OpCode LowererMD::MDConvertFloat32ToFloat64Opcode = Js::OpCode::FCVT;
const Js::OpCode LowererMD::MDConvertFloat64ToFloat32Opcode = Js::OpCode::FCVT;
const Js::OpCode LowererMD::MDCallOpcode = Js::OpCode::Call;
const Js::OpCode LowererMD::MDImulOpcode = Js::OpCode::MUL;
const Js::OpCode LowererMD::MDLea = Js::OpCode::LEA;
const Js::OpCode LowererMD::MDSpecBlockNEOpcode = Js::OpCode::CSELNE;
const Js::OpCode LowererMD::MDSpecBlockFNEOpcode = Js::OpCode::FCSELNE;

template<typename T>
inline void Swap(T& x, T& y)
{
    T temp = x;
    x = y;
    y = temp;
}

// Static utility fn()
//
bool
LowererMD::IsAssign(const IR::Instr *instr)
{
    return (instr->m_opcode == Js::OpCode::MOV ||
            instr->m_opcode == Js::OpCode::FMOV ||
            instr->m_opcode == Js::OpCode::LDIMM ||
            instr->m_opcode == Js::OpCode::LDR ||
            instr->m_opcode == Js::OpCode::LDRS ||
            instr->m_opcode == Js::OpCode::FLDR ||
            instr->m_opcode == Js::OpCode::STR ||
            instr->m_opcode == Js::OpCode::FSTR);
}

///----------------------------------------------------------------------------
///
/// LowererMD::IsCall
///
///----------------------------------------------------------------------------

bool
LowererMD::IsCall(const IR::Instr *instr)
{
    return (instr->m_opcode == Js::OpCode::BL ||
            instr->m_opcode == Js::OpCode::BLR);
}

///----------------------------------------------------------------------------
///
/// LowererMD::IsIndirectBranch
///
///----------------------------------------------------------------------------

bool
LowererMD::IsIndirectBranch(const IR::Instr *instr)
{
    return (instr->m_opcode == Js::OpCode::BR);
}


///----------------------------------------------------------------------------
///
/// LowererMD::IsUnconditionalBranch
///
///----------------------------------------------------------------------------

bool
LowererMD::IsUnconditionalBranch(const IR::Instr *instr)
{
    return (instr->m_opcode == Js::OpCode::B || 
            instr->m_opcode == Js::OpCode::BR);
}

bool
LowererMD::IsReturnInstr(const IR::Instr *instr)
{
    return instr->m_opcode == Js::OpCode::RET;
}

///----------------------------------------------------------------------------
///
/// LowererMD::InvertBranch
///
///----------------------------------------------------------------------------

void
LowererMD::InvertBranch(IR::BranchInstr *branchInstr)
{
    switch (branchInstr->m_opcode)
    {
    case Js::OpCode::BEQ:
        branchInstr->m_opcode = Js::OpCode::BNE;
        break;
    case Js::OpCode::BNE:
        branchInstr->m_opcode = Js::OpCode::BEQ;
        break;
    case Js::OpCode::BGE:
        branchInstr->m_opcode = Js::OpCode::BLT;
        break;
    case Js::OpCode::BGT:
        branchInstr->m_opcode = Js::OpCode::BLE;
        break;
    case Js::OpCode::BLT:
        branchInstr->m_opcode = Js::OpCode::BGE;
        break;
    case Js::OpCode::BLE:
        branchInstr->m_opcode = Js::OpCode::BGT;
        break;
    case Js::OpCode::BCS:
        branchInstr->m_opcode = Js::OpCode::BCC;
        break;
    case Js::OpCode::BCC:
        branchInstr->m_opcode = Js::OpCode::BCS;
        break;
    case Js::OpCode::BMI:
        branchInstr->m_opcode = Js::OpCode::BPL;
        break;
    case Js::OpCode::BPL:
        branchInstr->m_opcode = Js::OpCode::BMI;
        break;
    case Js::OpCode::BVS:
        branchInstr->m_opcode = Js::OpCode::BVC;
        break;
    case Js::OpCode::BVC:
        branchInstr->m_opcode = Js::OpCode::BVS;
        break;
    case Js::OpCode::BLS:
        branchInstr->m_opcode = Js::OpCode::BHI;
        break;
    case Js::OpCode::BHI:
        branchInstr->m_opcode = Js::OpCode::BLS;
        break;
    case Js::OpCode::CBZ:
        branchInstr->m_opcode = Js::OpCode::CBNZ;
        break;
    case Js::OpCode::CBNZ:
        branchInstr->m_opcode = Js::OpCode::CBZ;
        break;
    case Js::OpCode::TBZ:
        branchInstr->m_opcode = Js::OpCode::TBNZ;
        break;
    case Js::OpCode::TBNZ:
        branchInstr->m_opcode = Js::OpCode::TBZ;
        break;

    default:
        AssertMsg(UNREACHED, "B missing in InvertBranch()");
    }

}

Js::OpCode
LowererMD::MDConvertFloat64ToInt32Opcode(const RoundMode roundMode)
{
    switch (roundMode)
    {
    case RoundModeTowardZero:
        return Js::OpCode::FCVTZ;
    case RoundModeTowardInteger:
        return Js::OpCode::Nop;
    case RoundModeHalfToEven:
        return Js::OpCode::FCVTN;
    default:
        AssertMsg(0, "RoundMode has no MD mapping.");
        return Js::OpCode::Nop;
    }
}

// GenerateMemRef: Return an opnd that can be used to access the given address.
// ARM can't encode direct accesses to physical addresses, so put the address in a register
// and return an indir. (This facilitates re-use of the loaded address without having to re-load it.)
IR::Opnd *
LowererMD::GenerateMemRef(intptr_t addr, IRType type, IR::Instr *instr, bool dontEncode)
{
    IR::RegOpnd *baseOpnd = IR::RegOpnd::New(TyMachReg, this->m_func);
    IR::AddrOpnd *addrOpnd = IR::AddrOpnd::New(addr, IR::AddrOpndKindDynamicMisc, this->m_func, dontEncode);
    Lowerer::InsertMove(baseOpnd, addrOpnd, instr);

    return IR::IndirOpnd::New(baseOpnd, 0, type, this->m_func);
}

void
LowererMD::FlipHelperCallArgsOrder()
{
    int left  = 0;
    int right = helperCallArgsCount - 1;
    while (left < right)
    {
        IR::Opnd *tempOpnd = helperCallArgs[left];
        helperCallArgs[left] = helperCallArgs[right];
        helperCallArgs[right] = tempOpnd;

        left++;
        right--;
    }
}

IR::Instr *
LowererMD::LowerCallHelper(IR::Instr *instrCall)
{
    IR::Opnd *argOpnd = instrCall->UnlinkSrc2();
    IR::Instr          *prevInstr = instrCall;
    IR::JnHelperMethod helperMethod = instrCall->GetSrc1()->AsHelperCallOpnd()->m_fnHelper;
    instrCall->FreeSrc1();

    while (argOpnd)
    {
        Assert(argOpnd->IsRegOpnd());
        IR::RegOpnd *regArg = argOpnd->AsRegOpnd();

        Assert(regArg->m_sym->m_isSingleDef);
        IR::Instr *instrArg = regArg->m_sym->m_instrDef;

        Assert(instrArg->m_opcode == Js::OpCode::ArgOut_A || instrArg->m_opcode == Js::OpCode::ExtendArg_A &&
        (
            helperMethod == IR::JnHelperMethod::HelperOP_InitCachedScope ||
            helperMethod == IR::JnHelperMethod::HelperScrFunc_OP_NewScFuncHomeObj ||
            helperMethod == IR::JnHelperMethod::HelperScrFunc_OP_NewScGenFuncHomeObj ||
            helperMethod == IR::JnHelperMethod::HelperRestify ||
            helperMethod == IR::JnHelperMethod::HelperStPropIdArrFromVar
        ));
        prevInstr = this->LoadHelperArgument(prevInstr, instrArg->GetSrc1());

        argOpnd = instrArg->GetSrc2();

        if (instrArg->m_opcode == Js::OpCode::ArgOut_A)
        {
            instrArg->UnlinkSrc1();
            if (argOpnd)
            {
                instrArg->UnlinkSrc2();
            }
            regArg->Free(this->m_func);
            instrArg->Remove();
        }
        else if (instrArg->m_opcode == Js::OpCode::ExtendArg_A)
        {
            if (instrArg->GetSrc1()->IsRegOpnd())
            {
                m_lowerer->addToLiveOnBackEdgeSyms->Set(instrArg->GetSrc1()->AsRegOpnd()->GetStackSym()->m_id);
            }
        }
    }

    switch (helperMethod)
    {
    case IR::JnHelperMethod::HelperScrFunc_OP_NewScFuncHomeObj:
    case IR::JnHelperMethod::HelperScrFunc_OP_NewScGenFuncHomeObj:
        break;
    default:
        prevInstr = m_lowerer->LoadScriptContext(prevInstr);
        break;
    }
    this->FlipHelperCallArgsOrder();
    return this->ChangeToHelperCall(instrCall, helperMethod);
}

// Lower a call: May be either helper or native JS call. Just set the opcode, and
// put the result into the return register. (No stack adjustment required.)
IR::Instr *
LowererMD::LowerCall(IR::Instr * callInstr, Js::ArgSlot argCount)
{
    IR::Instr *retInstr = callInstr;
    IR::Opnd *targetOpnd = callInstr->GetSrc1();
    AssertMsg(targetOpnd, "Call without a target?");

    // This is required here due to calls created during lowering
    callInstr->m_func->SetHasCallsOnSelfAndParents();

    if (targetOpnd->IsRegOpnd())
    {
        // Indirect call
        callInstr->m_opcode = Js::OpCode::BLR;
    }
    else
    {
        AssertMsg(targetOpnd->IsHelperCallOpnd(), "Why haven't we loaded the call target?");
        // Direct call
        //
        // load the address into a register because we cannot directly access more than 24 bit constants
        // in BL instruction. Non helper call methods will already be accessed indirectly.
        //
        // Skip this for bailout calls. The register allocator will lower that as appropriate, without affecting spill choices.

        if (!callInstr->HasBailOutInfo())
        {
            IR::RegOpnd     *regOpnd = IR::RegOpnd::New(nullptr, RegLR, TyMachPtr, this->m_func);
            IR::Instr       *movInstr = IR::Instr::New(Js::OpCode::LDIMM, regOpnd, callInstr->UnlinkSrc1(), this->m_func);
            regOpnd->m_isCallArg = true;
            callInstr->SetSrc1(regOpnd);
            callInstr->InsertBefore(movInstr);
        }
        callInstr->m_opcode = Js::OpCode::BLR;
    }

    IR::Opnd *dstOpnd = callInstr->GetDst();
    if (dstOpnd)
    {
        Js::OpCode assignOp;
        RegNum returnReg;

        if(dstOpnd->IsFloat64())
        {
            assignOp = Js::OpCode::FMOV;
            returnReg = RETURN_DBL_REG;
        }
        else
        {
            assignOp = Js::OpCode::MOV;
            returnReg = RETURN_REG;

            if (callInstr->GetSrc1()->IsHelperCallOpnd())
            {
                // Truncate the result of a conversion to 32-bit int, because the C++ code doesn't.
                IR::HelperCallOpnd *helperOpnd = callInstr->GetSrc1()->AsHelperCallOpnd();
                if (helperOpnd->m_fnHelper == IR::HelperConv_ToInt32 ||
                    helperOpnd->m_fnHelper == IR::HelperConv_ToInt32_Full ||
                    helperOpnd->m_fnHelper == IR::HelperConv_ToInt32Core ||
                    helperOpnd->m_fnHelper == IR::HelperConv_ToUInt32 ||
                    helperOpnd->m_fnHelper == IR::HelperConv_ToUInt32_Full ||
                    helperOpnd->m_fnHelper == IR::HelperConv_ToUInt32Core)
                {
                    assignOp = Js::OpCode::MOV_TRUNC;
                }
            }
        }

        IR::Instr * movInstr = callInstr->SinkDst(assignOp);

        callInstr->GetDst()->AsRegOpnd()->SetReg(returnReg);
        movInstr->GetSrc1()->AsRegOpnd()->SetReg(returnReg);

        retInstr = movInstr;

        Legalize(retInstr);
    }

    //
    // assign the arguments to appropriate positions
    //

    AssertMsg(this->helperCallArgsCount >= 0, "Fatal. helper call arguments ought to be positive");
    AssertMsg(this->helperCallArgsCount <= MaxArgumentsToHelper, "Too many helper call arguments");

    uint16 argsLeft = this->helperCallArgsCount;
    uint16 doubleArgsLeft = this->helperCallDoubleArgsCount;
    uint16 intArgsLeft = argsLeft - doubleArgsLeft;

    while(argsLeft > 0)
    {
        IR::Opnd *helperArgOpnd = this->helperCallArgs[this->helperCallArgsCount - argsLeft];
        IR::Opnd * opndParam = nullptr;

        if (helperArgOpnd->IsFloat())
        {
            opndParam = this->GetOpndForArgSlot(doubleArgsLeft - 1, helperArgOpnd);
            AssertMsg(opndParam->IsRegOpnd(), "NYI for other kind of operands");
            --doubleArgsLeft;
        }
        else
        {
            opndParam = this->GetOpndForArgSlot(intArgsLeft - 1, helperArgOpnd);
            --intArgsLeft;
        }
        Lowerer::InsertMove(opndParam, helperArgOpnd, callInstr);
        --argsLeft;
    }
    Assert(doubleArgsLeft == 0 && intArgsLeft == 0 && argsLeft == 0);

    // We're done with the args (if any) now, so clear the param location state.
    this->FinishArgLowering();

    return retInstr;
}

IR::Instr *
LowererMD::LoadDynamicArgument(IR::Instr *instr, uint argNumber)
{
    Assert(instr->m_opcode == Js::OpCode::ArgOut_A_Dynamic);
    Assert(instr->GetSrc2() == nullptr);

    IR::Opnd* dst = GetOpndForArgSlot((Js::ArgSlot) (argNumber - 1));
    instr->SetDst(dst);
    instr->m_opcode = Js::OpCode::MOV;
    LegalizeMD::LegalizeInstr(instr);

    return instr;
}

IR::Instr *
LowererMD::LoadDynamicArgumentUsingLength(IR::Instr *instr)
{
    Assert(instr->m_opcode == Js::OpCode::ArgOut_A_Dynamic);
    IR::RegOpnd* src2 = instr->UnlinkSrc2()->AsRegOpnd();

    // We register store the first INT_ARG_REG_COUNT - 3 parameters, since the first 3 register parameters are taken by function object, callinfo, and this pointer
    IR::Instr *add = IR::Instr::New(Js::OpCode::SUB, IR::RegOpnd::New(src2->GetType(), this->m_func), src2, IR::IntConstOpnd::New(INT_ARG_REG_COUNT - 3, TyInt8, this->m_func), this->m_func);
    instr->InsertBefore(add);
    LegalizeMD::LegalizeInstr(add);
    //We need store nth actuals, so stack location is after function object, callinfo & this pointer
    IR::RegOpnd *stackPointer   = IR::RegOpnd::New(nullptr, GetRegStackPointer(), TyMachReg, this->m_func);
    IR::IndirOpnd *actualsLocation = IR::IndirOpnd::New(stackPointer, add->GetDst()->AsRegOpnd(), GetDefaultIndirScale(), TyMachReg, this->m_func);
    instr->SetDst(actualsLocation);
    instr->m_opcode = Js::OpCode::LDR;
    LegalizeMD::LegalizeInstr(instr);

    return instr;
}

void
LowererMD::SetMaxArgSlots(Js::ArgSlot actualCount /*including this*/)
{
    Js::ArgSlot offset = 3;//For function object & callInfo & this
    if (this->m_func->m_argSlotsForFunctionsCalled < (uint32) (actualCount + offset))
    {
        this->m_func->m_argSlotsForFunctionsCalled = (uint32)(actualCount + offset);
    }
    return;
}

void
LowererMD::GenerateMemInit(IR::RegOpnd * opnd, int32 offset, size_t value, IR::Instr * insertBeforeInstr, bool isZeroed)
{
    m_lowerer->GenerateMemInit(opnd, offset, (uint32)value, insertBeforeInstr, isZeroed);
}

IR::Instr *
LowererMD::LowerCallIDynamic(IR::Instr *callInstr, IR::Instr*saveThisArgOutInstr, IR::Opnd *argsLength, ushort callFlags, IR::Instr * insertBeforeInstrForCFG)
{
    callInstr->InsertBefore(saveThisArgOutInstr); //Move this Argout next to call;
    this->LoadDynamicArgument(saveThisArgOutInstr, 3); //this pointer is the 3rd argument

    //callInfo
    if (callInstr->m_func->IsInlinee())
    {
        Assert(argsLength->AsIntConstOpnd()->GetValue() == callInstr->m_func->actualCount);
        this->SetMaxArgSlots((Js::ArgSlot)callInstr->m_func->actualCount);
    }
    else
    {
        callInstr->InsertBefore(IR::Instr::New(Js::OpCode::ADD, argsLength, argsLength, IR::IntConstOpnd::New(1, TyInt8, this->m_func), this->m_func));
        this->SetMaxArgSlots(Js::InlineeCallInfo::MaxInlineeArgoutCount);
    }
    Lowerer::InsertMove( this->GetOpndForArgSlot(1), argsLength, callInstr);

    IR::RegOpnd    *funcObjOpnd = callInstr->UnlinkSrc1()->AsRegOpnd();
    GeneratePreCall(callInstr, funcObjOpnd, insertBeforeInstrForCFG);

    // functionOpnd is the first argument.
    IR::Opnd * opndParam = this->GetOpndForArgSlot(0);
    Lowerer::InsertMove(opndParam, funcObjOpnd, callInstr);
    return this->LowerCall(callInstr, 0);
}

void
LowererMD::GenerateFunctionObjectTest(IR::Instr * callInstr, IR::RegOpnd  *functionObjOpnd, bool isHelper, IR::LabelInstr* continueAfterExLabel /* = nullptr */)
{
    AssertMsg(!m_func->IsJitInDebugMode() || continueAfterExLabel, "When jit is in debug mode, continueAfterExLabel must be provided otherwise continue after exception may cause AV.");

    // Need check and error if we are calling a tagged int.
    if (!functionObjOpnd->IsNotTaggedValue())
    {
        IR::LabelInstr * helperLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
        if (this->GenerateObjectTest(functionObjOpnd, callInstr, helperLabel))
        {

            IR::LabelInstr * callLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, isHelper);
            IR::Instr * instr = IR::BranchInstr::New(Js::OpCode::B, callLabel, this->m_func);
            callInstr->InsertBefore(instr);

            callInstr->InsertBefore(helperLabel);
            callInstr->InsertBefore(callLabel);

            this->m_lowerer->GenerateRuntimeError(callLabel, JSERR_NeedFunction);

            if (continueAfterExLabel)
            {
                // Under debugger the RuntimeError (exception) can be ignored, generate branch to jmp to safe place
                // (which would normally be debugger bailout check).
                IR::BranchInstr* continueAfterEx = IR::BranchInstr::New(LowererMD::MDUncondBranchOpcode, continueAfterExLabel, this->m_func);
                callLabel->InsertBefore(continueAfterEx);
            }
        }
    }
}

IR::Instr*
LowererMD::GeneratePreCall(IR::Instr * callInstr, IR::Opnd  *functionObjOpnd, IR::Instr * insertBeforeInstrForCFGCheck)
{
    if (insertBeforeInstrForCFGCheck == nullptr)
    {
        insertBeforeInstrForCFGCheck = callInstr;
    }

    IR::RegOpnd * functionTypeRegOpnd = nullptr;
    IR::IndirOpnd * entryPointIndirOpnd = nullptr;

    // For calls to fixed functions we load the function's type directly from the known (hard-coded) function object address.
    // For other calls, we need to load it from the function object stored in a register operand.
    if (functionObjOpnd->IsAddrOpnd() && functionObjOpnd->AsAddrOpnd()->m_isFunction)
    {
        functionTypeRegOpnd = this->m_lowerer->GenerateFunctionTypeFromFixedFunctionObject(insertBeforeInstrForCFGCheck, functionObjOpnd);
    }
    else if (functionObjOpnd->IsRegOpnd())
    {
        AssertMsg(functionObjOpnd->AsRegOpnd()->m_sym->IsStackSym(), "Expected call target to be stackSym");

        functionTypeRegOpnd = IR::RegOpnd::New(TyMachReg, this->m_func);

        IR::IndirOpnd* functionTypeIndirOpnd = IR::IndirOpnd::New(functionObjOpnd->AsRegOpnd(),
            Js::RecyclableObject::GetOffsetOfType(), TyMachReg, this->m_func);
        Lowerer::InsertMove(functionTypeRegOpnd, functionTypeIndirOpnd, insertBeforeInstrForCFGCheck);
    }
    else
    {
        AnalysisAssertMsg(false, "Unexpected call target operand type.");
    }
    entryPointIndirOpnd = IR::IndirOpnd::New(functionTypeRegOpnd, Js::Type::GetOffsetOfEntryPoint(), TyMachPtr, m_func);

    IR::RegOpnd *entryPointRegOpnd = functionTypeRegOpnd;
    entryPointRegOpnd->m_isCallArg = true;

    IR::Instr * stackParamInsert = Lowerer::InsertMove(entryPointRegOpnd, entryPointIndirOpnd, insertBeforeInstrForCFGCheck);

    // targetAddrOpnd is the address we'll call.
    callInstr->SetSrc1(entryPointRegOpnd);

#if defined(_CONTROL_FLOW_GUARD)
    // verify that the call target is valid (CFG Check)
    if (!PHASE_OFF(Js::CFGInJitPhase, this->m_func))
    {
        this->GenerateCFGCheck(entryPointRegOpnd, insertBeforeInstrForCFGCheck);
    }
#endif

    return stackParamInsert;
}

IR::Instr *
LowererMD::LowerCallI(IR::Instr * callInstr, ushort callFlags, bool isHelper, IR::Instr * insertBeforeInstrForCFG)
{
    // Indirect call using JS calling convention:
    // R0 = callee func object
    // R1 = callinfo
    // R2 = arg0 ("this")
    // R3 = arg1
    // [sp] = arg2
    // etc.

    // First load the target address. Note that we want to wind up with this:
    // ...
    // [sp+4] = arg3
    // [sp] = arg2
    // load target addr from func obj
    // R3 = arg1
    // ...
    // R0 = func obj
    // BLX target addr
    // This way the register containing the target addr interferes with the param regs
    // only, not the regs we use to store params to the stack.

    // We're sinking the stores of stack params so that the call sequence is contiguous.
    // This is required by nested calls, since each call will re-use the same stack slots.
    // But if there is no nesting, stack params can be stored as soon as they're computed.

    IR::Opnd * functionObjOpnd = callInstr->UnlinkSrc1();
    IR::Instr * insertBeforeInstrForCFGCheck = callInstr;

    // If this is a call for new, we already pass the function operand through NewScObject,
    // which checks if the function operand is a real function or not, don't need to add a check again.
    // If this is a call to a fixed function, we've already verified that the target is, indeed, a function.
    if (callInstr->m_opcode != Js::OpCode::CallIFixed && !(callFlags & Js::CallFlags_New))
    {
        Assert(functionObjOpnd->IsRegOpnd());
        IR::LabelInstr* continueAfterExLabel = Lowerer::InsertContinueAfterExceptionLabelForDebugger(m_func, callInstr, isHelper);
        GenerateFunctionObjectTest(callInstr, functionObjOpnd->AsRegOpnd(), isHelper, continueAfterExLabel);
    }
    else if (insertBeforeInstrForCFG != nullptr)
    {
//        RegNum dstReg = insertBeforeInstrForCFG->GetDst()->AsRegOpnd()->GetReg();
//        AssertMsg(dstReg == RegArg2 || dstReg == RegArg3, "NewScObject should insert the first Argument in RegArg2/RegArg3 only based on Spread call or not.");
        insertBeforeInstrForCFGCheck = insertBeforeInstrForCFG;
    }

    IR::Instr * stackParamInsert = GeneratePreCall(callInstr, functionObjOpnd, insertBeforeInstrForCFGCheck);

    // We need to get the calculated CallInfo in SimpleJit because that doesn't include any changes for stack alignment
    IR::IntConstOpnd *callInfo;
    int32 argCount = this->LowerCallArgs(callInstr, stackParamInsert, callFlags, 1, &callInfo);

    // functionObjOpnd is the first argument.
    IR::Opnd * opndParam = this->GetOpndForArgSlot(0);
    Lowerer::InsertMove(opndParam, functionObjOpnd, callInstr);

    IR::Opnd *const finalDst = callInstr->GetDst();

    // Finally, lower the call instruction itself.
    IR::Instr* ret = this->LowerCall(callInstr, (Js::ArgSlot)argCount);

    IR::AutoReuseOpnd autoReuseSavedFunctionObjOpnd;
    if (callInstr->IsJitProfilingInstr())
    {
        Assert(callInstr->m_func->IsSimpleJit());
        Assert(!CONFIG_FLAG(NewSimpleJit));

        if(finalDst &&
            finalDst->IsRegOpnd() &&
            functionObjOpnd->IsRegOpnd() &&
            finalDst->AsRegOpnd()->m_sym == functionObjOpnd->AsRegOpnd()->m_sym)
        {
            // The function object sym is going to be overwritten, so save it in a temp for profiling
            IR::RegOpnd *const savedFunctionObjOpnd = IR::RegOpnd::New(functionObjOpnd->GetType(), callInstr->m_func);
            autoReuseSavedFunctionObjOpnd.Initialize(savedFunctionObjOpnd, callInstr->m_func);
            Lowerer::InsertMove(savedFunctionObjOpnd, functionObjOpnd, callInstr->m_next);
            functionObjOpnd = savedFunctionObjOpnd;
        }

        auto instr = callInstr->AsJitProfilingInstr();
        ret = this->m_lowerer->GenerateCallProfiling(
            instr->profileId,
            instr->inlineCacheIndex,
            instr->GetDst(),
            functionObjOpnd,
            callInfo,
            instr->isProfiledReturnCall,
            callInstr,
            ret);
    }
    return ret;
}

int32
LowererMD::LowerCallArgs(IR::Instr *callInstr, IR::Instr *stackParamInsert, ushort callFlags, Js::ArgSlot extraParams, IR::IntConstOpnd **callInfoOpndRef)
{
    AssertMsg(this->helperCallArgsCount == 0, "We don't support nested helper calls yet");

    uint32 argCount = 0;

    IR::Opnd* opndParam;
    // Now walk the user arguments and remember the arg count.

    IR::Instr * argInstr = callInstr;
    IR::Opnd *src2Opnd = callInstr->UnlinkSrc2();
    while (src2Opnd->IsSymOpnd())
    {
        // Get the arg instr
        IR::SymOpnd *   argLinkOpnd = src2Opnd->AsSymOpnd();
        StackSym *      argLinkSym  = argLinkOpnd->m_sym->AsStackSym();
        AssertMsg(argLinkSym->IsArgSlotSym() && argLinkSym->m_isSingleDef, "Arg tree not single def...");
        argLinkOpnd->Free(this->m_func);

        argInstr = argLinkSym->m_instrDef;

        // The arg sym isn't assigned a constant directly anymore
        argLinkSym->m_isConst = false;
        argLinkSym->m_isIntConst = false;
        argLinkSym->m_isTaggableIntConst = false;

        // The arg slot nums are 1-based, so subtract 1. Then add 1 for the non-user args (callinfo).
        auto argSlotNum = argLinkSym->GetArgSlotNum();
        if(argSlotNum + extraParams < argSlotNum)
        {
            Js::Throw::OutOfMemory();
        }
        opndParam = this->GetOpndForArgSlot(argSlotNum + extraParams);

        src2Opnd = argInstr->UnlinkSrc2();
        argInstr->ReplaceDst(opndParam);
        argInstr->Unlink();
        if (opndParam->IsRegOpnd())
        {
            callInstr->InsertBefore(argInstr);
        }
        else
        {
            stackParamInsert->InsertBefore(argInstr);
        }
        this->ChangeToAssign(argInstr);
        argCount++;
    }

    IR::RegOpnd * argLinkOpnd = src2Opnd->AsRegOpnd();
    StackSym *argLinkSym = argLinkOpnd->m_sym->AsStackSym();
    AssertMsg(!argLinkSym->IsArgSlotSym() && argLinkSym->m_isSingleDef, "Arg tree not single def...");

    IR::Instr *startCallInstr = argLinkSym->m_instrDef;

    AssertMsg(startCallInstr->m_opcode == Js::OpCode::StartCall || startCallInstr->m_opcode == Js::OpCode::LoweredStartCall, "Problem with arg chain.");
    AssertMsg(startCallInstr->GetArgOutCount(/*getInterpreterArgOutCount*/ false) == argCount,
        "ArgCount doesn't match StartCall count");

    // Deal with the SC.
    this->LowerStartCall(startCallInstr);

    // Second argument is the callinfo.
    IR::IntConstOpnd *opndCallInfo = Lowerer::MakeCallInfoConst(callFlags, argCount, m_func);
    if(callInfoOpndRef)
    {
        opndCallInfo->Use(m_func);
        *callInfoOpndRef = opndCallInfo;
    }
    opndParam = this->GetOpndForArgSlot(extraParams);
    Lowerer::InsertMove(opndParam, opndCallInfo, callInstr);

    return argCount + 1 + extraParams; // + 1 for call flags
}

IR::Instr *
LowererMD::LowerStartCall(IR::Instr * instr)
{
    // StartCall doesn't need to generate a stack adjustment. Just delete it.
    instr->m_opcode = Js::OpCode::LoweredStartCall;
    return instr;
}

IR::Instr *
LowererMD::LoadHelperArgument(IR::Instr * instr, IR::Opnd * opndArgValue)
{
    // Load the given parameter into the appropriate location.
    // We update the current param state so we can do this work without making the caller
    // do the work.
    Assert(this->helperCallArgsCount < LowererMD::MaxArgumentsToHelper);

    __analysis_assume(this->helperCallArgsCount < MaxArgumentsToHelper);

    helperCallArgs[helperCallArgsCount++] = opndArgValue;

    if (opndArgValue->GetType() == TyMachDouble)
    {
        this->helperCallDoubleArgsCount++;
    }

    return instr;
}

void
LowererMD::FinishArgLowering()
{
    this->helperCallArgsCount = 0;
    this->helperCallDoubleArgsCount = 0;
}

IR::Opnd *
LowererMD::GetOpndForArgSlot(Js::ArgSlot argSlot, IR::Opnd * argOpnd)
{
    IR::Opnd * opndParam = nullptr;

    IRType type = argOpnd ? argOpnd->GetType() : TyMachReg;
    if (argOpnd == nullptr || !argOpnd->IsFloat())
    {
        if (argSlot < NUM_INT_ARG_REGS)
        {
            // Return an instance of the next arg register.
            IR::RegOpnd *regOpnd;
            regOpnd = IR::RegOpnd::New(nullptr, (RegNum)(argSlot + FIRST_INT_ARG_REG), type, this->m_func);

            regOpnd->m_isCallArg = true;

            opndParam = regOpnd;
        }
        else
        {
            // Create a stack slot reference and bump up the size of this function's outgoing param area,
            // if necessary.
            argSlot = argSlot - NUM_INT_ARG_REGS;
            IntConstType offset = argSlot * MachRegInt;
            IR::RegOpnd * spBase = IR::RegOpnd::New(nullptr, this->GetRegStackPointer(), TyMachReg, this->m_func);
            opndParam = IR::IndirOpnd::New(spBase, int32(offset), type, this->m_func);

            if (this->m_func->m_argSlotsForFunctionsCalled < (uint32)(argSlot + 1))
            {
                this->m_func->m_argSlotsForFunctionsCalled = argSlot + 1;
            }
        }
    }
    else
    {
        if (argSlot < MaxDoubleArgumentsToHelper)
        {
            // Return an instance of the next arg register.
            IR::RegOpnd *regOpnd;
            regOpnd = IR::RegOpnd::New(nullptr, (RegNum)(argSlot + FIRST_DOUBLE_ARG_REG), type, this->m_func);
            regOpnd->m_isCallArg = true;
            opndParam = regOpnd;
        }
        else
        {
            AssertMsg(false,"More than 8 double parameter passing disallowed");
        }
    }
    return opndParam;
}

IR::Instr *
LowererMD::LoadDoubleHelperArgument(IR::Instr * instr, IR::Opnd * opndArg)
{
    // Load the given parameter into the appropriate location.
    // We update the current param state so we can do this work without making the caller
    // do the work.
    Assert(opndArg->GetType() == TyMachDouble);
    return this->LoadHelperArgument(instr, opndArg);
}

void
LowererMD::GenerateStackProbe(IR::Instr *insertInstr, bool afterProlog)
{
    //
    // Generate a stack overflow check. This can be as simple as a cmp esp, const
    // because this function is guaranteed to be called on its base thread only.
    // If the check fails call ThreadContext::ProbeCurrentStack which will check again and must throw.
    //
    //       LDIMM r17, ThreadContext::scriptStackLimit + frameSize //Load to register first, as this can be more than 12 bit supported in CMP
    //       CMP   sp, r17
    //       BHI   done
    // begin:
    //       LDIMM  r0, frameSize
    //       LDIMM  r1, scriptContext
    //       LDIMM  r2, ThreadContext::ProbeCurrentStack //MUST THROW
    //       BLX    r2                                  //BX r2 if the stackprobe is before prolog
    // done:
    //

    // For thread context with script interrupt enabled:
    //       LDIMM r17, &ThreadContext::scriptStackLimitForCurrentThread
    //       LDR   r17, [r17]
    //       MOV   r15, frameSize
    //       ADDS  r17, r17, r15
    //       BVS   $helper
    //       CMP   sp, r17
    //       BHI   done
    // $helper:
    //       LDIMM  r0, frameSize
    //       LDIMM  r1, scriptContext
    //       LDIMM  r2, ThreadContext::ProbeCurrentStack //MUST THROW
    //       BLX    r2                                  //BX r2 if the stackprobe is before prolog
    // done:
    //

    //m_localStackHeight for ARM contains (m_argSlotsForFunctionsCalled * MachPtr)
    uint32 frameSize = this->m_func->m_localStackHeight + Js::Constants::MinStackJIT;

    IR::RegOpnd *scratchOpnd = IR::RegOpnd::New(nullptr, SCRATCH_REG, TyMachReg, this->m_func);
    IR::LabelInstr *helperLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, afterProlog);
    IR::Instr *instr;
    bool doInterruptProbe = m_func->GetJITFunctionBody()->DoInterruptProbe();

    if (doInterruptProbe || !m_func->GetThreadContextInfo()->IsThreadBound())
    {
        // LDIMM r17, &ThreadContext::scriptStackLimitForCurrentThread
        intptr_t pLimit = m_func->GetThreadContextInfo()->GetThreadStackLimitAddr();
        Lowerer::InsertMove(scratchOpnd, IR::AddrOpnd::New(pLimit, IR::AddrOpndKindDynamicMisc, this->m_func), insertInstr);

        // LDR   r17, [r17, #0]
        Lowerer::InsertMove(scratchOpnd, IR::IndirOpnd::New(scratchOpnd, 0, TyMachReg, this->m_func), insertInstr);

        AssertMsg(!IS_CONST_00000FFF(frameSize), "For small size we can just add frameSize to r17");

        // MOV r15, frameSize
        IR::Opnd* spAllocRegOpnd = IR::RegOpnd::New(nullptr, SP_ALLOC_SCRATCH_REG, TyMachReg, this->m_func);
        Lowerer::InsertMove(spAllocRegOpnd, IR::IntConstOpnd::New(frameSize, TyMachReg, this->m_func), insertInstr);

        // ADDS r17, r17, r15
        instr = IR::Instr::New(Js::OpCode::ADDS, scratchOpnd, scratchOpnd, spAllocRegOpnd, this->m_func);
        insertInstr->InsertBefore(instr);

        // If this add overflows, we have to call the helper.
        instr = IR::BranchInstr::New(Js::OpCode::BVS, helperLabel, this->m_func);
        insertInstr->InsertBefore(instr);
    }
    else
    {
        // MOV r17, frameSize + scriptStackLimit
        uint64 scriptStackLimit = m_func->GetThreadContextInfo()->GetScriptStackLimit();
        IR::Opnd *stackLimitOpnd = IR::IntConstOpnd::New(frameSize + scriptStackLimit, TyMachReg, this->m_func);
        Lowerer::InsertMove(scratchOpnd, stackLimitOpnd, insertInstr);
    }

    IR::LabelInstr *doneLabelInstr = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, false);
    if (!IS_FAULTINJECT_STACK_PROBE_ON) // Do stack check fastpath only if not doing StackProbe fault injection
    {
        // CMP sp, r17
        instr = IR::Instr::New(Js::OpCode::CMP, this->m_func);
        instr->SetSrc1(IR::RegOpnd::New(nullptr, GetRegStackPointer(), TyMachReg, this->m_func));
        instr->SetSrc2(scratchOpnd);
        insertInstr->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);

        // BHI done
        instr = IR::BranchInstr::New(Js::OpCode::BHI, doneLabelInstr, this->m_func);
        insertInstr->InsertBefore(instr);
    }

    insertInstr->InsertBefore(helperLabel);

    // ToDo (SaAgarwa): Make sure all SP offsets are correct
    // Zero out the pointer to the list of stack nested funcs, since the functions won't be initialized on this path.
    /*
    scratchOpnd = IR::RegOpnd::New(nullptr, RegR0, TyMachReg, m_func);
    IR::RegOpnd *frameReg = IR::RegOpnd::New(nullptr, GetRegFramePointer(), TyMachReg, m_func);
    Lowerer::InsertMove(scratchOpnd, IR::IntConstOpnd::New(0, TyMachReg, m_func), insertInstr);
    IR::Opnd *indirOpnd = IR::IndirOpnd::New(
        frameReg, -(int32)(Js::Constants::StackNestedFuncList * sizeof(Js::Var)), TyMachReg, m_func);
    Lowerer::InsertMove(indirOpnd, scratchOpnd, insertInstr);
    */
    IR::RegOpnd *r0Opnd = IR::RegOpnd::New(nullptr, RegR0, TyMachReg, this->m_func);
    Lowerer::InsertMove(r0Opnd, IR::IntConstOpnd::New(frameSize, TyMachReg, this->m_func, true), insertInstr);

    IR::RegOpnd *r1Opnd = IR::RegOpnd::New(nullptr, RegR1, TyMachReg, this->m_func);
    Lowerer::InsertMove(r1Opnd, this->m_lowerer->LoadScriptContextOpnd(insertInstr), insertInstr);

    IR::RegOpnd *r2Opnd = IR::RegOpnd::New(nullptr, RegR2, TyMachReg, m_func);
    Lowerer::InsertMove(r2Opnd, IR::HelperCallOpnd::New(IR::HelperProbeCurrentStack, this->m_func), insertInstr);

    instr = IR::Instr::New(afterProlog? Js::OpCode::BLR : Js::OpCode::BR, this->m_func);
    instr->SetSrc1(r2Opnd);
    insertInstr->InsertBefore(instr);

    insertInstr->InsertBefore(doneLabelInstr);
}

//
// Emits the code to allocate 'size' amount of space on stack. for values smaller than PAGE_SIZE
// this will just emit sub SP,size otherwise calls _chkstk.
//
bool
LowererMD::GenerateStackAllocation(IR::Instr *instr, uint32 allocSize, uint32 probeSize)
{
    IR::RegOpnd* spOpnd = IR::RegOpnd::New(nullptr, GetRegStackPointer(), TyMachReg, this->m_func);

    if (IsSmallStack(probeSize))
    {
        AssertMsg(!(allocSize & 0xFFFFF000), "Must fit in 12 bits");
        AssertMsg(allocSize % MachStackAlignment == 0, "Must be aligned");
        // Generate SUB SP, SP, stackSize
        IR::IntConstOpnd *  stackSizeOpnd   = IR::IntConstOpnd::New(allocSize, TyMachReg, this->m_func, true);
        IR::Instr * subInstr = IR::Instr::New(Js::OpCode::SUB, spOpnd, spOpnd, stackSizeOpnd, this->m_func);
        instr->InsertBefore(subInstr);
        return false;
    }

    //__chkStk is a leaf function and hence alignment is not required.

    // Generate _chkstk call
    // LDIMM RegR15, stackSize/16
    // LDIMM RegR17, HelperCRT_chkstk
    // BLX RegR17
    // SUB SP, SP, x15, lsl #4

    //chkstk expects the stacksize argument in R15 register
    IR::RegOpnd *spAllocOpnd = IR::RegOpnd::New(nullptr, SP_ALLOC_SCRATCH_REG, TyMachReg, this->m_func);
    IR::RegOpnd *targetOpnd = IR::RegOpnd::New(nullptr, SCRATCH_REG, TyMachReg, this->m_func);

    IR::IntConstOpnd *  stackSizeOpnd   = IR::IntConstOpnd::New((allocSize / MachStackAlignment), TyMachReg, this->m_func, true);

    IR::Instr *movHelperAddrInstr = IR::Instr::New(Js::OpCode::LDIMM, targetOpnd, IR::HelperCallOpnd::New(IR::HelperCRT_chkstk, this->m_func), this->m_func);
    instr->InsertBefore(movHelperAddrInstr);

    IR::Instr *movInstr = IR::Instr::New(Js::OpCode::LDIMM, spAllocOpnd, stackSizeOpnd, this->m_func);
    instr->InsertBefore(movInstr);

    IR::Instr * callInstr = IR::Instr::New(Js::OpCode::BLR, spAllocOpnd, targetOpnd, this->m_func);
    instr->InsertBefore(callInstr);

    // _chkstk succeeded adjust SP by allocSize. r15 contains allocSize/16 so left shift r15 by 4 to get allocSize
    // Generate SUB SP, SP, x15, lsl #4
    IR::Instr * subInstr = IR::Instr::New(Js::OpCode::SUB_LSL4, spOpnd, spOpnd, spAllocOpnd, this->m_func);
    instr->InsertBefore(subInstr);

    // return true to imply scratch register is trashed
    return true;
}

void
LowererMD::GenerateStackDeallocation(IR::Instr *instr, uint32 allocSize)
{
    IR::RegOpnd * spOpnd = IR::RegOpnd::New(nullptr, this->GetRegStackPointer(), TyMachReg, this->m_func);

    IR::Instr * spAdjustInstr = IR::Instr::New(Js::OpCode::ADD,
        spOpnd,
        spOpnd,
        IR::IntConstOpnd::New(allocSize, TyMachReg, this->m_func, true), this->m_func);
    instr->InsertBefore(spAdjustInstr);
    LegalizeMD::LegalizeInstr(spAdjustInstr);
}



class ARM64StackLayout
{
    //
    // Canonical ARM64 prolog/epilog stack layout (stack grows downward):
    //
    //    +-------------------------------------+
    //    |     caller-allocated parameters     |
    //    +=====================================+-----> SP at time of call
    //    |   callee-saved parameters (x0-x7)   |
    //    +-------------------------------------+
    //    |    frame pointer + link register    |
    //    +-------------------------------------+-----> updated FP points here
    //    |  arguments slot + StackFunctionList |
    //    +-------------------------------------+
    //    |   callee-saved registers (x19-x28)  |
    //    +-------------------------------------+
    //    |    callee-saved FP regs (d8-d15)    |
    //    +-------------------------------------+-----> == regOffset
    //    |            locals area              |
    //    +-------------------------------------+-----> locals pointer if not SP
    //    |     caller-allocated parameters     |
    //    +=====================================+-----> SP points here when done
    //

public:
    ARM64StackLayout(Func* func);

    // Getters
    bool HasCalls() const { return m_hasCalls; }
    bool HasTry() const { return m_hasTry; }
    ULONG ArgSlotCount() const { return m_argSlotCount; }
    BitVector HomedParams() const { return m_homedParams; }
    BitVector SavedRegisters() const { return m_savedRegisters; }
    BitVector SavedDoubles() const { return m_savedDoubles; }

    // Locals area sits right after space allocated for arguments
    ULONG LocalsOffset() const { return this->m_argSlotCount * MachRegInt; }
    ULONG LocalsSize() const { return this->m_localsArea; }

    // Saved non-volatile double registers sit past the locals area
    ULONG SavedDoublesOffset() const { return this->LocalsOffset() + this->LocalsSize(); }
    ULONG SavedDoublesSize() const { return this->m_savedDoubles.Count() * MachRegDouble; }

    // Saved non-volatile integer registers sit after the saved doubles
    ULONG SavedRegistersOffset() const { return this->SavedDoublesOffset() + this->SavedDoublesSize(); }
    ULONG SavedRegistersSize() const { return this->m_savedRegisters.Count() * MachRegInt; }

    // The argument slot and StackFunctionList entry come after the saved integer registers
    ULONG ArgSlotOffset() const { return this->SavedRegistersOffset() + this->SavedRegistersSize(); }
    ULONG ArgSlotSize() const { return this->m_hasCalls ? (2 * MachRegInt) : 0; }

    // Next comes the frame chain
    ULONG FpLrOffset() const { return this->ArgSlotOffset() + this->ArgSlotSize(); }
    ULONG FpLrSize() const { return this->m_hasCalls ? (2 * MachRegInt) : 0; }

    // Followed by any homed parameters
    ULONG HomedParamsOffset() const { return this->FpLrOffset() + this->FpLrSize(); }
    ULONG HomedParamsSize() const { return this->m_homedParams.Count() * MachRegInt; }

    // And that's the total stack allocation
    ULONG TotalStackSize() const { return this->HomedParamsOffset() + this->HomedParamsSize(); }

    // The register area is the area at the far end that doesn't include locals or arg slots
    ULONG RegisterAreaOffset() const { return this->SavedDoublesOffset(); }
    ULONG RegisterAreaSize() const { return this->TotalStackSize() - this->RegisterAreaOffset(); }

private:
    bool m_hasCalls;
    bool m_hasTry;
    ULONG m_argSlotCount;
    ULONG m_localsArea;
    BitVector m_homedParams;
    BitVector m_savedRegisters;
    BitVector m_savedDoubles;
};

ARM64StackLayout::ARM64StackLayout(Func* func)
    : m_hasCalls(false),
    m_hasTry(func->HasTry()),
    m_argSlotCount(func->m_argSlotsForFunctionsCalled),
    m_localsArea(func->m_localStackHeight)
{
    Assert(m_localsArea % 16 == 0);
    Assert(m_argSlotCount % 2 == 0);

    // If there is a try, behave specially because the try/catch/finally helpers assume a
    // fully-populated stack layout.
    if (this->m_hasTry)
    {
        this->m_hasCalls = true;
        this->m_savedRegisters.SetRange(FIRST_CALLEE_SAVED_GP_REG, CALLEE_SAVED_GP_REG_COUNT);
        this->m_savedDoubles.SetRange(FIRST_CALLEE_SAVED_DBL_REG, CALLEE_SAVED_DOUBLE_REG_COUNT);
        this->m_homedParams.SetRange(0, NUM_INT_ARG_REGS);
    }

    // Otherwise, be more selective
    else
    {
        // Determine integer register saves. Since registers are always saved in pairs, mark both registers
        // in each pair as being saved even if only one is actually used.
        for (RegNum curReg = FIRST_CALLEE_SAVED_GP_REG; curReg <= LAST_CALLEE_SAVED_GP_REG; curReg = RegNum(curReg + 2))
        {
            Assert(LinearScan::IsCalleeSaved(curReg));
            RegNum nextReg = RegNum(curReg + 1);
            Assert(LinearScan::IsCalleeSaved(nextReg));
            if (func->m_regsUsed.Test(curReg) || func->m_regsUsed.Test(nextReg))
            {
                this->m_savedRegisters.SetRange(curReg, 2);
            }
        }

        // Determine double register saves. Since registers are always saved in pairs, mark both registers
        // in each pair as being saved even if only one is actually used.
        for (RegNum curReg = FIRST_CALLEE_SAVED_DBL_REG; curReg <= LAST_CALLEE_SAVED_DBL_REG; curReg = RegNum(curReg + 2))
        {
            Assert(LinearScan::IsCalleeSaved(curReg));
            RegNum nextReg = RegNum(curReg + 1);
            Assert(LinearScan::IsCalleeSaved(nextReg));
            if (func->m_regsUsed.Test(curReg) || func->m_regsUsed.Test(nextReg))
            {
                this->m_savedDoubles.SetRange(curReg, 2);
            }
        }

        // Determine if there are nested calls.
        //
        // If the function has a try, we need to have the same register saves in the prolog as the 
        // arm64_CallEhFrame helper, so that we can use the same epilog. So always allocate a slot 
        // for the stack nested func here whether we actually do have any stack nested func or not
        // TODO-STACK-NESTED-FUNC:  May be use a different arm64_CallEhFrame for when we have 
        // stack nested func?
        //
        // Note that this->TotalStackSize() will not include the homed parameters yet, so we add in
        // the worst case assumption (homing all NUM_INT_ARG_REGS).
        this->m_hasCalls = func->GetHasCalls() ||
            func->HasAnyStackNestedFunc() || 
            !LowererMD::IsSmallStack(this->TotalStackSize() + NUM_INT_ARG_REGS * MachRegInt) ||
            Lowerer::IsArgSaveRequired(func);

        // Home the params. This is done to enable on-the-fly creation of the arguments object,
        // Dyno bailout code, etc. For non-global functions, that means homing all the param registers
        // (since we have to assume they all have valid parameters). For the global function,
        // just home x0 (function object) and x1 (callinfo), which the runtime can't get by any other means.
        int homedParams = MIN_HOMED_PARAM_REGS;
        if (func->IsLoopBody())
        {
            // Jitted loop body takes only one "user" param: the pointer to the local slots.
            homedParams += 1;
        }
        else if (!this->m_hasCalls)
        {
            // A leaf function (no calls of any kind, including helpers) may still need its params, or, if it
            // has none, may still need the function object and call info.
            homedParams += func->GetInParamsCount();
        }
        else
        {
            homedParams = NUM_INT_ARG_REGS;
        }

        // Round up to an even number to keep stack alignment
        if (homedParams % 2 != 0)
        {
            homedParams += 1;
        }
        this->m_homedParams.SetRange(0, (homedParams < NUM_INT_ARG_REGS) ? homedParams : NUM_INT_ARG_REGS);
    }
}

IR::Instr *
LowererMD::LowerEntryInstr(IR::EntryInstr * entryInstr)
{
    IR::Instr *insertInstr = entryInstr->m_next;

    // Begin recording info for later pdata/xdata emission.
    this->m_func->m_unwindInfo.Init(this->m_func);

    // Ensure there are an even number of slots for called functions
    if (this->m_func->m_argSlotsForFunctionsCalled % 2 != 0)
    {
        this->m_func->m_argSlotsForFunctionsCalled += 1;
    }

    if (this->m_func->HasInlinee())
    {
        // Allocate the inlined arg out stack in the locals. Allocate an additional slot so that
        // we can unconditionally clear the first slot past the current frame.
        this->m_func->m_localStackHeight += this->m_func->GetInlineeArgumentStackSize();
    }

    // Ensure the locals area is 16 byte aligned.
    this->m_func->m_localStackHeight = Math::Align<int32>(this->m_func->m_localStackHeight, MachStackAlignment);

    // Now that the localStackHeight has been adjusted, compute the final layout
    ARM64StackLayout layout(this->m_func);
    Assert(layout.TotalStackSize() % 16 == 0);

    // Set the arguments offset relative to the end of the locals area
    this->m_func->m_ArgumentsOffset = layout.HomedParamsOffset() - (layout.LocalsOffset() + layout.LocalsSize());

    // Set the frame height if inlinee arguments are needed
    if (m_func->GetMaxInlineeArgOutSize() != 0)
    {
        // subtracting 2 for frame pointer & return address
        this->m_func->GetJITOutput()->SetFrameHeight(this->m_func->m_localStackHeight + this->m_func->m_ArgumentsOffset - 2 * MachRegInt);
    }

    // Two situations to handle:
    //
    //    1. If total stack allocation < 512, we can do a single allocation up front
    //    2. Otherwise, we allocate the register area first, save regs, then allocate locals
    //
    // Breaking this down, there are two stack allocations
    //
    //    Allocation 1 = situation1 ? TotalStackSize : RegisterAreaSize
    //    Allocation 2 = TotalStackSize - Allocation 1
    //
    //    <probe>
    // prologStart:
    //    sub   sp, sp, #allocation1
    //    stp   d8-d15, [sp, #savedDoublesOffset - allocation2]
    //    stp   x19-x28, [sp, #savedRegistersOffset - allocation2]
    //    stp   fp, lr, [sp, #fpLrOffset - allocation2]
    //    add   fp, sp, #fpLrOffset - allocation2
    //    sub   sp, sp, #allocation2 (might be call to _chkstk)
    // prologEnd:
    //    stp   zr, zr, [fp, #argSlotOffset - fpLrOffset]
    //    stp   x0-x7, [fp, #paramSaveOffset - fpLrOffset]
    //    add   localsptr, sp, #localsOffset
    //    sub   ehsave, fp, #fpLrOffset - registerAreaOffset
    //

    // Determine the 1 or 2 stack allocation sizes
    ULONG stackAllocation1 = (layout.TotalStackSize() < 512) ? layout.TotalStackSize() : layout.RegisterAreaSize();
    ULONG stackAllocation2 = layout.TotalStackSize() - stackAllocation1;

//    this->GenerateDebugBreak(insertInstr);

    // Generate a stack probe for large stacks first even before register push
    bool fStackProbeAfterProlog = IsSmallStack(layout.TotalStackSize());
    if (!fStackProbeAfterProlog)
    {
        GenerateStackProbe(insertInstr, false);
    }

    // Create the prologStart label
    IR::LabelInstr *prologStartLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    insertInstr->InsertBefore(prologStartLabel);
    this->m_func->m_unwindInfo.SetFunctionOffsetLabel(UnwindPrologStart, prologStartLabel);

    // Perform the initial stack allocation (guaranteed to be small)
    IR::RegOpnd *spOpnd = IR::RegOpnd::New(nullptr, RegSP, TyMachReg, this->m_func);
    if (stackAllocation1 > 0)
    {
        IR::Instr * instrSub = IR::Instr::New(Js::OpCode::SUB, spOpnd, spOpnd, IR::IntConstOpnd::New(stackAllocation1, TyMachReg, this->m_func), this->m_func);
        insertInstr->InsertBefore(instrSub);
    }

    // Save doubles in pairs
    if (!layout.SavedDoubles().IsEmpty())
    {
        ULONG curOffset = layout.SavedDoublesOffset() - stackAllocation2;
        for (RegNum curReg = FIRST_CALLEE_SAVED_DBL_REG; curReg <= LAST_CALLEE_SAVED_DBL_REG; curReg = RegNum(curReg + 2))
        {
            if (layout.SavedDoubles().Test(curReg))
            {
                RegNum nextReg = RegNum(curReg + 1);
                IR::Instr * instrStp = IR::Instr::New(Js::OpCode::FSTP,
                    IR::IndirOpnd::New(spOpnd, curOffset, TyMachReg, this->m_func),
                    IR::RegOpnd::New(curReg, TyMachDouble, this->m_func),
                    IR::RegOpnd::New(nextReg, TyMachDouble, this->m_func), this->m_func);
                insertInstr->InsertBefore(instrStp);
                curOffset += 2 * MachRegDouble;
            }
        }
    }

    // Save integer registers in pairs
    if (!layout.SavedRegisters().IsEmpty())
    {
        ULONG curOffset = layout.SavedRegistersOffset() - stackAllocation2;
        for (RegNum curReg = FIRST_CALLEE_SAVED_GP_REG; curReg <= LAST_CALLEE_SAVED_GP_REG; curReg = RegNum(curReg + 2))
        {
            if (layout.SavedRegisters().Test(curReg))
            {
                RegNum nextReg = RegNum(curReg + 1);
                IR::Instr * instrStp = IR::Instr::New(Js::OpCode::STP,
                    IR::IndirOpnd::New(spOpnd, curOffset, TyMachReg, this->m_func),
                    IR::RegOpnd::New(curReg, TyMachReg, this->m_func),
                    IR::RegOpnd::New(nextReg, TyMachReg, this->m_func), this->m_func);
                insertInstr->InsertBefore(instrStp);
                curOffset += 2 * MachRegInt;
            }
        }
    }

    // Save FP/LR and compute FP
    IR::RegOpnd *fpOpnd = fpOpnd = IR::RegOpnd::New(nullptr, RegFP, TyMachReg, this->m_func);
    if (layout.HasCalls())
    {
        // STP fp, lr, [sp, #offs]
        ULONG fpOffset = layout.FpLrOffset() - stackAllocation2;
        IR::Instr * instrStp = IR::Instr::New(Js::OpCode::STP,
            IR::IndirOpnd::New(spOpnd, fpOffset, TyMachReg, this->m_func),
            fpOpnd, IR::RegOpnd::New(RegLR, TyMachReg, this->m_func), this->m_func);
        insertInstr->InsertBefore(instrStp);

        // ADD fp, sp, #offs
        // For exception handling, do this part AFTER the prolog to allow for proper unwinding
        if (!layout.HasTry())
        {
            Lowerer::InsertAdd(false, fpOpnd, spOpnd, IR::IntConstOpnd::New(fpOffset, TyMachReg, this->m_func), insertInstr);
        }
    }

    // Perform the second (potentially large) stack allocation
    if (stackAllocation2 > 0)
    {
        // TODO: is the probeSize parameter correct here?
        this->GenerateStackAllocation(insertInstr, stackAllocation2, stackAllocation1 + stackAllocation2);
    }

    // Future work in the register area should be done FP-relative if it is set up
    IR::RegOpnd *regAreaBaseOpnd = layout.HasCalls() ? fpOpnd : spOpnd;
    ULONG regAreaBaseOffset = layout.HasCalls() ? layout.FpLrOffset() : 0;

    // This marks the end of the formal prolog (for EH purposes); create and register a label
    IR::LabelInstr *prologEndLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    insertInstr->InsertBefore(prologEndLabel);
    this->m_func->m_unwindInfo.SetFunctionOffsetLabel(UnwindPrologEnd, prologEndLabel);

    // Compute the FP now if there is a try present
    if (layout.HasTry())
    {
        Lowerer::InsertAdd(false, fpOpnd, spOpnd, IR::IntConstOpnd::New(layout.FpLrOffset(), TyMachReg, this->m_func), insertInstr);
    }

    // Zero the argument slot if present
    IR::RegOpnd *zrOpnd = IR::RegOpnd::New(nullptr, RegZR, TyMachReg, this->m_func);
    if (layout.ArgSlotSize() > 0)
    {
        IR::Instr * instrStp = IR::Instr::New(Js::OpCode::STP,
            IR::IndirOpnd::New(regAreaBaseOpnd, layout.ArgSlotOffset() - regAreaBaseOffset, TyMachReg, this->m_func),
            zrOpnd, zrOpnd, this->m_func);
        insertInstr->InsertBefore(instrStp);
    }

    // Home parameter registers in pairs
    if (!layout.HomedParams().IsEmpty())
    {
        ULONG curOffset = layout.HomedParamsOffset() - regAreaBaseOffset;
        for (RegNum curReg = FIRST_INT_ARG_REG; curReg <= LAST_INT_ARG_REG; curReg = RegNum(curReg + 2))
        {
            if (layout.HomedParams().Test(curReg))
            {
                RegNum nextReg = RegNum(curReg + 1);
                IR::Instr * instrStp = IR::Instr::New(Js::OpCode::STP,
                    IR::IndirOpnd::New(regAreaBaseOpnd, curOffset, TyMachReg, this->m_func),
                    IR::RegOpnd::New(curReg, TyMachReg, this->m_func),
                    IR::RegOpnd::New(nextReg, TyMachReg, this->m_func), this->m_func);
                insertInstr->InsertBefore(instrStp);
                curOffset += 2 * MachRegInt;
            }
        }
    }

    // Compute the locals pointer if needed
    RegNum localsReg = this->m_func->GetLocalsPointer();
    if (localsReg != RegSP)
    {
        IR::RegOpnd* localsOpnd = IR::RegOpnd::New(nullptr, localsReg, TyMachReg, this->m_func);
        Lowerer::InsertAdd(false, localsOpnd, spOpnd, IR::IntConstOpnd::New(layout.LocalsOffset(), TyMachReg, this->m_func), insertInstr);
    }

    // Zero initialize the first inlinee frames argc.
    if (this->m_func->GetMaxInlineeArgOutSize() != 0)
    {
        // STR argc, zr
        StackSym *sym = this->m_func->m_symTable->GetArgSlotSym((Js::ArgSlot) - 1);
        sym->m_isInlinedArgSlot = true;
        sym->m_offset = 0;
        IR::Instr * instrStr = IR::Instr::New(Js::OpCode::STR, IR::SymOpnd::New(sym, 0, TyMachReg, this->m_func), zrOpnd, this->m_func);
        insertInstr->InsertBefore(instrStr);
    }

    // Now do the stack probe for small stacks
    // hasCalls catches the recursion case
    if (layout.HasCalls() && fStackProbeAfterProlog)
    {
        GenerateStackProbe(insertInstr, true); //stack is already aligned in this case
    }

    return entryInstr;
}

IR::Instr *
LowererMD::LowerExitInstr(IR::ExitInstr * exitInstr)
{
    // Compute the final layout (should match the prolog)
    ARM64StackLayout layout(this->m_func);
    Assert(layout.TotalStackSize() % 16 == 0);

    // Determine the 1 or 2 stack allocation sizes
    // Note that on exit, if there is a try, we always do a 2-step deallocation because the
    // epilog is re-used by the try/catch/finally code
    ULONG stackAllocation1 = (layout.TotalStackSize() < 512 && !layout.HasTry()) ? layout.TotalStackSize() : layout.RegisterAreaSize();
    ULONG stackAllocation2 = layout.TotalStackSize() - stackAllocation1;

    // Mark the start of the epilog
    IR::LabelInstr *epilogStartLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    exitInstr->InsertBefore(epilogStartLabel);
    this->m_func->m_unwindInfo.SetFunctionOffsetLabel(UnwindEpilogStart, epilogStartLabel);

    IR::RegOpnd *spOpnd = IR::RegOpnd::New(nullptr, RegSP, TyMachReg, this->m_func);
    IR::RegOpnd *fpOpnd = IR::RegOpnd::New(nullptr, RegFP, TyMachReg, this->m_func);

    // Exception handling regions exit via the same epilog
    IR::LabelInstr* ehEpilogLabel = this->m_func->m_epilogLabel;
    if (ehEpilogLabel != nullptr)
    {
        ehEpilogLabel->Unlink();
        exitInstr->InsertBefore(ehEpilogLabel);
    }

    // Undo the last stack allocation
    if (stackAllocation2 > 0)
    {
        GenerateStackDeallocation(exitInstr, stackAllocation2);
    }

    // Recover FP and LR
    if (layout.HasCalls())
    {
        // LDP fp, lr, [sp, #offs]
        ULONG fpOffset = layout.FpLrOffset() - stackAllocation2;
        IR::Instr * instrLdp = IR::Instr::New(Js::OpCode::LDP, fpOpnd,
            IR::IndirOpnd::New(spOpnd, fpOffset, TyMachReg, this->m_func),
            IR::RegOpnd::New(RegLR, TyMachReg, this->m_func), this->m_func);
        exitInstr->InsertBefore(instrLdp);
    }

    // Recover integer registers in pairs
    if (!layout.SavedRegisters().IsEmpty())
    {
        ULONG curOffset = layout.SavedRegistersOffset() - stackAllocation2 + layout.SavedRegistersSize();
        for (RegNum curReg = RegNum(LAST_CALLEE_SAVED_GP_REG - 1); curReg >= FIRST_CALLEE_SAVED_GP_REG; curReg = RegNum(curReg - 2))
        {
            if (layout.SavedRegisters().Test(curReg))
            {
                curOffset -= 2 * MachRegInt;
                RegNum nextReg = RegNum(curReg + 1);
                IR::Instr * instrLdp = IR::Instr::New(Js::OpCode::LDP,
                    IR::RegOpnd::New(curReg, TyMachReg, this->m_func),
                    IR::IndirOpnd::New(spOpnd, curOffset, TyMachReg, this->m_func),
                    IR::RegOpnd::New(nextReg, TyMachReg, this->m_func), this->m_func);
                exitInstr->InsertBefore(instrLdp);
            }
        }
    }

    // Recover doubles in pairs
    if (!layout.SavedDoubles().IsEmpty())
    {
        ULONG curOffset = layout.SavedDoublesOffset() - stackAllocation2 + layout.SavedDoublesSize();
        for (RegNum curReg = RegNum(LAST_CALLEE_SAVED_DBL_REG - 1); curReg >= FIRST_CALLEE_SAVED_DBL_REG; curReg = RegNum(curReg - 2))
        {
            if (layout.SavedDoubles().Test(curReg))
            {
                curOffset -= 2 * MachRegDouble;
                RegNum nextReg = RegNum(curReg + 1);
                IR::Instr * instrLdp = IR::Instr::New(Js::OpCode::FLDP,
                    IR::RegOpnd::New(curReg, TyMachDouble, this->m_func),
                    IR::IndirOpnd::New(spOpnd, curOffset, TyMachReg, this->m_func),
                    IR::RegOpnd::New(nextReg, TyMachDouble, this->m_func), this->m_func);
                exitInstr->InsertBefore(instrLdp);
            }
        }
    }

    // Final stack deallocation
    if (stackAllocation1 > 0)
    {
        GenerateStackDeallocation(exitInstr, stackAllocation1);
    }

    // Return
    IR::Instr *  instrRet = IR::Instr::New(Js::OpCode::RET, nullptr, IR::RegOpnd::New(nullptr, RegLR, TyMachReg, this->m_func), this->m_func);
    exitInstr->InsertBefore(instrRet);

    // Label the end
    IR::LabelInstr *epilogEndLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    exitInstr->InsertBefore(epilogEndLabel);
    this->m_func->m_unwindInfo.SetFunctionOffsetLabel(UnwindEpilogEnd, epilogEndLabel);

    return exitInstr;
}

IR::Instr *
LowererMD::LoadNewScObjFirstArg(IR::Instr * instr, IR::Opnd * argSrc, ushort extraArgs)
{
    // Spread moves down the argument slot by one.
    // LowerCallArgs will handle the extraArgs. We only need to specify the argument number
    // i.e 1 and not + extraArgs as done in AMD64
    IR::SymOpnd *argOpnd = IR::SymOpnd::New(this->m_func->m_symTable->GetArgSlotSym(1), TyVar, this->m_func);
    IR::Instr *argInstr = IR::Instr::New(Js::OpCode::ArgOut_A, argOpnd, argSrc, this->m_func);
    instr->InsertBefore(argInstr);

    // Insert the argument into the arg chain.
    if (m_lowerer->IsSpreadCall(instr))
    {
        // Spread calls need LdSpreadIndices as the last arg in the arg chain.
        instr = m_lowerer->GetLdSpreadIndicesInstr(instr);
    }
    IR::Opnd *linkOpnd = instr->UnlinkSrc2();
    argInstr->SetSrc2(linkOpnd);
    instr->SetSrc2(argOpnd);

    return argInstr;
}

IR::Instr *
LowererMD::LowerTry(IR::Instr * tryInstr, IR::JnHelperMethod helperMethod)
{
    // Mark the entry to the try
    IR::Instr * instr = tryInstr->GetNextRealInstrOrLabel();
    AssertMsg(instr->IsLabelInstr(), "No label at the entry to a try?");
    IR::LabelInstr * tryAddr = instr->AsLabelInstr();

    // Arg 7: ScriptContext
    this->m_lowerer->LoadScriptContext(tryAddr);

    if (tryInstr->m_opcode == Js::OpCode::TryCatch || this->m_func->DoOptimizeTry() || (this->m_func->IsSimpleJit() && this->m_func->hasBailout))
    {
        // Arg 6 : hasBailedOutOffset
        IR::Opnd * hasBailedOutOffset = IR::IntConstOpnd::New(this->m_func->GetHasBailedOutSym()->m_offset + tryInstr->m_func->GetInlineeArgumentStackSize(), TyInt32, this->m_func);
        this->LoadHelperArgument(tryAddr, hasBailedOutOffset);
    }

    // Arg 5: arg out size
    IR::RegOpnd * argOutSize = IR::RegOpnd::New(TyMachReg, this->m_func);
    instr = IR::Instr::New(Js::OpCode::LDARGOUTSZ, argOutSize, this->m_func);
    tryAddr->InsertBefore(instr);
    this->LoadHelperArgument(tryAddr, argOutSize);

    // Arg 4: locals pointer
    IR::RegOpnd * localsPtr = IR::RegOpnd::New(nullptr, this->m_func->GetLocalsPointer(), TyMachReg, this->m_func);
    this->LoadHelperArgument(tryAddr, localsPtr);

    // Arg 3: frame pointer
    IR::RegOpnd * framePtr = IR::RegOpnd::New(nullptr, FRAME_REG, TyMachReg, this->m_func);
    this->LoadHelperArgument(tryAddr, framePtr);

    // Arg 2: helper address
    IR::LabelInstr * helperAddr = tryInstr->AsBranchInstr()->GetTarget();
    this->LoadHelperArgument(tryAddr, IR::LabelOpnd::New(helperAddr, this->m_func));

    // Arg 1: try address
    this->LoadHelperArgument(tryAddr, IR::LabelOpnd::New(tryAddr, this->m_func));

    // Call the helper
    IR::RegOpnd *continuationAddr =
        IR::RegOpnd::New(StackSym::New(TyMachReg,this->m_func), RETURN_REG, TyMachReg, this->m_func);
    IR::Instr * callInstr = IR::Instr::New(
        Js::OpCode::Call, continuationAddr, IR::HelperCallOpnd::New(helperMethod, this->m_func), this->m_func);
    tryAddr->InsertBefore(callInstr);
    this->LowerCall(callInstr, 0);

    // Jump to the continuation address supplied by the helper
    IR::BranchInstr *branchInstr = IR::MultiBranchInstr::New(Js::OpCode::BR, continuationAddr, this->m_func);
    tryAddr->InsertBefore(branchInstr);

    return tryInstr->m_prev;
}

IR::Instr *
LowererMD::LowerLeaveNull(IR::Instr * leaveInstr)
{
    IR::Instr * instrPrev = leaveInstr->m_prev;

    // Return a NULL continuation address to the caller to indicate that the finally did not seize the flow.
    this->LowerEHRegionReturn(leaveInstr, IR::IntConstOpnd::New(0, TyMachReg, this->m_func));

    leaveInstr->Remove();
    return instrPrev;
}

IR::Instr *
LowererMD::LowerEHRegionReturn(IR::Instr * insertBeforeInstr, IR::Opnd * targetOpnd)
{
    IR::RegOpnd *retReg    = IR::RegOpnd::New(nullptr, RETURN_REG, TyMachReg, this->m_func);

    // Load the continuation address into the return register.
    Lowerer::InsertMove(retReg, targetOpnd, insertBeforeInstr);

    IR::LabelInstr *epilogLabel = this->EnsureEHEpilogLabel();
    IR::BranchInstr *jmpInstr = IR::BranchInstr::New(Js::OpCode::B, epilogLabel, this->m_func);
    insertBeforeInstr->InsertBefore(jmpInstr);

    // return the last instruction inserted
    return jmpInstr;
}

///----------------------------------------------------------------------------
///
/// LowererMD::Init
///
///----------------------------------------------------------------------------

void
LowererMD::Init(Lowerer *lowerer)
{
    m_lowerer = lowerer;
    // The arg slot count computed by an earlier phase (e.g., IRBuilder) doesn't work for
    // ARM if it accounts for nesting. Clear it here and let Lower compute its own value.
    this->m_func->m_argSlotsForFunctionsCalled = 0;
}

///----------------------------------------------------------------------------
///
/// LowererMD::LoadInputParamPtr
///
///     Load the address of the start of the passed-in parameters not including
///     the this parameter.
///
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::LoadInputParamPtr(IR::Instr * instrInsert, IR::RegOpnd * optionalDstOpnd /* = nullptr */)
{
    if (this->m_func->GetJITFunctionBody()->IsCoroutine())
    {
        IR::RegOpnd * argPtrRegOpnd = Lowerer::LoadGeneratorArgsPtr(instrInsert);
        IR::IndirOpnd * indirOpnd = IR::IndirOpnd::New(argPtrRegOpnd, 1 * MachPtr, TyMachPtr, this->m_func);
        IR::RegOpnd * dstOpnd = optionalDstOpnd != nullptr ? optionalDstOpnd : IR::RegOpnd::New(TyMachPtr, this->m_func);

        return Lowerer::InsertLea(dstOpnd, indirOpnd, instrInsert);
    }
    else
    {
        StackSym * paramSym = GetImplicitParamSlotSym(3);
        return this->m_lowerer->InsertLoadStackAddress(paramSym, instrInsert);
    }
}

///----------------------------------------------------------------------------
///
/// LowererMD::LoadInputParamCount
///
///     Load the passed-in parameter count from the appropriate slot.
///
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::LoadInputParamCount(IR::Instr * instrInsert, int adjust, bool needFlags)
{
    //  LDR  Rz, CallInfo
    //  UBFX Rx, Rz, 27, #1 // Get CallEval bit.
    //  UBFX Rz, Rz, 0, #24 // Extract call count 
    //  SUB  Rz, Rz, Rx     // Now Rz has the right number of parameters

    IR::SymOpnd * srcOpnd = Lowerer::LoadCallInfo(instrInsert);
    IR::RegOpnd * dstOpnd = IR::RegOpnd::New(TyMachReg,  this->m_func);

    IR::Instr *instr = IR::Instr::New(Js::OpCode::LDR, dstOpnd, srcOpnd,  this->m_func);
    instrInsert->InsertBefore(instr);

    // Get the actual call count. On ARM64 top 32 bits are unused
    instr = IR::Instr::New(Js::OpCode::UBFX, dstOpnd, dstOpnd, IR::IntConstOpnd::New(BITFIELD(0, Js::CallInfo::ksizeofCount), TyMachReg, this->m_func), this->m_func);
    instrInsert->InsertBefore(instr);

    return Lowerer::InsertSub(needFlags, dstOpnd, dstOpnd, IR::IntConstOpnd::New(-adjust, TyUint32, this->m_func), instrInsert);
}

IR::Instr *
LowererMD::LoadStackArgPtr(IR::Instr * instr)
{
    if (this->m_func->IsLoopBody())
    {
        // Get the first user param from the interpreter frame instance that was passed in.
        // These args don't include the func object and callinfo; we just need to advance past "this".

        // t1 = LDR [prm1 + m_inParams]
        // dst = ADD t1, sizeof(var)

        Assert(this->m_func->GetLoopParamSym());
        IR::RegOpnd *baseOpnd = IR::RegOpnd::New(this->m_func->GetLoopParamSym(), TyMachReg, this->m_func);
        size_t offset = Js::InterpreterStackFrame::GetOffsetOfInParams();
        IR::IndirOpnd *indirOpnd = IR::IndirOpnd::New(baseOpnd, (int32)offset, TyMachReg, this->m_func);
        IR::RegOpnd *tmpOpnd = IR::RegOpnd::New(TyMachReg, this->m_func);
        Lowerer::InsertMove(tmpOpnd, indirOpnd, instr);

        instr->SetSrc1(tmpOpnd);
        instr->SetSrc2(IR::IntConstOpnd::New(sizeof(Js::Var), TyMachReg, this->m_func));
    }
    else if (this->m_func->GetJITFunctionBody()->IsCoroutine())
    {
        IR::Instr *instr2 = LoadInputParamPtr(instr, instr->UnlinkDst()->AsRegOpnd());
        instr->Remove();
        instr = instr2;
    }
    else
    {
        // Get the args pointer relative to fp. We assume that fp is set up, since we'll only be looking
        // for the stack arg pointer in a non-leaf.

        // dst = ADD r11, "this" offset + sizeof(var)

        instr->SetSrc1(IR::RegOpnd::New(nullptr, FRAME_REG, TyMachReg, this->m_func));
        instr->SetSrc2(IR::IntConstOpnd::New((ArgOffsetFromFramePtr + Js::JavascriptFunctionArgIndex_SecondScriptArg) * sizeof(Js::Var), TyMachReg, this->m_func));
    }

    instr->m_opcode = Js::OpCode::ADD;
    Legalize(instr);

    return instr->m_prev;
}

IR::Instr *
LowererMD::LoadArgumentsFromFrame(IR::Instr * instr)
{
    IR::RegOpnd *baseOpnd;
    int32 offset;

    if (this->m_func->IsLoopBody())
    {
        // Get the arguments ptr from the interpreter frame instance that was passed in.
        Assert(this->m_func->GetLoopParamSym());
        baseOpnd = IR::RegOpnd::New(this->m_func->GetLoopParamSym(), TyMachReg, this->m_func);
        offset = Js::InterpreterStackFrame::GetOffsetOfArguments();
    }
    else
    {
        // Get the arguments relative to the frame pointer.
        baseOpnd = IR::RegOpnd::New(nullptr, FRAME_REG, TyMachReg, this->m_func);
        offset = -MachArgsSlotOffset;
    }

    instr->SetSrc1(IR::IndirOpnd::New(baseOpnd, offset, TyMachReg, this->m_func));
    this->ChangeToAssign(instr);

    return instr->m_prev;
}

// load argument count as I4
IR::Instr *
LowererMD::LoadArgumentCount(IR::Instr * instr)
{
    IR::RegOpnd *baseOpnd;
    int32 offset;

    if (this->m_func->IsLoopBody())
    {
        // Pull the arg count from the interpreter frame instance that was passed in.
        // (The callinfo in the loop body's frame just shows the single parameter, the interpreter frame.)
        Assert(this->m_func->GetLoopParamSym());
        baseOpnd = IR::RegOpnd::New(this->m_func->GetLoopParamSym(), TyMachReg, this->m_func);
        offset = Js::InterpreterStackFrame::GetOffsetOfInSlotsCount();
    }
    else
    {
        baseOpnd = IR::RegOpnd::New(nullptr, FRAME_REG, TyMachReg, this->m_func);
        offset = (ArgOffsetFromFramePtr + Js::JavascriptFunctionArgIndex_CallInfo) * sizeof(Js::Var);
    }

    instr->SetSrc1(IR::IndirOpnd::New(baseOpnd, offset, TyInt32, this->m_func));
    this->ChangeToAssign(instr);

    return instr->m_prev;
}

///----------------------------------------------------------------------------
///
/// LowererMD::LoadHeapArguments
///
///     Load the arguments object
///     NOTE: The same caveat regarding arguments passed on the stack applies here
///           as in LoadInputParamCount above.
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::LoadHeapArguments(IR::Instr * instrArgs)
{
    ASSERT_INLINEE_FUNC(instrArgs);
    Func *func = instrArgs->m_func;
    IR::Instr * instrPrev = instrArgs->m_prev;

    if (func->IsStackArgsEnabled())
    {
        // The initial args slot value is zero.
        instrArgs->m_opcode = Js::OpCode::LDIMM;
        instrArgs->ReplaceSrc1(IR::AddrOpnd::NewNull(func));
        if (PHASE_TRACE1(Js::StackArgFormalsOptPhase) && func->GetJITFunctionBody()->GetInParamsCount() > 1)
        {
            Output::Print(_u("StackArgFormals : %s (%d) :Removing Heap Arguments object creation in Lowerer. \n"), instrArgs->m_func->GetJITFunctionBody()->GetDisplayName(), instrArgs->m_func->GetFunctionNumber());
            Output::Flush();
        }
    }
    else
    {
        // s7 = formals are let decls
        // s6 = memory context
        // s5 = array of property ID's
        // s4 = local frame instance
        // s3 = address of first actual argument (after "this")
        // s2 = actual argument count
        // s1 = current function
        // dst = JavascriptOperators::LoadHeapArguments(s1, s2, s3, s4, s5, s6, s7)

        // s7 = formals are let decls
        this->LoadHelperArgument(instrArgs, IR::IntConstOpnd::New(instrArgs->m_opcode == Js::OpCode::LdLetHeapArguments ? TRUE : FALSE, TyUint8, func));

        // s6 = memory context
        this->m_lowerer->LoadScriptContext(instrArgs);

        // s5 = array of property ID's

        intptr_t formalsPropIdArray = instrArgs->m_func->GetJITFunctionBody()->GetFormalsPropIdArrayAddr();
        if (!formalsPropIdArray)
        {
            formalsPropIdArray = instrArgs->m_func->GetScriptContextInfo()->GetNullAddr();
        }

        IR::Opnd * argArray = IR::AddrOpnd::New(formalsPropIdArray, IR::AddrOpndKindDynamicMisc, m_func);
        this->LoadHelperArgument(instrArgs, argArray);

        // s4 = local frame instance
        IR::Opnd * frameObj = instrArgs->UnlinkSrc1();
        this->LoadHelperArgument(instrArgs, frameObj);

        if (func->IsInlinee())
        {
            // s3 = address of first actual argument (after "this").
            StackSym *firstRealArgSlotSym = func->GetInlineeArgvSlotOpnd()->m_sym->AsStackSym();
            this->m_func->SetArgOffset(firstRealArgSlotSym, firstRealArgSlotSym->m_offset + MachPtr);
            IR::Instr *instr = this->m_lowerer->InsertLoadStackAddress(firstRealArgSlotSym, instrArgs);
            this->LoadHelperArgument(instrArgs, instr->GetDst());

            // s2 = actual argument count (without counting "this").
            this->LoadHelperArgument(instrArgs, IR::IntConstOpnd::New(func->actualCount - 1, TyUint32, func));

            // s1 = current function.
            this->LoadHelperArgument(instrArgs, func->GetInlineeFunctionObjectSlotOpnd());

            // Save the newly-created args object to its dedicated stack slot.
            IR::SymOpnd *argObjSlotOpnd = func->GetInlineeArgumentsObjectSlotOpnd();
            Lowerer::InsertMove(argObjSlotOpnd,instrArgs->GetDst(), instrArgs->m_next);
        }
        else
        {
            // s3 = address of first actual argument (after "this")
            // Stack looks like (function object)+0, (arg count)+4, (this)+8, actual args
            IR::Instr * instr = this->LoadInputParamPtr(instrArgs);
            this->LoadHelperArgument(instrArgs, instr->GetDst());

            // s2 = actual argument count (without counting "this")
            instr = this->LoadInputParamCount(instrArgs, -1);
            IR::Opnd * opndInputParamCount = instr->GetDst();
            
            this->LoadHelperArgument(instrArgs, opndInputParamCount);

            // s1 = current function
            StackSym * paramSym = GetImplicitParamSlotSym(0);
            IR::Opnd * srcOpnd = IR::SymOpnd::New(paramSym, TyMachReg, func);
            this->LoadHelperArgument(instrArgs, srcOpnd);

            // Save the newly-created args object to its dedicated stack slot.
            Lowerer::InsertMove(LowererMD::CreateStackArgumentsSlotOpnd(func), instrArgs->GetDst(), instrArgs->m_next);
        }
        this->ChangeToHelperCall(instrArgs, IR::HelperOp_LoadHeapArguments);
    }
    return instrPrev;
}

///----------------------------------------------------------------------------
///
/// LowererMD::LoadHeapArgsCached
///
///     Load the heap-based arguments object using a cached scope
///
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::LoadHeapArgsCached(IR::Instr * instrArgs)
{
    Assert(!this->m_func->GetJITFunctionBody()->IsGenerator());
    ASSERT_INLINEE_FUNC(instrArgs);
    Func *func = instrArgs->m_func;
    IR::Instr * instrPrev = instrArgs->m_prev;

    if (instrArgs->m_func->IsStackArgsEnabled())
    {
        instrArgs->m_opcode = Js::OpCode::LDIMM;
        instrArgs->ReplaceSrc1(IR::AddrOpnd::NewNull(func));

        if (PHASE_TRACE1(Js::StackArgFormalsOptPhase) && func->GetJITFunctionBody()->GetInParamsCount() > 1)
        {
            Output::Print(_u("StackArgFormals : %s (%d) :Removing Heap Arguments object creation in Lowerer. \n"), instrArgs->m_func->GetJITFunctionBody()->GetDisplayName(), instrArgs->m_func->GetFunctionNumber());
            Output::Flush();
        }
    }
    else
    {
        // s7 = formals are let decls
        // s6 = memory context
        // s5 = local frame instance
        // s4 = address of first actual argument (after "this")
        // s3 = formal argument count
        // s2 = actual argument count
        // s1 = current function
        // dst = JavascriptOperators::LoadHeapArgsCached(s1, s2, s3, s4, s5, s6, s7)


        // s7 = formals are let decls
        IR::Opnd * formalsAreLetDecls = IR::IntConstOpnd::New((IntConstType)(instrArgs->m_opcode == Js::OpCode::LdLetHeapArgsCached), TyUint8, func);
        this->LoadHelperArgument(instrArgs, formalsAreLetDecls);

        // s6 = memory context
        this->m_lowerer->LoadScriptContext(instrArgs);

        // s5 = local frame instance
        IR::Opnd * frameObj = instrArgs->UnlinkSrc1();
        this->LoadHelperArgument(instrArgs, frameObj);

        if (func->IsInlinee())
        {
            // s4 = address of first actual argument (after "this").
            StackSym *firstRealArgSlotSym = func->GetInlineeArgvSlotOpnd()->m_sym->AsStackSym();
            this->m_func->SetArgOffset(firstRealArgSlotSym, firstRealArgSlotSym->m_offset + MachPtr);

            IR::Instr *instr = this->m_lowerer->InsertLoadStackAddress(firstRealArgSlotSym, instrArgs);
            this->LoadHelperArgument(instrArgs, instr->GetDst());

            // s3 = formal argument count (without counting "this").
        uint32 formalsCount = func->GetJITFunctionBody()->GetInParamsCount() - 1;
            this->LoadHelperArgument(instrArgs, IR::IntConstOpnd::New(formalsCount, TyUint32, func));

            // s2 = actual argument count (without counting "this").
            this->LoadHelperArgument(instrArgs, IR::IntConstOpnd::New(func->actualCount - 1, TyUint32, func));

            // s1 = current function.
            this->LoadHelperArgument(instrArgs, func->GetInlineeFunctionObjectSlotOpnd());

            // Save the newly-created args object to its dedicated stack slot.
            IR::SymOpnd *argObjSlotOpnd = func->GetInlineeArgumentsObjectSlotOpnd();
            Lowerer::InsertMove(argObjSlotOpnd, instrArgs->GetDst(), instrArgs->m_next);
        }
        else
        {
            // s4 = address of first actual argument (after "this")
            IR::Instr * instr = this->LoadInputParamPtr(instrArgs);
            this->LoadHelperArgument(instrArgs, instr->GetDst());

            // s3 = formal argument count (without counting "this")
            uint32 formalsCount = func->GetInParamsCount() - 1;
            this->LoadHelperArgument(instrArgs, IR::IntConstOpnd::New(formalsCount, TyMachReg, func));

            // s2 = actual argument count (without counting "this")
            instr = this->LoadInputParamCount(instrArgs, -1);
            this->LoadHelperArgument(instrArgs, instr->GetDst());

            // s1 = current function
            StackSym * paramSym = GetImplicitParamSlotSym(0);
            IR::Opnd * srcOpnd = IR::SymOpnd::New(paramSym, TyMachReg, func);
            this->LoadHelperArgument(instrArgs, srcOpnd);


            // Save the newly-created args object to its dedicated stack slot.
            Lowerer::InsertMove(LowererMD::CreateStackArgumentsSlotOpnd(func), instrArgs->GetDst(), instrArgs->m_next);

        }

        this->ChangeToHelperCall(instrArgs, IR::HelperOp_LoadHeapArgsCached);
    }
    return instrPrev;
}

///----------------------------------------------------------------------------
///
/// LowererMD::ChangeToHelperCall
///
///     Change the current instruction to a call to the given helper.
///
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::ChangeToHelperCall(IR::Instr * callInstr, IR::JnHelperMethod helperMethod, IR::LabelInstr *labelBailOut,
                              IR::Opnd *opndInstance, IR::PropertySymOpnd *propSymOpnd, bool isHelperContinuation)
{
#if DBG
    this->m_lowerer->ReconcileWithLowererStateOnHelperCall(callInstr, helperMethod);
#endif
    IR::Instr * bailOutInstr = callInstr;
    if (callInstr->HasBailOutInfo())
    {
        const IR::BailOutKind bailOutKind = callInstr->GetBailOutKind();
        if (bailOutKind == IR::BailOutOnNotPrimitive)
        {
            callInstr = IR::Instr::New(callInstr->m_opcode, callInstr->m_func);
            bailOutInstr->TransferTo(callInstr);
            bailOutInstr->InsertBefore(callInstr);

            bailOutInstr->m_opcode = Js::OpCode::BailOnNotPrimitive;
            bailOutInstr->SetSrc1(opndInstance);
        }
        else if (BailOutInfo::IsBailOutOnImplicitCalls(bailOutKind))
        {
            bailOutInstr = this->m_lowerer->SplitBailOnImplicitCall(callInstr);
        }
        else
        {
            AssertMsg(false, "Unexpected BailOutKind, are we adding new BailOutKind on instructions?");
        }
    }

    IR::HelperCallOpnd *helperCallOpnd = Lowerer::CreateHelperCallOpnd(helperMethod, this->GetHelperArgsCount(), m_func);
    if (helperCallOpnd->IsDiagHelperCallOpnd())
    {
        // Load arguments for the wrapper.
        this->LoadHelperArgument(callInstr, IR::AddrOpnd::New((Js::Var)IR::GetMethodOriginalAddress(m_func->GetThreadContextInfo(), helperMethod), IR::AddrOpndKindDynamicMisc, m_func));
        this->m_lowerer->LoadScriptContext(callInstr);
    }
    callInstr->SetSrc1(helperCallOpnd);

    IR::Instr * instrRet = this->LowerCall(callInstr, 0);

    if (bailOutInstr != callInstr)
    {
        // The bailout needs to be lowered after we lower the helper call because the helper argument
        // has already been loaded. We need to drain them on AMD64 before starting another helper call
        if (bailOutInstr->m_opcode == Js::OpCode::BailOnNotPrimitive)
        {
            this->m_lowerer->LowerBailOnTrue(bailOutInstr, labelBailOut);
        }
        else if (bailOutInstr->m_opcode == Js::OpCode::BailOnNotEqual)
        {
            // `SplitBailOnImplicitCall` above changes the opcode to BailOnNotEqual
            Assert(BailOutInfo::IsBailOutOnImplicitCalls(bailOutInstr->GetBailOutKind()));
            this->m_lowerer->LowerBailOnEqualOrNotEqual(bailOutInstr, nullptr, labelBailOut, propSymOpnd, isHelperContinuation);
        }
        else
        {
            AssertMsg(false, "Unexpected OpCode for BailOutInstruction");
        }
    }

    return instrRet;
}

IR::Instr* LowererMD::ChangeToHelperCallMem(IR::Instr * instr,  IR::JnHelperMethod helperMethod)
{
    this->m_lowerer->LoadScriptContext(instr);

    return this->ChangeToHelperCall(instr, helperMethod);
}

///----------------------------------------------------------------------------
///
/// LowererMD::ChangeToAssign
///
///     Change to a copy. Handle riscification of operands.
///
///----------------------------------------------------------------------------

// ToDo (SaAgarwa) Copied from ARM32 to compile. Validate is this correct
IR::Instr *
LowererMD::ChangeToAssignNoBarrierCheck(IR::Instr * instr)
{
    return ChangeToAssign(instr, instr->GetDst()->GetType());
}

IR::Instr *
LowererMD::ChangeToAssign(IR::Instr * instr)
{
    return ChangeToAssign(instr, instr->GetDst()->GetType());
}

IR::Instr *
LowererMD::ChangeToAssign(IR::Instr * instr, IRType destType)
{
    Assert(!instr->HasBailOutInfo() || instr->GetBailOutKind() == IR::BailOutExpectingInteger
                                       || instr->GetBailOutKind() == IR::BailOutExpectingString);

    IR::Opnd *src = instr->GetSrc1();
    IRType srcType = src->GetType();
    if (src->IsImmediateOpnd() || src->IsLabelOpnd())
    {
        instr->m_opcode = Js::OpCode::LDIMM;
    }
    else if(destType == TyFloat32 && instr->GetDst()->IsRegOpnd())
    {
        Assert(instr->GetSrc1()->IsFloat32());
        instr->m_opcode = Js::OpCode::FLDR;

        // Note that we allocate double register for single precision floats as well, as the register allocator currently
        // does not support 32-bit float registers
        instr->ReplaceDst(instr->GetDst()->UseWithNewType(TyFloat64, instr->m_func));
        if(instr->GetSrc1()->IsRegOpnd())
        {
            instr->ReplaceSrc1(instr->GetSrc1()->UseWithNewType(TyFloat64, instr->m_func));
        }
    }
    else if (!src->IsIndirOpnd() && TySize[destType] > TySize[srcType] && (IRType_IsSignedInt(destType) || IRType_IsUnsignedInt(destType)))
    {
        // If we're moving between different lengths of registers, we need to use the
        // right operator - sign extend if the source is int, zero extend if uint.
        if (IRType_IsSignedInt(srcType))
        {
            instr->ReplaceSrc1(src->UseWithNewType(IRType_EnsureSigned(destType), instr->m_func));
            instr->SetSrc2(IR::IntConstOpnd::New(BITFIELD(0, TySize[srcType] * MachBits), TyMachReg, instr->m_func, true));
            instr->m_opcode = Js::OpCode::SBFX;
        }
        else if (IRType_IsUnsignedInt(srcType))
        {
            instr->ReplaceSrc1(src->UseWithNewType(IRType_EnsureUnsigned(destType), instr->m_func));
            instr->SetSrc2(IR::IntConstOpnd::New(BITFIELD(0, TySize[srcType] * MachBits), TyMachReg, instr->m_func, true));
            instr->m_opcode = Js::OpCode::UBFX;
        }
        else
        {
            AssertMsg(false, "argument size mismatch for mov instruction, with non int/uint types!");
        }
    }
    else
    {
        instr->m_opcode = IRType_IsFloat(destType) ? Js::OpCode::FMOV : Js::OpCode::MOV;
    }

    AutoRestoreLegalize restore(instr->m_func, false);
    LegalizeMD::LegalizeInstr(instr);

    return instr;
}

IR::Instr *
LowererMD::ChangeToWriteBarrierAssign(IR::Instr * assignInstr, const Func* func)
{
#ifdef RECYCLER_WRITE_BARRIER_JIT
    // WriteBarrier-TODO- Implement ARM JIT
#endif
    return ChangeToAssignNoBarrierCheck(assignInstr);
}

///----------------------------------------------------------------------------
///
/// LowererMD::LowerRet
///
///     Lower Ret to "MOV EAX, src"
///     The real RET is inserted at the exit of the function when emitting the
///     epilog.
///
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::LowerRet(IR::Instr * retInstr)
{
    IR::RegOpnd *retReg = IR::RegOpnd::New(TyMachReg, m_func);
    retReg->SetReg(RETURN_REG);
    Lowerer::InsertMove(retReg, retInstr->UnlinkSrc1(), retInstr);

    retInstr->SetSrc1(retReg);

    return retInstr;
}

///----------------------------------------------------------------------------
///
/// LowererMD::MDBranchOpcode
///
///     Map HIR branch opcode to machine-dependent equivalent.
///
///----------------------------------------------------------------------------

Js::OpCode
LowererMD::MDBranchOpcode(Js::OpCode opcode)
{
    switch (opcode)
    {
    case Js::OpCode::BrEq_A:
    case Js::OpCode::BrSrEq_A:
    case Js::OpCode::BrNotNeq_A:
    case Js::OpCode::BrSrNotNeq_A:
    case Js::OpCode::BrAddr_A:
        return Js::OpCode::BEQ;

    case Js::OpCode::BrNeq_A:
    case Js::OpCode::BrSrNeq_A:
    case Js::OpCode::BrNotEq_A:
    case Js::OpCode::BrSrNotEq_A:
    case Js::OpCode::BrNotAddr_A:
        return Js::OpCode::BNE;

    case Js::OpCode::BrLt_A:
    case Js::OpCode::BrNotGe_A:
        return Js::OpCode::BLT;

    case Js::OpCode::BrLe_A:
    case Js::OpCode::BrNotGt_A:
        return Js::OpCode::BLE;

    case Js::OpCode::BrGt_A:
    case Js::OpCode::BrNotLe_A:
        return Js::OpCode::BGT;

    case Js::OpCode::BrGe_A:
    case Js::OpCode::BrNotLt_A:
        return Js::OpCode::BGE;

    case Js::OpCode::BrUnGt_A:
        return Js::OpCode::BHI;

    case Js::OpCode::BrUnGe_A:
        return Js::OpCode::BCS;

    case Js::OpCode::BrUnLt_A:
        return Js::OpCode::BCC;

    case Js::OpCode::BrUnLe_A:
        return Js::OpCode::BLS;

    default:
        AssertMsg(0, "NYI");
        return opcode;
    }
}

Js::OpCode
LowererMD::MDUnsignedBranchOpcode(Js::OpCode opcode)
{
    switch (opcode)
    {
    case Js::OpCode::BrEq_A:
    case Js::OpCode::BrSrEq_A:
    case Js::OpCode::BrSrNotNeq_A:
    case Js::OpCode::BrNotNeq_A:
    case Js::OpCode::BrAddr_A:
        return Js::OpCode::BEQ;

    case Js::OpCode::BrNeq_A:
    case Js::OpCode::BrSrNeq_A:
    case Js::OpCode::BrSrNotEq_A:
    case Js::OpCode::BrNotEq_A:
    case Js::OpCode::BrNotAddr_A:
        return Js::OpCode::BNE;

    case Js::OpCode::BrLt_A:
    case Js::OpCode::BrNotGe_A:
        return Js::OpCode::BCC;

    case Js::OpCode::BrLe_A:
    case Js::OpCode::BrNotGt_A:
        return Js::OpCode::BLS;

    case Js::OpCode::BrGt_A:
    case Js::OpCode::BrNotLe_A:
        return Js::OpCode::BHI;

    case Js::OpCode::BrGe_A:
    case Js::OpCode::BrNotLt_A:
        return Js::OpCode::BCS;

    default:
        AssertMsg(0, "NYI");
        return opcode;
    }
}

Js::OpCode LowererMD::MDCompareWithZeroBranchOpcode(Js::OpCode opcode)
{
    Assert(opcode == Js::OpCode::BrLt_A || opcode == Js::OpCode::BrGe_A);
    return opcode == Js::OpCode::BrLt_A ? Js::OpCode::BMI : Js::OpCode::BPL;
}

void LowererMD::ChangeToAdd(IR::Instr *const instr, const bool needFlags)
{
    Assert(instr);
    Assert(instr->GetDst());
    Assert(instr->GetSrc1());
    Assert(instr->GetSrc2());

    if(instr->GetDst()->IsFloat64())
    {
        Assert(instr->GetSrc1()->IsFloat64());
        Assert(instr->GetSrc2()->IsFloat64());
        Assert(!needFlags);
        instr->m_opcode = Js::OpCode::FADD;
        return;
    }

    instr->m_opcode = needFlags ? Js::OpCode::ADDS : Js::OpCode::ADD;
}

void LowererMD::ChangeToSub(IR::Instr *const instr, const bool needFlags)
{
    Assert(instr);
    Assert(instr->GetDst());
    Assert(instr->GetSrc1());
    Assert(instr->GetSrc2());

    if(instr->GetDst()->IsFloat64())
    {
        Assert(instr->GetSrc1()->IsFloat64());
        Assert(instr->GetSrc2()->IsFloat64());
        Assert(!needFlags);
        instr->m_opcode = Js::OpCode::FSUB;
        return;
    }

    instr->m_opcode = needFlags ? Js::OpCode::SUBS : Js::OpCode::SUB;
}

void LowererMD::ChangeToShift(IR::Instr *const instr, const bool needFlags)
{
    Assert(instr);
    Assert(instr->GetDst());
    Assert(instr->GetSrc1());
    Assert(instr->GetSrc2());

    Func *const func = instr->m_func;

    switch(instr->m_opcode)
    {
        case Js::OpCode::Shl_A:
        case Js::OpCode::Shl_I4:
            Assert(!needFlags); // not implemented
            instr->m_opcode = Js::OpCode::LSL;
            break;

        case Js::OpCode::Shr_A:
        case Js::OpCode::Shr_I4:
            Assert(!needFlags); // not implemented
            instr->m_opcode = Js::OpCode::ASR;
            break;

        case Js::OpCode::ShrU_A:
        case Js::OpCode::ShrU_I4:
            Assert(!needFlags); // not implemented
            instr->m_opcode = Js::OpCode::LSR;
            break;

        default:
            Assert(false);
            __assume(false);
    }

    // Javascript requires the ShiftCount is masked to the bottom 5 bits.
    uint8 mask = TySize[instr->GetDst()->GetType()] == 8 ? 63 : 31;
    if (instr->GetSrc2()->IsIntConstOpnd())
    {
        // In the constant case, do the mask manually.
        IntConstType immed = instr->GetSrc2()->AsIntConstOpnd()->GetValue() & mask;
        if (immed == 0)
        {
            // Shift by zero is just a move, and the shift-right instructions
            // don't permit encoding of a zero shift amount.
            instr->m_opcode = Js::OpCode::MOV;
            instr->FreeSrc2();
        }
        else
        {
            instr->GetSrc2()->AsIntConstOpnd()->SetValue(immed);
        }
    }
    else
    {
        // In the variable case, generate code to do the mask.
        IR::Opnd *const src2 = instr->UnlinkSrc2();
        instr->SetSrc2(IR::RegOpnd::New(src2->GetType(), func));
        IR::Instr *const newInstr = IR::Instr::New(
            Js::OpCode::AND, instr->GetSrc2(), src2, IR::IntConstOpnd::New(mask, TyInt8, func), func);
        instr->InsertBefore(newInstr);
    }
}

const uint16
LowererMD::GetFormalParamOffset()
{
    //In ARM formal params are offset into the param area.
    //So we only count the non-user params (Function object & CallInfo and let the encoder account for the saved R11 and LR
    return 2;
}

///----------------------------------------------------------------------------
///
/// LowererMD::LowerCondBranch
///
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::LowerCondBranch(IR::Instr * instr)
{
    AssertMsg(instr->GetSrc1() != nullptr, "Expected src opnds on conditional branch");

    IR::Opnd *  opndSrc1 = instr->UnlinkSrc1();
    IR::Instr * instrPrev = nullptr;

    switch (instr->m_opcode)
    {
        case Js::OpCode::BrTrue_A:
        case Js::OpCode::BrOnNotEmpty:
        case Js::OpCode::BrNotNull_A:
        case Js::OpCode::BrOnObject_A:
        case Js::OpCode::BrOnObjectOrNull_A:
        case Js::OpCode::BrOnConstructor_A:
        case Js::OpCode::BrOnClassConstructor:
        case Js::OpCode::BrOnBaseConstructorKind:
            Assert(!opndSrc1->IsFloat64());
            AssertMsg(opndSrc1->IsRegOpnd(),"NYI for other operands");
            AssertMsg(instr->GetSrc2() == nullptr, "Expected 1 src on boolean branch");
            instrPrev = IR::Instr::New(Js::OpCode::CMP, this->m_func);
            instrPrev->SetSrc1(opndSrc1);
            instrPrev->SetSrc2(IR::IntConstOpnd::New(0, TyInt32, m_func));
            instr->InsertBefore(instrPrev);
            LegalizeMD::LegalizeInstr(instrPrev);

            instr->m_opcode = Js::OpCode::BNE;

            break;
        case Js::OpCode::BrFalse_A:
        case Js::OpCode::BrOnEmpty:
            Assert(!opndSrc1->IsFloat64());
            AssertMsg(opndSrc1->IsRegOpnd(),"NYI for other operands");
            AssertMsg(instr->GetSrc2() == nullptr, "Expected 1 src on boolean branch");
            instrPrev = IR::Instr::New(Js::OpCode::CMP, this->m_func);
            instrPrev->SetSrc1(opndSrc1);
            instrPrev->SetSrc2(IR::IntConstOpnd::New(0, TyInt32, m_func));
            instr->InsertBefore(instrPrev);
            LegalizeMD::LegalizeInstr(instrPrev);

            instr->m_opcode = Js::OpCode::BEQ;

            break;

        default:
            IR::Opnd *  opndSrc2 = instr->UnlinkSrc2();
            AssertMsg(opndSrc2 != nullptr, "Expected 2 src's on non-boolean branch");

            if (opndSrc1->IsFloat64())
            {
                AssertMsg(opndSrc1->IsRegOpnd(),"NYI for other operands");
                Assert(opndSrc2->IsFloat64());
                Assert(opndSrc2->IsRegOpnd() && opndSrc1->IsRegOpnd());
                //This comparison updates the FPSCR - floating point status control register
                instrPrev = IR::Instr::New(Js::OpCode::FCMP, this->m_func);
                instrPrev->SetSrc1(opndSrc1);
                instrPrev->SetSrc2(opndSrc2);
                instr->InsertBefore(instrPrev);
                LegalizeMD::LegalizeInstr(instrPrev);

                instr->m_opcode = LowererMD::MDBranchOpcode(instr->m_opcode);
            }
            else
            {
                AssertMsg(opndSrc2->IsRegOpnd() || opndSrc2->IsIntConstOpnd()  || (opndSrc2->IsAddrOpnd()), "NYI for other operands");

                instrPrev = IR::Instr::New(Js::OpCode::CMP, this->m_func);
                instrPrev->SetSrc1(opndSrc1);
                instrPrev->SetSrc2(opndSrc2);
                instr->InsertBefore(instrPrev);

                LegalizeMD::LegalizeInstr(instrPrev);

                instr->m_opcode = MDBranchOpcode(instr->m_opcode);
            }
            break;
    }
    return instr;
}

///----------------------------------------------------------------------------
///
/// LowererMD::ForceDstToReg
///
///----------------------------------------------------------------------------

IR::Instr*
LowererMD::ForceDstToReg(IR::Instr *instr)
{
    IR::Opnd * dst = instr->GetDst();

    if (dst->IsRegOpnd())
    {
        return instr;
    }

    IR::Instr * newInstr = instr->SinkDst(Js::OpCode::Ld_A);
    LowererMD::ChangeToAssign(newInstr);
    return newInstr;
}

IR::Instr *
LowererMD::LoadFunctionObjectOpnd(IR::Instr *instr, IR::Opnd *&functionObjOpnd)
{
    IR::Opnd * src1 = instr->GetSrc1();
    IR::Instr * instrPrev = instr->m_prev;
    if (src1 == nullptr)
    {
        IR::RegOpnd * regOpnd = IR::RegOpnd::New(TyMachPtr, m_func);
        //function object is first argument and mark it as IsParamSlotSym.
        StackSym *paramSym = GetImplicitParamSlotSym(0);
        IR::SymOpnd *paramOpnd = IR::SymOpnd::New(paramSym, TyMachPtr, m_func);

        instrPrev = Lowerer::InsertMove(regOpnd, paramOpnd, instr);
        functionObjOpnd = instrPrev->GetDst();
    }
    else
    {
        // Inlinee LdHomeObj, use the function object opnd on the instruction
        functionObjOpnd = instr->UnlinkSrc1();
        if (!functionObjOpnd->IsRegOpnd())
        {
            Assert(functionObjOpnd->IsAddrOpnd());
        }
    }

    return instrPrev;
}
bool
LowererMD::GenerateFastDivAndRem(IR::Instr *instrDiv, IR::LabelInstr* bailOutLabel)
{
    return false;
}
void
LowererMD::GenerateFastDivByPow2(IR::Instr *instrDiv)
{
    //// Given:
    //// dst = Div_A src1, src2
    //// where src2 == power of 2
    ////
    //// Generate:
    ////       (observation: positive q divides by p equally, where p = power of 2, if q's binary representation
    ////       has all zeroes to the right of p's power 2 bit, try to see if that is the case)
    //// s1 =  AND  src1, 0x80000001 | ((src2Value - 1) << 1)
    ////       CMP  s1, 1
    ////       BNE  $doesntDivideEqually
    //// s1  = ASR  src1, log2(src2Value)  -- do the equal divide
    //// dst = EOR   s1, 1                 -- restore tagged int bit
    ////       B  $done
    //// $doesntDivideEqually:
    ////       (now check if it divides with the remainder of 1, for which we can do integer divide and accommodate with +0.5
    ////       note that we need only the part that is to the left of p's power 2 bit)
    //// s1 =  AND  s1, 0x80000001 | (src2Value - 1)
    ////       CMP  s1, 1
    ////       BNE  $helper
    //// s1 =  ASR  src1, log2(src2Value) + 1 -- do the integer divide and also shift out the tagged int bit
    ////       PUSH 0xXXXXXXXX (ScriptContext)
    ////       PUSH s1
    //// dst = CALL Op_FinishOddDivByPow2     -- input: actual value, scriptContext; output: JavascriptNumber with 0.5 added to the input
    ////       JMP $done
    //// $helper:
    ////     ...
    //// $done:

    //if (instrDiv->GetSrc1()->IsRegOpnd() && instrDiv->GetSrc1()->AsRegOpnd()->m_sym->m_isNotInt)
    //{
    //    return;
    //}

    //IR::Opnd       *dst    = instrDiv->GetDst();
    //IR::Opnd       *src1   = instrDiv->GetSrc1();
    //IR::AddrOpnd   *src2   = instrDiv->GetSrc2()->IsAddrOpnd() ? instrDiv->GetSrc2()->AsAddrOpnd() : nullptr;
    //IR::LabelInstr *doesntDivideEqually = IR::LabelInstr::New(Js::OpCode::Label, m_func);
    //IR::LabelInstr *helper = IR::LabelInstr::New(Js::OpCode::Label, m_func, true);
    //IR::LabelInstr *done   = IR::LabelInstr::New(Js::OpCode::Label, m_func);
    //IR::RegOpnd    *s1     = IR::RegOpnd::New(TyVar, m_func);
    //IR::Instr      *instr;

    //Assert(src2 && src2->IsVar() && Js::TaggedInt::Is(src2->m_address) && (Math::IsPow2(Js::TaggedInt::ToInt32(src2->m_address))));
    //int32          src2Value = Js::TaggedInt::ToInt32(src2->m_address);

    //// s1 =  AND  src1, 0x80000001 | ((src2Value - 1) << 1)
    //instr = IR::Instr::New(Js::OpCode::AND, s1, src1, IR::IntConstOpnd::New((0x80000001 | ((src2Value - 1) << 1)), TyInt32, m_func), m_func);
    //instrDiv->InsertBefore(instr);
    //LegalizeMD::LegalizeInstr(instr);

    ////       CMP s1, 1
    //instr = IR::Instr::New(Js::OpCode::CMP, m_func);
    //instr->SetSrc1(s1);
    //instr->SetSrc2(IR::IntConstOpnd::New(1, TyInt32, m_func));
    //instrDiv->InsertBefore(instr);

    ////       BNE  $doesntDivideEqually
    //instr = IR::BranchInstr::New(Js::OpCode::BNE, doesntDivideEqually, m_func);
    //instrDiv->InsertBefore(instr);

    //// s1  = ASR  src1, log2(src2Value)  -- do the equal divide
    //instr = IR::Instr::New(Js::OpCode::ASR, s1, src1, IR::IntConstOpnd::New(Math::Log2(src2Value), TyInt32, m_func), m_func);
    //instrDiv->InsertBefore(instr);
    //LegalizeMD::LegalizeInstr(instr);

    //// dst = ORR   s1, 1                 -- restore tagged int bit
    //instr = IR::Instr::New(Js::OpCode::ORR, dst, s1, IR::IntConstOpnd::New(1, TyInt32, m_func), m_func);
    //instrDiv->InsertBefore(instr);
    //LegalizeMD::LegalizeInstr(instr);
    //
    ////       B  $done
    //instr = IR::BranchInstr::New(Js::OpCode::B, done, m_func);
    //instrDiv->InsertBefore(instr);

    //// $doesntDivideEqually:
    //instrDiv->InsertBefore(doesntDivideEqually);

    //// s1 =  AND  s1, 0x80000001 | (src2Value - 1)
    //instr = IR::Instr::New(Js::OpCode::AND, s1, s1, IR::IntConstOpnd::New((0x80000001 | (src2Value - 1)), TyInt32, m_func), m_func);
    //instrDiv->InsertBefore(instr);

    ////       CMP  s1, 1
    //instr = IR::Instr::New(Js::OpCode::CMP, m_func);
    //instr->SetSrc1(s1);
    //instr->SetSrc2(IR::IntConstOpnd::New(1, TyInt32, m_func));
    //instrDiv->InsertBefore(instr);

    ////       BNE  $helper
    //instrDiv->InsertBefore(IR::BranchInstr::New(Js::OpCode::BNE, helper, m_func));

    //// s1 =  ASR  src1, log2(src2Value) + 1 -- do the integer divide and also shift out the tagged int bit
    //instr = IR::Instr::New(Js::OpCode::ASR, s1, src1, IR::IntConstOpnd::New(Math::Log2(src2Value) + 1, TyInt32, m_func), m_func);
    //instrDiv->InsertBefore(instr);
    //LegalizeMD::LegalizeInstr(instr);

    //// Arg2: scriptContext
    //IR::JnHelperMethod helperMethod;
    //if (instrDiv->dstIsTempNumber)
    //{
    //    // Var JavascriptMath::FinishOddDivByPow2_InPlace(uint32 value, ScriptContext *scriptContext, _Out_ JavascriptNumber* result)
    //    helperMethod = IR::HelperOp_FinishOddDivByPow2InPlace;
    //    Assert(dst->IsRegOpnd());
    //    StackSym * tempNumberSym = this->m_lowerer->GetTempNumberSym(dst, instr->dstIsTempNumberTransferred);

    //    instr = this->m_lowerer->InsertLoadStackAddress(tempNumberSym, instrDiv);
    //    LegalizeMD::LegalizeInstr(instr);

    //    this->LoadHelperArgument(instrDiv, instr->GetDst());
    //}
    //else
    //{
    //    // Var JavascriptMath::FinishOddDivByPow2(uint32 value, ScriptContext *scriptContext)
    //    helperMethod = IR::HelperOp_FinishOddDivByPow2;
    //}
    //this->m_lowerer->LoadScriptContext(instrDiv);

    //// Arg1: value
    //this->LoadHelperArgument(instrDiv, s1);

    //// dst = CALL Op_FinishOddDivByPow2     -- input: actual value, output: JavascriptNumber with 0.5 added to the input
    //instr = IR::Instr::New(Js::OpCode::Call, dst, IR::HelperCallOpnd::New(helperMethod, m_func), m_func);
    //instrDiv->InsertBefore(instr);
    //this->LowerCall(instr, 0);

    ////       JMP $done
    //instrDiv->InsertBefore(IR::BranchInstr::New(Js::OpCode::B, done, m_func));

    //// $helper:
    //instrDiv->InsertBefore(helper);

    //// $done:
    //instrDiv->InsertAfter(done);

    return;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastCmSrEqConst
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastCmSrXxConst(IR::Instr *instr)
{
    //
    // Given:
    // s1 = CmSrXX_A s2, s3
    // where either s2 or s3 is 'null', 'true' or 'false'
    //
    // Generate:
    //
    //     CMP s2, s3
    //     JEQ $mov_res
    //     MOV s1, eq ? Library.GetFalse() : Library.GetTrue()
    //     JMP $done
    // $mov_res:
    //     MOV s1, eq ? Library.GetTrue() : Library.GetFalse()
    // $done:
    //

    Assert(m_lowerer->IsConstRegOpnd(instr->GetSrc2()->AsRegOpnd()));
    return false;
}

void LowererMD::GenerateFastCmXxI4(IR::Instr *instr)
{
    this->GenerateFastCmXx(instr);
}

void LowererMD::GenerateFastCmXxR8(IR::Instr *instr)
{
    this->GenerateFastCmXx(instr);
}

void LowererMD::GenerateFastCmXx(IR::Instr *instr)
{
    // For float src:
    // LDIMM dst, trueResult
    // FCMP src1, src2
    // - BVS $done (NaN check iff B.cond is BNE)
    // B.cond $done
    // LDIMM dst, falseResult
    // $done

    // For Int src:
    // LDIMM dst, trueResult
    // CMP src1, src2
    // B.cond $done
    // LDIMM dst, falseResult
    // $done:

    IR::Opnd * src1 = instr->UnlinkSrc1();
    IR::Opnd * src2 = instr->UnlinkSrc2();
    IR::Opnd * dst = instr->UnlinkDst();
    bool isIntDst = dst->AsRegOpnd()->m_sym->IsInt32();
    bool isFloatSrc = src1->IsFloat();
    Assert(!isFloatSrc || src2->IsFloat());
    Assert(!src1->IsInt64() || src2->IsInt64());
    Assert(!isFloatSrc || AutoSystemInfo::Data.SSE2Available());
    Assert(src1->IsRegOpnd());
    IR::Opnd * opndTrue;
    IR::Opnd * opndFalse;
    IR::Instr * newInstr;
    IR::LabelInstr * done = IR::LabelInstr::New(Js::OpCode::Label, m_func);

    if (dst->IsEqual(src1))
    {
        IR::RegOpnd *newSrc1 = IR::RegOpnd::New(src1->GetType(), m_func);
        Lowerer::InsertMove(newSrc1, src1, instr);
        src1 = newSrc1;
    }

    if (dst->IsEqual(src2))
    {
        IR::RegOpnd *newSrc2 = IR::RegOpnd::New(src1->GetType(), m_func);
        Lowerer::InsertMove(newSrc2, src2, instr);
        src2 = newSrc2;
    }

    if (isIntDst)
    {
        opndTrue = IR::IntConstOpnd::New(1, TyInt32, this->m_func);
        opndFalse = IR::IntConstOpnd::New(0, TyInt32, this->m_func);
    }
    else
    {
        opndTrue = this->m_lowerer->LoadLibraryValueOpnd(instr, LibraryValue::ValueTrue);
        opndFalse = this->m_lowerer->LoadLibraryValueOpnd(instr, LibraryValue::ValueFalse);
    }

    Lowerer::InsertMove(dst, opndTrue, instr);

    // CMP src1, src2
    newInstr = IR::Instr::New(isFloatSrc ? Js::OpCode::FCMP : Js::OpCode::CMP, this->m_func);
    newInstr->SetSrc1(src1);
    newInstr->SetSrc2(src2);
    instr->InsertBefore(newInstr);
    LowererMD::Legalize(newInstr);

    bool addNaNCheck = false;
    Js::OpCode opcode = Js::OpCode::InvalidOpCode;

    switch (instr->m_opcode)
    {
        case Js::OpCode::CmEq_A:
        case Js::OpCode::CmSrEq_A:
        case Js::OpCode::CmEq_I4:
            opcode = Js::OpCode::BEQ;
            break;

        case Js::OpCode::CmNeq_A:
        case Js::OpCode::CmSrNeq_A:
        case Js::OpCode::CmNeq_I4:
            opcode = Js::OpCode::BNE;
            addNaNCheck = isFloatSrc;
            break;

        case Js::OpCode::CmGt_A:
        case Js::OpCode::CmGt_I4:
            opcode = Js::OpCode::BGT;
            break;

        case Js::OpCode::CmGe_A:
        case Js::OpCode::CmGe_I4:
            opcode = Js::OpCode::BGE;
            break;

        case Js::OpCode::CmLt_A:
        case Js::OpCode::CmLt_I4:
            //Can't use BLT  as is set when the operands are unordered (NaN).
            opcode = isFloatSrc ? Js::OpCode::BCC : Js::OpCode::BLT;
            break;

        case Js::OpCode::CmLe_A:
        case Js::OpCode::CmLe_I4:
            //Can't use BLE as it is set when the operands are unordered (NaN).
            opcode = isFloatSrc ? Js::OpCode::BLS : Js::OpCode::BLE;
            break;

        case Js::OpCode::CmUnGt_A:
        case Js::OpCode::CmUnGt_I4:
            opcode = Js::OpCode::BHI;
            break;

        case Js::OpCode::CmUnGe_A:
        case Js::OpCode::CmUnGe_I4:
            opcode = Js::OpCode::BCS;
            break;

        case Js::OpCode::CmUnLt_A:
        case Js::OpCode::CmUnLt_I4:
            opcode = Js::OpCode::BCC;
            break;

        case Js::OpCode::CmUnLe_A:
        case Js::OpCode::CmUnLe_I4:
            opcode = Js::OpCode::BLS;
            break;

        default: Assert(false);
    }

    if (addNaNCheck)
    {
        newInstr = IR::BranchInstr::New(Js::OpCode::BVS, done, m_func);
        instr->InsertBefore(newInstr);
    }

    newInstr = IR::BranchInstr::New(opcode, done, m_func);
    instr->InsertBefore(newInstr);

    Lowerer::InsertMove(dst, opndFalse, instr);
    instr->InsertBefore(done);
    instr->Remove();
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastCmXxTaggedInt
///
///----------------------------------------------------------------------------
bool LowererMD::GenerateFastCmXxTaggedInt(IR::Instr *instr, bool isInHelper  /* = false */)
{
    // The idea is to do an inline compare if we can prove that both sources
    // are tagged ints (i.e., are vars with the low bit set).
    //
    // Given:
    //
    //      Cmxx_A dst, src1, src2
    //
    // Generate:
    //
    // (If not Int31's, goto $helper)
    //      LDIMM dst, trueResult
    //      CMP src1, src2
    //      BEQ $fallthru
    //      LDIMM dst, falseResult
    //      B $fallthru
    // $helper:
    //      (caller will generate normal helper call sequence)
    // $fallthru:
    IR::Opnd * src1 = instr->GetSrc1();
    IR::Opnd * src2 = instr->GetSrc2();
    IR::Opnd * dst = instr->GetDst();
    IR::LabelInstr * helper = IR::LabelInstr::New(Js::OpCode::Label, m_func, true);
    IR::LabelInstr * fallthru = IR::LabelInstr::New(Js::OpCode::Label, m_func, isInHelper);

    Assert(src1 && src2 && dst);

    // Not tagged ints?
    if (src1->IsRegOpnd() && src1->AsRegOpnd()->m_sym->m_isNotNumber)
    {
        return false;
    }
    if (src2->IsRegOpnd() && src2->AsRegOpnd()->m_sym->m_isNotNumber)
    {
        return false;
    }

    Js::OpCode opcode = Js::OpCode::InvalidOpCode;
    switch ( instr->m_opcode)
    {
        case Js::OpCode::CmEq_A:
        case Js::OpCode::CmSrEq_A:
        case Js::OpCode::CmEq_I4:
            opcode = Js::OpCode::BEQ;
            break;

        case Js::OpCode::CmNeq_A:
        case Js::OpCode::CmSrNeq_A:
        case Js::OpCode::CmNeq_I4:
            opcode = Js::OpCode::BNE;
            break;

        case Js::OpCode::CmGt_A:
        case Js::OpCode::CmGt_I4:
            opcode = Js::OpCode::BGT;
            break;

        case Js::OpCode::CmGe_A:
        case Js::OpCode::CmGe_I4:
            opcode = Js::OpCode::BGE;
            break;

        case Js::OpCode::CmLt_A:
        case Js::OpCode::CmLt_I4:
            opcode = Js::OpCode::BLT;
            break;

        case Js::OpCode::CmLe_A:
        case Js::OpCode::CmLe_I4:
            opcode = Js::OpCode::BLE;
            break;

        case Js::OpCode::CmUnGt_A:
        case Js::OpCode::CmUnGt_I4:
            opcode = Js::OpCode::BHI;
            break;

        case Js::OpCode::CmUnGe_A:
        case Js::OpCode::CmUnGe_I4:
            opcode = Js::OpCode::BCS;
            break;

        case Js::OpCode::CmUnLt_A:
        case Js::OpCode::CmUnLt_I4:
            opcode = Js::OpCode::BCC;
            break;

        case Js::OpCode::CmUnLe_A:
        case Js::OpCode::CmUnLe_I4:
            opcode = Js::OpCode::BLS;
            break;

        default: Assert(false);
    }

    // Tagged ints?
    bool isTaggedInts = false;
    if (src1->IsTaggedInt() || src1->IsInt32())
    {
        if (src2->IsTaggedInt() || src2->IsInt32())
        {
            isTaggedInts = true;
        }
    }

    if (!isTaggedInts)
    {
        this->GenerateSmIntPairTest(instr, src1, src2, helper);
    }

    if (dst->IsEqual(src1))
    {
        IR::RegOpnd *newSrc1 = IR::RegOpnd::New(TyMachReg, m_func);
        Lowerer::InsertMove(newSrc1, src1, instr);
        src1 = newSrc1;
    }

    if (dst->IsEqual(src2))
    {
        IR::RegOpnd *newSrc2 = IR::RegOpnd::New(TyMachReg, m_func);
        Lowerer::InsertMove(newSrc2, src2, instr);
        src2 = newSrc2;
    }

    IR::Opnd *opndTrue, *opndFalse;

    if (dst->IsInt32())
    {
        opndTrue = IR::IntConstOpnd::New(1, TyMachReg, this->m_func);
        opndFalse = IR::IntConstOpnd::New(0, TyMachReg, this->m_func);
    }
    else
    {
        opndTrue = m_lowerer->LoadLibraryValueOpnd(instr, LibraryValue::ValueTrue);
        opndFalse = m_lowerer->LoadLibraryValueOpnd(instr, LibraryValue::ValueFalse);
    }

    //      LDIMM dst, trueResult
    //      CMP src1, src2
    //      BEQ $fallthru
    //      LDIMM dst, falseResult
    //      B $fallthru

    src1 = src1->UseWithNewType(TyInt32, m_func);
    src2 = src2->UseWithNewType(TyInt32, m_func);

    instr->InsertBefore(IR::Instr::New(Js::OpCode::LDIMM, dst, opndTrue, m_func));
    IR::Instr *instrCmp = IR::Instr::New(Js::OpCode::CMP, m_func);
    instrCmp->SetSrc1(src1);
    instrCmp->SetSrc2(src2);
    instr->InsertBefore(instrCmp);
    LegalizeMD::LegalizeInstr(instrCmp);

    instr->InsertBefore(IR::BranchInstr::New(opcode, fallthru, m_func));
    instr->InsertBefore(IR::Instr::New(Js::OpCode::LDIMM, dst, opndFalse, m_func));

    if (isTaggedInts)
    {
        instr->InsertAfter(fallthru);
        instr->Remove();
        return true;
    }

    // B $fallthru
    instr->InsertBefore(IR::BranchInstr::New(Js::OpCode::B, fallthru, m_func));

    instr->InsertBefore(helper);
    instr->InsertAfter(fallthru);
    return false;
}

IR::Instr * LowererMD::GenerateConvBool(IR::Instr *instr)
{
    // dst = LDIMM true
    // TST src1, src2
    // BNE fallthrough
    // dst = LDIMM false
    // fallthrough:

    IR::RegOpnd *dst = instr->GetDst()->AsRegOpnd();
    IR::RegOpnd *src1 = instr->GetSrc1()->AsRegOpnd();
    IR::Opnd *opndTrue = m_lowerer->LoadLibraryValueOpnd(instr, LibraryValue::ValueTrue);
    IR::Opnd *opndFalse = m_lowerer->LoadLibraryValueOpnd(instr, LibraryValue::ValueFalse);
    IR::LabelInstr *fallthru = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);

    // dst = LDIMM true
    IR::Instr *instrFirst = IR::Instr::New(Js::OpCode::LDIMM, dst, opndTrue, m_func);
    instr->InsertBefore(instrFirst);

    // TST src1, src2
    IR::Instr *instrTst = IR::Instr::New(Js::OpCode::TST, m_func);
    instrTst->SetSrc1(src1);
    instrTst->SetSrc2(src1);
    instr->InsertBefore(instrTst);
    LegalizeMD::LegalizeInstr(instrTst);

    // BNE fallthrough
    instr->InsertBefore(IR::BranchInstr::New(Js::OpCode::BNE, fallthru, m_func));

    // dst = LDIMM false
    instr->InsertBefore(IR::Instr::New(Js::OpCode::LDIMM, dst, opndFalse, m_func));

    // fallthrough:
    instr->InsertAfter(fallthru);
    instr->Remove();

    return instrFirst;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastAdd
///
/// NOTE: We assume that only the sum of two Int31's will have 0x2 set. This
/// is only true until we have a var type with tag == 0x2.
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastAdd(IR::Instr * instrAdd)
{
    // Given:
    //
    // dst = Add src1, src2
    //
    // Generate:
    //
    // (If not 2 Int31's, jump to $helper.)
    // s1 = MOV src1
    // s1 = ADDS s1, src2   -- try an inline add
    //      BVS $helper     -- bail if the add overflowed
    // s1 = ORR s1, AtomTag_IntPtr
    // dst = MOV s1
    //      B $fallthru
    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    IR::Instr *      instr;
    IR::LabelInstr * labelHelper;
    IR::LabelInstr * labelFallThru;
    IR::Opnd *       opndReg;
    IR::Opnd *       opndSrc1;
    IR::Opnd *       opndSrc2;

    opndSrc1 = instrAdd->GetSrc1();
    opndSrc2 = instrAdd->GetSrc2();
    AssertMsg(opndSrc1 && opndSrc2, "Expected 2 src opnd's on Add instruction");

    // Generate fastpath for Incr_A anyway -
    // Incrementing strings representing integers can be inter-mixed with integers e.g. "1"++ -> converts 1 to an int and thereafter, integer increment is expected.
    if (opndSrc1->IsRegOpnd() && (opndSrc1->AsRegOpnd()->IsNotInt() || opndSrc1->GetValueType().IsString()
        || (instrAdd->m_opcode != Js::OpCode::Incr_A && opndSrc1->GetValueType().IsLikelyString())))
    {
        return false;
    }

    if (opndSrc2->IsRegOpnd() && (opndSrc2->AsRegOpnd()->IsNotInt() ||
        opndSrc2->GetValueType().IsLikelyString()))
    {
        return false;
    }

    // Tagged ints?
    bool isTaggedInts = false;
    if (opndSrc1->IsTaggedInt())
    {
        if (opndSrc2->IsTaggedInt())
        {
            isTaggedInts = true;
        }
    }

    labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);

    if (!isTaggedInts)
    {
        // (If not 2 Int31's, jump to $helper.)

        this->GenerateSmIntPairTest(instrAdd, opndSrc1, opndSrc2, labelHelper);
    }

    if (opndSrc1->IsImmediateOpnd())
    {
        // If opnd1 is a constant, just swap them.
        IR::Opnd *opndTmp = opndSrc1;
        opndSrc1 = opndSrc2;
        opndSrc2 = opndTmp;
    }

    //
    // For 32 bit arithmetic we copy them and set the size of operands to be 32 bits. This is
    // relevant only on ARM64.
    //

    opndSrc1 = opndSrc1->UseWithNewType(TyInt32, this->m_func);
    opndSrc2 = opndSrc2->UseWithNewType(TyInt32, this->m_func);

    // s1 = MOV src1

    opndReg = IR::RegOpnd::New(TyInt32, this->m_func);
    Lowerer::InsertMove(opndReg, opndSrc1, instrAdd);
    
    // s1 = ADDS s1, src2

    instr = IR::Instr::New(Js::OpCode::ADDS, opndReg, opndReg, opndSrc2, this->m_func);
    instrAdd->InsertBefore(instr);
    Legalize(instr);

    //      BVS $helper

    instr = IR::BranchInstr::New(Js::OpCode::BVS, labelHelper, this->m_func);
    instrAdd->InsertBefore(instr);

    //
    // Convert TyInt32 operand, back to TyMachPtr type.
    //

    if(TyMachReg != opndReg->GetType())
    {
        opndReg = opndReg->UseWithNewType(TyMachPtr, this->m_func);
    }

    // s1 = ORR s1, AtomTag_IntPtr
    GenerateInt32ToVarConversion(opndReg, instrAdd);
    
    // dst = MOV s1

    instr = IR::Instr::New(Js::OpCode::MOV, instrAdd->GetDst(), opndReg, this->m_func);
    instrAdd->InsertBefore(instr);

    //      B $fallthru

    labelFallThru = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    instr = IR::BranchInstr::New(Js::OpCode::B, labelFallThru, this->m_func);
    instrAdd->InsertBefore(instr);

    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    instrAdd->InsertBefore(labelHelper);
    instrAdd->InsertAfter(labelFallThru);

    return true;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastSub
///
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastSub(IR::Instr * instrSub)
{
    // Given:
    //
    // dst = Sub src1, src2
    //
    // Generate:
    //
    // (If not 2 Int31's, jump to $helper.)
    // s1 = MOV src1
    // s1 = SUBS s1, src2   -- try an inline sub
    //      BVS $helper     -- bail if the subtract overflowed
    //      BNE $helper
    // s1 = ORR s1, AtomTag_IntPtr
    // dst = MOV s1
    //      B $fallthru
    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    IR::Instr *      instr;
    IR::LabelInstr * labelHelper;
    IR::LabelInstr * labelFallThru;
    IR::Opnd *       opndReg;
    IR::Opnd *       opndSrc1;
    IR::Opnd *       opndSrc2;

    opndSrc1        =   instrSub->GetSrc1();
    opndSrc2        =   instrSub->GetSrc2();
    AssertMsg(opndSrc1 && opndSrc2, "Expected 2 src opnd's on Sub instruction");

    // Not tagged ints?
    if (opndSrc1->IsRegOpnd() && opndSrc1->AsRegOpnd()->IsNotInt())
    {
        return false;
    }
    if (opndSrc2->IsRegOpnd() && opndSrc2->AsRegOpnd()->IsNotInt())
    {
        return false;
    }

    // Tagged ints?
    bool isTaggedInts = false;
    if (opndSrc1->IsTaggedInt())
    {
        if (opndSrc2->IsTaggedInt())
        {
            isTaggedInts = true;
        }
    }

    labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);

    if (!isTaggedInts)
    {
        // (If not 2 Int31's, jump to $helper.)

        this->GenerateSmIntPairTest(instrSub, opndSrc1, opndSrc2, labelHelper);
    }

    //
    // For 32 bit arithmetic we copy them and set the size of operands to be 32 bits. This is
    // relevant only on ARM64.
    //

    opndSrc1    = opndSrc1->UseWithNewType(TyInt32, this->m_func);
    opndSrc2    = opndSrc2->UseWithNewType(TyInt32, this->m_func);

    // s1 = MOV src1

    opndReg = IR::RegOpnd::New(TyInt32, this->m_func);
    Lowerer::InsertMove(opndReg, opndSrc1, instrSub);

    // s1 = SUBS s1, src2

    instr = IR::Instr::New(Js::OpCode::SUBS, opndReg, opndReg, opndSrc2, this->m_func);
    instrSub->InsertBefore(instr);
    Legalize(instr);

    //      BVS $helper

    instr = IR::BranchInstr::New(Js::OpCode::BVS, labelHelper, this->m_func);
    instrSub->InsertBefore(instr);

    //
    // Convert TyInt32 operand, back to TyMachPtr type.
    //

    if(TyMachReg != opndReg->GetType())
    {
        opndReg = opndReg->UseWithNewType(TyMachPtr, this->m_func);
    }

    // s1 = ORR s1, AtomTag_IntPtr
    GenerateInt32ToVarConversion(opndReg, instrSub);

    // dst = MOV s1

    instr = IR::Instr::New(Js::OpCode::MOV, instrSub->GetDst(), opndReg, this->m_func);
    instrSub->InsertBefore(instr);

    //      B $fallthru

    labelFallThru = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    instr = IR::BranchInstr::New(Js::OpCode::B, labelFallThru, this->m_func);
    instrSub->InsertBefore(instr);

    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    instrSub->InsertBefore(labelHelper);
    instrSub->InsertAfter(labelFallThru);

    return true;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastMul
///
///----------------------------------------------------------------------------
bool
LowererMD::GenerateFastMul(IR::Instr * instrMul)
{
    // Given:
    //
    // dst = Mul src1, src2
    //
    // Generate:
    //
    // (If not 2 Int31's, jump to $helper.)
    // s1 = MOV src1
    // s2 = MOV src2
    // s3 = SMULL s1, s2    -- do the signed mul
    //      CMP s3, s3 SXTW
    //      BNE $helper     -- bail if the result overflowed
    //      CBZ s3, $zero   -- Check result is 0. might be -0. Result is -0 when a negative number is multiplied with 0.
    //      B $nonzero
    // $zero:               -- result of mul was 0. try to check for -0
    // s2 = ADDS s2, src1    --check for same sign
    //      BGE $nonzero     - positive 0 if signs are equal
    // dst = ToVar(-0.0)    -- load negative 0
    //      B $fallthru
    // $nonzero:
    // s3 = ORR s3, AtomTag_IntPtr
    // dst= MOV s3
    //      B $fallthru
    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    IR::LabelInstr * labelHelper;
    IR::LabelInstr * labelFallThru;
    IR::LabelInstr * labelNonZero;
    IR::Instr *      instr;
    IR::RegOpnd *    opndReg1;
    IR::RegOpnd *    opndReg2;
    IR::RegOpnd *    s3;
    IR::Opnd *       opndSrc1;
    IR::Opnd *       opndSrc2;

    opndSrc1 = instrMul->GetSrc1();
    opndSrc2 = instrMul->GetSrc2();
    AssertMsg(opndSrc1 && opndSrc2, "Expected 2 src opnd's on mul instruction");

    if (opndSrc1->IsRegOpnd() && opndSrc1->AsRegOpnd()->IsNotInt())
    {
        return true;
    }
    if (opndSrc2->IsRegOpnd() && opndSrc2->AsRegOpnd()->IsNotInt())
    {
        return true;
    }
    // (If not 2 Int31's, jump to $helper.)

    labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
    labelNonZero = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    labelFallThru = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);

    this->GenerateSmIntPairTest(instrMul, opndSrc1, opndSrc2, labelHelper);

    //
    // For 32 bit arithmetic we copy them and set the size of operands to be 32 bits. This is
    // relevant only on ARM64.
    //

    opndSrc1    = opndSrc1->UseWithNewType(TyInt32, this->m_func);
    opndSrc2    = opndSrc2->UseWithNewType(TyInt32, this->m_func);

    if (opndSrc1->IsImmediateOpnd())
    {
        IR::Opnd * temp = opndSrc1;
        opndSrc1 = opndSrc2;
        opndSrc2 = temp;
    }

    // s1 = MOV src1

    opndReg1 = IR::RegOpnd::New(TyInt32, this->m_func);
    Lowerer::InsertMove(opndReg1, opndSrc1, instrMul);

    // s2 = MOV src2

    opndReg2 = IR::RegOpnd::New(TyInt32, this->m_func);
    Lowerer::InsertMove(opndReg2, opndSrc2, instrMul);

    // s3 = SMULL s1, s2

    s3 = IR::RegOpnd::New(TyInt64, this->m_func);
    instr = IR::Instr::New(Js::OpCode::SMULL, s3, opndReg1, opndReg2, this->m_func);
    instrMul->InsertBefore(instr);

    //      CMP s3, s3 SXTW s3

    instr = IR::Instr::New(Js::OpCode::CMP_SXTW, this->m_func);
    instr->SetSrc1(s3);
    instr->SetSrc2(s3);
    instrMul->InsertBefore(instr);

    //      BNE $helper

    instr = IR::BranchInstr::New(Js::OpCode::BNE, labelHelper, this->m_func);
    instrMul->InsertBefore(instr);

    //      CBZ s3, $zero   -- Check result is 0. might be -0. Result is -0 when a negative number is multiplied with 0.

    IR::LabelInstr *labelZero = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
    instr = IR::BranchInstr::New(Js::OpCode::CBZ, labelZero, this->m_func);
    instr->SetSrc1(s3);
    instrMul->InsertBefore(instr);

    //      B $nonzero

    instr = IR::BranchInstr::New(Js::OpCode::B, labelNonZero, this->m_func);
    instrMul->InsertBefore(instr);

    // $zero:

    instrMul->InsertBefore(labelZero);

    // s2 = ADDS s2, s1

    instr = IR::Instr::New(Js::OpCode::ADDS, opndReg2, opndReg2, opndReg1, this->m_func);
    instrMul->InsertBefore(instr);
    Legalize(instr);

    //      BGE $nonzero
    instr = IR::BranchInstr::New(Js::OpCode::BGE, labelNonZero, this->m_func);
    instrMul->InsertBefore(instr);

    // dst = ToVar(-0.0)    -- load negative 0

    instr = IR::Instr::New(Js::OpCode::LDIMM, instrMul->GetDst(), m_lowerer->LoadLibraryValueOpnd(instrMul, LibraryValue::ValueNegativeZero), this->m_func);
    instrMul->InsertBefore(instr);

    //      B $fallthru

    instr = IR::BranchInstr::New(Js::OpCode::B, labelFallThru, this->m_func);
    instrMul->InsertBefore(instr);

    // $nonzero:

    instrMul->InsertBefore(labelNonZero);

    // dst = MOV_TRUNC s3

    instr = IR::Instr::New(Js::OpCode::MOV_TRUNC, instrMul->GetDst()->UseWithNewType(TyInt32,this->m_func), s3->UseWithNewType(TyInt32, this->m_func), this->m_func);
    instrMul->InsertBefore(instr);

    // dst = OR dst, AtomTag_IntPtr

    GenerateInt32ToVarConversion(instrMul->GetDst(), instrMul);


    //      B $fallthru

    instr = IR::BranchInstr::New(Js::OpCode::B, labelFallThru, this->m_func);
    instrMul->InsertBefore(instr);

    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    instrMul->InsertBefore(labelHelper);
    instrMul->InsertAfter(labelFallThru);

    return true;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastAnd
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastAnd(IR::Instr * instrAnd)
{
    // Left empty to match AMD64; assuming this is not performance critical
    return true;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastOr
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastOr(IR::Instr * instrOr)
{
    // Left empty to match AMD64; assuming this is not performance critical
    return true;
}


///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastXor
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastXor(IR::Instr * instrXor)
{
    // Left empty to match AMD64; assuming this is not performance critical
    return true;
}

//----------------------------------------------------------------------------
//
// LowererMD::GenerateFastNot
//
//----------------------------------------------------------------------------

bool
LowererMD::GenerateFastNot(IR::Instr * instrNot)
{
    // Left empty to match AMD64; assuming this is not performance critical
    return true;
}

//
// If value is zero in tagged int representation, jump to $labelHelper.
//
void
LowererMD::GenerateTaggedZeroTest( IR::Opnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr * labelHelper )
{
    // Cast the var to 32 bit integer.
    if(opndSrc->GetSize() != 4)
    {
        opndSrc = opndSrc->UseWithNewType(TyUint32, this->m_func);
    }
    AssertMsg(TySize[opndSrc->GetType()] == 4, "This technique works only on the 32-bit version");

    if(labelHelper != nullptr)
    {
        // CBZ src1, $labelHelper
        IR::Instr* instr = IR::BranchInstr::New(Js::OpCode::CBZ, labelHelper, this->m_func);
        instr->SetSrc1(opndSrc);
        insertInstr->InsertBefore(instr);
    }
    else
    {
        // TST src1, src1
        IR::Instr* instr = IR::Instr::New(Js::OpCode::TST, this->m_func);
        instr->SetSrc1(opndSrc);
        instr->SetSrc2(opndSrc);
        insertInstr->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);
    }
}

bool
LowererMD::GenerateFastNeg(IR::Instr * instrNeg)
{
    // Given:
    //
    // dst = Not src
    //
    // Generate:
    //
    //       if not int, jump $helper
    //       if src == 0     -- test for zero (must be handled by the runtime to preserve
    //       BEQ $helper     -- Difference between +0 and -0)
    // dst = SUB dst, 0, src -- do an inline NEG
    //       BVS $helper     -- bail if the subtract overflowed
    // dst = OR dst, tag     -- restore the var tag on the result
    //       BVS $helper
    //       B $fallthru
    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    IR::Instr *      instr;
    IR::LabelInstr * labelHelper = nullptr;
    IR::LabelInstr * labelFallThru = nullptr;
    IR::Opnd *       opndSrc1;
    IR::Opnd *       opndDst;
    bool usingNewDst = false;
    opndSrc1 = instrNeg->GetSrc1();
    AssertMsg(opndSrc1, "Expected src opnd on Neg instruction");


    if (opndSrc1->IsRegOpnd() && opndSrc1->AsRegOpnd()->m_sym->IsIntConst())
    {
        IR::Opnd *newOpnd;
        IntConstType value = opndSrc1->AsRegOpnd()->m_sym->GetIntConstValue();

        if (value == 0)
        {
            // If the negate operand is zero, the result is -0.0, which is a Number rather than an Int31.
            newOpnd = m_lowerer->LoadLibraryValueOpnd(instrNeg, LibraryValue::ValueNegativeZero);
        }
        else
        {
            // negation below can overflow because max negative int32 value > max positive value by 1.
            newOpnd = IR::AddrOpnd::NewFromNumber(-(int64)value, m_func);
        }

        instrNeg->ClearBailOutInfo();
        instrNeg->FreeSrc1();
        instrNeg->SetSrc1(newOpnd);
        instrNeg = this->ChangeToAssign(instrNeg);

        // Skip lowering call to helper
        return false;
    }

    bool isInt = (opndSrc1->IsTaggedInt());

    if (opndSrc1->IsRegOpnd() && opndSrc1->AsRegOpnd()->IsNotInt())
    {
        return true;
    }

    labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
    if (!isInt)
    {
        GenerateSmIntTest(opndSrc1, instrNeg, labelHelper);
    }

    // For 32 bit arithmetic we copy them and set the size of operands to be 32 bits.
    opndSrc1 = opndSrc1->UseWithNewType(TyInt32, this->m_func);
    GenerateTaggedZeroTest(opndSrc1, instrNeg, labelHelper);

    if (opndSrc1->IsEqual(instrNeg->GetDst()))
    {
        usingNewDst = true;
        opndDst = IR::RegOpnd::New(TyInt32, this->m_func);
    }
    else
    {
        opndDst = instrNeg->GetDst()->UseWithNewType(TyInt32, this->m_func);
    }

    // dst = SUBS zr, src

    instr = IR::Instr::New(Js::OpCode::SUBS, opndDst, IR::RegOpnd::New(nullptr, RegZR, TyInt32, this->m_func), opndSrc1, this->m_func);
    instrNeg->InsertBefore(instr);

    // BVS $helper

    instr = IR::BranchInstr::New(Js::OpCode::BVS, labelHelper, this->m_func);
    instrNeg->InsertBefore(instr);

    //
    // Convert TyInt32 operand, back to TyMachPtr type.
    //
    if (TyMachReg != opndDst->GetType())
    {
        opndDst = opndDst->UseWithNewType(TyMachPtr, this->m_func);
    }

    GenerateInt32ToVarConversion(opndDst, instrNeg);

    if (usingNewDst)
    {
        Lowerer::InsertMove(instrNeg->GetDst(), opndDst, instrNeg);
    }

    // B $fallthru

    labelFallThru = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    instr = IR::BranchInstr::New(Js::OpCode::B, labelFallThru, this->m_func);
    instrNeg->InsertBefore(instr);

    // $helper:
    //      (caller generates helper sequence)
    // $fallthru:

    AssertMsg(labelHelper, "Should not be NULL");
    instrNeg->InsertBefore(labelHelper);
    instrNeg->InsertAfter(labelFallThru);

    return true;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastShiftLeft
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastShiftLeft(IR::Instr * instrShift)
{
    // Left empty to match AMD64; assuming this is not performance critical
    return true;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateFastShiftRight
///
///----------------------------------------------------------------------------

bool
LowererMD::GenerateFastShiftRight(IR::Instr * instrShift)
{
    // Given:
    //
    // dst = Shr/ShrU src1, src2
    //
    // Generate:
    //
    // (If not 2 Int31's, jump to $helper.)
    // s1 = MOV src1
    // s2 = MOV src2
    //      AND s2, 0x1F               [unsigned only]  // Bail if unsigned and not shifting,
    //      BEQ $helper                [unsigned only]  // as we may not end up with a taggable int
    // s1 = ASR/LSR s1, s2
    //      ORR s1, 1 << VarTag_Shift
    //dst = MOV s1
    //      B $fallthru
    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    IR::Instr *      instr;
    IR::LabelInstr * labelHelper;
    IR::LabelInstr * labelFallThru;
    IR::Opnd *       opndReg;
    IR::Opnd *       opndSrc1;
    IR::Opnd *       opndSrc2;
    Assert(instrShift->m_opcode == Js::OpCode::ShrU_A || instrShift->m_opcode == Js::OpCode::Shr_A);
    bool             isUnsigned = (instrShift->m_opcode == Js::OpCode::ShrU_A);

    opndSrc1 = instrShift->GetSrc1();
    opndSrc2 = instrShift->GetSrc2();
    AssertMsg(opndSrc1 && opndSrc2, "Expected 2 src opnd's on Add instruction");

    // Not int?
    if (opndSrc1->IsRegOpnd() && opndSrc1->AsRegOpnd()->IsNotInt())
    {
        return true;
    }
    if (opndSrc2->IsRegOpnd() && opndSrc2->AsRegOpnd()->IsNotInt())
    {
        return true;
    }

    // Tagged ints?
    bool isTaggedInts = false;
    if (opndSrc1->IsTaggedInt())
    {
        if (opndSrc2->IsTaggedInt())
        {
            isTaggedInts = true;
        }
    }

    IntConstType s2Value = 0;
    bool src2IsIntConst = false;

    if (isUnsigned)
    {
        if (opndSrc2->IsRegOpnd())
        {
            src2IsIntConst = opndSrc2->AsRegOpnd()->m_sym->IsTaggableIntConst();
            if (src2IsIntConst)
            {
                s2Value = opndSrc2->AsRegOpnd()->m_sym->GetIntConstValue();
            }
        }
        else
        {
            AssertMsg(opndSrc2->IsAddrOpnd() && Js::TaggedInt::Is(opndSrc2->AsAddrOpnd()->m_address),
                "Expect src2 of shift right to be reg or Var.");
            src2IsIntConst = true;
            s2Value = Js::TaggedInt::ToInt32(opndSrc2->AsAddrOpnd()->m_address);
        }

        // 32-bit Shifts only uses the bottom 5 bits.
        s2Value &=  0x1F;

        // Unsigned shift by 0 could yield a value not encodable as a tagged int.
        if (isUnsigned && src2IsIntConst && s2Value == 0)
        {
            return true;
        }
    }


    labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);

    if (!isTaggedInts)
    {
        // (If not 2 Int31's, jump to $helper.)

        this->GenerateSmIntPairTest(instrShift, opndSrc1, opndSrc2, labelHelper);
    }

    opndSrc1    = opndSrc1->UseWithNewType(TyInt32, this->m_func);
    if (src2IsIntConst)
    {
        opndSrc2 = IR::IntConstOpnd::New(s2Value, TyInt32, this->m_func);
    }
    else
    {
        // s2 = MOV src2
        opndReg = IR::RegOpnd::New(TyInt32, this->m_func);
        Lowerer::InsertMove(opndReg, opndSrc2, instrShift);
        opndSrc2 = opndReg;
    }

    if (!src2IsIntConst && isUnsigned)
    {
        //      s2 = AND s2, 0x1F            [unsigned only]  // Bail if unsigned and not shifting,
        instr = IR::Instr::New(Js::OpCode::AND, opndSrc2, opndSrc2, IR::IntConstOpnd::New(0x1F, TyInt32, this->m_func), this->m_func);
        instrShift->InsertBefore(instr);

        //      CBZ s2, $helper              [unsigned only]  // as we may not end up with a taggable int
        instr = IR::BranchInstr::New(Js::OpCode::CBZ, labelHelper, this->m_func);
        instr->SetSrc1(opndSrc2);
        instrShift->InsertBefore(instr);
    }

    // s1 = MOV src1
    opndReg = IR::RegOpnd::New(TyInt32, this->m_func);
    Lowerer::InsertMove(opndReg, opndSrc1, instrShift);

    // s1 = ASR/LSR s1, RCX
    instr = IR::Instr::New(isUnsigned ? Js::OpCode::LSR : Js::OpCode::ASR, opndReg, opndReg, opndSrc2, this->m_func);
    instrShift->InsertBefore(instr);

    //
    // Convert TyInt32 operand, back to TyMachPtr type.
    //

    if(TyMachReg != opndReg->GetType())
    {
        opndReg = opndReg->UseWithNewType(TyMachPtr, this->m_func);
    }

    //    ORR s1, 1 << VarTag_Shift
    this->GenerateInt32ToVarConversion(opndReg, instrShift);

    // dst = MOV s1

    instr = IR::Instr::New(Js::OpCode::MOV, instrShift->GetDst(), opndReg, this->m_func);
    instrShift->InsertBefore(instr);

    //      B $fallthru

    labelFallThru = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    instr = IR::BranchInstr::New(Js::OpCode::B, labelFallThru, this->m_func);
    instrShift->InsertBefore(instr);

    // $helper:
    //      (caller generates helper call)
    // $fallthru:

    instrShift->InsertBefore(labelHelper);
    instrShift->InsertAfter(labelFallThru);

    return true;
}

void
LowererMD::GenerateFastBrS(IR::BranchInstr *brInstr)
{
    IR::Opnd *src1 = brInstr->UnlinkSrc1();

    Assert(src1->IsIntConstOpnd() || src1->IsAddrOpnd() || src1->IsRegOpnd());

    m_lowerer->InsertTest(
        m_lowerer->LoadOptimizationOverridesValueOpnd(
            brInstr, OptimizationOverridesValue::OptimizationOverridesSideEffects),
        src1,
        brInstr);

    Js::OpCode opcode;

    switch(brInstr->m_opcode)
    {
    case Js::OpCode::BrHasSideEffects:
        opcode = Js::OpCode::BNE;
        break;

    case Js::OpCode::BrNotHasSideEffects:
        opcode = Js::OpCode::BEQ;
        break;

    default:
        Assert(UNREACHED);
        __assume(false);
    }

    brInstr->m_opcode = opcode;
}

///----------------------------------------------------------------------------
///
/// LowererMD::GenerateSmIntPairTest
///
///     Generate code to test whether the given operands are both Int31 vars
/// and branch to the given label if not.
///
///----------------------------------------------------------------------------

IR::Instr *
LowererMD::GenerateSmIntPairTest(
    IR::Instr * instrInsert,
    IR::Opnd * opndSrc1,
    IR::Opnd * opndSrc2,
    IR::LabelInstr * labelFail)
{
    IR::Opnd *           opndReg;
    IR::Instr *          instrPrev = instrInsert->m_prev;
    IR::Instr *          instr;

    Assert(opndSrc1->GetType() == TyVar);
    Assert(opndSrc2->GetType() == TyVar);

    if (opndSrc1->IsTaggedInt())
    {
        IR::Opnd *tempOpnd = opndSrc1;
        opndSrc1 = opndSrc2;
        opndSrc2 = tempOpnd;
    }

    if (opndSrc2->IsTaggedInt())
    {
        if (opndSrc1->IsTaggedInt())
        {
            return instrPrev;
        }

        GenerateSmIntTest(opndSrc1, instrInsert, labelFail);
        return instrPrev;
    }

    opndReg = IR::RegOpnd::New(TyMachReg, this->m_func);

    // s1 = MOV src1
    // s1 = UBFX s1, VarTagShift - 16, 64 - (VarTag_Shift - 16)
    // s2 = MOV src2
    // s1 = BFXIL s2, VarTagShift, 64 - VarTag_Shift
    // s1 = EOR s1, AtomTag_Pair  ------ compare the tags together to the expected tag pair
    // CBNZ s1, $fail

    // s1 = MOV src1

    instr = IR::Instr::New(Js::OpCode::MOV, opndReg, opndSrc1, this->m_func);
    instrInsert->InsertBefore(instr);

    // s1 = UBFX s1, VarTagShift - 16, 64 - (VarTag_Shift - 16)

    instr = IR::Instr::New(Js::OpCode::UBFX, opndReg, opndReg, IR::IntConstOpnd::New(BITFIELD(Js::VarTag_Shift - 16, 64 - (Js::VarTag_Shift - 16)), TyMachReg, this->m_func), this->m_func);
    instrInsert->InsertBefore(instr);

    // s2 = MOV src2

    IR::Opnd * opndReg1 = IR::RegOpnd::New(TyMachReg, this->m_func);
    instr = IR::Instr::New(Js::OpCode::MOV, opndReg1, opndSrc2, this->m_func);
    instrInsert->InsertBefore(instr);

    // s1 = BFXIL s2, VarTagShift, 64 - VarTag_Shift

    instr = IR::Instr::New(Js::OpCode::BFXIL, opndReg, opndReg1, IR::IntConstOpnd::New(BITFIELD(Js::VarTag_Shift, 64 - Js::VarTag_Shift), TyMachReg, this->m_func), this->m_func);
    instrInsert->InsertBefore(instr);

    opndReg = opndReg->UseWithNewType(TyInt32, this->m_func)->AsRegOpnd();

    // s1 = EOR s1, AtomTag_Pair
    instr = IR::Instr::New(Js::OpCode::EOR, opndReg, opndReg, IR::IntConstOpnd::New(Js::AtomTag_Pair, TyInt32, this->m_func, true), this->m_func);
    instrInsert->InsertBefore(instr);

    //      CBNZ s1, $fail
    instr = IR::BranchInstr::New(Js::OpCode::CBNZ, labelFail, this->m_func);
    instr->SetSrc1(opndReg);
    instrInsert->InsertBefore(instr);

    return instrPrev;
}

bool LowererMD::GenerateObjectTest(IR::Opnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr * labelTarget, bool fContinueLabel)
{
    AssertMsg(opndSrc->GetSize() == MachPtr, "64-bit register required");

    if (opndSrc->IsTaggedValue() && fContinueLabel)
    {
        // Insert delete branch opcode to tell the dbChecks not to assert on the helper label we may fall through into
        IR::Instr *fakeBr = IR::PragmaInstr::New(Js::OpCode::DeletedNonHelperBranch, 0, this->m_func);
        insertInstr->InsertBefore(fakeBr);

        return false;
    }
    else if (opndSrc->IsNotTaggedValue() && !fContinueLabel)
    {
        return false;
    }

    IR::Opnd  * opndReg = IR::RegOpnd::New(TyMachReg, this->m_func);

    // s1 = MOV src1 - Move to a temporary
    IR::Instr * instr   = IR::Instr::New(Js::OpCode::MOV, opndReg, opndSrc, this->m_func);
    insertInstr->InsertBefore(instr);

    // s1 = UBFX s1, s1, #VarTag_Shift, #64 - VarTag_Shift
    instr = IR::Instr::New(Js::OpCode::UBFX, opndReg, opndReg, IR::IntConstOpnd::New(BITFIELD(Js::VarTag_Shift, 64 - Js::VarTag_Shift), TyMachReg, this->m_func), this->m_func);
    insertInstr->InsertBefore(instr);

    if (fContinueLabel)
    {
        // CBZ s1, $labelHelper
        instr = IR::BranchInstr::New(Js::OpCode::CBZ, labelTarget, this->m_func);
        instr->SetSrc1(opndReg);
        insertInstr->InsertBefore(instr);
        IR::LabelInstr *labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
        insertInstr->InsertBefore(labelHelper);
    }
    else
    {
        // CBNZ s1, $labelHelper
        instr = IR::BranchInstr::New(Js::OpCode::CBNZ, labelTarget, this->m_func);
        instr->SetSrc1(opndReg);
        insertInstr->InsertBefore(instr);
    }
    return true;
}

void
LowererMD::GenerateLoadTaggedType(IR::Instr * instrLdSt, IR::RegOpnd * opndType, IR::RegOpnd * opndTaggedType)
{
    // taggedType = OR type, InlineCacheAuxSlotTypeTag
    IR::IntConstOpnd * opndAuxSlotTag = IR::IntConstOpnd::New(InlineCacheAuxSlotTypeTag, TyInt8, instrLdSt->m_func);
    IR::Instr * instr = IR::Instr::New(Js::OpCode::ORR, opndTaggedType, opndType, opndAuxSlotTag, instrLdSt->m_func);
    instrLdSt->InsertBefore(instr);
}

void
LowererMD::GenerateLoadPolymorphicInlineCacheSlot(IR::Instr * instrLdSt, IR::RegOpnd * opndInlineCache, IR::RegOpnd * opndType, uint polymorphicInlineCacheSize)
{
    // Generate
    //
    // LDR r1, type
    // LSR r1, r1, #PolymorphicInlineCacheShift
    // AND r1, r1, #(size - 1)
    // LSL r1, r1, #log2(sizeof(Js::InlineCache))
    // ADD inlineCache, inlineCache, r1

    // MOV r1, type
    IR::RegOpnd * opndOffset = IR::RegOpnd::New(TyMachPtr, instrLdSt->m_func);
    IR::Instr * instr = IR::Instr::New(Js::OpCode::MOV, opndOffset, opndType, instrLdSt->m_func);
    instrLdSt->InsertBefore(instr);

    IntConstType rightShiftAmount = PolymorphicInlineCacheShift;
    IntConstType leftShiftAmount = Math::Log2(sizeof(Js::InlineCache));
    // instead of generating
    // LSR r1, r1, #PolymorphicInlineCacheShift
    // AND r1, r1, #(size - 1)
    // LSL r1, r1, #log2(sizeof(Js::InlineCache))
    //
    // we can generate:
    // LSR r1, r1, #(PolymorphicInlineCacheShift - log2(sizeof(Js::InlineCache))
    // AND r1, r1, #(size - 1) << log2(sizeof(Js::InlineCache))
    Assert(rightShiftAmount > leftShiftAmount);
    instr = IR::Instr::New(Js::OpCode::LSR, opndOffset, opndOffset, IR::IntConstOpnd::New(rightShiftAmount - leftShiftAmount, TyUint8, instrLdSt->m_func, true), instrLdSt->m_func);
    instrLdSt->InsertBefore(instr);
    Lowerer::InsertAnd(opndOffset, opndOffset, IR::IntConstOpnd::New(((IntConstType)(polymorphicInlineCacheSize - 1)) << leftShiftAmount, TyMachPtr, instrLdSt->m_func, true), instrLdSt);

    // ADD inlineCache, inlineCache, r1
    Lowerer::InsertAdd(false, opndInlineCache, opndInlineCache, opndOffset, instrLdSt);
}

//----------------------------------------------------------------------------
//
// LowererMD::GenerateFastScopedFldLookup
//
// This is a helper call which generates asm for both
// ScopedLdFld & ScopedStFld
//
//----------------------------------------------------------------------------

IR::Instr *
LowererMD::GenerateFastScopedFld(IR::Instr * instrScopedFld, bool isLoad)
{
    //      LDR s1, [base, offset(length)]
    //      CMP s1, 1                                -- get the length on array and test if it is 1.
    //      BNE $helper
    //      LDR s2, [base, offset(scopes)]           -- load the first scope
    //      LDR s3, [s2, offset(type)]
    //      LDIMM s4, inlineCache
    //      LDR s5, [s4, offset(u.local.type)]
    //      CMP s3, s5                               -- check type
    //      BNE $helper
    //      LDR s6, [s2, offset(slots)]             -- load the slots array
    //      LDR s7 , [s4, offset(u.local.slotIndex)]        -- load the cached slot index
    //
    //  if (load) {
    //      LDR dst, [s6, s7, LSL #2]                       -- load the value from the slot
    //  }
    //  else {
    //      STR src, [s6, s7, LSL #2]
    //  }
    //      B $done
    //$helper:
    //      dst = BLX PatchGetPropertyScoped(inlineCache, base, field, defaultInstance, scriptContext)
    //$done:

    IR::Instr *         instr;
    IR::Instr *         instrPrev = instrScopedFld->m_prev;

    IR::RegOpnd *       opndBase;
    IR::RegOpnd *       opndReg1; //s1
    IR::RegOpnd *       opndReg2; //s2
    IR::RegOpnd *       opndInlineCache; //s4
    IR::IndirOpnd *     indirOpnd;
    IR::Opnd *          propertyBase;

    IR::LabelInstr *    labelHelper;
    IR::LabelInstr *    labelFallThru;

    if (isLoad)
    {
        propertyBase = instrScopedFld->GetSrc1();
    }
    else
    {
        propertyBase = instrScopedFld->GetDst();
    }

    AssertMsg(propertyBase->IsSymOpnd() && propertyBase->AsSymOpnd()->IsPropertySymOpnd() && propertyBase->AsSymOpnd()->m_sym->IsPropertySym(),
        "Expected property sym operand of ScopedLdFld or ScopedStFld");

    IR::PropertySymOpnd * propertySymOpnd = propertyBase->AsPropertySymOpnd();

    opndBase = propertySymOpnd->CreatePropertyOwnerOpnd(m_func);
    const IR::AutoReuseOpnd holdAfterLegalization(opndBase, m_func);
    AssertMsg(opndBase->m_sym->m_isSingleDef, "We assume this isn't redefined");

    labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);

    // LDR s1, [base, offset(length)]     -- get the length on array and test if it is 1.
    indirOpnd = IR::IndirOpnd::New(opndBase, Js::FrameDisplay::GetOffsetOfLength(), TyInt16, this->m_func);
    opndReg1 = IR::RegOpnd::New(TyInt32, this->m_func);
    Lowerer::InsertMove(opndReg1, indirOpnd, instrScopedFld);

    // CMP s1, 1                                -- get the length on array and test if it is 1.
    instr = IR::Instr::New(Js::OpCode::CMP,  this->m_func);
    instr->SetSrc1(opndReg1);
    instr->SetSrc2(IR::IntConstOpnd::New(0x1, TyInt8, this->m_func));
    instrScopedFld->InsertBefore(instr);
    LegalizeMD::LegalizeInstr(instr);

    // BNE $helper
    instr = IR::BranchInstr::New(Js::OpCode::BNE, labelHelper, this->m_func);
    instrScopedFld->InsertBefore(instr);

    // LDR s2, [base, offset(scopes)]           -- load the first scope
    indirOpnd = IR::IndirOpnd::New(opndBase, Js::FrameDisplay::GetOffsetOfScopes(), TyMachReg,this->m_func);
    opndReg2 = IR::RegOpnd::New(TyMachReg, this->m_func);
    Lowerer::InsertMove(opndReg2, indirOpnd, instrScopedFld);

    // LDR s3, [s2, offset(type)]
    // LDIMM s4, inlineCache
    // LDR s5, [s4, offset(u.local.type)]
    // CMP s3, s5                               -- check type
    // BNE $helper

    opndInlineCache = IR::RegOpnd::New(TyMachReg, this->m_func);
    opndReg2->m_sym->m_isNotNumber = true;

    IR::RegOpnd * opndType = IR::RegOpnd::New(TyMachReg, this->m_func);
    this->m_lowerer->GenerateObjectTestAndTypeLoad(instrScopedFld, opndReg2, opndType, labelHelper);
    Lowerer::InsertMove(opndInlineCache, m_lowerer->LoadRuntimeInlineCacheOpnd(instrScopedFld, propertySymOpnd), instrScopedFld);

    labelFallThru = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);

    // Check the local cache with the tagged type
    IR::RegOpnd * opndTaggedType = IR::RegOpnd::New(TyMachReg, this->m_func);
    GenerateLoadTaggedType(instrScopedFld, opndType, opndTaggedType);
    Lowerer::GenerateLocalInlineCacheCheck(instrScopedFld, opndTaggedType, opndInlineCache, labelHelper);
    if (isLoad)
    {
        IR::Opnd *opndDst = instrScopedFld->GetDst();
        Lowerer::GenerateLdFldFromLocalInlineCache(instrScopedFld, opndReg2, opndDst, opndInlineCache, labelFallThru, false);
    }
    else
    {
        IR::Opnd *opndSrc = instrScopedFld->GetSrc1();
        GenerateStFldFromLocalInlineCache(instrScopedFld, opndReg2, opndSrc, opndInlineCache, labelFallThru, false);
    }

    // $helper:
    // if (isLoad) {
    // dst = BLX PatchGetPropertyScoped(inlineCache, opndBase, propertyId, srcBase, scriptContext)
    // }
    // else {
    //  BLX PatchSetPropertyScoped(inlineCache, base, field, value, defaultInstance, scriptContext)
    // }
    // $fallthru:
    instrScopedFld->InsertBefore(labelHelper);
    instrScopedFld->InsertAfter(labelFallThru);

    return instrPrev;
}

//----------------------------------------------------------------------------
//
// LowererMD::GenerateFastScopedLdFld
//
// Make use of the helper to cache the type and slot index used to do a ScopedLdFld
// when the scope is an array of length 1.
// Extract the only element from array and do an inline load from the appropriate slot
// if the type hasn't changed since the last time this ScopedLdFld was executed.
//
//----------------------------------------------------------------------------
IR::Instr *
LowererMD::GenerateFastScopedLdFld(IR::Instr * instrLdScopedFld)
{
    //Helper GenerateFastScopedFldLookup generates following:
    //
    //      LDR s1, [base, offset(length)]
    //      CMP s1, 1                                -- get the length on array and test if it is 1.
    //      BNE $helper
    //      LDR s2, [base, offset(scopes)]           -- load the first scope
    //      LDR s3, [s2, offset(type)]
    //      LDIMM s4, inlineCache
    //      LDR s5, [s4, offset(u.local.type)]
    //      CMP s3, s5                               -- check type
    //      BNE $helper
    //      LDR s6, [s2, offset(slots)]             -- load the slots array
    //      LDR s7 , [s4, offset(u.local.slotIndex)]        -- load the cached slot index
    //      LDR dst, [s6, s7, LSL #2]                       -- load the value from the slot
    //      B $done
    //$helper:
    //     dst = BLX PatchGetPropertyScoped(inlineCache, base, field, defaultInstance, scriptContext)
    //$done:

    return GenerateFastScopedFld(instrLdScopedFld, true);
}

//----------------------------------------------------------------------------
//
// LowererMD::GenerateFastScopedStFld
//
// Make use of the helper to cache the type and slot index used to do a ScopedStFld
// when the scope is an array of length 1.
// Extract the only element from array and do an inline load from the appropriate slot
// if the type hasn't changed since the last time this ScopedStFld was executed.
//
//----------------------------------------------------------------------------
IR::Instr *
LowererMD::GenerateFastScopedStFld(IR::Instr * instrStScopedFld)
{
    //      LDR s1, [base, offset(length)]
    //      CMP s1, 1                                -- get the length on array and test if it is 1.
    //      BNE $helper
    //      LDR s2, [base, offset(scopes)]           -- load the first scope
    //      LDR s3, [s2, offset(type)]
    //      LDIMM s4, inlineCache
    //      LDR s5, [s4, offset(u.local.type)]
    //      CMP s3, s5                               -- check type
    //      BNE $helper
    //      LDR s6, [s2, offset(slots)]             -- load the slots array
    //      LDR s7 , [s4, offset(u.local.slotIndex)]        -- load the cached slot index
    //      STR src, [s6, s7, LSL #2]                       -- store the value directly at the slot
    //  B $done
    //$helper:
    //      BLX PatchSetPropertyScoped(inlineCache, base, field, value, defaultInstance, scriptContext)
    //$done:

    return GenerateFastScopedFld(instrStScopedFld, false);
}

void
LowererMD::GenerateStFldFromLocalInlineCache(
    IR::Instr * instrStFld,
    IR::RegOpnd * opndBase,
    IR::Opnd * opndSrc,
    IR::RegOpnd * opndInlineCache,
    IR::LabelInstr * labelFallThru,
    bool isInlineSlot)
{
    IR::RegOpnd * opndSlotArray = nullptr;
    IR::IndirOpnd * opndIndir;
    IR::Instr * instr;

    if (!isInlineSlot)
    {
        // s2 = MOV base->slots -- load the slot array
        opndSlotArray = IR::RegOpnd::New(TyMachReg, instrStFld->m_func);
        opndIndir = IR::IndirOpnd::New(opndBase, Js::DynamicObject::GetOffsetOfAuxSlots(), TyMachReg, instrStFld->m_func);
        Lowerer::InsertMove(opndSlotArray, opndIndir, instrStFld);
    }

    // LDR s5, [s2, offset(u.local.slotIndex)] -- load the cached slot index
    IR::RegOpnd *opndSlotIndex = IR::RegOpnd::New(TyUint16, instrStFld->m_func);
    opndIndir = IR::IndirOpnd::New(opndInlineCache, offsetof(Js::InlineCache, u.local.slotIndex), TyUint16, instrStFld->m_func);
    Lowerer::InsertMove(opndSlotIndex, opndIndir, instrStFld);

    if (isInlineSlot)
    {
        // STR src, [base, s5, LSL #2]  -- store the value directly to the slot [s4 + s5 * 4] = src
        opndIndir = IR::IndirOpnd::New(opndBase, opndSlotIndex, LowererMD::GetDefaultIndirScale(), TyMachReg, instrStFld->m_func);
        instr = IR::Instr::New(Js::OpCode::STR, opndIndir, opndSrc, instrStFld->m_func);
        instrStFld->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);
    }
    else
    {
        // STR src, [s4, s5, LSL #2]  -- store the value directly to the slot [s4 + s5 * 4] = src
        opndIndir = IR::IndirOpnd::New(opndSlotArray, opndSlotIndex, LowererMD::GetDefaultIndirScale(), TyMachReg, instrStFld->m_func);
        instr = IR::Instr::New(Js::OpCode::STR, opndIndir, opndSrc, instrStFld->m_func);
        instrStFld->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);
    }

    // B $done
    instr = IR::BranchInstr::New(Js::OpCode::B, labelFallThru, instrStFld->m_func);
    instrStFld->InsertBefore(instr);
}

IR::Opnd *
LowererMD::CreateStackArgumentsSlotOpnd(Func *func)
{
    // Save the newly-created args object to its dedicated stack slot.
    IR::IndirOpnd *indirOpnd = IR::IndirOpnd::New(IR::RegOpnd::New(nullptr, FRAME_REG , TyMachReg, func),
            -MachArgsSlotOffset, TyMachPtr, func);

    return indirOpnd;
}

//
// jump to $labelHelper, based on the result of CMP
//
void LowererMD::GenerateSmIntTest(IR::Opnd *opndSrc, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, IR::Instr **instrFirst, bool fContinueLabel /* = false */)
{
    AssertMsg(opndSrc->GetSize() == MachPtr, "64-bit register required");

    IR::Opnd  * opndReg = IR::RegOpnd::New(TyMachReg, this->m_func);

    // s1 = MOV src1 - Move to a temporary
    IR::Instr * instr   = IR::Instr::New(Js::OpCode::MOV, opndReg, opndSrc, this->m_func);
    insertInstr->InsertBefore(instr);

    if (instrFirst)
    {
        *instrFirst = instr;
    }

    // s1 = UBFX s1, VarTag_Shift, 64 - VarTag_Shift
    instr = IR::Instr::New(Js::OpCode::UBFX, opndReg, opndReg, IR::IntConstOpnd::New(BITFIELD(Js::VarTag_Shift, 64 - Js::VarTag_Shift), TyMachReg, this->m_func), this->m_func);
    insertInstr->InsertBefore(instr);
    Legalize(instr);

    // s1 = EOR s1, AtomTag
    instr = IR::Instr::New(Js::OpCode::EOR, opndReg, opndReg, IR::IntConstOpnd::New(Js::AtomTag, TyInt32, this->m_func, /* dontEncode = */ true), this->m_func);
    insertInstr->InsertBefore(instr);

    if(fContinueLabel)
    {
        // CBZ s1, $labelHelper
        instr = IR::BranchInstr::New(Js::OpCode::CBZ, labelHelper, this->m_func);
    }
    else
    {
        // CBNZ s1,  $labelHelper
        instr = IR::BranchInstr::New(Js::OpCode::CBNZ, labelHelper, this->m_func);
    }
    instr->SetSrc1(opndReg);
    insertInstr->InsertBefore(instr);
}

void LowererMD::GenerateInt32ToVarConversion(IR::Opnd * opndSrc, IR::Instr * insertInstr )
{
    AssertMsg(opndSrc->IsRegOpnd(), "NYI for other types");

    IR::Instr* instr = IR::Instr::New(Js::OpCode::ORR, opndSrc, opndSrc, IR::IntConstOpnd::New(Js::AtomTag_IntPtr, TyMachReg, this->m_func), this->m_func);
    insertInstr->InsertBefore(instr);
}

IR::RegOpnd *
LowererMD::GenerateUntagVar(IR::RegOpnd * src, IR::LabelInstr * labelFail, IR::Instr * assignInstr, bool generateTagCheck)
{
    Assert(src->IsVar());

    //  MOV valueOpnd, index
    IR::RegOpnd *valueOpnd = IR::RegOpnd::New(TyInt32, this->m_func);

    //
    // Convert Index to 32 bits.
    //
    if (generateTagCheck)
    {
        IR::Opnd * opnd = src->UseWithNewType(TyMachReg, this->m_func);
        Assert(!opnd->IsTaggedInt());
        this->GenerateSmIntTest(opnd, assignInstr, labelFail);
    }

    // Doing a 32-bit MOV clears the tag bits on ARM64. Use MOV_TRUNC so it doesn't get peeped away.
    IR::Instr * instr = IR::Instr::New(Js::OpCode::MOV_TRUNC, valueOpnd, src->UseWithNewType(TyInt32, this->m_func), this->m_func);
    assignInstr->InsertBefore(instr);
    return valueOpnd;
}

IR::RegOpnd *LowererMD::LoadNonnegativeIndex(
    IR::RegOpnd *indexOpnd,
    const bool skipNegativeCheck,
    IR::LabelInstr *const notTaggedIntLabel,
    IR::LabelInstr *const negativeLabel,
    IR::Instr *const insertBeforeInstr)
{
    Assert(indexOpnd);
    Assert(indexOpnd->IsVar() || indexOpnd->GetType() == TyInt32 || indexOpnd->GetType() == TyUint32);
    Assert(indexOpnd->GetType() != TyUint32 || skipNegativeCheck);
    Assert(!indexOpnd->IsVar() || notTaggedIntLabel);
    Assert(skipNegativeCheck || negativeLabel);
    Assert(insertBeforeInstr);

    if(indexOpnd->IsVar())
    {
        if (indexOpnd->GetValueType().IsLikelyFloat())
        {
            return m_lowerer->LoadIndexFromLikelyFloat(indexOpnd, skipNegativeCheck, notTaggedIntLabel, negativeLabel, insertBeforeInstr);
        }

        indexOpnd = GenerateUntagVar(indexOpnd, notTaggedIntLabel, insertBeforeInstr, !indexOpnd->IsTaggedInt());
    }

    if(!skipNegativeCheck)
    {
        //     TBNZ index, #31, $notTaggedIntOrNegative
        IR::Instr *instr = IR::BranchInstr::New(Js::OpCode::TBNZ, negativeLabel, this->m_func);
        instr->SetSrc1(indexOpnd);
        instr->SetSrc2(IR::IntConstOpnd::New(31, TyVar, this->m_func));
        insertBeforeInstr->InsertBefore(instr);
    }
    return indexOpnd;
}

// Inlines fast-path for int Mul/Add or int Mul/Sub.  If not int, call MulAdd/MulSub helper
bool LowererMD::TryGenerateFastMulAdd(IR::Instr * instrAdd, IR::Instr ** pInstrPrev)
{
    IR::Instr *instrMul = instrAdd->GetPrevRealInstrOrLabel();
    IR::Opnd *addSrc;
    IR::RegOpnd *addCommonSrcOpnd;

    Assert(instrAdd->m_opcode == Js::OpCode::Add_A || instrAdd->m_opcode == Js::OpCode::Sub_A);

    bool isSub = (instrAdd->m_opcode == Js::OpCode::Sub_A) ? true : false;

    // Mul needs to be a single def reg
    if (instrMul->m_opcode != Js::OpCode::Mul_A || instrMul->GetDst()->IsRegOpnd() == false)
    {
        // Cannot generate MulAdd
        return false;
    }

    if (instrMul->HasBailOutInfo())
    {
        // Bailout will be generated for the Add, but not the Mul.
        // We could handle this, but this path isn't used that much anymore.
        return false;
    }

    IR::RegOpnd *regMulDst = instrMul->GetDst()->AsRegOpnd();

    if (regMulDst->m_sym->m_isSingleDef == false)
    {
        // Cannot generate MulAdd
        return false;
    }

    // Only handle a * b + c, so dst of Mul needs to match left source of Add
    if (instrMul->GetDst()->IsEqual(instrAdd->GetSrc1()))
    {
        addCommonSrcOpnd = instrAdd->GetSrc1()->AsRegOpnd();
        addSrc = instrAdd->GetSrc2();
    }
    else if (instrMul->GetDst()->IsEqual(instrAdd->GetSrc2()))
    {
        addSrc = instrAdd->GetSrc1();
        addCommonSrcOpnd = instrAdd->GetSrc2()->AsRegOpnd();
    }
    else
    {
        return false;
    }

    // Only handle a * b + c where c != a * b
    if (instrAdd->GetSrc1()->IsEqual(instrAdd->GetSrc2()))
    {
        return false;
    }

    if (addCommonSrcOpnd->m_isTempLastUse == false)
    {
        return false;
    }

    IR::Opnd *mulSrc1 = instrMul->GetSrc1();
    IR::Opnd *mulSrc2 = instrMul->GetSrc2();

    if (mulSrc1->IsRegOpnd() && mulSrc1->AsRegOpnd()->IsTaggedInt()
        && mulSrc2->IsRegOpnd() && mulSrc2->AsRegOpnd()->IsTaggedInt())
    {
        return false;
    }

    // Save prevInstr for the main lower loop
    *pInstrPrev = instrMul->m_prev;

    // Generate int31 fast-path for Mul, go to MulAdd helper if it fails, or one of the source is marked notInt
    if (!(addSrc->IsRegOpnd() && addSrc->AsRegOpnd()->IsNotInt())
        && !(mulSrc1->IsRegOpnd() && mulSrc1->AsRegOpnd()->IsNotInt())
        && !(mulSrc2->IsRegOpnd() && mulSrc2->AsRegOpnd()->IsNotInt()))
    {
        this->GenerateFastMul(instrMul);

        IR::LabelInstr *labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
        IR::Instr *instr = IR::BranchInstr::New(Js::OpCode::B, labelHelper, this->m_func);
        instrMul->InsertBefore(instr);

        // Generate int31 fast-path for Add
        bool success;
        if (isSub)
        {
            success = this->GenerateFastSub(instrAdd);
        }
        else
        {
            success = this->GenerateFastAdd(instrAdd);
        }

        if (!success)
        {
            labelHelper->isOpHelper = false;
        }
        // Generate MulAdd helper call
        instrAdd->InsertBefore(labelHelper);
    }

    if (instrAdd->dstIsTempNumber)
    {
        m_lowerer->LoadHelperTemp(instrAdd, instrAdd);
    }
    else
    {
        IR::Opnd *tempOpnd = IR::IntConstOpnd::New(0, TyInt32, this->m_func);
        this->LoadHelperArgument(instrAdd, tempOpnd);
    }
    this->m_lowerer->LoadScriptContext(instrAdd);

    IR::JnHelperMethod helper;

    if (addSrc == instrAdd->GetSrc2())
    {
        instrAdd->FreeSrc1();
        IR::Opnd *addOpnd = instrAdd->UnlinkSrc2();
        this->LoadHelperArgument(instrAdd, addOpnd);
        helper = isSub ? IR::HelperOp_MulSubRight : IR::HelperOp_MulAddRight;
    }
    else
    {
        instrAdd->FreeSrc2();
        IR::Opnd *addOpnd = instrAdd->UnlinkSrc1();
        this->LoadHelperArgument(instrAdd, addOpnd);
        helper = isSub ? IR::HelperOp_MulSubLeft : IR::HelperOp_MulAddLeft;
    }

    IR::Opnd *src2 = instrMul->UnlinkSrc2();
    this->LoadHelperArgument(instrAdd, src2);

    IR::Opnd *src1 = instrMul->UnlinkSrc1();
    this->LoadHelperArgument(instrAdd, src1);

    this->ChangeToHelperCall(instrAdd, helper);

    instrMul->Remove();

    return true;
}

IR::Instr *
LowererMD::LoadCheckedFloat(
    IR::RegOpnd *opndOrig,
    IR::RegOpnd *opndFloat,
    IR::LabelInstr *labelInline,
    IR::LabelInstr *labelHelper,
    IR::Instr *instrInsert,
    const bool checkForNullInLoopBody)
{
    //
    //   if (TaggedInt::Is(opndOrig))
    //       opndFloat = FCVT opndOrig_32
    //                   B $labelInline
    //   else
    //                   B $labelOpndIsNotInt
    //
    // $labelOpndIsNotInt:
    //   if (TaggedFloat::Is(opndOrig))
    //       s2        = MOV opndOrig
    //       s2        = EOR FloatTag_Value
    //       opndFloat = FCVT s2
    //   else
    //                   B $labelHelper
    //
    // $labelInline:
    //

    IR::Instr *instrFirst = nullptr;

    IR::LabelInstr *labelOpndIsNotInt = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    GenerateSmIntTest(opndOrig, instrInsert, labelOpndIsNotInt, &instrFirst);

    if (opndOrig->GetValueType().IsLikelyFloat())
    {
        // Make this path helper if value is likely a float
        instrInsert->InsertBefore(IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true));
    }

    IR::Opnd    *opndOrig_32 = opndOrig->UseWithNewType(TyInt32, this->m_func);
    EmitIntToFloat(opndFloat, opndOrig_32, instrInsert);

    IR::Instr   *jmpInline = IR::BranchInstr::New(Js::OpCode::B, labelInline, this->m_func);
    instrInsert->InsertBefore(jmpInline);

    instrInsert->InsertBefore(labelOpndIsNotInt);

    GenerateFloatTest(opndOrig, instrInsert, labelHelper, checkForNullInLoopBody);

    IR::RegOpnd *s2 = IR::RegOpnd::New(TyMachReg, this->m_func);
    IR::Instr   *mov = IR::Instr::New(Js::OpCode::MOV, s2, opndOrig, this->m_func);
    instrInsert->InsertBefore(mov);

    IR::Instr   *eorTag = IR::Instr::New(Js::OpCode::EOR,
        s2,
        s2,
        IR::IntConstOpnd::New(Js::FloatTag_Value,
            TyMachReg,
            this->m_func,
            /* dontEncode = */ true),
        this->m_func);
    instrInsert->InsertBefore(eorTag);

    IR::Instr   *movFloat = IR::Instr::New(Js::OpCode::FMOV_GEN, opndFloat, s2, this->m_func);
    instrInsert->InsertBefore(movFloat);

    return instrFirst;
}

void
LowererMD::EmitLoadFloatFromNumber(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr)
{
    IR::LabelInstr *labelDone;
    IR::Instr *instr;
    labelDone = EmitLoadFloatCommon(dst, src, insertInstr, insertInstr->HasBailOutInfo());

    if (labelDone == nullptr)
    {
        // We're done
        insertInstr->Remove();
        return;
    }

    // $Done   note: insertAfter
    insertInstr->InsertAfter(labelDone);

    if (!insertInstr->HasBailOutInfo())
    {
        // $Done
        insertInstr->Remove();
        return;
    }

    IR::LabelInstr *labelNoBailOut = nullptr;
    IR::SymOpnd *tempSymOpnd = nullptr;

    if (insertInstr->GetBailOutKind() == IR::BailOutPrimitiveButString)
    {
        if (!this->m_func->tempSymDouble)
        {
            this->m_func->tempSymDouble = StackSym::New(TyFloat64, this->m_func);
            this->m_func->StackAllocate(this->m_func->tempSymDouble, MachDouble);
        }

        // LEA r3, tempSymDouble
        IR::RegOpnd *reg3Opnd = IR::RegOpnd::New(TyMachReg, this->m_func);
        tempSymOpnd = IR::SymOpnd::New(this->m_func->tempSymDouble, TyFloat64, this->m_func);
        Lowerer::InsertLea(reg3Opnd, tempSymOpnd, insertInstr);

        // regBoolResult = to_number_fromPrimitive(value, &dst, allowUndef, scriptContext);

        this->m_lowerer->LoadScriptContext(insertInstr);
        IR::IntConstOpnd *allowUndefOpnd;
        if (insertInstr->GetBailOutKind() == IR::BailOutPrimitiveButString)
        {
            allowUndefOpnd = IR::IntConstOpnd::New(true, TyInt32, this->m_func);
        }
        else
        {
            Assert(insertInstr->GetBailOutKind() == IR::BailOutNumberOnly);
            allowUndefOpnd = IR::IntConstOpnd::New(false, TyInt32, this->m_func);
        }
        this->LoadHelperArgument(insertInstr, allowUndefOpnd);
        this->LoadHelperArgument(insertInstr, reg3Opnd);
        this->LoadHelperArgument(insertInstr, src);

        IR::RegOpnd *regBoolResult = IR::RegOpnd::New(TyInt32, this->m_func);
        instr = IR::Instr::New(Js::OpCode::Call, regBoolResult, IR::HelperCallOpnd::New(IR::HelperOp_ConvNumber_FromPrimitive, this->m_func), this->m_func);
        insertInstr->InsertBefore(instr);

        this->LowerCall(instr, 0);

        // TEST regBoolResult, regBoolResult
        instr = IR::Instr::New(Js::OpCode::TST, this->m_func);
        instr->SetSrc1(regBoolResult);
        instr->SetSrc2(regBoolResult);
        insertInstr->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);

        // BNE $noBailOut
        labelNoBailOut = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
        instr = IR::BranchInstr::New(Js::OpCode::BNE, labelNoBailOut, this->m_func);
        insertInstr->InsertBefore(instr);
    }

    // Bailout code
    Assert(insertInstr->m_opcode == Js::OpCode::FromVar);
    insertInstr->UnlinkDst();
    insertInstr->FreeSrc1();
    IR::Instr *bailoutInstr = insertInstr;
    insertInstr = bailoutInstr->m_next;
    this->m_lowerer->GenerateBailOut(bailoutInstr);

    // $noBailOut
    if (labelNoBailOut)
    {
        insertInstr->InsertBefore(labelNoBailOut);

        Assert(dst->IsRegOpnd());

        // VLDR dst, [pResult].f64
        instr = IR::Instr::New(Js::OpCode::FLDR, dst, tempSymOpnd, this->m_func);
        insertInstr->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);
    }
}

IR::LabelInstr*
LowererMD::EmitLoadFloatCommon(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr, bool needHelperLabel)
{
    IR::Instr *instr;

    Assert(src->GetType() == TyVar);
    Assert(dst->GetType() == TyFloat64 || TyFloat32);
    bool isFloatConst = false;
    IR::RegOpnd *regFloatOpnd = nullptr;

    if (src->IsRegOpnd() && src->AsRegOpnd()->m_sym->m_isFltConst)
    {
        IR::RegOpnd *regOpnd = src->AsRegOpnd();
        Assert(regOpnd->m_sym->m_isSingleDef);

        Js::Var value = regOpnd->m_sym->GetFloatConstValueAsVar_PostGlobOpt();
        IR::MemRefOpnd *memRef = IR::MemRefOpnd::New((BYTE*)value + Js::JavascriptNumber::GetValueOffset(), TyFloat64, this->m_func, IR::AddrOpndKindDynamicDoubleRef);
        regFloatOpnd = IR::RegOpnd::New(TyFloat64, this->m_func);
        instr = IR::Instr::New(Js::OpCode::FLDR, regFloatOpnd, memRef, this->m_func);
        insertInstr->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);
        isFloatConst = true;
    }
    // Src is constant?
    if (src->IsImmediateOpnd() || src->IsFloatConstOpnd())
    {
        regFloatOpnd = IR::RegOpnd::New(TyFloat64, this->m_func);
        m_lowerer->LoadFloatFromNonReg(src, regFloatOpnd, insertInstr);
        isFloatConst = true;
    }
    if (isFloatConst)
    {
        if (dst->GetType() == TyFloat32)
        {
            // FCVT.F32.F64 regOpnd32.f32, regOpnd.f64    -- Convert regOpnd from f64 to f32
            IR::RegOpnd *regOpnd32 = regFloatOpnd->UseWithNewType(TyFloat32, this->m_func)->AsRegOpnd();
            instr = IR::Instr::New(Js::OpCode::FCVT, regOpnd32, regFloatOpnd, this->m_func);
            insertInstr->InsertBefore(instr);

            // FMOV dst, regOpnd32
            instr = IR::Instr::New(Js::OpCode::FMOV, dst, regOpnd32, this->m_func);
            insertInstr->InsertBefore(instr);
        }
        else
        {
            instr = IR::Instr::New(Js::OpCode::FMOV, dst, regFloatOpnd, this->m_func);
            insertInstr->InsertBefore(instr);
        }
        LegalizeMD::LegalizeInstr(instr);
        return nullptr;
    }
    Assert(src->IsRegOpnd());

    IR::LabelInstr *labelStore = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    IR::LabelInstr *labelHelper;
    IR::LabelInstr *labelDone = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    if (needHelperLabel)
    {
        labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
    }
    else
    {
        labelHelper = labelDone;
    }
    IR::RegOpnd *reg2 = IR::RegOpnd::New(TyMachDouble, this->m_func);
    // Load the float value in reg2
    this->LoadCheckedFloat(src->AsRegOpnd(), reg2, labelStore, labelHelper, insertInstr, needHelperLabel);

    // $Store
    insertInstr->InsertBefore(labelStore);
    if (dst->GetType() == TyFloat32)
    {
        IR::RegOpnd *reg2_32 = reg2->UseWithNewType(TyFloat32, this->m_func)->AsRegOpnd();
        // FCVT.F32.F64 r2_32.f32, r2.f64    -- Convert regOpnd from f64 to f32
        instr = IR::Instr::New(Js::OpCode::FCVT, reg2_32, reg2, this->m_func);
        insertInstr->InsertBefore(instr);

        // FMOV dst, r2_32
        instr = IR::Instr::New(Js::OpCode::FMOV, dst, reg2_32, this->m_func);
        insertInstr->InsertBefore(instr);
    }
    else
    {
        // FMOV dst, r2
        instr = IR::Instr::New(Js::OpCode::FMOV, dst, reg2, this->m_func);
        insertInstr->InsertBefore(instr);
    }
    LegalizeMD::LegalizeInstr(instr);

    // B $Done
    instr = IR::BranchInstr::New(Js::OpCode::B, labelDone, this->m_func);
    insertInstr->InsertBefore(instr);

    if (needHelperLabel)
    {
        // $Helper
        insertInstr->InsertBefore(labelHelper);
    }

    return labelDone;
}

void
LowererMD::EmitLoadFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr, IR::Instr * instrBailOut, IR::LabelInstr * labelBailOut)
{
    IR::LabelInstr *labelDone;
    IR::Instr *instr;

    Assert(src->GetType() == TyVar);
    Assert(dst->GetType() == TyFloat64 || TyFloat32);
    Assert(src->IsRegOpnd());

    labelDone = EmitLoadFloatCommon(dst, src, insertInstr, true);

    if (labelDone == nullptr)
    {
        // We're done
        return;
    }

    IR::BailOutKind bailOutKind = instrBailOut && instrBailOut->HasBailOutInfo() ? instrBailOut->GetBailOutKind() : IR::BailOutInvalid;

    if (bailOutKind & IR::BailOutOnArrayAccessHelperCall)
    {
        // Bail out instead of making the helper call.
        Assert(labelBailOut);
        m_lowerer->InsertBranch(Js::OpCode::Br, labelBailOut, insertInstr);
        insertInstr->InsertBefore(labelDone);
        return;
    }

    IR::Opnd *memAddress = dst;
    if (dst->IsRegOpnd())
    {
        IR::SymOpnd *symOpnd = nullptr;
        if (dst->GetType() == TyFloat32)
        {
            symOpnd = IR::SymOpnd::New(StackSym::New(TyFloat32, this->m_func), TyFloat32, this->m_func);
            this->m_func->StackAllocate(symOpnd->m_sym->AsStackSym(), sizeof(float));
        }
        else
        {
            symOpnd = IR::SymOpnd::New(StackSym::New(TyFloat64,this->m_func), TyMachDouble, this->m_func);
            this->m_func->StackAllocate(symOpnd->m_sym->AsStackSym(), sizeof(double));
        }
        memAddress = symOpnd;
    }

    // LEA r3, dst
    IR::RegOpnd *reg3Opnd = IR::RegOpnd::New(TyMachReg, this->m_func);
    Lowerer::InsertLea(reg3Opnd, memAddress, insertInstr);

    // to_number_full(value, &dst, scriptContext);
    // Create dummy binary op to convert into helper

    instr = IR::Instr::New(Js::OpCode::Add_A, this->m_func);
    instr->SetSrc1(src);
    instr->SetSrc2(reg3Opnd);
    insertInstr->InsertBefore(instr);

    if (BailOutInfo::IsBailOutOnImplicitCalls(bailOutKind))
    {
        _Analysis_assume_(instrBailOut != nullptr);
        instr = instr->ConvertToBailOutInstr(instrBailOut->GetBailOutInfo(), bailOutKind);
        if (instrBailOut->GetBailOutInfo()->bailOutInstr == instrBailOut)
        {
            IR::Instr * instrShare = instrBailOut->ShareBailOut();
            m_lowerer->LowerBailTarget(instrShare);
        }
    }

    IR::JnHelperMethod helper;
    if (dst->GetType() == TyFloat32)
    {
        helper = IR::HelperOp_ConvFloat_Helper;
    }
    else
    {
        helper = IR::HelperOp_ConvNumber_Helper;
    }

    this->m_lowerer->LowerBinaryHelperMem(instr, helper);

    if (dst->IsRegOpnd())
    {
        instr = IR::Instr::New(Js::OpCode::FLDR, dst , memAddress, this->m_func);
        insertInstr->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);
    }

    // $Done
    insertInstr->InsertBefore(labelDone);
}

void
LowererMD::GenerateFastRecyclerAlloc(size_t allocSize, IR::RegOpnd* newObjDst, IR::Instr* insertionPointInstr, IR::LabelInstr* allocHelperLabel, IR::LabelInstr* allocDoneLabel)
{
    ScriptContextInfo* scriptContext = this->m_func->GetScriptContextInfo();
    void* allocatorAddress;
    uint32 endAddressOffset;
    uint32 freeListOffset;
    size_t alignedSize = HeapInfo::GetAlignedSizeNoCheck(allocSize);

    bool allowNativeCodeBumpAllocation = scriptContext->GetRecyclerAllowNativeCodeBumpAllocation();
    Recycler::GetNormalHeapBlockAllocatorInfoForNativeAllocation((void*)scriptContext->GetRecyclerAddr(), alignedSize,
        allocatorAddress, endAddressOffset, freeListOffset,
        allowNativeCodeBumpAllocation, this->m_func->IsOOPJIT());

    IR::RegOpnd * allocatorAddressRegOpnd = IR::RegOpnd::New(TyMachPtr, this->m_func);

    // LDIMM allocatorAddressRegOpnd, allocator
    IR::AddrOpnd* allocatorAddressOpnd = IR::AddrOpnd::New(allocatorAddress, IR::AddrOpndKindDynamicMisc, this->m_func);
    IR::Instr * loadAllocatorAddressInstr = IR::Instr::New(Js::OpCode::LDIMM, allocatorAddressRegOpnd, allocatorAddressOpnd, this->m_func);
    insertionPointInstr->InsertBefore(loadAllocatorAddressInstr);

    IR::IndirOpnd * endAddressOpnd = IR::IndirOpnd::New(allocatorAddressRegOpnd, endAddressOffset, TyMachPtr, this->m_func);
    IR::IndirOpnd * freeObjectListOpnd = IR::IndirOpnd::New(allocatorAddressRegOpnd, freeListOffset, TyMachPtr, this->m_func);

    // LDR newObjDst, allocator->freeObjectList
    IR::Instr * loadMemBlockInstr = IR::Instr::New(Js::OpCode::LDR, newObjDst, freeObjectListOpnd, this->m_func);
    insertionPointInstr->InsertBefore(loadMemBlockInstr);
    LegalizeMD::LegalizeInstr(loadMemBlockInstr);

    // nextMemBlock = ADD newObjDst, allocSize
    IR::RegOpnd * nextMemBlockOpnd = IR::RegOpnd::New(TyMachPtr, this->m_func);
    IR::IntConstOpnd* allocSizeOpnd = IR::IntConstOpnd::New((int32)allocSize, TyInt32, this->m_func);
    IR::Instr * loadNextMemBlockInstr = IR::Instr::New(Js::OpCode::ADD, nextMemBlockOpnd, newObjDst, allocSizeOpnd, this->m_func);
    insertionPointInstr->InsertBefore(loadNextMemBlockInstr);
    LegalizeMD::LegalizeInstr(loadNextMemBlockInstr);

    // CMP nextMemBlock, allocator->endAddress
    IR::Instr * checkInstr = IR::Instr::New(Js::OpCode::CMP, this->m_func);
    checkInstr->SetSrc1(nextMemBlockOpnd);
    checkInstr->SetSrc2(endAddressOpnd);
    insertionPointInstr->InsertBefore(checkInstr);
    LegalizeMD::LegalizeInstr(checkInstr);

    // BHI $allocHelper
    IR::BranchInstr * branchToAllocHelperInstr = IR::BranchInstr::New(Js::OpCode::BHI, allocHelperLabel, this->m_func);
    insertionPointInstr->InsertBefore(branchToAllocHelperInstr);

    // LDR allocator->freeObjectList, nextMemBlock
    IR::Instr * setFreeObjectListInstr = IR::Instr::New(Js::OpCode::LDR, freeObjectListOpnd, nextMemBlockOpnd, this->m_func);
    insertionPointInstr->InsertBefore(setFreeObjectListInstr);
    LegalizeMD::LegalizeInstr(setFreeObjectListInstr);

    // B $allocDone
    IR::BranchInstr * branchToAllocDoneInstr = IR::BranchInstr::New(Js::OpCode::B, allocDoneLabel, this->m_func);
    insertionPointInstr->InsertBefore(branchToAllocDoneInstr);
}

void
LowererMD::GenerateClz(IR::Instr * instr)
{
    Assert(instr->GetSrc1()->IsIntegral32());
    Assert(IRType_IsNativeInt(instr->GetDst()->GetType()));
    instr->m_opcode = Js::OpCode::CLZ;
    LegalizeMD::LegalizeInstr(instr);
}

void
LowererMD::SaveDoubleToVar(IR::RegOpnd * dstOpnd, IR::RegOpnd *opndFloat, IR::Instr *instrOrig, IR::Instr *instrInsert, bool isHelper)
{
    Assert(opndFloat->GetType() == TyFloat64);

    // Call JSNumber::ToVar to save the float operand to the result of the original (var) instruction

    // s1 = MOV opndFloat
    IR::RegOpnd *s1 = IR::RegOpnd::New(TyMachReg, m_func);
    IR::Instr *mov = IR::Instr::New(Js::OpCode::FMOV_GEN, s1, opndFloat, m_func);
    instrInsert->InsertBefore(mov);

    if (m_func->GetJITFunctionBody()->IsAsmJsMode())
    {
        // s1 = FMOV_GEN src
        // tmp = UBFX s1, #52, #11 ; extract exponent, bits 52-62
        // cmp = tmp, 0x7FF
        // beq tmp, helper
        // b done
        // helper:
        // tmp2 = tmp2 = UBFX s1, #0, #52 ; extract mantissa, bits 0-51
        // cbz tmp2, done
        // s1 = JavascriptNumber::k_Nan
        // done:

        IR::RegOpnd* tmp = IR::RegOpnd::New(TyMachReg, m_func);

        IR::Instr* newInstr = IR::Instr::New(Js::OpCode::UBFX, tmp, s1, IR::IntConstOpnd::New(BITFIELD(52, 11), TyMachReg, m_func, true), m_func);
        instrInsert->InsertBefore(newInstr);
        LowererMD::Legalize(newInstr);

        newInstr = IR::Instr::New(Js::OpCode::CMP, tmp, IR::IntConstOpnd::New(0x7FF, TyMachReg, m_func, true), m_func);
        instrInsert->InsertBefore(newInstr);
        LowererMD::Legalize(newInstr);

        IR::LabelInstr* helper = Lowerer::InsertLabel(true, instrInsert);
        IR::Instr* branch = IR::BranchInstr::New(Js::OpCode::BEQ, helper, m_func);
        helper->InsertBefore(branch);

        IR::LabelInstr* done = Lowerer::InsertLabel(isHelper, instrInsert);

        Lowerer::InsertBranch(Js::OpCode::Br, done, helper);

        IR::RegOpnd* tmp2 = IR::RegOpnd::New(TyMachReg, m_func);

        newInstr = IR::Instr::New(Js::OpCode::UBFX, tmp2, s1, IR::IntConstOpnd::New(BITFIELD(0, 52), TyMachReg, m_func, true), m_func);
        done->InsertBefore(newInstr);
        LowererMD::Legalize(newInstr);

        branch = IR::BranchInstr::New(Js::OpCode::CBZ, done, m_func);
        branch->SetSrc1(tmp2);
        done->InsertBefore(branch);

        IR::Opnd * opndNaN = IR::AddrOpnd::New((Js::Var)Js::JavascriptNumber::k_Nan, IR::AddrOpndKindConstantVar, m_func, true);
        Lowerer::InsertMove(s1, opndNaN, done);
    }

    // s1 = EOR s1, FloatTag_Value
    // dst = s1

    IR::Instr* setTag = IR::Instr::New(Js::OpCode::EOR, s1, s1, IR::AddrOpnd::New((Js::Var)Js::FloatTag_Value, IR::AddrOpndKindConstantVar, this->m_func, true), this->m_func);
    IR::Instr* movDst = IR::Instr::New(Js::OpCode::MOV, dstOpnd, s1, this->m_func);

    instrInsert->InsertBefore(setTag);
    instrInsert->InsertBefore(movDst);
    LowererMD::Legalize(setTag);
}

void
LowererMD::GenerateFastAbs(IR::Opnd *dst, IR::Opnd *src, IR::Instr *callInstr, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, IR::LabelInstr *doneLabel)
{
    //      if isFloat goto $float
    // s1 = MOV src
    //      CMP s1, #0
    // s1 = CSNEGPL s1, s1
    //      TBNZ s1, #31, $labelHelper
    // s1 = ORR s1, AtomTag_IntPtr
    //      JMP $done
    // $float
    //      CMP [src], JavascriptNumber.vtable
    //      JNE $helper
    //      MOVSD r1, [src + offsetof(value)]
    //      ANDPD r1, absDoubleCst
    //      dst = DoubleToVar(r1)

    IR::Instr      *instr      = nullptr;
    IR::LabelInstr *labelFloat = nullptr;
    bool            isInt      = false;
    bool            isNotInt   = false;

    if (src->IsRegOpnd())
    {
        if (src->AsRegOpnd()->IsTaggedInt())
        {
            isInt = true;

        }
        else if (src->AsRegOpnd()->IsNotInt())
        {
            isNotInt = true;
        }
    }
    else if (src->IsAddrOpnd())
    {
        IR::AddrOpnd *varOpnd = src->AsAddrOpnd();
        Assert(varOpnd->IsVar() && Js::TaggedInt::Is(varOpnd->m_address));

        int64 absValue = ::_abs64(Js::TaggedInt::ToInt32(varOpnd->m_address));

        if (!Js::TaggedInt::IsOverflow(absValue))
        {
            varOpnd->SetAddress(Js::TaggedInt::ToVarUnchecked((int32)absValue), IR::AddrOpndKindConstantVar);

            instr = IR::Instr::New(Js::OpCode::MOV, dst, varOpnd, this->m_func);
            insertInstr->InsertBefore(instr);

            return;
        }
    }

    if (src->IsRegOpnd() == false)
    {
        IR::RegOpnd *regOpnd = IR::RegOpnd::New(TyVar, this->m_func);
        instr = IR::Instr::New(Js::OpCode::MOV, regOpnd, src, this->m_func);
        insertInstr->InsertBefore(instr);
        src = regOpnd;
    }

    bool emitFloatAbs = !isInt;

    if (!isNotInt)
    {
        if (!isInt)
        {
            IR::LabelInstr *label = labelHelper;
            if (emitFloatAbs)
            {
                label = labelFloat = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
            }

            GenerateSmIntTest(src, insertInstr, label);
        }

        // s1 = MOV src
        IR::RegOpnd *regSrc = IR::RegOpnd::New(TyInt32, this->m_func);
        Lowerer::InsertMove(regSrc, src, insertInstr);

        //      CMP s1, #0
        instr = IR::Instr::New(Js::OpCode::CMP, this->m_func);
        instr->SetSrc1(regSrc);
        instr->SetSrc2(IR::IntConstOpnd::New(0, IRType::TyInt32, this->m_func));
        insertInstr->InsertBefore(instr);
        Legalize(instr);

        // s1 = CSNEGPL s1, s1
        instr = IR::Instr::New(Js::OpCode::CSNEGPL, regSrc, regSrc, regSrc, this->m_func);
        insertInstr->InsertBefore(instr);
        Legalize(instr);

        //      TBNZ s1, #31, $labelHelper
        instr = IR::BranchInstr::New(Js::OpCode::TBNZ, labelHelper, this->m_func);
        instr->SetSrc1(regSrc);
        instr->SetSrc2(IR::IntConstOpnd::New(31, IRType::TyInt32, this->m_func));
        insertInstr->InsertBefore(instr);

        //      MOV dst, s1
        instr = IR::Instr::New(Js::OpCode::MOV, dst, regSrc, this->m_func);
        insertInstr->InsertBefore(instr);

        GenerateInt32ToVarConversion(dst, insertInstr);
    }

    if (labelFloat)
    {
        // B $done
        instr = IR::BranchInstr::New(Js::OpCode::B, doneLabel, this->m_func);
        insertInstr->InsertBefore(instr);

        // $float
        insertInstr->InsertBefore(labelFloat);
    }

    if (emitFloatAbs)
    {
        // if (typeof(src) == double)
        IR::RegOpnd *src64 = src->AsRegOpnd();
        GenerateFloatTest(src64, insertInstr, labelHelper);

        // dst64 = MOV src64
        insertInstr->InsertBefore(IR::Instr::New(Js::OpCode::MOV, dst, src64, this->m_func));

        // Unconditionally set the sign bit. This will get XORd away when we remove the tag.
        // dst64 = ORR 0x8000000000000000
        insertInstr->InsertBefore(IR::Instr::New(Js::OpCode::ORR, dst, dst, IR::IntConstOpnd::New(MachSignBit, TyMachReg, this->m_func), this->m_func));
    }
    else if(!isInt)
    {
        // The source is not known to be a tagged int, so either it's definitely not an int (isNotInt), or the int version of
        // abs failed the tag check and jumped here. We can't emit the float version of abs (!emitFloatAbs) due to SSE2 not
        // being available, so jump straight to the helper.

        // JMP $helper
        instr = IR::BranchInstr::New(Js::OpCode::B, labelHelper, this->m_func);
        insertInstr->InsertBefore(instr);
    }
}

void
LowererMD::EmitInt4Instr(IR::Instr *instr)
{
    IR::Instr * newInstr;
    IR::Opnd *  src1;
    IR::Opnd *  src2;

    switch (instr->m_opcode)
    {
    case Js::OpCode::Neg_I4:
        instr->m_opcode = Js::OpCode::SUB;
        instr->SetSrc2(instr->UnlinkSrc1());
        instr->SetSrc1(IR::RegOpnd::New(nullptr, RegZR, TyInt32, instr->m_func));
        break;

    case Js::OpCode::Not_I4:
        instr->m_opcode = Js::OpCode::MVN;
        break;

    case Js::OpCode::Add_I4:
        instr->m_opcode = Js::OpCode::ADD;
        break;

    case Js::OpCode::Sub_I4:
        instr->m_opcode = Js::OpCode::SUB;
        break;

    case Js::OpCode::Mul_I4:
        instr->m_opcode = Js::OpCode::MUL;
        break;

    case Js::OpCode::DivU_I4:
        AssertMsg(UNREACHED, "Unsigned div NYI");
    case Js::OpCode::Div_I4:
        instr->m_opcode = Js::OpCode::SDIV;
        break;

    case Js::OpCode::RemU_I4:
        AssertMsg(UNREACHED, "Unsigned rem NYI");
    case Js::OpCode::Rem_I4:
        instr->m_opcode = Js::OpCode::REM;
        break;

    case Js::OpCode::Or_I4:
        instr->m_opcode = Js::OpCode::ORR;
        break;

    case Js::OpCode::Xor_I4:
        instr->m_opcode = Js::OpCode::EOR;
        break;

    case Js::OpCode::And_I4:
        instr->m_opcode = Js::OpCode::AND;
        break;

    case Js::OpCode::Shl_I4:
    case Js::OpCode::ShrU_I4:
    case Js::OpCode::Shr_I4:
        ChangeToShift(instr, false /* needFlags */);
        break;

    case Js::OpCode::BrTrue_I4:
        instr->m_opcode = Js::OpCode::CBNZ;
        break;

    case Js::OpCode::BrFalse_I4:
        instr->m_opcode = Js::OpCode::CBZ;
        break;

    case Js::OpCode::BrEq_I4:
        instr->m_opcode = Js::OpCode::BEQ;
        goto br2_Common;

    case Js::OpCode::BrNeq_I4:
        instr->m_opcode = Js::OpCode::BNE;
        goto br2_Common;

    case Js::OpCode::BrGt_I4:
        instr->m_opcode = Js::OpCode::BGT;
        goto br2_Common;

    case Js::OpCode::BrGe_I4:
        instr->m_opcode = Js::OpCode::BGE;
        goto br2_Common;

    case Js::OpCode::BrLe_I4:
        instr->m_opcode = Js::OpCode::BLE;
        goto br2_Common;

    case Js::OpCode::BrLt_I4:
        instr->m_opcode = Js::OpCode::BLT;
        goto br2_Common;

    case Js::OpCode::BrUnGt_I4:
        instr->m_opcode = Js::OpCode::BHI;
        goto br2_Common;

    case Js::OpCode::BrUnGe_I4:
        instr->m_opcode = Js::OpCode::BCS;
        goto br2_Common;

    case Js::OpCode::BrUnLt_I4:
        instr->m_opcode = Js::OpCode::BCC;
        goto br2_Common;

    case Js::OpCode::BrUnLe_I4:
        instr->m_opcode = Js::OpCode::BLS;
        goto br2_Common;

br2_Common:
        src1 = instr->UnlinkSrc1();
        src2 = instr->UnlinkSrc2();
        newInstr = IR::Instr::New(Js::OpCode::CMP, instr->m_func);
        instr->InsertBefore(newInstr);
        newInstr->SetSrc1(src1);
        newInstr->SetSrc2(src2);
        // Let instr point to the CMP so we can legalize it.
        instr = newInstr;
        break;

    default:
        AssertMsg(UNREACHED, "NYI I4 instr");
        break;
    }

    LegalizeMD::LegalizeInstr(instr);
}

void
LowererMD::LowerInt4NegWithBailOut(
    IR::Instr *const instr,
    const IR::BailOutKind bailOutKind,
    IR::LabelInstr *const bailOutLabel,
    IR::LabelInstr *const skipBailOutLabel)
{
    Assert(instr);
    Assert(instr->m_opcode == Js::OpCode::Neg_I4);
    Assert(!instr->HasBailOutInfo());
    Assert(bailOutKind & IR::BailOutOnResultConditions || bailOutKind == IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck);
    Assert(bailOutLabel);
    Assert(instr->m_next == bailOutLabel);
    Assert(skipBailOutLabel);
    Assert(instr->GetDst()->IsInt32());
    Assert(instr->GetSrc1()->IsInt32());

    // SUBS dst, zr, src1
    // BVS   $bailOutLabel
    // BEQ   $bailOutLabel
    // B     $skipBailOut
    // $bailOut:
    // ...
    // $skipBailOut:

    // Lower the instruction
    instr->m_opcode = Js::OpCode::SUBS;
    instr->ReplaceDst(instr->GetDst()->UseWithNewType(TyInt32, instr->m_func));
    instr->SetSrc2(instr->UnlinkSrc1()->UseWithNewType(TyInt32, instr->m_func));
    instr->SetSrc1(IR::RegOpnd::New(nullptr, RegZR, TyInt32, instr->m_func));
    Legalize(instr);

    if(bailOutKind & IR::BailOutOnOverflow)
    {
        bailOutLabel->InsertBefore(IR::BranchInstr::New(Js::OpCode::BVS, bailOutLabel, instr->m_func));
    }

    if(bailOutKind & IR::BailOutOnNegativeZero)
    {
        bailOutLabel->InsertBefore(IR::BranchInstr::New(Js::OpCode::BEQ, bailOutLabel, instr->m_func));
    }

    // Skip bailout
    bailOutLabel->InsertBefore(IR::BranchInstr::New(LowererMD::MDUncondBranchOpcode, skipBailOutLabel, instr->m_func));
}

void
LowererMD::LowerInt4AddWithBailOut(
    IR::Instr *const instr,
    const IR::BailOutKind bailOutKind,
    IR::LabelInstr *const bailOutLabel,
    IR::LabelInstr *const skipBailOutLabel)
{
    Assert(instr);
    Assert(instr->m_opcode == Js::OpCode::Add_I4);
    Assert(!instr->HasBailOutInfo());
    Assert(
        (bailOutKind & IR::BailOutOnResultConditions) == IR::BailOutOnOverflow ||
        bailOutKind == IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck);
    Assert(bailOutLabel);
    Assert(instr->m_next == bailOutLabel);
    Assert(skipBailOutLabel);
    Assert(instr->GetDst()->IsInt32());
    Assert(instr->GetSrc1()->IsInt32());
    Assert(instr->GetSrc2()->IsInt32());

    // ADDS dst, src1, src2
    // BVC skipBailOutLabel
    // fallthrough to bailout

    const auto dst = instr->GetDst(), src1 = instr->GetSrc1(), src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd());

    const bool dstEquSrc1 = dst->IsEqual(src1), dstEquSrc2 = dst->IsEqual(src2);
    if(dstEquSrc1 || dstEquSrc2)
    {
        LowererMD::ChangeToAssign(instr->SinkDst(Js::OpCode::Ld_I4, RegNOREG, skipBailOutLabel));
    }

    // Lower the instruction
    ChangeToAdd(instr, true /* needFlags */);
    Legalize(instr);

    // Skip bailout on no overflow
    bailOutLabel->InsertBefore(IR::BranchInstr::New(Js::OpCode::BVC, skipBailOutLabel, instr->m_func));

    // Fall through to bailOutLabel
}

void
LowererMD::LowerInt4SubWithBailOut(
    IR::Instr *const instr,
    const IR::BailOutKind bailOutKind,
    IR::LabelInstr *const bailOutLabel,
    IR::LabelInstr *const skipBailOutLabel)
{
    Assert(instr);
    Assert(instr->m_opcode == Js::OpCode::Sub_I4);
    Assert(!instr->HasBailOutInfo());
    Assert(
        (bailOutKind & IR::BailOutOnResultConditions) == IR::BailOutOnOverflow ||
        bailOutKind == IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck);
    Assert(bailOutLabel);
    Assert(instr->m_next == bailOutLabel);
    Assert(skipBailOutLabel);
    Assert(instr->GetDst()->IsInt32());
    Assert(instr->GetSrc1()->IsInt32());
    Assert(instr->GetSrc2()->IsInt32());

    // SUBS dst, src1, src2
    // BVC skipBailOutLabel
    // fallthrough to bailout

    const auto dst = instr->GetDst(), src1 = instr->GetSrc1(), src2 = instr->GetSrc2();
    Assert(dst->IsRegOpnd());

    const bool dstEquSrc1 = dst->IsEqual(src1), dstEquSrc2 = dst->IsEqual(src2);
    if(dstEquSrc1 || dstEquSrc2)
    {
        LowererMD::ChangeToAssign(instr->SinkDst(Js::OpCode::Ld_I4, RegNOREG, skipBailOutLabel));
    }

    // Lower the instruction
    ChangeToSub(instr, true /* needFlags */);
    Legalize(instr);

    // Skip bailout on no overflow
    bailOutLabel->InsertBefore(IR::BranchInstr::New(Js::OpCode::BVC, skipBailOutLabel, instr->m_func));

    // Fall through to bailOutLabel
}

void
LowererMD::LowerInt4MulWithBailOut(
    IR::Instr *const instr,
    const IR::BailOutKind bailOutKind,
    IR::LabelInstr *const bailOutLabel,
    IR::LabelInstr *const skipBailOutLabel)
{
    Assert(instr);
    Assert(instr->m_opcode == Js::OpCode::Mul_I4);
    Assert(!instr->HasBailOutInfo());
    Assert(bailOutKind & IR::BailOutOnResultConditions || bailOutKind == IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck);
    Assert(bailOutLabel);
    Assert(instr->m_next == bailOutLabel);
    Assert(skipBailOutLabel);

    IR::Opnd *dst = instr->GetDst();
    IR::Opnd *src1 = instr->UnlinkSrc1();
    IR::Opnd *src2 = instr->UnlinkSrc2();
    IR::Instr *insertInstr;

    Assert(dst->IsInt32());
    Assert(src1->IsInt32());
    Assert(src2->IsInt32());

    // s3 = SMULL src1, src2 // result is i64
    IR::Opnd* s3 = IR::RegOpnd::New(TyInt64, instr->m_func);
    insertInstr = IR::Instr::New(Js::OpCode::SMULL, s3, src1, src2, instr->m_func);
    instr->InsertBefore(insertInstr);
    LegalizeMD::LegalizeInstr(insertInstr);

    // dst = MOV_TRUNC s3
    instr->m_opcode = Js::OpCode::MOV_TRUNC;
    instr->SetSrc1(s3->UseWithNewType(TyInt32, instr->m_func));

    // check negative zero
    //
    // If the result is zero, we need to check and only bail out if it would be -0.
    // We know that if the result is 0/-0, at least operand should be zero.
    // We should bailout if src1 + src2 < 0, as this proves that the other operand is negative
    //
    //    CMN src1, src2
    //    BPL $skipBailOutLabel
    //
    // $bailOutLabel
    //    GenerateBailout
    //
    // $skipBailOutLabel
    IR::LabelInstr *checkForNegativeZeroLabel = nullptr;
    if(bailOutKind & IR::BailOutOnNegativeZero)
    {
        checkForNegativeZeroLabel = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func, true);
        bailOutLabel->InsertBefore(checkForNegativeZeroLabel);

        Assert(dst->IsRegOpnd());
        Assert(!src1->IsEqual(src2)); // cannot result in -0 if both operands are the same; GlobOpt should have figured that out

        // CMN src1, src2
        // BPL $skipBailOutLabel
        insertInstr = IR::Instr::New(Js::OpCode::CMN, instr->m_func);
        insertInstr->SetSrc1(src1);
        insertInstr->SetSrc2(src2);
        bailOutLabel->InsertBefore(insertInstr);
        LegalizeMD::LegalizeInstr(insertInstr);
        bailOutLabel->InsertBefore(IR::BranchInstr::New(Js::OpCode::BPL, skipBailOutLabel, instr->m_func));

        // Fall through to bailOutLabel
    }

    IR::LabelInstr* insertBeforeInstr = checkForNegativeZeroLabel ? checkForNegativeZeroLabel : bailOutLabel;

    //check overflow
    if(bailOutKind & IR::BailOutOnMulOverflow || bailOutKind == IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck)
    {
        insertInstr = IR::Instr::New(Js::OpCode::CMP_SXTW, instr->m_func);
        insertInstr->SetSrc1(s3);
        insertInstr->SetSrc2(s3);
        instr->InsertBefore(insertInstr);

        // BNE $bailOutHelper
        insertInstr = IR::BranchInstr::New(Js::OpCode::BNE, bailOutLabel, instr->m_func);
        instr->InsertBefore(insertInstr);
    }

    if(bailOutKind & IR::BailOutOnNegativeZero)
    {
        // TST dst, dst
        // BEQ $checkForNegativeZeroLabel
        insertInstr = IR::Instr::New(Js::OpCode::TST, instr->m_func);
        insertInstr->SetSrc1(dst);
        insertInstr->SetSrc2(dst);
        insertBeforeInstr->InsertBefore(insertInstr);
        LegalizeMD::LegalizeInstr(insertInstr);

        insertBeforeInstr->InsertBefore(IR::BranchInstr::New(Js::OpCode::BEQ, checkForNegativeZeroLabel, instr->m_func));
    }

    insertBeforeInstr->InsertBefore(IR::BranchInstr::New(Js::OpCode::B, skipBailOutLabel, instr->m_func));
}

void
LowererMD::LowerInt4RemWithBailOut(
    IR::Instr *const instr,
    const IR::BailOutKind bailOutKind,
    IR::LabelInstr *const bailOutLabel,
    IR::LabelInstr *const skipBailOutLabel) const
{
    Assert(instr);
    Assert(instr->m_opcode == Js::OpCode::Rem_I4);
    Assert(!instr->HasBailOutInfo());
    Assert(bailOutKind & IR::BailOutOnResultConditions);
    Assert(bailOutLabel);
    Assert(instr->m_next == bailOutLabel);
    Assert(skipBailOutLabel);

    IR::Opnd *dst = instr->GetDst();
    IR::Opnd *src1 = instr->GetSrc1();

    Assert(dst->IsInt32());
    Assert(src1->IsInt32());
    Assert(instr->GetSrc2()->IsInt32());

    //Lower the instruction
    EmitInt4Instr(instr);

    //check for negative zero
    //We have, dst = src1 % src2
    //We need to bailout if dst == 0 and src1 < 0
    //     tst dst, dst
    //     bne $skipBailOutLabel
    //     tst src1,src1
    //     bpl $skipBailOutLabel
    //
    //$bailOutLabel
    //     GenerateBailout();
    //
    //$skipBailOutLabel
    if(bailOutKind & IR::BailOutOnNegativeZero)
    {
        IR::LabelInstr *checkForNegativeZeroLabel = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func, true);
        bailOutLabel->InsertBefore(checkForNegativeZeroLabel);

        IR::Instr *insertInstr = IR::Instr::New(Js::OpCode::TST, instr->m_func);
        insertInstr->SetSrc1(dst);
        insertInstr->SetSrc2(dst);
        bailOutLabel->InsertBefore(insertInstr);
        LegalizeMD::LegalizeInstr(insertInstr);

        IR::Instr *branchInstr = IR::BranchInstr::New(Js::OpCode::BNE, skipBailOutLabel, instr->m_func);
        bailOutLabel->InsertBefore(branchInstr);

        insertInstr = IR::Instr::New(Js::OpCode::TST, instr->m_func);
        insertInstr->SetSrc1(src1);
        insertInstr->SetSrc2(src1);
        bailOutLabel->InsertBefore(insertInstr);
        LegalizeMD::LegalizeInstr(insertInstr);

        branchInstr = IR::BranchInstr::New(Js::OpCode::BPL, skipBailOutLabel, instr->m_func);
        bailOutLabel->InsertBefore(branchInstr);
    }
    // Fall through to bailOutLabel
}

void
LowererMD::EmitLoadVar(IR::Instr *instrLoad, bool isFromUint32, bool isHelper)
{
    //    MOV.32 e1, e_src1
    //    TBNZ e1, #31, $Helper [uint32]  -- overflows?
    //    ORR r1, 1<<VarTag_Shift
    //    MOV r_dst, r1
    //    JMP $done             [uint32]
    // $helper                  [uint32]
    //    EmitLoadVarNoCheck
    // $done                    [uint32]


    Assert(instrLoad->GetSrc1()->IsRegOpnd());
    Assert(instrLoad->GetDst()->GetType() == TyVar);

    bool isInt            = false;
    IR::Opnd *dst         = instrLoad->GetDst();
    IR::RegOpnd *src1     = instrLoad->GetSrc1()->AsRegOpnd();
    IR::LabelInstr *labelHelper = nullptr;

    // TODO: Fix bad lowering. We shouldn't get TyVars here.
    // Assert(instrLoad->GetSrc1()->GetType() == TyInt32);
    src1->SetType(isFromUint32 ? TyUint32 : TyInt32);

    if (src1->IsTaggedInt())
    {
        isInt = true;
    }
    else if (src1->IsNotInt())
    {
        // ToVar()
        this->EmitLoadVarNoCheck(dst->AsRegOpnd(), src1, instrLoad, isFromUint32, isHelper);
        return;
    }

    IR::RegOpnd *r1 = IR::RegOpnd::New(TyVar, m_func);

    // e1 = MOV_TRUNC src1
    // (Use 32-bit MOV_TRUNC here as we rely on the register copy to clear the upper 32 bits.)
    IR::RegOpnd *e1 = r1->Copy(m_func)->AsRegOpnd();
    e1->SetType(TyInt32);
    instrLoad->InsertBefore(IR::Instr::New(Js::OpCode::MOV_TRUNC,
        e1,
        src1,
        m_func));

    if (!isInt && isFromUint32)
    {
        Assert(!labelHelper);
        labelHelper = IR::LabelInstr::New(Js::OpCode::Label, m_func, true);

        // TBNZ e1, #31, $helper
        IR::Instr* instr = IR::BranchInstr::New(Js::OpCode::TBNZ, labelHelper, m_func);
        instr->SetSrc1(e1);
        instr->SetSrc2(IR::IntConstOpnd::New(31, TyInt32, m_func));
        instrLoad->InsertBefore(instr);
    }

    // The previous operation clears the top 32 bits.
    // ORR r1, 1<<VarTag_Shift
    this->GenerateInt32ToVarConversion(r1, instrLoad);

    // REVIEW: We need r1 only if we could generate sn = Ld_A_I4 sn. i.e. the destination and
    // source are the same.
    // r_dst = MOV r1
    instrLoad->InsertBefore(IR::Instr::New(Js::OpCode::MOV,
        dst,
        r1,
        m_func));

    if (labelHelper)
    {
        Assert(isFromUint32);

        // B $done
        IR::LabelInstr * labelDone = IR::LabelInstr::New(Js::OpCode::Label, m_func, isHelper);
        instrLoad->InsertBefore(IR::BranchInstr::New(Js::OpCode::B, labelDone, m_func));

        // $helper
        instrLoad->InsertBefore(labelHelper);

        // ToVar()
        this->EmitLoadVarNoCheck(dst->AsRegOpnd(), src1, instrLoad, isFromUint32, true);

        // $done
        instrLoad->InsertBefore(labelDone);
    }
    instrLoad->Remove();
}

void
LowererMD::EmitLoadVarNoCheck(IR::RegOpnd * dst, IR::RegOpnd * src, IR::Instr *instrLoad, bool isFromUint32, bool isHelper)
{
    IR::RegOpnd * floatReg = IR::RegOpnd::New(TyFloat64, this->m_func);
    if (isFromUint32)
    {
        this->EmitUIntToFloat(floatReg, src, instrLoad);
    }
    else
    {
        this->EmitIntToFloat(floatReg, src, instrLoad);
    }
    this->SaveDoubleToVar(dst, floatReg, instrLoad, instrLoad, isHelper);
}

bool
LowererMD::EmitLoadInt32(IR::Instr *instrLoad, bool conversionFromObjectAllowed, bool bailOutOnHelper, IR::LabelInstr * labelBailOut)
{
    //
    //    r1 = MOV src1
    // rtest = UBFX src1, AtomTag_Shift, 64 - AtomTag_Shift
    //         EOR rtest, 1
    //         CBNZ $helper or $float
    // r_dst = MOV.32 e_src1
    //         B $done
    //  $float:
    //     dst = ConvertToFloat(r1, $helper)
    // $helper:
    // r_dst = ToInt32()
    //

    Assert(instrLoad->GetSrc1()->IsRegOpnd());
    Assert(instrLoad->GetSrc1()->GetType() == TyVar);

    // TODO: Fix bad lowering. We shouldn't see TyVars here.
    // Assert(instrLoad->GetDst()->GetType() == TyInt32);

    bool isInt             = false;
    bool isNotInt          = false;
    IR::Opnd *dst          = instrLoad->GetDst();
    IR::RegOpnd *src1      = instrLoad->GetSrc1()->AsRegOpnd();
    IR::LabelInstr *helper = nullptr;
    IR::LabelInstr *labelFloat = nullptr;
    IR::LabelInstr *done   = nullptr;

    if (src1->IsTaggedInt())
    {
        isInt = true;
    }
    else if (src1->IsNotInt())
    {
        isNotInt = true;
    }

    if (src1->IsEqual(instrLoad->GetDst()) == false)
    {
        // r1 = MOV src1
        IR::RegOpnd *r1 = IR::RegOpnd::New(TyVar, instrLoad->m_func);
        r1->SetValueType(src1->GetValueType());
        instrLoad->InsertBefore(IR::Instr::New(Js::OpCode::MOV, r1, src1, instrLoad->m_func));
        src1 = r1;
    }

    const ValueType src1ValueType(src1->GetValueType());
    const bool doFloatToIntFastPath =
        (src1ValueType.IsLikelyFloat() || src1ValueType.IsLikelyUntaggedInt()) &&
        !(instrLoad->HasBailOutInfo() && (instrLoad->GetBailOutKind() == IR::BailOutIntOnly || instrLoad->GetBailOutKind() == IR::BailOutExpectingInteger));

    if (isNotInt)
    {
        // Known to be non-integer. If we are required to bail out on helper call, just re-jit.
        if (!doFloatToIntFastPath && bailOutOnHelper)
        {
            if(!GlobOpt::DoEliminateArrayAccessHelperCall(this->m_func))
            {
                // Array access helper call removal is already off for some reason. Prevent trying to rejit again
                // because it won't help and the same thing will happen again. Just abort jitting this function.
                if(PHASE_TRACE(Js::BailOutPhase, this->m_func))
                {
                    Output::Print(_u("    Aborting JIT because EliminateArrayAccessHelperCall is already off\n"));
                    Output::Flush();
                }
                throw Js::OperationAbortedException();
            }

            throw Js::RejitException(RejitReason::ArrayAccessHelperCallEliminationDisabled);
        }
    }
    else
    {
        // It could be an integer in this case.
        if (!isInt)
        {
            if(doFloatToIntFastPath)
            {
                labelFloat = IR::LabelInstr::New(Js::OpCode::Label, instrLoad->m_func, false);
            }
            else
            {
                helper = IR::LabelInstr::New(Js::OpCode::Label, instrLoad->m_func, true);
            }

            this->GenerateSmIntTest(src1, instrLoad, labelFloat ? labelFloat : helper);
        }

        instrLoad->InsertBefore(IR::Instr::New(Js::OpCode::MOV_TRUNC,
            dst->UseWithNewType(TyInt32, instrLoad->m_func),
            src1->UseWithNewType(TyInt32, instrLoad->m_func),
            instrLoad->m_func));

        if (!isInt)
        {
            // JMP $done
            done = instrLoad->GetOrCreateContinueLabel();
            instrLoad->InsertBefore(IR::BranchInstr::New(Js::OpCode::B, done, m_func));
        }
    }

    if (!isInt)
    {
        if(doFloatToIntFastPath)
        {
            if(labelFloat)
            {
                instrLoad->InsertBefore(labelFloat);
            }
            if(!helper)
            {
                helper = IR::LabelInstr::New(Js::OpCode::Label, instrLoad->m_func, true);
            }

            if(!done)
            {
                done = instrLoad->GetOrCreateContinueLabel();
            }
            IR::RegOpnd* floatOpnd = this->CheckFloatAndUntag(src1, instrLoad, helper);
            this->ConvertFloatToInt32(instrLoad->GetDst(), floatOpnd, helper, done, instrLoad);
        }

        // $helper:
        if (helper)
        {
            instrLoad->InsertBefore(helper);
        }
        if(instrLoad->HasBailOutInfo() && (instrLoad->GetBailOutKind() == IR::BailOutIntOnly || instrLoad->GetBailOutKind() == IR::BailOutExpectingInteger))
        {
            // Avoid bailout if we have a JavascriptNumber whose value is a signed 32-bit integer
            m_lowerer->LoadInt32FromUntaggedVar(instrLoad);

            // Need to bail out instead of calling a helper
            return true;
        }

        if (bailOutOnHelper)
        {
            Assert(labelBailOut);
            m_lowerer->InsertBranch(Js::OpCode::Br, labelBailOut, instrLoad);
            instrLoad->Remove();
        }
        else if (conversionFromObjectAllowed)
        {
            m_lowerer->LowerUnaryHelperMem(instrLoad, IR::HelperConv_ToInt32);
        }
        else
        {
            m_lowerer->LowerUnaryHelperMemWithBoolReference(instrLoad, IR::HelperConv_ToInt32_NoObjects, true /*useBoolForBailout*/);
        }
    }
    else
    {
        instrLoad->Remove();
    }

    return false;
}

void
LowererMD::ImmedSrcToReg(IR::Instr * instr, IR::Opnd * newOpnd, int srcNum)
{
    if (srcNum == 2)
    {
        instr->SetSrc2(newOpnd);
    }
    else
    {
        Assert(srcNum == 1);
        instr->SetSrc1(newOpnd);
    }

    switch (instr->m_opcode)
    {
    case Js::OpCode::LDIMM:
        instr->m_opcode = Js::OpCode::MOV;
        break;
    default:
        // Nothing to do (unless we have immed/reg variations for other instructions).
        break;
    }
}

IR::LabelInstr *
LowererMD::GetBailOutStackRestoreLabel(BailOutInfo * bailOutInfo, IR::LabelInstr * exitTargetInstr)
{
    return exitTargetInstr;
}

StackSym *
LowererMD::GetImplicitParamSlotSym(Js::ArgSlot argSlot)
{
    return GetImplicitParamSlotSym(argSlot, this->m_func);
}

StackSym *
LowererMD::GetImplicitParamSlotSym(Js::ArgSlot argSlot, Func * func)
{
    // For ARM, offset for implicit params always start at 0
    // TODO: Consider not to use the argSlot number for the param slot sym, which can
    // be confused with arg slot number from javascript
    StackSym * stackSym = StackSym::NewParamSlotSym(argSlot, func);
    func->SetArgOffset(stackSym, argSlot * MachPtr);
    func->SetHasImplicitParamLoad();
    return stackSym;
}

IR::LabelInstr *
LowererMD::EnsureEHEpilogLabel()
{
    if (this->m_func->m_epilogLabel)
    {
        return this->m_func->m_epilogLabel;
    }

    IR::Instr *exitInstr = this->m_func->m_exitInstr;
    IR::Instr *prevInstr = exitInstr->GetPrevRealInstrOrLabel();
    if (prevInstr->IsLabelInstr())
    {
        this->m_func->m_epilogLabel = prevInstr->AsLabelInstr();
        return prevInstr->AsLabelInstr();
    }
    IR::LabelInstr *labelInstr = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    exitInstr->InsertBefore(labelInstr);
    this->m_func->m_epilogLabel = labelInstr;
    return labelInstr;
}

// Helper method: inserts legalized assign for given srcOpnd into RegD0 in front of given instr in the following way:
//   dstReg = InsertMove srcOpnd
// Used to put args of inline built-in call into RegD0 and RegD1 before we call actual CRT function.
void LowererMD::GenerateAssignForBuiltinArg(RegNum dstReg, IR::Opnd* srcOpnd, IR::Instr* instr)
{
    IR::RegOpnd* tempDst = IR::RegOpnd::New(nullptr, dstReg, TyMachDouble, this->m_func);
    tempDst->m_isCallArg = true; // This is to make sure that lifetime of opnd is virtually extended until next CALL instr.
    Lowerer::InsertMove(tempDst, srcOpnd, instr);
}

// For given InlineMathXXX instr, generate the call to actual CRT function/CPU instr.
void LowererMD::GenerateFastInlineBuiltInCall(IR::Instr* instr, IR::JnHelperMethod helperMethod)
{
    switch (instr->m_opcode)
    {
    case Js::OpCode::InlineMathSqrt:
        // Sqrt maps directly to the VFP instruction.
        // src and dst are already float, all we need is just change the opcode and legalize.
        // Before:
        //      dst = InlineMathSqrt src1
        // After:
        //       <potential FSTR by legalizer if src1 is not a register>
        //       dst = FSQRT src1
        Assert(helperMethod == (IR::JnHelperMethod)0);
        Assert(instr->GetSrc2() == nullptr);
        instr->m_opcode = Js::OpCode::FSQRT;
        LegalizeMD::LegalizeInstr(instr);
        break;

    case Js::OpCode::InlineMathAbs:
        Assert(helperMethod == (IR::JnHelperMethod)0);
        return GenerateFastInlineBuiltInMathAbs(instr);

    case Js::OpCode::InlineMathFloor:
    case Js::OpCode::InlineMathCeil:
        Assert(helperMethod == (IR::JnHelperMethod)0);
        return GenerateFastInlineBuiltInMathFloorCeil(instr);

    case Js::OpCode::InlineMathRound:
        Assert(helperMethod == (IR::JnHelperMethod)0);
        return GenerateFastInlineBuiltInMathRound(instr);

    case Js::OpCode::InlineMathMin:
    case Js::OpCode::InlineMathMax:
        Assert(helperMethod == (IR::JnHelperMethod)0);
        return GenerateFastInlineBuiltInMathMinMax(instr);

    default:
        // Before:
        //      dst = <Built-in call> src1, src2
        // After:
        //       d0 = InsertMove src1
        //       lr = MOV helperAddr
        //            BLX lr
        //      dst = InsertMove call->dst (d0)

        // Src1
        AssertMsg(instr->GetDst()->IsFloat(), "Currently accepting only float args for math helpers -- dst.");
        AssertMsg(instr->GetSrc1()->IsFloat(), "Currently accepting only float args for math helpers -- src1.");
        AssertMsg(!instr->GetSrc2() || instr->GetSrc2()->IsFloat(), "Currently accepting only float args for math helpers -- src2.");
        this->GenerateAssignForBuiltinArg((RegNum)FIRST_FLOAT_REG, instr->UnlinkSrc1(), instr);

        // Src2
        if (instr->GetSrc2() != nullptr)
        {
            this->GenerateAssignForBuiltinArg((RegNum)(FIRST_FLOAT_REG + 1), instr->UnlinkSrc2(), instr);
        }

        // Call CRT.
        IR::RegOpnd* floatCallDst = IR::RegOpnd::New(nullptr, (RegNum)(FIRST_FLOAT_REG), TyMachDouble, this->m_func);   // Dst in d0.
        IR::Instr* floatCall = IR::Instr::New(Js::OpCode::BLR, floatCallDst, this->m_func);
        instr->InsertBefore(floatCall);

        // lr = MOV helperAddr
        //      BLX lr
        IR::AddrOpnd* targetAddr = IR::AddrOpnd::New((Js::Var)IR::GetMethodOriginalAddress(m_func->GetThreadContextInfo(), helperMethod), IR::AddrOpndKind::AddrOpndKindDynamicMisc, this->m_func);
        IR::RegOpnd *targetOpnd = IR::RegOpnd::New(nullptr, RegLR, TyMachPtr, this->m_func);
        IR::Instr   *movInstr   = IR::Instr::New(Js::OpCode::LDIMM, targetOpnd, targetAddr, this->m_func);
        targetOpnd->m_isCallArg = true;
        floatCall->SetSrc1(targetOpnd);
        floatCall->InsertBefore(movInstr);

        // Save the result.
        Lowerer::InsertMove(instr->UnlinkDst(), floatCall->GetDst(), instr);
        instr->Remove();
        break;
    }
}

void
LowererMD::GenerateFastInlineBuiltInMathAbs(IR::Instr *inlineInstr)
{
    IR::Opnd* src = inlineInstr->GetSrc1()->Copy(this->m_func);
    IR::Opnd* dst = inlineInstr->UnlinkDst();
    Assert(src);
    IR::Instr* tmpInstr;
    IRType srcType = src->GetType();

    IR::Instr* nextInstr = IR::LabelInstr::New(Js::OpCode::Label, m_func);
    IR::Instr* continueInstr = m_lowerer->LowerBailOnIntMin(inlineInstr);
    continueInstr->InsertAfter(nextInstr);
    if (srcType == IRType::TyInt32)
    {
        // Note: if execution gets so far, we always get (untagged) int32 here.
        Assert(src->IsRegOpnd());

        // CMP src, #0
        tmpInstr = IR::Instr::New(Js::OpCode::CMP, this->m_func);
        tmpInstr->SetSrc1(src);
        tmpInstr->SetSrc2(IR::IntConstOpnd::New(0, IRType::TyInt32, this->m_func));
        nextInstr->InsertBefore(tmpInstr);
        Legalize(tmpInstr);

        // dst = CSNEGPL dst, src, src
        tmpInstr = IR::Instr::New(Js::OpCode::CSNEGPL, dst, src, src, this->m_func);
        nextInstr->InsertBefore(tmpInstr);
        Legalize(tmpInstr);
    }
    else if (srcType == IRType::TyFloat64)
    {
        // FABS dst, src
        tmpInstr = IR::Instr::New(Js::OpCode::FABS, dst, src, this->m_func);
        nextInstr->InsertBefore(tmpInstr);
    }
    else
    {
        AssertMsg(FALSE, "GenerateFastInlineBuiltInMathAbs: unexpected type of the src!");
    }
}


void
LowererMD::GenerateFastInlineMathFround(IR::Instr* instr)
{
    // Note that this is fround, not round; this operation is to
    // round a double to Float32 precision.
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* dst = instr->GetDst();

    Assert(dst->IsFloat());
    Assert(src1->IsFloat());

    IRType srcType = src1->GetType();
    IRType dstType = dst->GetType();

    if (srcType == TyFloat32)
    {
        if (dstType == TyFloat32)
        {
            LowererMD::ChangeToAssign(instr);
        }
        else
        {
            Assert(dstType == TyFloat64);
            instr->m_opcode = Js::OpCode::FCVT;
            LegalizeMD::LegalizeInstr(instr);
        }
    }
    else
    {
        Assert(srcType == TyFloat64);
        if (dstType == TyFloat32)
        {
            instr->m_opcode = Js::OpCode::FCVT;
            LegalizeMD::LegalizeInstr(instr);
        }
        else
        {
            Assert(dstType == TyFloat64);
            IR::RegOpnd* tempOpnd = IR::RegOpnd::New(TyFloat32, instr->m_func);
            IR::Instr* shortener = IR::Instr::New(Js::OpCode::FCVT, tempOpnd, instr->UnlinkSrc1(), instr->m_func);
            instr->InsertBefore(shortener);
            instr->SetSrc1(tempOpnd);
            instr->m_opcode = Js::OpCode::FCVT;
            LegalizeMD::LegalizeInstr(instr);
        }
    }
}

void
LowererMD::GenerateFastInlineBuiltInMathRound(IR::Instr* instr)
{
    Assert(instr->GetDst()->IsInt32());

    IR::LabelInstr * doneLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);

    // Allocate an integer register for negative zero checks if needed
    IR::Opnd * negZeroReg = nullptr;
    if (instr->ShouldCheckForNegativeZero())
    {
        negZeroReg = IR::RegOpnd::New(TyInt64, this->m_func);
    }

    // FMOV floatOpnd, src
    IR::Opnd * src = instr->UnlinkSrc1();
    IR::RegOpnd* floatOpnd = IR::RegOpnd::New(TyFloat64, this->m_func);
    this->m_lowerer->InsertMove(floatOpnd, src, instr);

    IR::LabelInstr * bailoutLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);;
    bool sharedBailout = (instr->GetBailOutInfo()->bailOutInstr != instr) ? true : false;

    // FMOV_GEN negZeroReg, floatOpnd (note this is done before the 0.5 add below)
    if (negZeroReg)
    {
        instr->InsertBefore(IR::Instr::New(Js::OpCode::FMOV_GEN, negZeroReg, floatOpnd, instr->m_func));
    }

    // Add 0.5
    IR::Opnd * pointFive = IR::MemRefOpnd::New(m_func->GetThreadContextInfo()->GetDoublePointFiveAddr(), IRType::TyFloat64, this->m_func, IR::AddrOpndKindDynamicDoubleRef);
    this->m_lowerer->InsertAdd(false, floatOpnd, floatOpnd, pointFive, instr);

    // MSR FPSR, xzr
    IR::Instr* setFPSRInstr = IR::Instr::New(Js::OpCode::MSR_FPSR, instr->m_func);
    setFPSRInstr->SetSrc1(IR::RegOpnd::New(nullptr, RegZR, TyUint32, instr->m_func));
    instr->InsertBefore(setFPSRInstr);

    // FCVTM intOpnd, floatOpnd
    IR::Opnd * intOpnd = IR::RegOpnd::New(TyInt32, this->m_func);
    instr->InsertBefore(IR::Instr::New(Js::OpCode::FCVTM, intOpnd, floatOpnd, instr->m_func));

    // FCVTM would set FPSR.IOC (0th bit in FPSR) if the source cannot be represented within the destination register

    // MRS exceptReg, FPSR
    IR::Opnd * exceptReg = IR::RegOpnd::New(TyUint32, this->m_func);
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MRS_FPSR, exceptReg, instr->m_func));

    IR::LabelInstr* checkOverflowLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);

    // CBNZ intOpnd, done/checkOverflow
    IR::BranchInstr * cbnzInstr = cbnzInstr = IR::BranchInstr::New(Js::OpCode::CBNZ, checkOverflowLabel, instr->m_func);
    cbnzInstr->SetSrc1(intOpnd);
    instr->InsertBefore(cbnzInstr);

    if (negZeroReg)
    {
        // TBZ negZeroReg, 63
        IR::BranchInstr * tbzInstr = IR::BranchInstr::New(Js::OpCode::TBZ, doneLabel, instr->m_func);
        tbzInstr->SetSrc1(negZeroReg);
        tbzInstr->SetSrc2(IR::IntConstOpnd::New(63, TyMachReg, instr->m_func));
        instr->InsertBefore(tbzInstr);

        Lowerer::InsertBranch(LowererMD::MDUncondBranchOpcode, bailoutLabel, instr);
    }

    
    instr->InsertBefore(checkOverflowLabel);

    // TBZ exceptReg, #0, done
    IR::BranchInstr * tbzInstr = IR::BranchInstr::New(Js::OpCode::TBZ, doneLabel, instr->m_func);
    tbzInstr->SetSrc1(exceptReg);
    tbzInstr->SetSrc2(IR::IntConstOpnd::New(0, TyMachReg, instr->m_func));
    instr->InsertBefore(tbzInstr);

    IR::Opnd * dst = instr->UnlinkDst();
    instr->InsertAfter(doneLabel);
    if (!sharedBailout)
    {
        instr->InsertBefore(bailoutLabel);
    }

    // In case of a shared bailout, we should jump to the code that sets some data on the bailout record which is specific
    // to this bailout. Pass the bailoutLabel to GenerateFunction so that it may use the label as the collectRuntimeStatsLabel.
    this->m_lowerer->GenerateBailOut(instr, nullptr, nullptr, sharedBailout ? bailoutLabel : nullptr);

    // MOV dst, intOpnd
    IR::Instr* movInstr = IR::Instr::New(Js::OpCode::MOV, dst, intOpnd, this->m_func);
    doneLabel->InsertAfter(movInstr);
}

void
LowererMD::GenerateFastInlineBuiltInMathFloorCeil(IR::Instr* instr)
{
    Assert(instr->GetDst()->IsInt32());

    IR::LabelInstr * doneLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);

    // Allocate an integer register for negative zero checks if needed
    IR::Opnd * negZeroReg = nullptr;
    if (instr->ShouldCheckForNegativeZero())
    {
        negZeroReg = IR::RegOpnd::New(TyInt64, this->m_func);
    }

    // FMOV floatOpnd, src
    IR::Opnd * src = instr->UnlinkSrc1();
    IR::RegOpnd* floatOpnd = IR::RegOpnd::New(TyFloat64, this->m_func);
    this->m_lowerer->InsertMove(floatOpnd, src, instr);

    IR::LabelInstr * bailoutLabel = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);;
    bool sharedBailout = (instr->GetBailOutInfo()->bailOutInstr != instr) ? true : false;

    // MSR FPSR, xzr
    IR::Instr* setFPSRInstr = IR::Instr::New(Js::OpCode::MSR_FPSR, instr->m_func);
    setFPSRInstr->SetSrc1(IR::RegOpnd::New(nullptr, RegZR, TyUint32, instr->m_func));
    instr->InsertBefore(setFPSRInstr);

    // FMOV_GEN negZeroReg, floatOpnd (note this is done before the 0.5 add below)
    if (negZeroReg)
    {
        instr->InsertBefore(IR::Instr::New(Js::OpCode::FMOV_GEN, negZeroReg, floatOpnd, instr->m_func));
    }

    // FCVTM/FCVTP intOpnd, floatOpnd
    IR::Opnd * intOpnd = IR::RegOpnd::New(TyInt32, this->m_func);
    instr->InsertBefore(IR::Instr::New((instr->m_opcode == Js::OpCode::InlineMathCeil) ? Js::OpCode::FCVTP : Js::OpCode::FCVTM, intOpnd, floatOpnd, instr->m_func));

    // EOR negZeroReg, #0x8000000000000000
    if (negZeroReg)
    {
        instr->InsertBefore(IR::Instr::New(Js::OpCode::EOR, negZeroReg, negZeroReg, IR::IntConstOpnd::New(0x8000000000000000ULL, IRType::TyInt64, this->m_func), instr->m_func));
    }

    // FCVTM would set FPSR.IOC (0th bit in FPSR) if the source cannot be represented within the destination register

    // MRS exceptReg, FPSR
    IR::Opnd * exceptReg = IR::RegOpnd::New(TyUint32, this->m_func);
    instr->InsertBefore(IR::Instr::New(Js::OpCode::MRS_FPSR, exceptReg, instr->m_func));

    // CBZ negZeroReg, bailout
    if (negZeroReg)
    {
        IR::BranchInstr * cbzInstr = IR::BranchInstr::New(Js::OpCode::CBZ, bailoutLabel, instr->m_func);
        cbzInstr->SetSrc1(negZeroReg);
        instr->InsertBefore(cbzInstr);
    }

    // TBZ exceptReg, #0, done
    IR::BranchInstr * tbzInstr = IR::BranchInstr::New(Js::OpCode::TBZ, doneLabel, instr->m_func);
    tbzInstr->SetSrc1(exceptReg);
    tbzInstr->SetSrc2(IR::IntConstOpnd::New(0, TyMachReg, instr->m_func));
    instr->InsertBefore(tbzInstr);

    IR::Opnd * dst = instr->UnlinkDst();
    instr->InsertAfter(doneLabel);
    if(!sharedBailout)
    {
        instr->InsertBefore(bailoutLabel);
    }

    // In case of a shared bailout, we should jump to the code that sets some data on the bailout record which is specific
    // to this bailout. Pass the bailoutLabel to GenerateFunction so that it may use the label as the collectRuntimeStatsLabel.
    this->m_lowerer->GenerateBailOut(instr, nullptr, nullptr, sharedBailout ? bailoutLabel : nullptr);

    // MOV dst, intOpnd
    IR::Instr* movInstr = IR::Instr::New(Js::OpCode::MOV, dst, intOpnd, this->m_func);
    doneLabel->InsertAfter(movInstr);
}

void
LowererMD::GenerateFastInlineBuiltInMathMinMax(IR::Instr* instr)
{
    IR::Opnd* dst = instr->GetDst();

    if (dst->IsInt32())
    {
        IR::Opnd* src1 = instr->GetSrc1();
        IR::Opnd* src2 = instr->GetSrc2();

        // CMP src1, src2
        IR::Instr* cmpInstr = IR::Instr::New(Js::OpCode::CMP, instr->m_func);
        cmpInstr->SetSrc1(src1);
        cmpInstr->SetSrc2(src2);
        instr->InsertBefore(cmpInstr);
        Legalize(cmpInstr);

        // (min) CSELLT dst, src1, src2
        // (max) CSELLT dst, src2, src1
        IR::Opnd* op1 = (instr->m_opcode == Js::OpCode::InlineMathMin) ? src1 : src2;
        IR::Opnd* op2 = (instr->m_opcode == Js::OpCode::InlineMathMin) ? src2 : src1;
        IR::Instr * csellinstr = IR::Instr::New(Js::OpCode::CSELLT, dst, op1, op2, instr->m_func);
        instr->InsertBefore(csellinstr);
        Legalize(csellinstr);

        instr->Remove();
    }

    else if (dst->IsFloat64())
    {
        // (min) FMIN dst, src1, src2
        // (max) FMAX dst, src1, src2
        instr->m_opcode = (instr->m_opcode == Js::OpCode::InlineMathMin) ? Js::OpCode::FMIN : Js::OpCode::FMAX;
    }
}

IR::Instr *
LowererMD::LowerToFloat(IR::Instr *instr)
{
    switch (instr->m_opcode)
    {
    case Js::OpCode::Add_A:
        instr->m_opcode = Js::OpCode::FADD;
        break;

    case Js::OpCode::Sub_A:
        instr->m_opcode = Js::OpCode::FSUB;
        break;

    case Js::OpCode::Mul_A:
        instr->m_opcode = Js::OpCode::FMUL;
        break;

    case Js::OpCode::Div_A:
        instr->m_opcode = Js::OpCode::FDIV;
        break;

    case Js::OpCode::Neg_A:
        instr->m_opcode = Js::OpCode::FNEG;
        break;

    case Js::OpCode::BrEq_A:
    case Js::OpCode::BrNeq_A:
    case Js::OpCode::BrSrEq_A:
    case Js::OpCode::BrSrNeq_A:
    case Js::OpCode::BrGt_A:
    case Js::OpCode::BrGe_A:
    case Js::OpCode::BrLt_A:
    case Js::OpCode::BrLe_A:
    case Js::OpCode::BrNotEq_A:
    case Js::OpCode::BrNotNeq_A:
    case Js::OpCode::BrSrNotEq_A:
    case Js::OpCode::BrSrNotNeq_A:
    case Js::OpCode::BrNotGt_A:
    case Js::OpCode::BrNotGe_A:
    case Js::OpCode::BrNotLt_A:
    case Js::OpCode::BrNotLe_A:
        return this->LowerFloatCondBranch(instr->AsBranchInstr());

    default:
        Assume(UNREACHED);
    }

    LegalizeMD::LegalizeInstr(instr);
    return instr;
}

IR::BranchInstr *
LowererMD::LowerFloatCondBranch(IR::BranchInstr *instrBranch, bool ignoreNaN)
{
    IR::Instr *instr;
    Js::OpCode brOpcode = Js::OpCode::InvalidOpCode;
    bool addNaNCheck = false;

    Func * func = instrBranch->m_func;
    IR::Opnd *src1 = instrBranch->UnlinkSrc1();
    IR::Opnd *src2 = instrBranch->UnlinkSrc2();

    IR::Instr *instrCmp = IR::Instr::New(Js::OpCode::FCMP, func);
    instrCmp->SetSrc1(src1);
    instrCmp->SetSrc2(src2);
    instrBranch->InsertBefore(instrCmp);
    LegalizeMD::LegalizeInstr(instrCmp);

    switch (instrBranch->m_opcode)
    {
        case Js::OpCode::BrSrEq_A:
        case Js::OpCode::BrEq_A:
        case Js::OpCode::BrNotNeq_A:
        case Js::OpCode::BrSrNotNeq_A:
                brOpcode = Js::OpCode::BEQ;
                break;

        case Js::OpCode::BrNeq_A:
        case Js::OpCode::BrSrNeq_A:
        case Js::OpCode::BrSrNotEq_A:
        case Js::OpCode::BrNotEq_A:
                brOpcode = Js::OpCode::BNE;
                addNaNCheck = !ignoreNaN;   //Special check for BNE as it is set when the operands are unordered (NaN).
                break;

        case Js::OpCode::BrLe_A:
                brOpcode = Js::OpCode::BLS; //Can't use BLE as it is set when the operands are unordered (NaN).
                break;

        case Js::OpCode::BrLt_A:
                brOpcode = Js::OpCode::BCC; //Can't use BLT  as is set when the operands are unordered (NaN).
                break;

        case Js::OpCode::BrGe_A:
                brOpcode = Js::OpCode::BGE;
                break;

        case Js::OpCode::BrGt_A:
                brOpcode = Js::OpCode::BGT;
                break;

        case Js::OpCode::BrNotLe_A:
                brOpcode = Js::OpCode::BHI;
                break;

        case Js::OpCode::BrNotLt_A:
                brOpcode = Js::OpCode::BPL;
                break;

        case Js::OpCode::BrNotGe_A:
                brOpcode = Js::OpCode::BLT;
                break;

        case Js::OpCode::BrNotGt_A:
                brOpcode = Js::OpCode::BLE;
                break;

        default:
            Assert(false);
            break;
    }

    if (addNaNCheck)
    {
        instr = IR::BranchInstr::New(Js::OpCode::BVS, instrBranch->GetTarget(), func);
        instrBranch->InsertBefore(instr);
    }

    instr = IR::BranchInstr::New(brOpcode, instrBranch->GetTarget(), func);
    instrBranch->InsertBefore(instr);

    instrBranch->Remove();

    return instr->AsBranchInstr();
}

void
LowererMD::EmitIntToFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert)
{
    IR::Instr *instr;

    Assert(dst->IsRegOpnd() && dst->IsFloat64());
    Assert(src->IsRegOpnd() && src->IsInt32());

    // Convert to Float
    instr = IR::Instr::New(Js::OpCode::FCVT, dst, src, this->m_func);
    instrInsert->InsertBefore(instr);
}

void
LowererMD::EmitUIntToFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert)
{
    IR::Instr *instr;

    Assert(dst->IsRegOpnd() && dst->IsFloat64());
    Assert(src->IsRegOpnd() && src->IsUInt32());

    // Convert to Float
    instr = IR::Instr::New(Js::OpCode::FCVT, dst, src, this->m_func);
    instrInsert->InsertBefore(instr);
}

void LowererMD::ConvertFloatToInt32(IR::Opnd* intOpnd, IR::Opnd* floatOpnd, IR::LabelInstr * labelHelper, IR::LabelInstr * labelDone, IR::Instr * instrInsert)
{
    Assert(floatOpnd->IsFloat64());
    Assert(intOpnd->IsInt32());

    // VCVTS32F64 dst.i32, src.f64
    // Convert to int
    // ARM64_WORKITEM: On ARM32 this used the current rounding mode; here we are explicitly rounding toward zero -- is that ok?
    IR::Instr * instr = IR::Instr::New(Js::OpCode::FCVTZ, intOpnd, floatOpnd, this->m_func);
    instrInsert->InsertBefore(instr);
    Legalize(instr);

    this->CheckOverflowOnFloatToInt32(instrInsert, intOpnd, labelHelper, labelDone);
}

void
LowererMD::EmitIntToLong(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert)
{
    Assert(UNREACHED);
}

void
LowererMD::EmitUIntToLong(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert)
{
    Assert(UNREACHED);
}

void
LowererMD::EmitLongToInt(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert)
{
    Assert(UNREACHED);
}

void
LowererMD::CheckOverflowOnFloatToInt32(IR::Instr* instrInsert, IR::Opnd* intOpnd, IR::LabelInstr * labelHelper, IR::LabelInstr * labelDone)
{
    // Test for 0x80000000 or 0x7FFFFFFF

    // tmp = EOR src, 0x80000000;   gives 0 or -1 for overflow values
    // tmp = EOR_ASR31 tmp, tmp;    tmp = tmp ^ ((int32)tmp >> 31) -- converts -1 or 0 to 0
    // CBZ tmp, helper;             branch if tmp was -1 or 0
    // B done;

    IR::RegOpnd* tmp = IR::RegOpnd::New(TyInt32, this->m_func);

    IR::Instr* instr = IR::Instr::New(Js::OpCode::EOR, tmp, intOpnd, IR::IntConstOpnd::New(0x80000000, TyUint32, this->m_func, true), this->m_func);
    instrInsert->InsertBefore(instr);

    instr = IR::Instr::New(Js::OpCode::EOR_ASR31, tmp, tmp, tmp, this->m_func);
    instrInsert->InsertBefore(instr);

    // CBZ $helper
    instr = IR::BranchInstr::New(Js::OpCode::CBZ, labelHelper, this->m_func);
    instr->SetSrc1(tmp);
    instrInsert->InsertBefore(instr);

    // B $done
    instr = IR::BranchInstr::New(Js::OpCode::B, labelDone, this->m_func);
    instrInsert->InsertBefore(instr);
}

void
LowererMD::EmitFloatToInt(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert, IR::Instr * instrBailOut, IR::LabelInstr * labelBailOut)
{    
    IR::BailOutKind bailOutKind = IR::BailOutInvalid;
    if (instrBailOut && instrBailOut->HasBailOutInfo())
    {
        bailOutKind = instrBailOut->GetBailOutKind(); 
        if (bailOutKind & IR::BailOutOnArrayAccessHelperCall)
        {
            // Bail out instead of calling helper. If this is happening unconditionally, the caller should instead throw a rejit exception.
            Assert(labelBailOut);
            m_lowerer->InsertBranch(Js::OpCode::Br, labelBailOut, instrInsert);
            return;
        }
    }

    IR::LabelInstr *labelDone = IR::LabelInstr::New(Js::OpCode::Label, this->m_func);
    IR::LabelInstr *labelHelper = IR::LabelInstr::New(Js::OpCode::Label, this->m_func, true);
    IR::Instr *instr;

    ConvertFloatToInt32(dst, src, labelHelper, labelDone, instrInsert);

    // $Helper
    instrInsert->InsertBefore(labelHelper);

    instr = IR::Instr::New(Js::OpCode::Call, dst, this->m_func);
    instrInsert->InsertBefore(instr);

    if (BailOutInfo::IsBailOutOnImplicitCalls(bailOutKind))
    {
        _Analysis_assume_(instrBailOut != nullptr);
        instr = instr->ConvertToBailOutInstr(instrBailOut->GetBailOutInfo(), bailOutKind);
        if (instrBailOut->GetBailOutInfo()->bailOutInstr == instrBailOut)
        {
            IR::Instr * instrShare = instrBailOut->ShareBailOut();
            m_lowerer->LowerBailTarget(instrShare);
        }
    }

    // dst = ToInt32Core(src);
    LoadDoubleHelperArgument(instr, src);

    this->ChangeToHelperCall(instr, IR::HelperConv_ToInt32Core);

    // $Done
    instrInsert->InsertBefore(labelDone);
}

IR::Instr *
LowererMD::InsertConvertFloat64ToInt32(const RoundMode roundMode, IR::Opnd *const dst, IR::Opnd *const src, IR::Instr *const insertBeforeInstr)
{
    Assert(dst);
    Assert(dst->IsInt32());
    Assert(src);
    Assert(src->IsFloat64());
    Assert(insertBeforeInstr);

    // The caller is expected to check for overflow. To have that work be done automatically, use LowererMD::EmitFloatToInt.

    Func *const func = insertBeforeInstr->m_func;
    IR::AutoReuseOpnd autoReuseSrcPlusHalf;
    IR::Instr *instr = nullptr;

    switch (roundMode)
    {
        case RoundModeTowardInteger:
        case RoundModeHalfToEven:
        {
            // Conversion with rounding towards nearest integer is not supported by the architecture. Add 0.5 and do a
            // round-toward-zero conversion instead.
            IR::RegOpnd *const srcPlusHalf = IR::RegOpnd::New(TyFloat64, func);
            autoReuseSrcPlusHalf.Initialize(srcPlusHalf, func);
            Lowerer::InsertAdd(
                false /* needFlags */,
                srcPlusHalf,
                src,
                IR::MemRefOpnd::New(insertBeforeInstr->m_func->GetThreadContextInfo()->GetDoublePointFiveAddr(), TyFloat64, func,
                    IR::AddrOpndKindDynamicDoubleRef),
                insertBeforeInstr);

            instr = IR::Instr::New(LowererMD::MDConvertFloat64ToInt32Opcode(RoundModeTowardZero), dst, srcPlusHalf, func);

            insertBeforeInstr->InsertBefore(instr);
            LowererMD::Legalize(instr);
            return instr;
        }
        default:
            AssertMsg(0, "RoundMode not supported.");
            return nullptr;
    }
}

IR::Instr *
LowererMD::LoadFloatZero(IR::Opnd * opndDst, IR::Instr * instrInsert)
{
    Assert(opndDst->GetType() == TyFloat64);
    IR::Opnd * zero = IR::MemRefOpnd::New(instrInsert->m_func->GetThreadContextInfo()->GetDoubleZeroAddr(), TyFloat64, instrInsert->m_func, IR::AddrOpndKindDynamicDoubleRef);

    // Todo(magardn): Make sure the correct opcode is used for moving between float and non-float regs (FMOV_GEN)
    return Lowerer::InsertMove(opndDst, zero, instrInsert);
}

IR::Instr *
LowererMD::LoadFloatValue(IR::Opnd * opndDst, double value, IR::Instr * instrInsert)
{
    // Floating point zero is a common value to load.  Let's use a single memory location instead of allocating new memory for each.
    const bool isFloatZero = value == 0.0 && !Js::JavascriptNumber::IsNegZero(value); // (-0.0 == 0.0) yields true
    if (isFloatZero)
    {
        return LowererMD::LoadFloatZero(opndDst, instrInsert);
    }
    void * pValue = NativeCodeDataNewNoFixup(instrInsert->m_func->GetNativeCodeDataAllocator(), DoubleType<DataDesc_LowererMD_LoadFloatValue_Double>, value);
    IR::Opnd * opnd;
    if (instrInsert->m_func->IsOOPJIT())
    {
        int offset = NativeCodeData::GetDataTotalOffset(pValue);
        auto addressRegOpnd = IR::RegOpnd::New(TyMachPtr, instrInsert->m_func);

        Lowerer::InsertMove(
            addressRegOpnd,
            IR::MemRefOpnd::New(instrInsert->m_func->GetWorkItem()->GetWorkItemData()->nativeDataAddr, TyMachPtr, instrInsert->m_func, IR::AddrOpndKindDynamicNativeCodeDataRef),
            instrInsert);

        opnd = IR::IndirOpnd::New(addressRegOpnd, offset, TyMachDouble,
#if DBG
            NativeCodeData::GetDataDescription(pValue, instrInsert->m_func->m_alloc),
#endif
            instrInsert->m_func, true);
    }
    else
    {
        opnd = IR::MemRefOpnd::New((void*)pValue, TyMachDouble, instrInsert->m_func);
    }
    IR::Instr * instr = IR::Instr::New(Js::OpCode::FLDR, opndDst, opnd, instrInsert->m_func);
    instrInsert->InsertBefore(instr);
    LegalizeMD::LegalizeInstr(instr);
    return instr;
}

void LowererMD::GenerateFloatTest(IR::RegOpnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr* labelHelper, const bool checkForNullInLoopBody)
{
    if (opndSrc->GetValueType().IsFloat())
    {
        return;
    }

    // TST s1, floatTagReg
    IR::Opnd* floatTag = IR::IntConstOpnd::New(Js::FloatTag_Value, TyMachReg, this->m_func, /* dontEncode = */ true);
    IR::Instr* instr = IR::Instr::New(Js::OpCode::TST, this->m_func);
    instr->SetSrc1(opndSrc);
    instr->SetSrc2(floatTag);
    insertInstr->InsertBefore(instr);
    LegalizeMD::LegalizeInstr(instr);

    // BZ $helper
    instr = IR::BranchInstr::New(Js::OpCode::BEQ /* BZ */, labelHelper, this->m_func);
    insertInstr->InsertBefore(instr);
}

IR::RegOpnd* LowererMD::CheckFloatAndUntag(IR::RegOpnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr* labelHelper)
{
    IR::Opnd* floatTag = IR::IntConstOpnd::New(Js::FloatTag_Value, TyMachReg, this->m_func, /* dontEncode = */ true);

    // MOV floatTagReg, FloatTag_Value
    if (!opndSrc->GetValueType().IsFloat())
    {
        // TST s1, floatTagReg
        IR::Instr* instr = IR::Instr::New(Js::OpCode::TST, this->m_func);
        instr->SetSrc1(opndSrc);
        instr->SetSrc2(floatTag);
        insertInstr->InsertBefore(instr);
        LegalizeMD::LegalizeInstr(instr);

        // BZ $helper
        instr = IR::BranchInstr::New(Js::OpCode::BEQ /* BZ */, labelHelper, this->m_func);
        insertInstr->InsertBefore(instr);
    }

    IR::RegOpnd* untaggedFloat = IR::RegOpnd::New(TyMachReg, this->m_func);
    IR::Instr* instr = IR::Instr::New(Js::OpCode::EOR, untaggedFloat, opndSrc, floatTag, this->m_func);
    insertInstr->InsertBefore(instr);

    IR::RegOpnd *floatReg = IR::RegOpnd::New(TyMachDouble, this->m_func);
    instr = IR::Instr::New(Js::OpCode::FMOV_GEN, floatReg, untaggedFloat, this->m_func);
    insertInstr->InsertBefore(instr);
    return floatReg;
}

template <bool verify>
void
LowererMD::Legalize(IR::Instr *const instr, bool fPostRegAlloc)
{
    if (verify)
    {
        // NYI for the rest of legalization
        return;
    }
    LegalizeMD::LegalizeInstr(instr);
}

template void LowererMD::Legalize<false>(IR::Instr *const instr, bool fPostRegalloc);
#if DBG
template void LowererMD::Legalize<true>(IR::Instr *const instr, bool fPostRegalloc);
#endif

void
LowererMD::FinalLower()
{
    NoRecoverMemoryArenaAllocator tempAlloc(_u("BE-ARMFinalLower"), m_func->m_alloc->GetPageAllocator(), Js::Throw::OutOfMemory);
    EncodeReloc *pRelocList = nullptr;

    size_t totalJmpTableSizeInBytes = 0;

    uintptr_t instrOffset = 0;
    FOREACH_INSTR_BACKWARD_EDITING_IN_RANGE(instr, instrPrev, this->m_func->m_tailInstr, this->m_func->m_headInstr)
    {
        if (instr->IsLowered() == false)
        {
            if (instr->IsLabelInstr())
            {
                //This is not the real set, Real offset gets set in encoder.
                IR::LabelInstr *labelInstr = instr->AsLabelInstr();
                labelInstr->SetOffset(instrOffset);
            }

            switch (instr->m_opcode)
            {
            case Js::OpCode::Ret:
                instr->Remove();
                break;
            case Js::OpCode::Leave:
                Assert(this->m_func->DoOptimizeTry() && !this->m_func->IsLoopBodyInTry());
                instrPrev = m_lowerer->LowerLeave(instr, instr->AsBranchInstr()->GetTarget(), true /*fromFinalLower*/);
                break;
            }
        }
        else
        {
            instrOffset = instrOffset + MachMaxInstrSize;

            if (instr->IsBranchInstr())
            {
                IR::BranchInstr *branchInstr = instr->AsBranchInstr();

                if (branchInstr->IsMultiBranch())
                {
                    Assert(instr->GetSrc1() && instr->GetSrc1()->IsRegOpnd());
                    IR::MultiBranchInstr * multiBranchInstr = instr->AsBranchInstr()->AsMultiBrInstr();

                    if (multiBranchInstr->m_isSwitchBr &&
                        (multiBranchInstr->m_kind == IR::MultiBranchInstr::IntJumpTable || multiBranchInstr->m_kind == IR::MultiBranchInstr::SingleCharStrJumpTable))
                    {
                        BranchJumpTableWrapper * branchJumpTableWrapper = multiBranchInstr->GetBranchJumpTable();
                        totalJmpTableSizeInBytes += (branchJumpTableWrapper->tableSize * sizeof(void*));

                        // instrOffset is relative to the end of the function. Jump tables come after the function and so would result in negative offsets. label offsets
                        // are unsigned so instead give jump table lables offsets relative to the end of the jump table section.
                        branchJumpTableWrapper->labelInstr->SetOffset(totalJmpTableSizeInBytes);
                    }
                }
                else if (!LowererMD::IsUnconditionalBranch(branchInstr)) //Ignore other direct branches
                {
                    uintptr_t targetOffset = branchInstr->GetTarget()->GetOffset();

                    if (targetOffset != 0)
                    {
                        // this is forward reference
                        if (LegalizeMD::LegalizeDirectBranch(branchInstr, instrOffset))
                        {
                            //There might be an instruction inserted for legalizing conditional branch
                            instrOffset = instrOffset + MachMaxInstrSize;
                        }
                    }
                    else
                    {
                        EncodeReloc::New(&pRelocList, RelocTypeBranch19, (BYTE*)instrOffset, branchInstr, &tempAlloc);
                        //Assume this is a backward long branch, we shall fix up after complete pass, be conservative here
                        instrOffset = instrOffset + MachMaxInstrSize;
                    }
                }
            }
            else if (LowererMD::IsAssign(instr) || instr->m_opcode == Js::OpCode::LEA || instr->m_opcode == Js::OpCode::LDARGOUTSZ || instr->m_opcode == Js::OpCode::REM)
            {
                // Cleanup spill code
                // INSTR_BACKWARD_EDITING_IN_RANGE implies that next loop iteration will use instrPrev (instr->m_prev computed before entering current loop iteration).
                IR::Instr* instrNext = instr->m_next;
                bool canExpand = this->FinalLowerAssign(instr);

                if (canExpand)
                {
                    uint32 expandedInstrCount = 0;   // The number of instrs the LDIMM expands into.
                    FOREACH_INSTR_IN_RANGE(instrCount, instrPrev->m_next, instrNext)
                    {
                        ++expandedInstrCount;
                    }
                    NEXT_INSTR_IN_RANGE;
                    Assert(expandedInstrCount > 0);

                    // Adjust the offset for expanded instrs.
                    instrOffset += (expandedInstrCount - 1) * MachMaxInstrSize;    // We already accounted for one MachMaxInstrSize.
                }
            }

            if (instr->m_opcode == Js::OpCode::ADR)
            {
                IR::LabelInstr* label = instr->GetSrc1()->AsLabelOpnd()->GetLabel();
                if (label->GetOffset() != 0 && !label->m_isDataLabel)
                {
                    // this is forward reference
                    if (LegalizeMD::LegalizeAdrOffset(instr, instrOffset))
                    {
                        //Additional instructions were inserted.
                        instrOffset = instrOffset + MachMaxInstrSize * 2;
                    }
                }
                else
                {
                    EncodeReloc::New(&pRelocList, RelocTypeLabelAdr, (BYTE*)instrOffset, instr, &tempAlloc);
                    //Assume this is a backward long branch, we shall fix up after complete pass, be conservative here
                    instrOffset = instrOffset + MachMaxInstrSize * 2;
                }
            }
        }
    } NEXT_INSTR_BACKWARD_EDITING_IN_RANGE;

    //Fixup all the backward branches
    for (EncodeReloc *reloc = pRelocList; reloc; reloc = reloc->m_next)
    {
        uintptr_t relocAddress = (uintptr_t)reloc->m_consumerOffset;

        switch (reloc->m_relocType)
        {
        case RelocTypeBranch19:
            AssertMsg(relocAddress < reloc->m_relocInstr->AsBranchInstr()->GetTarget()->GetOffset(), "Only backward branches require fixup");
            LegalizeMD::LegalizeDirectBranch(reloc->m_relocInstr->AsBranchInstr(), relocAddress);
            break;

        case RelocTypeLabelAdr:
        {
            IR::LabelInstr* label = reloc->m_relocInstr->GetSrc1()->AsLabelOpnd()->GetLabel();
            if (label->m_isDataLabel)
            {
                uintptr_t dataOffset;
                if (label == m_func->GetFuncStartLabel())
                {
                    dataOffset = instrOffset - relocAddress;
                }
                else if (label == m_func->GetFuncEndLabel())
                {
                    dataOffset = relocAddress;
                }
                else
                {
                    Assert(label->GetOffset() != 0);

                    // jump table label offsets are relative to the end of the jump table area.
                    dataOffset = relocAddress + totalJmpTableSizeInBytes - label->GetOffset();

                    // PC is a union with offset. Encoder expects this to be nullptr for jump table labels.
                    label->SetPC(nullptr);
                }

                LegalizeMD::LegalizeDataAdr(reloc->m_relocInstr, dataOffset);
                break;
            }

            AssertMsg(relocAddress < label->GetOffset(), "Only backward branches require fixup");
            LegalizeMD::LegalizeAdrOffset(reloc->m_relocInstr, relocAddress);
            break;
        }
        default:
            Assert(false);
        }
    }
}

// Returns true, if and only if the assign may expand into multiple instrs.
bool
LowererMD::FinalLowerAssign(IR::Instr * instr)
{
    if (instr->m_opcode == Js::OpCode::LDIMM)
    {
        LegalizeMD::LegalizeInstr(instr);

        // LDIMM can expand into up to 4 instructions when the immediate is more than 16 bytes,
        // it can also expand into multiple different no-op (normally MOV) instrs when we obfuscate it, which is randomly.
        return true;
    }
    else if (EncoderMD::IsLoad(instr) || instr->m_opcode == Js::OpCode::LEA)
    {
        Assert(instr->GetDst()->IsRegOpnd());
        if (!instr->GetSrc1()->IsRegOpnd())
        {
            LegalizeMD::LegalizeSrc(instr, instr->GetSrc1(), 1);
            return true;
        }
        instr->m_opcode = instr->GetSrc1()->IsFloat() ? Js::OpCode::FMOV : Js::OpCode::MOV;
    }
    else if (EncoderMD::IsStore(instr))
    {
        Assert(instr->GetSrc1()->IsRegOpnd());
        if (!instr->GetDst()->IsRegOpnd())
        {
            LegalizeMD::LegalizeDst(instr);
            return true;
        }
        instr->m_opcode = instr->GetDst()->IsFloat() ? Js::OpCode::FMOV : Js::OpCode::MOV;
    }
    else if (instr->m_opcode == Js::OpCode::LDARGOUTSZ)
    {
        Assert(instr->GetDst()->IsRegOpnd());
        Assert((instr->GetSrc1() == nullptr) && (instr->GetSrc2() == nullptr));
        // dst = LDARGOUTSZ
        // This loads the function's arg out area size into the dst operand. We need a pseudo-op,
        // because we generate the instruction during Lower but don't yet know the value of the constant it needs
        // to load. Change it to the appropriate LDIMM here.
        uint32 argOutSize = UInt32Math::Mul(this->m_func->m_argSlotsForFunctionsCalled, MachRegInt, Js::Throw::OutOfMemory);
        instr->SetSrc1(IR::IntConstOpnd::New(argOutSize, TyMachReg, this->m_func));
        instr->m_opcode = Js::OpCode::LDIMM;
        LegalizeMD::LegalizeInstr(instr);
        return true;
    }
    else if (instr->m_opcode == Js::OpCode::REM)
    {
        IR::Opnd* dst = instr->GetDst();
        IR::Opnd* src1 = instr->GetSrc1();
        IR::Opnd* src2 = instr->GetSrc2();

        Assert(src1->IsRegOpnd());
        Assert(src2->IsRegOpnd());

        RegNum dstReg = dst->AsRegOpnd()->GetReg();

        if (dstReg == src1->AsRegOpnd()->GetReg() || dstReg == src2->AsRegOpnd()->GetReg())
        {
            Assert(src1->AsRegOpnd()->GetReg() != SCRATCH_REG);
            Assert(src2->AsRegOpnd()->GetReg() != SCRATCH_REG);
            Assert(src1->GetType() == src2->GetType());

            // r17 = SDIV src1, src2
            IR::RegOpnd *regScratch = IR::RegOpnd::New(nullptr, SCRATCH_REG, src1->GetType(), instr->m_func);
            IR::Instr *insertInstr = IR::Instr::New(Js::OpCode::SDIV, regScratch, src1, src2, instr->m_func);
            instr->InsertBefore(insertInstr);

            // r17 = MSUB src1, src2, r17 (r17 = src1 - src2 * r17)
            insertInstr = IR::Instr::New(Js::OpCode::MSUB, regScratch, src1, src2, instr->m_func);
            instr->InsertBefore(insertInstr);

            // mov dst, r17
            insertInstr = IR::Instr::New(dst->IsFloat() ? Js::OpCode::FMOV : Js::OpCode::MOV, dst, regScratch, instr->m_func);
            instr->InsertBefore(insertInstr);
            instr->Remove();
        }
        else
        {
            // dst = SDIV src1, src2
            IR::Instr *divInstr = IR::Instr::New(Js::OpCode::SDIV, dst, src1, src2, instr->m_func);
            instr->InsertBefore(divInstr);

            // dst = MSUB src1, src2, dst (dst = src1 - src2 * dst)
            instr->m_opcode = Js::OpCode::MSUB;
        }
        return true;
    }

    return false;
}
IR::Opnd *
LowererMD::GenerateArgOutForStackArgs(IR::Instr* callInstr, IR::Instr* stackArgsInstr)
{
    return this->m_lowerer->GenerateArgOutForStackArgs(callInstr, stackArgsInstr);
}

IR::Instr *
LowererMD::LowerDivI4AndBailOnReminder(IR::Instr * instr, IR::LabelInstr * bailOutLabel)
{
    //      result = SDIV numerator, denominator
    //   mulResult = MUL result, denominator
    //               CMP mulResult, numerator
    //               BNE bailout
    //               <Caller insert more checks here>
    //         dst = MOV result                             <-- insertBeforeInstr


    instr->m_opcode = Js::OpCode::SDIV;

    // delay assigning to the final dst.
    IR::Instr * sinkedInstr = instr->SinkDst(Js::OpCode::MOV);
    LegalizeMD::LegalizeInstr(instr);
    LegalizeMD::LegalizeInstr(sinkedInstr);

    IR::Opnd * resultOpnd = instr->GetDst();
    IR::Opnd * numerator = instr->GetSrc1();
    IR::Opnd * denominatorOpnd = instr->GetSrc2();

    // Insert all check before the assignment to the actual
    IR::Instr * insertBeforeInstr = instr->m_next;

    // Jump to bailout if the reminder is not 0 (or the divResult * denominator is not same as the numerator)
    IR::RegOpnd * mulResult = IR::RegOpnd::New(TyInt32, m_func);
    IR::Instr * mulInstr = IR::Instr::New(Js::OpCode::MUL, mulResult, resultOpnd, denominatorOpnd, m_func);
    insertBeforeInstr->InsertBefore(mulInstr);
    LegalizeMD::LegalizeInstr(mulInstr);

    this->m_lowerer->InsertCompareBranch(mulResult, numerator, Js::OpCode::BrNeq_A, bailOutLabel, insertBeforeInstr);
    return insertBeforeInstr;
}

void
LowererMD::LowerInlineSpreadArgOutLoop(IR::Instr *callInstr, IR::RegOpnd *indexOpnd, IR::RegOpnd *arrayElementsStartOpnd)
{
    this->m_lowerer->LowerInlineSpreadArgOutLoopUsingRegisters(callInstr, indexOpnd, arrayElementsStartOpnd);
}

void
LowererMD::LowerTypeof(IR::Instr* typeOfInstr)
{
    Func * func = typeOfInstr->m_func;
    IR::Opnd * src1 = typeOfInstr->GetSrc1();
    IR::Opnd * dst = typeOfInstr->GetDst();
    Assert(src1->IsRegOpnd() && dst->IsRegOpnd());
    IR::LabelInstr * helperLabel = IR::LabelInstr::New(Js::OpCode::Label, func, true);
    IR::LabelInstr * taggedIntLabel = IR::LabelInstr::New(Js::OpCode::Label, func);
    IR::LabelInstr * doneLabel = IR::LabelInstr::New(Js::OpCode::Label, func);

    // MOV typeDisplayStringsArray, &javascriptLibrary->typeDisplayStrings
    IR::RegOpnd * typeDisplayStringsArrayOpnd = IR::RegOpnd::New(TyMachPtr, func);
    m_lowerer->InsertMove(typeDisplayStringsArrayOpnd, IR::AddrOpnd::New((BYTE*)m_func->GetScriptContextInfo()->GetLibraryAddr() + Js::JavascriptLibrary::GetTypeDisplayStringsOffset(), IR::AddrOpndKindConstantAddress, this->m_func), typeOfInstr);

    GenerateObjectTest(src1, typeOfInstr, taggedIntLabel);

    // MOV typeRegOpnd, [src1 + offset(Type)]
    IR::RegOpnd * typeRegOpnd = IR::RegOpnd::New(TyMachReg, func);
    m_lowerer->InsertMove(typeRegOpnd,
        IR::IndirOpnd::New(src1->AsRegOpnd(), Js::RecyclableObject::GetOffsetOfType(), TyMachReg, func),
        typeOfInstr);

    IR::LabelInstr * falsyLabel = IR::LabelInstr::New(Js::OpCode::Label, func);
    m_lowerer->GenerateFalsyObjectTest(typeOfInstr, typeRegOpnd, falsyLabel);

    // <$not falsy>
    //   MOV typeId, TypeIds_Object
    //   MOV objTypeId, [typeRegOpnd + offsetof(typeId)]
    //   CMP objTypeId, TypeIds_Limit                                      /*external object test*/
    //   BCS $externalObjectLabel
    //   MOV typeId, objTypeId
    // $loadTypeDisplayStringLabel:
    //   MOV dst, typeDisplayStrings[typeId]
    //   TEST dst, dst
    //   BEQ $helper
    //   B $done
    IR::RegOpnd * typeIdOpnd = IR::RegOpnd::New(TyUint32, func);
    m_lowerer->InsertMove(typeIdOpnd, IR::IntConstOpnd::New(Js::TypeIds_Object, TyUint32, func), typeOfInstr);

    IR::RegOpnd * objTypeIdOpnd = IR::RegOpnd::New(TyUint32, func);
    m_lowerer->InsertMove(objTypeIdOpnd, IR::IndirOpnd::New(typeRegOpnd, Js::Type::GetOffsetOfTypeId(), TyInt32, func), typeOfInstr);

    IR::LabelInstr * loadTypeDisplayStringLabel = IR::LabelInstr::New(Js::OpCode::Label, func);
    m_lowerer->InsertCompareBranch(objTypeIdOpnd, IR::IntConstOpnd::New(Js::TypeIds_Limit, TyUint32, func), Js::OpCode::BrGe_A, true /*unsigned*/, loadTypeDisplayStringLabel, typeOfInstr);
    
    m_lowerer->InsertMove(typeIdOpnd, objTypeIdOpnd, typeOfInstr);
    typeOfInstr->InsertBefore(loadTypeDisplayStringLabel);

    if (dst->IsEqual(src1))
    {
        ChangeToAssign(typeOfInstr->HoistSrc1(Js::OpCode::Ld_A));
    }
    m_lowerer->InsertMove(dst, IR::IndirOpnd::New(typeDisplayStringsArrayOpnd, typeIdOpnd, this->GetDefaultIndirScale(), TyMachPtr, func), typeOfInstr);
    m_lowerer->InsertTestBranch(dst, dst, Js::OpCode::BrEq_A, helperLabel, typeOfInstr);

    m_lowerer->InsertBranch(Js::OpCode::Br, doneLabel, typeOfInstr);

    // $taggedInt:
    //   MOV dst, typeDisplayStrings[TypeIds_Number]
    //   B $done
    typeOfInstr->InsertBefore(taggedIntLabel);
    m_lowerer->InsertMove(dst, IR::IndirOpnd::New(typeDisplayStringsArrayOpnd, Js::TypeIds_Number * sizeof(Js::Var), TyMachPtr, func), typeOfInstr);
    m_lowerer->InsertBranch(Js::OpCode::Br, doneLabel, typeOfInstr);

    // $falsy:
    //   MOV dst, "undefined"
    //   B $done
    typeOfInstr->InsertBefore(falsyLabel);
    IR::Opnd * undefinedDisplayStringOpnd = IR::IndirOpnd::New(typeDisplayStringsArrayOpnd, Js::TypeIds_Undefined, TyMachPtr, func);
    m_lowerer->InsertMove(dst, undefinedDisplayStringOpnd, typeOfInstr);
    m_lowerer->InsertBranch(Js::OpCode::Br, doneLabel, typeOfInstr);

    // $helper
    //   CALL OP_TypeOf
    // $done
    typeOfInstr->InsertBefore(helperLabel);
    typeOfInstr->InsertAfter(doneLabel);
    m_lowerer->LowerUnaryHelperMem(typeOfInstr, IR::HelperOp_Typeof);
}

void
LowererMD::InsertObjectPoison(IR::Opnd* poisonedOpnd, IR::BranchInstr* branchInstr, IR::Instr* insertInstr, bool isForStore)
{
    if ((isForStore && CONFIG_FLAG_RELEASE(PoisonObjectsForStores)) || (!isForStore && CONFIG_FLAG_RELEASE(PoisonObjectsForLoads)))
    {
        Js::OpCode opcode;
        if (branchInstr->m_opcode == Js::OpCode::BNE)
        {
            opcode = Js::OpCode::CSELEQ;
        }
        else
        {
            AssertOrFailFastMsg(branchInstr->m_opcode == Js::OpCode::BEQ, "Unexpected branch type in InsertObjectPoison preceeding instruction");
            opcode = Js::OpCode::CSELNE;
        }
        AssertOrFailFast(branchInstr->m_prev->m_opcode == Js::OpCode::SUBS || branchInstr->m_prev->m_opcode == Js::OpCode::ANDS);

        IR::RegOpnd* regZero = IR::RegOpnd::New(nullptr, RegZR, TyMachPtr, insertInstr->m_func);
        IR::Instr* csel = IR::Instr::New(opcode, poisonedOpnd, poisonedOpnd, regZero, insertInstr->m_func);
        insertInstr->InsertBefore(csel);
    }
}

IR::BranchInstr*
LowererMD::InsertMissingItemCompareBranch(IR::Opnd* compareSrc, IR::Opnd* missingItemOpnd, Js::OpCode opcode, IR::LabelInstr* target, IR::Instr* insertBeforeInstr)
{
    Assert(compareSrc->IsFloat64() && missingItemOpnd->IsUint64());

    IR::Opnd * compareSrcUint64Opnd = IR::RegOpnd::New(TyUint64, m_func);
    if (compareSrc->IsRegOpnd())
    {
        IR::Instr * movDoubleToUint64Instr = IR::Instr::New(Js::OpCode::FMOV_GEN, compareSrcUint64Opnd, compareSrc, insertBeforeInstr->m_func);
        insertBeforeInstr->InsertBefore(movDoubleToUint64Instr);
    }
    else if (compareSrc->IsIndirOpnd())
    {
        compareSrcUint64Opnd = compareSrc->UseWithNewType(TyUint64, m_func);
    }

    return m_lowerer->InsertCompareBranch(compareSrcUint64Opnd, missingItemOpnd, opcode, target, insertBeforeInstr);
}

#if DBG
//
// Helps in debugging of fast paths.
//
void LowererMD::GenerateDebugBreak( IR::Instr * insertInstr )
{
    IR::Instr *int3 = IR::Instr::New(Js::OpCode::DEBUGBREAK, insertInstr->m_func);
    insertInstr->InsertBefore(int3);
}
#endif

#ifdef _CONTROL_FLOW_GUARD
void
LowererMD::GenerateCFGCheck(IR::Opnd * entryPointOpnd, IR::Instr * insertBeforeInstr)
{
    bool useJITTrampoline = CONFIG_FLAG(UseJITTrampoline);
    IR::LabelInstr * callLabelInstr = nullptr;
    uintptr_t jitThunkStartAddress = NULL;

    if (useJITTrampoline)
    {
#if ENABLE_OOP_NATIVE_CODEGEN
        if (m_func->IsOOPJIT())
        {
            OOPJITThunkEmitter * jitThunkEmitter = m_func->GetOOPThreadContext()->GetJITThunkEmitter();
            jitThunkStartAddress = jitThunkEmitter->EnsureInitialized();
        }
        else
#endif
        {
            InProcJITThunkEmitter * jitThunkEmitter = m_func->GetInProcThreadContext()->GetJITThunkEmitter();
            jitThunkStartAddress = jitThunkEmitter->EnsureInitialized();
        }
        if (jitThunkStartAddress)
        {
            uintptr_t endAddressOfSegment = jitThunkStartAddress + InProcJITThunkEmitter::TotalThunkSize;
            Assert(endAddressOfSegment > jitThunkStartAddress);
            // Generate instructions for local Pre-Reserved Segment Range check

            IR::AddrOpnd * endAddressOfSegmentConstOpnd = IR::AddrOpnd::New(endAddressOfSegment, IR::AddrOpndKindDynamicMisc, m_func);
            IR::RegOpnd *resultOpnd = IR::RegOpnd::New(TyMachReg, this->m_func);

            callLabelInstr = IR::LabelInstr::New(Js::OpCode::Label, m_func);
            IR::LabelInstr * cfgLabelInstr = IR::LabelInstr::New(Js::OpCode::Label, m_func, true);

            // resultOpnd = SUB endAddressOfSegmentConstOpnd, entryPointOpnd
            // CMP resultOpnd, TotalThunkSize
            // BHS $cfgLabel
            // AND entryPointOpnd,  ~(ThunkSize-1)
            // JMP $callLabel
            m_lowerer->InsertSub(false, resultOpnd, endAddressOfSegmentConstOpnd, entryPointOpnd, insertBeforeInstr);
            m_lowerer->InsertCompareBranch(resultOpnd, IR::IntConstOpnd::New(InProcJITThunkEmitter::TotalThunkSize, TyMachReg, m_func, true), Js::OpCode::BrGe_A, true, cfgLabelInstr, insertBeforeInstr);
            m_lowerer->InsertAnd(entryPointOpnd, entryPointOpnd, IR::IntConstOpnd::New(InProcJITThunkEmitter::ThunkAlignmentMask, TyMachReg, m_func, true), insertBeforeInstr);
            m_lowerer->InsertBranch(Js::OpCode::Br, callLabelInstr, insertBeforeInstr);

            insertBeforeInstr->InsertBefore(cfgLabelInstr);
        }
    }
    //MOV  x15, entryPoint
    IR::RegOpnd * entryPointRegOpnd = IR::RegOpnd::New(nullptr, RegR15, TyMachReg, this->m_func);
    entryPointRegOpnd->m_isCallArg = true;
    IR::Instr *movInstrEntryPointToRegister = Lowerer::InsertMove(entryPointRegOpnd, entryPointOpnd, insertBeforeInstr);

    //Generate CheckCFG CALL here
    IR::HelperCallOpnd *cfgCallOpnd = IR::HelperCallOpnd::New(IR::HelperGuardCheckCall, this->m_func);
    IR::Instr* cfgCallInstr = IR::Instr::New(Js::OpCode::BLR, this->m_func);
    this->m_func->SetHasCallsOnSelfAndParents();

    //mov x16, __guard_check_icall_fptr
    IR::RegOpnd *targetOpnd = IR::RegOpnd::New(nullptr, RegR16, TyMachPtr, this->m_func);
    IR::Instr   *movInstr = Lowerer::InsertMove(targetOpnd, cfgCallOpnd, insertBeforeInstr);
    Legalize(movInstr);

    //call x16
    cfgCallInstr->SetSrc1(targetOpnd);

    //CALL cfg(x15)
    insertBeforeInstr->InsertBefore(cfgCallInstr);

    if (jitThunkStartAddress)
    {
        Assert(callLabelInstr);
        if (CONFIG_FLAG(ForceJITCFGCheck))
        {
            // Always generate CFG check to make sure that the address is still valid
            movInstrEntryPointToRegister->InsertBefore(callLabelInstr);
        }
        else
        {
            insertBeforeInstr->InsertBefore(callLabelInstr);
        }
    }
}
#endif
