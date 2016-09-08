//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"
#include "ByteCode/OpCodeUtilAsmJs.h"

void
IRBuilderAsmJs::Build()
{
    m_funcAlloc = m_func->m_alloc;

    NoRecoverMemoryJitArenaAllocator localAlloc(_u("BE-IRBuilder"), m_funcAlloc->GetPageAllocator(), Js::Throw::OutOfMemory);
    m_tempAlloc = &localAlloc;

    uint32 offset;
    uint32 statementIndex = m_statementReader.GetStatementIndex();

    m_argStack = JitAnew(m_tempAlloc, SList<IR::Instr *>, m_tempAlloc);
    m_tempList = JitAnew(m_tempAlloc, SList<IR::Instr *>, m_tempAlloc);
    m_argOffsetStack = JitAnew(m_tempAlloc, SList<int32>, m_tempAlloc);

    m_branchRelocList = JitAnew(m_tempAlloc, SList<BranchReloc *>, m_tempAlloc);

    m_switchBuilder.Init(m_func, m_tempAlloc, true);

    m_firstVarConst = 0;

    m_firstIntConst = AsmJsRegSlots::RegCount + m_firstVarConst;
    m_firstFloatConst = m_asmFuncInfo->GetIntConstCount() + m_firstIntConst;
    m_firstDoubleConst = m_asmFuncInfo->GetFloatConstCount() + m_firstFloatConst;


    m_firstSimdConst = m_asmFuncInfo->GetDoubleConstCount() + m_firstDoubleConst;
    m_firstIntVar = m_asmFuncInfo->GetSimdConstCount() + m_firstSimdConst;


    m_firstFloatVar = m_asmFuncInfo->GetIntVarCount() + m_firstIntVar;
    m_firstDoubleVar = m_asmFuncInfo->GetFloatVarCount() + m_firstFloatVar;

    m_firstSimdVar = m_asmFuncInfo->GetDoubleVarCount() + m_firstDoubleVar;
    m_firstIntTemp = m_asmFuncInfo->GetSimdVarCount() + m_firstSimdVar;

    m_firstFloatTemp = m_asmFuncInfo->GetIntTmpCount() + m_firstIntTemp;
    m_firstDoubleTemp = m_asmFuncInfo->GetFloatTmpCount() + m_firstFloatTemp;


    m_firstSimdTemp = m_asmFuncInfo->GetDoubleTmpCount() + m_firstDoubleTemp;
    m_firstIRTemp = m_asmFuncInfo->GetSimdTmpCount() + m_firstSimdTemp;

    m_simdOpcodesMap = JitAnewArrayZ(m_tempAlloc, Js::OpCode, Js::Simd128AsmJsOpcodeCount());
    {

#define MACRO_SIMD(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...) m_simdOpcodesMap[(uint32)(Js::OpCodeAsmJs::opcode - Js::OpCodeAsmJs::Simd128_Start)] = Js::OpCode::opcode;

#define MACRO_SIMD_WMS(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...) MACRO_SIMD(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr)

        // append extended opcodes
#define MACRO_SIMD_EXTEND(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...) \
    m_simdOpcodesMap[(uint32)(Js::OpCodeAsmJs::opcode - Js::OpCodeAsmJs::Simd128_Start_Extend) + (Js::OpCodeAsmJs::Simd128_End - Js::OpCodeAsmJs::Simd128_Start + 1)] = Js::OpCode::opcode;

#define MACRO_SIMD_EXTEND_WMS(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...) MACRO_SIMD_EXTEND(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr)

#include "ByteCode/OpCodesSimd.h"

    }

    // we will be using lower space for type specialized syms, so bump up where new temp syms can be created
    m_func->m_symTable->IncreaseStartingID(m_firstIRTemp - m_func->m_symTable->GetMaxSymID());

    Js::RegSlot tempCount = m_asmFuncInfo->GetDoubleTmpCount() + m_asmFuncInfo->GetFloatTmpCount() + m_asmFuncInfo->GetIntTmpCount();

    tempCount += m_asmFuncInfo->GetSimdTmpCount();

    if (tempCount > 0)
    {
        m_tempMap = (SymID*)m_tempAlloc->AllocZero(sizeof(SymID) * tempCount);
        m_fbvTempUsed = BVFixed::New<JitArenaAllocator>(tempCount, m_tempAlloc);
    }
    else
    {
        m_tempMap = nullptr;
        m_fbvTempUsed = nullptr;
    }

    m_func->m_headInstr = IR::EntryInstr::New(Js::OpCode::FunctionEntry, m_func);
    m_func->m_exitInstr = IR::ExitInstr::New(Js::OpCode::FunctionExit, m_func);
    m_func->m_tailInstr = m_func->m_exitInstr;
    m_func->m_headInstr->InsertAfter(m_func->m_tailInstr);
    m_func->m_isLeaf = true;  // until proven otherwise

    m_functionStartOffset = m_jnReader.GetCurrentOffset();
    m_lastInstr = m_func->m_headInstr;

    AssertMsg(sizeof(SymID) >= sizeof(Js::RegSlot), "sizeof(SymID) != sizeof(Js::RegSlot)!!");

    offset = m_functionStartOffset;

    // Skip the last EndOfBlock opcode
    // EndOfBlock opcode has same value in Asm
    Assert(!OpCodeAttr::HasMultiSizeLayout(Js::OpCode::EndOfBlock));
    uint32 lastOffset = m_func->GetJITFunctionBody()->GetByteCodeLength() - Js::OpCodeUtil::EncodedSize(Js::OpCode::EndOfBlock, Js::SmallLayout);
    uint32 offsetToInstructionCount = lastOffset;
    if (this->IsLoopBody())
    {
        // LdSlot needs to cover all the register, including the temps, because we might treat
        // those as if they are local for the value of the with statement
        this->m_ldSlots = BVFixed::New<JitArenaAllocator>(m_firstIntTemp + tempCount, m_tempAlloc);
        this->m_stSlots = BVFixed::New<JitArenaAllocator>(m_firstIntTemp, m_tempAlloc);
        this->m_loopBodyRetIPSym = StackSym::New(TyInt32, this->m_func);
#if DBG
        if (m_func->GetJITFunctionBody()->GetTempCount() != 0)
        {
            this->m_usedAsTemp = BVFixed::New<JitArenaAllocator>(m_func->GetJITFunctionBody()->GetTempCount(), m_tempAlloc);
        }
#endif
        lastOffset = m_func->GetWorkItem()->GetLoopHeader()->endOffset;
        // Ret is created at lastOffset + 1, so we need lastOffset + 2 entries
        offsetToInstructionCount = lastOffset + 2;
    }

#if DBG
    m_offsetToInstructionCount = offsetToInstructionCount;
#endif
    m_offsetToInstruction = JitAnewArrayZ(m_tempAlloc, IR::Instr *, offsetToInstructionCount);

    LoadNativeCodeData();

    BuildConstantLoads();
    if (!this->IsLoopBody() && m_func->GetJITFunctionBody()->HasImplicitArgIns())
    {
        BuildImplicitArgIns();
    }

    if (m_statementReader.AtStatementBoundary(&m_jnReader))
    {
        statementIndex = AddStatementBoundary(statementIndex, offset);
    }

    Js::LayoutSize layoutSize;
    for (Js::OpCodeAsmJs newOpcode = m_jnReader.ReadAsmJsOp(layoutSize); (uint)m_jnReader.GetCurrentOffset() <= lastOffset; newOpcode = m_jnReader.ReadAsmJsOp(layoutSize))
    {
        Assert(newOpcode != Js::OpCodeAsmJs::EndOfBlock);

        AssertMsg(Js::OpCodeUtilAsmJs::IsValidByteCodeOpcode(newOpcode), "Error getting opcode from m_jnReader.Op()");

        uint layoutAndSize = layoutSize * Js::OpLayoutTypeAsmJs::Count + Js::OpCodeUtilAsmJs::GetOpCodeLayout(newOpcode);
        switch (layoutAndSize)
        {
#define LAYOUT_TYPE(layout) \
        case Js::OpLayoutTypeAsmJs::layout: \
            Assert(layoutSize == Js::SmallLayout); \
            Build##layout(newOpcode, offset); \
            break;
#define LAYOUT_TYPE_WMS(layout) \
        case Js::SmallLayout * Js::OpLayoutTypeAsmJs::Count + Js::OpLayoutTypeAsmJs::layout: \
            Build##layout<Js::SmallLayoutSizePolicy>(newOpcode, offset); \
            break; \
        case Js::MediumLayout * Js::OpLayoutTypeAsmJs::Count + Js::OpLayoutTypeAsmJs::layout: \
            Build##layout<Js::MediumLayoutSizePolicy>(newOpcode, offset); \
            break; \
        case Js::LargeLayout * Js::OpLayoutTypeAsmJs::Count + Js::OpLayoutTypeAsmJs::layout: \
            Build##layout<Js::LargeLayoutSizePolicy>(newOpcode, offset); \
            break;
#define EXCLUDE_FRONTEND_LAYOUT
#include "ByteCode/LayoutTypesAsmJs.h"

        default:
            AssertMsg(UNREACHED, "Unimplemented layout");
            break;
        }
        offset = m_jnReader.GetCurrentOffset();

        if (m_statementReader.AtStatementBoundary(&m_jnReader))
        {
            statementIndex = AddStatementBoundary(statementIndex, offset);
        }

    }

    if (Js::Constants::NoStatementIndex != statementIndex)
    {
        statementIndex = AddStatementBoundary(statementIndex, Js::Constants::NoByteCodeOffset);
    }

    if (IsLoopBody())
    {
        // Insert the LdSlot/StSlot and Ret
        IR::Opnd * retOpnd = this->InsertLoopBodyReturnIPInstr(offset, offset);

        // Restore and Ret are at the last offset + 1
        GenerateLoopBodySlotAccesses(lastOffset + 1);

        IR::Instr *      retInstr = IR::Instr::New(Js::OpCode::Ret, m_func);
        retInstr->SetSrc1(retOpnd);
        this->AddInstr(retInstr, lastOffset + 1);
    }

    // Now fix up the targets for all the branches we've introduced.
    InsertLabels();

    // Now that we know whether the func is a leaf or not, decide whether we'll emit fast paths.
    // Do this once and for all, per-func, since the source size on the ThreadContext will be
    // changing while we JIT.
    if (m_func->IsTopFunc())
    {
        m_func->SetDoFastPaths();
    }
}

void
IRBuilderAsmJs::LoadNativeCodeData()
{
    Assert(m_func->IsTopFunc());
    if (m_func->IsOOPJIT())
    {
        IR::RegOpnd * nativeDataOpnd = IR::RegOpnd::New(TyVar, m_func);
        IR::Instr * instr = IR::Instr::New(Js::OpCode::LdNativeCodeData, nativeDataOpnd, m_func);
        this->AddInstr(instr, Js::Constants::NoByteCodeOffset);
        m_func->SetNativeCodeDataSym(nativeDataOpnd->GetStackSym());
    }
}

void
IRBuilderAsmJs::AddInstr(IR::Instr * instr, uint32 offset)
{
    m_lastInstr->InsertAfter(instr);
    if (offset != Js::Constants::NoByteCodeOffset)
    {
        Assert(offset < m_offsetToInstructionCount);
        if (m_offsetToInstruction[offset] == nullptr)
        {
            m_offsetToInstruction[offset] = instr;
        }
        else
        {
            Assert(m_lastInstr->GetByteCodeOffset() == offset);
        }
        instr->SetByteCodeOffset(offset);
    }
    else
    {
        instr->SetByteCodeOffset(m_lastInstr->GetByteCodeOffset());
    }
    m_lastInstr = instr;

    Func *topFunc = m_func->GetTopFunc();
    if (!topFunc->GetHasTempObjectProducingInstr())
    {
        if (OpCodeAttr::TempObjectProducing(instr->m_opcode))
        {
            topFunc->SetHasTempObjectProducingInstr(true);
        }
    }

#if DBG_DUMP
    if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::IRBuilderPhase, m_func->GetTopFunc()->GetSourceContextId(), m_func->GetTopFunc()->GetLocalFunctionId()))
    {
        instr->Dump();
    }
#endif
}

IR::RegOpnd *
IRBuilderAsmJs::BuildDstOpnd(Js::RegSlot dstRegSlot, IRType type)
{
    SymID symID;
    if (RegIsTemp(dstRegSlot))
    {
#if DBG
        if (this->IsLoopBody())
        {
            // If we are doing loop body, and a temp reg slot is loaded via LdSlot
            // That means that we have detected that the slot is live coming in to the loop.
            // This would only happen for the value of a "with" statement, so there shouldn't
            // be any def for those
            Assert(!this->m_ldSlots->Test(dstRegSlot));
            this->m_usedAsTemp->Set(dstRegSlot - m_firstIntTemp);
        }
#endif
        // This is a def of a temp. Create a new sym ID for it if it's been used since its last def.
        //     !!!NOTE: always process an instruction's temp uses before its temp defs!!!
        if (GetTempUsed(dstRegSlot))
        {
            symID = m_func->m_symTable->NewID();
            SetTempUsed(dstRegSlot, FALSE);
            SetMappedTemp(dstRegSlot, symID);
        }
        else
        {
            symID = GetMappedTemp(dstRegSlot);
            // The temp hasn't been used since its last def. There are 2 possibilities:
            if (symID == 0)
            {
                // First time we've seen the temp. Just use the number that the front end gave it.
                symID = static_cast<SymID>(dstRegSlot);
                SetMappedTemp(dstRegSlot, symID);
            }
            else if (IRType_IsSimd128(type))
            {
                //In Asm.js, SIMD register space is untyped, so we could have SIMD temp registers.
                //Make sure that the StackSym types matches before reusing simd temps.
                StackSym *  stackSym = m_func->m_symTable->FindStackSym(symID);
                if (!stackSym || stackSym->GetType() != type)
                {
                    symID = m_func->m_symTable->NewID();
                    SetMappedTemp(dstRegSlot, symID);
                }
            }
        }
    }
    else
    {
        symID = static_cast<SymID>(dstRegSlot);
        if (RegIsConstant(dstRegSlot))
        {
            // Don't need to track constant registers for bailout. Don't set the byte code register for constant.
            dstRegSlot = Js::Constants::NoRegister;
        }
        else if (IsLoopBody() && RegIsVar(dstRegSlot))
        {
            // Loop body and not constants
            this->m_stSlots->Set(symID);
            // We need to make sure that the symbols is loaded as well
            // so that the sym will be defined on all path.
            this->EnsureLoopBodyAsmJsLoadSlot(symID, type);
        }
        else
        {
            Assert(!IsLoopBody() || dstRegSlot == 0); // if loop body then one of the above two conditions should hold true
        }
    }

    //Simd return values of different IR types share the same reg slot.
    //To avoid symbol type mismatch, use the stack symbol with a dummy simd type.
    if (RegIsSimd128ReturnVar(symID))
    {
        type = TySimd128F4;
    }
    StackSym * symDst = StackSym::FindOrCreate(symID, dstRegSlot, m_func, type);
    // Always reset isSafeThis to false.  We'll set it to true for singleDef cases,
    // but want to reset it to false if it is multi-def.
    // NOTE: We could handle the multiDef if they are all safe, but it probably isn't very common.
    symDst->m_isSafeThis = false;

    IR::RegOpnd *regOpnd = IR::RegOpnd::New(symDst, type, m_func);
    return regOpnd;
}

IR::RegOpnd *
IRBuilderAsmJs::BuildSrcOpnd(Js::RegSlot srcRegSlot, IRType type)
{
    StackSym * symSrc = m_func->m_symTable->FindStackSym(BuildSrcStackSymID(srcRegSlot, type));
    AssertMsg(symSrc, "Tried to use an undefined stack slot?");
        IR::RegOpnd * regOpnd = IR::RegOpnd::New(symSrc, type, m_func);

    return regOpnd;
}

IR::RegOpnd *
IRBuilderAsmJs::BuildIntConstOpnd(Js::RegSlot regSlot)
{
    Js::Var * constTable = (Js::Var*)m_func->GetJITFunctionBody()->GetConstTable();
    int * intConstTable = reinterpret_cast<int *>(constTable + Js::AsmJsFunctionMemory::RequiredVarConstants - 1);
    uint32 intConstCount = m_func->GetJITFunctionBody()->GetAsmJsInfo()->GetIntConstCount();

    Assert(regSlot >= Js::FunctionBody::FirstRegSlot && regSlot < intConstCount);
    const int32 value = intConstTable[regSlot];
    IR::IntConstOpnd *opnd = IR::IntConstOpnd::New(value, TyInt32, m_func);

    return (IR::RegOpnd*)opnd;
}

SymID
IRBuilderAsmJs::BuildSrcStackSymID(Js::RegSlot regSlot, IRType type /*= IRType::TyVar*/)
{
    SymID symID;

    if (this->RegIsTemp(regSlot))
    {
        // This is a use of a temp. Map the reg slot to its sym ID.
        //     !!!NOTE: always process an instruction's temp uses before its temp defs!!!
        symID = this->GetMappedTemp(regSlot);
        if (symID == 0)
        {
            // We might have temps that are live through the loop body via "with" statement
            // We need to treat those as if they are locals and don't remap them
            Assert(this->IsLoopBody());
            Assert(!this->m_usedAsTemp->Test(regSlot - m_firstIntTemp));

            symID = static_cast<SymID>(regSlot);
            this->SetMappedTemp(regSlot, symID);
            this->EnsureLoopBodyAsmJsLoadSlot(symID, type);
        }
        this->SetTempUsed(regSlot, TRUE);
    }
    else
    {
        symID = static_cast<SymID>(regSlot);
        if (IsLoopBody() && RegIsVar(regSlot))
        {
            this->EnsureLoopBodyAsmJsLoadSlot(symID, type);

        }
        else
        {
            Assert(!IsLoopBody() || this->RegIsConstant(regSlot) || regSlot == 0);
        }
    }
    return symID;

}

IR::SymOpnd *
IRBuilderAsmJs::BuildFieldOpnd(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind, IRType type, bool scale)
{
    Js::PropertyId scaledPropertyId = propertyId;
    if (scale)
    {
        scaledPropertyId *= TySize[type];
    }
    PropertySym * propertySym = BuildFieldSym(reg, scaledPropertyId, propertyKind);
    IR::SymOpnd * symOpnd = IR::SymOpnd::New(propertySym, type, m_func);

    return symOpnd;
}

PropertySym *
IRBuilderAsmJs::BuildFieldSym(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind)
{
    SymID symId = BuildSrcStackSymID(reg);

    AssertMsg(m_func->m_symTable->FindStackSym(symId), "Tried to use an undefined stacksym?");

    PropertySym * propertySym = PropertySym::FindOrCreate(symId, propertyId, (Js::PropertyIdIndexType)-1, (uint)-1, propertyKind, m_func);
    return propertySym;
}

uint
IRBuilderAsmJs::AddStatementBoundary(uint statementIndex, uint offset)
{
    IR::PragmaInstr* pragmaInstr = IR::PragmaInstr::New(Js::OpCode::StatementBoundary, statementIndex, m_func);
    this->AddInstr(pragmaInstr, offset);

    return m_statementReader.MoveNextStatementBoundary();
}

Js::RegSlot
IRBuilderAsmJs::GetRegSlotFromIntReg(Js::RegSlot srcIntReg)
{
    Assert(m_asmFuncInfo->GetIntConstCount() >= 0);
    Js::RegSlot reg;
    if (srcIntReg < (Js::RegSlot)m_asmFuncInfo->GetIntConstCount())
    {
        reg = srcIntReg + m_firstIntConst;
        Assert(reg >= m_firstIntConst && reg < m_firstFloatConst);
    }
    else if (srcIntReg < (Js::RegSlot)(m_asmFuncInfo->GetIntVarCount() + m_asmFuncInfo->GetIntConstCount()))
    {
        reg = srcIntReg - m_asmFuncInfo->GetIntConstCount() + m_firstIntVar;
        Assert(reg >= m_firstIntVar && reg < m_firstFloatVar);
    }
    else
    {
        reg = srcIntReg - (m_asmFuncInfo->GetIntVarCount() + m_asmFuncInfo->GetIntConstCount()) + m_firstIntTemp;
        Assert(reg >= m_firstIntTemp && reg < m_firstFloatTemp);
    }
    return reg;
}

Js::RegSlot
IRBuilderAsmJs::GetRegSlotFromFloatReg(Js::RegSlot srcFloatReg)
{
    Assert(m_asmFuncInfo->GetFloatConstCount() >= 0);
    Js::RegSlot reg;
    if (srcFloatReg < (Js::RegSlot)m_asmFuncInfo->GetFloatConstCount())
    {
        reg = srcFloatReg + m_firstFloatConst;
        Assert(reg >= m_firstFloatConst && reg < m_firstDoubleConst);
    }
    else if (srcFloatReg < (Js::RegSlot)(m_asmFuncInfo->GetFloatVarCount() + m_asmFuncInfo->GetFloatConstCount()))
    {
        reg = srcFloatReg - m_asmFuncInfo->GetFloatConstCount() + m_firstFloatVar;
        Assert(reg >= m_firstFloatVar && reg < m_firstDoubleVar);
    }
    else
    {
        reg = srcFloatReg - (m_asmFuncInfo->GetFloatVarCount() + m_asmFuncInfo->GetFloatConstCount()) + m_firstFloatTemp;
        Assert(reg >= m_firstFloatTemp && reg < m_firstDoubleTemp);
    }
    return reg;
}

Js::RegSlot
IRBuilderAsmJs::GetRegSlotFromDoubleReg(Js::RegSlot srcDoubleReg)
{
    Assert(m_asmFuncInfo->GetDoubleConstCount() >= 0);
    Js::RegSlot reg;
    if (srcDoubleReg < (Js::RegSlot)m_asmFuncInfo->GetDoubleConstCount())
    {
        reg = srcDoubleReg + m_firstDoubleConst;
        Assert(reg >= m_firstDoubleConst && reg < m_firstSimdConst);
    }
    else if (srcDoubleReg < (Js::RegSlot)(m_asmFuncInfo->GetDoubleVarCount() + m_asmFuncInfo->GetDoubleConstCount()))
    {
        reg = srcDoubleReg - m_asmFuncInfo->GetDoubleConstCount() + m_firstDoubleVar;
        Assert(reg >= m_firstDoubleVar && reg < m_firstSimdVar);
    }
    else
    {
        reg = srcDoubleReg - (m_asmFuncInfo->GetDoubleVarCount() + m_asmFuncInfo->GetDoubleConstCount()) + m_firstDoubleTemp;
        Assert(reg >= m_firstDoubleTemp && reg < m_firstSimdTemp);

    }
    return reg;
}

Js::RegSlot
IRBuilderAsmJs::GetRegSlotFromSimd128Reg(Js::RegSlot srcSimd128Reg)
{
    Js::RegSlot reg;
    if (srcSimd128Reg < (uint32)m_asmFuncInfo->GetSimdConstCount())
    {
        reg = srcSimd128Reg + m_firstSimdConst;
        Assert(reg >= m_firstSimdConst && reg < m_firstIntVar);
    }
    else if (srcSimd128Reg < (Js::RegSlot)(m_asmFuncInfo->GetSimdVarCount() + m_asmFuncInfo->GetSimdConstCount()))
    {
        reg = srcSimd128Reg - m_asmFuncInfo->GetSimdConstCount() + m_firstSimdVar;
        Assert(reg >= m_firstSimdVar && reg < m_firstIntTemp);
    }
    else
    {
        reg = srcSimd128Reg - (m_asmFuncInfo->GetSimdVarCount() + m_asmFuncInfo->GetSimdConstCount()) + m_firstSimdTemp;
        Assert(reg >= m_firstSimdTemp && reg < m_firstIRTemp);
    }
    return reg;
}

IR::Instr *
IRBuilderAsmJs::AddExtendedArg(IR::RegOpnd *src1, IR::RegOpnd *src2, uint32 offset)
{
    Assert(src1);
    IR::RegOpnd * dst = IR::RegOpnd::New(src1->GetType(), m_func);
    dst->SetValueType(src1->GetValueType());

    IR::Instr * instr = IR::Instr::New(Js::OpCode::ExtendArg_A, dst, src1, m_func);
    if (src2)
    {
        instr->SetSrc2(src2);
    }
    AddInstr(instr, offset);
    return instr;
}

Js::RegSlot
IRBuilderAsmJs::GetRegSlotFromVarReg(Js::RegSlot srcVarReg)
{
    Js::RegSlot reg;
    if (srcVarReg < (Js::RegSlot)(AsmJsRegSlots::RegCount - 1))
    {
        reg = srcVarReg + m_firstVarConst;
        Assert(reg >= m_firstVarConst && reg < m_firstIntConst);
    }
    else
    {
        reg = srcVarReg - AsmJsRegSlots::RegCount + m_firstIntTemp - 1;
        Assert(reg >= m_firstIntTemp);
    }
    return reg;
}

SymID
IRBuilderAsmJs::GetMappedTemp(Js::RegSlot reg)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_tempMap, "Processing non-temp reg without a temp map?");

    return m_tempMap[reg - m_firstIntTemp];
}

void
IRBuilderAsmJs::SetMappedTemp(Js::RegSlot reg, SymID tempId)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_tempMap, "Processing non-temp reg without a temp map?");

    m_tempMap[reg - m_firstIntTemp] = tempId;
}

BOOL
IRBuilderAsmJs::GetTempUsed(Js::RegSlot reg)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_fbvTempUsed, "Processing non-temp reg without a used BV?");

    return m_fbvTempUsed->Test(reg - m_firstIntTemp);
}

void
IRBuilderAsmJs::SetTempUsed(Js::RegSlot reg, BOOL used)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_fbvTempUsed, "Processing non-temp reg without a used BV?");

    if (used)
    {
        m_fbvTempUsed->Set(reg - m_firstIntTemp);
    }
    else
    {
        m_fbvTempUsed->Clear(reg - m_firstIntTemp);
    }
}

BOOL
IRBuilderAsmJs::RegIsTemp(Js::RegSlot reg)
{
    return reg >= m_firstIntTemp;
}

BOOL
IRBuilderAsmJs::RegIsVar(Js::RegSlot reg)
{
    BOOL result =  RegIsIntVar(reg) || RegIsFloatVar(reg) || RegIsDoubleVar(reg);
    result = result || RegIsSimd128Var(reg);
    return result;
}

BOOL
IRBuilderAsmJs::RegIsIntVar(Js::RegSlot reg)
{
    Js::RegSlot endVarSlotCount = (Js::RegSlot)(m_asmFuncInfo->GetIntVarCount()) + m_firstIntVar;
    return reg >= m_firstIntVar && reg < endVarSlotCount;

}
BOOL
IRBuilderAsmJs::RegIsFloatVar(Js::RegSlot reg)
{
    Js::RegSlot endVarSlotCount = (Js::RegSlot)(m_asmFuncInfo->GetFloatVarCount()) + m_firstFloatVar;
    return reg >= m_firstFloatVar && reg < endVarSlotCount;

}
BOOL
IRBuilderAsmJs::RegIsDoubleVar(Js::RegSlot reg)
{
    Js::RegSlot endVarSlotCount = (Js::RegSlot)(m_asmFuncInfo->GetDoubleVarCount()) + m_firstDoubleVar;
    return reg >= m_firstDoubleVar && reg < endVarSlotCount;

}
BOOL
IRBuilderAsmJs::RegIsSimd128Var(Js::RegSlot reg)
{
    Js::RegSlot endVarSlotCount = (Js::RegSlot)(m_asmFuncInfo->GetSimdVarCount()) + m_firstSimdVar;
    return reg >= m_firstSimdVar && reg < endVarSlotCount;

}
bool
IRBuilderAsmJs::RegIsSimd128ReturnVar(Js::RegSlot reg)
{
    return (reg == m_firstSimdConst &&
            Js::AsmJsRetType(m_asmFuncInfo->GetRetType()).toVarType().isSIMD());
}
BOOL
IRBuilderAsmJs::RegIsConstant(Js::RegSlot reg)
{
    return (reg > 0 && reg < m_firstIntVar);
}

BranchReloc *
IRBuilderAsmJs::AddBranchInstr(IR::BranchInstr * branchInstr, uint32 offset, uint32 targetOffset)
{
    //
    // Loop jitting would be done only till the LoopEnd
    // Any branches beyond that offset are for the return statement
    //
    if (IsLoopBodyOuterOffset(targetOffset))
    {
        // if we have loaded the loop IP sym from the ProfiledLoopEnd then don't add it here
        if (!IsLoopBodyReturnIPInstr(m_lastInstr))
        {
            this->InsertLoopBodyReturnIPInstr(targetOffset, offset);
        }

        // Jump the restore StSlot and Ret instruction
        targetOffset = GetLoopBodyExitInstrOffset();
    }
    BranchReloc *  reloc = nullptr;
    reloc = CreateRelocRecord(branchInstr, offset, targetOffset);

    AddInstr(branchInstr, offset);
    return reloc;
}

BranchReloc *
IRBuilderAsmJs::CreateRelocRecord(IR::BranchInstr * branchInstr, uint32 offset, uint32 targetOffset)
{
    BranchReloc * reloc = JitAnew(m_tempAlloc, BranchReloc, branchInstr, offset, targetOffset);
    m_branchRelocList->Prepend(reloc);
    return reloc;
}

void
IRBuilderAsmJs::BuildHeapBufferReload(uint32 offset)
{

    // ArrayBuffer
    IR::RegOpnd * dstOpnd = BuildDstOpnd(AsmJsRegSlots::ArrayReg, TyVar);
    IR::Opnd * srcOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::ModuleMemReg, TyVar), Js::AsmJsModuleMemory::MemoryTableBeginOffset, TyVar, m_func);
    IR::Instr * instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);

    // ArrayBuffer buffer
    dstOpnd = BuildDstOpnd(AsmJsRegSlots::BufferReg, TyVar);
    srcOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::ArrayReg, TyVar), Js::ArrayBuffer::GetBufferOffset(), TyVar, m_func);
    instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);

    // ArrayBuffer length
    dstOpnd = BuildDstOpnd(AsmJsRegSlots::LengthReg, TyUint32);
    srcOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::ArrayReg, TyVar), Js::ArrayBuffer::GetByteLengthOffset(), TyUint32, m_func);
    instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildConstantLoads()
{
    uint32 intConstCount = m_func->GetJITFunctionBody()->GetAsmJsInfo()->GetIntConstCount();
    uint32 floatConstCount = m_func->GetJITFunctionBody()->GetAsmJsInfo()->GetFloatConstCount();
    uint32 doubleConstCount = m_func->GetJITFunctionBody()->GetAsmJsInfo()->GetDoubleConstCount();
    Js::Var * constTable = (Js::Var *)m_func->GetJITFunctionBody()->GetConstTable();

    // Load FrameDisplay
    IR::RegOpnd * asmJsEnvDstOpnd = BuildDstOpnd(AsmJsRegSlots::ModuleMemReg, TyVar);
    IR::Instr * ldAsmJsEnvInstr = IR::Instr::New(Js::OpCode::LdAsmJsEnv, asmJsEnvDstOpnd, m_func);
    AddInstr(ldAsmJsEnvInstr, Js::Constants::NoByteCodeOffset);

    // Load heap buffer
    if (m_asmFuncInfo->UsesHeapBuffer())
    {
        BuildHeapBufferReload(Js::Constants::NoByteCodeOffset);
    }

    uint32 regAllocated = AsmJsRegSlots::RegCount;

    // build int const loads

    int * intConstTable = reinterpret_cast<int *>(constTable + Js::AsmJsFunctionMemory::RequiredVarConstants - 1);

    for (Js::RegSlot reg = Js::FunctionBody::FirstRegSlot; reg < intConstCount; ++reg)
    {
        int32 intConst = intConstTable[reg];

        IR::RegOpnd * dstOpnd = BuildDstOpnd(regAllocated + Js::FunctionBody::FirstRegSlot, TyInt32);
        Assert(RegIsConstant(reg));
        dstOpnd->m_sym->SetIsFromByteCodeConstantTable();
        dstOpnd->SetValueType(ValueType::GetInt(false));

        IR::Instr *instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, IR::IntConstOpnd::New(intConst, TyInt32, m_func), m_func);

        if (!m_func->IsOOPJIT() && dstOpnd->m_sym->IsSingleDef())
        {
            dstOpnd->m_sym->SetIsIntConst(intConst);
        }

        AddInstr(instr, Js::Constants::NoByteCodeOffset);

        ++regAllocated;
    }

    // do the same for float constants

    // Space for F0
    ++regAllocated;

    float * floatConstTable = reinterpret_cast<float *>(intConstTable + intConstCount);

    for (Js::RegSlot reg = Js::FunctionBody::FirstRegSlot; reg < floatConstCount; ++reg)
    {
        float floatConst = floatConstTable[reg];

        IR::RegOpnd * dstOpnd = BuildDstOpnd(regAllocated + Js::FunctionBody::FirstRegSlot, TyFloat32);
        Assert(RegIsConstant(reg));
        dstOpnd->m_sym->SetIsFromByteCodeConstantTable();
        dstOpnd->SetValueType(ValueType::Float);

        IR::Instr *instr = IR::Instr::New(Js::OpCode::LdC_F8_R8, dstOpnd, IR::FloatConstOpnd::New(floatConst, TyFloat32, m_func), m_func);

#if _M_IX86
        if (!m_func->IsOOPJIT() && dstOpnd->m_sym->IsSingleDef())
        {
            dstOpnd->m_sym->SetIsFloatConst();
        }
#endif

        AddInstr(instr, Js::Constants::NoByteCodeOffset);

        ++regAllocated;
    }

    // ... and doubles

    // Space for D0
    ++regAllocated;
    double * doubleConstTable = reinterpret_cast<double *>(floatConstTable + floatConstCount);

    for (Js::RegSlot reg = Js::FunctionBody::FirstRegSlot; reg < doubleConstCount; ++reg)
    {
        double doubleConst = doubleConstTable[reg];

        IR::RegOpnd * dstOpnd = BuildDstOpnd(regAllocated + Js::FunctionBody::FirstRegSlot, TyFloat64);
        Assert(RegIsConstant(reg));
        dstOpnd->m_sym->SetIsFromByteCodeConstantTable();
        dstOpnd->SetValueType(ValueType::Float);

        IR::Instr *instr = IR::Instr::New(Js::OpCode::LdC_F8_R8, dstOpnd, IR::FloatConstOpnd::New(doubleConst, TyFloat64, m_func), m_func);

#if _M_IX86
        if (!m_func->IsOOPJIT() && dstOpnd->m_sym->IsSingleDef())
        {
            dstOpnd->m_sym->SetIsFloatConst();
        }
#endif

        AddInstr(instr, Js::Constants::NoByteCodeOffset);

        ++regAllocated;
    }

    uint32 simdConstCount = m_func->GetJITFunctionBody()->GetAsmJsInfo()->GetSimdConstCount();
    // Space for SIMD0
    ++regAllocated;
    AsmJsSIMDValue *simdConstTable = reinterpret_cast<AsmJsSIMDValue *>(doubleConstTable + doubleConstCount);

    for (Js::RegSlot reg = Js::FunctionBody::FirstRegSlot; reg < simdConstCount; ++reg)
    {
        AsmJsSIMDValue simdConst = simdConstTable[reg];

        // Simd constants are not sub-typed, we pick any IR type for now, when the constant is actually used (Ld_A to a variable), we type the variable from the opcode.
        IR::RegOpnd * dstOpnd = BuildDstOpnd(regAllocated + Js::FunctionBody::FirstRegSlot, TySimd128F4);
        Assert(RegIsConstant(reg));
        dstOpnd->m_sym->SetIsFromByteCodeConstantTable();
        // Constants vars are generic SIMD128 values, no sub-type. We currently don't have top SIMD128 type in ValueType, since it is not needed by globOpt.
        // However, for ASMJS, the IR type is enough to tell us it is a Simd128 value.
        dstOpnd->SetValueType(ValueType::UninitializedObject);

        IR::Instr *instrLdC = IR::Instr::New(Js::OpCode::Simd128_LdC, dstOpnd, IR::Simd128ConstOpnd::New(simdConst, TySimd128F4, m_func), m_func);

#if _M_IX86
        if (!m_func->IsOOPJIT() && dstOpnd->m_sym->IsSingleDef())
        {
            dstOpnd->m_sym->SetIsSimd128Const();
        }
#endif

        AddInstr(instrLdC, Js::Constants::NoByteCodeOffset);
        ++regAllocated;
    }
}

void
IRBuilderAsmJs::BuildImplicitArgIns()
{
    int32 intArgInCount = 0;
    int32 floatArgInCount = 0;
    int32 doubleArgInCount = 0;
    int32 simd128ArgInCount = 0;

    // formal params are offset from EBP by the EBP chain, return address, and function object
    int32 offset = 3 * MachPtr;
    for (Js::ArgSlot i = 1; i < m_func->GetJITFunctionBody()->GetInParamsCount(); ++i)
    {
        StackSym * symSrc = nullptr;
        IR::Opnd * srcOpnd = nullptr;
        IR::RegOpnd * dstOpnd = nullptr;
        IR::Instr * instr = nullptr;
        // TODO: double args are not aligned on stack
        Js::AsmJsVarType varType = m_func->GetJITFunctionBody()->GetAsmJsInfo()->GetArgType(i - 1);
        switch (varType.which())
        {
        case Js::AsmJsVarType::Which::Int:
            symSrc = StackSym::NewParamSlotSym(i, m_func, TyInt32);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, TyInt32, m_func);
            dstOpnd = BuildDstOpnd(m_firstIntVar + intArgInCount, TyInt32);
            dstOpnd->SetValueType(ValueType::GetInt(false));
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += MachPtr;
            ++intArgInCount;
            break;
        case Js::AsmJsVarType::Which::Float:
            symSrc = StackSym::NewParamSlotSym(i, m_func, TyFloat32);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, TyFloat32, m_func);
            dstOpnd = BuildDstOpnd(m_firstFloatVar + floatArgInCount, TyFloat32);
            dstOpnd->SetValueType(ValueType::Float);
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += MachPtr;
            ++floatArgInCount;
            break;
        case Js::AsmJsVarType::Which::Double:
            symSrc = StackSym::NewParamSlotSym(i, m_func, TyFloat64);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, TyFloat64, m_func);
            dstOpnd = BuildDstOpnd(m_firstDoubleVar + doubleArgInCount, TyFloat64);
            dstOpnd->SetValueType(ValueType::Float);
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += MachDouble;
            ++doubleArgInCount;
            break;

        default:
        {
            // SIMD_JS
            IRType argType;
            GetSimdTypesFromAsmType((Js::AsmJsType::Which)varType.which(), &argType);

            symSrc = StackSym::NewParamSlotSym(i, m_func, argType);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, argType, m_func);
            dstOpnd = BuildDstOpnd(m_firstSimdVar + simd128ArgInCount, argType);
            dstOpnd->SetValueType(ValueType::UninitializedObject);
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += sizeof(AsmJsSIMDValue);
            ++simd128ArgInCount;
            break;
        }
        
        }

        AddInstr(instr, Js::Constants::NoByteCodeOffset);
    }
}

void
IRBuilderAsmJs::InsertLabels()
{
    AssertMsg(m_branchRelocList, "Malformed branch reloc list");

    SList<BranchReloc *>::Iterator iter(m_branchRelocList);

    while (iter.Next())
    {
        IR::LabelInstr * labelInstr;
        BranchReloc * reloc = iter.Data();
        IR::BranchInstr * branchInstr = reloc->GetBranchInstr();
        uint offset = reloc->GetOffset();
        uint const branchOffset = reloc->GetBranchOffset();

        Assert(!IsLoopBody() || offset <= GetLoopBodyExitInstrOffset());

        if (branchInstr->m_opcode == Js::OpCode::MultiBr)
        {
            IR::MultiBranchInstr * multiBranchInstr = branchInstr->AsBranchInstr()->AsMultiBrInstr();

            multiBranchInstr->UpdateMultiBrTargetOffsets([&](uint32 offset) -> IR::LabelInstr *
            {
                labelInstr = this->CreateLabel(branchInstr, offset);
                multiBranchInstr->ChangeLabelRef(nullptr, labelInstr);
                return labelInstr;
            });
        }
        else
        {
            labelInstr = CreateLabel(branchInstr, offset);
            branchInstr->SetTarget(labelInstr);
        }

        if (!reloc->IsNotBackEdge() && branchOffset >= offset)
        {
            labelInstr->m_isLoopTop = true;
        }
    }
}

IR::LabelInstr *
IRBuilderAsmJs::CreateLabel(IR::BranchInstr * branchInstr, uint & offset)
{
    IR::Instr * targetInstr = nullptr;
    while (targetInstr == nullptr)
    {
        targetInstr = m_offsetToInstruction[offset];
        Assert(offset < m_offsetToInstructionCount);
        ++offset;
    }

    IR::Instr *instrPrev = targetInstr->m_prev;

    if (instrPrev)
    {
        instrPrev = targetInstr->GetPrevRealInstrOrLabel();
    }

    IR::LabelInstr * labelInstr;
    if (instrPrev && instrPrev->IsLabelInstr())
    {
        // Found an existing label at the right offset. Just reuse it.
        labelInstr = instrPrev->AsLabelInstr();
    }
    else
    {
        // No label at the desired offset. Create one.
        labelInstr = IR::LabelInstr::New(Js::OpCode::Label, m_func);
        labelInstr->SetByteCodeOffset(offset);
        if (instrPrev)
        {
            instrPrev->InsertAfter(labelInstr);
        }
        else
        {
            targetInstr->InsertBefore(labelInstr);
        }
    }
    return labelInstr;
}

void
IRBuilderAsmJs::BuildEmpty(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    m_jnReader.Empty();

    IR::Instr * instr = nullptr;
    IR::RegOpnd * regOpnd = nullptr;
    Js::RegSlot retSlot;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Ret:
        instr = IR::Instr::New(Js::OpCode::Ret, m_func);

        switch (m_asmFuncInfo->GetRetType())
        {
        case Js::AsmJsRetType::Which::Signed:
            retSlot = GetRegSlotFromIntReg(0);
            regOpnd = BuildDstOpnd(retSlot, TyInt32);
            regOpnd->SetValueType(ValueType::GetInt(false));
            break;

        case Js::AsmJsRetType::Which::Float:
            retSlot = GetRegSlotFromFloatReg(0);
            regOpnd = BuildDstOpnd(retSlot, TyFloat32);
            regOpnd->SetValueType(ValueType::Float);
            break;

        case Js::AsmJsRetType::Which::Double:
            retSlot = GetRegSlotFromDoubleReg(0);
            regOpnd = BuildDstOpnd(retSlot, TyFloat64);
            regOpnd->SetValueType(ValueType::Float);
            break;

        case Js::AsmJsRetType::Which::Void:
            retSlot = GetRegSlotFromVarReg(0);
            regOpnd = BuildDstOpnd(retSlot, TyVar);
            break;
        default:
        {
            IRType irType;
            ValueType vType;
            GetSimdTypesFromAsmType(Js::AsmJsRetType(m_asmFuncInfo->GetRetType()).toType().GetWhich(), &irType, &vType);
            retSlot = GetRegSlotFromSimd128Reg(0);
            regOpnd = BuildDstOpnd(retSlot, irType);
            regOpnd->SetValueType(vType);
        }

        }
        instr->SetSrc1(regOpnd);
        AddInstr(instr, offset);
        break;

    case Js::OpCodeAsmJs::Label:
        // NOP
        break;

    default:
        Assume(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildElementSlot(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_ElementSlot<SizePolicy>>();
    BuildElementSlot(newOpcode, offset, layout->SlotIndex, layout->Value, layout->Instance);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildAsmUnsigned1(Js::OpCodeAsmJs newOpcode, uint value)
{
   // we do not support counter in loop body ,just read the layout here
   m_jnReader.GetLayout<Js::OpLayoutT_AsmUnsigned1<SizePolicy>>();
}
void
IRBuilderAsmJs::BuildElementSlot(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 slotIndex, Js::RegSlot value, Js::RegSlot instance)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));

    Assert(instance == 1 || newOpcode == Js::OpCodeAsmJs::LdArr_Func);

    Js::RegSlot valueRegSlot;
    IR::Opnd * slotOpnd;
    IR::RegOpnd * regOpnd;
    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::LdSlot_Int:
        valueRegSlot = GetRegSlotFromIntReg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyInt32);

        regOpnd = BuildDstOpnd(valueRegSlot, TyInt32);
        regOpnd->SetValueType(ValueType::GetInt(false));

        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdSlot_Flt:
        valueRegSlot = GetRegSlotFromFloatReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyFloat32);

        regOpnd = BuildDstOpnd(valueRegSlot, TyFloat32);
        regOpnd->SetValueType(ValueType::Float);
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdSlot_Db:
        valueRegSlot = GetRegSlotFromDoubleReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyFloat64);

        regOpnd = BuildDstOpnd(valueRegSlot, TyFloat64);
        regOpnd->SetValueType(ValueType::Float);
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdSlot:
        valueRegSlot = GetRegSlotFromVarReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlotArray, TyVar);

        regOpnd = BuildDstOpnd(valueRegSlot, TyVar);
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdSlotArr:
        valueRegSlot = GetRegSlotFromVarReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyVar);

        regOpnd = BuildDstOpnd(valueRegSlot, TyVar);
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::StSlot_Int:
        valueRegSlot = GetRegSlotFromIntReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyInt32);

        regOpnd = BuildSrcOpnd(valueRegSlot, TyInt32);
        regOpnd->SetValueType(ValueType::GetInt(false));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::StSlot_Flt:
        valueRegSlot = GetRegSlotFromFloatReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyFloat32);

        regOpnd = BuildSrcOpnd(valueRegSlot, TyFloat32);
        regOpnd->SetValueType(ValueType::Float);
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::StSlot_Db:
        valueRegSlot = GetRegSlotFromDoubleReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyFloat64);

        regOpnd = BuildSrcOpnd(valueRegSlot, TyFloat64);
        regOpnd->SetValueType(ValueType::Float);
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdArr_Func:
    {
        IR::RegOpnd * baseOpnd = BuildSrcOpnd(GetRegSlotFromVarReg(instance), TyVar);
        IR::RegOpnd * indexOpnd = BuildSrcOpnd(GetRegSlotFromIntReg(slotIndex), TyUint32);
        // we multiply indexOpnd by 2^scale for the actual location
#if _M_IX86_OR_ARM32
        byte scale = 2;
#elif _M_X64_OR_ARM64
        byte scale = 3;
#endif
        IR::IndirOpnd * indirOpnd = IR::IndirOpnd::New(baseOpnd, indexOpnd, scale, TyVar, m_func);

        regOpnd = BuildDstOpnd(GetRegSlotFromVarReg(value), TyVar);
        instr = IR::Instr::New(Js::OpCode::Ld_A, regOpnd, indirOpnd, m_func);
        break;
    }

    case Js::OpCodeAsmJs::Simd128_LdSlot_I4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128I4);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128I4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
     }
    case Js::OpCodeAsmJs::Simd128_LdSlot_B4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128B4);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128B4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_LdSlot_B8:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128B8);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128B8);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_LdSlot_B16:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128B16);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128B16);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_LdSlot_F4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128F4);
        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128F4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
     }
#if 0
    case Js::OpCodeAsmJs::Simd128_LdSlot_D2:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128D2);
        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128D2);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    }
#endif // 0

    case Js::OpCodeAsmJs::Simd128_LdSlot_I8:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128I8);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128I8);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_LdSlot_I16:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128I16);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128I16);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_LdSlot_U4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128U4);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128U4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_LdSlot_U8:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128U8);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128U8);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_LdSlot_U16:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128U16);

        regOpnd = BuildDstOpnd(valueRegSlot, TySimd128U16);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_I4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128I4);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128I4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_B4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128B4);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128B4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_B8:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128B8);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128B8);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_B16:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128B16);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128B16);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_F4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128F4);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128F4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
#if 0
    case Js::OpCodeAsmJs::Simd128_StSlot_D2:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128D2);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128D2);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
#endif
    case Js::OpCodeAsmJs::Simd128_StSlot_I8:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128I8);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128I8);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_I16:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128I16);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128I16);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_U4:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128U4);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128U4);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_U8:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128U8);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128U8);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Simd128_StSlot_U16:
    {
        valueRegSlot = GetRegSlotFromSimd128Reg(value);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TySimd128U16);
        regOpnd = BuildSrcOpnd(valueRegSlot, TySimd128U16);
        regOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));
        instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        break;
    }

    default:
        Assume(UNREACHED);
    }

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildStartCall(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(!OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));

    const unaligned Js::OpLayoutStartCall * layout = m_jnReader.StartCall();
    IR::RegOpnd * dstOpnd = IR::RegOpnd::New(TyVar, m_func);
    IR::IntConstOpnd * srcOpnd = IR::IntConstOpnd::New(layout->ArgCount, TyInt32, m_func);

    IR::Instr * instr = nullptr;
    StackSym * symDst = nullptr;
    IR::SymOpnd * argDst = nullptr;
    IR::AddrOpnd * addrOpnd = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::I_StartCall:
        instr = IR::Instr::New(Js::OpCode::StartCall, dstOpnd, srcOpnd, m_func);

        AddInstr(instr, offset);

        // save this so we can calculate arg offsets later on
        m_argOffsetStack->Push(layout->ArgCount);
        m_argStack->Push(instr);
        break;

    case Js::OpCodeAsmJs::StartCall:
        instr = IR::Instr::New(Js::OpCode::StartCall, dstOpnd, srcOpnd, m_func);

        AddInstr(instr, offset);

        m_argStack->Push(instr);

        // also need to add undefined as arg0
        addrOpnd = IR::AddrOpnd::New(m_func->GetScriptContextInfo()->GetUndefinedAddr(), IR::AddrOpndKindDynamicVar, m_func, true);
        addrOpnd->SetValueType(ValueType::Undefined);

        symDst = m_func->m_symTable->GetArgSlotSym(1);

        argDst = IR::SymOpnd::New(symDst, TyVar, m_func);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, argDst, addrOpnd, m_func);

        AddInstr(instr, offset);
        m_argStack->Push(instr);

        break;
    default:
        Assume(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildAsmTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_AsmTypedArr<SizePolicy>>();
    BuildAsmTypedArr(newOpcode, offset, layout->SlotIndex, layout->Value, layout->ViewType);
}

void
IRBuilderAsmJs::BuildAsmTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, int8 viewType)
{
    IRType type = TyInt32;
    Js::RegSlot valueRegSlot = Js::Constants::NoRegister;
    bool isLd = newOpcode == Js::OpCodeAsmJs::LdArr || newOpcode == Js::OpCodeAsmJs::LdArrConst;
    Js::OpCode op = Js::OpCode::InvalidOpCode;
    ValueType arrayType;
    switch (viewType)
    {
    case Js::ArrayBufferView::TYPE_INT8:
        valueRegSlot = GetRegSlotFromIntReg(value);
        arrayType = ValueType::GetObject(ObjectType::Int8Array);
        type = TyInt8;
        op = isLd ? Js::OpCode::LdInt8ArrViewElem : Js::OpCode::StInt8ArrViewElem;
        break;
    case Js::ArrayBufferView::TYPE_UINT8:
        valueRegSlot = GetRegSlotFromIntReg(value);
        arrayType = ValueType::GetObject(ObjectType::Uint8Array);
        type = TyUint8;
        op = isLd ? Js::OpCode::LdUInt8ArrViewElem : Js::OpCode::StUInt8ArrViewElem;
        break;
    case Js::ArrayBufferView::TYPE_INT16:
        valueRegSlot = GetRegSlotFromIntReg(value);
        arrayType = ValueType::GetObject(ObjectType::Int16Array);
        type = TyInt16;
        op = isLd ? Js::OpCode::LdInt16ArrViewElem : Js::OpCode::StInt16ArrViewElem;
        break;
    case Js::ArrayBufferView::TYPE_UINT16:
        valueRegSlot = GetRegSlotFromIntReg(value);
        arrayType = ValueType::GetObject(ObjectType::Uint16Array);
        type = TyUint16;
        op = isLd ? Js::OpCode::LdUInt16ArrViewElem : Js::OpCode::StUInt16ArrViewElem;
        break;
    case Js::ArrayBufferView::TYPE_INT32:
        valueRegSlot = GetRegSlotFromIntReg(value);
        arrayType = ValueType::GetObject(ObjectType::Int32Array);
        type = TyInt32;
        op = isLd ? Js::OpCode::LdInt32ArrViewElem : Js::OpCode::StInt32ArrViewElem;
        break;
    case Js::ArrayBufferView::TYPE_UINT32:
        valueRegSlot = GetRegSlotFromIntReg(value);
        arrayType = ValueType::GetObject(ObjectType::Uint32Array);
        type = TyUint32;
        op = isLd ? Js::OpCode::LdUInt32ArrViewElem : Js::OpCode::StUInt32ArrViewElem;
        break;
    case Js::ArrayBufferView::TYPE_FLOAT32:
        valueRegSlot = GetRegSlotFromFloatReg(value);
        arrayType = ValueType::GetObject(ObjectType::Float32Array);
        type = TyFloat32;
        op = isLd ? Js::OpCode::LdFloat32ArrViewElem : Js::OpCode::StFloat32ArrViewElem;
        break;
    case Js::ArrayBufferView::TYPE_FLOAT64:
        valueRegSlot = GetRegSlotFromDoubleReg(value);
        arrayType = ValueType::GetObject(ObjectType::Float64Array);
        type = TyFloat64;
        op = isLd ? Js::OpCode::LdFloat64ArrViewElem : Js::OpCode::StFloat64ArrViewElem;
        break;
    default:
        Assume(UNREACHED);
    }
    IR::Instr * instr = nullptr;
    IR::Instr * maskInstr = nullptr;
    IR::RegOpnd * regOpnd = nullptr;
    IR::IndirOpnd * indirOpnd = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::LdArr || newOpcode == Js::OpCodeAsmJs::StArr)
    {
        uint32 mask = 0;
        switch (type)
        {
        case TyInt8:
        case TyUint8:
            // don't need to mask
            break;
        case TyInt16:
        case TyUint16:
            mask = (uint32)~1;
            break;
        case TyInt32:
        case TyUint32:
        case TyFloat32:
            mask = (uint32)~3;
            break;
        case TyFloat64:
            mask = (uint32)~7;
            break;
        default:
            Assume(UNREACHED);
        }
        Js::RegSlot indexRegSlot = GetRegSlotFromIntReg(slotIndex);
        IR::RegOpnd * maskedOpnd = nullptr;
        if (mask)
        {
            maskedOpnd = IR::RegOpnd::New(TyUint32, m_func);
            maskInstr = IR::Instr::New(Js::OpCode::And_I4, maskedOpnd, BuildSrcOpnd(indexRegSlot, TyInt32), IR::IntConstOpnd::New(mask, TyUint32, m_func), m_func);
        }
        else
        {
            maskedOpnd = BuildSrcOpnd(indexRegSlot, TyInt32);
        }
        indirOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::BufferReg, TyVar), maskedOpnd, type, m_func);
        indirOpnd->GetBaseOpnd()->SetValueType(arrayType);

    }
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::LdArr:
        if (IRType_IsFloat(type))
        {
            regOpnd = BuildDstOpnd(valueRegSlot, type);
            regOpnd->SetValueType(ValueType::Float);
        }
        else
        {
            regOpnd = BuildDstOpnd(valueRegSlot, TyInt32);
            regOpnd->SetValueType(ValueType::GetInt(false));
        }
        instr = IR::Instr::New(op, regOpnd, indirOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdArrConst:
        if (IRType_IsFloat(type))
        {
            regOpnd = BuildDstOpnd(valueRegSlot, type);
            regOpnd->SetValueType(ValueType::Float);
        }
        else
        {
            regOpnd = BuildDstOpnd(valueRegSlot, TyInt32);
            regOpnd->SetValueType(ValueType::GetInt(false));
        }

        indirOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::BufferReg, TyVar), slotIndex, type, m_func);
        indirOpnd->GetBaseOpnd()->SetValueType(arrayType);
        instr = IR::Instr::New(op, regOpnd, indirOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::StArr:

        if (IRType_IsFloat(type))
        {
            regOpnd = BuildSrcOpnd(valueRegSlot, type);
            regOpnd->SetValueType(ValueType::Float);
        }
        else
        {
            regOpnd = BuildSrcOpnd(valueRegSlot, TyInt32);
            regOpnd->SetValueType(ValueType::GetInt(false));
        }
        instr = IR::Instr::New(op, indirOpnd, regOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::StArrConst:
        if (IRType_IsFloat(type))
        {
            regOpnd = BuildSrcOpnd(valueRegSlot, type);
            regOpnd->SetValueType(ValueType::Float);
        }
        else
        {
            regOpnd = BuildSrcOpnd(valueRegSlot, TyInt32);
            regOpnd->SetValueType(ValueType::GetInt(false));
        }

        indirOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::BufferReg, TyVar), slotIndex, type, m_func);
        indirOpnd->GetBaseOpnd()->SetValueType(arrayType);
        instr = IR::Instr::New(op, indirOpnd, regOpnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }
    // constant loads won't have mask instr
    if (maskInstr)
    {
        AddInstr(maskInstr, offset);
    }
#if _M_IX86
    instr->SetSrc2(BuildSrcOpnd(AsmJsRegSlots::LengthReg, TyUint32));
#endif
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_AsmCall<SizePolicy>>();
    BuildAsmCall(newOpcode, offset, layout->ArgCount, layout->Return, layout->Function, layout->ReturnType);
}

void
IRBuilderAsmJs::BuildAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::ArgSlot argCount, Js::RegSlot ret, Js::RegSlot function, int8 returnType)
{
#if DBG
    int count = 0;
#endif

    IR::Instr * instr = nullptr;
    IR::RegOpnd * dstOpnd = nullptr;
    IR::Opnd * srcOpnd = nullptr;
    Js::RegSlot dstRegSlot;
    Js::RegSlot srcRegSlot;
    int32 argOffset = 0;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::I_Call:
        srcRegSlot = GetRegSlotFromVarReg(function);
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyVar);

        switch ((Js::AsmJsRetType::Which)returnType)
        {
        case Js::AsmJsRetType::Which::Signed:
            dstRegSlot = GetRegSlotFromIntReg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
            dstOpnd->SetValueType(ValueType::GetInt(false));
            break;

        case Js::AsmJsRetType::Which::Float:
            dstRegSlot = GetRegSlotFromFloatReg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
            dstOpnd->SetValueType(ValueType::Float);
            break;

        case Js::AsmJsRetType::Which::Double:
            dstRegSlot = GetRegSlotFromDoubleReg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
            dstOpnd->SetValueType(ValueType::Float);
            break;

        case Js::AsmJsRetType::Which::Void:
            break;

        case Js::AsmJsRetType::Which::Float32x4:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
            break;
        case Js::AsmJsRetType::Which::Int32x4:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
            break;
        case Js::AsmJsRetType::Which::Bool32x4:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128B4);
            break;
        case Js::AsmJsRetType::Which::Bool16x8:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128B8);
            break;
        case Js::AsmJsRetType::Which::Bool8x16:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128B16);
            break;
        case Js::AsmJsRetType::Which::Float64x2:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
            break;
        case Js::AsmJsRetType::Which::Int16x8:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I8);
            break;
        case Js::AsmJsRetType::Which::Int8x16:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I16);
            break;
        case Js::AsmJsRetType::Which::Uint32x4:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U4);
            break;
        case Js::AsmJsRetType::Which::Uint16x8:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U8);
            break;
        case Js::AsmJsRetType::Which::Uint8x16:
            dstRegSlot = GetRegSlotFromSimd128Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U16);
            break;
            default:
            Assume(UNREACHED);
        }

        instr = IR::Instr::New(Js::OpCode::AsmJsCallI, m_func);
        instr->SetSrc1(srcOpnd);
        if (dstOpnd)
        {
            instr->SetDst(dstOpnd);
        }

        argOffset = m_argOffsetStack->Pop();
        argOffset -= MachPtr;
        break;

    case Js::OpCodeAsmJs::Call:
        srcRegSlot = GetRegSlotFromVarReg(function);
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyVar);

        dstRegSlot = GetRegSlotFromVarReg(ret);
        dstOpnd = BuildDstOpnd(dstRegSlot, TyVar);

        instr = IR::Instr::New(Js::OpCode::AsmJsCallE, dstOpnd, srcOpnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);

    IR::Instr * argInstr = nullptr;
    IR::Instr * prevInstr = instr;


    for (argInstr = m_argStack->Pop(); argInstr && argInstr->m_opcode != Js::OpCode::StartCall; argInstr = m_argStack->Pop())
    {
        if (newOpcode == Js::OpCodeAsmJs::I_Call)
        {
#if _M_IX86
            argOffset -= argInstr->GetDst()->GetSize();
#elif _M_X64
            argOffset -= (argInstr->GetDst()->GetSize() <= MachPtr ? MachPtr : argInstr->GetDst()->GetSize());
#else
            Assert(UNREACHED);
#endif
            argInstr->GetDst()->GetStackSym()->m_offset = argOffset;

        }
        // associate the ArgOuts with this call via src2
        prevInstr->SetSrc2(argInstr->GetDst());
        prevInstr = argInstr;

#if defined(_M_X64)
        if (m_func->IsSIMDEnabled())
        {
            m_tempList->Push(argInstr);
        }
#endif

#if DBG
        count++;
#endif
    }
    Assert(argOffset == 0);
    AnalysisAssert(argInstr);
    prevInstr->SetSrc2(argInstr->GetDst());

#if defined(_M_X64)
    // Without SIMD vars, all args are Var in size. So offset in Var = arg position in args list.
    // With SIMD, args have variable size, so we need to track argument position in the args list to be able to assign arg register for first four args on x64.
    if (m_func->IsSIMDEnabled())
    {
        for (uint i = 1; !m_tempList->Empty(); i++)
        {
            IR::Instr * instrArg = m_tempList->Pop();
            // record argument position and make room for implicit args
            instrArg->GetDst()->GetStackSym()->m_argPosition = i;
            if (newOpcode == Js::OpCodeAsmJs::I_Call)
            {
                // implicit func obj arg
                instrArg->GetDst()->GetStackSym()->m_argPosition += 1;
            }
            else
            {
                // implicit func obj + callInfo args
                Assert(newOpcode == Js::OpCodeAsmJs::Call);
                instrArg->GetDst()->GetStackSym()->m_argPosition += 2;
            }
        }
    }
#endif

    if (m_func->m_argSlotsForFunctionsCalled < argCount)
    {
        m_func->m_argSlotsForFunctionsCalled = argCount;
    }
    if (m_asmFuncInfo->UsesHeapBuffer())
    {
        // if heap buffer can change, then we will insert reload after each call
        if (!m_asmFuncInfo->IsHeapBufferConst())
        {
            BuildHeapBufferReload(offset);
        }
        // after foreign function call, we need to make sure that the heap hasn't been detached
        if (newOpcode == Js::OpCodeAsmJs::Call)
        {
            IR::Instr * instrArrayDetachedCheck = IR::Instr::New(Js::OpCode::ArrayDetachedCheck, m_func);
            instrArrayDetachedCheck->SetSrc1(IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::ArrayReg, TyVar), Js::ArrayBuffer::GetIsDetachedOffset(), TyInt8, m_func));
            AddInstr(instrArrayDetachedCheck, offset);
        }
    }
}

void
IRBuilderAsmJs::BuildAsmBr(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(!OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));

    const unaligned Js::OpLayoutAsmBr * branchInsn = m_jnReader.AsmBr();
    uint targetOffset = m_jnReader.GetCurrentOffset() + branchInsn->RelativeJumpOffset;

    if (newOpcode == Js::OpCodeAsmJs::EndSwitch_Int)
    {
        m_switchBuilder.EndSwitch(offset, targetOffset);
        return;
    }

    IR::BranchInstr * branchInstr = IR::BranchInstr::New(Js::OpCode::Br, NULL, m_func);
    AddBranchInstr(branchInstr, offset, targetOffset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildAsmReg1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_AsmReg1<SizePolicy>>();
    BuildAsmReg1(newOpcode, offset, layout->R0);
}

void
IRBuilderAsmJs::BuildAsmReg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg)
{
    Assert(newOpcode == Js::OpCodeAsmJs::LdUndef);

    Js::RegSlot dstRegSlot = GetRegSlotFromVarReg(dstReg);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyVar);

    if (dstOpnd->m_sym->m_isSingleDef)
    {
        dstOpnd->m_sym->m_isConst = true;
        dstOpnd->m_sym->m_isNotInt = true;
    }

    IR::AddrOpnd * addrOpnd = IR::AddrOpnd::New(m_func->GetScriptContextInfo()->GetUndefinedAddr(), IR::AddrOpndKindDynamicVar, m_func, true);
    addrOpnd->SetValueType(ValueType::Undefined);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, addrOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Double1<SizePolicy>>();
    BuildInt1Double1(newOpcode, offset, layout->I0, layout->D1);
}

void
IRBuilderAsmJs::BuildInt1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstIntReg, Js::RegSlot srcDoubleReg)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_DTI);

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dstIntReg);
    Js::RegSlot srcRegSlot = GetRegSlotFromDoubleReg(srcDoubleReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Float1<SizePolicy>>();
    BuildInt1Float1(newOpcode, offset, layout->I0, layout->F1);
}

void
IRBuilderAsmJs::BuildInt1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstIntReg, Js::RegSlot srcFloatReg)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_FTI);

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dstIntReg);
    Js::RegSlot srcRegSlot = GetRegSlotFromFloatReg(srcFloatReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat32);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildDouble1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Double1Int1<SizePolicy>>();
    BuildDouble1Int1(newOpcode, offset, layout->D0, layout->I1);
}

void
IRBuilderAsmJs::BuildDouble1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstDoubleReg, Js::RegSlot srcIntReg)
{
    Js::RegSlot srcRegSlot = GetRegSlotFromIntReg(srcIntReg);
    Js::RegSlot dstRegSlot = GetRegSlotFromDoubleReg(dstDoubleReg);

    IR::RegOpnd * srcOpnd = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_ITD:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
        break;

    case Js::OpCodeAsmJs::Conv_UTD:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyUint32);
        break;

    default:
        Assume(UNREACHED);
    }
    srcOpnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildDouble1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Double1Float1<SizePolicy>>();
    BuildDouble1Float1(newOpcode, offset, layout->D0, layout->F1);
}

void
IRBuilderAsmJs::BuildDouble1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstDoubleReg, Js::RegSlot srcFloatReg)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_FTD);

    Js::RegSlot dstRegSlot = GetRegSlotFromDoubleReg(dstDoubleReg);
    Js::RegSlot srcRegSlot = GetRegSlotFromFloatReg(srcFloatReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat32);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float1Reg1<SizePolicy>>();
    BuildFloat1Reg1(newOpcode, offset, layout->F0, layout->R1);
}

void
IRBuilderAsmJs::BuildFloat1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstFloatReg, Js::RegSlot srcVarReg)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_VTF);

    Js::RegSlot srcRegSlot = GetRegSlotFromVarReg(srcVarReg);
    Js::RegSlot dstRegSlot = GetRegSlotFromFloatReg(dstFloatReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyVar);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::FromVar, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildDouble1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Double1Reg1<SizePolicy>>();
    BuildDouble1Reg1(newOpcode, offset, layout->D0, layout->R1);
}

void
IRBuilderAsmJs::BuildDouble1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstDoubleReg, Js::RegSlot srcVarReg)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_VTD);
    Js::RegSlot srcRegSlot = GetRegSlotFromVarReg(srcVarReg);
    Js::RegSlot dstRegSlot = GetRegSlotFromDoubleReg(dstDoubleReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyVar);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::FromVar, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Reg1<SizePolicy>>();
    BuildInt1Reg1(newOpcode, offset, layout->I0, layout->R1);
}

void
IRBuilderAsmJs::BuildInt1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstIntReg, Js::RegSlot srcVarReg)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_VTI);

    Js::RegSlot srcRegSlot = GetRegSlotFromVarReg(srcVarReg);
    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dstIntReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyVar);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = IR::Instr::New(Js::OpCode::FromVar, dstOpnd, srcOpnd, m_func);

    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildReg1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Double1<SizePolicy>>();
    BuildReg1Double1(newOpcode, offset, layout->R0, layout->D1);
}

void
IRBuilderAsmJs::BuildReg1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcDoubleReg)
{
    Js::RegSlot srcRegSlot = GetRegSlotFromDoubleReg(srcDoubleReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    IR::Opnd * tmpDst = nullptr;
    StackSym * symDst = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::ArgOut_Db:
        symDst = m_func->m_symTable->GetArgSlotSym((uint16)(dstReg+1));
        if ((uint16)(dstReg + 1) != (dstReg + 1))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        tmpDst = IR::RegOpnd::New(StackSym::New(m_func), TyVar, m_func);

        instr = IR::Instr::New(Js::OpCode::ToVar, tmpDst, srcOpnd, m_func);
        AddInstr(instr, offset);

        dstOpnd = IR::SymOpnd::New(symDst, TyVar, m_func);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, tmpDst, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
        break;

    case Js::OpCodeAsmJs::I_ArgOut_Db:
        symDst = StackSym::NewArgSlotSym((uint16)dstReg, m_func, TyFloat64);
        symDst->m_allocated = true;
        if ((uint16)(dstReg) != (dstReg))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        dstOpnd = IR::SymOpnd::New(symDst, TyFloat64, m_func);
        dstOpnd->SetValueType(ValueType::Float);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
        break;

    default:
        Assume(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildReg1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Float1<SizePolicy>>();
    BuildReg1Float1(newOpcode, offset, layout->R0, layout->F1);
}

void
IRBuilderAsmJs::BuildReg1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcFloatReg)
{
    Js::RegSlot srcRegSlot = GetRegSlotFromFloatReg(srcFloatReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat32);
    srcOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::I_ArgOut_Flt:
    {
        StackSym * symDst = StackSym::NewArgSlotSym((uint16)dstReg, m_func, TyFloat32);
        symDst->m_allocated = true;
        if ((uint16)(dstReg) != (dstReg))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        dstOpnd = IR::SymOpnd::New(symDst, TyFloat32, m_func);
        dstOpnd->SetValueType(ValueType::Float);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
        break;
    }
    default:
        Assume(UNREACHED);
    }
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildReg1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Int1<SizePolicy>>();
    BuildReg1Int1(newOpcode, offset, layout->R0, layout->I1);
}

void
IRBuilderAsmJs::BuildReg1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcIntReg)
{
    Js::RegSlot srcRegSlot = GetRegSlotFromIntReg(srcIntReg);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
    srcOpnd->SetValueType(ValueType::GetInt(false));

    IR::Opnd * dstOpnd;
    IR::Opnd * tmpDst;
    IR::Instr * instr;
    StackSym * symDst;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::ArgOut_Int:
        symDst = m_func->m_symTable->GetArgSlotSym((uint16)(dstReg + 1));
        if ((uint16)(dstReg + 1) != (dstReg + 1))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        tmpDst = IR::RegOpnd::New(StackSym::New(m_func), TyVar, m_func);

        instr = IR::Instr::New(Js::OpCode::ToVar, tmpDst, srcOpnd, m_func);
        AddInstr(instr, offset);

        dstOpnd = IR::SymOpnd::New(symDst, TyVar, m_func);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, tmpDst, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
        break;

    case Js::OpCodeAsmJs::I_ArgOut_Int:
        symDst = StackSym::NewArgSlotSym((uint16)dstReg, m_func, TyInt32);
        symDst->m_allocated = true;
        if ((uint16)(dstReg) != (dstReg))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        dstOpnd = IR::SymOpnd::New(symDst, TyInt32, m_func);
        dstOpnd->SetValueType(ValueType::GetInt(false));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);
        m_argStack->Push(instr);
        break;

    default:
        Assume(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Const1<SizePolicy>>();
    BuildInt1Const1(newOpcode, offset, layout->I0, layout->C1);
}

void
IRBuilderAsmJs::BuildInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstInt, int constInt)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Ld_IntConst);

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dstInt);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, IR::IntConstOpnd::New(constInt, TyInt32, m_func), m_func);

    if (dstOpnd->m_sym->IsSingleDef())
    {
        dstOpnd->m_sym->SetIsIntConst(constInt);
    }

    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Double2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Double2<SizePolicy>>();
    BuildInt1Double2(newOpcode, offset, layout->I0, layout->D1, layout->D2);
}

void
IRBuilderAsmJs::BuildInt1Double2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dst);
    Js::RegSlot src1RegSlot = GetRegSlotFromDoubleReg(src1);
    Js::RegSlot src2RegSlot = GetRegSlotFromDoubleReg(src2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat64);
    src2Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::CmLt_Db:
        instr = IR::Instr::New(Js::OpCode::CmLt_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmLe_Db:
        instr = IR::Instr::New(Js::OpCode::CmLe_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmGt_Db:
        instr = IR::Instr::New(Js::OpCode::CmGt_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmGe_Db:
        instr = IR::Instr::New(Js::OpCode::CmGe_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmEq_Db:
        instr = IR::Instr::New(Js::OpCode::CmEq_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmNe_Db:
        instr = IR::Instr::New(Js::OpCode::CmNeq_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Float2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Float2<SizePolicy>>();
    BuildInt1Float2(newOpcode, offset, layout->I0, layout->F1, layout->F2);
}

void
IRBuilderAsmJs::BuildInt1Float2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dst);
    Js::RegSlot src1RegSlot = GetRegSlotFromFloatReg(src1);
    Js::RegSlot src2RegSlot = GetRegSlotFromFloatReg(src2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat32);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat32);
    src2Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::CmLt_Flt:
        instr = IR::Instr::New(Js::OpCode::CmLt_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmLe_Flt:
        instr = IR::Instr::New(Js::OpCode::CmLe_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmGt_Flt:
        instr = IR::Instr::New(Js::OpCode::CmGt_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmGe_Flt:
        instr = IR::Instr::New(Js::OpCode::CmGe_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmEq_Flt:
        instr = IR::Instr::New(Js::OpCode::CmEq_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::CmNe_Flt:
        instr = IR::Instr::New(Js::OpCode::CmNeq_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }

    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int2<SizePolicy>>();
    BuildInt2(newOpcode, offset, layout->I0, layout->I1);
}

void
IRBuilderAsmJs::BuildInt2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dst);
    Js::RegSlot srcRegSlot = GetRegSlotFromIntReg(src);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
    srcOpnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::BeginSwitch_Int:
        m_switchBuilder.BeginSwitch();
        // fall-through
    case Js::OpCodeAsmJs::Ld_Int:
        instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Neg_Int:
        instr = IR::Instr::New(Js::OpCode::Neg_I4, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Not_Int:
        instr = IR::Instr::New(Js::OpCode::Not_I4, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LogNot_Int:
        instr = IR::Instr::New(Js::OpCode::CmEq_I4, dstOpnd, srcOpnd, IR::IntConstOpnd::New(0, TyInt32, m_func), m_func);
        break;

    case Js::OpCodeAsmJs::Conv_ITB:
        instr = IR::Instr::New(Js::OpCode::CmNeq_I4, dstOpnd, srcOpnd, IR::IntConstOpnd::New(0, TyInt32, m_func), m_func);
        break;

    case Js::OpCodeAsmJs::Abs_Int:
        instr = IR::Instr::New(Js::OpCode::InlineMathAbs, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Clz32_Int:
        instr = IR::Instr::New(Js::OpCode::InlineMathClz32, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::I_Conv_VTI:
        instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Return_Int:
        instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, srcOpnd, m_func);
        if (m_func->IsLoopBody())
        {
            IR::Instr* slotInstr = GenerateStSlotForReturn(srcOpnd, IRType::TyInt32);
            AddInstr(slotInstr, offset);
        }

        break;

    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int3<SizePolicy>>();
    BuildInt3(newOpcode, offset, layout->I0, layout->I1, layout->I2);
}

void
IRBuilderAsmJs::BuildInt3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dst);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(src1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(src2);


    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt32);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Add_Int:
        instr = IR::Instr::New(Js::OpCode::Add_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Sub_Int:
        instr = IR::Instr::New(Js::OpCode::Sub_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Mul_UInt:
        src1Opnd->SetType(TyUint32);
        src2Opnd->SetType(TyUint32);
    case Js::OpCodeAsmJs::Mul_Int:
        instr = IR::Instr::New(Js::OpCode::Mul_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Div_UInt:
        src1Opnd->SetType(TyUint32);
        src2Opnd->SetType(TyUint32);
    case Js::OpCodeAsmJs::Div_Int:
        instr = IR::Instr::New(Js::OpCode::Div_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Rem_UInt:
        src1Opnd->SetType(TyUint32);
        src2Opnd->SetType(TyUint32);
    case Js::OpCodeAsmJs::Rem_Int:
        instr = IR::Instr::New(Js::OpCode::Rem_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::And_Int:
        instr = IR::Instr::New(Js::OpCode::And_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Or_Int:
        instr = IR::Instr::New(Js::OpCode::Or_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Xor_Int:
        instr = IR::Instr::New(Js::OpCode::Xor_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Shl_Int:
        instr = IR::Instr::New(Js::OpCode::Shl_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Shr_Int:
        instr = IR::Instr::New(Js::OpCode::Shr_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::ShrU_Int:
        instr = IR::Instr::New(Js::OpCode::ShrU_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLt_Int:
        instr = IR::Instr::New(Js::OpCode::CmLt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLe_Int:
        instr = IR::Instr::New(Js::OpCode::CmLe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGt_Int:
        instr = IR::Instr::New(Js::OpCode::CmGt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGe_Int:
        instr = IR::Instr::New(Js::OpCode::CmGe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmEq_Int:
        instr = IR::Instr::New(Js::OpCode::CmEq_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmNe_Int:
        instr = IR::Instr::New(Js::OpCode::CmNeq_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLt_UnInt:
        instr = IR::Instr::New(Js::OpCode::CmUnLt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLe_UnInt:
        instr = IR::Instr::New(Js::OpCode::CmUnLe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGt_UnInt:
        instr = IR::Instr::New(Js::OpCode::CmUnGt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGe_UnInt:
        instr = IR::Instr::New(Js::OpCode::CmUnGe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Min_Int:
        instr = IR::Instr::New(Js::OpCode::InlineMathMin, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Max_Int:
        instr = IR::Instr::New(Js::OpCode::InlineMathMax, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Imul_Int:
        instr = IR::Instr::New(Js::OpCode::InlineMathImul, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildDouble2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Double2<SizePolicy>>();
    BuildDouble2(newOpcode, offset, layout->D0, layout->D1);
}

void
IRBuilderAsmJs::BuildDouble2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromDoubleReg(dst);
    Js::RegSlot srcRegSlot = GetRegSlotFromDoubleReg(src);


    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Ld_Db:
        instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Neg_Db:
        instr = IR::Instr::New(Js::OpCode::Neg_A, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Sin_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathSin, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Cos_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathCos, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Tan_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathTan, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Asin_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathAsin, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Acos_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathAcos, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Atan_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathAtan, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Abs_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathAbs, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Ceil_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathCeil, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Floor_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathFloor, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Exp_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathExp, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Log_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathLog, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Sqrt_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathSqrt, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Return_Db:
        instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
        if (m_func->IsLoopBody())
        {
            IR::Instr* slotInstr = GenerateStSlotForReturn(srcOpnd, IRType::TyFloat64);
            AddInstr(slotInstr, offset);
        }
        break;
    case Js::OpCodeAsmJs::I_Conv_VTD:
        instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float2<SizePolicy>>();
    BuildFloat2(newOpcode, offset, layout->F0, layout->F1);
}

void
IRBuilderAsmJs::BuildFloat2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromFloatReg(dst);
    Js::RegSlot srcRegSlot = GetRegSlotFromFloatReg(src);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat32);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Ld_Flt:
        instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Neg_Flt:
        instr = IR::Instr::New(Js::OpCode::Neg_A, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Ceil_Flt:
        instr = IR::Instr::New(Js::OpCode::InlineMathCeil, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Floor_Flt:
        instr = IR::Instr::New(Js::OpCode::InlineMathFloor, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Sqrt_Flt:
        instr = IR::Instr::New(Js::OpCode::InlineMathSqrt, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Abs_Flt:
        instr = IR::Instr::New(Js::OpCode::InlineMathAbs, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Fround_Flt:
        instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Return_Flt:
        instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
        if (m_func->IsLoopBody())
        {
            IR::Instr* slotInstr = GenerateStSlotForReturn(srcOpnd, IRType::TyFloat32);
            AddInstr(slotInstr, offset);
        }
        break;
    case Js::OpCodeAsmJs::I_Conv_VTF:
        instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float3<SizePolicy>>();
    BuildFloat3(newOpcode, offset, layout->F0, layout->F1, layout->F2);
}

void
IRBuilderAsmJs::BuildFloat3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromFloatReg(dst);
    Js::RegSlot src1RegSlot = GetRegSlotFromFloatReg(src1);
    Js::RegSlot src2RegSlot = GetRegSlotFromFloatReg(src2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat32);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat32);
    src2Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Add_Flt:
        instr = IR::Instr::New(Js::OpCode::Add_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Sub_Flt:
        instr = IR::Instr::New(Js::OpCode::Sub_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Mul_Flt:
        instr = IR::Instr::New(Js::OpCode::Mul_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Div_Flt:
        instr = IR::Instr::New(Js::OpCode::Div_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float1Double1<SizePolicy>>();
    BuildFloat1Double1(newOpcode, offset, layout->F0, layout->D1);
}

void
IRBuilderAsmJs::BuildFloat1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Fround_Db);

    Js::RegSlot dstRegSlot = GetRegSlotFromFloatReg(dst);
    Js::RegSlot srcRegSlot = GetRegSlotFromDoubleReg(src);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::InlineMathFround, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float1Int1<SizePolicy>>();
    BuildFloat1Int1(newOpcode, offset, layout->F0, layout->I1);
}

void
IRBuilderAsmJs::BuildFloat1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Fround_Int);

    Js::RegSlot dstRegSlot = GetRegSlotFromFloatReg(dst);
    Js::RegSlot srcRegSlot = GetRegSlotFromIntReg(src);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
    srcOpnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildDouble3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Double3<SizePolicy>>();
    BuildDouble3(newOpcode, offset, layout->D0, layout->D1, layout->D2);
}

void
IRBuilderAsmJs::BuildDouble3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2)
{
    Js::RegSlot dstRegSlot = GetRegSlotFromDoubleReg(dst);
    Js::RegSlot src1RegSlot = GetRegSlotFromDoubleReg(src1);
    Js::RegSlot src2RegSlot = GetRegSlotFromDoubleReg(src2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat64);
    src2Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Add_Db:
        instr = IR::Instr::New(Js::OpCode::Add_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Sub_Db:
        instr = IR::Instr::New(Js::OpCode::Sub_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Mul_Db:
        instr = IR::Instr::New(Js::OpCode::Mul_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Div_Db:
        instr = IR::Instr::New(Js::OpCode::Div_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Rem_Db:
        instr = IR::Instr::New(Js::OpCode::Rem_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Pow_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathPow, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Atan2_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathAtan2, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Min_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathMin, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Max_Db:
        instr = IR::Instr::New(Js::OpCode::InlineMathMax, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBrInt1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_BrInt1<SizePolicy>>();
    BuildBrInt1(newOpcode, offset, layout->RelativeJumpOffset, layout->I1);
}

void
IRBuilderAsmJs::BuildBrInt1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src)
{
    Assert(newOpcode == Js::OpCodeAsmJs::BrTrue_Int);

    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(src);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt32);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    uint targetOffset = m_jnReader.GetCurrentOffset() + relativeOffset;

    IR::BranchInstr * branchInstr = IR::BranchInstr::New(Js::OpCode::BrTrue_I4, nullptr, src1Opnd, m_func);
    AddBranchInstr(branchInstr, offset, targetOffset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBrInt2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_BrInt2<SizePolicy>>();
    BuildBrInt2(newOpcode, offset, layout->RelativeJumpOffset, layout->I1, layout->I2);
}

void
IRBuilderAsmJs::BuildBrInt2(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, Js::RegSlot src2)
{
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(src1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(src2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt32);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    uint targetOffset = m_jnReader.GetCurrentOffset() + relativeOffset;

    if (newOpcode == Js::OpCodeAsmJs::Case_Int)
    {
        // branches for cases are generated entirely by the switch builder
        m_switchBuilder.OnCase(
            src1Opnd,
            src2Opnd,
            offset,
            targetOffset);
    }
    else
    {
        Assert(newOpcode == Js::OpCodeAsmJs::BrEq_Int);
        IR::BranchInstr * branchInstr = IR::BranchInstr::New(Js::OpCode::BrEq_I4, nullptr, src1Opnd, src2Opnd, m_func);
        AddBranchInstr(branchInstr, offset, targetOffset);
    }
}

///Loop Body Code

bool
IRBuilderAsmJs::IsLoopBody() const
{
    return m_func->IsLoopBody();
}

bool
IRBuilderAsmJs::IsLoopBodyReturnIPInstr(IR::Instr * instr) const
{
    IR::Opnd * dst = instr->GetDst();
    return (dst && dst->IsRegOpnd() && dst->AsRegOpnd()->m_sym == m_loopBodyRetIPSym);
}


bool
IRBuilderAsmJs::IsLoopBodyOuterOffset(uint offset) const
{
    if (!IsLoopBody())
    {
        return false;
    }

    return (offset >= m_func->GetWorkItem()->GetLoopHeader()->endOffset || offset < m_func->GetWorkItem()->GetLoopHeader()->startOffset);
}

uint
IRBuilderAsmJs::GetLoopBodyExitInstrOffset() const
{
    // End of loop body, start of StSlot and Ret instruction at endOffset + 1
    return m_func->GetWorkItem()->GetLoopHeader()->endOffset + 1;
}

IR::Instr *
IRBuilderAsmJs::CreateLoopBodyReturnIPInstr(uint targetOffset, uint offset)
{
    IR::RegOpnd * retOpnd = IR::RegOpnd::New(m_loopBodyRetIPSym, TyInt32, m_func);
    IR::IntConstOpnd * exitOffsetOpnd = IR::IntConstOpnd::New(targetOffset, TyInt32, m_func);
    return IR::Instr::New(Js::OpCode::Ld_I4, retOpnd, exitOffsetOpnd, m_func);
}


IR::Opnd *
IRBuilderAsmJs::InsertLoopBodyReturnIPInstr(uint targetOffset, uint offset)
{
    IR::Instr * setRetValueInstr = CreateLoopBodyReturnIPInstr(targetOffset, offset);
    this->AddInstr(setRetValueInstr, offset);
    return setRetValueInstr->GetDst();
}

IR::SymOpnd *
IRBuilderAsmJs::BuildAsmJsLoopBodySlotOpnd(SymID symId, IRType opndType)
{
    // Get the interpreter frame instance that was passed in.
    StackSym *loopParamSym = m_func->EnsureLoopParamSym();

    // Compute the offset of the start of the locals array as a Var index.
    size_t localsOffset = 0;
    if (!m_IsTJLoopBody)
    {
        localsOffset = Js::InterpreterStackFrame::GetOffsetOfLocals();
    }
    Assert(localsOffset % sizeof(AsmJsSIMDValue) == 0);
    Js::PropertyId propOffSet = 0;
    IRType type = IRType::TyInt32;
    BOOL scale = true;
    if (RegIsIntVar(symId))
    {
        const int intOffset = m_asmFuncInfo->GetIntByteOffset() / sizeof(int);
        Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(int));

        // Get the bytecodeRegSlot
        Js::PropertyId intRegSlot = symId - m_firstIntVar + m_asmFuncInfo->GetIntConstCount();

        // Get the offset from m_localSlots
        propOffSet = (Js::PropertyId)(intRegSlot + intOffset + localsStartSlot);
        type = IRType::TyInt32;
    }
    else if (RegIsFloatVar(symId))
    {
        const int floatOffset = m_asmFuncInfo->GetFloatByteOffset() / sizeof(float);
        Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(float));

        // Get the bytecodeRegSlot
        Js::PropertyId fltRegSlot = symId - m_firstFloatVar + m_asmFuncInfo->GetFloatConstCount();

        // Get the offset from m_localSlots
        propOffSet = (Js::PropertyId)(fltRegSlot + floatOffset + localsStartSlot);
        type = IRType::TyFloat32;
    }
    else if (RegIsDoubleVar(symId))
    {
        const int doubleOffset = m_asmFuncInfo->GetDoubleByteOffset() / sizeof(double);
        Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(double));

        // Get the bytecodeRegSlot
        Js::PropertyId dbRegSlot = symId - m_firstDoubleVar + m_asmFuncInfo->GetDoubleConstCount();

        // Get the offset from m_localSlots
        propOffSet = (Js::PropertyId)(dbRegSlot + doubleOffset + localsStartSlot);

        type = IRType::TyFloat64;
    }
    else if (RegIsSimd128Var(symId))
    {
        // SimdByteOffset is not guaranteed to be divisible by Simd size. So we do computation in bytes.
        const int simdOffset = m_asmFuncInfo->GetSimdByteOffset();
        Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset);

        // Get the bytecodeRegSlot
        Js::PropertyId dbRegSlot = symId - m_firstSimdVar + m_asmFuncInfo->GetSimdConstCount();
        // Get the offset from m_localSlots
        propOffSet = (Js::PropertyId)(dbRegSlot * sizeof(AsmJsSIMDValue) + simdOffset + localsStartSlot);
        type = opndType;
        scale = false;
    }
    else
    {
        Assert(UNREACHED);
    }
    if (scale)
    {
        // property ID is the offset
        propOffSet = propOffSet * TySize[type];
    }


    PropertySym * fieldSym = PropertySym::FindOrCreate(loopParamSym->m_id, propOffSet, (uint32)-1, (uint)-1, PropertyKindLocalSlots, m_func);
    return IR::SymOpnd::New(fieldSym, type, m_func);
}


void
IRBuilderAsmJs::EnsureLoopBodyAsmJsLoadSlot(SymID symId, IRType type)
{
    if (this->m_ldSlots->TestAndSet(symId))
    {
        return;
    }

    IR::SymOpnd * fieldSymOpnd = this->BuildAsmJsLoopBodySlotOpnd(symId, type);

    StackSym * symDst = StackSym::FindOrCreate(symId, (Js::RegSlot)symId, m_func, fieldSymOpnd->GetType());
    IR::RegOpnd * dstOpnd = IR::RegOpnd::New(symDst, symDst->GetType(), m_func);
    IR::Instr * ldSlotInstr = IR::Instr::New(Js::OpCode::LdSlot, dstOpnd, fieldSymOpnd, m_func);

    m_func->m_headInstr->InsertAfter(ldSlotInstr);
    if (m_lastInstr == m_func->m_headInstr)
    {
        m_lastInstr = ldSlotInstr;
    }
}


void
IRBuilderAsmJs::GenerateLoopBodySlotAccesses(uint offset)
{
    //
    // The interpreter instance is passed as 0th argument to the JITted loop body function.
    // Always load the argument, then use it to generate any necessary store-slots.
    //
    uint16      argument = 0;

    StackSym *symSrc = StackSym::NewParamSlotSym(argument + 1, m_func);
    symSrc->m_offset = (argument + LowererMD::GetFormalParamOffset()) * MachPtr;
    symSrc->m_allocated = true;
    m_func->SetHasImplicitParamLoad();
    IR::SymOpnd *srcOpnd = IR::SymOpnd::New(symSrc, TyMachPtr, m_func);

    StackSym *loopParamSym = m_func->EnsureLoopParamSym();
    IR::RegOpnd *loopParamOpnd = IR::RegOpnd::New(loopParamSym, TyMachPtr, m_func);

    IR::Instr *instrArgIn = IR::Instr::New(Js::OpCode::ArgIn_A, loopParamOpnd, srcOpnd, m_func);
    m_func->m_headInstr->InsertAfter(instrArgIn);

    GenerateLoopBodyStSlots(loopParamSym->m_id, offset);
}

void
IRBuilderAsmJs::GenerateLoopBodyStSlots(SymID loopParamSymId, uint offset)
{
    if (this->m_stSlots->Count() == 0)
    {
        return;
    }

    // Compute the offset to the start of the interpreter frame's locals array as a Var index.
    size_t localsOffset = 0;
    if (!m_IsTJLoopBody)
    {
        localsOffset  = Js::InterpreterStackFrame::GetOffsetOfLocals();
    }
    Assert(localsOffset % sizeof(AsmJsSIMDValue) == 0);
    // Offset m_localSlot
    const int intOffset = m_asmFuncInfo->GetIntByteOffset() / sizeof(int);
    const int doubleOffset = m_asmFuncInfo->GetDoubleByteOffset() / sizeof(double);
    const int floatOffset = m_asmFuncInfo->GetFloatByteOffset() / sizeof(float);
    BOOL scale = true;

    FOREACH_BITSET_IN_FIXEDBV(regSlot, this->m_stSlots)
    {
        Assert(!this->RegIsConstant((Js::RegSlot)regSlot));
        Js::PropertyId propOffSet = 0;
        IRType type = IRType::TyInt32;
        IR::RegOpnd * regOpnd = nullptr;
        if (RegIsIntVar(regSlot))
        {
            Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(int));

            // Get the bytecodeRegSlot
            Js::PropertyId intRegSlot = regSlot - m_firstIntVar + m_asmFuncInfo->GetIntConstCount();

            // Get the offset from m_localSlots
            propOffSet = (Js::PropertyId)(intRegSlot + intOffset + localsStartSlot);

            type = IRType::TyInt32;
            regOpnd = this->BuildSrcOpnd((Js::RegSlot)regSlot, type);
            regOpnd->SetValueType(ValueType::GetInt(false));
        }
        else if (RegIsFloatVar(regSlot))
        {
            Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(float));

            // Get the bytecodeRegSlot
            Js::PropertyId fltRegSlot = regSlot - m_firstFloatVar + m_asmFuncInfo->GetFloatConstCount();

            // Get the offset from m_localSlots
            propOffSet = (Js::PropertyId)(fltRegSlot + floatOffset + localsStartSlot);

            type = IRType::TyFloat32;
            regOpnd = this->BuildSrcOpnd((Js::RegSlot)regSlot, type);
            regOpnd->SetValueType(ValueType::Float);
        }
        else if (RegIsDoubleVar(regSlot))
        {
            Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(double));

            // Get the bytecodeRegSlot
            Js::PropertyId dbRegSlot = regSlot - m_firstDoubleVar + m_asmFuncInfo->GetDoubleConstCount();

            // Get the bytecodeRegSlot and Get the offset from m_localSlots
            propOffSet = (Js::PropertyId)(dbRegSlot + doubleOffset + localsStartSlot);

            type = IRType::TyFloat64;
            // for double change the offset to ensure we don't have the same propertyId as the ints or floats
            // this will be corrected in the lowering code and the right offset will be used

            regOpnd = this->BuildSrcOpnd((Js::RegSlot)regSlot, type);
            regOpnd->SetValueType(ValueType::Float);
        }
        else if (RegIsSimd128Var(regSlot))
        {
            Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset);
            int simdOffset = m_asmFuncInfo->GetSimdByteOffset();
            // Get the bytecodeRegSlot
            Js::PropertyId dbRegSlot = regSlot - m_firstSimdVar + m_asmFuncInfo->GetSimdConstCount();

            // Get the bytecodeRegSlot and Get the offset from m_localSlots
            propOffSet = (Js::PropertyId)(dbRegSlot * sizeof(AsmJsSIMDValue) + simdOffset + localsStartSlot);

            // SIMD regs are non-typed. There is no way to know the incoming SIMD type to a StSlot after a loop body, so we pick any type.
            // However, at this point all src syms are already defined and assigned a type.
            type = IRType::TySimd128F4;
            regOpnd = this->BuildSrcOpnd((Js::RegSlot)regSlot, type);

            regOpnd->SetValueType(ValueType::GetObject(ObjectType::UninitializedObject));
            scale = false;
        }
        else
        {
            AnalysisAssert(UNREACHED);
        }

        if (scale)
        {
            // we will use the actual offset as the propertyId
            propOffSet = propOffSet * TySize[type];
        }
        PropertySym * fieldSym = PropertySym::FindOrCreate(loopParamSymId, propOffSet, (uint32)-1, (uint)-1, PropertyKindLocalSlots, m_func);

        IR::SymOpnd * fieldSymOpnd = IR::SymOpnd::New(fieldSym, regOpnd->GetType(), m_func);
        Js::OpCode opcode = Js::OpCode::StSlot;
        IR::Instr * stSlotInstr = IR::Instr::New(opcode, fieldSymOpnd, regOpnd, m_func);
        this->AddInstr(stSlotInstr, offset);
    }
    NEXT_BITSET_IN_FIXEDBV;
}
IR::Instr* IRBuilderAsmJs::GenerateStSlotForReturn(IR::RegOpnd* srcOpnd, IRType retType)
{
    // Compute the offset to the start of the interpreter frame's locals array as a Var index.
    size_t localsOffset = 0;
    if (!m_IsTJLoopBody)
    {
        localsOffset = Js::InterpreterStackFrame::GetOffsetOfLocals();
    }
    Assert(localsOffset % sizeof(AsmJsSIMDValue) == 0);
    StackSym *loopParamSym = m_func->EnsureLoopParamSym();

    Js::PropertyId propOffSet = 0;
    // Offset m_localSlot
    const int intOffset = m_asmFuncInfo->GetIntByteOffset() / sizeof(int);
    const int doubleOffset = m_asmFuncInfo->GetDoubleByteOffset() / sizeof(double);
    const int floatOffset = m_asmFuncInfo->GetFloatByteOffset() / sizeof(float);
    const int simdOffset = m_asmFuncInfo->GetSimdByteOffset();

    BOOL scale = true;
    Js::PropertyId localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(int));
    IRType type = IRType::TyInt32;
    switch (retType)
    {
    case IRType::TyInt32:
        propOffSet = (Js::PropertyId)(intOffset + localsStartSlot);
        type = IRType::TyInt32;
        break;
    case IRType::TyFloat32:
        localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(float));
        propOffSet = (Js::PropertyId)(floatOffset + localsStartSlot);
        type = IRType::TyFloat32;
        break;
    case IRType::TyFloat64:
        localsStartSlot = (Js::PropertyId)(localsOffset / sizeof(double));
        propOffSet = (Js::PropertyId)(doubleOffset + localsStartSlot);
        type = IRType::TyFloat64;
        break;

    case IRType::TySimd128F4:
    case IRType::TySimd128I4:
    case IRType::TySimd128B4:
    case IRType::TySimd128B8:
    case IRType::TySimd128B16:
    case IRType::TySimd128D2:
    case IRType::TySimd128I8:
    case IRType::TySimd128U4:
    case IRType::TySimd128U8:
    case IRType::TySimd128U16:
    case IRType::TySimd128I16:
        localsStartSlot = (Js::PropertyId)(localsOffset);
        propOffSet = (Js::PropertyId)(simdOffset + localsStartSlot);
        type = retType;
        scale = false;
        break;

    default:
        Assume(false);

    }
    if (scale)
    {
        // we will use the actual offset as the propertyId
        propOffSet = propOffSet * TySize[type];
    }

    // Get the bytecodeRegSlot and Get the offset from m_localSlots
    PropertySym * fieldSym = PropertySym::FindOrCreate(loopParamSym->m_id, propOffSet, (uint32)-1, (uint)-1, PropertyKindLocalSlots, m_func);
    IR::SymOpnd * fieldSymOpnd = IR::SymOpnd::New(fieldSym, srcOpnd->GetType(), m_func);
    Js::OpCode opcode = Js::OpCode::StSlot;
    IR::Instr * stSlotInstr = IR::Instr::New(opcode, fieldSymOpnd, srcOpnd, m_func);
    return stSlotInstr;
}

inline Js::OpCode IRBuilderAsmJs::GetSimdOpcode(Js::OpCodeAsmJs asmjsOpcode)
{
    Js::OpCode opcode = (Js::OpCode) 0;
    Assert(IsSimd128AsmJsOpcode(asmjsOpcode));
    if (asmjsOpcode <= Js::OpCodeAsmJs::Simd128_End)
    {
        opcode =  m_simdOpcodesMap[(uint32)((Js::OpCodeAsmJs)asmjsOpcode - Js::OpCodeAsmJs::Simd128_Start)];
    }
    else
    {
        Assert(asmjsOpcode >= Js::OpCodeAsmJs::Simd128_Start_Extend && asmjsOpcode <= Js::OpCodeAsmJs::Simd128_End_Extend);
        opcode = m_simdOpcodesMap[(uint32)((Js::OpCodeAsmJs)asmjsOpcode - Js::OpCodeAsmJs::Simd128_Start_Extend) + (uint32)(Js::OpCodeAsmJs::Simd128_End - Js::OpCodeAsmJs::Simd128_Start) + 1];
    }
    Assert(IsSimd128Opcode(opcode));
    return opcode;
}

void IRBuilderAsmJs::GetSimdTypesFromAsmType(Js::AsmJsType::Which asmType, IRType *pIRType, ValueType *pValueType /* = nullptr */)
{
    IRType irType = IRType::TyVar;
    ValueType vType = ValueType::Uninitialized;

#define SIMD_TYPE_CHECK(type1, type2, type3) \
case Js::AsmJsType::Which::##type1: \
        irType = type2; \
        vType = ValueType::GetSimd128(ObjectType::##type3); \
        break;

    switch (asmType)
    {
        SIMD_TYPE_CHECK(Float32x4,  TySimd128F4,    Simd128Float32x4)
        SIMD_TYPE_CHECK(Int32x4,    TySimd128I4,    Simd128Int32x4  )
        SIMD_TYPE_CHECK(Int16x8,    TySimd128I8,    Simd128Int16x8  )
        SIMD_TYPE_CHECK(Int8x16,    TySimd128I16,   Simd128Int8x16  )
        SIMD_TYPE_CHECK(Uint32x4,   TySimd128U4,    Simd128Uint32x4 )
        SIMD_TYPE_CHECK(Uint16x8,   TySimd128U8,    Simd128Uint16x8 )
        SIMD_TYPE_CHECK(Uint8x16,   TySimd128U16,   Simd128Uint8x16 )
        SIMD_TYPE_CHECK(Bool32x4,   TySimd128B4,    Simd128Bool32x4 )
        SIMD_TYPE_CHECK(Bool16x8,   TySimd128B8,    Simd128Bool16x8 )
        SIMD_TYPE_CHECK(Bool8x16,   TySimd128B16,   Simd128Bool8x16 )
    default:
        Assert(UNREACHED);
    }
    *pIRType = irType;
    if (pValueType)
    {
        *pValueType = vType;
    }
#undef SIMD_TYPE_CHECK
}


// !!NOTE: Always build the src opnds first, before dst. So we record the use of any temps before assigning new symId for the dst temp.

// Float32x4
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_3<SizePolicy>>();

    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->F4_2);
    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool32x4_1Float32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_1Float32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->F4_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Bool32x4_1Float32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Bool32x4_1Float32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->F4_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->F4_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128F4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128F4);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_F4, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_4<SizePolicy>>();

    Js::RegSlot dstRegSlot  = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->F4_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->F4_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128F4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128F4);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);

    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_I4, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildFloat32x4_1Float4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Float4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromFloatReg(layout->F1);
    Js::RegSlot src2RegSlot = GetRegSlotFromFloatReg(layout->F2);
    Js::RegSlot src3RegSlot = GetRegSlotFromFloatReg(layout->F3);
    Js::RegSlot src4RegSlot = GetRegSlotFromFloatReg(layout->F4);


    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat32);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat32);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TyFloat32);
    IR::RegOpnd * src4Opnd = BuildSrcOpnd(src4RegSlot, TyFloat32);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);

    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src1Opnd->SetValueType(ValueType::Float);
    src2Opnd->SetValueType(ValueType::Float);
    src3Opnd->SetValueType(ValueType::Float);
    src4Opnd->SetValueType(ValueType::Float);


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_FloatsToF4, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);

}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_2Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_2Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(layout->I2);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_F4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_3Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_3Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->F4_2);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128F4);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_F4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Float1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromFloatReg(layout->F1);


    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat32);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_F4);
    Js::OpCode opcode = Js::OpCode::Simd128_Splat_F4;

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_2Float1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_2Float1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromFloatReg(layout->F2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat32);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");


    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

// Disable for now
#if 0
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Float64x2_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Float64x2_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_FromFloat64x2_F4 || opcode == Js::OpCode::Simd128_FromFloat64x2Bits_F4, "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}
#endif // 0


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Int32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4_F4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Uint32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4_F4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Int16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Uint16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Int8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128I16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_1Uint8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128U16);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Float32x4_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128F4);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_F4)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128F4);
        symDst->m_allocated = true;
        if ((uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        dstOpnd = IR::SymOpnd::New(symDst, TySimd128F4, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

/* Int32x4 */
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I4_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool32x4_1Int32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_1Int32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I4_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Bool32x4_1Int32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Bool32x4_1Int32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I4_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->I4_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128I4);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_I4, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt32x4_1Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToI4, "Unexpected opcode for this format.");
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    uint const LANES = 4;
    Js::RegSlot srcRegSlot[LANES];
    srcRegSlot[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlot[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlot[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlot[3] = GetRegSlotFromIntReg(layout->I4);
    
    BuildSimd_1Ints(newOpcode, offset, TySimd128I4, srcRegSlot, dstRegSlot, LANES);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt32x4_2Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_2Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(layout->I2);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_I4);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_I4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt32x4_3Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_3Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I4_2);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I4);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_I4);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_I4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_I4);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_2Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I4);
}
//ReplaceLane
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_I4), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Int32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Int32x4_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot  = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_I4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat32x4_2Int1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float32x4_2Int1Float1<SizePolicy>>();

    Js::RegSlot dstRegSlot  = GetRegSlotFromSimd128Reg(layout->F4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromFloatReg(layout->F3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TyFloat32);
    src3Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3

    IR::Instr* instr = nullptr;

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((opcode == Js::OpCode::Simd128_ReplaceLane_F4), "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat1Float32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float1Float32x4_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot  = GetRegSlotFromFloatReg(layout->F0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_F4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Float32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4_I4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Uint32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128U4);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Int16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Uint16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Int8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128I16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Uint8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128U16);
}

#if 0
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt32x4_1Float64x2_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int32x4_1Float64x2_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);

    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat64x2_I4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat64x2Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128D2);
}
#endif //0
template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Int32x4_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128I4);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_I4)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128I4);
        symDst->m_allocated = true;
        if ((uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128I4, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

//Int8x16
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16);
}
//
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I16_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I16);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt8x16_1Int16(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToI16, "Unexpected opcode for this format.");
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Int16<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    uint const LANES = 16;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0]  = GetRegSlotFromIntReg(layout->I1);
    srcRegSlots[1]  = GetRegSlotFromIntReg(layout->I2);
    srcRegSlots[2]  = GetRegSlotFromIntReg(layout->I3);
    srcRegSlots[3]  = GetRegSlotFromIntReg(layout->I4);
    srcRegSlots[4]  = GetRegSlotFromIntReg(layout->I5);
    srcRegSlots[5]  = GetRegSlotFromIntReg(layout->I6);
    srcRegSlots[6]  = GetRegSlotFromIntReg(layout->I7);
    srcRegSlots[7]  = GetRegSlotFromIntReg(layout->I8);
    srcRegSlots[8]  = GetRegSlotFromIntReg(layout->I9);
    srcRegSlots[9]  = GetRegSlotFromIntReg(layout->I10);
    srcRegSlots[10] = GetRegSlotFromIntReg(layout->I11);
    srcRegSlots[11] = GetRegSlotFromIntReg(layout->I12);
    srcRegSlots[12] = GetRegSlotFromIntReg(layout->I13);
    srcRegSlots[13] = GetRegSlotFromIntReg(layout->I14);
    srcRegSlots[14] = GetRegSlotFromIntReg(layout->I15);
    srcRegSlots[15] = GetRegSlotFromIntReg(layout->I16);

    BuildSimd_1Ints(newOpcode, offset, TySimd128I16, srcRegSlots, dstRegSlot, LANES);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool8x16_1Int8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool8x16_1Int8x16_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I16_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Bool8x16_1Int8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Bool8x16_1Int8x16_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I16_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->I16_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I16);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128I16);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I16);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_I16, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_I16);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16);
}
//ExtractLane ReplaceLane
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_2Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_I16), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128I16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Int8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Int8x16_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I16);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_I16), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt8x16_3Int16(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_3Int16<SizePolicy>>();

    Js::RegSlot dstRegSlot  = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I16_2);

    IR::RegOpnd * dstOpnd   = BuildDstOpnd(dstRegSlot, TySimd128I16);
    IR::RegOpnd * src1Opnd  = BuildSrcOpnd(src1RegSlot, TySimd128I16);
    IR::RegOpnd * src2Opnd  = BuildSrcOpnd(src2RegSlot, TySimd128I16);

    IR::RegOpnd* srcOpnds[16];

    srcOpnds[0] = BuildIntConstOpnd(layout->I3);
    srcOpnds[1] = BuildIntConstOpnd(layout->I4);
    srcOpnds[2] = BuildIntConstOpnd(layout->I5);
    srcOpnds[3] = BuildIntConstOpnd(layout->I6);
    srcOpnds[4] = BuildIntConstOpnd(layout->I7);
    srcOpnds[5] = BuildIntConstOpnd(layout->I8);
    srcOpnds[6] = BuildIntConstOpnd(layout->I9);
    srcOpnds[7] = BuildIntConstOpnd(layout->I10);
    srcOpnds[8] = BuildIntConstOpnd(layout->I11);
    srcOpnds[9] = BuildIntConstOpnd(layout->I12);
    srcOpnds[10] = BuildIntConstOpnd(layout->I13);
    srcOpnds[11] = BuildIntConstOpnd(layout->I14);
    srcOpnds[12] = BuildIntConstOpnd(layout->I15);
    srcOpnds[13] = BuildIntConstOpnd(layout->I16);
    srcOpnds[14] = BuildIntConstOpnd(layout->I17);
    srcOpnds[15] = BuildIntConstOpnd(layout->I18);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    
    for (int i = 0; i < 16; ++i)
    {
        instr = AddExtendedArg(srcOpnds[i], instr->GetDst()->AsRegOpnd(), offset);
    }

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_I16);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_I16;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt8x16_2Int16(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_2Int16<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I16);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I16);

    IR::RegOpnd* srcOpnds[16];

    srcOpnds[0]  = BuildIntConstOpnd(layout->I2);
    srcOpnds[1]  = BuildIntConstOpnd(layout->I3);
    srcOpnds[2]  = BuildIntConstOpnd(layout->I4);
    srcOpnds[3]  = BuildIntConstOpnd(layout->I5);
    srcOpnds[4]  = BuildIntConstOpnd(layout->I6);
    srcOpnds[5]  = BuildIntConstOpnd(layout->I7);
    srcOpnds[6]  = BuildIntConstOpnd(layout->I8);
    srcOpnds[7]  = BuildIntConstOpnd(layout->I9);
    srcOpnds[8]  = BuildIntConstOpnd(layout->I10);
    srcOpnds[9]  = BuildIntConstOpnd(layout->I11);
    srcOpnds[10] = BuildIntConstOpnd(layout->I12);
    srcOpnds[11] = BuildIntConstOpnd(layout->I13);
    srcOpnds[12] = BuildIntConstOpnd(layout->I14);
    srcOpnds[13] = BuildIntConstOpnd(layout->I15);
    srcOpnds[14] = BuildIntConstOpnd(layout->I16);
    srcOpnds[15] = BuildIntConstOpnd(layout->I17);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));
    
    instr = AddExtendedArg(src1Opnd, nullptr, offset);

    for (int i = 0; i < 16; ++i)
    {
        instr = AddExtendedArg(srcOpnds[i], instr->GetDst()->AsRegOpnd(), offset);
    }

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_I16);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_I16;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Float32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Int32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Int16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Uint32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Uint16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt8x16_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int8x16_1Uint8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128U16);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Int8x16_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128I16);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_I16)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128I16);
        symDst->m_allocated = true;
        if (symDst == NULL || (uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        dstOpnd = IR::SymOpnd::New(symDst, TySimd128I16, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int8x16));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

/* Float64x2 */
// Disabled for now
#if 0
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    Js::OpCode opcode;

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Simd128_Return_D2:
    if (m_func->IsLoopBody())
    {
        IR::Instr* slotInstr = GenerateStSlotForReturn(src1Opnd, IRType::TySimd128D2);
        AddInstr(slotInstr, offset);
    }
    opcode = Js::OpCode::Ld_A;
    break;
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTD2:
    case Js::OpCodeAsmJs::Simd128_Ld_D2:
        opcode = Js::OpCode::Ld_A;
        break;
    default:
        opcode = GetSimdOpcode(newOpcode);
    }

    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->D2_2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128D2);
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    Js::OpCode opcode;

    opcode = GetSimdOpcode(newOpcode);

    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->D2_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->D2_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128D2);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128D2);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Clamp_D2, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildFloat64x2_1Double2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_1Double2<SizePolicy >> ();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromDoubleReg(layout->D1);
    Js::RegSlot src2RegSlot = GetRegSlotFromDoubleReg(layout->D2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat64);
    src2Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_DoublesToD2, "Invalid backend SIMD opcode");
    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildFloat64x2_1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_1Double1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromDoubleReg(layout->D1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Splat_D2, "Invalid backend SIMD opcode");
    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_2Double1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_2Double1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromDoubleReg(layout->D2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");


    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(layout->I2);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_D2);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_D2;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_3Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_3Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->D2_2);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128D2);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_D2);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_D2;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_1Float32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float32x4));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_FromFloat32x4_D2 || opcode == Js::OpCode::Simd128_FromFloat32x4Bits_D2, "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_1Int32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_FromInt32x4_D2 || opcode == Js::OpCode::Simd128_FromInt32x4Bits_D2, "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildFloat64x2_1Int32x4_1Float64x2_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Float64x2_1Int32x4_1Float64x2_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->D2_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->D2_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->D2_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128D2);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128D2);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);

    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_D2, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Float64x2_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Float64x2_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->D2_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128D2);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_D2)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128D2);
        symDst->m_allocated = true;
        if ((uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }
        dstOpnd = IR::SymOpnd::New(symDst, TySimd128D2, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Float64x2));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}
#endif // 0

/* Int16x8 */
template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt16x8_1Int8(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToI8, "Unexpected opcode for this format.");

    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Int8<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    uint const LANES = 8;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlots[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlots[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlots[3] = GetRegSlotFromIntReg(layout->I4);
    srcRegSlots[4] = GetRegSlotFromIntReg(layout->I5);
    srcRegSlots[5] = GetRegSlotFromIntReg(layout->I6);
    srcRegSlots[6] = GetRegSlotFromIntReg(layout->I7);
    srcRegSlots[7] = GetRegSlotFromIntReg(layout->I8);

    BuildSimd_1Ints(newOpcode, offset, TySimd128I8, srcRegSlots, dstRegSlot, LANES);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Int16x8_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128I8);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_I8)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128I8);
        symDst->m_allocated = true;
        if (symDst == nullptr || (uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128I8, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Int16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Int16x8_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I8);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_I8), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt16x8_2Int8(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_2Int8<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I8);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(layout->I2);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(layout->I7);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(layout->I8);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(layout->I9);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src7Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src8Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src9Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_I8);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_I8;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildInt16x8_3Int8(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_3Int8<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I8_2);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I8);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(layout->I7);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(layout->I8);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(layout->I9);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(layout->I10);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src7Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src8Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src9Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src10Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_I8);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_U8;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_I8);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_I8), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I8_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool16x8_1Int16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool16x8_1Int16x8_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I8_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Bool16x8_1Int16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Bool16x8_1Int16x8_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->I8_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->I8_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I8);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128I8);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I8);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Int16x8));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_I8, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_2Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Float32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Int32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128I4);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Int8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128I16);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Uint32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Uint16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt16x8_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int16x8_1Uint8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->I8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128U16);
}

/* Uint32x4 */
template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint32x4_1Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToU4, "Unexpected opcode for this format.");
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    uint const LANES = 4;
    Js::RegSlot srcRegSlot[LANES];
    srcRegSlot[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlot[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlot[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlot[3] = GetRegSlotFromIntReg(layout->I4);

    BuildSimd_1Ints(newOpcode, offset, TySimd128U4, srcRegSlot, dstRegSlot, LANES);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Uint32x4_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128U4);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_U4)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128U4);
        symDst->m_allocated = true;
        if (symDst == nullptr || (uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128U4, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Uint32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Uint32x4_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_U4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint32x4_2Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_2Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U4);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(layout->I2);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_U4);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_U4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint32x4_3Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_3Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U4_2);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U4);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_U4);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_U4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_U4);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_U4), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U4_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool32x4_1Uint32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_1Uint32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U4_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Bool32x4_1Uint32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Bool32x4_1Uint32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U4_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->U4_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128U4);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U4);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint32x4));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_U4, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_2Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Float32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4_U4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Int32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Int16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128I8);
}

/* Enable with Int8x16 support*/

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Int8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128I16);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Uint16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint32x4_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint32x4_1Uint8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128U16);
}

/* Uint16x8 */
template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint16x8_1Int8(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToU8, "Unexpected opcode for this format.");

    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Int8<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    uint const LANES = 8;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlots[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlots[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlots[3] = GetRegSlotFromIntReg(layout->I4);
    srcRegSlots[4] = GetRegSlotFromIntReg(layout->I5);
    srcRegSlots[5] = GetRegSlotFromIntReg(layout->I6);
    srcRegSlots[6] = GetRegSlotFromIntReg(layout->I7);
    srcRegSlots[7] = GetRegSlotFromIntReg(layout->I8);

    BuildSimd_1Ints(newOpcode, offset, TySimd128U8, srcRegSlots, dstRegSlot, LANES);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Uint16x8_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128U8);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_U8)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128U8);
        symDst->m_allocated = true;
        if (symDst == nullptr || (uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128U4, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Uint16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Uint16x8_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U8);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_U8), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint16x8_2Int8(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_2Int8<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U8);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(layout->I2);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(layout->I7);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(layout->I8);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(layout->I9);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src7Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src8Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src9Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_U8);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_U8;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint16x8_3Int8(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_3Int8<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U8_2);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U8);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(layout->I7);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(layout->I8);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(layout->I9);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(layout->I10);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src7Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src8Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src9Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src10Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_U8);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_U8;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_U8);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_U8), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U8_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool16x8_1Uint16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool16x8_1Uint16x8_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U8_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Bool16x8_1Uint16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Bool16x8_1Uint16x8_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U8_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->U8_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U8);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128U8);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U8);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_U8, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_2Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Float32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Int32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Int16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128I8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Int8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128I16);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Uint32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint16x8_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint16x8_1Uint8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128U16);
}


/* Uint8x16 */
template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint8x16_1Int16(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToU16, "Unexpected opcode for this format.");
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Int16<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    uint const LANES = 16;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlots[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlots[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlots[3] = GetRegSlotFromIntReg(layout->I4);
    srcRegSlots[4] = GetRegSlotFromIntReg(layout->I5);
    srcRegSlots[5] = GetRegSlotFromIntReg(layout->I6);
    srcRegSlots[6] = GetRegSlotFromIntReg(layout->I7);
    srcRegSlots[7] = GetRegSlotFromIntReg(layout->I8);
    srcRegSlots[8] = GetRegSlotFromIntReg(layout->I9);
    srcRegSlots[9] = GetRegSlotFromIntReg(layout->I10);
    srcRegSlots[10] = GetRegSlotFromIntReg(layout->I11);
    srcRegSlots[11] = GetRegSlotFromIntReg(layout->I12);
    srcRegSlots[12] = GetRegSlotFromIntReg(layout->I13);
    srcRegSlots[13] = GetRegSlotFromIntReg(layout->I14);
    srcRegSlots[14] = GetRegSlotFromIntReg(layout->I15);
    srcRegSlots[15] = GetRegSlotFromIntReg(layout->I16);

    BuildSimd_1Ints(newOpcode, offset, TySimd128U16, srcRegSlots, dstRegSlot, LANES);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Uint8x16_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128U16);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_U16)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128U16);
        symDst->m_allocated = true;
        if (symDst == nullptr || (uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128U16, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint16x8));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Uint8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Uint8x16_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U16);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_U16), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint8x16_2Int16(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_2Int16<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U16);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U16);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(layout->I2);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(layout->I7);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(layout->I8);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(layout->I9);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(layout->I10);
    IR::RegOpnd * src11Opnd = BuildIntConstOpnd(layout->I11);
    IR::RegOpnd * src12Opnd = BuildIntConstOpnd(layout->I12);
    IR::RegOpnd * src13Opnd = BuildIntConstOpnd(layout->I13);
    IR::RegOpnd * src14Opnd = BuildIntConstOpnd(layout->I14);
    IR::RegOpnd * src15Opnd = BuildIntConstOpnd(layout->I15);
    IR::RegOpnd * src16Opnd = BuildIntConstOpnd(layout->I16);
    IR::RegOpnd * src17Opnd = BuildIntConstOpnd(layout->I17);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);

    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src7Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src8Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src9Opnd, instr->GetDst()->AsRegOpnd(), offset);

    instr = AddExtendedArg(src10Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src11Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src12Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src13Opnd, instr->GetDst()->AsRegOpnd(), offset);

    instr = AddExtendedArg(src14Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src15Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src16Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src17Opnd, instr->GetDst()->AsRegOpnd(), offset);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_U16);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_U16;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildUint8x16_3Int16(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_3Int16<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U16_2);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U16);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U16);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U16);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(layout->I3);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(layout->I4);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(layout->I5);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(layout->I6);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(layout->I7);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(layout->I8);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(layout->I9);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(layout->I10);

    IR::RegOpnd * src11Opnd = BuildIntConstOpnd(layout->I11);
    IR::RegOpnd * src12Opnd = BuildIntConstOpnd(layout->I12);
    IR::RegOpnd * src13Opnd = BuildIntConstOpnd(layout->I13);
    IR::RegOpnd * src14Opnd = BuildIntConstOpnd(layout->I14);
    IR::RegOpnd * src15Opnd = BuildIntConstOpnd(layout->I15);
    IR::RegOpnd * src16Opnd = BuildIntConstOpnd(layout->I16);
    IR::RegOpnd * src17Opnd = BuildIntConstOpnd(layout->I17);
    IR::RegOpnd * src18Opnd = BuildIntConstOpnd(layout->I18);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));


    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);

    instr = AddExtendedArg(src7Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src8Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src9Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src10Opnd, instr->GetDst()->AsRegOpnd(), offset);

    instr = AddExtendedArg(src11Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src12Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src13Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src14Opnd, instr->GetDst()->AsRegOpnd(), offset);

    instr = AddExtendedArg(src15Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src16Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src17Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src18Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_U16);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_U16;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_U16), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128U16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_U16);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U16_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool8x16_1Uint8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool8x16_1Uint8x16_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U16_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B16, TySimd128U16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Bool8x16_1Uint8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Bool8x16_1Uint8x16_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->U16_2);
    Js::RegSlot src3RegSlot = GetRegSlotFromSimd128Reg(layout->U16_3);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U16);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128U16);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U16);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));
    src2Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));
    src3Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Uint8x16));

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3
    // Later phases will chain the arguments by following singleDefInstr of each use of ti.

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_U16, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_2Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Float32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->F4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128F4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Int32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128I4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Int16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128I8);
}

/* Enable with Int8x16 support */

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Int8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->I16_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128I16);
}


template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Uint32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U4_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128U4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildUint8x16_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Uint8x16_1Uint16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->U16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->U8_1);
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128U8);
}


//Bool32x4
template <typename SizePolicy>
void IRBuilderAsmJs::BuildBool32x4_1Int4(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToB4, "Unexpected opcode for this format.");
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_1Int4<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    uint const LANES = 4;
    Js::RegSlot srcRegSlot[LANES];
    srcRegSlot[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlot[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlot[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlot[3] = GetRegSlotFromIntReg(layout->I4);

    BuildSimd_1Ints(newOpcode, offset, TySimd128B4, srcRegSlot, dstRegSlot, LANES);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Bool32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Bool32x4_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_AllTrue_B4 || opcode == Js::OpCode::Simd128_AnyTrue_B4),
        "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->B4_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Bool32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Bool32x4_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128B4);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_B4)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128B4);
        symDst->m_allocated = true;
        if ((uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128B4, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

//Bool16x8
template <typename SizePolicy>
void IRBuilderAsmJs::BuildBool16x8_1Int8(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToB8, "Unexpected opcode for this format.");

    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool16x8_1Int8<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B8_0);
    uint const LANES = 8;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlots[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlots[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlots[3] = GetRegSlotFromIntReg(layout->I4);
    srcRegSlots[4] = GetRegSlotFromIntReg(layout->I5);
    srcRegSlots[5] = GetRegSlotFromIntReg(layout->I6);
    srcRegSlots[6] = GetRegSlotFromIntReg(layout->I7);
    srcRegSlots[7] = GetRegSlotFromIntReg(layout->I8);

    BuildSimd_1Ints(newOpcode, offset, TySimd128B8, srcRegSlots, dstRegSlot, LANES);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Bool16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Bool16x8_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_AllTrue_B8 || opcode == Js::OpCode::Simd128_AnyTrue_B8),
        "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool16x8_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool16x8_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool16x8_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->B8_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Bool16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Bool16x8_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128B8);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_B8)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128B8);
        symDst->m_allocated = true;
        if ((uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128B8, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

//Bool8x16
template <typename SizePolicy>
void IRBuilderAsmJs::BuildBool8x16_1Int16(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToB16, "Unexpected opcode for this format.");
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool8x16_1Int16<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B16_0);
    uint const LANES = 16;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = GetRegSlotFromIntReg(layout->I1);
    srcRegSlots[1] = GetRegSlotFromIntReg(layout->I2);
    srcRegSlots[2] = GetRegSlotFromIntReg(layout->I3);
    srcRegSlots[3] = GetRegSlotFromIntReg(layout->I4);
    srcRegSlots[4] = GetRegSlotFromIntReg(layout->I5);
    srcRegSlots[5] = GetRegSlotFromIntReg(layout->I6);
    srcRegSlots[6] = GetRegSlotFromIntReg(layout->I7);
    srcRegSlots[7] = GetRegSlotFromIntReg(layout->I8);
    srcRegSlots[8] = GetRegSlotFromIntReg(layout->I9);
    srcRegSlots[9] = GetRegSlotFromIntReg(layout->I10);
    srcRegSlots[10] = GetRegSlotFromIntReg(layout->I11);
    srcRegSlots[11] = GetRegSlotFromIntReg(layout->I12);
    srcRegSlots[12] = GetRegSlotFromIntReg(layout->I13);
    srcRegSlots[13] = GetRegSlotFromIntReg(layout->I14);
    srcRegSlots[14] = GetRegSlotFromIntReg(layout->I15);
    srcRegSlots[15] = GetRegSlotFromIntReg(layout->I16);

    BuildSimd_1Ints(newOpcode, offset, TySimd128B16, srcRegSlots, dstRegSlot, LANES);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Bool8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Bool8x16_1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_AllTrue_B16 || opcode == Js::OpCode::Simd128_AnyTrue_B16),
        "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool8x16_2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);

    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool8x16_3(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool8x16_3<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromSimd128Reg(layout->B16_2);

    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B16);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildReg1Bool8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Reg1Bool8x16_1<SizePolicy>>();

    Js::RegSlot srcRegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);
    Js::RegSlot dstRegSlot = layout->R0;
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TySimd128B16);
    srcOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));

    IR::Instr * instr = nullptr;
    IR::Opnd * dstOpnd = nullptr;
    StackSym * symDst = nullptr;

    if (newOpcode == Js::OpCodeAsmJs::Simd128_I_ArgOut_B16)
    {
        symDst = StackSym::NewArgSlotSym((uint16)dstRegSlot, m_func, TySimd128B16);
        symDst->m_allocated = true;
        if ((uint16)(dstRegSlot) != (dstRegSlot))
        {
            AssertMsg(UNREACHED, "Arg count too big...");
            Fatal();
        }

        dstOpnd = IR::SymOpnd::New(symDst, TySimd128B16, m_func);
        dstOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

//Bool extractLane/ReplaceLane
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool32x4_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_B4), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128B4);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool16x8_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool16x8_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_B8), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128B8);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool8x16_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool8x16_2Int2<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);
    Js::RegSlot src3RegSlot = GetRegSlotFromIntReg(layout->I3);
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_B16), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128B16);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Bool32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Bool32x4_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B4_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool32x4));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_B4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Bool16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Bool16x8_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B8_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool16x8));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_B8), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildInt1Bool8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Int1Bool8x16_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(layout->I0);
    Js::RegSlot src1RegSlot = GetRegSlotFromSimd128Reg(layout->B16_1);
    Js::RegSlot src2RegSlot = GetRegSlotFromIntReg(layout->I2);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    src1Opnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128Bool8x16));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_B16), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}


void IRBuilderAsmJs::BuildSimd_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt32);
    src1Opnd->SetValueType(ValueType::GetInt(false));
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, simdType);
    dstOpnd->SetValueType(GetSimdValueTypeFromIRType(simdType));
    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildSimd_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, Js::RegSlot src3RegSlot, IRType simdType)
{
    ValueType valueType = GetSimdValueTypeFromIRType(simdType);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, simdType);
    src1Opnd->SetValueType(valueType);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TyInt32);
    src3Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, simdType);
    dstOpnd->SetValueType(valueType);

    // Given bytecode: dst = op s1, s2, s3
    // Generate:
    // t1 = ExtendedArg_A s1
    // t2 = ExtendedArg_A s2, t1
    // t3 = ExtendedArg_A s3, t2
    // dst = op t3

    IR::Instr* instr = nullptr;

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void IRBuilderAsmJs::BuildSimd_2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType)
{
    ValueType valueType = GetSimdValueTypeFromIRType(simdType);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, simdType);
    src1Opnd->SetValueType(valueType);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, simdType);
    dstOpnd->SetValueType(valueType);

    Js::OpCode opcode;

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Simd128_Return_F4:
    case Js::OpCodeAsmJs::Simd128_Return_I4:
    case Js::OpCodeAsmJs::Simd128_Return_I8:
    case Js::OpCodeAsmJs::Simd128_Return_I16:
    case Js::OpCodeAsmJs::Simd128_Return_U4:
    case Js::OpCodeAsmJs::Simd128_Return_U8:
    case Js::OpCodeAsmJs::Simd128_Return_U16:
    case Js::OpCodeAsmJs::Simd128_Return_B4:
    case Js::OpCodeAsmJs::Simd128_Return_B8:
    case Js::OpCodeAsmJs::Simd128_Return_B16:
        if (m_func->IsLoopBody())
        {
            IR::Instr* slotInstr = GenerateStSlotForReturn(src1Opnd, simdType);
            AddInstr(slotInstr, offset);
        }
        opcode = Js::OpCode::Ld_A;
        break;
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTF4:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTI4:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTI8:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTI16:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTU4:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTU8:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTU16:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTB4:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTB8:
    case Js::OpCodeAsmJs::Simd128_I_Conv_VTB16:
    case Js::OpCodeAsmJs::Simd128_Ld_F4:
    case Js::OpCodeAsmJs::Simd128_Ld_I4:
    case Js::OpCodeAsmJs::Simd128_Ld_I8:
    case Js::OpCodeAsmJs::Simd128_Ld_I16:
    case Js::OpCodeAsmJs::Simd128_Ld_U4:
    case Js::OpCodeAsmJs::Simd128_Ld_U8:
    case Js::OpCodeAsmJs::Simd128_Ld_U16:
    case Js::OpCodeAsmJs::Simd128_Ld_B4:
    case Js::OpCodeAsmJs::Simd128_Ld_B8:
    case Js::OpCodeAsmJs::Simd128_Ld_B16:
        opcode = Js::OpCode::Ld_A;
        break;
    default:
        opcode = GetSimdOpcode(newOpcode);
    }

    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildSimd_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType)
{
    ValueType valueType = GetSimdValueTypeFromIRType(simdType);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, simdType);
    src1Opnd->SetValueType(valueType);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, simdType);
    dstOpnd->SetValueType(valueType);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}
void IRBuilderAsmJs::BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, simdType, simdType);
}

void IRBuilderAsmJs::BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType dstSimdType, IRType srcSimdType)
{
    ValueType valueType = GetSimdValueTypeFromIRType(srcSimdType);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, srcSimdType);
    src1Opnd->SetValueType(valueType);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, srcSimdType);
    src2Opnd->SetValueType(valueType);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, dstSimdType);
    dstOpnd->SetValueType(GetSimdValueTypeFromIRType(dstSimdType));

    Js::OpCode opcode;

    opcode = GetSimdOpcode(newOpcode);

    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

// bool32x4
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool32x4_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B4_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_B4);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B4);
 
}

// bool16x8
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool16x8_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B8_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_B8);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B8);
}

// bool8x16
template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBool8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_Bool8x16_1Int1<SizePolicy>>();

    Js::RegSlot dstRegSlot = GetRegSlotFromSimd128Reg(layout->B16_0);
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(layout->I1);
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_B16);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B16);
}

void IRBuilderAsmJs::BuildSimdConversion(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot, IRType dstSimdType, IRType srcSimdType)
{
    ValueType srcValueType = GetSimdValueTypeFromIRType(srcSimdType);
    ValueType dstValueType = GetSimdValueTypeFromIRType(dstSimdType);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(srcRegSlot, srcSimdType);
    src1Opnd->SetValueType(srcValueType);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, dstSimdType);
    dstOpnd->SetValueType(dstValueType);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

ValueType IRBuilderAsmJs::GetSimdValueTypeFromIRType(IRType type)
{
    switch (type)
    {
    case TySimd128F4:
        return ValueType::GetSimd128(ObjectType::Simd128Float32x4);
    case TySimd128D2:
        return ValueType::GetSimd128(ObjectType::Simd128Float64x2);
    case TySimd128I4:
        return ValueType::GetSimd128(ObjectType::Simd128Int32x4);
    case TySimd128I8:
        return ValueType::GetSimd128(ObjectType::Simd128Int16x8);
    case TySimd128I16:
        return ValueType::GetSimd128(ObjectType::Simd128Int8x16);
    case TySimd128U4:
        return ValueType::GetSimd128(ObjectType::Simd128Uint32x4);
    case TySimd128U8:
        return ValueType::GetSimd128(ObjectType::Simd128Uint16x8);
    case TySimd128U16:
        return ValueType::GetSimd128(ObjectType::Simd128Uint8x16);
    case TySimd128B4:
        return ValueType::GetSimd128(ObjectType::Simd128Bool32x4);
    case TySimd128B8:
        return ValueType::GetSimd128(ObjectType::Simd128Bool16x8);
    case TySimd128B16:
        return ValueType::GetSimd128(ObjectType::Simd128Bool8x16);
    default:
        Assert(UNREACHED);
    }
    return ValueType::GetObject(ObjectType::UninitializedObject);
    
}

void IRBuilderAsmJs::BuildSimd_1Ints(Js::OpCodeAsmJs newOpcode, uint32 offset, IRType dstSimdType, Js::RegSlot* srcRegSlots, Js::RegSlot dstRegSlot, uint LANES)
{

    Assert(dstSimdType == TySimd128B4 || dstSimdType == TySimd128I4 || dstSimdType == TySimd128U4 ||
           dstSimdType == TySimd128B8 || dstSimdType == TySimd128I8 || dstSimdType == TySimd128U8 ||
           dstSimdType == TySimd128B16|| dstSimdType == TySimd128I16|| dstSimdType == TySimd128U16);

    IR::RegOpnd * srcOpnds[16];
    IR::Instr * instr = nullptr;
    Assert(LANES <= 16);
    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    // first arg
    srcOpnds[0] = BuildSrcOpnd(srcRegSlots[0], TyInt32);
    srcOpnds[0]->SetValueType(ValueType::GetInt(false));
    instr = AddExtendedArg(srcOpnds[0], nullptr, offset);
    // reset of args
    for (uint i = 1; i < LANES && i < 16; i++)
    {
        srcOpnds[i] = BuildSrcOpnd(srcRegSlots[i], TyInt32);
        srcOpnds[i]->SetValueType(ValueType::GetInt(false));
        instr = AddExtendedArg(srcOpnds[i], instr->GetDst()->AsRegOpnd(), offset);
    }

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, dstSimdType);
    dstOpnd->SetValueType(GetSimdValueTypeFromIRType(dstSimdType));

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

template <typename SizePolicy>
void IRBuilderAsmJs::BuildAsmSimdTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_AsmSimdTypedArr<SizePolicy>>();
    BuildAsmSimdTypedArr(newOpcode, offset, layout->SlotIndex, layout->Value, layout->ViewType, layout->DataWidth);
}

void
IRBuilderAsmJs::BuildAsmSimdTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, int8 viewType, uint8 dataWidth)
{
    IRType type = TySimd128F4;
    Js::RegSlot valueRegSlot = GetRegSlotFromSimd128Reg(value);

    IR::RegOpnd * maskedOpnd = nullptr;
    IR::Instr * maskInstr = nullptr;

    Js::OpCode op = GetSimdOpcode(newOpcode);
    ValueType arrayType, valueType;
    bool isLd = false, isConst = false;
    uint32 mask = 0;

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Simd128_LdArr_I4:
        valueType = ValueType::GetObject(ObjectType::Simd128Int32x4);
        isLd = true;
        isConst = false;
        type = TySimd128I4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_I8:
        valueType = ValueType::GetObject(ObjectType::Simd128Int16x8);
        isLd = true;
        isConst = false;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_I16:
        valueType = ValueType::GetObject(ObjectType::Simd128Int8x16);
        isLd = true;
        isConst = false;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_U4:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint32x4);
        isLd = true;
        isConst = false;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_U8:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint16x8);
        isLd = true;
        isConst = false;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_U16:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint8x16);
        isLd = true;
        isConst = false;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_F4:
        valueType = ValueType::GetObject(ObjectType::Simd128Float32x4);
        isLd = true;
        isConst = false;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_LdArr_D2:
        valueType = ValueType::GetObject(ObjectType::Simd128Float64x2);
        isLd = true;
        isConst = false;
        type = TySimd128D2;
        break;
#endif // 0

    case Js::OpCodeAsmJs::Simd128_StArr_I4:
        valueType = ValueType::GetObject(ObjectType::Simd128Int32x4);
        isLd = false;
        isConst = false;
        type = TySimd128I4;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_I8:
        valueType = ValueType::GetObject(ObjectType::Simd128Int16x8);
        isLd = false;
        isConst = false;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_I16:
        valueType = ValueType::GetObject(ObjectType::Simd128Int8x16);
        isLd = false;
        isConst = false;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_U4:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint32x4);
        isLd = false;
        isConst = false;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_U8:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint16x8);
        isLd = false;
        isConst = false;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_U16:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint8x16);
        isLd = false;
        isConst = false;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_F4:
        valueType = ValueType::GetObject(ObjectType::Simd128Float32x4);
        isLd = false;
        isConst = false;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_StArr_D2:
        valueType = ValueType::GetObject(ObjectType::Simd128Float64x2);
        isLd = false;
        isConst = false;
        type = TySimd128D2;
        break;
#endif // 0

    case Js::OpCodeAsmJs::Simd128_LdArrConst_I4:
        valueType = ValueType::GetObject(ObjectType::Simd128Int32x4);
        isLd = true;
        isConst = true;
        type = TySimd128I4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_I8:
        valueType = ValueType::GetObject(ObjectType::Simd128Int16x8);
        isLd = true;
        isConst = true;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_I16:
        valueType = ValueType::GetObject(ObjectType::Simd128Int8x16);
        isLd = true;
        isConst = true;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_U4:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint32x4);
        isLd = true;
        isConst = true;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_U8:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint16x8);
        isLd = true;
        isConst = true;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_U16:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint8x16);
        isLd = true;
        isConst = true;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_F4:
        valueType = ValueType::GetObject(ObjectType::Simd128Float32x4);
        isLd = true;
        isConst = true;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_LdArrConst_D2:
        valueType = ValueType::GetObject(ObjectType::Simd128Float64x2);
        isLd = true;
        isConst = true;
        type = TySimd128D2;
        break;
#endif
    case Js::OpCodeAsmJs::Simd128_StArrConst_I4:
        valueType = ValueType::GetObject(ObjectType::Simd128Int32x4);
        isLd = false;
        type = TySimd128I4;
        isConst = true;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_I8:
        valueType = ValueType::GetObject(ObjectType::Simd128Int16x8);
        isLd = false;
        isConst = true;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_I16:
        valueType = ValueType::GetObject(ObjectType::Simd128Int8x16);
        isLd = false;
        isConst = true;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_U4:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint32x4);
        isLd = false;
        isConst = true;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_U8:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint16x8);
        isLd = false;
        isConst = true;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_U16:
        valueType = ValueType::GetObject(ObjectType::Simd128Uint8x16);
        isLd = false;
        isConst = true;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_F4:
        valueType = ValueType::GetObject(ObjectType::Simd128Float32x4);
        isLd = false;
        isConst = true;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_StArrConst_D2:
        valueType = ValueType::GetObject(ObjectType::Simd128Float64x2);
        isLd = false;
        isConst = true;
        type = TySimd128D2;
        break;
#endif
    default:
        Assert(UNREACHED);
    }

    switch (viewType)
    {
    case Js::ArrayBufferView::TYPE_INT8:
        arrayType = ValueType::GetObject(ObjectType::Int8Array);
        break;
    case Js::ArrayBufferView::TYPE_UINT8:
        arrayType = ValueType::GetObject(ObjectType::Uint8Array);
        break;
    case Js::ArrayBufferView::TYPE_INT16:
        arrayType = ValueType::GetObject(ObjectType::Int16Array);
        mask = (uint32)~1;
        break;
    case Js::ArrayBufferView::TYPE_UINT16:
        arrayType = ValueType::GetObject(ObjectType::Uint16Array);
        mask = (uint32)~1;
        break;
    case Js::ArrayBufferView::TYPE_INT32:
        arrayType = ValueType::GetObject(ObjectType::Int32Array);
        mask = (uint32)~3;
        break;
    case Js::ArrayBufferView::TYPE_UINT32:
        arrayType = ValueType::GetObject(ObjectType::Uint32Array);
        mask = (uint32)~3;
        break;
    case Js::ArrayBufferView::TYPE_FLOAT32:
        arrayType = ValueType::GetObject(ObjectType::Float32Array);
        mask = (uint32)~3;
        break;
    case Js::ArrayBufferView::TYPE_FLOAT64:
        arrayType = ValueType::GetObject(ObjectType::Float64Array);
        mask = (uint32)~7;
        break;
    default:
        Assert(UNREACHED);
    }

    IR::Opnd * sizeOpnd = BuildSrcOpnd(AsmJsRegSlots::LengthReg, TyUint32);
    if (!isConst)
    {

        Js::RegSlot indexRegSlot = GetRegSlotFromIntReg(slotIndex);

        if (mask)
        {
            // AND_I4 index, mask
            maskedOpnd = IR::RegOpnd::New(TyUint32, m_func);
            maskInstr = IR::Instr::New(Js::OpCode::And_I4, maskedOpnd, BuildSrcOpnd(indexRegSlot, TyInt32), IR::IntConstOpnd::New(mask, TyUint32, m_func), m_func);

        }
        else
        {
            maskedOpnd = BuildSrcOpnd(indexRegSlot, TyInt32);
        }
    }

    IR::Instr * instr = nullptr;
    IR::RegOpnd * regOpnd = nullptr;
    IR::IndirOpnd * indirOpnd = nullptr;
    IR::RegOpnd * baseOpnd = BuildSrcOpnd(AsmJsRegSlots::BufferReg, TyVar);
    baseOpnd->SetValueType(arrayType);
    baseOpnd->SetValueTypeFixed();

    if (isLd)
    {
        regOpnd = BuildDstOpnd(valueRegSlot, type);
        regOpnd->SetValueType(valueType);
        if (!isConst)
        {
            Assert(maskedOpnd);
            // Js::OpCodeAsmJs::Simd128_LdArr_I4:
            // Js::OpCodeAsmJs::Simd128_LdArr_F4:
            // Js::OpCodeAsmJs::Simd128_LdArr_D2:
            indirOpnd = IR::IndirOpnd::New(baseOpnd, maskedOpnd, type, m_func);
        }
        else
        {
            // Js::OpCodeAsmJs::Simd128_LdArrConst_I4:
            // Js::OpCodeAsmJs::Simd128_LdArrConst_F4:
            // Js::OpCodeAsmJs::Simd128_LdArrConst_D2:
            indirOpnd = IR::IndirOpnd::New(baseOpnd, slotIndex, type, m_func);
        }
        instr = IR::Instr::New(op, regOpnd, indirOpnd, sizeOpnd, m_func);
    }
    else
    {
        regOpnd = BuildSrcOpnd(valueRegSlot, type);
        regOpnd->SetValueType(valueType);
        if (!isConst)
        {
            Assert(maskedOpnd);
            // Js::OpCodeAsmJs::Simd128_StArr_I4:
            // Js::OpCodeAsmJs::Simd128_StArr_F4:
            // Js::OpCodeAsmJs::Simd128_StArr_D2:
            indirOpnd = IR::IndirOpnd::New(baseOpnd, maskedOpnd, type, m_func);
        }
        else
        {
            // Js::OpCodeAsmJs::Simd128_StArrConst_I4:
            // Js::OpCodeAsmJs::Simd128_StArrConst_F4:
            // Js::OpCodeAsmJs::Simd128_StArrConst_D2:
            indirOpnd = IR::IndirOpnd::New(baseOpnd, slotIndex, type, m_func);
        }
        instr = IR::Instr::New(op, indirOpnd, regOpnd, sizeOpnd, m_func);
    }
    // REVIEW: Store dataWidth in the instruction itself instead of an argument to avoid using ExtendedArgs or excessive opcodes.
    Assert(dataWidth >= 4 && dataWidth <= 16);
    instr->dataWidth = dataWidth;
    if (maskInstr)
    {
        AddInstr(maskInstr, offset);
    }
    AddInstr(instr, offset);

}
