//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ASMJS_PLAT
namespace AsmJsRegSlots
{
    enum ConstSlots
    {
        ReturnReg = 0,
        ModuleMemReg,
        ArrayReg,
        BufferReg,
        LengthReg,
        RegCount
    };
};

class IRBuilderAsmJs
{
    friend struct IRBuilderAsmJsSwitchAdapter;

public:
    IRBuilderAsmJs(Func * func)
        : m_func(func)
        , m_IsTJLoopBody(false)
        , m_switchAdapter(this)
        , m_switchBuilder(&m_switchAdapter)
    {
        func->m_workItem->InitializeReader(m_jnReader, m_statementReader);
        m_asmFuncInfo = m_func->GetJnFunction()->GetAsmJsFunctionInfoWithLock();
        if (func->IsLoopBody())
        {
            Js::LoopEntryPointInfo* loopEntryPointInfo = (Js::LoopEntryPointInfo*)(func->m_workItem->GetEntryPoint());
            if (loopEntryPointInfo->GetIsTJMode())
            {
                m_IsTJLoopBody = true;
                func->isTJLoopBody = true;
            }
        }
    }

    void Build();

private:

    void                    AddInstr(IR::Instr * instr, uint32 offset);
    bool                    IsLoopBody()const;
    uint                    GetLoopBodyExitInstrOffset() const;
    IR::SymOpnd *           BuildLoopBodySlotOpnd(SymID symId);
    IR::SymOpnd *           BuildAsmJsLoopBodySlotOpnd(SymID symId, IRType opndType);
    void                    EnsureLoopBodyLoadSlot(SymID symId);
    void                    EnsureLoopBodyAsmJsLoadSlot(SymID symId, IRType type);
    bool                    IsLoopBodyOuterOffset(uint offset) const;
    bool                    IsLoopBodyReturnIPInstr(IR::Instr * instr) const;
    IR::Opnd *              InsertLoopBodyReturnIPInstr(uint targetOffset, uint offset);
    IR::Instr *             CreateLoopBodyReturnIPInstr(uint targetOffset, uint offset);
    IR::RegOpnd *           BuildDstOpnd(Js::RegSlot dstRegSlot, IRType type);
    IR::RegOpnd *           BuildSrcOpnd(Js::RegSlot srcRegSlot, IRType type);
    IR::RegOpnd *           BuildIntConstOpnd(Js::RegSlot regSlot);
    SymID                   BuildSrcStackSymID(Js::RegSlot regSlot, IRType type = IRType::TyVar);

    IR::SymOpnd *           BuildFieldOpnd(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind, IRType type, bool scale = true);
    PropertySym *           BuildFieldSym(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind);
    uint                    AddStatementBoundary(uint statementIndex, uint offset);
    BranchReloc *           AddBranchInstr(IR::BranchInstr *instr, uint32 offset, uint32 targetOffset);
    BranchReloc *           CreateRelocRecord(IR::BranchInstr * branchInstr, uint32 offset, uint32 targetOffset);
    void                    BuildHeapBufferReload(uint32 offset);
    template<typename T, typename ConstOpnd, typename F> 
    void                    CreateLoadConstInstrForType(byte* table, Js::RegSlot& regAllocated, uint32 constCount, uint32 offset, IRType irType, ValueType valueType, Js::OpCode opcode, F extraProcess);
    void                    BuildConstantLoads();
    void                    BuildImplicitArgIns();
    void                    InsertLabels();
    IR::LabelInstr *        CreateLabel(IR::BranchInstr * branchInstr, uint& offset);
#if DBG
    BVFixed *               m_usedAsTemp;
#endif
    Js::RegSlot             GetRegSlotFromTypedReg(Js::RegSlot srcReg, WAsmJs::Types type);
    Js::RegSlot             GetRegSlotFromIntReg(Js::RegSlot srcIntReg) {return GetRegSlotFromTypedReg(srcIntReg, WAsmJs::INT32);}
    Js::RegSlot             GetRegSlotFromInt64Reg(Js::RegSlot srcIntReg) {return GetRegSlotFromTypedReg(srcIntReg, WAsmJs::INT64);}
    Js::RegSlot             GetRegSlotFromFloatReg(Js::RegSlot srcFloatReg) {return GetRegSlotFromTypedReg(srcFloatReg, WAsmJs::FLOAT32);}
    Js::RegSlot             GetRegSlotFromDoubleReg(Js::RegSlot srcDoubleReg) {return GetRegSlotFromTypedReg(srcDoubleReg, WAsmJs::FLOAT64);}
    Js::RegSlot             GetRegSlotFromSimd128Reg(Js::RegSlot srcSimd128Reg) {return GetRegSlotFromTypedReg(srcSimd128Reg, WAsmJs::SIMD);}

    Js::RegSlot             GetRegSlotFromVarReg(Js::RegSlot srcVarReg);
    Js::OpCode              GetSimdOpcode(Js::OpCodeAsmJs asmjsOpcode);
    void                    GetSimdTypesFromAsmType(Js::AsmJsType::Which asmType, IRType *pIRType, ValueType *pValueType = nullptr);
    IR::Instr *             AddExtendedArg(IR::RegOpnd *src1, IR::RegOpnd *src2, uint32 offset);
    bool                    RegIsSimd128ReturnVar(Js::RegSlot reg);
    SymID                   GetMappedTemp(Js::RegSlot reg);
    void                    SetMappedTemp(Js::RegSlot reg, SymID tempId);
    BOOL                    GetTempUsed(Js::RegSlot reg);
    void                    SetTempUsed(Js::RegSlot reg, BOOL used);
    BOOL                    RegIsTemp(Js::RegSlot reg);
    BOOL                    RegIsConstant(Js::RegSlot reg);
    BOOL                    RegIsVar(Js::RegSlot reg);
    BOOL                    RegIsTypedVar(Js::RegSlot reg, WAsmJs::Types type);
    BOOL                    RegIsIntVar(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::INT32);}
    BOOL                    RegIsInt64Var(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::INT64);}
    BOOL                    RegIsFloatVar(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::FLOAT32);}
    BOOL                    RegIsDoubleVar(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::FLOAT64);}
    BOOL                    RegIsSimd128Var(Js::RegSlot reg) {return RegIsTypedVar(reg, WAsmJs::SIMD);}

#define LAYOUT_TYPE(layout) \
    void                    Build##layout(Js::OpCodeAsmJs newOpcode, uint32 offset);
#define LAYOUT_TYPE_WMS(layout) \
    template <typename SizePolicy> void Build##layout(Js::OpCodeAsmJs newOpcode, uint32 offset);
#define EXCLUDE_FRONTEND_LAYOUT
#include "ByteCode/LayoutTypesAsmJs.h"
    void                    BuildSimd_1Ints(Js::OpCodeAsmJs newOpcode, uint32 offset, IRType dstSimdType, Js::RegSlot* srcRegSlots, Js::RegSlot dstRegSlot, uint LANES);
    void                    BuildSimd_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType);
    void                    BuildSimd_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, Js::RegSlot src3RegSlot, IRType simdType);
    void                    BuildSimd_2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType);
    void                    BuildSimd_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType);
    void                    BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType);
    void                    BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType dstSimdType, IRType srcSimdType);
    void                    BuildSimdConversion(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot, IRType dstSimdType, IRType srcSimdType);
    ValueType               GetSimdValueTypeFromIRType(IRType type);

    void                    BuildElementSlot(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 slotIndex, Js::RegSlot value, Js::RegSlot instance);
    void                    BuildAsmUnsigned1(Js::OpCodeAsmJs newOpcode, uint value);
    void                    BuildAsmTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, int8 viewType);
    void                    BuildAsmSimdTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, int8 viewType, uint8 DataWidth);
    void                    BuildAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::ArgSlot argCount, Js::RegSlot ret, Js::RegSlot function, int8 returnType);
    void                    BuildAsmReg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg);
    void                    BuildInt1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstIntReg, Js::RegSlot srcDoubleReg);
    void                    BuildInt1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstIntReg, Js::RegSlot srcFloatReg);
    void                    BuildDouble1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstDoubleReg, Js::RegSlot srcIntReg);
    void                    BuildDouble1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstDoubleReg, Js::RegSlot srcFloatReg);
    void                    BuildFloat1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstFloatReg, Js::RegSlot srcVarReg);
    void                    BuildDouble1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstDoubleReg, Js::RegSlot srcVarReg);
    void                    BuildInt1Reg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstIntReg, Js::RegSlot srcVarReg);
    void                    BuildReg1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcDoubleReg);
    void                    BuildReg1Float1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcFloatReg);
    void                    BuildReg1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg, Js::RegSlot srcIntReg);
    void                    BuildInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstInt, int constInt);
    void                    BuildFloat1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, float constVal);
    void                    BuildDouble1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, double constVal);
    void                    BuildInt1Double2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildInt1Float2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildInt2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src);
    void                    BuildInt3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildDouble2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src);
    void                    BuildFloat2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src);
    void                    BuildFloat3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildFloat1Double1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src);
    void                    BuildFloat1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src);
    void                    BuildDouble3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dst, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildBrInt1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src);
    void                    BuildBrInt2(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildBrInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, int32 src2);
    void                    BuildBrCmp(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, IR::RegOpnd* src1Opnd, IR::Opnd* src2Opnd);
    void                    GenerateLoopBodySlotAccesses(uint offset);
    void                    GenerateLoopBodyStSlots(SymID loopParamSymId, uint offset);

    Js::PropertyId          CalculatePropertyOffset(SymID id, IRType type, bool isVar = true);

    IR::Instr*              GenerateStSlotForReturn(IR::RegOpnd* srcOpnd, IRType type);
    JitArenaAllocator *     m_tempAlloc;
    JitArenaAllocator *     m_funcAlloc;
    Func *                  m_func;
    IR::Instr *             m_lastInstr;
    IR::Instr **            m_offsetToInstruction;
    Js::ByteCodeReader      m_jnReader;
    Js::StatementReader     m_statementReader;
    SList<IR::Instr *> *    m_argStack;
    SList<IR::Instr *> *    m_tempList;
    SList<int32> *          m_argOffsetStack;
    SList<BranchReloc *> *  m_branchRelocList;
    // 1 for const, 1 for var, 1 for temps for each type and 1 for last
    static constexpr uint32 m_firstsTypeCount = WAsmJs::LIMIT * 3 + 1;
    Js::RegSlot             m_firstsType[m_firstsTypeCount];
    Js::RegSlot             m_firstVarConst;
    Js::RegSlot             m_firstIRTemp;
    Js::OpCode *            m_simdOpcodesMap;

    Js::RegSlot GetFirstConst(WAsmJs::Types type) { return m_firstsType[type]; }
    Js::RegSlot GetFirstVar(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT]; }
    Js::RegSlot GetFirstTmp(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT * 2]; }
    
    Js::RegSlot GetLastConst(WAsmJs::Types type) { return m_firstsType[type + 1]; }
    Js::RegSlot GetLastVar(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT + 1]; }
    Js::RegSlot GetLastTmp(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT * 2 + 1]; }

    SymID *                 m_tempMap;
    BVFixed *               m_fbvTempUsed;
    uint32                  m_functionStartOffset;
    Js::AsmJsFunctionInfo * m_asmFuncInfo;
    StackSym *              m_loopBodyRetIPSym;
    BVFixed *               m_ldSlots;
    BVFixed *               m_stSlots;
    BOOL                    m_IsTJLoopBody;
    IRBuilderAsmJsSwitchAdapter m_switchAdapter;
    SwitchIRBuilder         m_switchBuilder;
    IR::RegOpnd *           m_funcOpnd;
#if DBG
    uint32                  m_offsetToInstructionCount;
#endif
};

#endif