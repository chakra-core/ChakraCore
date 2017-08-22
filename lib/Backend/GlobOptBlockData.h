//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class ExprAttributes
{
protected:
    uint32 attributes;

public:
    ExprAttributes(const uint32 attributes = 0) : attributes(attributes)
    {
    }

    uint32 Attributes() const
    {
        return attributes;
    }

private:
    static const uint32 BitMask(const uint index)
    {
        return 1u << index;
    }

protected:
    void SetBitAttribute(const uint index, const bool bit)
    {
        if(bit)
        {
            attributes |= BitMask(index);
        }
        else
        {
            attributes &= ~BitMask(index);
        }
    }
};

class ExprHash
{
public:
    ExprHash() { this->opcode = 0; }
    ExprHash(int init) { Assert(init == 0); this->opcode = 0; }

    void Init(Js::OpCode opcode, ValueNumber src1Val, ValueNumber src2Val, ExprAttributes exprAttributes)
    {
        extern uint8 OpCodeToHash[(int)Js::OpCode::Count];

        uint32 opCodeHash = OpCodeToHash[(int)opcode];
        this->opcode = opCodeHash;
        this->src1Val = src1Val;
        this->src2Val = src2Val;
        this->attributes = exprAttributes.Attributes();

        // Assert too many opcodes...
        AssertMsg(this->opcode == (uint32)opCodeHash, "Opcode value too large for CSEs");
        AssertMsg(this->attributes == exprAttributes.Attributes(), "Not enough bits for expr attributes");

        // If value numbers are too large, just give up
        if (this->src1Val != src1Val || this->src2Val != src2Val)
        {
            this->opcode = 0;
            this->src1Val = 0;
            this->src2Val = 0;
            this->attributes = 0;
        }
    }

    Js::OpCode  GetOpcode()             { return (Js::OpCode)this->opcode; }
    ValueNumber GetSrc1ValueNumber()    { return this->src1Val; }
    ValueNumber GetSrc2ValueNumber()    { return this->src2Val; }
    ExprAttributes GetExprAttributes()  { return this->attributes; }
    bool        IsValid()               { return this->opcode != 0; }

    operator    uint()                  { return *(uint*)this; }

private:
    uint32  opcode: 8;
    uint32  src1Val: 11;
    uint32  src2Val: 11;
    uint32  attributes: 2;
};

#include "GlobHashTable.h"

typedef ValueHashTable<Sym *, Value *> GlobHashTable;
typedef HashBucket<Sym *, Value *> GlobHashBucket;

typedef ValueHashTable<ExprHash, Value *> ExprHashTable;
typedef HashBucket<ExprHash, Value *> ExprHashBucket;

typedef JsUtil::BaseHashSet<Value *, JitArenaAllocator> ValueSet;

struct StackLiteralInitFldData
{
    const Js::PropertyIdArray * propIds;
    uint currentInitFldCount;
};

typedef JsUtil::BaseDictionary<StackSym *, StackLiteralInitFldData, JitArenaAllocator> StackLiteralInitFldDataMap;

typedef SList<GlobHashBucket*, JitArenaAllocator> PRECandidatesList;

class GlobOptBlockData
{
    friend class BasicBlock;
public:
    GlobOptBlockData(Func *func) :
        symToValueMap(nullptr),
        exprToValueMap(nullptr),
        liveFields(nullptr),
        liveArrayValues(nullptr),
        maybeWrittenTypeSyms(nullptr),
        liveVarSyms(nullptr),
        liveInt32Syms(nullptr),
        liveLossyInt32Syms(nullptr),
        liveFloat64Syms(nullptr),
#ifdef ENABLE_SIMDJS
        liveSimd128F4Syms(nullptr),
        liveSimd128I4Syms(nullptr),
#endif
        hoistableFields(nullptr),
        argObjSyms(nullptr),
        maybeTempObjectSyms(nullptr),
        canStoreTempObjectSyms(nullptr),
        valuesToKillOnCalls(nullptr),
        inductionVariables(nullptr),
        availableIntBoundChecks(nullptr),
        startCallCount(0),
        argOutCount(0),
        totalOutParamCount(0),
        callSequence(nullptr),
        capturedValuesCandidate(nullptr),
        capturedValues(nullptr),
        changedSyms(nullptr),
        hasCSECandidates(false),
        curFunc(func),
        hasDataRef(nullptr),
        stackLiteralInitFldDataMap(nullptr),
        globOpt(nullptr)
    {
    }

    // Data
    GlobHashTable *                         symToValueMap;
    ExprHashTable *                         exprToValueMap;
    BVSparse<JitArenaAllocator> *           liveFields;
    BVSparse<JitArenaAllocator> *           liveArrayValues;
    BVSparse<JitArenaAllocator> *           maybeWrittenTypeSyms;
    BVSparse<JitArenaAllocator> *           isTempSrc;
    BVSparse<JitArenaAllocator> *           liveVarSyms;
    BVSparse<JitArenaAllocator> *           liveInt32Syms;
    // 'liveLossyInt32Syms' includes only syms that contain an int value that may not fully represent the value of the
    // equivalent var sym. The set (liveInt32Syms - liveLossyInt32Syms) includes only syms that contain an int value that fully
    // represents the value of the equivalent var sym, such as when (a + 1) is type-specialized. Among other things, this
    // bit-vector is used, based on the type of conversion that is needed, to determine whether conversion is necessary, and if
    // so, whether a bailout is needed. For instance, after type-specializing (a | 0), the int32 sym of 'a' cannot be reused in
    // (a + 1) during type-specialization. It needs to be converted again using a lossless conversion with a bailout.
    // Conversely, a lossless int32 sym can be reused to avoid a lossy conversion.
    BVSparse<JitArenaAllocator> *           liveLossyInt32Syms;
    BVSparse<JitArenaAllocator> *           liveFloat64Syms;

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    BVSparse<JitArenaAllocator> *           liveSimd128F4Syms;
    BVSparse<JitArenaAllocator> *           liveSimd128I4Syms;
#endif
    BVSparse<JitArenaAllocator> *           hoistableFields;
    BVSparse<JitArenaAllocator> *           argObjSyms;
    BVSparse<JitArenaAllocator> *           maybeTempObjectSyms;
    BVSparse<JitArenaAllocator> *           canStoreTempObjectSyms;

    // This is the func that this block comes from, so the inlinee if from an inlined function
    Func *                                  curFunc;

    // 'valuesToKillOnCalls' includes values whose value types need to be killed upon a call. Upon a call, the value types of
    // values in the set are updated and removed from the set as appropriate.
    ValueSet *                              valuesToKillOnCalls;

    InductionVariableSet *                  inductionVariables;
    IntBoundCheckSet *                      availableIntBoundChecks;

    // Bailout data
    uint                                    startCallCount;
    uint                                    argOutCount;
    uint                                    totalOutParamCount;
    SListBase<IR::Opnd *> *                 callSequence;
    StackLiteralInitFldDataMap *            stackLiteralInitFldDataMap;

    CapturedValues *                        capturedValuesCandidate;
    CapturedValues *                        capturedValues;
    BVSparse<JitArenaAllocator> *           changedSyms;

    uint                                    inlinedArgOutCount;

    bool                                    hasCSECandidates;

private:
    bool *                                  hasDataRef;

    GlobOpt *                               globOpt;

public:
    void OnDataInitialized(JitArenaAllocator *const allocator)
    {
        Assert(allocator);

        hasDataRef = JitAnew(allocator, bool, true);
    }

    void OnDataReused(GlobOptBlockData *const fromData)
    {
        // If a block's data is deleted, *hasDataRef will be set to false. Since these two blocks are pointing to the same data,
        // they also need to point to the same has-data info.
        hasDataRef = fromData->hasDataRef;
    }

    void OnDataUnreferenced()
    {
        // Other blocks may still be using the data, we should only un-reference the previous data
        hasDataRef = nullptr;
    }

    void OnDataDeleted()
    {
        if (hasDataRef)
            *hasDataRef = false;
        OnDataUnreferenced();
    }

    bool HasData()
    {
        if (!hasDataRef)
            return false;
        if (*hasDataRef)
            return true;
        OnDataUnreferenced();
        return false;
    }

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    BVSparse<JitArenaAllocator> * GetSimd128LivenessBV(IRType type)
    {
        switch (type)
        {
        case TySimd128F4:
            return liveSimd128F4Syms;
        case TySimd128I4:
            return liveSimd128I4Syms;
        default:
            Assert(UNREACHED);
            return nullptr;
        }
    }
#endif
    // Functions pulled out of GlobOpt.cpp

    // Initialization/copying/moving
public:
    void                    NullOutBlockData(GlobOpt* globOpt, Func* func);
    void                    InitBlockData(GlobOpt* globOpt, Func* func);
    void                    ReuseBlockData(GlobOptBlockData *fromData);
    void                    CopyBlockData(GlobOptBlockData *fromData);
    void                    DeleteBlockData();
    void                    CloneBlockData(BasicBlock *const toBlock, BasicBlock *const fromBlock);

public:
    void                    RemoveUnavailableCandidates(PRECandidatesList *candidates);

    // Merging functions
public:
    void                    MergeBlockData(BasicBlock *toBlockContext, BasicBlock *fromBlock, BVSparse<JitArenaAllocator> *const symsRequiringCompensation, BVSparse<JitArenaAllocator> *const symsCreatedForMerge, bool forceTypeSpecOnLoopHeader);
private:
    template <typename CaptureList, typename CapturedItemsAreEqual>
    void                    MergeCapturedValues(SListBase<CaptureList>* toList, SListBase<CaptureList> * fromList, CapturedItemsAreEqual itemsAreEqual);
    void                    MergeValueMaps(BasicBlock *toBlock, BasicBlock *fromBlock, BVSparse<JitArenaAllocator> *const symsRequiringCompensation, BVSparse<JitArenaAllocator> *const symsCreatedForMerge);
    Value *                 MergeValues(Value *toDataValue, Value *fromDataValue, Sym *fromDataSym, bool isLoopBackEdge, BVSparse<JitArenaAllocator> *const symsRequiringCompensation, BVSparse<JitArenaAllocator> *const symsCreatedForMerge);
    ValueInfo *             MergeValueInfo(Value *toDataVal, Value *fromDataVal, Sym *fromDataSym, bool isLoopBackEdge, bool sameValueNumber, BVSparse<JitArenaAllocator> *const symsRequiringCompensation, BVSparse<JitArenaAllocator> *const symsCreatedForMerge);
    JsTypeValueInfo *       MergeJsTypeValueInfo(JsTypeValueInfo * toValueInfo, JsTypeValueInfo * fromValueInfo, bool isLoopBackEdge, bool sameValueNumber);
    ValueInfo *             MergeArrayValueInfo(const ValueType mergedValueType, const ArrayValueInfo *const toDataValueInfo, const ArrayValueInfo *const fromDataValueInfo, Sym *const arraySym, BVSparse<JitArenaAllocator> *const symsRequiringCompensation, BVSparse<JitArenaAllocator> *const symsCreatedForMerge);

    // Argument Tracking
public:
    void                    TrackArgumentsSym(IR::RegOpnd const* opnd);
    void                    ClearArgumentsSym(IR::RegOpnd const* opnd);
    BOOL                    TestAnyArgumentsSym() const;
    BOOL                    IsArgumentsSymID(SymID id) const;
    BOOL                    IsArgumentsOpnd(IR::Opnd const* opnd) const;
private:

    // Value Tracking
public:
    Value *                 FindValue(Sym *sym);
    Value *                 FindValueFromMapDirect(SymID symId);
    Value *                 FindPropertyValue(SymID symId);
    Value *                 FindObjectTypeValue(StackSym* typeSym);
    Value *                 FindObjectTypeValue(SymID typeSymId);
    Value *                 FindObjectTypeValueNoLivenessCheck(StackSym* typeSym);
    Value *                 FindObjectTypeValueNoLivenessCheck(SymID typeSymId);
    Value *                 FindFuturePropertyValue(PropertySym *const propertySym);

    StackSym *              GetCopyPropSym(Sym * sym, Value * val);

    Value *                 InsertNewValue(Value *val, IR::Opnd *opnd);
    Value *                 SetValue(Value *val, IR::Opnd *opnd);
    void                    SetValue(Value *val, Sym * sym);

    void                    ClearSymValue(Sym *sym);

private:
    void                    SetValueToHashTable(GlobHashTable * valueNumberMap, Value *val, Sym *sym);

    // Temp Tracking
public:
    void                    MarkTempLastUse(IR::Instr *instr, IR::RegOpnd *regOpnd);
private:

    // Liveness Tracking
public:
    void                    MakeLive(StackSym *sym, const bool lossy);

    // Checks if the symbol is live in this block under any type
    bool                    IsLive(Sym const * sym) const;
    bool                    IsTypeSpecialized(Sym const * sym) const;
    bool                    IsSwitchInt32TypeSpecialized(IR::Instr const * instr) const;
    bool                    IsInt32TypeSpecialized(Sym const * sym) const;
    bool                    IsFloat64TypeSpecialized(Sym const * sym) const;

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    bool                    IsSimd128TypeSpecialized(Sym const * sym) const;
    bool                    IsSimd128TypeSpecialized(IRType type, Sym const * sym) const;
    bool                    IsSimd128F4TypeSpecialized(Sym const * sym) const;
    bool                    IsSimd128I4TypeSpecialized(Sym const * sym) const;
    bool                    IsLiveAsSimd128(Sym const * sym) const;
    bool                    IsLiveAsSimd128F4(Sym const * sym) const;
    bool                    IsLiveAsSimd128I4(Sym const * sym) const;
#endif

    // Changed Symbol Tracking
public:
    void                    SetChangedSym(SymID symId);
private:

    // Other
public:
    void                    KillStateForGeneratorYield();

    // Debug
public:
    void                    DumpSymToValueMap() const;
private:
    static void             DumpSym(Sym *sym);
};
