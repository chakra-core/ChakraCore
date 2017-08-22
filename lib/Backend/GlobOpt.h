//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
class BackwardPass;

class LoopCount;
class GlobOpt;

#if ENABLE_DEBUG_CONFIG_OPTIONS && DBG_DUMP

#define GOPT_TRACE_OPND(opnd, ...) \
    if (PHASE_TRACE(Js::GlobOptPhase, this->func) && !this->IsLoopPrePass()) \
    { \
        Output::Print(_u("TRACE: ")); \
        opnd->Dump(); \
        Output::Print(_u(" : ")); \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }
#define GOPT_TRACE(...) \
    if (PHASE_TRACE(Js::GlobOptPhase, this->func) && !this->IsLoopPrePass()) \
    { \
        Output::Print(_u("TRACE: ")); \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define GOPT_TRACE_INSTRTRACE(instr) \
    if (PHASE_TRACE(Js::GlobOptPhase, this->func) && !this->IsLoopPrePass()) \
    { \
        instr->Dump(); \
        Output::Flush(); \
    }

#define GOPT_TRACE_INSTR(instr, ...) \
    if (PHASE_TRACE(Js::GlobOptPhase, this->func) && !this->IsLoopPrePass()) \
    { \
        Output::Print(_u("TRACE: ")); \
        Output::Print(__VA_ARGS__); \
        instr->Dump(); \
        Output::Flush(); \
    }

#define GOPT_TRACE_BLOCK(block, before) \
    this->Trace(block, before); \
    Output::Flush();

// TODO: OOP JIT, add back line number
#define TRACE_PHASE_INSTR(phase, instr, ...) \
    if(PHASE_TRACE(phase, this->func)) \
    { \
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]; \
        Output::Print( \
            _u("Function %s (%s)"), \
            this->func->GetJITFunctionBody()->GetDisplayName(), \
            this->func->GetDebugNumberSet(debugStringBuffer)); \
        if(this->func->IsLoopBody()) \
        { \
            Output::Print(_u(", loop %u"), this->func->GetWorkItem()->GetLoopNumber()); \
        } \
        if(instr->m_func != this->func) \
        { \
            Output::Print( \
                _u(", Inlinee %s (%s)"), \
                instr->m_func->GetJITFunctionBody()->GetDisplayName(), \
                instr->m_func->GetDebugNumberSet(debugStringBuffer)); \
        } \
        Output::Print(_u(" - %s\n    "), Js::PhaseNames[phase]); \
        instr->Dump(); \
        Output::Print(_u("    ")); \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define TRACE_PHASE_INSTR_VERBOSE(phase, instr, ...) \
    if(CONFIG_FLAG(Verbose)) \
    { \
        TRACE_PHASE_INSTR(phase, instr, __VA_ARGS__); \
    }

#define TRACE_TESTTRACE_PHASE_INSTR(phase, instr, ...) \
    TRACE_PHASE_INSTR(phase, instr, __VA_ARGS__); \
    TESTTRACE_PHASE_INSTR(phase, instr, __VA_ARGS__);

#else   // ENABLE_DEBUG_CONFIG_OPTIONS && DBG_DUMP

#define GOPT_TRACE(...)
#define GOPT_TRACE_OPND(opnd, ...)
#define GOPT_TRACE_INSTRTRACE(instr)
#define GOPT_TRACE_INSTR(instr, ...)
#define GOPT_TRACE_BLOCK(block, before)
#define TRACE_PHASE_INSTR(phase, instr, ...)
#define TRACE_PHASE_INSTR_VERBOSE(phase, instr, ...)
#define TRACE_TESTTRACE_PHASE_INSTR(phase, instr, ...) TESTTRACE_PHASE_INSTR(phase, instr, __VA_ARGS__);

#endif  // ENABLE_DEBUG_CONFIG_OPTIONS && DBG_DUMP

class IntMathExprAttributes : public ExprAttributes
{
private:
    static const uint IgnoredIntOverflowIndex = 0;
    static const uint IgnoredNegativeZeroIndex = 1;

public:
    IntMathExprAttributes(const ExprAttributes &exprAttributes) : ExprAttributes(exprAttributes)
    {
    }

    IntMathExprAttributes(const bool ignoredIntOverflow, const bool ignoredNegativeZero)
    {
        SetBitAttribute(IgnoredIntOverflowIndex, ignoredIntOverflow);
        SetBitAttribute(IgnoredNegativeZeroIndex, ignoredNegativeZero);
    }
};

class ConvAttributes : public ExprAttributes
{
private:
    static const uint DstUnsignedIndex = 0;
    static const uint SrcUnsignedIndex = 1;

public:
    ConvAttributes(const ExprAttributes &exprAttributes) : ExprAttributes(exprAttributes)
    {
    }

    ConvAttributes(const bool isDstUnsigned, const bool isSrcUnsigned)
    {
        SetBitAttribute(DstUnsignedIndex, isDstUnsigned);
        SetBitAttribute(SrcUnsignedIndex, isSrcUnsigned);
    }
};

class DstIsIntOrNumberAttributes : public ExprAttributes
{
private:
    static const uint DstIsIntOnlyIndex = 0;
    static const uint DstIsNumberOnlyIndex = 1;

public:
    DstIsIntOrNumberAttributes(const ExprAttributes &exprAttributes) : ExprAttributes(exprAttributes)
    {
    }

    DstIsIntOrNumberAttributes(const bool dstIsIntOnly, const bool dstIsNumberOnly)
    {
        SetBitAttribute(DstIsIntOnlyIndex, dstIsIntOnly);
        SetBitAttribute(DstIsNumberOnlyIndex, dstIsNumberOnly);
    }
};

enum class PathDependentRelationship : uint8
{
    Equal,
    NotEqual,
    GreaterThanOrEqual,
    GreaterThan,
    LessThanOrEqual,
    LessThan
};

class PathDependentInfo
{
private:
    Value *leftValue, *rightValue;
    int32 rightConstantValue;
    PathDependentRelationship relationship;

public:
    PathDependentInfo(const PathDependentRelationship relationship, Value *const leftValue, Value *const rightValue)
        : relationship(relationship), leftValue(leftValue), rightValue(rightValue)
    {
        Assert(leftValue);
        Assert(rightValue);
    }

    PathDependentInfo(
        const PathDependentRelationship relationship,
        Value *const leftValue,
        Value *const rightValue,
        const int32 rightConstantValue)
        : relationship(relationship), leftValue(leftValue), rightValue(rightValue), rightConstantValue(rightConstantValue)
    {
        Assert(leftValue);
    }

public:
    bool HasInfo() const
    {
        return !!leftValue;
    }

    PathDependentRelationship Relationship() const
    {
        Assert(HasInfo());
        return relationship;
    }

    Value *LeftValue() const
    {
        Assert(HasInfo());
        return leftValue;
    }

    Value *RightValue() const
    {
        Assert(HasInfo());
        return rightValue;
    }

    int32 RightConstantValue() const
    {
        Assert(!RightValue());
        return rightConstantValue;
    }
};

class PathDependentInfoToRestore
{
private:
    ValueInfo *leftValueInfo, *rightValueInfo;

public:
    PathDependentInfoToRestore() : leftValueInfo(nullptr), rightValueInfo(nullptr)
    {
    }

    PathDependentInfoToRestore(ValueInfo *const leftValueInfo, ValueInfo *const rightValueInfo)
        : leftValueInfo(leftValueInfo), rightValueInfo(rightValueInfo)
    {
    }

public:
    ValueInfo *LeftValueInfo() const
    {
        return leftValueInfo;
    }

    ValueInfo *RightValueInfo() const
    {
        return rightValueInfo;
    }

public:
    void Clear()
    {
        leftValueInfo = nullptr;
        rightValueInfo = nullptr;
    }
};

typedef JsUtil::List<IR::Opnd *, JitArenaAllocator> OpndList;
typedef JsUtil::BaseDictionary<Sym *, ValueInfo *, JitArenaAllocator> SymToValueInfoMap;
typedef JsUtil::BaseDictionary<SymID, IR::Instr *, JitArenaAllocator> SymIdToInstrMap;
typedef JsUtil::BaseHashSet<Value *, JitArenaAllocator, PowerOf2SizePolicy, ValueNumber> ValueSetByValueNumber;
typedef JsUtil::BaseDictionary<SymID, StackSym *, JitArenaAllocator> SymIdToStackSymMap;
typedef JsUtil::Pair<ValueNumber, ValueNumber> ValueNumberPair;
typedef JsUtil::BaseDictionary<ValueNumberPair, Value *, JitArenaAllocator> ValueNumberPairToValueMap;

namespace JsUtil
{
    template <>
    class ValueEntry<StackLiteralInitFldData> : public BaseValueEntry<StackLiteralInitFldData>
    {
    public:
        void Clear()
        {
#if DBG
            this->value.propIds = nullptr;
            this->value.currentInitFldCount = (uint)-1;
#endif
        }
    };
};

typedef JsUtil::BaseDictionary<IntConstType, StackSym *, JitArenaAllocator> IntConstantToStackSymMap;
typedef JsUtil::BaseDictionary<int32, Value *, JitArenaAllocator> IntConstantToValueMap;
typedef JsUtil::BaseDictionary<int64, Value *, JitArenaAllocator> Int64ConstantToValueMap;

typedef JsUtil::BaseDictionary<Js::Var, Value *, JitArenaAllocator> AddrConstantToValueMap;
typedef JsUtil::BaseDictionary<Js::InternalString, Value *, JitArenaAllocator> StringConstantToValueMap;

class JsArrayKills
{
private:
    union
    {
        struct
        {
            bool killsAllArrays : 1;
            bool killsArraysWithNoMissingValues : 1;
            bool killsNativeArrays : 1;
            bool killsArrayHeadSegments : 1;
            bool killsArrayHeadSegmentLengths : 1;
            bool killsArrayLengths : 1;
        };
        byte bits;
    };

public:
    JsArrayKills() : bits(0)
    {
    }

private:
    JsArrayKills(const byte bits) : bits(bits)
    {
    }

public:
    bool KillsAllArrays() const { return killsAllArrays; }
    void SetKillsAllArrays() { killsAllArrays = true; }

    bool KillsArraysWithNoMissingValues() const { return killsArraysWithNoMissingValues; }
    void SetKillsArraysWithNoMissingValues() { killsArraysWithNoMissingValues = true; }

    bool KillsNativeArrays() const { return killsNativeArrays; }
    void SetKillsNativeArrays() { killsNativeArrays = true; }

    bool KillsArrayHeadSegments() const { return killsArrayHeadSegments; }
    void SetKillsArrayHeadSegments() { killsArrayHeadSegments = true; }

    bool KillsArrayHeadSegmentLengths() const { return killsArrayHeadSegmentLengths; }
    void SetKillsArrayHeadSegmentLengths() { killsArrayHeadSegmentLengths = true; }

    bool KillsTypedArrayHeadSegmentLengths() const { return KillsAllArrays(); }

    bool KillsArrayLengths() const { return killsArrayLengths; }
    void SetKillsArrayLengths() { killsArrayLengths = true; }

public:
    bool KillsValueType(const ValueType valueType) const
    {
        Assert(valueType.IsArrayOrObjectWithArray());

        return
            killsAllArrays ||
            (killsArraysWithNoMissingValues && valueType.HasNoMissingValues()) ||
            (killsNativeArrays && !valueType.HasVarElements());
    }

    bool AreSubsetOf(const JsArrayKills &other) const
    {
        return (bits & other.bits) == bits;
    }

    JsArrayKills Merge(const JsArrayKills &other)
    {
        return bits | other.bits;
    }
};

class InvariantBlockBackwardIterator
{
private:
    GlobOpt *const globOpt;
    BasicBlock *const exclusiveEndBlock;
    StackSym *const invariantSym;
    const ValueNumber invariantSymValueNumber;
    BasicBlock *block;
    Value *invariantSymValue;

#if DBG
    BasicBlock *const inclusiveEndBlock;
#endif

public:
    InvariantBlockBackwardIterator(GlobOpt *const globOpt, BasicBlock *const exclusiveBeginBlock, BasicBlock *const inclusiveEndBlock, StackSym *const invariantSym, const ValueNumber invariantSymValueNumber = InvalidValueNumber);

public:
    bool IsValid() const;
    void MoveNext();
    BasicBlock *Block() const;
    Value *InvariantSymValue() const;

    PREVENT_ASSIGN(InvariantBlockBackwardIterator);
};

class FlowGraph;

class GlobOpt
{
private:
    class AddSubConstantInfo;
    class ArrayLowerBoundCheckHoistInfo;
    class ArrayUpperBoundCheckHoistInfo;

    friend BackwardPass;
#if DBG
    friend class ObjectTempVerify;
#endif
    friend class GlobOptBlockData;
    friend class BasicBlock;

private:
    SparseArray<Value>       *  byteCodeConstantValueArray;
    // Global bitvectors
    BVSparse<JitArenaAllocator> * byteCodeConstantValueNumbersBv;
   
    // Global bitvectors
    IntConstantToStackSymMap *  intConstantToStackSymMap;
    IntConstantToValueMap*      intConstantToValueMap;
    Int64ConstantToValueMap*    int64ConstantToValueMap;
    AddrConstantToValueMap *    addrConstantToValueMap;
    StringConstantToValueMap *  stringConstantToValueMap;
#if DBG
    // We can still track the finished stack literal InitFld lexically.
    BVSparse<JitArenaAllocator> * finishedStackLiteralInitFld;
#endif

    BVSparse<JitArenaAllocator> *  byteCodeUses;
    BVSparse<JitArenaAllocator> *  tempBv;            // Bit vector for temporary uses
    BVSparse<JitArenaAllocator> *  objectTypeSyms;
    BVSparse<JitArenaAllocator> *  prePassCopyPropSym;  // Symbols that were copy prop'd during loop prepass

    // Symbols that refer to slots in the stack frame.  We still use currentBlock->liveFields to tell us
    // which of these slots are live; this bit-vector just identifies which entries in liveFields represent
    // slots, so we can zero them all out quickly.
    BVSparse<JitArenaAllocator> *  slotSyms;

    PropertySym *               propertySymUse;

    BVSparse<JitArenaAllocator> *  lengthEquivBv;
    BVSparse<JitArenaAllocator> *  argumentsEquivBv;
    BVSparse<JitArenaAllocator> *  callerEquivBv;

    BVSparse<JitArenaAllocator> *   changedSymsAfterIncBailoutCandidate;

    JitArenaAllocator *             alloc;
    JitArenaAllocator *             tempAlloc;

    Func *                  func;
    ValueNumber             currentValue;
    BasicBlock *            currentBlock;
    Region *                currentRegion;
    IntOverflowDoesNotMatterRange *intOverflowDoesNotMatterRange;
    Loop *                  prePassLoop;
    Loop *                  rootLoopPrePass;
    uint                    instrCountSinceLastCleanUp;
    SymIdToInstrMap *       prePassInstrMap;
    SymID                   maxInitialSymID;
    bool                    isCallHelper: 1;
    bool                    intOverflowCurrentlyMattersInRange : 1;
    bool                    ignoredIntOverflowForCurrentInstr : 1;
    bool                    ignoredNegativeZeroForCurrentInstr : 1;
    bool                    inInlinedBuiltIn : 1;
    bool                    isRecursiveCallOnLandingPad : 1;
    bool                    updateInductionVariableValueNumber : 1;
    bool                    isPerformingLoopBackEdgeCompensation : 1;

    bool                    doTypeSpec : 1;
    bool                    doAggressiveIntTypeSpec : 1;
    bool                    doAggressiveMulIntTypeSpec : 1;
    bool                    doDivIntTypeSpec : 1;
    bool                    doLossyIntTypeSpec : 1;
    bool                    doFloatTypeSpec : 1;
    bool                    doArrayCheckHoist : 1;
    bool                    doArrayMissingValueCheckHoist : 1;

    bool                    doArraySegmentHoist : 1;
    bool                    doJsArraySegmentHoist : 1;
    bool                    doArrayLengthHoist : 1;
    bool                    doEliminateArrayAccessHelperCall : 1;
    bool                    doTrackRelativeIntBounds : 1;
    bool                    doBoundCheckElimination : 1;
    bool                    doBoundCheckHoist : 1;
    bool                    doLoopCountBasedBoundCheckHoist : 1;

    bool                    doPowIntIntTypeSpec : 1;
    bool                    isAsmJSFunc : 1;
    bool                    doTagChecks : 1;
    OpndList *              noImplicitCallUsesToInsert;

    ValueSetByValueNumber * valuesCreatedForClone;
    ValueNumberPairToValueMap *valuesCreatedForMerge;

#if DBG
    BVSparse<JitArenaAllocator> * byteCodeUsesBeforeOpt;
#endif
public:
    GlobOpt(Func * func);

    void                    Optimize();

    // Function used by the backward pass as well.
    // GlobOptBailout.cpp
    static void             TrackByteCodeSymUsed(IR::Instr * instr, BVSparse<JitArenaAllocator> * instrByteCodeStackSymUsed, PropertySym **pPropertySym);

    // GlobOptFields.cpp
    void                    ProcessFieldKills(IR::Instr *instr, BVSparse<JitArenaAllocator> * bv, bool inGlobOpt);
    static bool             DoFieldHoisting(Loop * loop);

    IR::ByteCodeUsesInstr * ConvertToByteCodeUses(IR::Instr * isntr);
    bool GetIsAsmJSFunc()const{ return isAsmJSFunc; };
private:
    bool                    IsLoopPrePass() const { return this->prePassLoop != nullptr; }
    void                    OptBlock(BasicBlock *block);
    void                    BackwardPass(Js::Phase tag);
    void                    ForwardPass();
    void                    OptLoops(Loop *loop);
    void                    TailDupPass();
    bool                    TryTailDup(IR::BranchInstr *tailBranch);
    PRECandidatesList *     FindBackEdgePRECandidates(BasicBlock *block, JitArenaAllocator *alloc);
    PRECandidatesList *     FindPossiblePRECandidates(Loop *loop, JitArenaAllocator *alloc);
    void                    PreloadPRECandidates(Loop *loop, PRECandidatesList *candidates);
    BOOL                    PreloadPRECandidate(Loop *loop, GlobHashBucket* candidate);
    void                    SetLoopFieldInitialValue(Loop *loop, IR::Instr *instr, PropertySym *propertySym, PropertySym *originalPropertySym);
    void                    FieldPRE(Loop *loop);
    void                    CloneBlockData(BasicBlock *const toBlock, BasicBlock *const fromBlock);
    void                    CloneValues(BasicBlock *const toBlock, GlobOptBlockData *toData, GlobOptBlockData *fromData);

    void                    TryReplaceLdLen(IR::Instr *& instr);
    IR::Instr *             OptInstr(IR::Instr *&instr, bool* isInstrCleared);
    Value*                  OptDst(IR::Instr **pInstr, Value *dstVal, Value *src1Val, Value *src2Val, Value *dstIndirIndexVal, Value *src1IndirIndexVal);
    void                    CopyPropDstUses(IR::Opnd *opnd, IR::Instr *instr, Value *src1Val);
    Value *                 OptSrc(IR::Opnd *opnd, IR::Instr * *pInstr, Value **indirIndexValRef = nullptr, IR::IndirOpnd *parentIndirOpnd = nullptr);
    void                    MarkArgumentsUsedForBranch(IR::Instr *inst);
    bool                    OptTagChecks(IR::Instr *instr);
    void                    TryOptimizeInstrWithFixedDataProperty(IR::Instr * * const pInstr);
    bool                    CheckIfPropOpEmitsTypeCheck(IR::Instr *instr, IR::PropertySymOpnd *opnd);
    IR::PropertySymOpnd *   CreateOpndForTypeCheckOnly(IR::PropertySymOpnd* opnd, Func* func);
    bool                    FinishOptPropOp(IR::Instr *instr, IR::PropertySymOpnd *opnd, BasicBlock* block = nullptr, bool updateExistingValue = false, bool* emitsTypeCheckOut = nullptr, bool* changesTypeValueOut = nullptr);
    void                    FinishOptHoistedPropOps(Loop * loop);
    IR::Instr *             SetTypeCheckBailOut(IR::Opnd *opnd, IR::Instr *instr, BailOutInfo *bailOutInfo);
    void                    OptArguments(IR::Instr *Instr);
    void                    TrackInstrsForScopeObjectRemoval(IR::Instr * instr);
    bool                    AreFromSameBytecodeFunc(IR::RegOpnd const* src1, IR::RegOpnd const* dst) const;
    Value *                 ValueNumberDst(IR::Instr **pInstr, Value *src1Val, Value *src2Val);
    Value *                 ValueNumberLdElemDst(IR::Instr **pInstr, Value *srcVal);
    ValueType               GetPrepassValueTypeForDst(const ValueType desiredValueType, IR::Instr *const instr, Value *const src1Value, Value *const src2Value, bool *const isValueInfoPreciseRef = nullptr) const;
    bool                    IsPrepassSrcValueInfoPrecise(IR::Opnd *const src, Value *const srcValue) const;
    Value *                 CreateDstUntransferredIntValue(const int32 min, const int32 max, IR::Instr *const instr, Value *const src1Value, Value *const src2Value);
    Value *                 CreateDstUntransferredValue(const ValueType desiredValueType, IR::Instr *const instr, Value *const src1Value, Value *const src2Value);
    Value *                 ValueNumberTransferDst(IR::Instr *const instr, Value *src1Val);
    bool                    IsSafeToTransferInPrePass(IR::Opnd *src, Value *srcValue);
    Value *                 ValueNumberTransferDstInPrepass(IR::Instr *const instr, Value *const src1Val);
    IR::Opnd *              CopyProp(IR::Opnd *opnd, IR::Instr *instr, Value *val, IR::IndirOpnd *parentIndirOpnd = nullptr);
    IR::Opnd *              CopyPropReplaceOpnd(IR::Instr * instr, IR::Opnd * opnd, StackSym * copySym, IR::IndirOpnd *parentIndirOpnd = nullptr);

    ValueNumber             NewValueNumber();
    Value *                 NewValue(ValueInfo *const valueInfo);
    Value *                 NewValue(const ValueNumber valueNumber, ValueInfo *const valueInfo);
    Value *                 CopyValue(Value const *const value);
    Value *                 CopyValue(Value const *const value, const ValueNumber valueNumber);

    Value *                 NewGenericValue(const ValueType valueType);
    Value *                 NewGenericValue(const ValueType valueType, IR::Opnd *const opnd);
    Value *                 NewGenericValue(const ValueType valueType, Sym *const sym);
    Value *                 GetIntConstantValue(const int32 intConst, IR::Instr * instr, IR::Opnd *const opnd = nullptr);
    Value *                 GetIntConstantValue(const int64 intConst, IR::Instr * instr, IR::Opnd *const opnd = nullptr);
    Value *                 NewIntConstantValue(const int32 intConst, IR::Instr * instr, bool isTaggable);
    Value *                 NewInt64ConstantValue(const int64 intConst, IR::Instr * instr);
    ValueInfo *             NewIntRangeValueInfo(const int32 min, const int32 max, const bool wasNegativeZeroPreventedByBailout);
    ValueInfo *             NewIntRangeValueInfo(const ValueInfo *const originalValueInfo, const int32 min, const int32 max) const;
    Value *                 NewIntRangeValue(const int32 min, const int32 max, const bool wasNegativeZeroPreventedByBailout, IR::Opnd *const opnd = nullptr);
    IntBoundedValueInfo *   NewIntBoundedValueInfo(const ValueInfo *const originalValueInfo, const IntBounds *const bounds) const;
    Value *                 NewIntBoundedValue(const ValueType valueType, const IntBounds *const bounds, const bool wasNegativeZeroPreventedByBailout, IR::Opnd *const opnd = nullptr);
    Value *                 NewFloatConstantValue(const FloatConstType floatValue, IR::Opnd *const opnd = nullptr);
    Value *                 GetVarConstantValue(IR::AddrOpnd *addrOpnd);
    Value *                 NewVarConstantValue(IR::AddrOpnd *addrOpnd, bool isString);
    Value *                 HoistConstantLoadAndPropagateValueBackward(Js::Var varConst, IR::Instr * origInstr, Value * value);
    Value *                 NewFixedFunctionValue(Js::JavascriptFunction *functionValue, IR::AddrOpnd *addrOpnd);

    StackSym *              GetTaggedIntConstantStackSym(const int32 intConstantValue) const;
    StackSym *              GetOrCreateTaggedIntConstantStackSym(const int32 intConstantValue) const;
    Sym *                   SetSymStore(ValueInfo *valueInfo, Sym *sym);
    void                    SetSymStoreDirect(ValueInfo *valueInfo, Sym *sym);
    IR::Instr *             TypeSpecialization(IR::Instr *instr, Value **pSrc1Val, Value **pSrc2Val, Value **pDstVal, bool *redoTypeSpecRef, bool *const forceInvariantHoistingRef);

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    bool                    TypeSpecializeSimd128(IR::Instr *instr, Value **pSrc1Val, Value **pSrc2Val, Value **pDstVal);
    bool                    Simd128DoTypeSpec(IR::Instr *instr, const Value *src1Val, const Value *src2Val, const Value *dstVal);
    bool                    Simd128DoTypeSpecLoadStore(IR::Instr *instr, const Value *src1Val, const Value *src2Val, const Value *dstVal, const ThreadContext::SimdFuncSignature *simdFuncSignature);
    bool                    Simd128CanTypeSpecOpnd(const ValueType opndType, const ValueType expectedType);
    bool                    Simd128ValidateIfLaneIndex(const IR::Instr * instr, IR::Opnd * opnd, uint argPos);
    void                    UpdateBoundCheckHoistInfoForSimd(ArrayUpperBoundCheckHoistInfo &upperHoistInfo, ValueType arrValueType, const IR::Instr *instr);    
    void                    Simd128SetIndirOpndType(IR::IndirOpnd *indirOpnd, Js::OpCode opcode);
#endif

    IRType                  GetIRTypeFromValueType(const ValueType &valueType);
    ValueType               GetValueTypeFromIRType(const IRType &type);
    IR::BailOutKind         GetBailOutKindFromValueType(const ValueType &valueType);
    IR::Instr *             GetExtendedArg(IR::Instr *instr);
    int                     GetBoundCheckOffsetForSimd(ValueType arrValueType, const IR::Instr *instr, const int oldOffset = -1);

    IR::Instr *             OptNewScObject(IR::Instr** instrPtr, Value* srcVal);
    template <typename T>
    bool                    OptConstFoldBinaryWasm(IR::Instr * *pInstr, const Value* src1, const Value* src2, Value **pDstVal);
    template <typename T>
    IR::Opnd*               ReplaceWConst(IR::Instr **pInstr, T value, Value **pDstVal);
    bool                    OptConstFoldBinary(IR::Instr * *pInstr, const IntConstantBounds &src1IntConstantBounds, const IntConstantBounds &src2IntConstantBounds, Value **pDstVal);
    bool                    OptConstFoldUnary(IR::Instr * *pInstr, const int32 intConstantValue, const bool isUsingOriginalSrc1Value, Value **pDstVal);
    bool                    OptConstPeep(IR::Instr *instr, IR::Opnd *constSrc, Value **pDstVal, ValueInfo *vInfo);
    bool                    OptConstFoldBranch(IR::Instr *instr, Value *src1Val, Value*src2Val, Value **pDstVal);
    Js::Var                 GetConstantVar(IR::Opnd *opnd, Value *val);
    bool                    IsWorthSpecializingToInt32DueToSrc(IR::Opnd *const src, Value *const val);
    bool                    IsWorthSpecializingToInt32DueToDst(IR::Opnd *const dst);
    bool                    IsWorthSpecializingToInt32(IR::Instr *const instr, Value *const src1Val, Value *const src2Val = nullptr);
    bool                    TypeSpecializeNumberUnary(IR::Instr *instr, Value *src1Val, Value **pDstVal);
    bool                    TypeSpecializeIntUnary(IR::Instr **pInstr, Value **pSrc1Val, Value **pDstVal, int32 min, int32 max, Value *const src1OriginalVal, bool *redoTypeSpecRef, bool skipDst = false);
    bool                    TypeSpecializeIntBinary(IR::Instr **pInstr, Value *src1Val, Value *src2Val, Value **pDstVal, int32 min, int32 max, bool skipDst = false);
    void                    TypeSpecializeInlineBuiltInUnary(IR::Instr **pInstr, Value **pSrc1Val, Value **pDstVal, Value *const src1OriginalVal, bool *redoTypeSpecRef);
    void                    TypeSpecializeInlineBuiltInBinary(IR::Instr **pInstr, Value *src1Val, Value* src2Val, Value **pDstVal, Value *const src1OriginalVal, Value *const src2OriginalVal);
    void                    TypeSpecializeInlineBuiltInDst(IR::Instr **pInstr, Value **pDstVal);
    bool                    TypeSpecializeUnary(IR::Instr **pInstr, Value **pSrc1Val, Value **pDstVal, Value *const src1OriginalVal, bool *redoTypeSpecRef, bool *const forceInvariantHoistingRef);
    bool                    TypeSpecializeBinary(IR::Instr **pInstr, Value **pSrc1Val, Value **pSrc2Val, Value **pDstVal, Value *const src1OriginalVal, Value *const src2OriginalVal, bool *redoTypeSpecRef);
    bool                    TypeSpecializeFloatUnary(IR::Instr **pInstr, Value *src1Val, Value **pDstVal, bool skipDst = false);
    bool                    TypeSpecializeFloatBinary(IR::Instr *instr, Value *src1Val, Value *src2Val, Value **pDstVal);
    void                    TypeSpecializeFloatDst(IR::Instr *instr, Value *valToTransfer, Value *const src1Value, Value *const src2Value, Value **pDstVal);
    bool                    TypeSpecializeLdLen(IR::Instr * *const instrRef, Value * *const src1ValueRef, Value * *const dstValueRef, bool *const forceInvariantHoistingRef);
    void                    TypeSpecializeIntDst(IR::Instr* instr, Js::OpCode originalOpCode, Value* valToTransfer, Value *const src1Value, Value *const src2Value, const IR::BailOutKind bailOutKind, int32 newMin, int32 newMax, Value** pDstVal, const AddSubConstantInfo *const addSubConstantInfo = nullptr);
    void                    TypeSpecializeIntDst(IR::Instr* instr, Js::OpCode originalOpCode, Value* valToTransfer, Value *const src1Value, Value *const src2Value, const IR::BailOutKind bailOutKind, ValueType valueType, Value** pDstVal, const AddSubConstantInfo *const addSubConstantInfo = nullptr);
    void                    TypeSpecializeIntDst(IR::Instr* instr, Js::OpCode originalOpCode, Value* valToTransfer, Value *const src1Value, Value *const src2Value, const IR::BailOutKind bailOutKind, ValueType valueType, int32 newMin, int32 newMax, Value** pDstVal, const AddSubConstantInfo *const addSubConstantInfo = nullptr);

    bool                    TryTypeSpecializeUnaryToFloatHelper(IR::Instr** pInstr, Value** pSrc1Val, Value* const src1OriginalVal, Value **pDstVal);
    bool                    TypeSpecializeBailoutExpectedInteger(IR::Instr* instr, Value* src1Val, Value** dstVal);
    bool                    TypeSpecializeStElem(IR::Instr **pInstr, Value *src1Val, Value **pDstVal);
    bool                    ShouldExpectConventionalArrayIndexValue(IR::IndirOpnd *const indirOpnd);
    ValueType               GetDivValueType(IR::Instr* instr, Value* src1Val, Value* src2Val, bool specialize);

    bool                    IsInstrInvalidForMemOp(IR::Instr *, Loop *, Value *, Value *);
    bool                    CollectMemOpInfo(IR::Instr *, IR::Instr *, Value *, Value *);
    bool                    CollectMemOpStElementI(IR::Instr *, Loop *);
    bool                    CollectMemsetStElementI(IR::Instr *, Loop *);
    bool                    CollectMemcopyStElementI(IR::Instr *, Loop *);
    bool                    CollectMemOpLdElementI(IR::Instr *, Loop *);
    bool                    CollectMemcopyLdElementI(IR::Instr *, Loop *);
    SymID                   GetVarSymID(StackSym *);
    const InductionVariable* GetInductionVariable(SymID, Loop *);
    bool                    IsSymIDInductionVariable(SymID, Loop *);
    bool                    IsAllowedForMemOpt(IR::Instr* instr, bool isMemset, IR::RegOpnd *baseOpnd, IR::Opnd *indexOpnd);

    void                    ProcessMemOp();
    bool                    InspectInstrForMemSetCandidate(Loop* loop, IR::Instr* instr, struct MemSetEmitData* emitData, bool& errorInInstr);
    bool                    InspectInstrForMemCopyCandidate(Loop* loop, IR::Instr* instr, struct MemCopyEmitData* emitData, bool& errorInInstr);
    bool                    ValidateMemOpCandidates(Loop * loop, _Out_writes_(iEmitData) struct MemOpEmitData** emitData, int& iEmitData);
    void                    EmitMemop(Loop * loop, LoopCount *loopCount, const struct MemOpEmitData* emitData);
    IR::Opnd*               GenerateInductionVariableChangeForMemOp(Loop *loop, byte unroll, IR::Instr *insertBeforeInstr = nullptr);
    IR::RegOpnd*            GenerateStartIndexOpndForMemop(Loop *loop, IR::Opnd *indexOpnd, IR::Opnd *sizeOpnd, bool isInductionVariableChangeIncremental, bool bIndexAlreadyChanged, IR::Instr *insertBeforeInstr = nullptr);
    LoopCount*              GetOrGenerateLoopCountForMemOp(Loop *loop);
    IR::Instr*              FindUpperBoundsCheckInstr(IR::Instr* instr);
    IR::Instr*              FindArraySegmentLoadInstr(IR::Instr* instr);
    void                    RemoveMemOpSrcInstr(IR::Instr* memopInstr, IR::Instr* srcInstr, BasicBlock* block);
    void                    GetMemOpSrcInfo(Loop* loop, IR::Instr* instr, IR::RegOpnd*& base, IR::RegOpnd*& index, IRType& arrayType);
    bool                    HasMemOp(Loop * loop);

private:
    void                    ChangeValueType(BasicBlock *const block, Value *const value, const ValueType newValueType, const bool preserveSubclassInfo, const bool allowIncompatibleType = false) const;
    void                    ChangeValueInfo(BasicBlock *const block, Value *const value, ValueInfo *const newValueInfo, const bool allowIncompatibleType = false, const bool compensated = false) const;
    bool                    AreValueInfosCompatible(const ValueInfo *const v0, const ValueInfo *const v1) const;

private:
#if DBG
    void                    VerifyArrayValueInfoForTracking(const ValueInfo *const valueInfo, const bool isJsArray, const BasicBlock *const block, const bool ignoreKnownImplicitCalls = false) const;
#endif
    void                    TrackNewValueForKills(Value *const value);
    void                    DoTrackNewValueForKills(Value *const value);
    void                    TrackCopiedValueForKills(Value *const value);
    void                    DoTrackCopiedValueForKills(Value *const value);
    void                    TrackMergedValueForKills(Value *const value, GlobOptBlockData *const blockData, BVSparse<JitArenaAllocator> *const mergedValueTypesTrackedForKills) const;
    void                    DoTrackMergedValueForKills(Value *const value, GlobOptBlockData *const blockData, BVSparse<JitArenaAllocator> *const mergedValueTypesTrackedForKills) const;
    void                    TrackValueInfoChangeForKills(BasicBlock *const block, Value *const value, ValueInfo *const newValueInfo, const bool compensated) const;
    void                    ProcessValueKills(IR::Instr *const instr);
    void                    ProcessValueKills(BasicBlock *const block, GlobOptBlockData *const blockData);
    void                    ProcessValueKillsForLoopHeaderAfterBackEdgeMerge(BasicBlock *const block, GlobOptBlockData *const blockData);
    bool                    NeedBailOnImplicitCallForLiveValues(BasicBlock const * const block, const bool isForwardPass) const;
    IR::Instr*              CreateBoundsCheckInstr(IR::Opnd* lowerBound, IR::Opnd* upperBound, int offset, Func* func);
    IR::Instr*              CreateBoundsCheckInstr(IR::Opnd* lowerBound, IR::Opnd* upperBound, int offset, IR::BailOutKind bailoutkind, BailOutInfo* bailoutInfo, Func* func);
    IR::Instr*              AttachBoundsCheckData(IR::Instr* instr, IR::Opnd* lowerBound, IR::Opnd* upperBound, int offset);
    void                    OptArraySrc(IR::Instr * *const instrRef);

private:
    void                    TrackIntSpecializedAddSubConstant(IR::Instr *const instr, const AddSubConstantInfo *const addSubConstantInfo, Value *const dstValue, const bool updateSourceBounds);
    void                    CloneBoundCheckHoistBlockData(BasicBlock *const toBlock, GlobOptBlockData *const toData, BasicBlock *const fromBlock, GlobOptBlockData *const fromData);
    void                    MergeBoundCheckHoistBlockData(BasicBlock *const toBlock, GlobOptBlockData *const toData, BasicBlock *const fromBlock, GlobOptBlockData *const fromData);
    void                    DetectUnknownChangesToInductionVariables(GlobOptBlockData *const blockData);
    void                    SetInductionVariableValueNumbers(GlobOptBlockData *const blockData);
    void                    FinalizeInductionVariables(Loop *const loop, GlobOptBlockData *const headerData);
    enum class SymBoundType {OFFSET, VALUE, UNKNOWN};
    SymBoundType DetermineSymBoundOffsetOrValueRelativeToLandingPad(StackSym *const sym, const bool landingPadValueIsLowerBound, ValueInfo *const valueInfo, const IntBounds *const bounds, GlobOptBlockData *const landingPadGlobOptBlockData, int *const boundOffsetOrValueRef);

private:
    void                    DetermineDominatingLoopCountableBlock(Loop *const loop, BasicBlock *const headerBlock);
    void                    DetermineLoopCount(Loop *const loop);
    void                    GenerateLoopCount(Loop *const loop, LoopCount *const loopCount);
    void                    GenerateLoopCountPlusOne(Loop *const loop, LoopCount *const loopCount);
    void                    GenerateSecondaryInductionVariableBound(Loop *const loop, StackSym *const inductionVariableSym, const LoopCount *const loopCount, const int maxMagnitudeChange, StackSym *const boundSym);

private:
    void                    DetermineArrayBoundCheckHoistability(bool needLowerBoundCheck, bool needUpperBoundCheck, ArrayLowerBoundCheckHoistInfo &lowerHoistInfo, ArrayUpperBoundCheckHoistInfo &upperHoistInfo, const bool isJsArray, StackSym *const indexSym, Value *const indexValue, const IntConstantBounds &indexConstantBounds, StackSym *const headSegmentLengthSym, Value *const headSegmentLengthValue, const IntConstantBounds &headSegmentLengthConstantBounds, Loop *const headSegmentLengthInvariantLoop, bool &failedToUpdateCompatibleLowerBoundCheck, bool &failedToUpdateCompatibleUpperBoundCheck);

private:
    void                    CaptureNoImplicitCallUses(IR::Opnd *opnd, const bool usesNoMissingValuesInfo, IR::Instr *const includeCurrentInstr = nullptr);
    void                    InsertNoImplicitCallUses(IR::Instr *const instr);
    void                    PrepareLoopArrayCheckHoist();

public:
    JsArrayKills            CheckJsArrayKills(IR::Instr *const instr);

    GlobOptBlockData const * CurrentBlockData() const;
    GlobOptBlockData * CurrentBlockData();

private:
    bool                    IsOperationThatLikelyKillsJsArraysWithNoMissingValues(IR::Instr *const instr);
    bool                    NeedBailOnImplicitCallForArrayCheckHoist(BasicBlock const * const block, const bool isForwardPass) const;

private:
    bool                    PrepareForIgnoringIntOverflow(IR::Instr *const instr);
    void                    VerifyIntSpecForIgnoringIntOverflow(IR::Instr *const instr);

    void                    PreLowerCanonicalize(IR::Instr *instr, Value **pSrc1Val, Value **pSrc2Val);
    void                    ProcessKills(IR::Instr *instr);
    void                    InsertCloneStrs(BasicBlock *toBlock, GlobOptBlockData *toData, GlobOptBlockData *fromData);
    void                    InsertValueCompensation(BasicBlock *const predecessor, const SymToValueInfoMap &symsRequiringCompensationToMergedValueInfoMap);
    IR::Instr *             ToVarUses(IR::Instr *instr, IR::Opnd *opnd, bool isDst, Value *val);
    void                    ToVar(BVSparse<JitArenaAllocator> *bv, BasicBlock *block);
    IR::Instr *             ToVar(IR::Instr *instr, IR::RegOpnd *regOpnd, BasicBlock *block, Value *val, bool needsUpdate);
    void                    ToInt32(BVSparse<JitArenaAllocator> *bv, BasicBlock *block, bool lossy, IR::Instr *insertBeforeInstr = nullptr);
    void                    ToFloat64(BVSparse<JitArenaAllocator> *bv, BasicBlock *block);
    void                    ToTypeSpec(BVSparse<JitArenaAllocator> *bv, BasicBlock *block, IRType toType, IR::BailOutKind bailOutKind = IR::BailOutInvalid, bool lossy = false, IR::Instr *insertBeforeInstr = nullptr);
    IR::Instr *             ToInt32(IR::Instr *instr, IR::Opnd *opnd, BasicBlock *block, Value *val, IR::IndirOpnd *indir, bool lossy);
    IR::Instr *             ToFloat64(IR::Instr *instr, IR::Opnd *opnd, BasicBlock *block, Value *val, IR::IndirOpnd *indir, IR::BailOutKind bailOutKind);
    IR::Instr *             ToTypeSpecUse(IR::Instr *instr, IR::Opnd *opnd, BasicBlock *block, Value *val, IR::IndirOpnd *indir,
        IRType toType, IR::BailOutKind bailOutKind, bool lossy = false, IR::Instr *insertBeforeInstr = nullptr);
    void                    ToVarRegOpnd(IR::RegOpnd *dst, BasicBlock *block);
    void                    ToVarStackSym(StackSym *varSym, BasicBlock *block);
    void                    ToInt32Dst(IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block);
    void                    ToUInt32Dst(IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block);
    void                    ToFloat64Dst(IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block);

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    void                    TypeSpecializeSimd128Dst(IRType type, IR::Instr *instr, Value *valToTransfer, Value *const src1Value, Value **pDstVal);
    void                    ToSimd128Dst(IRType toType, IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block);
#endif

    void                    OptConstFoldBr(bool test, IR::Instr *instr, Value * intTypeSpecSrc1Val = nullptr, Value * intTypeSpecSrc2Val = nullptr);
    void                    PropagateIntRangeForNot(int32 minimum, int32 maximum, int32 *pNewMin, int32 * pNewMax);
    void                    PropagateIntRangeBinary(IR::Instr *instr, int32 min1, int32 max1,
                            int32 min2, int32 max2, int32 *pNewMin, int32 *pNewMax);
    bool                    OptIsInvariant(IR::Opnd *src, BasicBlock *block, Loop *loop, Value *srcVal, bool isNotTypeSpecConv, bool allowNonPrimitives);
    bool                    OptIsInvariant(Sym *sym, BasicBlock *block, Loop *loop, Value *srcVal, bool isNotTypeSpecConv, bool allowNonPrimitives, Value **loopHeadValRef = nullptr);
    bool                    OptDstIsInvariant(IR::RegOpnd *dst);
    bool                    OptIsInvariant(IR::Instr *instr, BasicBlock *block, Loop *loop, Value *src1Val, Value *src2Val, bool isNotTypeSpecConv, const bool forceInvariantHoisting = false);
    void                    OptHoistInvariant(IR::Instr *instr, BasicBlock *block, Loop *loop, Value *dstVal, Value *const src1Val, Value *const src2Value,
                                                bool isNotTypeSpecConv, bool lossy = false, IR::BailOutKind bailoutKind = IR::BailOutInvalid);
    bool                    TryHoistInvariant(IR::Instr *instr, BasicBlock *block, Value *dstVal, Value *src1Val, Value *src2Val, bool isNotTypeSpecConv,
                                                const bool lossy = false, const bool forceInvariantHoisting = false, IR::BailOutKind bailoutKind = IR::BailOutInvalid);
    void                    HoistInvariantValueInfo(ValueInfo *const invariantValueInfoToHoist, Value *const valueToUpdate, BasicBlock *const targetBlock);
    void                    OptHoistUpdateValueType(Loop* loop, IR::Instr* instr, IR::Opnd* srcOpnd, Value *const srcVal);
public:
    static bool             IsTypeSpecPhaseOff(Func const * func);
    static bool             DoAggressiveIntTypeSpec(Func const * func);
    static bool             DoLossyIntTypeSpec(Func const * func);
    static bool             DoFloatTypeSpec(Func const * func);
    static bool             DoStringTypeSpec(Func const * func);
    static bool             DoArrayCheckHoist(Func const * const func);
    static bool             DoArrayMissingValueCheckHoist(Func const * const func);
    static bool             DoArraySegmentHoist(const ValueType baseValueType, Func const * const func);
    static bool             DoArrayLengthHoist(Func const * const func);
    static bool             DoEliminateArrayAccessHelperCall(Func* func);
    static bool             DoTypedArrayTypeSpec(Func const * func);
    static bool             DoNativeArrayTypeSpec(Func const * func);
    static bool             IsSwitchOptEnabled(Func const * func);
    static bool             DoInlineArgsOpt(Func const * func);
    static bool             IsPREInstrCandidateLoad(Js::OpCode opcode);
    static bool             IsPREInstrCandidateStore(Js::OpCode opcode);
    static bool             ImplicitCallFlagsAllowOpts(Loop * loop);
    static bool             ImplicitCallFlagsAllowOpts(Func const * func);

private:
    bool                    DoConstFold() const;
    bool                    DoTypeSpec() const;
    bool                    DoAggressiveIntTypeSpec() const;
    bool                    DoAggressiveMulIntTypeSpec() const;
    bool                    DoDivIntTypeSpec() const;
    bool                    DoLossyIntTypeSpec() const;
    bool                    DoFloatTypeSpec() const;
    bool                    DoStringTypeSpec() const { return GlobOpt::DoStringTypeSpec(this->func); }
    bool                    DoArrayCheckHoist() const;
    bool                    DoArrayCheckHoist(const ValueType baseValueType, Loop* loop, IR::Instr const * const instr = nullptr) const;
    bool                    DoArrayMissingValueCheckHoist() const;
    bool                    DoArraySegmentHoist(const ValueType baseValueType) const;
    bool                    DoTypedArraySegmentLengthHoist(Loop *const loop) const;
    bool                    DoArrayLengthHoist() const;
    bool                    DoEliminateArrayAccessHelperCall() const;
    bool                    DoTypedArrayTypeSpec() const { return GlobOpt::DoTypedArrayTypeSpec(this->func); }
    bool                    DoNativeArrayTypeSpec() const { return GlobOpt::DoNativeArrayTypeSpec(this->func); }
    bool                    DoLdLenIntSpec(IR::Instr * const instr, const ValueType baseValueType);
    bool                    IsSwitchOptEnabled() const { return GlobOpt::IsSwitchOptEnabled(this->func); }
    bool                    DoPathDependentValues() const;
    bool                    DoTrackRelativeIntBounds() const;
    bool                    DoBoundCheckElimination() const;
    bool                    DoBoundCheckHoist() const;
    bool                    DoLoopCountBasedBoundCheckHoist() const;
    bool                    DoPowIntIntTypeSpec() const;
    bool                    DoTagChecks() const;

private:
    // GlobOptBailout.cpp
    bool                    MayNeedBailOut(Loop * loop) const;
    static void             TrackByteCodeSymUsed(IR::Opnd * opnd, BVSparse<JitArenaAllocator> * instrByteCodeStackSymUsed, PropertySym **pPropertySymUse);
    static void             TrackByteCodeSymUsed(IR::RegOpnd * opnd, BVSparse<JitArenaAllocator> * instrByteCodeStackSymUsed);
    static void             TrackByteCodeSymUsed(StackSym * sym, BVSparse<JitArenaAllocator> * instrByteCodeStackSymUsed);
    void                    CaptureValues(BasicBlock *block, BailOutInfo * bailOutInfo);
    void                    CaptureValuesFromScratch(
                                BasicBlock * block,
                                SListBase<ConstantStackSymValue>::EditingIterator & bailOutConstValuesIter,
                                SListBase<CopyPropSyms>::EditingIterator & bailOutCopyPropIter);
    void                    CaptureValuesIncremental(
                                BasicBlock * block,
                                SListBase<ConstantStackSymValue>::EditingIterator & bailOutConstValuesIter,
                                SListBase<CopyPropSyms>::EditingIterator & bailOutCopyPropIter);
    void                    CaptureCopyPropValue(BasicBlock * block, Sym * sym, Value * val, SListBase<CopyPropSyms>::EditingIterator & bailOutCopySymsIter);
    void                    CaptureArguments(BasicBlock *block, BailOutInfo * bailOutInfo, JitArenaAllocator *allocator);
    void                    CaptureByteCodeSymUses(IR::Instr * instr);
    IR::ByteCodeUsesInstr * InsertByteCodeUses(IR::Instr * instr, bool includeDef = false);
    void                    TrackCalls(IR::Instr * instr);
    void                    RecordInlineeFrameInfo(IR::Instr* instr);
    void                    EndTrackCall(IR::Instr * instr);
    void                    EndTrackingOfArgObjSymsForInlinee();
    void                    FillBailOutInfo(BasicBlock *block, BailOutInfo *bailOutInfo);

    static void             MarkNonByteCodeUsed(IR::Instr * instr);
    static void             MarkNonByteCodeUsed(IR::Opnd * opnd);

    bool                    IsImplicitCallBailOutCurrentlyNeeded(IR::Instr * instr, Value const * src1Val, Value const * src2Val) const;
    bool                    IsImplicitCallBailOutCurrentlyNeeded(IR::Instr * instr, Value const * src1Val, Value const * src2Val, BasicBlock const * block, bool hasLiveFields, bool mayNeedImplicitCallBailOut, bool isForwardPass) const;
    static bool             IsTypeCheckProtected(const IR::Instr * instr);
    static bool             MayNeedBailOnImplicitCall(IR::Instr const * instr, Value const * src1Val, Value const * src2Val);
    static bool             MaySrcNeedBailOnImplicitCall(IR::Opnd const * opnd, Value const * val);

    void                    GenerateBailAfterOperation(IR::Instr * *const pInstr, IR::BailOutKind kind);
public:
    void                    GenerateBailAtOperation(IR::Instr * *const pInstr, const IR::BailOutKind bailOutKind);

private:
    IR::Instr *             EnsureBailTarget(Loop * loop);

    // GlobOptFields.cpp
    void                    ProcessFieldKills(IR::Instr * instr);
    void                    KillLiveFields(StackSym * stackSym, BVSparse<JitArenaAllocator> * bv);
    void                    KillLiveFields(PropertySym * propertySym, BVSparse<JitArenaAllocator> * bv);
    void                    KillLiveFields(BVSparse<JitArenaAllocator> *const fieldsToKill, BVSparse<JitArenaAllocator> *const bv) const;
    void                    KillLiveElems(IR::IndirOpnd * indirOpnd, BVSparse<JitArenaAllocator> * bv, bool inGlobOpt, Func *func);
    void                    KillAllFields(BVSparse<JitArenaAllocator> * bv);
    void                    SetAnyPropertyMayBeWrittenTo();
    void                    AddToPropertiesWrittenTo(Js::PropertyId propertyId);

    bool                    DoFieldCopyProp() const;
    bool                    DoFieldCopyProp(Loop * loop) const;
    bool                    DoFunctionFieldCopyProp() const;

    bool                    DoFieldHoisting() const;
    bool                    DoObjTypeSpec() const;
    bool                    DoObjTypeSpec(Loop * loop) const;
    bool                    DoFieldRefOpts() const { return DoObjTypeSpec(); }
    bool                    DoFieldRefOpts(Loop * loop) const { return DoObjTypeSpec(loop); }
    bool                    DoFieldOpts(Loop * loop) const;
    bool                    DoFieldPRE() const;
    bool                    DoFieldPRE(Loop *loop) const;

    bool                    FieldHoistOptSrc(IR::Opnd *opnd, IR::Instr *instr, PropertySym * propertySym);
    void                    FieldHoistOptDst(IR::Instr * instr, PropertySym * propertySym, Value * src1Val);

    bool                    TrackHoistableFields() const;
    void                    PreparePrepassFieldHoisting(Loop * loop);
    void                    PrepareFieldHoisting(Loop * loop);
    void                    CheckFieldHoistCandidate(IR::Instr * instr, PropertySym * sym);
    Loop *                  FindFieldHoistStackSym(Loop * startLoop, SymID propertySymId, StackSym ** copySym, IR::Instr * instrToHoist = nullptr) const;
    bool                    CopyPropHoistedFields(PropertySym * sym, IR::Opnd ** ppOpnd, IR::Instr * instr);
    void                    HoistFieldLoad(PropertySym * sym, Loop * loop, IR::Instr * instr, Value * oldValue, Value * newValue);
    void                    HoistNewFieldLoad(PropertySym * sym, Loop * loop, IR::Instr * instr, Value * oldValue, Value * newValue);
    void                    GenerateHoistFieldLoad(PropertySym * sym, Loop * loop, IR::Instr * instr, StackSym * newStackSym, Value * oldValue, Value * newValue);
    void                    HoistFieldLoadValue(Loop * loop, Value * newValue, SymID symId, Js::OpCode opcode, IR::Opnd * srcOpnd);
    void                    ReloadFieldHoistStackSym(IR::Instr * instr, PropertySym * propertySym);
    void                    CopyStoreFieldHoistStackSym(IR::Instr * storeFldInstr, PropertySym * sym, Value * src1Val);
    Value *                 CreateFieldSrcValue(PropertySym * sym, PropertySym * originalSym, IR::Opnd **ppOpnd, IR::Instr * instr);

    static bool             HasHoistableFields(BasicBlock const * block);
    static bool             HasHoistableFields(GlobOptBlockData const * globOptData);
    bool                    IsHoistablePropertySym(SymID symId) const;
    bool                    NeedBailOnImplicitCallWithFieldOpts(Loop *loop, bool hasLiveFields) const;
    IR::Instr *             EnsureDisableImplicitCallRegion(Loop * loop);
    void                    UpdateObjPtrValueType(IR::Opnd * opnd, IR::Instr * instr);

    bool                    TrackArgumentsObject();
    void                    CannotAllocateArgumentsObjectOnStack();

#if DBG
    bool                    IsPropertySymId(SymID symId) const;
    bool                    IsHoistedPropertySym(PropertySym * sym) const;
    bool                    IsHoistedPropertySym(SymID symId, Loop * loop) const;

    static void             AssertCanCopyPropOrCSEFieldLoad(IR::Instr * instr);
#endif

    StackSym *              EnsureObjectTypeSym(StackSym * objectSym);
    PropertySym *           EnsurePropertyWriteGuardSym(PropertySym * propertySym);
    void                    PreparePropertySymForTypeCheckSeq(PropertySym *propertySym);
    bool                    IsPropertySymPreparedForTypeCheckSeq(PropertySym *propertySym);
    bool                    PreparePropertySymOpndForTypeCheckSeq(IR::PropertySymOpnd *propertySymOpnd, IR::Instr * instr, Loop *loop);
    static bool             AreTypeSetsIdentical(Js::EquivalentTypeSet * leftTypeSet, Js::EquivalentTypeSet * rightTypeSet);
    static bool             IsSubsetOf(Js::EquivalentTypeSet * leftTypeSet, Js::EquivalentTypeSet * rightTypeSet);
    static bool             CompareCurrentTypesWithExpectedTypes(JsTypeValueInfo *valueInfo, IR::PropertySymOpnd * propertySymOpnd);
    bool                    ProcessPropOpInTypeCheckSeq(IR::Instr* instr, IR::PropertySymOpnd *opnd);
    bool                    CheckIfInstrInTypeCheckSeqEmitsTypeCheck(IR::Instr* instr, IR::PropertySymOpnd *opnd);
    template<bool makeChanges>
    bool                    ProcessPropOpInTypeCheckSeq(IR::Instr* instr, IR::PropertySymOpnd *opnd, BasicBlock* block, bool updateExistingValue, bool* emitsTypeCheckOut = nullptr, bool* changesTypeValueOut = nullptr, bool *isObjTypeChecked = nullptr);
    void                    KillObjectHeaderInlinedTypeSyms(BasicBlock *block, bool isObjTypeSpecialized, SymID symId = (SymID)-1);
    void                    ValueNumberObjectType(IR::Opnd *dstOpnd, IR::Instr *instr);
    void                    SetSingleTypeOnObjectTypeValue(Value* value, const JITTypeHolder type);
    void                    SetTypeSetOnObjectTypeValue(Value* value, Js::EquivalentTypeSet* typeSet);
    void                    UpdateObjectTypeValue(Value* value, const JITTypeHolder type, bool setType, Js::EquivalentTypeSet* typeSet, bool setTypeSet);
    void                    SetObjectTypeFromTypeSym(StackSym *typeSym, Value* value, BasicBlock* block = nullptr);
    void                    SetObjectTypeFromTypeSym(StackSym *typeSym, const JITTypeHolder type, Js::EquivalentTypeSet * typeSet, BasicBlock* block = nullptr, bool updateExistingValue = false);
    void                    SetObjectTypeFromTypeSym(StackSym *typeSym, const JITTypeHolder type, Js::EquivalentTypeSet * typeSet, GlobOptBlockData *blockData, bool updateExistingValue = false);
    void                    KillObjectType(StackSym *objectSym, BVSparse<JitArenaAllocator>* liveFields = nullptr);
    void                    KillAllObjectTypes(BVSparse<JitArenaAllocator>* liveFields = nullptr);
    void                    EndFieldLifetime(IR::SymOpnd *symOpnd);
    PropertySym *           CopyPropPropertySymObj(IR::SymOpnd *opnd, IR::Instr *instr);
    static bool             NeedsTypeCheckBailOut(const IR::Instr *instr, IR::PropertySymOpnd *propertySymOpnd, bool isStore, bool* pIsTypeCheckProtected, IR::BailOutKind *pBailOutKind);
    IR::Instr *             PreOptPeep(IR::Instr *instr);
    IR::Instr *             OptPeep(IR::Instr *instr, Value *src1Val, Value *src2Val);
    void                    OptimizeIndirUses(IR::IndirOpnd *indir, IR::Instr * *pInstr, Value **indirIndexValRef);
    void                    RemoveCodeAfterNoFallthroughInstr(IR::Instr *instr);
    void                    ProcessTryHandler(IR::Instr* instr);
    bool                    ProcessExceptionHandlingEdges(IR::Instr* instr);
    void                    InsertToVarAtDefInTryRegion(IR::Instr * instr, IR::Opnd * dstOpnd);
    void                    RemoveFlowEdgeToCatchBlock(IR::Instr * instr);
    bool                    RemoveFlowEdgeToFinallyOnExceptionBlock(IR::Instr * instr);

    void                    CSEAddInstr(BasicBlock *block, IR::Instr *instr, Value *dstVal, Value *src1Val, Value *src2Val, Value *dstIndirIndexVal, Value *src1IndirIndexVal);
    void                    OptimizeChecks(IR::Instr * const instr);
    bool                    CSEOptimize(BasicBlock *block, IR::Instr * *const instrRef, Value **pSrc1Val, Value **pSrc2Val, Value **pSrc1IndirIndexVal, bool intMathExprOnly = false);
    bool                    GetHash(IR::Instr *instr, Value *src1Val, Value *src2Val, ExprAttributes exprAttributes, ExprHash *pHash);
    void                    ProcessArrayValueKills(IR::Instr *instr);
    static bool             NeedBailOnImplicitCallForCSE(BasicBlock const *block, bool isForwardPass);
    bool                    DoCSE();
    bool                    CanCSEArrayStore(IR::Instr *instr);

#if DBG_DUMP
    void                    Dump() const;
    void                    DumpSymToValueMap() const;
    void                    DumpSymToValueMap(BasicBlock const * block) const;
    void                    DumpSymVal(int index);

    void                    Trace(BasicBlock * basicBlock, bool before) const;
    void                    TraceSettings() const;
#endif

    bool                    IsWorthSpecializingToInt32Branch(IR::Instr const * instr, Value const * src1Val, Value const * src2Val) const;

    bool                    TryOptConstFoldBrFalse(IR::Instr *const instr, Value *const srcValue, const int32 min, const int32 max);
    bool                    TryOptConstFoldBrEqual(IR::Instr *const instr, const bool branchOnEqual, Value *const src1Value, const int32 min1, const int32 max1, Value *const src2Value, const int32 min2, const int32 max2);
    bool                    TryOptConstFoldBrGreaterThan(IR::Instr *const instr, const bool branchOnGreaterThan, Value *const src1Value, const int32 min1, const int32 max1, Value *const src2Value, const int32 min2, const int32 max2);
    bool                    TryOptConstFoldBrGreaterThanOrEqual(IR::Instr *const instr, const bool branchOnGreaterThanOrEqual, Value *const src1Value, const int32 min1, const int32 max1, Value *const src2Value, const int32 min2, const int32 max2);
    bool                    TryOptConstFoldBrUnsignedLessThan(IR::Instr *const instr, const bool branchOnLessThan, Value *const src1Value, const int32 min1, const int32 max1, Value *const src2Value, const int32 min2, const int32 max2);
    bool                    TryOptConstFoldBrUnsignedGreaterThan(IR::Instr *const instr, const bool branchOnGreaterThan, Value *const src1Value, const int32 min1, const int32 max1, Value *const src2Value, const int32 min2, const int32 max2);

    void                    UpdateIntBoundsForEqualBranch(Value *const src1Value, Value *const src2Value, const int32 src2ConstantValue = 0);
    void                    UpdateIntBoundsForNotEqualBranch(Value *const src1Value, Value *const src2Value, const int32 src2ConstantValue = 0);
    void                    UpdateIntBoundsForGreaterThanOrEqualBranch(Value *const src1Value, Value *const src2Value);
    void                    UpdateIntBoundsForGreaterThanBranch(Value *const src1Value, Value *const src2Value);
    void                    UpdateIntBoundsForLessThanOrEqualBranch(Value *const src1Value, Value *const src2Value);
    void                    UpdateIntBoundsForLessThanBranch(Value *const src1Value, Value *const src2Value);

    IntBounds *             GetIntBoundsToUpdate(const ValueInfo *const valueInfo, const IntConstantBounds &constantBounds, const bool isSettingNewBound, const bool isBoundConstant, const bool isSettingUpperBound, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForEqual(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForNotEqual(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForGreaterThanOrEqual(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForGreaterThanOrEqual(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const int boundOffset, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForGreaterThan(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForLessThanOrEqual(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForLessThanOrEqual(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const int boundOffset, const bool isExplicit);
    ValueInfo *             UpdateIntBoundsForLessThan(Value *const value, const IntConstantBounds &constantBounds, Value *const boundValue, const IntConstantBounds &boundConstantBounds, const bool isExplicit);

    void                    SetPathDependentInfo(const bool conditionToBranch, const PathDependentInfo &info);
    PathDependentInfoToRestore UpdatePathDependentInfo(PathDependentInfo *const info);
    void                    RestorePathDependentInfo(PathDependentInfo *const info, const PathDependentInfoToRestore infoToRestore);

    IR::Instr *             TrackMarkTempObject(IR::Instr * instrStart, IR::Instr * instrEnd);
    void                    TrackTempObjectSyms(IR::Instr * instr, IR::RegOpnd * opnd);
    IR::Instr *             GenerateBailOutMarkTempObjectIfNeeded(IR::Instr * instr, IR::Opnd * opnd, bool isDst);

    friend class InvariantBlockBackwardIterator;
};
