//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"
#ifdef ASMJS_PLAT
#include "ByteCode/OpCodeUtilAsmJs.h"
#include "../../WasmReader/WasmParseTree.h"

void
IRBuilderAsmJs::Build()
{
    m_funcAlloc = m_func->m_alloc;

    NoRecoverMemoryJitArenaAllocator localAlloc(_u("BE-IRBuilder"), m_funcAlloc->GetPageAllocator(), Js::Throw::OutOfMemory);
    m_tempAlloc = &localAlloc;

    uint32 offset;
    uint32 statementIndex = m_statementReader ? m_statementReader->GetStatementIndex() : Js::Constants::NoStatementIndex;

    m_argStack = JitAnew(m_tempAlloc, SListCounted<IR::Instr *>, m_tempAlloc);
    m_tempList = JitAnew(m_tempAlloc, SList<IR::Instr *>, m_tempAlloc);
    m_argOffsetStack = JitAnew(m_tempAlloc, SList<int32>, m_tempAlloc);

    m_branchRelocList = JitAnew(m_tempAlloc, SList<BranchReloc *>, m_tempAlloc);

    m_switchBuilder.Init(m_func, m_tempAlloc, true);

    m_firstVarConst = 0;
    m_tempCount = 0;
    m_firstsType[0] = m_firstVarConst + AsmJsRegSlots::RegCount;
    for (int i = 0, j = 1; i < WAsmJs::LIMIT; ++i, ++j)
    {
        WAsmJs::Types type = (WAsmJs::Types)i;
        const auto typedInfo = m_asmFuncInfo->GetTypedSlotInfo(type);
        m_firstsType[j] = typedInfo.constCount;
        m_firstsType[j + WAsmJs::LIMIT] = typedInfo.varCount;
        m_firstsType[j + 2 * WAsmJs::LIMIT] = typedInfo.tmpCount;
        m_tempCount += typedInfo.tmpCount;
    }
    // Fixup the firsts by looking at the previous value
    for (int i = 1; i < m_firstsTypeCount; ++i)
    {
        m_firstsType[i] += m_firstsType[i - 1];
    }

    m_func->firstIRTemp = m_firstsType[m_firstsTypeCount - 1];

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
    m_func->m_symTable->IncreaseStartingID(m_func->firstIRTemp);

    if (m_tempCount > 0)
    {
        m_tempMap = AnewArrayZ(m_tempAlloc, SymID, m_tempCount);
        m_fbvTempUsed = BVFixed::New<JitArenaAllocator>(m_tempCount, m_tempAlloc);
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

    m_functionStartOffset = m_jnReader.GetCurrentOffset();
    m_lastInstr = m_func->m_headInstr;

    CompileAssert(sizeof(SymID) == sizeof(Js::RegSlot));

    offset = m_functionStartOffset;

    // Skip the last EndOfBlock opcode
    // EndOfBlock opcode has same value in Asm
    Assert(!OpCodeAttr::HasMultiSizeLayout(Js::OpCode::EndOfBlock));
    uint32 lastOffset = m_func->GetJITFunctionBody()->GetByteCodeLength() - Js::OpCodeUtil::EncodedSize(Js::OpCode::EndOfBlock, Js::SmallLayout);
    uint32 offsetToInstructionCount = lastOffset;
    if (this->IsLoopBody())
    {
        // LdSlot & StSlot needs to cover all the register, including the temps, because we might treat
        // those as if they are local for yielding
        m_jitLoopBodyData = JitAnew(m_tempAlloc, JitLoopBodyData,
            BVFixed::New<JitArenaAllocator>(GetLastTmp(WAsmJs::LastType), m_tempAlloc),
            BVFixed::New<JitArenaAllocator>(GetLastTmp(WAsmJs::LastType), m_tempAlloc),
            StackSym::New(TyInt32, this->m_func)
        );
#if DBG
        GetJitLoopBodyData().m_usedAsTemp = BVFixed::New<JitArenaAllocator>(GetLastTmp(WAsmJs::LastType), m_tempAlloc);
#endif
        lastOffset = m_func->GetWorkItem()->GetLoopHeader()->endOffset;
        // Ret is created at lastOffset + 1, so we need lastOffset + 2 entries
        offsetToInstructionCount = lastOffset + 2;
    }

    m_offsetToInstructionCount = offsetToInstructionCount;
    m_offsetToInstruction = JitAnewArrayZ(m_tempAlloc, IR::Instr *, offsetToInstructionCount);

    LoadNativeCodeData();

    BuildConstantLoads();
    if (!this->IsLoopBody() && m_func->GetJITFunctionBody()->HasImplicitArgIns())
    {
        BuildImplicitArgIns();
    }

    if (m_statementReader && m_statementReader->AtStatementBoundary(&m_jnReader))
    {
        statementIndex = AddStatementBoundary(statementIndex, offset);
    }

    Js::LayoutSize layoutSize;
    for (Js::OpCodeAsmJs newOpcode = m_jnReader.ReadAsmJsOp(layoutSize); (uint)m_jnReader.GetCurrentOffset() <= lastOffset; newOpcode = m_jnReader.ReadAsmJsOp(layoutSize))
    {
        Assert(newOpcode != Js::OpCodeAsmJs::EndOfBlock);

        AssertOrFailFastMsg(Js::OpCodeUtilAsmJs::IsValidByteCodeOpcode(newOpcode), "Error getting opcode from m_jnReader.Op()");

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
            Js::Throw::InternalError();
            break;
        }
        offset = m_jnReader.GetCurrentOffset();

        if (m_statementReader && m_statementReader->AtStatementBoundary(&m_jnReader))
        {
            statementIndex = AddStatementBoundary(statementIndex, offset);
        }

    }

    if (m_statementReader && Js::Constants::NoStatementIndex != statementIndex)
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
    if (m_func->IsOOPJIT() && m_func->IsTopFunc())
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
        AssertOrFailFast(offset < m_offsetToInstructionCount);
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
    SymID symID = static_cast<SymID>(dstRegSlot);
    if (IsLoopBody() && (RegIsVar(dstRegSlot) || RegIsJitLoopYield(dstRegSlot)))
    {
        // Use default symID for yield and var registers
        EnsureLoopBodyAsmJsStoreSlot(dstRegSlot, type);
    }
    else if (RegIsTemp(dstRegSlot))
    {
#if DBG
        if (this->IsLoopBody())
        {
            // If we are doing loop body, and a temp reg slot is loaded via LdSlot
            // That means that we have detected that the slot is live coming in to the loop.
            // This would only happen for the value of a "with" statement, so there shouldn't
            // be any def for those
            Assert(!GetJitLoopBodyData().GetLdSlots()->Test(dstRegSlot));
            GetJitLoopBodyData().m_usedAsTemp->Set(dstRegSlot);
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
    else if (RegIsConstant(dstRegSlot))
    {
        // Don't need to track constant registers for bailout. Don't set the byte code register for constant.
        dstRegSlot = Js::Constants::NoRegister;
    }
    else
    {
        // if loop body then one of the above conditions should hold true
        Assert(!IsLoopBody() || dstRegSlot == 0);
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
    const WAsmJs::TypedSlotInfo& info = m_func->GetJITFunctionBody()->GetAsmJsInfo()->GetTypedSlotInfo(WAsmJs::INT32);
    Assert(info.constSrcByteOffset != Js::Constants::InvalidOffset);
    AssertOrFailFast(info.constSrcByteOffset < UInt32Math::Mul<sizeof(Js::Var)>(m_func->GetJITFunctionBody()->GetConstCount()));
    int* intConstTable = reinterpret_cast<int*>(((byte*)constTable) + info.constSrcByteOffset);
    uint32 srcReg = GetTypedRegFromRegSlot(regSlot, WAsmJs::INT32);
    AssertOrFailFast(srcReg >= Js::FunctionBody::FirstRegSlot && srcReg < info.constCount);
    const int32 value = intConstTable[srcReg];
    IR::IntConstOpnd *opnd = IR::IntConstOpnd::New(value, TyInt32, m_func);

    return (IR::RegOpnd*)opnd;
}

SymID
IRBuilderAsmJs::BuildSrcStackSymID(Js::RegSlot regSlot, IRType type /*= IRType::TyVar*/)
{
    SymID symID = static_cast<SymID>(regSlot);

    if (IsLoopBody() && (RegIsVar(regSlot) || RegIsJitLoopYield(regSlot)))
    {
        this->EnsureLoopBodyAsmJsLoadSlot(regSlot, type);
    }
    else if (this->RegIsTemp(regSlot))
    {
        // This is a use of a temp. Map the reg slot to its sym ID.
        //     !!!NOTE: always process an instruction's temp uses before its temp defs!!!
        symID = this->GetMappedTemp(regSlot);
        if (symID == 0)
        {
            // We might have temps that are live through the loop body via "with" statement
            // We need to treat those as if they are locals and don't remap them
            Assert(this->IsLoopBody());
            Assert(!GetJitLoopBodyData().m_usedAsTemp->Test(regSlot));

            symID = static_cast<SymID>(regSlot);
            this->SetMappedTemp(regSlot, symID);
            this->EnsureLoopBodyAsmJsLoadSlot(regSlot, type);
        }
        this->SetTempUsed(regSlot, TRUE);
    }
    else
    {
        Assert(!IsLoopBody() || this->RegIsConstant(regSlot) || regSlot == 0);
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
    AssertOrFailFast(m_statementReader);
    IR::PragmaInstr* pragmaInstr = IR::PragmaInstr::New(Js::OpCode::StatementBoundary, statementIndex, m_func);
    this->AddInstr(pragmaInstr, offset);

    return m_statementReader->MoveNextStatementBoundary();
}

uint32 IRBuilderAsmJs::GetTypedRegFromRegSlot(Js::RegSlot reg, WAsmJs::Types type)
{
    const auto typedInfo = m_asmFuncInfo->GetTypedSlotInfo(type);
    Js::RegSlot srcReg = reg;
    if (RegIsTypedVar(reg, type))
    {
        srcReg = reg - GetFirstVar(type);
        Assert(srcReg < typedInfo.varCount);
        srcReg += typedInfo.constCount;
    }
    else if (RegIsTemp(reg))
    {
        srcReg = reg - GetFirstTmp(type);
        Assert(srcReg < typedInfo.tmpCount);
        srcReg += typedInfo.varCount + typedInfo.constCount;
    }
    else if (RegIsConstant(reg))
    {
        srcReg = reg - GetFirstConst(type);
        Assert(srcReg < typedInfo.constCount);
    }
    return srcReg;
}

Js::RegSlot
IRBuilderAsmJs::GetRegSlotFromTypedReg(Js::RegSlot srcReg, WAsmJs::Types type)
{
    const auto typedInfo = m_asmFuncInfo->GetTypedSlotInfo(type);
    Js::RegSlot reg;
    if (srcReg < typedInfo.constCount)
    {
        reg = srcReg + GetFirstConst(type);
        Assert(reg >= GetFirstConst(type) && reg < GetLastConst(type));
        return reg;
    }

    srcReg -= typedInfo.constCount;
    if (srcReg < typedInfo.varCount)
    {
        reg = srcReg + GetFirstVar(type);
        Assert(reg >= GetFirstVar(type) && reg < GetLastVar(type));
        return reg;
    }

    srcReg -= typedInfo.varCount;
    Assert(srcReg < typedInfo.tmpCount);

    reg = srcReg + GetFirstTmp(type);
    Assert(reg >= GetFirstTmp(type) && reg < GetLastTmp(type));
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
        Assert(reg >= m_firstVarConst && reg < GetFirstConst(WAsmJs::FirstType));
    }
    else
    {
        reg = srcVarReg - AsmJsRegSlots::RegCount + GetFirstTmp(WAsmJs::FirstType) - 1;
    }
    return reg;
}

SymID
IRBuilderAsmJs::GetMappedTemp(Js::RegSlot reg)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_tempMap, "Processing non-temp reg without a temp map?");

    Js::RegSlot tempIndex = reg - GetFirstTmp(WAsmJs::FirstType);
    AssertOrFailFast(tempIndex < m_tempCount);
    return m_tempMap[tempIndex];
}

void
IRBuilderAsmJs::SetMappedTemp(Js::RegSlot reg, SymID tempId)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_tempMap, "Processing non-temp reg without a temp map?");

    Js::RegSlot tempIndex = reg - GetFirstTmp(WAsmJs::FirstType);
    AssertOrFailFast(tempIndex < m_tempCount);
    m_tempMap[tempIndex] = tempId;
}

bool
IRBuilderAsmJs::GetTempUsed(Js::RegSlot reg)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_fbvTempUsed, "Processing non-temp reg without a used BV?");

    Js::RegSlot tempIndex = reg - GetFirstTmp(WAsmJs::FirstType);
    AssertOrFailFast(tempIndex < m_tempCount);
    return !!m_fbvTempUsed->Test(tempIndex);
}

void
IRBuilderAsmJs::SetTempUsed(Js::RegSlot reg, bool used)
{
    AssertMsg(RegIsTemp(reg), "Processing non-temp reg as a temp?");
    AssertMsg(m_fbvTempUsed, "Processing non-temp reg without a used BV?");

    Js::RegSlot tempIndex = reg - GetFirstTmp(WAsmJs::FirstType);
    AssertOrFailFast(tempIndex < m_tempCount);
    if (used)
    {
        m_fbvTempUsed->Set(tempIndex);
    }
    else
    {
        m_fbvTempUsed->Clear(tempIndex);
    }
}

bool
IRBuilderAsmJs::RegIsTemp(Js::RegSlot reg)
{
    return reg >= GetFirstTmp(WAsmJs::FirstType);
}

bool
IRBuilderAsmJs::RegIsVar(Js::RegSlot reg)
{
    for (int i = 0; i < WAsmJs::LIMIT; ++i)
    {
        if (RegIsTypedVar(reg, (WAsmJs::Types)i))
        {
            return true;
        }
    }
    return false;
}

bool
IRBuilderAsmJs::RegIsTypedVar(Js::RegSlot reg, WAsmJs::Types type)
{
    return reg >= GetFirstVar(type) && reg < GetLastVar(type);
}

bool
IRBuilderAsmJs::RegIsTypedConst(Js::RegSlot reg, WAsmJs::Types type)
{
    return reg >= GetFirstConst(type) && reg < GetLastConst(type);
}

bool
IRBuilderAsmJs::RegIsTypedTmp(Js::RegSlot reg, WAsmJs::Types type)
{
    return reg >= GetFirstTmp(type) && reg < GetLastTmp(type);
}

bool
IRBuilderAsmJs::RegIs(Js::RegSlot reg, WAsmJs::Types type)
{
    return (
        RegIsTypedVar(reg, type) ||
        RegIsTypedConst(reg, type) ||
        RegIsTypedTmp(reg, type)
    );
}

bool
IRBuilderAsmJs::RegIsJitLoopYield(Js::RegSlot reg)
{
    return IsLoopBody() && GetJitLoopBodyData().IsYieldReg(reg);
}

void
IRBuilderAsmJs::CheckJitLoopReturn(Js::RegSlot reg, IRType type)
{
    if (IsLoopBody())
    {
#ifdef ENABLE_WASM
        if (GetJitLoopBodyData().IsLoopCurRegsInitialized())
        {
            Assert(m_func->GetJITFunctionBody()->IsWasmFunction());
            // In Wasm we use the Return opcode for Yields
            // Check if that yield is outside of the loop (this also covers return)
            if (!RegIsJitLoopYield(reg))
            {
                WAsmJs::Types wasmType = WAsmJs::FromIRType(type);
                Assert(wasmType < WAsmJs::LIMIT);
                uint32 typedReg = GetTypedRegFromRegSlot(reg, wasmType);
                if (GetJitLoopBodyData().IsRegOutsideOfLoop(typedReg, wasmType))
                {
                    // The reg should be either a constant (return register)
                    // Or a temp, in which case, make sure we haven't mapped that temp to something else
                    Assert(RegIsConstant(reg) || (RegIsTemp(reg) && (GetMappedTemp(reg) == (SymID)reg || GetMappedTemp(reg) == 0)));
                    GetJitLoopBodyData().SetRegIsYield(reg);
                    EnsureLoopBodyAsmJsStoreSlot(reg, type);
                }
            }
        }
        else
#endif
        {
            // In Asm.js there is no Yields outside of the loop, if we're here
            // It means we are returning a value from the loop
            Assert(RegIsConstant(reg));
            EnsureLoopBodyAsmJsStoreSlot(reg, type);
        }
    }
}

bool
IRBuilderAsmJs::RegIsSimd128ReturnVar(Js::RegSlot reg)
{
    return (reg == GetFirstConst(WAsmJs::SIMD) &&
            Js::AsmJsRetType(m_asmFuncInfo->GetRetType()).toVarType().isSIMD());
}
bool
IRBuilderAsmJs::RegIsConstant(Js::RegSlot reg)
{
    return (reg > 0 && reg < GetLastConst(WAsmJs::LastType));
}

BranchReloc *
IRBuilderAsmJs::AddBranchInstr(IR::BranchInstr * branchInstr, uint32 offset, uint32 targetOffset)
{
    AssertOrFailFast(targetOffset <= m_func->GetJITFunctionBody()->GetByteCodeLength());
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
IRBuilderAsmJs::BuildHeapBufferReload(uint32 offset, bool isFirstLoad)
{
    enum ShouldReload
    {
        DoReload,
        DontReload
    };
    const auto AddLoadField = [&](AsmJsRegSlots::ConstSlots dst, AsmJsRegSlots::ConstSlots src, int32 fieldOffset, IRType type, ShouldReload shouldReload)
    {
        if (isFirstLoad || shouldReload == DoReload)
        {
            IR::RegOpnd * dstOpnd = BuildDstOpnd(dst, type);
            IR::Opnd * srcOpnd = IR::IndirOpnd::New(BuildSrcOpnd(src, type), fieldOffset, type, m_func);
            IR::Instr * instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, m_func);
            AddInstr(instr, offset);
        }
    };

#ifdef ENABLE_WASM
    const bool isWasm = m_func->GetJITFunctionBody()->IsWasmFunction();
    const bool isSharedMem = m_func->GetJITFunctionBody()->GetAsmJsInfo()->IsSharedMemory();

    if(isWasm)
    {
        // WebAssembly.Memory only needs to be loaded once as it can't change over the course of the function
        AddLoadField(AsmJsRegSlots::WasmMemoryReg, AsmJsRegSlots::ModuleMemReg, (int32)Js::WebAssemblyModule::GetMemoryOffset(), TyVar, DontReload);

        if (!isSharedMem)
        {
            // ArrayBuffer
            // GrowMemory can change the ArrayBuffer, we have to reload it
            AddLoadField(AsmJsRegSlots::ArrayReg, AsmJsRegSlots::WasmMemoryReg, Js::WebAssemblyMemory::GetOffsetOfArrayBuffer(), TyVar, DoReload);

            // The buffer doesn't change when using Fast Virtual buffer even if we grow the memory
            ShouldReload shouldReloadBufferPointer = m_func->GetJITFunctionBody()->UsesWAsmJsFastVirtualBuffer() ? DontReload : DoReload;

            // ArrayBuffer.bufferContent
            AddLoadField(AsmJsRegSlots::RefCountedBuffer, AsmJsRegSlots::ArrayReg, Js::ArrayBuffer::GetBufferContentsOffset(), TyVar, DoReload);

            // RefCountedBuffer.buffer
            AddLoadField(AsmJsRegSlots::BufferReg, AsmJsRegSlots::RefCountedBuffer, Js::RefCountedBuffer::GetBufferOffset(), TyVar, shouldReloadBufferPointer);
            // ArrayBuffer.length
            AddLoadField(AsmJsRegSlots::LengthReg, AsmJsRegSlots::ArrayReg, Js::ArrayBuffer::GetByteLengthOffset(), TyUint32, DoReload);
        }
        else
        {
            // SharedArrayBuffer
            // SharedArrayBuffer cannot be detached and the buffer cannot change, no need to reload
            AddLoadField(AsmJsRegSlots::ArrayReg, AsmJsRegSlots::WasmMemoryReg, Js::WebAssemblyMemory::GetOffsetOfArrayBuffer(), TyVar, DontReload);
            // SharedArrayBuffer.SharedContents
            AddLoadField(AsmJsRegSlots::SharedContents, AsmJsRegSlots::ArrayReg, Js::SharedArrayBuffer::GetSharedContentsOffset(), TyVar, DontReload);
            // SharedContents.buffer
            AddLoadField(AsmJsRegSlots::BufferReg, AsmJsRegSlots::SharedContents, Js::SharedContents::GetBufferOffset(), TyVar, DontReload);
            // SharedContents.length
            AddLoadField(AsmJsRegSlots::LengthReg, AsmJsRegSlots::SharedContents, Js::SharedContents::GetBufferLengthOffset(), TyUint32, DoReload);
        }
    }
    else
#endif
    {
        // ArrayBuffer
        // The ArrayBuffer can be changed on the environment, if it is detached, we'll throw
        AddLoadField(AsmJsRegSlots::ArrayReg, AsmJsRegSlots::ModuleMemReg, (int32)Js::AsmJsModuleMemory::MemoryTableBeginOffset, TyVar, DontReload);
        // ArrayBuffer.bufferContent
        AddLoadField(AsmJsRegSlots::RefCountedBuffer, AsmJsRegSlots::ArrayReg, Js::ArrayBuffer::GetBufferContentsOffset(), TyVar, DontReload);
        // RefCountedBuffer.buffer
        AddLoadField(AsmJsRegSlots::BufferReg, AsmJsRegSlots::RefCountedBuffer, Js::RefCountedBuffer::GetBufferOffset(), TyVar, DontReload);
        // ArrayBuffer.length
        AddLoadField(AsmJsRegSlots::LengthReg, AsmJsRegSlots::ArrayReg, Js::ArrayBuffer::GetByteLengthOffset(), TyUint32, DontReload);
    }
}

template<typename T, typename ConstOpnd, typename F>
void IRBuilderAsmJs::CreateLoadConstInstrForType(
    byte* table,
    Js::RegSlot& regAllocated,
    uint32 constCount,
    uint32 byteOffset,
    IRType irType,
    ValueType valueType,
    Js::OpCode opcode,
    F extraProcess
)
{
    T* typedTable = (T*)(table + byteOffset);
    AssertOrFailFast(byteOffset < UInt32Math::Mul<sizeof(Js::Var)>(m_func->GetJITFunctionBody()->GetConstCount()));
    AssertOrFailFast(AllocSizeMath::Add((size_t)typedTable, UInt32Math::Mul<sizeof(T)>(constCount)) <= (size_t)((Js::Var*)m_func->GetJITFunctionBody()->GetConstTable() + m_func->GetJITFunctionBody()->GetConstCount()));
    // 1 for return register
    ++regAllocated;
    ++typedTable;

    for (uint32 i = 1; i < constCount; ++i)
    {
        uint32 reg = regAllocated++;
        T constVal = *typedTable++;
        IR::RegOpnd * dstOpnd = BuildDstOpnd(reg, irType);
        Assert(RegIsConstant(reg));
        dstOpnd->m_sym->SetIsFromByteCodeConstantTable();
        dstOpnd->SetValueType(valueType);

        IR::Instr *instr = IR::Instr::New(opcode, dstOpnd, ConstOpnd::New(constVal, irType, m_func), m_func);

        extraProcess(instr, constVal);
        AddInstr(instr, Js::Constants::NoByteCodeOffset);
    }
}

void
IRBuilderAsmJs::BuildConstantLoads()
{
    Js::Var * constTable = (Js::Var *)m_func->GetJITFunctionBody()->GetConstTable();
    // Load FrameDisplay
    IR::RegOpnd * asmJsEnvDstOpnd = BuildDstOpnd(AsmJsRegSlots::ModuleMemReg, TyVar);
    IR::Instr * ldAsmJsEnvInstr = IR::Instr::New(Js::OpCode::LdAsmJsEnv, asmJsEnvDstOpnd, m_func);
    AddInstr(ldAsmJsEnvInstr, Js::Constants::NoByteCodeOffset);
    // Load heap buffer
    if (m_asmFuncInfo->UsesHeapBuffer())
    {
        BuildHeapBufferReload(Js::Constants::NoByteCodeOffset, true);
    }
    if (!constTable)
    {
        return;
    }

    uint32 regAllocated = AsmJsRegSlots::RegCount;
    byte* table = (byte*)constTable;
    const bool isOOPJIT = m_func->IsOOPJIT();
    for (int i = 0; i < WAsmJs::LIMIT; ++i)
    {
        WAsmJs::Types type = (WAsmJs::Types)i;
        WAsmJs::TypedSlotInfo info = m_asmFuncInfo->GetTypedSlotInfo(type);
        if (info.constCount == 0)
        {
            continue;
        }

        switch(type)
        {
        case WAsmJs::INT32:
            CreateLoadConstInstrForType<int32, IR::IntConstOpnd>(
                table,
                regAllocated,
                info.constCount,
                info.constSrcByteOffset,
                TyInt32,
                ValueType::GetInt(false),
                Js::OpCode::Ld_I4,
                [isOOPJIT](IR::Instr* instr, int32 val)
                {
                    IR::RegOpnd* dstOpnd = instr->GetDst()->AsRegOpnd();
                    if (!isOOPJIT && dstOpnd->m_sym->IsSingleDef())
                    {
                        dstOpnd->m_sym->SetIsIntConst(val);
                    }
                }
            );
            break;
#if TARGET_64
        case WAsmJs::INT64:
            CreateLoadConstInstrForType<int64, IR::Int64ConstOpnd>(
                table,
                regAllocated,
                info.constCount,
                info.constSrcByteOffset,
                TyInt64,
                ValueType::GetInt(false),
                Js::OpCode::Ld_I4,
                [&](IR::Instr* instr, int64 val) {}
            );
            break;
#endif
        case WAsmJs::FLOAT32:
            CreateLoadConstInstrForType<float, IR::Float32ConstOpnd>(
                table,
                regAllocated,
                info.constCount,
                info.constSrcByteOffset,
                TyFloat32,
                ValueType::Float,
                Js::OpCode::LdC_F8_R8,
                [isOOPJIT](IR::Instr* instr, float val)
            {
#if _M_IX86
                IR::RegOpnd* dstOpnd = instr->GetDst()->AsRegOpnd();
                if (!isOOPJIT && dstOpnd->m_sym->IsSingleDef())
                {
                    dstOpnd->m_sym->SetIsFloatConst();
                }
#endif
            }
            );
            break;
        case WAsmJs::FLOAT64:
            CreateLoadConstInstrForType<double, IR::FloatConstOpnd>(
                table,
                regAllocated,
                info.constCount,
                info.constSrcByteOffset,
                TyFloat64,
                ValueType::Float,
                Js::OpCode::LdC_F8_R8,
                [isOOPJIT](IR::Instr* instr, double val)
                {
#if _M_IX86
                    IR::RegOpnd* dstOpnd = instr->GetDst()->AsRegOpnd();
                    if (!isOOPJIT && dstOpnd->m_sym->IsSingleDef())
                    {
                        dstOpnd->m_sym->SetIsFloatConst();
                    }
#endif
                }
            );
            break;
        case WAsmJs::SIMD:
            CreateLoadConstInstrForType<AsmJsSIMDValue, IR::Simd128ConstOpnd>(
                table,
                regAllocated,
                info.constCount,
                info.constSrcByteOffset,
                TySimd128F4,
                ValueType::UninitializedObject,
                Js::OpCode::Simd128_LdC,
                [isOOPJIT](IR::Instr* instr, AsmJsSIMDValue val)
                {
#if _M_IX86
                    IR::RegOpnd* dstOpnd = instr->GetDst()->AsRegOpnd();
                    if (!isOOPJIT && dstOpnd->m_sym->IsSingleDef())
                    {
                        dstOpnd->m_sym->SetIsSimd128Const();
                    }
#endif
                }
            );
            break;
        default:
            Assert(false);
            break;
        }
    }
}

void
IRBuilderAsmJs::BuildImplicitArgIns()
{
    int32 intArgInCount = 0;
    int32 int64ArgInCount = 0;
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
            dstOpnd = BuildDstOpnd(GetFirstVar(WAsmJs::INT32) + intArgInCount, TyInt32);
            dstOpnd->SetValueType(ValueType::GetInt(false));
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += MachPtr;
            ++intArgInCount;
            break;
        case Js::AsmJsVarType::Which::Float:
            symSrc = StackSym::NewParamSlotSym(i, m_func, TyFloat32);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, TyFloat32, m_func);
            dstOpnd = BuildDstOpnd(GetFirstVar(WAsmJs::FLOAT32) + floatArgInCount, TyFloat32);
            dstOpnd->SetValueType(ValueType::Float);
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += MachPtr;
            ++floatArgInCount;
            break;
        case Js::AsmJsVarType::Which::Double:
            symSrc = StackSym::NewParamSlotSym(i, m_func, TyFloat64);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, TyFloat64, m_func);
            dstOpnd = BuildDstOpnd(GetFirstVar(WAsmJs::FLOAT64) + doubleArgInCount, TyFloat64);
            dstOpnd->SetValueType(ValueType::Float);
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += MachDouble;
            ++doubleArgInCount;
            break;
        case Js::AsmJsVarType::Which::Int64:
            symSrc = StackSym::NewParamSlotSym(i, m_func, TyInt64);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, TyInt64, m_func);
            dstOpnd = BuildDstOpnd(GetFirstVar(WAsmJs::INT64) + int64ArgInCount, TyInt64);
            dstOpnd->SetValueType(ValueType::GetInt(false));
            instr = IR::Instr::New(Js::OpCode::ArgIn_A, dstOpnd, srcOpnd, m_func);
            offset += 8;
            ++int64ArgInCount;
            break;
        default:
        {
            // SIMD_JS
            IRType argType;
            GetSimdTypesFromAsmType((Js::AsmJsType::Which)varType.which(), &argType);

            symSrc = StackSym::NewParamSlotSym(i, m_func, argType);
            m_func->SetArgOffset(symSrc, offset);
            srcOpnd = IR::SymOpnd::New(symSrc, argType, m_func);
            dstOpnd = BuildDstOpnd(GetFirstVar(WAsmJs::SIMD) + simd128ArgInCount, argType);
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
        IR::LabelInstr * labelInstr = nullptr;
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
        AssertOrFailFast(offset < m_offsetToInstructionCount);
        targetInstr = m_offsetToInstruction[offset];
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
    case Js::OpCodeAsmJs::CheckHeap:
        instr = IR::Instr::New(Js::OpCode::ArrayDetachedCheck, m_func);
        instr->SetSrc1(IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::ArrayReg, TyVar), Js::ArrayBuffer::GetIsDetachedOffset(), TyInt8, m_func));
        AddInstr(instr, offset);
        break;
    case Js::OpCodeAsmJs::Unreachable_Void:
        instr = IR::Instr::New(Js::OpCode::ThrowRuntimeError, m_func);
        instr->SetSrc1(IR::IntConstOpnd::New(SCODE_CODE(WASMERR_Unreachable), TyInt32, instr->m_func));
        AddInstr(instr, offset);
        break;
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

        case Js::AsmJsVarType::Which::Int64:
            retSlot = GetRegSlotFromInt64Reg(0);
            regOpnd = BuildDstOpnd(retSlot, TyInt64);
            regOpnd->SetValueType(ValueType::GetInt(false));
            break;

        case Js::AsmJsRetType::Which::Void:
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
        if (regOpnd)
        {
            instr->SetSrc1(regOpnd);
        }
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
    //TraceIrBuilder(newOpcode, ElementSlot, layout);
    BuildElementSlot(newOpcode, offset, layout->SlotIndex, layout->Value, layout->Instance);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildAsmUnsigned1(Js::OpCodeAsmJs newOpcode, uint offset)
{
    Assert(!m_func->GetJITFunctionBody()->IsWasmFunction());
   // we do not support counter in loop body ,just read the layout here
   m_jnReader.GetLayout<Js::OpLayoutT_AsmUnsigned1<SizePolicy>>();
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildWasmLoopStart(Js::OpCodeAsmJs newOpcode, uint offset)
{
    Assert(m_func->GetJITFunctionBody()->IsWasmFunction());
    auto loopStart = m_jnReader.GetLayout<Js::OpLayoutT_WasmLoopStart<SizePolicy>>();
    if (IsLoopBody() && !this->GetJitLoopBodyData().IsLoopCurRegsInitialized())
    {
        // There can be nested loop, only initialize with the first loop found
        this->GetJitLoopBodyData().InitLoopCurRegs(loopStart->curRegs, BVFixed::New<JitArenaAllocator>(GetLastTmp(WAsmJs::LastType), m_tempAlloc));
    }
}

void
IRBuilderAsmJs::BuildElementSlot(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 slotIndex, Js::RegSlot value, Js::RegSlot instance)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));

    Assert(instance == 1 || newOpcode == Js::OpCodeAsmJs::LdArr_Func || newOpcode == Js::OpCodeAsmJs::LdArr_WasmFunc);

    Js::RegSlot valueRegSlot;
    IR::Opnd * slotOpnd;
    IR::RegOpnd * regOpnd;
    IR::Instr * instr = nullptr;

    WAsmJs::Types type;
    IRType irType;
    ValueType valueType;
    bool isStore = false;

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::LdSlot:
        valueRegSlot = GetRegSlotFromPtrReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlotArray, TyVar);

        regOpnd = BuildDstOpnd(valueRegSlot, TyMachReg);
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdSlotArr:
        valueRegSlot = GetRegSlotFromPtrReg(value);

        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, TyVar);

        regOpnd = BuildDstOpnd(valueRegSlot, TyMachReg);
        instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::LdArr_Func:
    {
        IR::RegOpnd * baseOpnd = BuildSrcOpnd(GetRegSlotFromPtrReg(instance), TyMachReg);
        IR::RegOpnd * indexOpnd = BuildSrcOpnd(GetRegSlotFromIntReg(slotIndex), TyUint32);

        IR::IndirOpnd * indirOpnd = IR::IndirOpnd::New(baseOpnd, indexOpnd, TyMachReg, m_func);

        regOpnd = BuildDstOpnd(GetRegSlotFromPtrReg(value), TyMachReg);
        instr = IR::Instr::New(Js::OpCode::LdAsmJsFunc, regOpnd, indirOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::LdArr_WasmFunc:
    {
        IR::RegOpnd * baseOpnd = BuildSrcOpnd(GetRegSlotFromPtrReg(instance), TyMachReg);
        IR::RegOpnd * indexOpnd = BuildSrcOpnd(GetRegSlotFromIntReg(slotIndex), TyUint32);

        regOpnd = BuildDstOpnd(GetRegSlotFromPtrReg(value), TyMachReg);
        instr = IR::Instr::New(Js::OpCode::LdWasmFunc, regOpnd, baseOpnd, indexOpnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::StSlot_Int:
    case Js::OpCodeAsmJs::LdSlot_Int:
        type = WAsmJs::INT32;
        irType = TyInt32;
        valueType = ValueType::GetInt(false);
        isStore = newOpcode == Js::OpCodeAsmJs::StSlot_Int;
        goto ProcessGenericSlot;

    case Js::OpCodeAsmJs::StSlot_Long:
    case Js::OpCodeAsmJs::LdSlot_Long:
        type = WAsmJs::INT64;
        irType = TyInt64;
        valueType = ValueType::GetInt(false);
        isStore = newOpcode == Js::OpCodeAsmJs::StSlot_Long;
        goto ProcessGenericSlot;

    case Js::OpCodeAsmJs::StSlot_Flt:
    case Js::OpCodeAsmJs::LdSlot_Flt:
        type = WAsmJs::FLOAT32;
        irType = TyFloat32;
        valueType = ValueType::Float;
        isStore = newOpcode == Js::OpCodeAsmJs::StSlot_Flt;
        goto ProcessGenericSlot;

    case Js::OpCodeAsmJs::StSlot_Db:
    case Js::OpCodeAsmJs::LdSlot_Db:
        type = WAsmJs::FLOAT64;
        irType = TyFloat64;
        valueType = ValueType::Float;
        isStore = newOpcode == Js::OpCodeAsmJs::StSlot_Db;
        goto ProcessGenericSlot;

    case Js::OpCodeAsmJs::Simd128_StSlot_I4:
    case Js::OpCodeAsmJs::Simd128_LdSlot_I4:
        irType = TySimd128I4;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_I4;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_B4:
    case Js::OpCodeAsmJs::Simd128_LdSlot_B4:
        irType = TySimd128B4;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_B4;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_B8:
    case Js::OpCodeAsmJs::Simd128_LdSlot_B8:
        irType = TySimd128B8;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_B8;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_B16:
    case Js::OpCodeAsmJs::Simd128_LdSlot_B16:
        irType = TySimd128B16;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_B16;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_F4:
    case Js::OpCodeAsmJs::Simd128_LdSlot_F4:
        irType = TySimd128F4;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_F4;
        goto ProcessSimdSlot;
#if 0
    case Js::OpCodeAsmJs::Simd128_StSlot_D2:
    case Js::OpCodeAsmJs::Simd128_LdSlot_D2:
        irType = TySimd128D2;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_D2;
        goto ProcessSimdSlot;
#endif // 0
    case Js::OpCodeAsmJs::Simd128_StSlot_I8:
    case Js::OpCodeAsmJs::Simd128_LdSlot_I8:
        irType = TySimd128I8;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_I8;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_I16:
    case Js::OpCodeAsmJs::Simd128_LdSlot_I16:
        irType = TySimd128I16;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_I16;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_U4:
    case Js::OpCodeAsmJs::Simd128_LdSlot_U4:
        irType = TySimd128U4;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_U4;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_U8:
    case Js::OpCodeAsmJs::Simd128_LdSlot_U8:
        irType = TySimd128U8;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_U8;
        goto ProcessSimdSlot;
    case Js::OpCodeAsmJs::Simd128_StSlot_U16:
    case Js::OpCodeAsmJs::Simd128_LdSlot_U16:
        irType = TySimd128U16;
        isStore = newOpcode == Js::OpCodeAsmJs::Simd128_StSlot_U16;
        goto ProcessSimdSlot;

    default:
        Assume(UNREACHED);
        break;
ProcessSimdSlot:
        type = WAsmJs::SIMD;
        valueType = GetSimdValueTypeFromIRType(irType);
ProcessGenericSlot:
        valueRegSlot = GetRegSlotFromTypedReg(value, type);
        slotOpnd = BuildFieldOpnd(AsmJsRegSlots::ModuleMemReg, slotIndex, PropertyKindSlots, irType);
        if (isStore)
        {
            regOpnd = BuildSrcOpnd(valueRegSlot, irType);
            regOpnd->SetValueType(valueType);
            instr = IR::Instr::New(Js::OpCode::StSlot, slotOpnd, regOpnd, m_func);
        }
        else
        {
            regOpnd = BuildDstOpnd(valueRegSlot, irType);
            regOpnd->SetValueType(valueType);
            instr = IR::Instr::New(Js::OpCode::LdSlot, regOpnd, slotOpnd, m_func);
        }
        break;
    }

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildStartCall(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(!OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));

    const unaligned Js::OpLayoutStartCall * layout = m_jnReader.StartCall();
    IR::RegOpnd * dstOpnd = IR::RegOpnd::New(TyMachReg, m_func);

    IR::Instr * instr = nullptr;
    StackSym * symDst = nullptr;
    IR::SymOpnd * argDst = nullptr;
    IR::AddrOpnd * addrOpnd = nullptr;
    IR::IntConstOpnd * srcOpnd = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::I_StartCall:
        srcOpnd = IR::IntConstOpnd::New(layout->ArgCount - MachPtr, TyUint16, m_func);
        instr = IR::Instr::New(Js::OpCode::StartCall, dstOpnd, m_func);
        // this is the arg size, we will set src1 to be the actual arg count
        instr->SetSrc2(srcOpnd);

        AddInstr(instr, offset);

        // save this so we can calculate arg offsets later on
        m_argOffsetStack->Push(layout->ArgCount);
        m_argStack->Push(instr);
        break;

    case Js::OpCodeAsmJs::StartCall:
        srcOpnd = IR::IntConstOpnd::New(layout->ArgCount, TyUint16, m_func);
        instr = IR::Instr::New(Js::OpCode::StartCall, dstOpnd, m_func);
        // we will set src1 to be the actual arg count
        instr->SetSrc2(srcOpnd);
        AddInstr(instr, offset);

        m_argStack->Push(instr);

        // also need to add undefined as arg0
        addrOpnd = IR::AddrOpnd::New(m_func->GetScriptContextInfo()->GetUndefinedAddr(), IR::AddrOpndKindDynamicVar, m_func, true);
        addrOpnd->SetValueType(ValueType::Undefined);

        symDst = m_func->m_symTable->GetArgSlotSym(1);

        argDst = IR::SymOpnd::New(symDst, TyMachReg, m_func);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, argDst, addrOpnd, m_func);

        AddInstr(instr, offset);
        m_argStack->Push(instr);

        break;
    default:
        Assume(UNREACHED);
    }
}

void
IRBuilderAsmJs::InitializeMemAccessTypeInfo(Js::ArrayBufferView::ViewType viewType, _Out_ MemAccessTypeInfo * typeInfo)
{
    AssertOrFailFast(typeInfo);

    switch (viewType)
    {
#define ARRAYBUFFER_VIEW(name, align, RegType, MemType, irSuffix) \
    case Js::ArrayBufferView::TYPE_##name: \
        typeInfo->valueRegType = WAsmJs::FromPrimitiveType<RegType>(); \
        typeInfo->type = Ty##irSuffix;\
        typeInfo->arrayType = ValueType::GetObject(ObjectType::##irSuffix##Array); \
        Assert(TySize[Ty##irSuffix] == (1<<align)); \
        break;
#include "Language/AsmJsArrayBufferViews.h"
    default:
        AssertOrFailFast(UNREACHED);
    }
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildWasmMemAccess(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_WasmMemAccess<SizePolicy>>();
    BuildWasmMemAccess(newOpcode, offset, layout->SlotIndex, layout->Value, layout->Offset, layout->ViewType);
}

void
IRBuilderAsmJs::BuildWasmMemAccess(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, uint32 constOffset, Js::ArrayBufferView::ViewType viewType)
{
    bool isAtomic = newOpcode == Js::OpCodeAsmJs::StArrAtomic || newOpcode == Js::OpCodeAsmJs::LdArrAtomic;
    bool isLd = newOpcode == Js::OpCodeAsmJs::LdArrWasm || newOpcode == Js::OpCodeAsmJs::LdArrAtomic;
    Js::OpCode op = isAtomic ? 
        isLd ? Js::OpCode::LdAtomicWasm : Js::OpCode::StAtomicWasm
        : isLd ? Js::OpCode::LdArrViewElemWasm : Js::OpCode::StArrViewElem;

    MemAccessTypeInfo typeInfo;
    InitializeMemAccessTypeInfo(viewType, &typeInfo);
    const uint32 memAccessSize = TySize[typeInfo.type];

    Js::RegSlot valueRegSlot = GetRegSlotFromTypedReg(value, typeInfo.valueRegType);
    IR::Instr * instr = nullptr;
    IR::RegOpnd * regOpnd = nullptr;
    IR::IndirOpnd * indirOpnd = nullptr;

    Js::RegSlot indexRegSlot = GetRegSlotFromIntReg(slotIndex);
    IR::RegOpnd * indexOpnd = BuildSrcOpnd(indexRegSlot, TyUint32);
    if (isAtomic && memAccessSize > 1)
    {
        const uint32 mask = memAccessSize - 1;
        // We need (constOffset + index) & mask == 0
        // Since we know constOffset ahead of time
        // what we need to check is index & mask == (memAccessSize - (constOffset & mask)) & mask
        const uint32 offseted = constOffset & mask;
        // In this IntContOpnd, the value is what the index&mask should be, the type carries the size of the access
        IR::Opnd* offsetedOpnd = IR::IntConstOpnd::NewFromType((memAccessSize - offseted) & mask, typeInfo.type, m_func);
        IR::RegOpnd* intermediateIndex = IR::RegOpnd::New(TyUint32, m_func);
        instr = IR::Instr::New(Js::OpCode::TrapIfUnalignedAccess, intermediateIndex, indexOpnd, offsetedOpnd, m_func);
        AddInstr(instr, offset);

        // Create dependency between load/store and trap through the index
        indexOpnd = intermediateIndex;
    }
    indirOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::BufferReg, TyVar), constOffset, typeInfo.type, m_func);
    indirOpnd->SetIndexOpnd(indexOpnd);
    indirOpnd->GetBaseOpnd()->SetValueType(typeInfo.arrayType);

    // Setup the value/destination
    if (typeInfo.valueRegType == WAsmJs::FLOAT32 || typeInfo.valueRegType == WAsmJs::FLOAT64)
    {
        Assert(IRType_IsFloat(typeInfo.type));
        regOpnd = !isLd ? BuildSrcOpnd(valueRegSlot, typeInfo.type) : BuildDstOpnd(valueRegSlot, typeInfo.type);
        regOpnd->SetValueType(ValueType::Float);
    }
    else if (typeInfo.valueRegType == WAsmJs::INT64)
    {
        Assert(IRType_IsNativeInt(typeInfo.type));
        regOpnd = !isLd ? BuildSrcOpnd(valueRegSlot, TyInt64) : BuildDstOpnd(valueRegSlot, TyInt64);
        regOpnd->SetValueType(ValueType::GetInt(false));
    }
    else
    {
        Assert(IRType_IsNativeInt(typeInfo.type));
        Assert(typeInfo.valueRegType == WAsmJs::INT32);
        regOpnd = !isLd ? BuildSrcOpnd(valueRegSlot, TyInt32) : BuildDstOpnd(valueRegSlot, TyInt32);
        regOpnd->SetValueType(ValueType::GetInt(false));
    }

    // Create the instruction
    if (isLd)
    {
        instr = IR::Instr::New(op, regOpnd, indirOpnd, m_func);
    }
    else
    {
        instr = IR::Instr::New(op, indirOpnd, regOpnd, m_func);
    }

    if (!m_func->GetJITFunctionBody()->UsesWAsmJsFastVirtualBuffer())
    {
        instr->SetSrc2(BuildSrcOpnd(AsmJsRegSlots::LengthReg, TyUint32));
    }
    AddInstr(instr, offset);

#if DBG && defined(ENABLE_WASM)
    if (newOpcode == Js::OpCodeAsmJs::StArrWasm && PHASE_TRACE(Js::WasmMemWritesPhase, m_func))
    {
        IR::Opnd* prevArg = nullptr;
        auto PushArg = [&](ValueType valueType, IR::Opnd* srcOpnd) {
            IR::RegOpnd* dstOpnd = IR::RegOpnd::New(srcOpnd->GetType(), m_func);
            dstOpnd->SetValueType(valueType);
            IR::Instr* instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
            if (prevArg)
            {
                instr->SetSrc2(prevArg);
            }
            prevArg = dstOpnd;
            AddInstr(instr, offset);
        };

        // static void TraceMemWrite(WebAssemblyMemory* mem, uint32 index, uint32 offset, Js::ArrayBufferView::ViewType viewType, uint bytecodeOffset, ScriptContext* context);
        // ScriptContext is added automatically by CallHelper lower
        PushArg(ValueType::GetInt(false), IR::IntConstOpnd::New(offset, TyUint32, m_func, true));
        PushArg(ValueType::GetInt(false), IR::IntConstOpnd::New(viewType, TyUint8, m_func, true));
        PushArg(ValueType::GetInt(false), IR::IntConstOpnd::New(constOffset, TyUint32, m_func));
        PushArg(ValueType::GetInt(false), indexOpnd);
        PushArg(ValueType::GetObject(ObjectType::Object), BuildSrcOpnd(AsmJsRegSlots::WasmMemoryReg, TyVar));

        IR::Instr* callInstr = IR::Instr::New(Js::OpCode::CallHelper, m_func);
        callInstr->SetSrc1(IR::HelperCallOpnd::New(IR::HelperOp_WasmMemoryTraceWrite, m_func));
        callInstr->SetSrc2(prevArg);
        AddInstr(callInstr, offset);
    }
#endif
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
IRBuilderAsmJs::BuildAsmTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, Js::ArrayBufferView::ViewType viewType)
{
    bool isLd = newOpcode == Js::OpCodeAsmJs::LdArr || newOpcode == Js::OpCodeAsmJs::LdArrConst;
    Js::OpCode op = isLd ? Js::OpCode::LdArrViewElem : Js::OpCode::StArrViewElem;

    MemAccessTypeInfo typeInfo;
    InitializeMemAccessTypeInfo(viewType, &typeInfo);

    Js::RegSlot valueRegSlot = GetRegSlotFromTypedReg(value, typeInfo.valueRegType);
    IR::Instr * instr = nullptr;
    IR::Instr * maskInstr = nullptr;
    IR::RegOpnd * regOpnd = nullptr;
    IR::IndirOpnd * indirOpnd = nullptr;

    // Get the index
    if (newOpcode == Js::OpCodeAsmJs::LdArr || newOpcode == Js::OpCodeAsmJs::StArr)
    {
        uint32 mask = Js::ArrayBufferView::ViewMask[viewType];
        Js::RegSlot indexRegSlot = GetRegSlotFromIntReg(slotIndex);
        IR::RegOpnd * maskedOpnd = nullptr;
        if (mask != ~0)
        {
            maskedOpnd = IR::RegOpnd::New(TyUint32, m_func);
            maskInstr = IR::Instr::New(Js::OpCode::And_I4, maskedOpnd, BuildSrcOpnd(indexRegSlot, TyInt32), IR::IntConstOpnd::New(mask, TyUint32, m_func), m_func);
            AddInstr(maskInstr, offset);
        }
        else
        {
            maskedOpnd = BuildSrcOpnd(indexRegSlot, TyInt32);
        }
        indirOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::BufferReg, TyVar), maskedOpnd, typeInfo.type, m_func);
        indirOpnd->GetBaseOpnd()->SetValueType(typeInfo.arrayType);
    }
    else
    {
        Assert(newOpcode == Js::OpCodeAsmJs::LdArrConst || newOpcode == Js::OpCodeAsmJs::StArrConst);
        indirOpnd = IR::IndirOpnd::New(BuildSrcOpnd(AsmJsRegSlots::BufferReg, TyVar), slotIndex, typeInfo.type, m_func);
        indirOpnd->GetBaseOpnd()->SetValueType(typeInfo.arrayType);
    }

    // Setup the value/destination
    if (typeInfo.valueRegType == WAsmJs::FLOAT32 || typeInfo.valueRegType == WAsmJs::FLOAT64)
    {
        Assert(IRType_IsFloat(typeInfo.type));
        regOpnd = !isLd ? BuildSrcOpnd(valueRegSlot, typeInfo.type) : BuildDstOpnd(valueRegSlot, typeInfo.type);
        regOpnd->SetValueType(ValueType::Float);
    }
    else
    {
        Assert(IRType_IsNativeInt(typeInfo.type));
        Assert(typeInfo.valueRegType == WAsmJs::INT32);
        regOpnd = !isLd ? BuildSrcOpnd(valueRegSlot, TyInt32) : BuildDstOpnd(valueRegSlot, TyInt32);
        regOpnd->SetValueType(ValueType::GetInt(false));
    }

    // Create the instruction
    if (isLd)
    {
        instr = IR::Instr::New(op, regOpnd, indirOpnd, m_func);
    }
    else
    {
        instr = IR::Instr::New(op, indirOpnd, regOpnd, m_func);
    }

    if (!m_func->GetJITFunctionBody()->UsesWAsmJsFastVirtualBuffer())
    {
        instr->SetSrc2(BuildSrcOpnd(AsmJsRegSlots::LengthReg, TyUint32));
    }
    AddInstr(instr, offset);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildProfiledAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutDynamicProfile<Js::OpLayoutT_AsmCall<SizePolicy>>>();
    BuildAsmCall(newOpcode, offset, layout->ArgCount, layout->Return, layout->Function, layout->ReturnType, layout->profileId);
}

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_AsmCall<SizePolicy>>();
    BuildAsmCall(newOpcode, offset, layout->ArgCount, layout->Return, layout->Function, layout->ReturnType, Js::Constants::NoProfileId);
}

void
IRBuilderAsmJs::BuildAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::ArgSlot argCount, Js::RegSlot ret, Js::RegSlot function, int8 returnType, Js::ProfileId profileId)
{
    Js::ArgSlot count = 0;

    IR::Instr * instr = nullptr;
    IR::RegOpnd * dstOpnd = nullptr;
    IR::Opnd * srcOpnd = nullptr;
    Js::RegSlot dstRegSlot;
    Js::RegSlot srcRegSlot;
    int32 argOffset = 0;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::I_Call:
    case Js::OpCodeAsmJs::ProfiledI_Call:
        srcRegSlot = GetRegSlotFromPtrReg(function);
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyMachReg);

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

        case Js::AsmJsRetType::Which::Int64:
            dstRegSlot = GetRegSlotFromInt64Reg(ret);
            dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
            dstOpnd->SetValueType(ValueType::GetInt(false));
            break;

        case Js::AsmJsRetType::Which::Void:
            break;

#ifdef ENABLE_WASM_SIMD
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
#endif
        default:
            Assume(UNREACHED);
        }

        {
            if (profileId == Js::Constants::NoProfileId)
            {
                instr = IR::Instr::New(Js::OpCode::AsmJsCallI, dstOpnd, srcOpnd, m_func);
            }
            else
            {
                Assert(newOpcode == Js::OpCodeAsmJs::ProfiledI_Call);
                instr = IR::ProfiledInstr::New(Js::OpCode::AsmJsCallI, dstOpnd, srcOpnd, m_func);
                AssertOrFailFast(profileId < m_func->GetJITFunctionBody()->GetProfiledCallSiteCount());
                instr->AsProfiledInstr()->u.profileId = profileId;
            }
        }
        AssertOrFailFast(!this->m_argOffsetStack->Empty());
        argOffset = m_argOffsetStack->Pop();
        argOffset -= MachPtr;
        break;

    case Js::OpCodeAsmJs::Call:
        srcRegSlot = GetRegSlotFromPtrReg(function);
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyMachReg);

        dstRegSlot = GetRegSlotFromPtrReg(ret);
        dstOpnd = BuildDstOpnd(dstRegSlot, TyMachReg);

        instr = IR::Instr::New(Js::OpCode::AsmJsCallE, dstOpnd, srcOpnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);

    IR::Instr * argInstr = nullptr;
    IR::Instr * prevInstr = instr;

    AssertOrFailFast(!this->m_argStack->Empty());
    for (argInstr = m_argStack->Pop(); !m_argStack->Empty() && argInstr->m_opcode != Js::OpCode::StartCall; argInstr = m_argStack->Pop())
    {
        if (newOpcode == Js::OpCodeAsmJs::I_Call || newOpcode == Js::OpCodeAsmJs::ProfiledI_Call)
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
        count++;
    }

    AssertOrFailFast(argInstr->m_opcode == Js::OpCode::StartCall);
    argInstr->SetSrc1(IR::IntConstOpnd::New(count, TyUint16, m_func));

    AssertOrFailFast(argOffset == 0);
    prevInstr->SetSrc2(argInstr->GetDst());

    // todo:: are we sure we don't need this for wasm ?
#ifdef ENABLE_SIMDJS
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
            if (newOpcode == Js::OpCodeAsmJs::I_Call || newOpcode == Js::OpCodeAsmJs::ProfiledI_Call)
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
#endif
    if (m_func->m_argSlotsForFunctionsCalled < argCount)
    {
        m_func->m_argSlotsForFunctionsCalled = argCount;
    }
#ifdef ENABLE_WASM
    // heap buffer can change for wasm
    if (m_asmFuncInfo->UsesHeapBuffer() && m_func->GetJITFunctionBody()->IsWasmFunction())
    {
        BuildHeapBufferReload(offset);
    }
#endif
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
    Assert(newOpcode == Js::OpCodeAsmJs::MemorySize_Int);
    Js::RegSlot dstRegSlot = GetRegSlotFromIntReg(dstReg);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    IR::IntConstOpnd* constZero = IR::IntConstOpnd::New(0, TyInt32, m_func);
    IR::IntConstOpnd* constSixteen = IR::IntConstOpnd::New(16, TyUint8, m_func);

    IR::Instr * instr = m_asmFuncInfo->UsesHeapBuffer() ?
        IR::Instr::New(Js::OpCode::ShrU_I4, dstOpnd, BuildSrcOpnd(AsmJsRegSlots::LengthReg, TyUint32), constSixteen, m_func) :
        IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, constZero, m_func);

    AddInstr(instr, offset);

}

#define BUILD_LAYOUT_IMPL(layout, ...) \
    template <typename SizePolicy> void IRBuilderAsmJs::Build##layout (Js::OpCodeAsmJs newOpcode, uint32 offset) \
    { \
        Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));\
        auto _layout = m_jnReader.GetLayout<Js::OpLayoutT_##layout <SizePolicy>>();\
        Build##layout(newOpcode, offset, __VA_ARGS__);\
    }

#define RegProc(v) v
#define IntProc(v) GetRegSlotFromIntReg(v)
#define LongProc(v) GetRegSlotFromInt64Reg(v)
#define FloatProc(v) GetRegSlotFromFloatReg(v)
#define DoubleProc(v) GetRegSlotFromDoubleReg(v)
#define PtrProc(v) GetRegSlotFromPtrReg(v)
#define IntConstProc(v) v
#define LongConstProc(v) v
#define FloatConstProc(v) v
#define DoubleConstProc(v) v
#define Float32x4Proc(v) GetRegSlotFromSimd128Reg(v)
#define Bool32x4Proc(v) GetRegSlotFromSimd128Reg(v)
#define Int32x4Proc(v) GetRegSlotFromSimd128Reg(v)
#define Float64x2Proc(v) GetRegSlotFromSimd128Reg(v)
#define Int64x2Proc(v) GetRegSlotFromSimd128Reg(v)
#define Int16x8Proc(v) GetRegSlotFromSimd128Reg(v)
#define Bool16x8Proc(v) GetRegSlotFromSimd128Reg(v)
#define Int8x16Proc(v) GetRegSlotFromSimd128Reg(v)
#define Bool8x16Proc(v) GetRegSlotFromSimd128Reg(v)
#define Uint32x4Proc(v) GetRegSlotFromSimd128Reg(v)
#define Uint16x8Proc(v) GetRegSlotFromSimd128Reg(v)
#define Uint8x16Proc(v) GetRegSlotFromSimd128Reg(v)
#define _PREFIX_HELPER(prefix, index) prefix##index
#define _PREFIX_NAME(prefix, index) _PREFIX_HELPER(prefix, index)
#define _M(ti, i) ti##Proc(_layout-> _PREFIX_NAME(LAYOUT_PREFIX_##ti(), i))
#define LAYOUT_TYPE_WMS_REG2(layout, t0, t1) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1))
#define LAYOUT_TYPE_WMS_REG3(layout, t0, t1, t2) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2))
#define LAYOUT_TYPE_WMS_REG4(layout, t0, t1, t2, t3) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3))
#define LAYOUT_TYPE_WMS_REG5(layout, t0, t1, t2, t3, t4) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4))
#define LAYOUT_TYPE_WMS_REG6(layout, t0, t1, t2, t3, t4, t5) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5))
#define LAYOUT_TYPE_WMS_REG7(layout, t0, t1, t2, t3, t4, t5, t6) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5), _M(t6, 6))
#define LAYOUT_TYPE_WMS_REG9(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5), _M(t6, 6), _M(t7, 7), _M(t8, 8))
#define LAYOUT_TYPE_WMS_REG10(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5), _M(t6, 6), _M(t7, 7), _M(t8, 8), _M(t9, 9))
#define LAYOUT_TYPE_WMS_REG11(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5), _M(t6, 6), _M(t7, 7), _M(t8, 8), _M(t9, 9), _M(t10, 10))
#define LAYOUT_TYPE_WMS_REG17(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5), _M(t6, 6), _M(t7, 7), _M(t8, 8), _M(t9, 9), _M(t10, 10), _M(t11, 11), _M(t12, 12), _M(t13, 13), _M(t14, 14), _M(t15, 15), _M(t16, 16))
#define LAYOUT_TYPE_WMS_REG18(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5), _M(t6, 6), _M(t7, 7), _M(t8, 8), _M(t9, 9), _M(t10, 10), _M(t11, 11), _M(t12, 12), _M(t13, 13), _M(t14, 14), _M(t15, 15), _M(t16, 16), _M(t17, 17))
#define LAYOUT_TYPE_WMS_REG19(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18) BUILD_LAYOUT_IMPL(layout, _M(t0, 0), _M(t1, 1), _M(t2, 2), _M(t3, 3), _M(t4, 4), _M(t5, 5), _M(t6, 6), _M(t7, 7), _M(t8, 8), _M(t9, 9), _M(t10, 10), _M(t11, 11), _M(t12, 12), _M(t13, 13), _M(t14, 14), _M(t15, 15), _M(t16, 16), _M(t17, 17), _M(t18, 18))
#define EXCLUDE_FRONTEND_LAYOUT
#include "LayoutTypesAsmJs.h"
#undef BUILD_LAYOUT_IMPL
#undef _PREFIX_NAME
#undef _PREFIX_HELPER
#undef _M
#undef RegProc
#undef PtrProc
#undef IntProc
#undef LongProc
#undef FloatProc
#undef DoubleProc
#undef IntConstProc
#undef LongConstProc
#undef FloatConstProc
#undef DoubleConstProc
#undef Float32x4Proc
#undef Bool32x4Proc
#undef Int32x4Proc
#undef Float64x2Proc
#undef Int16x8Proc
#undef Bool16x8Proc
#undef Int8x16Proc
#undef Bool8x16Proc
#undef Uint32x4Proc
#undef Uint16x8Proc
#undef Uint8x16Proc

void IRBuilderAsmJs::BuildArgOut(IR::Opnd* srcOpnd, uint32 dstRegSlot, uint32 offset, IRType type, ValueType valueType)
{
    Js::ArgSlot dstArgSlot = (Js::ArgSlot)dstRegSlot;
    if ((uint32)dstArgSlot != dstRegSlot)
    {
        AssertMsg(UNREACHED, "Arg count too big...");
        Fatal();
    }
    StackSym * symDst = nullptr;
    if (type == TyVar)
    {
        symDst = m_func->m_symTable->GetArgSlotSym(ArgSlotMath::Add(dstArgSlot, 1));
        IR::Opnd * tmpDst = IR::RegOpnd::New(StackSym::New(m_func), TyVar, m_func);

        IR::Instr * instr = IR::Instr::New(Js::OpCode::ToVar, tmpDst, srcOpnd, m_func);
        AddInstr(instr, offset);
        srcOpnd = tmpDst;
    }
    else
    {
        // Some arg types may use multiple slots, so can't rely on count from bytecode
        Assert(dstArgSlot >= m_argStack->Count());
        uint slotIndex = m_argStack->Count();
        AssertOrFailFast(slotIndex < UINT16_MAX);

        symDst = StackSym::NewArgSlotSym((Js::ArgSlot)slotIndex, m_func, type);
        symDst->m_allocated = true;
    }

    IR::Opnd * dstOpnd = IR::SymOpnd::New(symDst, type, m_func);
    if (!valueType.IsUninitialized())
    {
        dstOpnd->SetValueType(valueType);
    }

    IR::Instr * instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);

    m_argStack->Push(instr);
}

void IRBuilderAsmJs::BuildFromVar(uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot, IRType irType, ValueType valueType)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(GetRegSlotFromPtrReg(srcRegSlot), TyVar);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, irType);
    dstOpnd->SetValueType(valueType);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::FromVar, dstOpnd, srcOpnd, m_func);

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);
    IR::RegOpnd * dstOpnd = nullptr;
    Js::OpCode op = Js::OpCode::Nop;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_DTI:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
        op = Js::OpCode::Conv_Prim;
        break;
    case Js::OpCodeAsmJs::Conv_DTU:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyUint32);
        op = Js::OpCode::Conv_Prim;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_DTI:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
        op = Js::OpCode::Conv_Prim_Sat;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_DTU:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyUint32);
        op = Js::OpCode::Conv_Prim_Sat;
        break;
    case Js::OpCodeAsmJs::Conv_Check_DTI:
    case Js::OpCodeAsmJs::Conv_Check_DTU:
    {
        IR::RegOpnd* tmpDst = IR::RegOpnd::New(TyFloat64, m_func);
        op = Js::OpCode::Conv_Prim;
        tmpDst->SetValueType(ValueType::Float);
        AddInstr(IR::Instr::New(Js::OpCode::TrapIfTruncOverflow, tmpDst, srcOpnd, m_func), offset);
        dstOpnd = BuildDstOpnd(dstRegSlot, newOpcode == Js::OpCodeAsmJs::Conv_Check_DTI ? TyInt32 : TyUint32);
        dstOpnd->m_dontDeadStore = true;
        srcOpnd = tmpDst;
        break;
    }
    default:
        Assume(UNREACHED);
    }
    dstOpnd->SetValueType(ValueType::GetInt(false));
    IR::Instr * instr = IR::Instr::New(op, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat32);
    srcOpnd->SetValueType(ValueType::Float);
    IR::RegOpnd * dstOpnd = nullptr;
    Js::OpCode op = Js::OpCode::Nop;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_FTI:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
        op = Js::OpCode::Conv_Prim;
        break;
    case Js::OpCodeAsmJs::Conv_FTU:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyUint32);
        op = Js::OpCode::Conv_Prim;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_FTI:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
        op = Js::OpCode::Conv_Prim_Sat;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_FTU:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyUint32);
        op = Js::OpCode::Conv_Prim_Sat;
        break;
    case Js::OpCodeAsmJs::Reinterpret_FTI:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
        op = Js::OpCode::Reinterpret_Prim;
        break;

    case Js::OpCodeAsmJs::Conv_Check_FTI:
    case Js::OpCodeAsmJs::Conv_Check_FTU:
    {
        IR::RegOpnd* tmpDst = IR::RegOpnd::New(TyFloat32, m_func);
        tmpDst->SetValueType(ValueType::Float);
        AddInstr(IR::Instr::New(Js::OpCode::TrapIfTruncOverflow, tmpDst, srcOpnd, m_func), offset);
        dstOpnd = BuildDstOpnd(dstRegSlot, newOpcode == Js::OpCodeAsmJs::Conv_Check_FTI ? TyInt32 : TyUint32);
        dstOpnd->m_dontDeadStore = true;
        srcOpnd = tmpDst;
        op = Js::OpCode::Conv_Prim;
        break;
    }
    default:
        Assume(UNREACHED);
    }
    dstOpnd->SetValueType(ValueType::GetInt(false));
    IR::Instr * instr = IR::Instr::New(op, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildDouble1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
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

void
IRBuilderAsmJs::BuildDouble1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_FTD);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat32);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_VTF);
    BuildFromVar(offset, dstRegSlot, srcRegSlot, TyFloat32, ValueType::Float);
}

void
IRBuilderAsmJs::BuildDouble1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_VTD);
    BuildFromVar(offset, dstRegSlot, srcRegSlot, TyFloat64, ValueType::Float);
}

void
IRBuilderAsmJs::BuildInt1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_VTI);
    BuildFromVar(offset, dstRegSlot, srcRegSlot, TyInt32, ValueType::GetInt(false));
}

void
IRBuilderAsmJs::BuildReg1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::ArgOut_Db:
        BuildArgOut(srcOpnd, dstReg, offset, TyVar);
        break;
    case Js::OpCodeAsmJs::I_ArgOut_Db:
        BuildArgOut(srcOpnd, dstReg, offset, TyFloat64, ValueType::Float);
        break;
    default:
        Assume(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildReg1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat32);
    srcOpnd->SetValueType(ValueType::Float);

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::ArgOut_Flt:
        BuildArgOut(srcOpnd, dstReg, offset, TyVar);
        break;
    case Js::OpCodeAsmJs::I_ArgOut_Flt:
        BuildArgOut(srcOpnd, dstReg, offset, TyFloat32, ValueType::Float);
        break;
    default:
        Assume(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildReg1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
    srcOpnd->SetValueType(ValueType::GetInt(false));

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::ArgOut_Int:
        BuildArgOut(srcOpnd, dstReg, offset, TyVar);
        break;
    case Js::OpCodeAsmJs::I_ArgOut_Int:
        BuildArgOut(srcOpnd, dstReg, offset, TyInt32, ValueType::GetInt(false));
        break;
    default:
        Assume(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildFloat32x4_IntConst4(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, int C1, int C2, int C3, int C4)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_LdC);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);
    SIMDValue simdConst{ C1, C2, C3, C4 };
    IR::Instr * instr = IR::Instr::New(Js::OpCode::Simd128_LdC, dstOpnd, IR::Simd128ConstOpnd::New(simdConst, TySimd128F4, m_func), m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, int constInt)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Ld_IntConst);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, IR::IntConstOpnd::New(constInt, TyInt32, m_func), m_func);

    if (dstOpnd->m_sym->IsSingleDef())
    {
        dstOpnd->m_sym->SetIsIntConst(constInt);
    }

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildReg1IntConst1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot reg1, int constInt)
{
    Assert(newOpcode == Js::OpCodeAsmJs::CheckSignature);

    IR::RegOpnd * funcReg = BuildSrcOpnd(GetRegSlotFromPtrReg(reg1), TyMachReg);

    IR::IntConstOpnd * sigIndex = IR::IntConstOpnd::New(constInt, TyInt32, m_func);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::CheckWasmSignature, m_func);
    instr->SetSrc1(funcReg);
    instr->SetSrc2(sigIndex);

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, float constVal)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Ld_FltConst);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::LdC_F8_R8, dstOpnd, IR::Float32ConstOpnd::New(constVal, TyFloat32, m_func), m_func);

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildDouble1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, double constVal)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Ld_DbConst);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::LdC_F8_R8, dstOpnd, IR::FloatConstOpnd::New(constVal, TyFloat64, m_func), m_func);

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Double2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot)
{
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

void
IRBuilderAsmJs::BuildInt1Float2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot)
{
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

void
IRBuilderAsmJs::BuildInt2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
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
        instr = IR::Instr::New(Js::OpCode::InlineMathClz, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Ctz_Int:
        instr = IR::Instr::New(Js::OpCode::Ctz, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::PopCnt_Int:
        instr = IR::Instr::New(Js::OpCode::PopCnt, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Return_Int:
        instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, srcOpnd, m_func);
        CheckJitLoopReturn(dstRegSlot, TyInt32);
        break;

    case Js::OpCodeAsmJs::Eqz_Int:
        instr = IR::Instr::New(Js::OpCode::CmEq_I4, dstOpnd, srcOpnd, IR::IntConstOpnd::New(0, TyInt32, m_func), m_func);
        break;

    case Js::OpCodeAsmJs::GrowMemory:
        instr = IR::Instr::New(Js::OpCode::GrowWasmMemory, dstOpnd, BuildSrcOpnd(AsmJsRegSlots::WasmMemoryReg, TyVar), srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::I32Extend8_s:
        instr = CreateSignExtendInstr(dstOpnd, srcOpnd, TyInt8);
        break;
    case Js::OpCodeAsmJs::I32Extend16_s:
        instr = CreateSignExtendInstr(dstOpnd, srcOpnd, TyInt16);
        break;
    default:
        Assume(UNREACHED);
    }

    AddInstr(instr, offset);

    if (newOpcode == Js::OpCodeAsmJs::GrowMemory)
    {
        BuildHeapBufferReload(offset);
    }
}

IR::RegOpnd* IRBuilderAsmJs::BuildTrapIfZero(IR::RegOpnd* srcOpnd, uint32 offset)
{
    IR::RegOpnd* newSrc = IR::RegOpnd::New(srcOpnd->GetType(), m_func);
    newSrc->SetValueType(ValueType::GetInt(false));
    AddInstr(IR::Instr::New(Js::OpCode::TrapIfZero, newSrc, srcOpnd, m_func), offset);
    return newSrc;
}

IR::RegOpnd* IRBuilderAsmJs::BuildTrapIfMinIntOverNegOne(IR::RegOpnd* src1Opnd, IR::RegOpnd* src2Opnd, uint32 offset)
{
    IR::RegOpnd* newSrc = IR::RegOpnd::New(src2Opnd->GetType(), m_func);
    newSrc->SetValueType(ValueType::GetInt(false));
    AddInstr(IR::Instr::New(Js::OpCode::TrapIfMinIntOverNegOne, newSrc, src1Opnd, src2Opnd, m_func), offset);
    return newSrc;
}

IR::Instr* IRBuilderAsmJs::CreateSignExtendInstr(IR::Opnd* dst, IR::Opnd* src, IRType fromType)
{
    // Since CSE ignores the type of the type, the int const value carries that information to prevent
    // cse of sign extension of different types.
    IR::Opnd* fromTypeOpnd = IR::IntConstOpnd::New(fromType, fromType, m_func);
    // Src2 is a dummy source, used only to carry the type to cast from
    return IR::Instr::New(Js::OpCode::Conv_Prim, dst, src, fromTypeOpnd, m_func);
}

void
IRBuilderAsmJs::BuildInt3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot)
{
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

    case Js::OpCodeAsmJs::Mul_Int:
        instr = IR::Instr::New(Js::OpCode::Mul_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Div_Trap_UInt:
        src1Opnd->SetType(TyUint32);
        src2Opnd->SetType(TyUint32);
        // Fall through for trap
    case Js::OpCodeAsmJs::Div_Trap_Int:
#ifdef _WIN32
        if (CONFIG_FLAG(WasmMathExFilter))
        {
            // Do not emit traps, but make sure we don't remove the div
            dstOpnd->m_dontDeadStore = true;
        }
        else
#endif
        {
            src2Opnd = BuildTrapIfZero(src2Opnd, offset);
            if (newOpcode == Js::OpCodeAsmJs::Div_Trap_Int)
            {
                src1Opnd = BuildTrapIfMinIntOverNegOne(src1Opnd, src2Opnd, offset);
            }
        }
        instr = IR::Instr::New(newOpcode == Js::OpCodeAsmJs::Div_Trap_UInt ? Js::OpCode::DivU_I4 : Js::OpCode::Div_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Div_UInt:
        src1Opnd->SetType(TyUint32);
        src2Opnd->SetType(TyUint32);
        instr = IR::Instr::New(Js::OpCode::DivU_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Div_Int:
        instr = IR::Instr::New(Js::OpCode::Div_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Rem_Trap_UInt:
        src1Opnd->SetType(TyUint32);
        src2Opnd->SetType(TyUint32);
        // Fall through for trap
    case Js::OpCodeAsmJs::Rem_Trap_Int:
#ifdef _WIN32
        if (CONFIG_FLAG(WasmMathExFilter))
        {
            // Do not emit traps, but make sure we don't remove the rem
            dstOpnd->m_dontDeadStore = true;
        }
        else
#endif
        {
            src2Opnd = BuildTrapIfZero(src2Opnd, offset);
        }
        instr = IR::Instr::New(newOpcode == Js::OpCodeAsmJs::Rem_Trap_UInt ? Js::OpCode::RemU_I4 : Js::OpCode::Rem_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Rem_UInt:
        src1Opnd->SetType(TyUint32);
        src2Opnd->SetType(TyUint32);
        instr = IR::Instr::New(Js::OpCode::RemU_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
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
    case Js::OpCodeAsmJs::Shr_UInt:
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
    case Js::OpCodeAsmJs::CmLt_UInt:
        instr = IR::Instr::New(Js::OpCode::CmUnLt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLe_UInt:
        instr = IR::Instr::New(Js::OpCode::CmUnLe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGt_UInt:
        instr = IR::Instr::New(Js::OpCode::CmUnGt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGe_UInt:
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
    case Js::OpCodeAsmJs::Rol_Int:
        instr = IR::Instr::New(Js::OpCode::Rol_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Ror_Int:
        instr = IR::Instr::New(Js::OpCode::Ror_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildDouble2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
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
        CheckJitLoopReturn(dstRegSlot, TyFloat64);
        break;
    case Js::OpCodeAsmJs::Trunc_Db:
        instr = IR::Instr::New(Js::OpCode::Trunc_A, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Nearest_Db:
        instr = IR::Instr::New(Js::OpCode::Nearest_A, dstOpnd, srcOpnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
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
        CheckJitLoopReturn(dstRegSlot, TyFloat32);
        break;
    case Js::OpCodeAsmJs::Trunc_Flt:
        instr = IR::Instr::New(Js::OpCode::Trunc_A, dstOpnd, srcOpnd, m_func);
        break;
    case Js::OpCodeAsmJs::Nearest_Flt:
        instr = IR::Instr::New(Js::OpCode::Nearest_A, dstOpnd, srcOpnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot)
{
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

    case Js::OpCodeAsmJs::Copysign_Flt:
        instr = IR::Instr::New(Js::OpCode::Copysign_A, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Min_Flt:
        instr = IR::Instr::New(Js::OpCode::InlineMathMin, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Max_Flt:
        instr = IR::Instr::New(Js::OpCode::InlineMathMax, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Fround_Db);

    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::InlineMathFround, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Fround_Int || newOpcode == Js::OpCodeAsmJs::Conv_UTF || newOpcode == Js::OpCodeAsmJs::Reinterpret_ITF);

    Js::OpCode op = Js::OpCode::Conv_Prim;
    IR::RegOpnd * srcOpnd = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Fround_Int:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
        break;
    case Js::OpCodeAsmJs::Conv_UTF:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyUint32);
        break;
    case Js::OpCodeAsmJs::Reinterpret_ITF:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
        op = Js::OpCode::Reinterpret_Prim;
        break;
    default:
        Assume(UNREACHED);
    }

    srcOpnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(op, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildDouble3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot)
{
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

    case Js::OpCodeAsmJs::Copysign_Db:
        instr = IR::Instr::New(Js::OpCode::Copysign_A, dstOpnd, src1Opnd, src2Opnd, m_func);
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
    Js::OpCode op = Js::OpCode::Nop;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::BrTrue_Int:
        op = Js::OpCode::BrTrue_I4;
        break;
    case Js::OpCodeAsmJs::BrFalse_Int:
        op = Js::OpCode::BrFalse_I4;
        break;
    default:
        Assume(UNREACHED);
    }
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(src);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt32);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    uint targetOffset = m_jnReader.GetCurrentOffset() + relativeOffset;

    IR::BranchInstr * branchInstr = IR::BranchInstr::New(op, nullptr, src1Opnd, m_func);
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

template <typename SizePolicy>
void
IRBuilderAsmJs::BuildBrInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_BrInt1Const1<SizePolicy>>();
    BuildBrInt1Const1(newOpcode, offset, layout->RelativeJumpOffset, layout->I1, layout->C1);
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

    BuildBrCmp(newOpcode, offset, relativeOffset, src1Opnd, src2Opnd);
}

void
IRBuilderAsmJs::BuildBrInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, int32 src2)
{
    Js::RegSlot src1RegSlot = GetRegSlotFromIntReg(src1);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt32);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    IR::Opnd * src2Opnd = IR::IntConstOpnd::New(src2, TyInt32, this->m_func);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    BuildBrCmp(newOpcode, offset, relativeOffset, src1Opnd, src2Opnd);
}

void
IRBuilderAsmJs::BuildBrCmp(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, IR::RegOpnd* src1Opnd, IR::Opnd* src2Opnd)
{
    uint targetOffset = m_jnReader.GetCurrentOffset() + relativeOffset;

    if (newOpcode == Js::OpCodeAsmJs::Case_Int || newOpcode == Js::OpCodeAsmJs::Case_IntConst)
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

void
IRBuilderAsmJs::BuildLong1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Conv_VTL);
    BuildFromVar(offset, dstRegSlot, src1RegSlot, TyInt64, ValueType::GetInt(false));
}

void
IRBuilderAsmJs::BuildReg1Long1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt64);
    srcOpnd->SetValueType(ValueType::GetInt(false));

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::ArgOut_Long:
        BuildArgOut(srcOpnd, dstReg, offset, TyVar);
        break;
    case Js::OpCodeAsmJs::I_ArgOut_Long:
        BuildArgOut(srcOpnd, dstReg, offset, TyInt64, ValueType::GetInt(false));
        break;
    default:
        Assume(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildLong1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, int64 constInt64)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Ld_LongConst);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, IR::Int64ConstOpnd::New(constInt64, TyInt64, m_func), m_func);

    if (dstOpnd->m_sym->IsSingleDef())
    {
        dstOpnd->m_sym->SetIsInt64Const();
    }

    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildLong2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt64);
    srcOpnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Ld_Long:
        instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Clz_Long:
        instr = IR::Instr::New(Js::OpCode::InlineMathClz, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Ctz_Long:
        instr = IR::Instr::New(Js::OpCode::Ctz, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::PopCnt_Long:
        instr = IR::Instr::New(Js::OpCode::PopCnt, dstOpnd, srcOpnd, m_func);
        break;

    case Js::OpCodeAsmJs::Return_Long:
        instr = IR::Instr::New(Js::OpCode::Ld_I4, dstOpnd, srcOpnd, m_func);
        CheckJitLoopReturn(dstRegSlot, TyInt64);
        break;
    case Js::OpCodeAsmJs::I64Extend8_s:
        instr = CreateSignExtendInstr(dstOpnd, srcOpnd, TyInt8);
        break;
    case Js::OpCodeAsmJs::I64Extend16_s:
        instr = CreateSignExtendInstr(dstOpnd, srcOpnd, TyInt16);
        break;
    case Js::OpCodeAsmJs::I64Extend32_s:
        instr = CreateSignExtendInstr(dstOpnd, srcOpnd, TyInt32);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildLong3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt64);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt64);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Add_Long:
        instr = IR::Instr::New(Js::OpCode::Add_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Sub_Long:
        instr = IR::Instr::New(Js::OpCode::Sub_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;

    case Js::OpCodeAsmJs::Mul_Long:
        instr = IR::Instr::New(Js::OpCode::Mul_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Div_Trap_ULong:
        src1Opnd->SetType(TyUint64);
        src2Opnd->SetType(TyUint64);
        // Fall Through for trap
    case Js::OpCodeAsmJs::Div_Trap_Long:
    {
#ifdef _WIN32
        if (CONFIG_FLAG(WasmMathExFilter))
        {
            // Do not emit traps, but make sure we don't remove the div
            dstOpnd->m_dontDeadStore = true;
        }
        else
#endif
        {
            src2Opnd = BuildTrapIfZero(src2Opnd, offset);
            if (newOpcode == Js::OpCodeAsmJs::Div_Trap_Long)
            {
                src1Opnd = BuildTrapIfMinIntOverNegOne(src1Opnd, src2Opnd, offset);
            }
        }
        Js::OpCode op = newOpcode == Js::OpCodeAsmJs::Div_Trap_ULong ? Js::OpCode::DivU_I4 : Js::OpCode::Div_I4;
        instr = IR::Instr::New(op, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::Rem_Trap_ULong:
        src1Opnd->SetType(TyUint64);
        src2Opnd->SetType(TyUint64);
        // Fall Through for trap
    case Js::OpCodeAsmJs::Rem_Trap_Long:
    {
#ifdef _WIN32
        if (CONFIG_FLAG(WasmMathExFilter))
        {
            // Do not emit traps, but make sure we don't remove the rem
            dstOpnd->m_dontDeadStore = true;
        }
        else
#endif
        {
            src2Opnd = BuildTrapIfZero(src2Opnd, offset);
        }
        Js::OpCode op = newOpcode == Js::OpCodeAsmJs::Rem_Trap_ULong ? Js::OpCode::RemU_I4 : Js::OpCode::Rem_I4;
        instr = IR::Instr::New(op, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    }
    case Js::OpCodeAsmJs::And_Long:
        instr = IR::Instr::New(Js::OpCode::And_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Or_Long:
        instr = IR::Instr::New(Js::OpCode::Or_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Xor_Long:
        instr = IR::Instr::New(Js::OpCode::Xor_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Shl_Long:
        instr = IR::Instr::New(Js::OpCode::Shl_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Shr_Long:
        instr = IR::Instr::New(Js::OpCode::Shr_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Shr_ULong:
        instr = IR::Instr::New(Js::OpCode::ShrU_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Rol_Long:
        instr = IR::Instr::New(Js::OpCode::Rol_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::Ror_Long:
        instr = IR::Instr::New(Js::OpCode::Ror_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Long2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt64);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt64);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::CmLt_Long:
        instr = IR::Instr::New(Js::OpCode::CmLt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLe_Long:
        instr = IR::Instr::New(Js::OpCode::CmLe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGt_Long:
        instr = IR::Instr::New(Js::OpCode::CmGt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGe_Long:
        instr = IR::Instr::New(Js::OpCode::CmGe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmEq_Long:
        instr = IR::Instr::New(Js::OpCode::CmEq_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmNe_Long:
        instr = IR::Instr::New(Js::OpCode::CmNeq_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLt_ULong:
        instr = IR::Instr::New(Js::OpCode::CmUnLt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmLe_ULong:
        instr = IR::Instr::New(Js::OpCode::CmUnLe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGt_ULong:
        instr = IR::Instr::New(Js::OpCode::CmUnGt_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    case Js::OpCodeAsmJs::CmGe_ULong:
        instr = IR::Instr::New(Js::OpCode::CmUnGe_I4, dstOpnd, src1Opnd, src2Opnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildLong1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_ITL:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt32);
        break;

    case Js::OpCodeAsmJs::Conv_UTL:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyUint32);
        break;

    default:
        Assume(UNREACHED);
    }
    srcOpnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
    dstOpnd->SetValueType(ValueType::Float);

    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Long1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt64);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    IR::Instr * instr = nullptr;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Eqz_Long:
        instr = IR::Instr::New(Js::OpCode::CmEq_I4, dstOpnd, src1Opnd, IR::Int64ConstOpnd::New(0, TyInt64, m_func), m_func);
        break;
    case Js::OpCodeAsmJs::Conv_LTI:
        instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, src1Opnd, m_func);
        break;
    default:
        Assume(UNREACHED);
    }
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildLong1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TyFloat32);
    IR::RegOpnd * dstOpnd = nullptr;
    Js::OpCode op = Js::OpCode::Nop;
    bool trapping = false;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_Check_FTL:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
        op = Js::OpCode::Conv_Prim;
        trapping = true;
        break;
    case Js::OpCodeAsmJs::Conv_Check_FTUL:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyUint64);
        op = Js::OpCode::Conv_Prim;
        trapping = true;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_FTL:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
        op = Js::OpCode::Conv_Prim_Sat;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_FTUL:
        dstOpnd = BuildDstOpnd(dstRegSlot, TyUint64);
        op = Js::OpCode::Conv_Prim_Sat;
        break;
    default:
        Assume(UNREACHED);
    }

    if (trapping)
    {
        IR::RegOpnd* tmpDst = IR::RegOpnd::New(srcOpnd->GetType(), m_func);
        tmpDst->SetValueType(ValueType::Float);
        AddInstr(IR::Instr::New(Js::OpCode::TrapIfTruncOverflow, tmpDst, srcOpnd, m_func), offset);
        dstOpnd->m_dontDeadStore = true;
        srcOpnd = tmpDst;
    }
    IR::Instr * instr = IR::Instr::New(op, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat1Long1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot)
{
    IR::RegOpnd * srcOpnd = nullptr;
    IR::RegOpnd * dstOpnd  = BuildDstOpnd(dstRegSlot, TyFloat32);
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_LTF:
        srcOpnd = BuildSrcOpnd(src1RegSlot, TyInt64);
        break;
    case Js::OpCodeAsmJs::Conv_ULTF:
        srcOpnd = BuildSrcOpnd(src1RegSlot, TyUint64);
        break;
    default:
        Assume(UNREACHED);
    }
    IR::Instr * instr = IR::Instr::New(Js::OpCode::Conv_Prim, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildLong1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TyFloat64);
    srcOpnd->SetValueType(ValueType::Float);

    IRType dstType;
    Js::OpCode op;
    bool doTruncTrapCheck = false;
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_Check_DTL:
        op = Js::OpCode::Conv_Prim;
        dstType = TyInt64;
        doTruncTrapCheck = true;
        break;
    case Js::OpCodeAsmJs::Conv_Check_DTUL:
        op = Js::OpCode::Conv_Prim;
        dstType = TyUint64;
        doTruncTrapCheck = true;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_DTL:
        op = Js::OpCode::Conv_Prim_Sat;
        dstType = TyInt64;
        break;
    case Js::OpCodeAsmJs::Conv_Sat_DTUL:
        op = Js::OpCode::Conv_Prim_Sat;
        dstType = TyUint64;
        break;
    case Js::OpCodeAsmJs::Reinterpret_DTL:
        op = Js::OpCode::Reinterpret_Prim;
        dstType = TyInt64;
        break;
    default:
        Assume(UNREACHED);
        Js::Throw::FatalInternalError();
    }

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, dstType);
    dstOpnd->SetValueType(ValueType::GetInt(false));
    if (doTruncTrapCheck)
    {
        IR::RegOpnd* tmpDst = IR::RegOpnd::New(srcOpnd->GetType(), m_func);
        tmpDst->SetValueType(ValueType::Float);
        AddInstr(IR::Instr::New(Js::OpCode::TrapIfTruncOverflow, tmpDst, srcOpnd, m_func), offset);
        dstOpnd->m_dontDeadStore = true;
        srcOpnd = tmpDst;
    }
    IR::Instr * instr = IR::Instr::New(op, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildDouble1Long1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot)
{
    IR::RegOpnd * srcOpnd = nullptr;
    Js::OpCode op = Js::OpCode::Conv_Prim;

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    dstOpnd->SetValueType(ValueType::Float);
    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Conv_LTD:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt64);
        break;
    case Js::OpCodeAsmJs::Conv_ULTD:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyUint64);
        break;
    case Js::OpCodeAsmJs::Reinterpret_LTD:
        srcOpnd = BuildSrcOpnd(srcRegSlot, TyInt64);
        op = Js::OpCode::Reinterpret_Prim;
        break;
    default:
        Assume(UNREACHED);
    }
    srcOpnd->SetValueType(ValueType::GetInt(false));
    IR::Instr* instr = IR::Instr::New(op, dstOpnd, srcOpnd, m_func);
    AddInstr(instr, offset);
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
    return (dst && dst->IsRegOpnd() && dst->AsRegOpnd()->m_sym == GetJitLoopBodyData().GetLoopBodyRetIPSym());
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

IR::Opnd *
IRBuilderAsmJs::InsertLoopBodyReturnIPInstr(uint targetOffset, uint offset)
{
    IR::RegOpnd * retOpnd = IR::RegOpnd::New(GetJitLoopBodyData().GetLoopBodyRetIPSym(), TyInt32, m_func);
    IR::IntConstOpnd * exitOffsetOpnd = IR::IntConstOpnd::New(targetOffset, TyInt32, m_func);
    IR::Instr * setRetValueInstr = IR::Instr::New(Js::OpCode::Ld_I4, retOpnd, exitOffsetOpnd, m_func);
    this->AddInstr(setRetValueInstr, offset);
    return setRetValueInstr->GetDst();
}

IR::SymOpnd *
IRBuilderAsmJs::BuildAsmJsLoopBodySlotOpnd(Js::RegSlot regSlot, IRType opndType)
{
    Assert(IsLoopBody());
    // There is no unsigned locals, make sure we create only signed locals
    opndType = IRType_EnsureSigned(opndType);
    // Get the interpreter frame instance that was passed in.
    StackSym *loopParamSym = m_func->EnsureLoopParamSym();

    // property ID is the offset
    Js::PropertyId propOffSet = CalculatePropertyOffset(regSlot, opndType);

    // Get the bytecodeRegSlot and Get the offset from m_localSlots
    PropertySym * fieldSym = PropertySym::FindOrCreate(loopParamSym->m_id, propOffSet, (uint32)-1, (uint)-1, PropertyKindLocalSlots, m_func);
    return IR::SymOpnd::New(fieldSym, opndType, m_func);
}

void
IRBuilderAsmJs::EnsureLoopBodyAsmJsLoadSlot(Js::RegSlot regSlot, IRType type)
{
    if (GetJitLoopBodyData().GetLdSlots()->TestAndSet(regSlot))
    {
        return;
    }

    IR::SymOpnd * fieldSymOpnd = this->BuildAsmJsLoopBodySlotOpnd(regSlot, type);

    StackSym * symDst = StackSym::FindOrCreate(static_cast<SymID>(regSlot), regSlot, m_func, fieldSymOpnd->GetType());
    IR::RegOpnd * dstOpnd = IR::RegOpnd::New(symDst, symDst->GetType(), m_func);
    IR::Instr * ldSlotInstr = IR::Instr::New(Js::OpCode::LdSlot, dstOpnd, fieldSymOpnd, m_func);

    m_func->m_headInstr->InsertAfter(ldSlotInstr);
    if (m_lastInstr == m_func->m_headInstr)
    {
        m_lastInstr = ldSlotInstr;
    }
}

void
IRBuilderAsmJs::EnsureLoopBodyAsmJsStoreSlot(Js::RegSlot regSlot, IRType type)
{
    Assert(!RegIsTemp(regSlot) || RegIsJitLoopYield(regSlot));
    GetJitLoopBodyData().GetStSlots()->Set(regSlot);
    EnsureLoopBodyAsmJsLoadSlot(regSlot, type);
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
    IR::SymOpnd *srcOpnd = IR::SymOpnd::New(symSrc, TyMachReg, m_func);

    StackSym *loopParamSym = m_func->EnsureLoopParamSym();
    IR::RegOpnd *loopParamOpnd = IR::RegOpnd::New(loopParamSym, TyMachReg, m_func);

    IR::Instr *instrArgIn = IR::Instr::New(Js::OpCode::ArgIn_A, loopParamOpnd, srcOpnd, m_func);
    m_func->m_headInstr->InsertAfter(instrArgIn);

    if (GetJitLoopBodyData().GetLdSlots()->Count() == 0)
    {
        return;
    }
    SymID loopParamSymId = loopParamSym->m_id;

    FOREACH_BITSET_IN_FIXEDBV(index, GetJitLoopBodyData().GetLdSlots())
    {
        Js::RegSlot regSlot = (Js::RegSlot)index;
        IRType type = IRType::TyInt32;
        ValueType valueType = ValueType::GetInt(false);
        if (RegIs(regSlot, WAsmJs::INT32))
        {
            type = IRType::TyInt32;
            valueType = ValueType::GetInt(false);
        }
        else if (RegIs(regSlot, WAsmJs::FLOAT32))
        {
            type = IRType::TyFloat32;
            valueType = ValueType::Float;
        }
        else if (RegIs(regSlot, WAsmJs::FLOAT64))
        {
            type = IRType::TyFloat64;
            valueType = ValueType::Float;
        }
        else if (RegIs(regSlot, WAsmJs::INT64))
        {
            type = IRType::TyInt64;
            valueType = ValueType::GetInt(false);
        }
        else if (RegIs(regSlot, WAsmJs::SIMD))
        {
            type = IRType::TySimd128F4;
            // SIMD regs are non-typed. There is no way to know the incoming SIMD type to a StSlot after a loop body, so we pick any type.
            // However, at this point all src syms are already defined and assigned a type.
            valueType = ValueType::GetObject(ObjectType::UninitializedObject);
        }
        else
        {
            AnalysisAssert(UNREACHED);
            continue;
        }

        Js::PropertyId propOffSet = CalculatePropertyOffset(regSlot, type);
        IR::RegOpnd* regOpnd = this->BuildSrcOpnd(regSlot, type);
        regOpnd->SetValueType(valueType);

        // Get the bytecodeRegSlot and Get the offset from m_localSlots
        PropertySym * fieldSym = PropertySym::FindOrCreate(loopParamSymId, propOffSet, (uint32)-1, (uint)-1, PropertyKindLocalSlots, m_func);

        IR::SymOpnd * fieldSymOpnd = IR::SymOpnd::New(fieldSym, regOpnd->GetType(), m_func);
        Js::OpCode opcode = Js::OpCode::StSlot;
        IR::Instr * stSlotInstr = IR::Instr::New(opcode, fieldSymOpnd, regOpnd, m_func);
        this->AddInstr(stSlotInstr, offset);
    }
    NEXT_BITSET_IN_FIXEDBV;
}

Js::PropertyId IRBuilderAsmJs::CalculatePropertyOffset(Js::RegSlot regSlot, IRType type)
{
    // Compute the offset to the start of the interpreter frame's locals array as a Var index.
    size_t localsOffset = Js::InterpreterStackFrame::GetOffsetOfLocals();
    Assert(localsOffset % sizeof(AsmJsSIMDValue) == 0);
    WAsmJs::Types asmType = WAsmJs::FromIRType(type);
    const auto typedInfo = m_asmFuncInfo->GetTypedSlotInfo(asmType);
    uint32 bytecodeRegSlot = GetTypedRegFromRegSlot(regSlot, asmType);
    return (Js::PropertyId)(bytecodeRegSlot * TySize[type] + typedInfo.byteOffset + localsOffset);
}

Js::OpCode IRBuilderAsmJs::GetSimdOpcode(Js::OpCodeAsmJs asmjsOpcode)
{
    Assert(m_func->GetJITFunctionBody()->IsWasmFunction());
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

#define SIMD_TYPE_CHECK(type1, type2) \
case Js::AsmJsType::Which::##type1: \
        irType = type2; \
        vType = ValueType::Simd; \
        break;

    switch (asmType)
    {
        SIMD_TYPE_CHECK(Float32x4,  TySimd128F4)
        SIMD_TYPE_CHECK(Int32x4,    TySimd128I4)
        SIMD_TYPE_CHECK(Int16x8,    TySimd128I8)
        SIMD_TYPE_CHECK(Int8x16,    TySimd128I16)
        SIMD_TYPE_CHECK(Uint32x4,   TySimd128U4)
        SIMD_TYPE_CHECK(Uint16x8,   TySimd128U8)
        SIMD_TYPE_CHECK(Uint8x16,   TySimd128U16)
        SIMD_TYPE_CHECK(Bool32x4,   TySimd128B4)
        SIMD_TYPE_CHECK(Bool16x8,   TySimd128B8)
        SIMD_TYPE_CHECK(Bool8x16,   TySimd128B16)
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
#define BUILD_SIMD_ARGS_REG2 Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot
#define BUILD_SIMD_ARGS_REG3 BUILD_SIMD_ARGS_REG2, Js::RegSlot src2RegSlot
#define BUILD_SIMD_ARGS_REG4 BUILD_SIMD_ARGS_REG3, Js::RegSlot src3RegSlot
#define BUILD_SIMD_ARGS_REG5 BUILD_SIMD_ARGS_REG4, Js::RegSlot src4RegSlot
#define BUILD_SIMD_ARGS_REG6 BUILD_SIMD_ARGS_REG5, Js::RegSlot src5RegSlot
#define BUILD_SIMD_ARGS_REG7 BUILD_SIMD_ARGS_REG6, Js::RegSlot src6RegSlot
#define BUILD_SIMD_ARGS_REG9 BUILD_SIMD_ARGS_REG7, Js::RegSlot src7RegSlot, Js::RegSlot src8RegSlot
#define BUILD_SIMD_ARGS_REG10 BUILD_SIMD_ARGS_REG9, Js::RegSlot src9RegSlot
#define BUILD_SIMD_ARGS_REG11 BUILD_SIMD_ARGS_REG10, Js::RegSlot src10RegSlot
#define BUILD_SIMD_ARGS_REG17 BUILD_SIMD_ARGS_REG11, Js::RegSlot src11RegSlot, Js::RegSlot src12RegSlot, Js::RegSlot src13RegSlot, Js::RegSlot src14RegSlot, Js::RegSlot src15RegSlot, Js::RegSlot src16RegSlot
#define BUILD_SIMD_ARGS_REG18 BUILD_SIMD_ARGS_REG17, Js::RegSlot src17RegSlot
#define BUILD_SIMD_ARGS_REG19 BUILD_SIMD_ARGS_REG18, Js::RegSlot src18RegSlot

// Float32x4
void
IRBuilderAsmJs::BuildFloat32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4);
}

void
IRBuilderAsmJs::BuildFloat32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128F4);
}

void
IRBuilderAsmJs::BuildBool32x4_1Float32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4, TySimd128F4);
}

void
IRBuilderAsmJs::BuildFloat32x4_1Bool32x4_1Float32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128F4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128F4);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildFloat32x4_4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128F4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128F4);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);

    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void IRBuilderAsmJs::BuildFloat32x4_1Float4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG5)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat32);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat32);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TyFloat32);
    IR::RegOpnd * src4Opnd = BuildSrcOpnd(src4RegSlot, TyFloat32);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);

    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
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

void
IRBuilderAsmJs::BuildFloat32x4_2Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG6)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(src2RegSlot);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_F4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void
IRBuilderAsmJs::BuildFloat32x4_3Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG7)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128F4);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src6Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_F4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void
IRBuilderAsmJs::BuildFloat32x4_1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat32);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_F4);
    Js::OpCode opcode = Js::OpCode::Simd128_Splat_F4;

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat32x4_2Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat32);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

// Disable for now
#if 0
void
IRBuilderAsmJs::BuildFloat32x4_1Float64x2_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_FromFloat64x2_F4 || opcode == Js::OpCode::Simd128_FromFloat64x2Bits_F4, "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}
#endif // 0

void
IRBuilderAsmJs::BuildFloat32x4_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4_F4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128I4);
}

void
IRBuilderAsmJs::BuildFloat32x4_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4_F4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128U4);
}

void
IRBuilderAsmJs::BuildFloat32x4_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128I8);
}

void
IRBuilderAsmJs::BuildFloat32x4_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128U8);
}

void
IRBuilderAsmJs::BuildFloat32x4_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128I16);
}

void
IRBuilderAsmJs::BuildFloat32x4_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_F4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128F4, TySimd128U16);
}

void IRBuilderAsmJs::BuildReg1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

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
void
IRBuilderAsmJs::BuildInt32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4);
}

void
IRBuilderAsmJs::BuildInt32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I4);
}

void
IRBuilderAsmJs::BuildInt32x4_4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    ValueType valueType = GetSimdValueTypeFromIRType(TySimd128I4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    src1Opnd->SetValueType(valueType);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I4);
    src2Opnd->SetValueType(valueType);

    IR::RegOpnd * mask = BuildSrcOpnd(src3RegSlot, TySimd128I4);
    src2Opnd->SetValueType(valueType);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
    dstOpnd->SetValueType(GetSimdValueTypeFromIRType(TySimd128I4));

    Js::OpCode opcode;

    opcode = GetSimdOpcode(newOpcode);

    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr* instr = nullptr;

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(mask, instr->GetDst()->AsRegOpnd(), offset);

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void
IRBuilderAsmJs::BuildBool32x4_1Int32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4, TySimd128I4);
}

void
IRBuilderAsmJs::BuildInt32x4_1Bool32x4_1Int32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128I4);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void IRBuilderAsmJs::BuildInt32x4_1Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG5)
{
    uint const LANES = 4;
    Js::RegSlot srcRegSlot[LANES];
    srcRegSlot[0] = src1RegSlot;
    srcRegSlot[1] = src2RegSlot;
    srcRegSlot[2] = src3RegSlot;
    srcRegSlot[3] = src4RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128I4, srcRegSlot, dstRegSlot, LANES);
}

void IRBuilderAsmJs::BuildInt32x4_2Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG6)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(src2RegSlot);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_I4);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_I4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void IRBuilderAsmJs::BuildInt32x4_3Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG7)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I4);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildInt32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_I4);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4);
}

void
IRBuilderAsmJs::BuildInt32x4_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I4);
}

//ReplaceLane
void
IRBuilderAsmJs::BuildInt32x4_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_I4), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128I4);
}

void
IRBuilderAsmJs::BuildInt1Int32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_I4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat32x4_2Int1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TyFloat32);
    src3Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildFloat1Float32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat32);
    dstOpnd->SetValueType(ValueType::Float);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_F4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt32x4_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4_I4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128F4);
}

void
IRBuilderAsmJs::BuildInt32x4_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128U4);
}

void
IRBuilderAsmJs::BuildInt32x4_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt32x4_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128U8);
}

void
IRBuilderAsmJs::BuildInt32x4_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128I16);
}

void
IRBuilderAsmJs::BuildInt32x4_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128U16);
}

#if 0
void
IRBuilderAsmJs::BuildInt32x4_1Float64x2_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat64x2_I4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat64x2Bits_I4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I4, TySimd128D2);
}
#endif //0
void IRBuilderAsmJs::BuildReg1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

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
void
IRBuilderAsmJs::BuildInt8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16);
}
//
void
IRBuilderAsmJs::BuildInt8x16_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I16);
}

void IRBuilderAsmJs::BuildInt8x16_1Int16(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG17)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToI16, "Unexpected opcode for this format.");
    uint const LANES = 16;
    Js::RegSlot srcRegSlots[LANES] = {
        srcRegSlots[0]  = src1RegSlot,
        srcRegSlots[1]  = src2RegSlot,
        srcRegSlots[2]  = src3RegSlot,
        srcRegSlots[3]  = src4RegSlot,
        srcRegSlots[4]  = src5RegSlot,
        srcRegSlots[5]  = src6RegSlot,
        srcRegSlots[6]  = src7RegSlot,
        srcRegSlots[7]  = src8RegSlot,
        srcRegSlots[8]  = src9RegSlot,
        srcRegSlots[9]  = src10RegSlot,
        srcRegSlots[10] = src11RegSlot,
        srcRegSlots[11] = src12RegSlot,
        srcRegSlots[12] = src13RegSlot,
        srcRegSlots[13] = src14RegSlot,
        srcRegSlots[14] = src15RegSlot,
        srcRegSlots[15] = src16RegSlot
    };

    BuildSimd_1Ints(newOpcode, offset, TySimd128I16, srcRegSlots, dstRegSlot, LANES);
}

void
IRBuilderAsmJs::BuildBool8x16_1Int8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt8x16_1Bool8x16_1Int8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I16);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128I16);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I16);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildInt8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_I16);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16);
}
//ExtractLane ReplaceLane
void
IRBuilderAsmJs::BuildInt8x16_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I16);
}

void
IRBuilderAsmJs::BuildInt8x16_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_I16), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128I16);
}

void
IRBuilderAsmJs::BuildInt1Int8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I16);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_I16), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildInt8x16_3Int16(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG19)
{
    IR::RegOpnd * dstOpnd   = BuildDstOpnd(dstRegSlot, TySimd128I16);
    IR::RegOpnd * src1Opnd  = BuildSrcOpnd(src1RegSlot, TySimd128I16);
    IR::RegOpnd * src2Opnd  = BuildSrcOpnd(src2RegSlot, TySimd128I16);

    IR::RegOpnd* srcOpnds[16];

    srcOpnds[0] = BuildIntConstOpnd(src3RegSlot);
    srcOpnds[1] = BuildIntConstOpnd(src4RegSlot);
    srcOpnds[2] = BuildIntConstOpnd(src5RegSlot);
    srcOpnds[3] = BuildIntConstOpnd(src6RegSlot);
    srcOpnds[4] = BuildIntConstOpnd(src7RegSlot);
    srcOpnds[5] = BuildIntConstOpnd(src8RegSlot);
    srcOpnds[6] = BuildIntConstOpnd(src9RegSlot);
    srcOpnds[7] = BuildIntConstOpnd(src10RegSlot);
    srcOpnds[8] = BuildIntConstOpnd(src11RegSlot);
    srcOpnds[9] = BuildIntConstOpnd(src12RegSlot);
    srcOpnds[10] = BuildIntConstOpnd(src13RegSlot);
    srcOpnds[11] = BuildIntConstOpnd(src14RegSlot);
    srcOpnds[12] = BuildIntConstOpnd(src15RegSlot);
    srcOpnds[13] = BuildIntConstOpnd(src16RegSlot);
    srcOpnds[14] = BuildIntConstOpnd(src17RegSlot);
    srcOpnds[15] = BuildIntConstOpnd(src18RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

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

void IRBuilderAsmJs::BuildInt8x16_2Int16(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG18)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I16);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I16);

    IR::RegOpnd* srcOpnds[16];

    srcOpnds[0]  = BuildIntConstOpnd(src2RegSlot);
    srcOpnds[1]  = BuildIntConstOpnd(src3RegSlot);
    srcOpnds[2]  = BuildIntConstOpnd(src4RegSlot);
    srcOpnds[3]  = BuildIntConstOpnd(src5RegSlot);
    srcOpnds[4]  = BuildIntConstOpnd(src6RegSlot);
    srcOpnds[5]  = BuildIntConstOpnd(src7RegSlot);
    srcOpnds[6]  = BuildIntConstOpnd(src8RegSlot);
    srcOpnds[7]  = BuildIntConstOpnd(src9RegSlot);
    srcOpnds[8]  = BuildIntConstOpnd(src10RegSlot);
    srcOpnds[9]  = BuildIntConstOpnd(src11RegSlot);
    srcOpnds[10] = BuildIntConstOpnd(src12RegSlot);
    srcOpnds[11] = BuildIntConstOpnd(src13RegSlot);
    srcOpnds[12] = BuildIntConstOpnd(src14RegSlot);
    srcOpnds[13] = BuildIntConstOpnd(src15RegSlot);
    srcOpnds[14] = BuildIntConstOpnd(src16RegSlot);
    srcOpnds[15] = BuildIntConstOpnd(src17RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);

    for (int i = 0; i < 16; ++i)
    {
        instr = AddExtendedArg(srcOpnds[i], instr->GetDst()->AsRegOpnd(), offset);
    }

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_I16);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_I16;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void
IRBuilderAsmJs::BuildInt8x16_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128F4);
}

void
IRBuilderAsmJs::BuildInt8x16_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128I4);
}

void
IRBuilderAsmJs::BuildInt8x16_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt8x16_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128U4);
}

void
IRBuilderAsmJs::BuildInt8x16_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128U8);
}

void
IRBuilderAsmJs::BuildInt8x16_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_I16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I16, TySimd128U16);
}

void IRBuilderAsmJs::BuildReg1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128I16);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

/* Int64x2 */
void
IRBuilderAsmJs::BuildInt64x2_1Long1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_I2);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyInt64);
    src1Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I2);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Bool64x2_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_AnyTrue_B2 || newOpcode == Js::OpCodeAsmJs::Simd128_AllTrue_B2);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I2);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildLong1Int64x2_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_ExtractLane_I2);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I2);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt64);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt64x2_2_Int1_Long1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_I2);
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128I2, TyInt64);
}

void
IRBuilderAsmJs::BuildInt64x2_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_ShLtByScalar_I2 ||
        newOpcode == Js::OpCodeAsmJs::Simd128_ShRtByScalar_I2 ||
        newOpcode == Js::OpCodeAsmJs::Simd128_ShRtByScalar_U2
    );
    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I2);
}

void
IRBuilderAsmJs::BuildInt64x2_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Add_I2 || newOpcode == Js::OpCodeAsmJs::Simd128_Sub_I2);
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128F4);
}

void
IRBuilderAsmJs::BuildInt64x2_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Neg_I2 ||
           newOpcode == Js::OpCodeAsmJs::Simd128_FromUint64x2_D2 ||
           newOpcode == Js::OpCodeAsmJs::Simd128_FromInt64x2_D2 ||
           newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat64x2_I2 ||
           newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat64x2_U2);
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I2);
}

/* Float64x2 */
void
IRBuilderAsmJs::BuildDouble1Float64x2_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyFloat64);
    dstOpnd->SetValueType(ValueType::Float);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_D2), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildFloat64x2_1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Splat_D2, "Invalid backend SIMD opcode");
    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode;

    opcode = GetSimdOpcode(newOpcode);

    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128F4);
    src2Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode;

    opcode = GetSimdOpcode(newOpcode);

    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_2Int1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TyFloat64);
    src3Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128F4);
    dstOpnd->SetValueType(ValueType::Simd);

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
    AssertMsg((opcode == Js::OpCode::Simd128_ReplaceLane_D2), "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}
// Disabled for now
#if 0
void
IRBuilderAsmJs::BuildFloat64x2_4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128D2);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128D2);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void IRBuilderAsmJs::BuildFloat64x2_1Double2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat64);
    src2Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_DoublesToD2, "Invalid backend SIMD opcode");
    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}



void
IRBuilderAsmJs::BuildFloat64x2_2Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyFloat64);
    src1Opnd->SetValueType(ValueType::Float);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);
    AssertMsg((uint32)opcode, "Invalid backend SIMD opcode");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(src2RegSlot);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_D2);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_D2;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_3Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG5)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128D2);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_D2);
    Js::OpCode opcode = Js::OpCode::Simd128_Shuffle_D2;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128F4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_FromFloat32x4_D2 || opcode == Js::OpCode::Simd128_FromFloat32x4Bits_D2, "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);
    dstOpnd->SetValueType(ValueType::Simd);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_FromInt32x4_D2 || opcode == Js::OpCode::Simd128_FromInt32x4Bits_D2, "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildFloat64x2_1Int32x4_1Float64x2_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128D2);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128D2);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128D2);

    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg(opcode == Js::OpCode::Simd128_Select_D2, "Unexpected opcode for this format.");
    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void IRBuilderAsmJs::BuildReg1Float64x2_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128D2);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

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
void IRBuilderAsmJs::BuildInt16x8_1Int8(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG9)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToI8, "Unexpected opcode for this format.");

    uint const LANES = 8;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = src1RegSlot;
    srcRegSlots[1] = src2RegSlot;
    srcRegSlots[2] = src3RegSlot;
    srcRegSlots[3] = src4RegSlot;
    srcRegSlots[4] = src5RegSlot;
    srcRegSlots[5] = src6RegSlot;
    srcRegSlots[6] = src7RegSlot;
    srcRegSlots[7] = src8RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128I8, srcRegSlots, dstRegSlot, LANES);
}

void IRBuilderAsmJs::BuildReg1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128I8);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildInt1Int16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I8);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_I8), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildInt16x8_2Int8(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG10)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I8);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(src2RegSlot);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(src7RegSlot);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(src8RegSlot);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(src9RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

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

void IRBuilderAsmJs::BuildInt16x8_3Int8(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG11)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128I8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I8);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(src7RegSlot);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(src8RegSlot);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(src9RegSlot);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(src10RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildInt16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_I8);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt16x8_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_I8), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt16x8_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I8);
}

void
IRBuilderAsmJs::BuildBool16x8_1Int16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt16x8_1Bool16x8_1Int16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128I8);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128I8);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128I8);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildInt16x8_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128I8);
}

void
IRBuilderAsmJs::BuildInt16x8_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128F4);
}

void
IRBuilderAsmJs::BuildInt16x8_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128I4);
}

void
IRBuilderAsmJs::BuildInt16x8_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128I16);
}
void
IRBuilderAsmJs::BuildInt16x8_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128U4);
}

void
IRBuilderAsmJs::BuildInt16x8_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128U8);
}

void
IRBuilderAsmJs::BuildInt16x8_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_I8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128I8, TySimd128U16);
}

/* Uint32x4 */
void IRBuilderAsmJs::BuildUint32x4_1Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG5)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToU4, "Unexpected opcode for this format.");

    uint const LANES = 4;
    Js::RegSlot srcRegSlot[LANES];
    srcRegSlot[0] = src1RegSlot;
    srcRegSlot[1] = src2RegSlot;
    srcRegSlot[2] = src3RegSlot;
    srcRegSlot[3] = src4RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128U4, srcRegSlot, dstRegSlot, LANES);
}

void IRBuilderAsmJs::BuildReg1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128U4);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildInt1Uint32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_U4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildUint32x4_2Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG6)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U4);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(src2RegSlot);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src3Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src4Opnd, instr->GetDst()->AsRegOpnd(), offset);
    instr = AddExtendedArg(src5Opnd, instr->GetDst()->AsRegOpnd(), offset);

    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Swizzle_U4);
    Js::OpCode opcode = Js::OpCode::Simd128_Swizzle_U4;

    AddInstr(IR::Instr::New(opcode, dstOpnd, instr->GetDst(), m_func), offset);
}

void IRBuilderAsmJs::BuildUint32x4_3Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG7)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U4);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U4);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildUint32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_U4);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4);
}

void
IRBuilderAsmJs::BuildUint32x4_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_U4), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128U4);
}

void
IRBuilderAsmJs::BuildUint32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4);
}

void
IRBuilderAsmJs::BuildUint32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U4);
}

void
IRBuilderAsmJs::BuildBool32x4_1Uint32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4, TySimd128U4);
}

void
IRBuilderAsmJs::BuildUint32x4_1Bool32x4_1Uint32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U4);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128U4);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U4);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildUint32x4_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U4);
}

void
IRBuilderAsmJs::BuildUint32x4_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4_U4 || newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128F4);
}

void
IRBuilderAsmJs::BuildUint32x4_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128I4);
}

void
IRBuilderAsmJs::BuildUint32x4_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{;
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128I8);
}

/* Enable with Int8x16 support*/
void
IRBuilderAsmJs::BuildUint32x4_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128I16);
}

void
IRBuilderAsmJs::BuildUint32x4_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128U8);
}

void
IRBuilderAsmJs::BuildUint32x4_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_U4, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U4, TySimd128U16);
}

/* Uint16x8 */
void IRBuilderAsmJs::BuildUint16x8_1Int8(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG9)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToU8, "Unexpected opcode for this format.");

    uint const LANES = 8;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = src1RegSlot;
    srcRegSlots[1] = src2RegSlot;
    srcRegSlots[2] = src3RegSlot;
    srcRegSlots[3] = src4RegSlot;
    srcRegSlots[4] = src5RegSlot;
    srcRegSlots[5] = src6RegSlot;
    srcRegSlots[6] = src7RegSlot;
    srcRegSlots[7] = src8RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128U8, srcRegSlots, dstRegSlot, LANES);
}

void IRBuilderAsmJs::BuildReg1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128U8);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildInt1Uint16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U8);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_U8), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildUint16x8_2Int8(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG10)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U8);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(src2RegSlot);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(src7RegSlot);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(src8RegSlot);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(src9RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

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

void IRBuilderAsmJs::BuildUint16x8_3Int8(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG11)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U8);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U8);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(src7RegSlot);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(src8RegSlot);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(src9RegSlot);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(src10RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildUint16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_U8);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8);
}

void
IRBuilderAsmJs::BuildUint16x8_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_U8), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128U8);
}

void
IRBuilderAsmJs::BuildUint16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8);
}

void
IRBuilderAsmJs::BuildUint16x8_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U8);
}

void
IRBuilderAsmJs::BuildBool16x8_1Uint16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8, TySimd128U8);
}

void
IRBuilderAsmJs::BuildUint16x8_1Bool16x8_1Uint16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U8);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128U8);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U8);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildUint16x8_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U8);
}

void
IRBuilderAsmJs::BuildUint16x8_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128F4);
}

void
IRBuilderAsmJs::BuildUint16x8_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128I4);
}

void
IRBuilderAsmJs::BuildUint16x8_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128I8);
}

void
IRBuilderAsmJs::BuildUint16x8_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128I16);
}

void
IRBuilderAsmJs::BuildUint16x8_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128U4);
}

void
IRBuilderAsmJs::BuildUint16x8_1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint8x16Bits_U8, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U8, TySimd128U16);
}
/* Uint8x16 */
void IRBuilderAsmJs::BuildUint8x16_1Int16(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG17)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToU16, "Unexpected opcode for this format.");

    uint const LANES = 16;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = src1RegSlot;
    srcRegSlots[1] = src2RegSlot;
    srcRegSlots[2] = src3RegSlot;
    srcRegSlots[3] = src4RegSlot;
    srcRegSlots[4] = src5RegSlot;
    srcRegSlots[5] = src6RegSlot;
    srcRegSlots[6] = src7RegSlot;
    srcRegSlots[7] = src8RegSlot;
    srcRegSlots[8] = src9RegSlot;
    srcRegSlots[9] = src10RegSlot;
    srcRegSlots[10] = src11RegSlot;
    srcRegSlots[11] = src12RegSlot;
    srcRegSlots[12] = src13RegSlot;
    srcRegSlots[13] = src14RegSlot;
    srcRegSlots[14] = src15RegSlot;
    srcRegSlots[15] = src16RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128U16, srcRegSlots, dstRegSlot, LANES);
}

void IRBuilderAsmJs::BuildReg1Uint8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128U16);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

        instr = IR::Instr::New(Js::OpCode::ArgOut_A, dstOpnd, srcOpnd, m_func);
        AddInstr(instr, offset);

        m_argStack->Push(instr);
    }
    else
    {
        Assert(UNREACHED);
    }
}

void
IRBuilderAsmJs::BuildUint8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16);
}

void
IRBuilderAsmJs::BuildInt1Uint8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U16);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_U16), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void IRBuilderAsmJs::BuildUint8x16_2Int16(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG18)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U16);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U16);

    IR::RegOpnd * src2Opnd = BuildIntConstOpnd(src2RegSlot);
    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(src7RegSlot);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(src8RegSlot);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(src9RegSlot);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(src10RegSlot);
    IR::RegOpnd * src11Opnd = BuildIntConstOpnd(src11RegSlot);
    IR::RegOpnd * src12Opnd = BuildIntConstOpnd(src12RegSlot);
    IR::RegOpnd * src13Opnd = BuildIntConstOpnd(src13RegSlot);
    IR::RegOpnd * src14Opnd = BuildIntConstOpnd(src14RegSlot);
    IR::RegOpnd * src15Opnd = BuildIntConstOpnd(src15RegSlot);
    IR::RegOpnd * src16Opnd = BuildIntConstOpnd(src16RegSlot);
    IR::RegOpnd * src17Opnd = BuildIntConstOpnd(src17RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);

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
void
IRBuilderAsmJs::BuildAsmShuffle(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode) && newOpcode == Js::OpCodeAsmJs::Simd128_Shuffle_V8X16);
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_AsmShuffle<SizePolicy>>();

    IR::RegOpnd * dstOpnd = BuildDstOpnd(GetRegSlotFromSimd128Reg(layout->R0), TySimd128U16);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(GetRegSlotFromSimd128Reg(layout->R1), TySimd128U16);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(GetRegSlotFromSimd128Reg(layout->R2), TySimd128U16);
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

    IR::Instr * instr = nullptr;
    instr = AddExtendedArg(src1Opnd, nullptr, offset);
    instr = AddExtendedArg(src2Opnd, instr->GetDst()->AsRegOpnd(), offset);

    for (uint i = 0; i < Wasm::Simd::MAX_LANES; i++)
    {
        IR::RegOpnd* shuffleOpnd = (IR::RegOpnd*)IR::IntConstOpnd::New(layout->INDICES[i], TyInt32, this->m_func);
        instr = AddExtendedArg(shuffleOpnd, instr->GetDst()->AsRegOpnd(), offset);
    }
    AddInstr(IR::Instr::New(Js::OpCode::Simd128_Shuffle_U16, dstOpnd, instr->GetDst(), m_func), offset);
}

void IRBuilderAsmJs::BuildUint8x16_3Int16(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG19)
{
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U16);
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128U16);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U16);

    IR::RegOpnd * src3Opnd = BuildIntConstOpnd(src3RegSlot);
    IR::RegOpnd * src4Opnd = BuildIntConstOpnd(src4RegSlot);
    IR::RegOpnd * src5Opnd = BuildIntConstOpnd(src5RegSlot);
    IR::RegOpnd * src6Opnd = BuildIntConstOpnd(src6RegSlot);
    IR::RegOpnd * src7Opnd = BuildIntConstOpnd(src7RegSlot);
    IR::RegOpnd * src8Opnd = BuildIntConstOpnd(src8RegSlot);
    IR::RegOpnd * src9Opnd = BuildIntConstOpnd(src9RegSlot);
    IR::RegOpnd * src10Opnd = BuildIntConstOpnd(src10RegSlot);
    IR::RegOpnd * src11Opnd = BuildIntConstOpnd(src11RegSlot);
    IR::RegOpnd * src12Opnd = BuildIntConstOpnd(src12RegSlot);
    IR::RegOpnd * src13Opnd = BuildIntConstOpnd(src13RegSlot);
    IR::RegOpnd * src14Opnd = BuildIntConstOpnd(src14RegSlot);
    IR::RegOpnd * src15Opnd = BuildIntConstOpnd(src15RegSlot);
    IR::RegOpnd * src16Opnd = BuildIntConstOpnd(src16RegSlot);
    IR::RegOpnd * src17Opnd = BuildIntConstOpnd(src17RegSlot);
    IR::RegOpnd * src18Opnd = BuildIntConstOpnd(src18RegSlot);

    IR::Instr * instr = nullptr;
    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildUint8x16_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_U16), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128U16);
}

void
IRBuilderAsmJs::BuildUint8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_U16);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16);
}

void
IRBuilderAsmJs::BuildUint8x16_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U16);
}

void
IRBuilderAsmJs::BuildBool8x16_1Uint8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B16, TySimd128U16);
}

void
IRBuilderAsmJs::BuildUint8x16_1Bool8x16_1Uint8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TySimd128U16);
    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, TySimd128U16);
    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TySimd128U16);
    IR::Instr * instr = nullptr;

    dstOpnd->SetValueType(ValueType::Simd);
    src1Opnd->SetValueType(ValueType::Simd);
    src2Opnd->SetValueType(ValueType::Simd);
    src3Opnd->SetValueType(ValueType::Simd);

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

void
IRBuilderAsmJs::BuildUint8x16_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_2Int1(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128U16);
}

void
IRBuilderAsmJs::BuildUint8x16_1Float32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromFloat32x4Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128F4);
}

void
IRBuilderAsmJs::BuildUint8x16_1Int32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt32x4Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128I4);
}

void
IRBuilderAsmJs::BuildUint8x16_1Int16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt16x8Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128I8);
}

/* Enable with Int8x16 support */
void
IRBuilderAsmJs::BuildUint8x16_1Int8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromInt8x16Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128I16);
}

void
IRBuilderAsmJs::BuildUint8x16_1Uint32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint32x4Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128U4);
}

void
IRBuilderAsmJs::BuildUint8x16_1Uint16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_FromUint16x8Bits_U16, "Unexpected opcode for this format.");
    BuildSimdConversion(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128U16, TySimd128U8);
}
//Bool32x4
void IRBuilderAsmJs::BuildBool32x4_1Int4(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG5)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToB4, "Unexpected opcode for this format.");

    uint const LANES = 4;
    Js::RegSlot srcRegSlot[LANES];
    srcRegSlot[0] = src1RegSlot;
    srcRegSlot[1] = src2RegSlot;
    srcRegSlot[2] = src3RegSlot;
    srcRegSlot[3] = src4RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128B4, srcRegSlot, dstRegSlot, LANES);
}

void
IRBuilderAsmJs::BuildInt1Bool32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_AllTrue_B4 || opcode == Js::OpCode::Simd128_AnyTrue_B4),
        "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildBool32x4_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B4);
}

void
IRBuilderAsmJs::BuildBool32x4_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B4);
}

void IRBuilderAsmJs::BuildReg1Bool32x4_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

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
void IRBuilderAsmJs::BuildBool16x8_1Int8(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG9)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToB8, "Unexpected opcode for this format.");

    uint const LANES = 8;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = src1RegSlot;
    srcRegSlots[1] = src2RegSlot;
    srcRegSlots[2] = src3RegSlot;
    srcRegSlots[3] = src4RegSlot;
    srcRegSlots[4] = src5RegSlot;
    srcRegSlots[5] = src6RegSlot;
    srcRegSlots[6] = src7RegSlot;
    srcRegSlots[7] = src8RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128B8, srcRegSlots, dstRegSlot, LANES);
}

void
IRBuilderAsmJs::BuildInt1Bool16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_AllTrue_B8 || opcode == Js::OpCode::Simd128_AnyTrue_B8),
        "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildBool16x8_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B8);
}

void
IRBuilderAsmJs::BuildBool16x8_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B8);
}

void
IRBuilderAsmJs::BuildReg1Bool16x8_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

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
void IRBuilderAsmJs::BuildBool8x16_1Int16(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG17)
{
    AssertMsg(newOpcode == Js::OpCodeAsmJs::Simd128_IntsToB16, "Unexpected opcode for this format.");

    uint const LANES = 16;
    Js::RegSlot srcRegSlots[LANES];

    srcRegSlots[0] = src1RegSlot;
    srcRegSlots[1] = src2RegSlot;
    srcRegSlots[2] = src3RegSlot;
    srcRegSlots[3] = src4RegSlot;
    srcRegSlots[4] = src5RegSlot;
    srcRegSlots[5] = src6RegSlot;
    srcRegSlots[6] = src7RegSlot;
    srcRegSlots[7] = src8RegSlot;
    srcRegSlots[8] = src9RegSlot;
    srcRegSlots[9] = src10RegSlot;
    srcRegSlots[10] = src11RegSlot;
    srcRegSlots[11] = src12RegSlot;
    srcRegSlots[12] = src13RegSlot;
    srcRegSlots[13] = src14RegSlot;
    srcRegSlots[14] = src15RegSlot;
    srcRegSlots[15] = src16RegSlot;

    BuildSimd_1Ints(newOpcode, offset, TySimd128B16, srcRegSlots, dstRegSlot, LANES);
}

void
IRBuilderAsmJs::BuildInt1Bool8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_AllTrue_B16 || opcode == Js::OpCode::Simd128_AnyTrue_B16),
        "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildBool8x16_2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    BuildSimd_2(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B16);
}

void
IRBuilderAsmJs::BuildBool8x16_3(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    BuildSimd_3(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, TySimd128B16);
}

void IRBuilderAsmJs::BuildReg1Bool8x16_1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    IR::RegOpnd * srcOpnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    srcOpnd->SetValueType(ValueType::Simd);

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
        dstOpnd->SetValueType(ValueType::Simd);

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
void
IRBuilderAsmJs::BuildBool32x4_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_B4), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128B4);
}

void
IRBuilderAsmJs::BuildBool16x8_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_B8), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128B8);
}

void
IRBuilderAsmJs::BuildBool8x16_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG4)
{
    AssertMsg((newOpcode == Js::OpCodeAsmJs::Simd128_ReplaceLane_B16), "Unexpected opcode for this format.");
    BuildSimd_2Int2(newOpcode, offset, dstRegSlot, src1RegSlot, src2RegSlot, src3RegSlot, TySimd128B16);
}

void
IRBuilderAsmJs::BuildInt1Bool32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B4);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_B4), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Bool16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B8);
    src1Opnd->SetValueType(ValueType::Simd);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * dstOpnd = BuildDstOpnd(dstRegSlot, TyInt32);
    dstOpnd->SetValueType(ValueType::GetInt(false));

    Js::OpCode opcode = GetSimdOpcode(newOpcode);

    AssertMsg((opcode == Js::OpCode::Simd128_ExtractLane_B8), "Unexpected opcode for this format.");

    IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, src2Opnd, m_func);
    AddInstr(instr, offset);
}

void
IRBuilderAsmJs::BuildInt1Bool8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG3)
{
    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, TySimd128B16);
    src1Opnd->SetValueType(ValueType::Simd);

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

void IRBuilderAsmJs::BuildSimd_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, Js::RegSlot src3RegSlot, IRType simdType, IRType valType)
{
    ValueType valueType = GetSimdValueTypeFromIRType(simdType);

    IR::RegOpnd * src1Opnd = BuildSrcOpnd(src1RegSlot, simdType);
    src1Opnd->SetValueType(valueType);

    IR::RegOpnd * src2Opnd = BuildSrcOpnd(src2RegSlot, TyInt32);
    src2Opnd->SetValueType(ValueType::GetInt(false));

    IR::RegOpnd * src3Opnd = BuildSrcOpnd(src3RegSlot, valType);
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
        CheckJitLoopReturn(dstRegSlot, simdType);
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

    if (newOpcode == Js::OpCodeAsmJs::Simd128_Neg_I2)
    {
        SIMDValue zeroVec{ 0 };
        IR::Opnd* zeroConst = IR::Simd128ConstOpnd::New(zeroVec, TySimd128F4, m_func);
        IR::RegOpnd* tmpReg = IR::RegOpnd::New(TyMachSimd128F4, m_func);
        tmpReg->SetValueType(ValueType::Simd);
        IR::Instr * instr = IR::Instr::New(Js::OpCode::Simd128_LdC, tmpReg, zeroConst, m_func);
        AddInstr(instr, offset);
        instr = IR::Instr::New(Js::OpCode::Simd128_Sub_I2, dstOpnd, tmpReg, src1Opnd, m_func);
        AddInstr(instr, offset);

    }
    else
    {
        IR::Instr * instr = IR::Instr::New(opcode, dstOpnd, src1Opnd, m_func);
        AddInstr(instr, offset);
    }

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
void
IRBuilderAsmJs::BuildBool32x4_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_B4);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B4);
}

// bool16x8
void
IRBuilderAsmJs::BuildBool16x8_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
    Assert(newOpcode == Js::OpCodeAsmJs::Simd128_Splat_B8);
    BuildSimd_1Int1(newOpcode, offset, dstRegSlot, src1RegSlot, TySimd128B8);
}

// bool8x16
void
IRBuilderAsmJs::BuildBool8x16_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, BUILD_SIMD_ARGS_REG2)
{
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
        return ValueType::Simd;
    case TySimd128D2:
        return ValueType::Simd;
    case TySimd128I2:
        return ValueType::Simd;
    case TySimd128I4:
        return ValueType::Simd;
    case TySimd128I8:
        return ValueType::Simd;
    case TySimd128I16:
        return ValueType::Simd;
    case TySimd128U4:
        return ValueType::Simd;
    case TySimd128U8:
        return ValueType::Simd;
    case TySimd128U16:
        return ValueType::Simd;
    case TySimd128B4:
        return ValueType::Simd;
    case TySimd128B8:
        return ValueType::Simd;
    case TySimd128B16:
        return ValueType::Simd;
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

template<typename SizePolicy>
void IRBuilderAsmJs::BuildAsmSimdTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset)
{
    Assert(OpCodeAttrAsmJs::HasMultiSizeLayout(newOpcode));
    auto layout = m_jnReader.GetLayout<Js::OpLayoutT_AsmSimdTypedArr<SizePolicy>>();
    BuildAsmSimdTypedArr(newOpcode, offset, layout->SlotIndex, layout->Value, layout->ViewType, layout->DataWidth, layout->Offset);
}

void
IRBuilderAsmJs::BuildAsmSimdTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, Js::ArrayBufferView::ViewType viewType, uint8 dataWidth, uint32 simdOffset)
{
    IRType type = TySimd128F4;
    Js::RegSlot valueRegSlot = GetRegSlotFromSimd128Reg(value);

    IR::RegOpnd * maskedOpnd = nullptr;
    IR::Instr * maskInstr = nullptr;

    Js::OpCode op = GetSimdOpcode(newOpcode);
    ValueType arrayType;
    bool isLd = false, isConst = false;
    uint32 mask = 0;

    switch (newOpcode)
    {
    case Js::OpCodeAsmJs::Simd128_LdArr_I4:
        isLd = true;
        isConst = false;
        type = TySimd128I4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_I8:
        isLd = true;
        isConst = false;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_I16:
        isLd = true;
        isConst = false;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_U4:
        isLd = true;
        isConst = false;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_U8:
        isLd = true;
        isConst = false;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_U16:
        isLd = true;
        isConst = false;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArr_F4:
        isLd = true;
        isConst = false;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_LdArr_D2:
        isLd = true;
        isConst = false;
        type = TySimd128D2;
        break;
#endif // 0

    case Js::OpCodeAsmJs::Simd128_StArr_I4:
        isLd = false;
        isConst = false;
        type = TySimd128I4;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_I8:
        isLd = false;
        isConst = false;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_I16:
        isLd = false;
        isConst = false;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_U4:
        isLd = false;
        isConst = false;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_U8:
        isLd = false;
        isConst = false;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_U16:
        isLd = false;
        isConst = false;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArr_F4:
        isLd = false;
        isConst = false;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_StArr_D2:
        isLd = false;
        isConst = false;
        type = TySimd128D2;
        break;
#endif // 0

    case Js::OpCodeAsmJs::Simd128_LdArrConst_I4:
        isLd = true;
        isConst = true;
        type = TySimd128I4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_I8:
        isLd = true;
        isConst = true;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_I16:
        isLd = true;
        isConst = true;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_U4:
        isLd = true;
        isConst = true;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_U8:
        isLd = true;
        isConst = true;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_U16:
        isLd = true;
        isConst = true;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_LdArrConst_F4:
        isLd = true;
        isConst = true;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_LdArrConst_D2:
        isLd = true;
        isConst = true;
        type = TySimd128D2;
        break;
#endif
    case Js::OpCodeAsmJs::Simd128_StArrConst_I4:
        isLd = false;
        type = TySimd128I4;
        isConst = true;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_I8:
        isLd = false;
        isConst = true;
        type = TySimd128I8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_I16:
        isLd = false;
        isConst = true;
        type = TySimd128I16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_U4:
        isLd = false;
        isConst = true;
        type = TySimd128U4;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_U8:
        isLd = false;
        isConst = true;
        type = TySimd128U8;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_U16:
        isLd = false;
        isConst = true;
        type = TySimd128U16;
        break;
    case Js::OpCodeAsmJs::Simd128_StArrConst_F4:
        isLd = false;
        isConst = true;
        type = TySimd128F4;
        break;
#if 0
    case Js::OpCodeAsmJs::Simd128_StArrConst_D2:
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
#define ARRAYBUFFER_VIEW(name, align, RegType, MemType, irSuffix) \
    case Js::ArrayBufferView::TYPE_##name: \
        mask = ARRAYBUFFER_VIEW_MASK(align); \
        arrayType = ValueType::GetObject(ObjectType::##irSuffix##Array); \
        break;
#include "Language/AsmJsArrayBufferViews.h"
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
        regOpnd->SetValueType(ValueType::Simd);
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
        regOpnd->SetValueType(ValueType::Simd);
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
    indirOpnd->SetOffset(simdOffset);
    if (maskInstr)
    {
        AddInstr(maskInstr, offset);
    }
    AddInstr(instr, offset);

}
#endif
