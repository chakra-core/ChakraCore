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
        WasmMemoryReg,
        BufferReg,
        LengthReg,
        SharedContents,
        RefCountedBuffer,
        RegCount
    };
};

struct JitLoopBodyData
{
private:
    BVFixed* m_ldSlots = nullptr;
    StackSym* m_loopBodyRetIPSym = nullptr;
    BVFixed* m_yieldRegs = nullptr;
    uint32 m_loopCurRegs[WAsmJs::LIMIT];

public:
    JitLoopBodyData(BVFixed* ldSlots, BVFixed* stSlots, StackSym* loopBodyRetIPSym)
    {
        Assert(ldSlots && stSlots && loopBodyRetIPSym);
        m_ldSlots = ldSlots;
        m_loopBodyRetIPSym = loopBodyRetIPSym;
    }
    // Use m_yieldRegs initialization to determine if m_loopCurRegs is initialized
    bool IsLoopCurRegsInitialized() const { return !!m_yieldRegs; }
    template<typename T> void InitLoopCurRegs(__in_ecount(WAsmJs::LIMIT) T* curRegs, BVFixed* yieldRegs)
    {
        Assert(yieldRegs && curRegs);
        m_yieldRegs = yieldRegs;
        for (WAsmJs::Types type = WAsmJs::Types(0); type != WAsmJs::LIMIT; type = WAsmJs::Types(type + 1))
        {
            m_loopCurRegs[type] = curRegs[type];
        }
    }
    bool IsRegOutsideOfLoop(uint32 typeReg, WAsmJs::Types type) const
    {
        Assert(IsLoopCurRegsInitialized());
        return typeReg < m_loopCurRegs[type];
    }
    bool IsYieldReg(Js::RegSlot reg) const
    {
        if (!m_yieldRegs)
        {
            return false;
        }
        AssertOrFailFast(reg < m_yieldRegs->Length());
        return m_yieldRegs->Test(reg);
    }
    void SetRegIsYield(Js::RegSlot reg)
    {
        Assert(m_yieldRegs);
        AssertOrFailFast(reg < m_yieldRegs->Length());
        m_yieldRegs->Set(reg);
    }
    BVFixed* GetLdSlots() const { return m_ldSlots; }
    StackSym* GetLoopBodyRetIPSym() const { return m_loopBodyRetIPSym; }

#if DBG
    BVFixed* m_usedAsTemp;
#endif
};

class IRBuilderAsmJs
{
    friend struct IRBuilderAsmJsSwitchAdapter;

public:
    IRBuilderAsmJs(Func * func)
        : m_func(func)
        , m_switchAdapter(this)
        , m_switchBuilder(&m_switchAdapter)
    {
        if (!m_func->GetJITFunctionBody()->IsWasmFunction())
        {
            m_statementReader = Anew(func->m_alloc, Js::StatementReader<Js::FunctionBody::ArenaStatementMapList>);
        }
        func->m_workItem->InitializeReader(&m_jnReader, m_statementReader, func->m_alloc);
        m_asmFuncInfo = m_func->GetJITFunctionBody()->GetAsmJsInfo();
#if 0
        // templatized JIT loop body
        if (func->IsLoopBody())
        {
            Js::LoopEntryPointInfo* loopEntryPointInfo = (Js::LoopEntryPointInfo*)(func->m_workItem->GetEntryPoint());
            if (loopEntryPointInfo->GetIsTJMode())
            {
                m_IsTJLoopBody = true;
                func->isTJLoopBody = true;
            }
        }
#endif
    }

    void Build();

private:
    struct MemAccessTypeInfo
    {
        WAsmJs::Types valueRegType;
        IRType type;
        ValueType arrayType;
    };

    void                    LoadNativeCodeData();
    void                    AddInstr(IR::Instr * instr, uint32 offset);
    bool                    IsLoopBody()const;
    uint                    GetLoopBodyExitInstrOffset() const;
    IR::SymOpnd *           BuildAsmJsLoopBodySlotOpnd(Js::RegSlot regSlot, IRType opndType);
    void                    EnsureLoopBodyAsmJsLoadSlot(Js::RegSlot regSlot, IRType type);
    void                    EnsureLoopBodyAsmJsStoreSlot(Js::RegSlot regSlot, IRType type);
    bool                    IsLoopBodyOuterOffset(uint offset) const;
    bool                    IsLoopBodyReturnIPInstr(IR::Instr * instr) const;
    IR::Opnd *              InsertLoopBodyReturnIPInstr(uint targetOffset, uint offset);
    IR::RegOpnd *           BuildDstOpnd(Js::RegSlot dstRegSlot, IRType type);
    IR::RegOpnd *           BuildSrcOpnd(Js::RegSlot srcRegSlot, IRType type);
    IR::RegOpnd *           BuildIntConstOpnd(Js::RegSlot regSlot);
    SymID                   BuildSrcStackSymID(Js::RegSlot regSlot, IRType type = IRType::TyVar);

    IR::SymOpnd *           BuildFieldOpnd(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind, IRType type, bool scale = true);
    PropertySym *           BuildFieldSym(Js::RegSlot reg, Js::PropertyId propertyId, PropertyKind propertyKind);
    uint                    AddStatementBoundary(uint statementIndex, uint offset);
    BranchReloc *           AddBranchInstr(IR::BranchInstr *instr, uint32 offset, uint32 targetOffset);
    BranchReloc *           CreateRelocRecord(IR::BranchInstr * branchInstr, uint32 offset, uint32 targetOffset);
    void                    BuildHeapBufferReload(uint32 offset, bool isFirstLoad = false);
    template<typename T, typename ConstOpnd, typename F>
    void                    CreateLoadConstInstrForType(byte* table, Js::RegSlot& regAllocated, uint32 constCount, uint32 offset, IRType irType, ValueType valueType, Js::OpCode opcode, F extraProcess);
    void                    BuildConstantLoads();
    void                    BuildImplicitArgIns();
    void                    InsertLabels();
    IR::LabelInstr *        CreateLabel(IR::BranchInstr * branchInstr, uint& offset);
    uint32                  GetTypedRegFromRegSlot(Js::RegSlot reg, WAsmJs::Types type);
    Js::RegSlot             GetRegSlotFromTypedReg(Js::RegSlot srcReg, WAsmJs::Types type);
    Js::RegSlot             GetRegSlotFromPtrReg(Js::RegSlot srcReg)
    {
#if TARGET_32
        return GetRegSlotFromIntReg(srcReg);
#elif TARGET_64
        return GetRegSlotFromInt64Reg(srcReg);
#endif
    }

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
    bool                    GetTempUsed(Js::RegSlot reg);
    void                    SetTempUsed(Js::RegSlot reg, bool used);
    bool                    RegIsTemp(Js::RegSlot reg);
    bool                    RegIsConstant(Js::RegSlot reg);
    bool                    RegIsVar(Js::RegSlot reg);
    bool                    RegIsTypedVar(Js::RegSlot reg, WAsmJs::Types type);
    bool                    RegIsTypedConst(Js::RegSlot reg, WAsmJs::Types type);
    bool                    RegIsTypedTmp(Js::RegSlot reg, WAsmJs::Types type);
    bool                    RegIs(Js::RegSlot reg, WAsmJs::Types type);
    bool                    RegIsJitLoopYield(Js::RegSlot reg);
    void                    CheckJitLoopReturn(Js::RegSlot reg, IRType type);

    void                    BuildArgOut(IR::Opnd* srcOpnd, uint32 dstRegSlot, uint32 offset, IRType type, ValueType valueType = ValueType::Uninitialized);
    void                    BuildFromVar(uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot, IRType irType, ValueType valueType);
#define LAYOUT_TYPE(layout) \
    void                    Build##layout(Js::OpCodeAsmJs newOpcode, uint32 offset);
#define LAYOUT_TYPE_WMS(layout) \
    template <typename SizePolicy> void Build##layout(Js::OpCodeAsmJs newOpcode, uint32 offset);
#define EXCLUDE_FRONTEND_LAYOUT
#include "ByteCode/LayoutTypesAsmJs.h"
    void                    BuildSimd_1Ints(Js::OpCodeAsmJs newOpcode, uint32 offset, IRType dstSimdType, Js::RegSlot* srcRegSlots, Js::RegSlot dstRegSlot, uint LANES);
    void                    BuildSimd_1Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType);
    void                    BuildSimd_2Int2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, Js::RegSlot src3RegSlot, IRType simdType, IRType valType = TyInt32);
    void                    BuildSimd_2(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, IRType simdType);
    void                    BuildSimd_2Int1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType);
    void                    BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType simdType);
    void                    BuildSimd_3(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot src1RegSlot, Js::RegSlot src2RegSlot, IRType dstSimdType, IRType srcSimdType);
    void                    BuildSimdConversion(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstRegSlot, Js::RegSlot srcRegSlot, IRType dstSimdType, IRType srcSimdType);
    ValueType               GetSimdValueTypeFromIRType(IRType type);

    void                    BuildElementSlot(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 slotIndex, Js::RegSlot value, Js::RegSlot instance);
    void                    BuildAsmUnsigned1(Js::OpCodeAsmJs newOpcode, uint offset);
    void                    BuildWasmLoopStart(Js::OpCodeAsmJs newOpcode, uint offset);
    void                    BuildWasmMemAccess(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, uint32 constOffset, Js::ArrayBufferView::ViewType viewType);
    void                    BuildAsmTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, Js::ArrayBufferView::ViewType viewType);
    void                    BuildAsmSimdTypedArr(Js::OpCodeAsmJs newOpcode, uint32 offset, uint32 slotIndex, Js::RegSlot value, Js::ArrayBufferView::ViewType viewType, uint8 DataWidth, uint32 simdOffset);
    void                    BuildAsmCall(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::ArgSlot argCount, Js::RegSlot ret, Js::RegSlot function, int8 returnType, Js::ProfileId profileId);
    void                    BuildAsmReg1(Js::OpCodeAsmJs newOpcode, uint32 offset, Js::RegSlot dstReg);
    void                    BuildBrInt1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src);
    void                    BuildBrInt2(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, Js::RegSlot src2);
    void                    BuildBrInt1Const1(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, Js::RegSlot src1, int32 src2);
    void                    BuildBrCmp(Js::OpCodeAsmJs newOpcode, uint32 offset, int32 relativeOffset, IR::RegOpnd* src1Opnd, IR::Opnd* src2Opnd);
    void                    GenerateLoopBodySlotAccesses(uint offset);

    static void             InitializeMemAccessTypeInfo(Js::ArrayBufferView::ViewType viewType, _Out_ MemAccessTypeInfo * typeInfo);

    Js::PropertyId          CalculatePropertyOffset(Js::RegSlot regSlot, IRType type);

    IR::RegOpnd*            BuildTrapIfZero(IR::RegOpnd* srcOpnd, uint32 offset);
    IR::RegOpnd*            BuildTrapIfMinIntOverNegOne(IR::RegOpnd* src1Opnd, IR::RegOpnd* src2Opnd, uint32 offset);

    IR::Instr*              CreateSignExtendInstr(IR::Opnd* dst, IR::Opnd* src, IRType fromType);

    JitArenaAllocator *     m_tempAlloc;
    JitArenaAllocator *     m_funcAlloc;
    Func *                  m_func;
    IR::Instr *             m_lastInstr;
    IR::Instr **            m_offsetToInstruction;
    Js::ByteCodeReader      m_jnReader;
    Js::StatementReader<Js::FunctionBody::ArenaStatementMapList>* m_statementReader = nullptr;
    SListCounted<IR::Instr *> *m_argStack;
    SList<IR::Instr *> *    m_tempList;
    SList<int32> *          m_argOffsetStack;
    SList<BranchReloc *> *  m_branchRelocList;
    // 1 for const, 1 for var, 1 for temps for each type and 1 for last
    static const uint32     m_firstsTypeCount = WAsmJs::LIMIT * 3 + 1;
    Js::RegSlot             m_firstsType[m_firstsTypeCount];
    Js::RegSlot             m_firstVarConst;
    Js::RegSlot             m_tempCount;

    Js::OpCode *            m_simdOpcodesMap;

    Js::RegSlot GetFirstConst(WAsmJs::Types type) { return m_firstsType[type]; }
    Js::RegSlot GetFirstVar(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT]; }
    Js::RegSlot GetFirstTmp(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT * 2]; }
    
    Js::RegSlot GetLastConst(WAsmJs::Types type) { return m_firstsType[type + 1]; }
    Js::RegSlot GetLastVar(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT + 1]; }
    Js::RegSlot GetLastTmp(WAsmJs::Types type) { return m_firstsType[type + WAsmJs::LIMIT * 2 + 1]; }

    JitLoopBodyData& GetJitLoopBodyData() { Assert(IsLoopBody()); return *m_jitLoopBodyData; }
    const JitLoopBodyData& GetJitLoopBodyData() const { Assert(IsLoopBody()); return *m_jitLoopBodyData; }

    SymID *                 m_tempMap;
    BVFixed *               m_fbvTempUsed;
    uint32                  m_functionStartOffset;
    const AsmJsJITInfo *    m_asmFuncInfo;
    JitLoopBodyData*        m_jitLoopBodyData = nullptr;
    IRBuilderAsmJsSwitchAdapter m_switchAdapter;
    SwitchIRBuilder         m_switchBuilder;
    uint32                  m_offsetToInstructionCount;

#define BUILD_LAYOUT_DEF(layout, ...) void Build##layout (Js::OpCodeAsmJs, uint32, __VA_ARGS__);
#define Reg_Type Js::RegSlot
#define Int_Type Js::RegSlot
#define Long_Type Js::RegSlot
#define Float_Type Js::RegSlot
#define Double_Type Js::RegSlot
#define IntConst_Type int
#define LongConst_Type int64
#define FloatConst_Type float
#define DoubleConst_Type double
#define Float32x4_Type Js::RegSlot
#define Bool32x4_Type Js::RegSlot
#define Int32x4_Type Js::RegSlot
#define Float64x2_Type Js::RegSlot
#define Int64x2_Type Js::RegSlot
#define Int16x8_Type Js::RegSlot
#define Bool16x8_Type Js::RegSlot
#define Int8x16_Type Js::RegSlot
#define Bool8x16_Type Js::RegSlot
#define Uint32x4_Type Js::RegSlot
#define Uint16x8_Type Js::RegSlot
#define Uint8x16_Type Js::RegSlot
#define LAYOUT_TYPE_WMS_REG2(layout, t0, t1) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type)
#define LAYOUT_TYPE_WMS_REG3(layout, t0, t1, t2) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type)
#define LAYOUT_TYPE_WMS_REG4(layout, t0, t1, t2, t3) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type)
#define LAYOUT_TYPE_WMS_REG5(layout, t0, t1, t2, t3, t4) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type)
#define LAYOUT_TYPE_WMS_REG6(layout, t0, t1, t2, t3, t4, t5) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type)
#define LAYOUT_TYPE_WMS_REG7(layout, t0, t1, t2, t3, t4, t5, t6) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type, t6##_Type)
#define LAYOUT_TYPE_WMS_REG9(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type, t6##_Type, t7##_Type, t8##_Type)
#define LAYOUT_TYPE_WMS_REG10(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type, t6##_Type, t7##_Type, t8##_Type, t9##_Type)
#define LAYOUT_TYPE_WMS_REG11(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type, t6##_Type, t7##_Type, t8##_Type, t9##_Type, t10##_Type)
#define LAYOUT_TYPE_WMS_REG17(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type, t6##_Type, t7##_Type, t8##_Type, t9##_Type, t10##_Type, t11##_Type, t12##_Type, t13##_Type, t14##_Type, t15##_Type, t16##_Type)
#define LAYOUT_TYPE_WMS_REG18(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type, t6##_Type, t7##_Type, t8##_Type, t9##_Type, t10##_Type, t11##_Type, t12##_Type, t13##_Type, t14##_Type, t15##_Type, t16##_Type, t17##_Type)
#define LAYOUT_TYPE_WMS_REG19(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18) BUILD_LAYOUT_DEF(layout, t0##_Type, t1##_Type, t2##_Type, t3##_Type, t4##_Type, t5##_Type, t6##_Type, t7##_Type, t8##_Type, t9##_Type, t10##_Type, t11##_Type, t12##_Type, t13##_Type, t14##_Type, t15##_Type, t16##_Type, t17##_Type, t18##_Type)
#define EXCLUDE_FRONTEND_LAYOUT
#include "LayoutTypesAsmJs.h"
#undef BUILD_LAYOUT_DEF
#undef RegType
#undef IntType
#undef LongType
#undef FloatType
#undef DoubleType
#undef IntConstType
#undef LongConstType
#undef FloatConstType
#undef DoubleConstType
#undef Float32x4Type
#undef Bool32x4Type
#undef Int32x4Type
#undef Float64x2Type
#undef Int16x8Type
#undef Bool16x8Type
#undef Int8x16Type
#undef Bool8x16Type
#undef Uint32x4Type
#undef Uint16x8Type
#undef Uint8x16Type
};

#endif