//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

#if ENABLE_DEBUG_CONFIG_OPTIONS

#define TESTTRACE_PHASE_INSTR(phase, instr, ...) \
    if(PHASE_TESTTRACE(phase, this->func)) \
    { \
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]; \
        Output::Print( \
            _u("Testtrace: %s function %s (%s): "), \
            Js::PhaseNames[phase], \
            instr->m_func->GetJITFunctionBody()->GetDisplayName(), \
            instr->m_func->GetDebugNumberSet(debugStringBuffer)); \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#else // ENABLE_DEBUG_CONFIG_OPTIONS

#define TESTTRACE_PHASE_INSTR(phase, instr, ...)

#endif // ENABLE_DEBUG_CONFIG_OPTIONS

#if DBG_DUMP
#define DO_MEMOP_TRACE() (PHASE_TRACE(Js::MemOpPhase, this->func) ||\
        PHASE_TRACE(Js::MemSetPhase, this->func) ||\
        PHASE_TRACE(Js::MemCopyPhase, this->func))
#define DO_MEMOP_TRACE_PHASE(phase) (PHASE_TRACE(Js::MemOpPhase, this->func) || PHASE_TRACE(Js::phase ## Phase, this->func))

#define OUTPUT_MEMOP_TRACE(loop, instr, ...) {\
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];\
    Output::Print(15, _u("Function: %s%s, Loop: %u: "), this->func->GetJITFunctionBody()->GetDisplayName(), this->func->GetDebugNumberSet(debugStringBuffer), loop->GetLoopNumber());\
    Output::Print(__VA_ARGS__);\
    IR::Instr* __instr__ = instr;\
    if(__instr__) __instr__->DumpByteCodeOffset();\
    if(__instr__) Output::Print(_u(" (%s)"), Js::OpCodeUtil::GetOpCodeName(__instr__->m_opcode));\
    Output::Print(_u("\n"));\
    Output::Flush(); \
}
#define TRACE_MEMOP(loop, instr, ...) \
    if (DO_MEMOP_TRACE()) {\
        Output::Print(_u("TRACE MemOp:"));\
        OUTPUT_MEMOP_TRACE(loop, instr, __VA_ARGS__)\
    }
#define TRACE_MEMOP_VERBOSE(loop, instr, ...) if(CONFIG_FLAG(Verbose)) {TRACE_MEMOP(loop, instr, __VA_ARGS__)}

#define TRACE_MEMOP_PHASE(phase, loop, instr, ...) \
    if (DO_MEMOP_TRACE_PHASE(phase))\
    {\
        Output::Print(_u("TRACE ") _u(#phase) _u(":"));\
        OUTPUT_MEMOP_TRACE(loop, instr, __VA_ARGS__)\
    }
#define TRACE_MEMOP_PHASE_VERBOSE(phase, loop, instr, ...) if(CONFIG_FLAG(Verbose)) {TRACE_MEMOP_PHASE(phase, loop, instr, __VA_ARGS__)}

#else
#define DO_MEMOP_TRACE()
#define DO_MEMOP_TRACE_PHASE(phase)
#define OUTPUT_MEMOP_TRACE(loop, instr, ...)
#define TRACE_MEMOP(loop, instr, ...)
#define TRACE_MEMOP_VERBOSE(loop, instr, ...)
#define TRACE_MEMOP_PHASE(phase, loop, instr, ...)
#define TRACE_MEMOP_PHASE_VERBOSE(phase, loop, instr, ...)
#endif

class AutoRestoreVal
{
private:
    Value *const originalValue;
    Value *const tempValue;
    Value * *const valueRef;

public:
    AutoRestoreVal(Value *const originalValue, Value * *const tempValueRef)
        : originalValue(originalValue), tempValue(*tempValueRef), valueRef(tempValueRef)
    {
    }

    ~AutoRestoreVal()
    {
        if(*valueRef == tempValue)
        {
            *valueRef = originalValue;
        }
    }

    PREVENT_COPY(AutoRestoreVal);
};

GlobOpt::GlobOpt(Func * func)
    : func(func),
    intConstantToStackSymMap(nullptr),
    intConstantToValueMap(nullptr),
    currentValue(FirstNewValueNumber),
    prePassLoop(nullptr),
    alloc(nullptr),
    isCallHelper(false),
    inInlinedBuiltIn(false),
    rootLoopPrePass(nullptr),
    noImplicitCallUsesToInsert(nullptr),
    valuesCreatedForClone(nullptr),
    valuesCreatedForMerge(nullptr),
    instrCountSinceLastCleanUp(0),
    isRecursiveCallOnLandingPad(false),
    updateInductionVariableValueNumber(false),
    isPerformingLoopBackEdgeCompensation(false),
    currentRegion(nullptr),
    changedSymsAfterIncBailoutCandidate(nullptr),
    doTypeSpec(
        !IsTypeSpecPhaseOff(func)),
    doAggressiveIntTypeSpec(
        doTypeSpec &&
        DoAggressiveIntTypeSpec(func)),
    doAggressiveMulIntTypeSpec(
        doTypeSpec &&
        !PHASE_OFF(Js::AggressiveMulIntTypeSpecPhase, func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsAggressiveMulIntTypeSpecDisabled(func->IsLoopBody()))),
    doDivIntTypeSpec(
        doAggressiveIntTypeSpec &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsDivIntTypeSpecDisabled(func->IsLoopBody()))),
    doLossyIntTypeSpec(
        doTypeSpec &&
        DoLossyIntTypeSpec(func)),
    doFloatTypeSpec(
        doTypeSpec &&
        DoFloatTypeSpec(func)),
    doArrayCheckHoist(
        DoArrayCheckHoist(func)),
    doArrayMissingValueCheckHoist(
        doArrayCheckHoist &&
        DoArrayMissingValueCheckHoist(func)),
    doArraySegmentHoist(
        doArrayCheckHoist &&
        DoArraySegmentHoist(ValueType::GetObject(ObjectType::Int32Array), func)),
    doJsArraySegmentHoist(
        doArraySegmentHoist &&
        DoArraySegmentHoist(ValueType::GetObject(ObjectType::Array), func)),
    doArrayLengthHoist(
        doArrayCheckHoist &&
        DoArrayLengthHoist(func)),
    doEliminateArrayAccessHelperCall(
        doArrayCheckHoist &&
        !PHASE_OFF(Js::EliminateArrayAccessHelperCallPhase, func)),
    doTrackRelativeIntBounds(
        doAggressiveIntTypeSpec &&
        DoPathDependentValues() &&
        !PHASE_OFF(Js::Phase::TrackRelativeIntBoundsPhase, func)),
    doBoundCheckElimination(
        doTrackRelativeIntBounds &&
        !PHASE_OFF(Js::Phase::BoundCheckEliminationPhase, func)),
    doBoundCheckHoist(
        doEliminateArrayAccessHelperCall &&
        doBoundCheckElimination &&
        DoConstFold() &&
        !PHASE_OFF(Js::Phase::BoundCheckHoistPhase, func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsBoundCheckHoistDisabled(func->IsLoopBody()))),
    doLoopCountBasedBoundCheckHoist(
        doBoundCheckHoist &&
        !PHASE_OFF(Js::Phase::LoopCountBasedBoundCheckHoistPhase, func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsLoopCountBasedBoundCheckHoistDisabled(func->IsLoopBody()))),
    doPowIntIntTypeSpec(
        doAggressiveIntTypeSpec &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsPowIntIntTypeSpecDisabled())),
    doTagChecks(
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsTagCheckDisabled())),
    isAsmJSFunc(func->GetJITFunctionBody()->IsAsmJsMode())
{
}

void
GlobOpt::BackwardPass(Js::Phase tag)
{
    BEGIN_CODEGEN_PHASE(this->func, tag);

    ::BackwardPass backwardPass(this->func, this, tag);
    backwardPass.Optimize();

    END_CODEGEN_PHASE(this->func, tag);
}

void
GlobOpt::Optimize()
{
    this->objectTypeSyms = nullptr;
    this->func->argInsCount = this->func->GetInParamsCount() - 1;   //Don't include "this" pointer in the count.

    if (!func->DoGlobOpt())
    {
        this->lengthEquivBv = nullptr;
        this->argumentsEquivBv = nullptr;
        this->callerEquivBv = nullptr;

        // Still need to run the dead store phase to calculate the live reg on back edge
        this->BackwardPass(Js::DeadStorePhase);
        CannotAllocateArgumentsObjectOnStack();
        return;
    }

    {
        this->lengthEquivBv = this->func->m_symTable->m_propertyEquivBvMap->Lookup(Js::PropertyIds::length, nullptr); // Used to kill live "length" properties
        this->argumentsEquivBv = func->m_symTable->m_propertyEquivBvMap->Lookup(Js::PropertyIds::arguments, nullptr); // Used to kill live "arguments" properties
        this->callerEquivBv = func->m_symTable->m_propertyEquivBvMap->Lookup(Js::PropertyIds::caller, nullptr); // Used to kill live "caller" properties

        // The backward phase needs the glob opt's allocator to allocate the propertyTypeValueMap
        // in GlobOpt::EnsurePropertyTypeValue and ranges of instructions where int overflow may be ignored.
        // (see BackwardPass::TrackIntUsage)
        PageAllocator * pageAllocator = this->func->m_alloc->GetPageAllocator();
        NoRecoverMemoryJitArenaAllocator localAlloc(_u("BE-GlobOpt"), pageAllocator, Js::Throw::OutOfMemory);
        this->alloc = &localAlloc;

        NoRecoverMemoryJitArenaAllocator localTempAlloc(_u("BE-GlobOpt temp"), pageAllocator, Js::Throw::OutOfMemory);
        this->tempAlloc = &localTempAlloc;

        // The forward passes use info (upwardExposedUses) from the backward pass. This info
        // isn't available for some of the symbols created during the backward pass, or the forward pass.
        // Keep track of the last symbol for which we're guaranteed to have data.
        this->maxInitialSymID = this->func->m_symTable->GetMaxSymID();
        this->BackwardPass(Js::BackwardPhase);
        this->ForwardPass();
    }
    this->BackwardPass(Js::DeadStorePhase);
    this->TailDupPass();
}

bool GlobOpt::ShouldExpectConventionalArrayIndexValue(IR::IndirOpnd *const indirOpnd)
{
    Assert(indirOpnd);

    if(!indirOpnd->GetIndexOpnd())
    {
        return indirOpnd->GetOffset() >= 0;
    }

    IR::RegOpnd *const indexOpnd = indirOpnd->GetIndexOpnd();
    if(indexOpnd->m_sym->m_isNotInt)
    {
        // Typically, single-def or any sym-specific information for type-specialized syms should not be used because all of
        // their defs will not have been accounted for until after the forward pass. But m_isNotInt is only ever changed from
        // false to true, so it's okay in this case.
        return false;
    }

    StackSym *indexVarSym = indexOpnd->m_sym;
    if(indexVarSym->IsTypeSpec())
    {
        indexVarSym = indexVarSym->GetVarEquivSym(nullptr);
        Assert(indexVarSym);
    }
    else if(!IsLoopPrePass())
    {
        // Don't use single-def info or const flags for type-specialized syms, as all of their defs will not have been accounted
        // for until after the forward pass. Also, don't use the const flags in a loop prepass because the const flags may not
        // be up-to-date.
        StackSym *const indexSym = indexOpnd->m_sym;
        if(indexSym->IsIntConst())
        {
            return indexSym->GetIntConstValue() >= 0;
        }
    }

    Value *const indexValue = CurrentBlockData()->FindValue(indexVarSym);
    if(!indexValue)
    {
        // Treat it as Uninitialized, assume it's going to be valid
        return true;
    }

    ValueInfo *const indexValueInfo = indexValue->GetValueInfo();
    int32 indexConstantValue;
    if(indexValueInfo->TryGetIntConstantValue(&indexConstantValue))
    {
        return indexConstantValue >= 0;
    }

    if(indexValueInfo->IsUninitialized())
    {
        // Assume it's going to be valid
        return true;
    }

    return indexValueInfo->HasBeenNumber() && !indexValueInfo->HasBeenFloat();
}

//
// Either result is float or 1/x  or cst1/cst2 where cst1%cst2 != 0
//
ValueType GlobOpt::GetDivValueType(IR::Instr* instr, Value* src1Val, Value* src2Val, bool specialize)
{
    ValueInfo *src1ValueInfo = (src1Val ? src1Val->GetValueInfo() : nullptr);
    ValueInfo *src2ValueInfo = (src2Val ? src2Val->GetValueInfo() : nullptr);

    if (instr->IsProfiledInstr() && instr->m_func->HasProfileInfo())
    {
        ValueType resultType = instr->m_func->GetReadOnlyProfileInfo()->GetDivProfileInfo(static_cast<Js::ProfileId>(instr->AsProfiledInstr()->u.profileId));
        if (resultType.IsLikelyInt())
        {
            if (specialize && src1ValueInfo && src2ValueInfo
                && ((src1ValueInfo->IsInt() && src2ValueInfo->IsInt()) ||
                (this->DoDivIntTypeSpec() && src1ValueInfo->IsLikelyInt() && src2ValueInfo->IsLikelyInt())))
            {
                return ValueType::GetInt(true);
            }
            return resultType;
        }

        // Consider: Checking that the sources are numbers.
        if (resultType.IsLikelyFloat())
        {
            return ValueType::Float;
        }
        return resultType;
    }
    int32 src1IntConstantValue;
    if(!src1ValueInfo || !src1ValueInfo->TryGetIntConstantValue(&src1IntConstantValue))
    {
        return ValueType::Number;
    }
    if (src1IntConstantValue == 1)
    {
        return ValueType::Float;
    }
    int32 src2IntConstantValue;
    if(!src2Val || !src2ValueInfo->TryGetIntConstantValue(&src2IntConstantValue))
    {
        return ValueType::Number;
    }
    if (src2IntConstantValue     // Avoid divide by zero
        && !(src1IntConstantValue == 0x80000000 && src2IntConstantValue == -1)  // Avoid integer overflow
        && (src1IntConstantValue % src2IntConstantValue) != 0)
    {
        return ValueType::Float;
    }
    return ValueType::Number;
}

void
GlobOpt::ForwardPass()
{
    BEGIN_CODEGEN_PHASE(this->func, Js::ForwardPhase);

#if DBG_DUMP
    if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::GlobOptPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId()))
    {
        this->func->DumpHeader();
    }
    if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::GlobOptPhase))
    {
        this->TraceSettings();
    }
#endif

    // GetConstantCount() gives us the right size to pick for the SparseArray, but we may need more if we've inlined
    // functions with constants. There will be a gap in the symbol numbering between the main constants and
    // the inlined ones, so we'll most likely need a new array chunk. Make the min size of the array chunks be 64
    // in case we have a main function with very few constants and a bunch of constants from inlined functions.
    this->byteCodeConstantValueArray = SparseArray<Value>::New(this->alloc, max(this->func->GetJITFunctionBody()->GetConstCount(), 64U));
    this->byteCodeConstantValueNumbersBv = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
    this->tempBv = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
    this->prePassCopyPropSym = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
    this->slotSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
    this->byteCodeUses = nullptr;
    this->propertySymUse = nullptr;

    // changedSymsAfterIncBailoutCandidate helps track building incremental bailout in ForwardPass
    this->changedSymsAfterIncBailoutCandidate = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);

#if DBG
    this->byteCodeUsesBeforeOpt = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
    if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::FieldCopyPropPhase) && this->DoFunctionFieldCopyProp())
    {
        Output::Print(_u("TRACE: CanDoFieldCopyProp Func: "));
        this->func->DumpFullFunctionName();
        Output::Print(_u("\n"));
    }
#endif

    OpndList localNoImplicitCallUsesToInsert(alloc);
    this->noImplicitCallUsesToInsert = &localNoImplicitCallUsesToInsert;
    IntConstantToStackSymMap localIntConstantToStackSymMap(alloc);
    this->intConstantToStackSymMap = &localIntConstantToStackSymMap;
    IntConstantToValueMap localIntConstantToValueMap(alloc);
    this->intConstantToValueMap = &localIntConstantToValueMap;
    Int64ConstantToValueMap localInt64ConstantToValueMap(alloc);
    this->int64ConstantToValueMap = &localInt64ConstantToValueMap;
    AddrConstantToValueMap localAddrConstantToValueMap(alloc);
    this->addrConstantToValueMap = &localAddrConstantToValueMap;
    StringConstantToValueMap localStringConstantToValueMap(alloc);
    this->stringConstantToValueMap = &localStringConstantToValueMap;
    SymIdToInstrMap localPrePassInstrMap(alloc);
    this->prePassInstrMap = &localPrePassInstrMap;
    ValueSetByValueNumber localValuesCreatedForClone(alloc, 64);
    this->valuesCreatedForClone = &localValuesCreatedForClone;
    ValueNumberPairToValueMap localValuesCreatedForMerge(alloc, 64);
    this->valuesCreatedForMerge = &localValuesCreatedForMerge;

#if DBG
    BVSparse<JitArenaAllocator> localFinishedStackLiteralInitFld(alloc);
    this->finishedStackLiteralInitFld = &localFinishedStackLiteralInitFld;
#endif

    FOREACH_BLOCK_IN_FUNC_EDITING(block, this->func)
    {
        this->OptBlock(block);
    } NEXT_BLOCK_IN_FUNC_EDITING;

    if (!PHASE_OFF(Js::MemOpPhase, this->func))
    {
        ProcessMemOp();
    }

    this->noImplicitCallUsesToInsert = nullptr;
    this->intConstantToStackSymMap = nullptr;
    this->intConstantToValueMap = nullptr;
    this->int64ConstantToValueMap = nullptr;
    this->addrConstantToValueMap = nullptr;
    this->stringConstantToValueMap = nullptr;
#if DBG
    this->finishedStackLiteralInitFld = nullptr;
    uint freedCount = 0;
    uint spilledCount = 0;
#endif

    FOREACH_BLOCK_IN_FUNC(block, this->func)
    {
#if DBG
        if (block->GetDataUseCount() == 0)
        {
            freedCount++;
        }
        else
        {
            spilledCount++;
        }
#endif
        block->SetDataUseCount(0);
        if (block->cloneStrCandidates)
        {
            JitAdelete(this->alloc, block->cloneStrCandidates);
            block->cloneStrCandidates = nullptr;
        }
    } NEXT_BLOCK_IN_FUNC;

    // Make sure we free most of them.
    Assert(freedCount >= spilledCount);

    // this->alloc will be freed right after return, no need to free it here
    this->changedSymsAfterIncBailoutCandidate = nullptr;

    END_CODEGEN_PHASE(this->func, Js::ForwardPhase);
}

void
GlobOpt::OptBlock(BasicBlock *block)
{
    if (this->func->m_fg->RemoveUnreachableBlock(block, this))
    {
        GOPT_TRACE(_u("Removing unreachable block #%d\n"), block->GetBlockNum());
        return;
    }

    Loop * loop = block->loop;
    if (loop && block->isLoopHeader)
    {
        if (loop != this->prePassLoop)
        {
            OptLoops(loop);
            if (!this->IsLoopPrePass() && DoFieldPRE(loop))
            {
                // Note: !IsLoopPrePass means this was a root loop pre-pass. FieldPre() is called once per loop.
                this->FieldPRE(loop);

                // Re-optimize the landing pad
                BasicBlock *landingPad = loop->landingPad;
                this->isRecursiveCallOnLandingPad = true;

                this->OptBlock(landingPad);

                this->isRecursiveCallOnLandingPad = false;
            }
        }
    }

    this->currentBlock = block;
    PrepareLoopArrayCheckHoist();

    block->MergePredBlocksValueMaps(this);

    this->intOverflowCurrentlyMattersInRange = true;
    this->intOverflowDoesNotMatterRange = this->currentBlock->intOverflowDoesNotMatterRange;

    if (loop && DoFieldHoisting(loop))
    {
        if (block->isLoopHeader)
        {
            if (!this->IsLoopPrePass())
            {
                this->PrepareFieldHoisting(loop);
            }
            else if (loop == this->rootLoopPrePass)
            {
                this->PreparePrepassFieldHoisting(loop);
            }
        }
    }
    else
    {
        Assert(!TrackHoistableFields() || !HasHoistableFields(CurrentBlockData()));
        if (!DoFieldCopyProp() && !DoFieldRefOpts())
        {
            this->KillAllFields(CurrentBlockData()->liveFields);
        }
    }

    this->tempAlloc->Reset();

    if(loop && block->isLoopHeader)
    {
        loop->firstValueNumberInLoop = this->currentValue;
    }

    GOPT_TRACE_BLOCK(block, true);

    FOREACH_INSTR_IN_BLOCK_EDITING(instr, instrNext, block)
    {
        GOPT_TRACE_INSTRTRACE(instr);
        BailOutInfo* oldBailOutInfo = nullptr;
        bool isCheckAuxBailoutNeeded = this->func->IsJitInDebugMode() && !this->IsLoopPrePass();
        if (isCheckAuxBailoutNeeded && instr->HasAuxBailOut() && !instr->HasBailOutInfo())
        {
            oldBailOutInfo = instr->GetBailOutInfo();
            Assert(oldBailOutInfo);
        }
        bool isInstrRemoved = false;
        instrNext = this->OptInstr(instr, &isInstrRemoved);

        // If we still have instrs with only aux bail out, convert aux bail out back to regular bail out and fill it.
        // During OptInstr some instr can be moved out to a different block, in this case bailout info is going to be replaced
        // with e.g. loop bailout info which is filled as part of processing that block, thus we don't need to fill it here.
        if (isCheckAuxBailoutNeeded && !isInstrRemoved && instr->HasAuxBailOut() && !instr->HasBailOutInfo())
        {
            if (instr->GetBailOutInfo() == oldBailOutInfo)
            {
                instr->PromoteAuxBailOut();
                FillBailOutInfo(block, instr->GetBailOutInfo());
            }
            else
            {
                AssertMsg(instr->GetBailOutInfo(), "With aux bailout, the bailout info should not be removed by OptInstr.");
            }
        }

    } NEXT_INSTR_IN_BLOCK_EDITING;

    GOPT_TRACE_BLOCK(block, false);
    if (block->loop)
    {
        if (IsLoopPrePass())
        {
            if (DoBoundCheckHoist())
            {
                DetectUnknownChangesToInductionVariables(&block->globOptData);
            }
        }
        else
        {
            isPerformingLoopBackEdgeCompensation = true;

            Assert(this->tempBv->IsEmpty());
            BVSparse<JitArenaAllocator> tempBv2(this->tempAlloc);

            // On loop back-edges, we need to restore the state of the type specialized
            // symbols to that of the loop header.
            FOREACH_SUCCESSOR_BLOCK(succ, block)
            {
                if (succ->isLoopHeader && succ->loop->IsDescendentOrSelf(block->loop))
                {
                    BVSparse<JitArenaAllocator> *liveOnBackEdge = block->loop->regAlloc.liveOnBackEdgeSyms;

                    this->tempBv->Minus(block->loop->varSymsOnEntry, block->globOptData.liveVarSyms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToVar(this->tempBv, block);

                    // Lossy int in the loop header, and no int on the back-edge - need a lossy conversion to int
                    this->tempBv->Minus(block->loop->lossyInt32SymsOnEntry, block->globOptData.liveInt32Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToInt32(this->tempBv, block, true /* lossy */);

                    // Lossless int in the loop header, and no lossless int on the back-edge - need a lossless conversion to int
                    this->tempBv->Minus(block->loop->int32SymsOnEntry, block->loop->lossyInt32SymsOnEntry);
                    tempBv2.Minus(block->globOptData.liveInt32Syms, block->globOptData.liveLossyInt32Syms);
                    this->tempBv->Minus(&tempBv2);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToInt32(this->tempBv, block, false /* lossy */);

                    this->tempBv->Minus(block->loop->float64SymsOnEntry, block->globOptData.liveFloat64Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToFloat64(this->tempBv, block);

#ifdef ENABLE_SIMDJS
                    // SIMD_JS
                    // Compensate on backedge if sym is live on loop entry but not on backedge
                    this->tempBv->Minus(block->loop->simd128F4SymsOnEntry, block->globOptData.liveSimd128F4Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToTypeSpec(this->tempBv, block, TySimd128F4, IR::BailOutSimd128F4Only);

                    this->tempBv->Minus(block->loop->simd128I4SymsOnEntry, block->globOptData.liveSimd128I4Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToTypeSpec(this->tempBv, block, TySimd128I4, IR::BailOutSimd128I4Only);
#endif

                    // For ints and floats, go aggressive and type specialize in the landing pad any symbol which was specialized on
                    // entry to the loop body (in the loop header), and is still specialized on this tail, but wasn't specialized in
                    // the landing pad.

                    // Lossy int in the loop header and no int in the landing pad - need a lossy conversion to int
                    // (entry.lossyInt32 - landingPad.int32)
                    this->tempBv->Minus(block->loop->lossyInt32SymsOnEntry, block->loop->landingPad->globOptData.liveInt32Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToInt32(this->tempBv, block->loop->landingPad, true /* lossy */);

                    // Lossless int in the loop header, and no lossless int in the landing pad - need a lossless conversion to int
                    // ((entry.int32 - entry.lossyInt32) - (landingPad.int32 - landingPad.lossyInt32))
                    this->tempBv->Minus(block->loop->int32SymsOnEntry, block->loop->lossyInt32SymsOnEntry);
                    tempBv2.Minus(
                        block->loop->landingPad->globOptData.liveInt32Syms,
                        block->loop->landingPad->globOptData.liveLossyInt32Syms);
                    this->tempBv->Minus(&tempBv2);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToInt32(this->tempBv, block->loop->landingPad, false /* lossy */);

                    // ((entry.float64 - landingPad.float64) & block.float64)
                    this->tempBv->Minus(block->loop->float64SymsOnEntry, block->loop->landingPad->globOptData.liveFloat64Syms);
                    this->tempBv->And(block->globOptData.liveFloat64Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToFloat64(this->tempBv, block->loop->landingPad);

#ifdef ENABLE_SIMDJS
                    // SIMD_JS
                    // compensate on landingpad if live on loopEntry and Backedge.
                    this->tempBv->Minus(block->loop->simd128F4SymsOnEntry, block->loop->landingPad->globOptData.liveSimd128F4Syms);
                    this->tempBv->And(block->globOptData.liveSimd128F4Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToTypeSpec(this->tempBv, block->loop->landingPad, TySimd128F4, IR::BailOutSimd128F4Only);

                    this->tempBv->Minus(block->loop->simd128I4SymsOnEntry, block->loop->landingPad->globOptData.liveSimd128I4Syms);
                    this->tempBv->And(block->globOptData.liveSimd128I4Syms);
                    this->tempBv->And(liveOnBackEdge);
                    this->ToTypeSpec(this->tempBv, block->loop->landingPad, TySimd128I4, IR::BailOutSimd128I4Only);
#endif

                    // Now that we're done with the liveFields within this loop, trim the set to those syms
                    // that the backward pass told us were live out of the loop.
                    // This assumes we have no further need of the liveFields within the loop.
                    if (block->loop->liveOutFields)
                    {
                        block->globOptData.liveFields->And(block->loop->liveOutFields);
                    }
                }
            } NEXT_SUCCESSOR_BLOCK;

            this->tempBv->ClearAll();

            isPerformingLoopBackEdgeCompensation = false;
        }
    }

#if DBG
    // The set of live lossy int32 syms should be a subset of all live int32 syms
    this->tempBv->And(block->globOptData.liveInt32Syms, block->globOptData.liveLossyInt32Syms);
    Assert(this->tempBv->Count() == block->globOptData.liveLossyInt32Syms->Count());

    // The set of live lossy int32 syms should be a subset of live var or float syms (var or float sym containing the lossless
    // value of the sym should be live)
    this->tempBv->Or(block->globOptData.liveVarSyms, block->globOptData.liveFloat64Syms);
    this->tempBv->And(block->globOptData.liveLossyInt32Syms);
    Assert(this->tempBv->Count() == block->globOptData.liveLossyInt32Syms->Count());

    this->tempBv->ClearAll();
    Assert(this->currentBlock == block);
#endif
}

void
GlobOpt::OptLoops(Loop *loop)
{
    Assert(loop != nullptr);
#if DBG
    if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::FieldCopyPropPhase) &&
        !DoFunctionFieldCopyProp() && DoFieldCopyProp(loop))
    {
        Output::Print(_u("TRACE: CanDoFieldCopyProp Loop: "));
        this->func->DumpFullFunctionName();
        uint loopNumber = loop->GetLoopNumber();
        Assert(loopNumber != Js::LoopHeader::NoLoop);
        Output::Print(_u(" Loop: %d\n"), loopNumber);
    }
#endif

    Loop *previousLoop = this->prePassLoop;
    this->prePassLoop = loop;

    if (previousLoop == nullptr)
    {
        Assert(this->rootLoopPrePass == nullptr);
        this->rootLoopPrePass = loop;
        this->prePassInstrMap->Clear();
        if (loop->parent == nullptr)
        {
            // Outer most loop...
            this->prePassCopyPropSym->ClearAll();
        }
    }

    if (loop->symsUsedBeforeDefined == nullptr)
    {
        loop->symsUsedBeforeDefined = JitAnew(alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->likelyIntSymsUsedBeforeDefined = JitAnew(alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->likelyNumberSymsUsedBeforeDefined = JitAnew(alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->forceFloat64SymsOnEntry = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);

#ifdef ENABLE_SIMDJS
        loop->likelySimd128F4SymsUsedBeforeDefined = JitAnew(alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->likelySimd128I4SymsUsedBeforeDefined = JitAnew(alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->forceSimd128F4SymsOnEntry = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->forceSimd128I4SymsOnEntry = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
#endif

        loop->symsDefInLoop = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->fieldKilled = JitAnew(alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->fieldPRESymStore = JitAnew(alloc, BVSparse<JitArenaAllocator>, this->alloc);
        loop->allFieldsKilled = false;
    }
    else
    {
        loop->symsUsedBeforeDefined->ClearAll();
        loop->likelyIntSymsUsedBeforeDefined->ClearAll();
        loop->likelyNumberSymsUsedBeforeDefined->ClearAll();
        loop->forceFloat64SymsOnEntry->ClearAll();
#ifdef ENABLE_SIMDJS
        loop->likelySimd128F4SymsUsedBeforeDefined->ClearAll();
        loop->likelySimd128I4SymsUsedBeforeDefined->ClearAll();
        loop->forceSimd128F4SymsOnEntry->ClearAll();
        loop->forceSimd128I4SymsOnEntry->ClearAll();
#endif
        loop->symsDefInLoop->ClearAll();
        loop->fieldKilled->ClearAll();
        loop->allFieldsKilled = false;
        loop->initialValueFieldMap.Reset();
    }

    FOREACH_BLOCK_IN_LOOP(block, loop)
    {
        block->SetDataUseCount(block->GetSuccList()->Count());
        OptBlock(block);
    } NEXT_BLOCK_IN_LOOP;

    if (previousLoop == nullptr)
    {
        Assert(this->rootLoopPrePass == loop);
        this->rootLoopPrePass = nullptr;
    }

    this->prePassLoop = previousLoop;
}

void
GlobOpt::TailDupPass()
{
    FOREACH_LOOP_IN_FUNC_EDITING(loop, this->func)
    {
        BasicBlock* header = loop->GetHeadBlock();
        BasicBlock* loopTail = nullptr;
        FOREACH_PREDECESSOR_BLOCK(pred, header)
        {
            if (loop->IsDescendentOrSelf(pred->loop))
            {
                loopTail = pred;
                break;
            }
        } NEXT_PREDECESSOR_BLOCK;

        if (loopTail)
        {
            AssertMsg(loopTail->GetLastInstr()->IsBranchInstr(), "LastInstr of loop should always be a branch no?");

            if (!loopTail->GetPredList()->HasOne())
            {
                TryTailDup(loopTail->GetLastInstr()->AsBranchInstr());
            }
        }
    } NEXT_LOOP_IN_FUNC_EDITING;
}

bool
GlobOpt::TryTailDup(IR::BranchInstr *tailBranch)
{
    if (PHASE_OFF(Js::TailDupPhase, tailBranch->m_func->GetTopFunc()))
    {
        return false;
    }

    if (tailBranch->IsConditional())
    {
        return false;
    }

    IR::Instr *instr;
    uint instrCount = 0;
    for (instr = tailBranch->GetPrevRealInstrOrLabel(); !instr->IsLabelInstr(); instr = instr->GetPrevRealInstrOrLabel())
    {
        if (instr->HasBailOutInfo())
        {
            break;
        }
        if (!OpCodeAttr::CanCSE(instr->m_opcode))
        {
            // Consider: We could be more aggressive here
            break;
        }

        instrCount++;

        if (instrCount > 1)
        {
            // Consider: If copy handled single-def tmps renaming, we could do more instrs
            break;
        }
    }

    if (!instr->IsLabelInstr())
    {
        return false;
    }

    IR::LabelInstr *mergeLabel = instr->AsLabelInstr();
    IR::Instr *mergeLabelPrev = mergeLabel->m_prev;

    // Skip unreferenced labels
    while (mergeLabelPrev->IsLabelInstr() && mergeLabelPrev->AsLabelInstr()->labelRefs.Empty())
    {
        mergeLabelPrev = mergeLabelPrev->m_prev;
    }

    BasicBlock* labelBlock = mergeLabel->GetBasicBlock();
    uint origPredCount = labelBlock->GetPredList()->Count();
    uint dupCount = 0;

    // We are good to go. Let's do the tail duplication.
    FOREACH_SLISTCOUNTED_ENTRY_EDITING(IR::BranchInstr*, branchEntry, &mergeLabel->labelRefs, iter)
    {
        if (branchEntry->IsUnconditional() && !branchEntry->IsMultiBranch() && branchEntry != mergeLabelPrev && branchEntry != tailBranch)
        {
            for (instr = mergeLabel->m_next; instr != tailBranch; instr = instr->m_next)
            {
                branchEntry->InsertBefore(instr->Copy());
            }

            instr = branchEntry;
            branchEntry->ReplaceTarget(mergeLabel, tailBranch->GetTarget());

            while(!instr->IsLabelInstr())
            {
                instr = instr->m_prev;
            }
            BasicBlock* branchBlock = instr->AsLabelInstr()->GetBasicBlock();

            labelBlock->RemovePred(branchBlock, func->m_fg);
            func->m_fg->AddEdge(branchBlock, tailBranch->GetTarget()->GetBasicBlock());

            dupCount++;
        }
    } NEXT_SLISTCOUNTED_ENTRY_EDITING;

    // If we've duplicated everywhere, tail block is dead and should be removed.
    if (dupCount == origPredCount)
    {
        AssertMsg(mergeLabel->labelRefs.Empty(), "Should not remove block with referenced label.");
        func->m_fg->RemoveBlock(labelBlock, nullptr, true);
    }

    return true;
}

void
GlobOpt::ToVar(BVSparse<JitArenaAllocator> *bv, BasicBlock *block)
{
    FOREACH_BITSET_IN_SPARSEBV(id, bv)
    {
        StackSym *stackSym = this->func->m_symTable->FindStackSym(id);
        IR::RegOpnd *newOpnd = IR::RegOpnd::New(stackSym, TyVar, this->func);
        IR::Instr *lastInstr = block->GetLastInstr();
        if (lastInstr->IsBranchInstr() || lastInstr->m_opcode == Js::OpCode::BailTarget)
        {
            // If branch is using this symbol, hoist the operand as the ToVar load will get
            // inserted right before the branch.
            IR::Opnd *src1 = lastInstr->GetSrc1();
            if (src1)
            {
                if (src1->IsRegOpnd() && src1->AsRegOpnd()->m_sym == stackSym)
                {
                    lastInstr->HoistSrc1(Js::OpCode::Ld_A);
                }
                IR::Opnd *src2 = lastInstr->GetSrc2();
                if (src2)
                {
                    if (src2->IsRegOpnd() && src2->AsRegOpnd()->m_sym == stackSym)
                    {
                        lastInstr->HoistSrc2(Js::OpCode::Ld_A);
                    }
                }
            }
            this->ToVar(lastInstr, newOpnd, block, nullptr, false);
        }
        else
        {
            IR::Instr *lastNextInstr = lastInstr->m_next;
            this->ToVar(lastNextInstr, newOpnd, block, nullptr, false);
        }
    } NEXT_BITSET_IN_SPARSEBV;
}

void
GlobOpt::ToInt32(BVSparse<JitArenaAllocator> *bv, BasicBlock *block, bool lossy, IR::Instr *insertBeforeInstr)
{
    return this->ToTypeSpec(bv, block, TyInt32, IR::BailOutIntOnly, lossy, insertBeforeInstr);
}

void
GlobOpt::ToFloat64(BVSparse<JitArenaAllocator> *bv, BasicBlock *block)
{
    return this->ToTypeSpec(bv, block, TyFloat64, IR::BailOutNumberOnly);
}

void
GlobOpt::ToTypeSpec(BVSparse<JitArenaAllocator> *bv, BasicBlock *block, IRType toType, IR::BailOutKind bailOutKind, bool lossy, IR::Instr *insertBeforeInstr)
{
    FOREACH_BITSET_IN_SPARSEBV(id, bv)
    {
        StackSym *stackSym = this->func->m_symTable->FindStackSym(id);
        IRType fromType = TyIllegal;

        // Win8 bug: 757126. If we are trying to type specialize the arguments object,
        // let's make sure stack args optimization is not enabled. This is a problem, particularly,
        // if the instruction comes from an unreachable block. In other cases, the pass on the
        // instruction itself should disable arguments object optimization.
        if(block->globOptData.argObjSyms && block->globOptData.IsArgumentsSymID(id))
        {
            CannotAllocateArgumentsObjectOnStack();
        }

        if (block->globOptData.liveVarSyms->Test(id))
        {
            fromType = TyVar;
        }
        else if (block->globOptData.liveInt32Syms->Test(id) && !block->globOptData.liveLossyInt32Syms->Test(id))
        {
            fromType = TyInt32;
            stackSym = stackSym->GetInt32EquivSym(this->func);
        }
        else if (block->globOptData.liveFloat64Syms->Test(id))
        {

            fromType = TyFloat64;
            stackSym = stackSym->GetFloat64EquivSym(this->func);
        }
        else
        {
#ifdef ENABLE_SIMDJS
            Assert(block->globOptData.IsLiveAsSimd128(stackSym));
            if (block->globOptData.IsLiveAsSimd128F4(stackSym))
            {
                fromType = TySimd128F4;
                stackSym = stackSym->GetSimd128F4EquivSym(this->func);
            }
            else
            {
                fromType = TySimd128I4;
                stackSym = stackSym->GetSimd128I4EquivSym(this->func);
            }
#else
            Assert(UNREACHED);
#endif
        }


        IR::RegOpnd *newOpnd = IR::RegOpnd::New(stackSym, fromType, this->func);
        IR::Instr *lastInstr = block->GetLastInstr();

        if (!insertBeforeInstr && lastInstr->IsBranchInstr())
        {
            // If branch is using this symbol, hoist the operand as the ToInt32 load will get
            // inserted right before the branch.
            IR::Instr *instrPrev = lastInstr->m_prev;
            IR::Opnd *src1 = lastInstr->GetSrc1();
            if (src1)
            {
                if (src1->IsRegOpnd() && src1->AsRegOpnd()->m_sym == stackSym)
                {
                    lastInstr->HoistSrc1(Js::OpCode::Ld_A);
                }
                IR::Opnd *src2 = lastInstr->GetSrc2();
                if (src2)
                {
                    if (src2->IsRegOpnd() && src2->AsRegOpnd()->m_sym == stackSym)
                    {
                        lastInstr->HoistSrc2(Js::OpCode::Ld_A);
                    }
                }

                // Did we insert anything?
                if (lastInstr->m_prev != instrPrev)
                {
                    // If we had ByteCodeUses right before the branch, move them back down.
                    IR::Instr *insertPoint = lastInstr;
                    for (IR::Instr *instrBytecode = instrPrev; instrBytecode->m_opcode == Js::OpCode::ByteCodeUses; instrBytecode = instrBytecode->m_prev)
                    {
                        instrBytecode->Unlink();
                        insertPoint->InsertBefore(instrBytecode);
                        insertPoint = instrBytecode;
                    }
                }
            }
        }
        this->ToTypeSpecUse(nullptr, newOpnd, block, nullptr, nullptr, toType, bailOutKind, lossy, insertBeforeInstr);
    } NEXT_BITSET_IN_SPARSEBV;
}

PRECandidatesList * GlobOpt::FindPossiblePRECandidates(Loop *loop, JitArenaAllocator *alloc)
{
    // Find the set of PRE candidates
    BasicBlock *loopHeader = loop->GetHeadBlock();
    PRECandidatesList *candidates = nullptr;
    bool firstBackEdge = true;

    FOREACH_PREDECESSOR_BLOCK(blockPred, loopHeader)
    {
        if (!loop->IsDescendentOrSelf(blockPred->loop))
        {
            // Not a loop back-edge
            continue;
        }
        if (firstBackEdge)
        {
            candidates = this->FindBackEdgePRECandidates(blockPred, alloc);
        }
        else
        {
            blockPred->globOptData.RemoveUnavailableCandidates(candidates);
        }
    } NEXT_PREDECESSOR_BLOCK;

    return candidates;
}

BOOL GlobOpt::PreloadPRECandidate(Loop *loop, GlobHashBucket* candidate)
{
    // Insert a load for each field PRE candidate.
    PropertySym *propertySym = candidate->value->AsPropertySym();
    StackSym *objPtrSym = propertySym->m_stackSym;

    // If objPtr isn't live, we'll retry later.
    // Another PRE candidate may insert a load for it.
    if (!loop->landingPad->globOptData.IsLive(objPtrSym))
    {
        return false;
    }
    BasicBlock *landingPad = loop->landingPad;
    Value *value = candidate->element;
    Sym *symStore = value->GetValueInfo()->GetSymStore();

    // The symStore can't be live into the loop
    // The symStore needs to still have the same value
    Assert(symStore && symStore->IsStackSym());

    if (loop->landingPad->globOptData.IsLive(symStore))
    {
        // May have already been hoisted:
        //  o.x = t1;
        //  o.y = t1;
        return false;
    }
    Value *landingPadValue = landingPad->globOptData.FindValue(propertySym);

    // Value should be added as initial value or already be there.
    Assert(landingPadValue);

    IR::Instr * ldInstr = this->prePassInstrMap->Lookup(propertySym->m_id, nullptr);
    Assert(ldInstr);

    // Create instr to put in landing pad for compensation
    Assert(IsPREInstrCandidateLoad(ldInstr->m_opcode));
    IR::SymOpnd *ldSrc = ldInstr->GetSrc1()->AsSymOpnd();

    if (ldSrc->m_sym != propertySym)
    {
        // It's possible that the propertySym but have equivalent objPtrs.  Verify their values.
        Value *val1 = CurrentBlockData()->FindValue(ldSrc->m_sym->AsPropertySym()->m_stackSym);
        Value *val2 = CurrentBlockData()->FindValue(propertySym->m_stackSym);
        if (!val1 || !val2 || val1->GetValueNumber() != val2->GetValueNumber())
        {
            return false;
        }
    }

    ldInstr = ldInstr->Copy();

    // Consider: Shouldn't be necessary once we have copy-prop in prepass...
    ldInstr->GetSrc1()->AsSymOpnd()->m_sym = propertySym;
    ldSrc = ldInstr->GetSrc1()->AsSymOpnd();

    if (ldSrc->IsPropertySymOpnd())
    {
        IR::PropertySymOpnd *propSymOpnd = ldSrc->AsPropertySymOpnd();
        IR::PropertySymOpnd *newPropSymOpnd;

        newPropSymOpnd = propSymOpnd->AsPropertySymOpnd()->CopyWithoutFlowSensitiveInfo(this->func);
        ldInstr->ReplaceSrc1(newPropSymOpnd);
    }

    if (ldInstr->GetDst()->AsRegOpnd()->m_sym != symStore)
    {
        ldInstr->ReplaceDst(IR::RegOpnd::New(symStore->AsStackSym(), TyVar, this->func));
    }

    ldInstr->GetSrc1()->SetIsJITOptimizedReg(true);
    ldInstr->GetDst()->SetIsJITOptimizedReg(true);

    landingPad->globOptData.liveVarSyms->Set(symStore->m_id);
    loop->fieldPRESymStore->Set(symStore->m_id);

    ValueType valueType(ValueType::Uninitialized);
    Value *initialValue = nullptr;

    if (loop->initialValueFieldMap.TryGetValue(propertySym, &initialValue))
    {
        if (ldInstr->IsProfiledInstr())
        {
            if (initialValue->GetValueNumber() == value->GetValueNumber())
            {
                if (value->GetValueInfo()->IsUninitialized())
                {
                    valueType = ldInstr->AsProfiledInstr()->u.FldInfo().valueType;
                }
                else
                {
                    valueType = value->GetValueInfo()->Type();
                }
            }
            else
            {
                valueType = ValueType::Uninitialized;
            }
            ldInstr->AsProfiledInstr()->u.FldInfo().valueType = valueType;
        }
    }
    else
    {
        valueType = landingPadValue->GetValueInfo()->Type();
    }

    loop->symsUsedBeforeDefined->Set(symStore->m_id);

    if (valueType.IsLikelyNumber())
    {
        loop->likelyNumberSymsUsedBeforeDefined->Set(symStore->m_id);
        if (DoAggressiveIntTypeSpec() ? valueType.IsLikelyInt() : valueType.IsInt())
        {
            // Can only force int conversions in the landing pad based on likely-int values if aggressive int type
            // specialization is enabled
            loop->likelyIntSymsUsedBeforeDefined->Set(symStore->m_id);
        }
    }

    // Insert in landing pad
    if (ldInstr->HasAnyImplicitCalls())
    {
        IR::Instr * bailInstr = EnsureDisableImplicitCallRegion(loop);

        bailInstr->InsertBefore(ldInstr);
    }
    else if (loop->endDisableImplicitCall)
    {
        loop->endDisableImplicitCall->InsertBefore(ldInstr);
    }
    else
    {
        loop->landingPad->InsertAfter(ldInstr);
    }

    ldInstr->ClearByteCodeOffset();
    ldInstr->SetByteCodeOffset(landingPad->GetFirstInstr());

#if DBG_DUMP
    if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::FieldPREPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId()))
    {
        Output::Print(_u("** TRACE: Field PRE: field pre-loaded in landing pad of loop head #%-3d: "), loop->GetHeadBlock()->GetBlockNum());
        ldInstr->Dump();
        Output::Print(_u("\n"));
    }
#endif

    return true;
}

void GlobOpt::PreloadPRECandidates(Loop *loop, PRECandidatesList *candidates)
{
    // Insert loads in landing pad for field PRE candidates. Iterate while(changed)
    // for the o.x.y cases.
    BOOL changed = true;

    if (!candidates)
    {
        return;
    }

    Assert(loop->landingPad->GetFirstInstr() == loop->landingPad->GetLastInstr());

    while (changed)
    {
        changed = false;
        FOREACH_SLIST_ENTRY_EDITING(GlobHashBucket*, candidate, (SList<GlobHashBucket*>*)candidates, iter)
        {
            if (this->PreloadPRECandidate(loop, candidate))
            {
                changed = true;
                iter.RemoveCurrent();
            }
        } NEXT_SLIST_ENTRY_EDITING;
    }
}

void GlobOpt::FieldPRE(Loop *loop)
{
    if (!DoFieldPRE(loop))
    {
        return;
    }

    PRECandidatesList *candidates;
    JitArenaAllocator *alloc = this->tempAlloc;

    candidates = this->FindPossiblePRECandidates(loop, alloc);
    this->PreloadPRECandidates(loop, candidates);
}

void GlobOpt::InsertValueCompensation(
    BasicBlock *const predecessor,
    const SymToValueInfoMap &symsRequiringCompensationToMergedValueInfoMap)
{
    Assert(predecessor);
    Assert(symsRequiringCompensationToMergedValueInfoMap.Count() != 0);

    IR::Instr *insertBeforeInstr = predecessor->GetLastInstr();
    Func *const func = insertBeforeInstr->m_func;
    bool setLastInstrInPredecessor;
    if(insertBeforeInstr->IsBranchInstr() || insertBeforeInstr->m_opcode == Js::OpCode::BailTarget)
    {
        // Don't insert code between the branch and the corresponding ByteCodeUses instructions
        while(insertBeforeInstr->m_prev->m_opcode == Js::OpCode::ByteCodeUses)
        {
            insertBeforeInstr = insertBeforeInstr->m_prev;
        }
        setLastInstrInPredecessor = false;
    }
    else
    {
        // Insert at the end of the block and set the last instruction
        Assert(insertBeforeInstr->m_next);
        insertBeforeInstr = insertBeforeInstr->m_next; // Instruction after the last instruction in the predecessor
        setLastInstrInPredecessor = true;
    }

    GlobOptBlockData &predecessorBlockData = predecessor->globOptData;
    GlobOptBlockData &successorBlockData = *CurrentBlockData();
    struct DelayChangeValueInfo
    {
        Value* predecessorValue;
        ArrayValueInfo* valueInfo;
        void ChangeValueInfo(BasicBlock* predecessor, GlobOpt* g)
        {
            g->ChangeValueInfo(
                predecessor,
                predecessorValue,
                valueInfo,
                false /*allowIncompatibleType*/,
                true /*compensated*/);
        }
    };
    JsUtil::List<DelayChangeValueInfo, ArenaAllocator> delayChangeValueInfo(alloc);
    for(auto it = symsRequiringCompensationToMergedValueInfoMap.GetIterator(); it.IsValid(); it.MoveNext())
    {
        const auto &entry = it.Current();
        Sym *const sym = entry.Key();
        Value *const predecessorValue = predecessorBlockData.FindValue(sym);
        Assert(predecessorValue);
        ValueInfo *const predecessorValueInfo = predecessorValue->GetValueInfo();

        // Currently, array value infos are the only ones that require compensation based on values
        Assert(predecessorValueInfo->IsAnyOptimizedArray());
        const ArrayValueInfo *const predecessorArrayValueInfo = predecessorValueInfo->AsArrayValueInfo();
        StackSym *const predecessorHeadSegmentSym = predecessorArrayValueInfo->HeadSegmentSym();
        StackSym *const predecessorHeadSegmentLengthSym = predecessorArrayValueInfo->HeadSegmentLengthSym();
        StackSym *const predecessorLengthSym = predecessorArrayValueInfo->LengthSym();
        ValueInfo *const mergedValueInfo = entry.Value();
        const ArrayValueInfo *const mergedArrayValueInfo = mergedValueInfo->AsArrayValueInfo();
        StackSym *const mergedHeadSegmentSym = mergedArrayValueInfo->HeadSegmentSym();
        StackSym *const mergedHeadSegmentLengthSym = mergedArrayValueInfo->HeadSegmentLengthSym();
        StackSym *const mergedLengthSym = mergedArrayValueInfo->LengthSym();
        Assert(!mergedHeadSegmentSym || predecessorHeadSegmentSym);
        Assert(!mergedHeadSegmentLengthSym || predecessorHeadSegmentLengthSym);
        Assert(!mergedLengthSym || predecessorLengthSym);

        bool compensated = false;
        if(mergedHeadSegmentSym && predecessorHeadSegmentSym != mergedHeadSegmentSym)
        {
            IR::Instr *const newInstr =
                IR::Instr::New(
                    Js::OpCode::Ld_A,
                    IR::RegOpnd::New(mergedHeadSegmentSym, mergedHeadSegmentSym->GetType(), func),
                    IR::RegOpnd::New(predecessorHeadSegmentSym, predecessorHeadSegmentSym->GetType(), func),
                    func);
            newInstr->GetDst()->SetIsJITOptimizedReg(true);
            newInstr->GetSrc1()->SetIsJITOptimizedReg(true);
            newInstr->SetByteCodeOffset(insertBeforeInstr);
            insertBeforeInstr->InsertBefore(newInstr);
            compensated = true;
        }

        if(mergedHeadSegmentLengthSym && predecessorHeadSegmentLengthSym != mergedHeadSegmentLengthSym)
        {
            IR::Instr *const newInstr =
                IR::Instr::New(
                    Js::OpCode::Ld_I4,
                    IR::RegOpnd::New(mergedHeadSegmentLengthSym, mergedHeadSegmentLengthSym->GetType(), func),
                    IR::RegOpnd::New(predecessorHeadSegmentLengthSym, predecessorHeadSegmentLengthSym->GetType(), func),
                    func);
            newInstr->GetDst()->SetIsJITOptimizedReg(true);
            newInstr->GetSrc1()->SetIsJITOptimizedReg(true);
            newInstr->SetByteCodeOffset(insertBeforeInstr);
            insertBeforeInstr->InsertBefore(newInstr);
            compensated = true;

            // Merge the head segment length value
            Assert(predecessorBlockData.liveVarSyms->Test(predecessorHeadSegmentLengthSym->m_id));
            predecessorBlockData.liveVarSyms->Set(mergedHeadSegmentLengthSym->m_id);
            successorBlockData.liveVarSyms->Set(mergedHeadSegmentLengthSym->m_id);
            Value *const predecessorHeadSegmentLengthValue =
                predecessorBlockData.FindValue(predecessorHeadSegmentLengthSym);
            Assert(predecessorHeadSegmentLengthValue);
            predecessorBlockData.SetValue(predecessorHeadSegmentLengthValue, mergedHeadSegmentLengthSym);
            Value *const mergedHeadSegmentLengthValue = successorBlockData.FindValue(mergedHeadSegmentLengthSym);
            if(mergedHeadSegmentLengthValue)
            {
                Assert(mergedHeadSegmentLengthValue->GetValueNumber() != predecessorHeadSegmentLengthValue->GetValueNumber());
                if(predecessorHeadSegmentLengthValue->GetValueInfo() != mergedHeadSegmentLengthValue->GetValueInfo())
                {
                    mergedHeadSegmentLengthValue->SetValueInfo(
                        ValueInfo::MergeLikelyIntValueInfo(
                            this->alloc,
                            mergedHeadSegmentLengthValue,
                            predecessorHeadSegmentLengthValue,
                            mergedHeadSegmentLengthValue->GetValueInfo()->Type()
                                .Merge(predecessorHeadSegmentLengthValue->GetValueInfo()->Type())));
                }
            }
            else
            {
                successorBlockData.SetValue(CopyValue(predecessorHeadSegmentLengthValue), mergedHeadSegmentLengthSym);
            }
        }

        if(mergedLengthSym && predecessorLengthSym != mergedLengthSym)
        {
            IR::Instr *const newInstr =
                IR::Instr::New(
                    Js::OpCode::Ld_I4,
                    IR::RegOpnd::New(mergedLengthSym, mergedLengthSym->GetType(), func),
                    IR::RegOpnd::New(predecessorLengthSym, predecessorLengthSym->GetType(), func),
                    func);
            newInstr->GetDst()->SetIsJITOptimizedReg(true);
            newInstr->GetSrc1()->SetIsJITOptimizedReg(true);
            newInstr->SetByteCodeOffset(insertBeforeInstr);
            insertBeforeInstr->InsertBefore(newInstr);
            compensated = true;

            // Merge the length value
            Assert(predecessorBlockData.liveVarSyms->Test(predecessorLengthSym->m_id));
            predecessorBlockData.liveVarSyms->Set(mergedLengthSym->m_id);
            successorBlockData.liveVarSyms->Set(mergedLengthSym->m_id);
            Value *const predecessorLengthValue = predecessorBlockData.FindValue(predecessorLengthSym);
            Assert(predecessorLengthValue);
            predecessorBlockData.SetValue(predecessorLengthValue, mergedLengthSym);
            Value *const mergedLengthValue = successorBlockData.FindValue(mergedLengthSym);
            if(mergedLengthValue)
            {
                Assert(mergedLengthValue->GetValueNumber() != predecessorLengthValue->GetValueNumber());
                if(predecessorLengthValue->GetValueInfo() != mergedLengthValue->GetValueInfo())
                {
                    mergedLengthValue->SetValueInfo(
                        ValueInfo::MergeLikelyIntValueInfo(
                            this->alloc,
                            mergedLengthValue,
                            predecessorLengthValue,
                            mergedLengthValue->GetValueInfo()->Type().Merge(predecessorLengthValue->GetValueInfo()->Type())));
                }
            }
            else
            {
                successorBlockData.SetValue(CopyValue(predecessorLengthValue), mergedLengthSym);
            }
        }

        if(compensated)
        {
            // Save the new ValueInfo for later.
            // We don't want other symbols needing compensation to see this new one
            delayChangeValueInfo.Add({
                predecessorValue,
                ArrayValueInfo::New(
                    alloc,
                    predecessorValueInfo->Type(),
                    mergedHeadSegmentSym ? mergedHeadSegmentSym : predecessorHeadSegmentSym,
                    mergedHeadSegmentLengthSym ? mergedHeadSegmentLengthSym : predecessorHeadSegmentLengthSym,
                    mergedLengthSym ? mergedLengthSym : predecessorLengthSym,
                    predecessorValueInfo->GetSymStore())
            });
        }
    }

    // Once we've compensated all the symbols, update the new ValueInfo.
    delayChangeValueInfo.Map([predecessor, this](int, DelayChangeValueInfo d) { d.ChangeValueInfo(predecessor, this); });

    if(setLastInstrInPredecessor)
    {
        predecessor->SetLastInstr(insertBeforeInstr->m_prev);
    }
}

bool
GlobOpt::AreFromSameBytecodeFunc(IR::RegOpnd const* src1, IR::RegOpnd const* dst) const
{
    Assert(this->func->m_symTable->FindStackSym(src1->m_sym->m_id) == src1->m_sym);
    Assert(this->func->m_symTable->FindStackSym(dst->m_sym->m_id) == dst->m_sym);
    if (dst->m_sym->HasByteCodeRegSlot() && src1->m_sym->HasByteCodeRegSlot())
    {
        return src1->m_sym->GetByteCodeFunc() == dst->m_sym->GetByteCodeFunc();
    }
    return false;
}

/*
*   This is for scope object removal along with Heap Arguments optimization.
*   We track several instructions to facilitate the removal of scope object.
*   - LdSlotArr - This instr is tracked to keep track of the formals array (the dest)
*   - InlineeStart - To keep track of the stack syms for the formals of the inlinee.
*/
void
GlobOpt::TrackInstrsForScopeObjectRemoval(IR::Instr * instr)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();

    if (instr->m_opcode == Js::OpCode::Ld_A && src1->IsRegOpnd())
    {
        AssertMsg(!instr->m_func->IsStackArgsEnabled() || !src1->IsScopeObjOpnd(instr->m_func), "There can be no aliasing for scope object.");
    }

    // The following is to track formals array for Stack Arguments optimization with Formals
    if (instr->m_func->IsStackArgsEnabled() && !this->IsLoopPrePass())
    {
        if (instr->m_opcode == Js::OpCode::LdSlotArr)
        {
            if (instr->GetSrc1()->IsScopeObjOpnd(instr->m_func))
            {
                AssertMsg(!instr->m_func->GetJITFunctionBody()->HasImplicitArgIns(), "No mapping is required in this case. So it should already be generating ArgIns.");
                instr->m_func->TrackFormalsArraySym(dst->GetStackSym()->m_id);
            }
        }
        else if (instr->m_opcode == Js::OpCode::InlineeStart)
        {
            Assert(instr->m_func->IsInlined());
            Js::ArgSlot actualsCount = instr->m_func->actualCount - 1;
            Js::ArgSlot formalsCount = instr->m_func->GetJITFunctionBody()->GetInParamsCount() - 1;

            Func * func = instr->m_func;
            Func * inlinerFunc = func->GetParentFunc(); //Inliner's func

            IR::Instr * argOutInstr = instr->GetSrc2()->GetStackSym()->GetInstrDef();

            //The argout immediately before the InlineeStart will be the ArgOut for NewScObject
            //So we don't want to track the stack sym for this argout.- Skipping it here.
            if (instr->m_func->IsInlinedConstructor())
            {
                //PRE might introduce a second defintion for the Src1. So assert for the opcode only when it has single definition.
                Assert(argOutInstr->GetSrc1()->GetStackSym()->GetInstrDef() == nullptr ||
                    argOutInstr->GetSrc1()->GetStackSym()->GetInstrDef()->m_opcode == Js::OpCode::NewScObjectNoCtor);
                argOutInstr = argOutInstr->GetSrc2()->GetStackSym()->GetInstrDef();
            }
            if (formalsCount < actualsCount)
            {
                Js::ArgSlot extraActuals = actualsCount - formalsCount;

                //Skipping extra actuals passed
                for (Js::ArgSlot i = 0; i < extraActuals; i++)
                {
                    argOutInstr = argOutInstr->GetSrc2()->GetStackSym()->GetInstrDef();
                }
            }

            StackSym * undefinedSym = nullptr;

            for (Js::ArgSlot param = formalsCount; param > 0; param--)
            {
                StackSym * argOutSym = nullptr;

                if (argOutInstr->GetSrc1())
                {
                    if (argOutInstr->GetSrc1()->IsRegOpnd())
                    {
                        argOutSym = argOutInstr->GetSrc1()->GetStackSym();
                    }
                    else
                    {
                        // We will always have ArgOut instr - so the source operand will not be removed.
                        argOutSym = StackSym::New(inlinerFunc);
                        IR::Opnd * srcOpnd = argOutInstr->GetSrc1();
                        IR::Opnd * dstOpnd = IR::RegOpnd::New(argOutSym, TyVar, inlinerFunc);
                        IR::Instr * assignInstr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, inlinerFunc);
                        instr->InsertBefore(assignInstr);
                    }
                }

                Assert(!func->HasStackSymForFormal(param - 1));

                if (param <= actualsCount)
                {
                    Assert(argOutSym);
                    func->TrackStackSymForFormalIndex(param - 1, argOutSym);
                    argOutInstr = argOutInstr->GetSrc2()->GetStackSym()->GetInstrDef();
                }
                else
                {
                    /*When param is out of range of actuals count, load undefined*/
                    // TODO: saravind: This will insert undefined for each of the param not having an actual. - Clean up this by having a sym for undefined on func ?
                    Assert(formalsCount > actualsCount);
                    if (undefinedSym == nullptr)
                    {
                        undefinedSym = StackSym::New(inlinerFunc);
                        IR::Opnd * srcOpnd = IR::AddrOpnd::New(inlinerFunc->GetScriptContextInfo()->GetUndefinedAddr(), IR::AddrOpndKindDynamicMisc, inlinerFunc);
                        IR::Opnd * dstOpnd = IR::RegOpnd::New(undefinedSym, TyVar, inlinerFunc);
                        IR::Instr * assignUndefined = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, inlinerFunc);
                        instr->InsertBefore(assignUndefined);
                    }
                    func->TrackStackSymForFormalIndex(param - 1, undefinedSym);
                }
            }
        }
    }
}

void
GlobOpt::OptArguments(IR::Instr *instr)
{
    IR::Opnd* dst = instr->GetDst();
    IR::Opnd* src1 = instr->GetSrc1();
    IR::Opnd* src2 = instr->GetSrc2();

    TrackInstrsForScopeObjectRemoval(instr);

    if (!TrackArgumentsObject())
    {
        return;
    }

    if (instr->HasAnyLoadHeapArgsOpCode())
    {
        if (instr->m_func->IsStackArgsEnabled())
        {
            if (instr->GetSrc1()->IsRegOpnd() && instr->m_func->GetJITFunctionBody()->GetInParamsCount() > 1)
            {
                StackSym * scopeObjSym = instr->GetSrc1()->GetStackSym();
                Assert(scopeObjSym);
                Assert(scopeObjSym->GetInstrDef()->m_opcode == Js::OpCode::InitCachedScope || scopeObjSym->GetInstrDef()->m_opcode == Js::OpCode::NewScopeObject);
                Assert(instr->m_func->GetScopeObjSym() == scopeObjSym);

                if (PHASE_VERBOSE_TRACE1(Js::StackArgFormalsOptPhase))
                {
                    Output::Print(_u("StackArgFormals : %s (%d) :Setting scopeObjSym in forward pass. \n"), instr->m_func->GetJITFunctionBody()->GetDisplayName(), instr->m_func->GetJITFunctionBody()->GetFunctionNumber());
                    Output::Flush();
                }
            }
        }

        if (instr->m_func->GetJITFunctionBody()->GetInParamsCount() != 1 && !instr->m_func->IsStackArgsEnabled())
        {
            CannotAllocateArgumentsObjectOnStack();
        }
        else
        {
            CurrentBlockData()->TrackArgumentsSym(dst->AsRegOpnd());
        }
        return;
    }
    // Keep track of arguments objects and its aliases
    // LdHeapArguments loads the arguments object and Ld_A tracks the aliases.
    if ((instr->m_opcode == Js::OpCode::Ld_A || instr->m_opcode == Js::OpCode::BytecodeArgOutCapture) && (src1->IsRegOpnd() && CurrentBlockData()->IsArgumentsOpnd(src1)))
    {
        // In the debug mode, we don't want to optimize away the aliases. Since we may have to show them on the inspection.
        if (((!AreFromSameBytecodeFunc(src1->AsRegOpnd(), dst->AsRegOpnd()) || this->currentBlock->loop) && instr->m_opcode != Js::OpCode::BytecodeArgOutCapture) || this->func->IsJitInDebugMode())
        {
            CannotAllocateArgumentsObjectOnStack();
            return;
        }
        if(!dst->AsRegOpnd()->GetStackSym()->m_nonEscapingArgObjAlias)
        {
            CurrentBlockData()->TrackArgumentsSym(dst->AsRegOpnd());
        }
        return;
    }

    if (!CurrentBlockData()->TestAnyArgumentsSym())
    {
        // There are no syms to track yet, don't start tracking arguments sym.
        return;
    }

    // Avoid loop prepass
    if (this->currentBlock->loop && this->IsLoopPrePass())
    {
        return;
    }

    SymID id = 0;

    switch(instr->m_opcode)
    {
    case Js::OpCode::LdElemI_A:
    case Js::OpCode::TypeofElem:
    {
        Assert(src1->IsIndirOpnd());
        IR::RegOpnd *indexOpnd = src1->AsIndirOpnd()->GetIndexOpnd();
        if (indexOpnd && CurrentBlockData()->IsArgumentsSymID(indexOpnd->m_sym->m_id))
        {
            // Pathological test cases such as a[arguments]
            CannotAllocateArgumentsObjectOnStack();
            return;
        }

        IR::RegOpnd *baseOpnd = src1->AsIndirOpnd()->GetBaseOpnd();
        id = baseOpnd->m_sym->m_id;
        if (CurrentBlockData()->IsArgumentsSymID(id))
        {
            instr->usesStackArgumentsObject = true;
        }

        break;
    }
    case Js::OpCode::LdLen_A:
    {
        Assert(src1->IsRegOpnd());
        if(CurrentBlockData()->IsArgumentsOpnd(src1))
        {
            instr->usesStackArgumentsObject = true;
        }
        break;
    }
    case Js::OpCode::ArgOut_A_InlineBuiltIn:
    {
        if (CurrentBlockData()->IsArgumentsOpnd(src1))
        {
            instr->usesStackArgumentsObject = true;
        }

        if (CurrentBlockData()->IsArgumentsOpnd(src1) &&
            src1->AsRegOpnd()->m_sym->GetInstrDef()->m_opcode == Js::OpCode::BytecodeArgOutCapture)
        {
            // Apply inlining results in such usage - this is to ignore this sym that is def'd by ByteCodeArgOutCapture
            // It's needed because we do not have block level merging of arguments object and this def due to inlining can turn off stack args opt.
            IR::Instr* builtinStart = instr->GetNextRealInstr();
            if (builtinStart->m_opcode == Js::OpCode::InlineBuiltInStart)
            {
                IR::Opnd* builtinOpnd = builtinStart->GetSrc1();
                if (builtinStart->GetSrc1()->IsAddrOpnd())
                {
                    Assert(builtinOpnd->AsAddrOpnd()->m_isFunction);

                    Js::BuiltinFunction builtinFunction = Js::JavascriptLibrary::GetBuiltInForFuncInfo(((FixedFieldInfo*)builtinOpnd->AsAddrOpnd()->m_metadata)->GetFuncInfoAddr(), func->GetThreadContextInfo());
                    if (builtinFunction == Js::BuiltinFunction::JavascriptFunction_Apply)
                    {
                        CurrentBlockData()->ClearArgumentsSym(src1->AsRegOpnd());
                    }
                }
                else if (builtinOpnd->IsRegOpnd())
                {
                    if (builtinOpnd->AsRegOpnd()->m_sym->m_builtInIndex == Js::BuiltinFunction::JavascriptFunction_Apply)
                    {
                        CurrentBlockData()->ClearArgumentsSym(src1->AsRegOpnd());
                    }
                }
            }
        }
        break;
    }
    case Js::OpCode::BailOnNotStackArgs:
    case Js::OpCode::ArgOut_A_FromStackArgs:
    case Js::OpCode::BytecodeArgOutUse:
    {
        if (src1 && CurrentBlockData()->IsArgumentsOpnd(src1))
        {
            instr->usesStackArgumentsObject = true;
        }

        break;
    }

    default:
        {
            // Super conservative here, if we see the arguments or any of its alias being used in any
            // other opcode just don't do this optimization. Revisit this to optimize further if we see any common
            // case is missed.

            if (src1)
            {
                if (src1->IsRegOpnd() || src1->IsSymOpnd() || src1->IsIndirOpnd())
                {
                    if (CurrentBlockData()->IsArgumentsOpnd(src1))
                    {
#ifdef PERF_HINT
                        if (PHASE_TRACE1(Js::PerfHintPhase))
                        {
                            WritePerfHint(PerfHints::HeapArgumentsCreated, instr->m_func, instr->GetByteCodeOffset());
                        }
#endif
                        CannotAllocateArgumentsObjectOnStack();
                        return;
                    }
                }
            }

            if (src2)
            {
                if (src2->IsRegOpnd() || src2->IsSymOpnd() || src2->IsIndirOpnd())
                {
                    if (CurrentBlockData()->IsArgumentsOpnd(src2))
                    {
#ifdef PERF_HINT
                        if (PHASE_TRACE1(Js::PerfHintPhase))
                        {
                            WritePerfHint(PerfHints::HeapArgumentsCreated, instr->m_func, instr->GetByteCodeOffset());
                        }
#endif
                        CannotAllocateArgumentsObjectOnStack();
                        return;
                    }
                }
            }

            // We should look at dst last to correctly handle cases where it's the same as one of the src operands.
            if (dst)
            {
                if (dst->IsIndirOpnd() || dst->IsSymOpnd())
                {
                    if (CurrentBlockData()->IsArgumentsOpnd(dst))
                    {
#ifdef PERF_HINT
                        if (PHASE_TRACE1(Js::PerfHintPhase))
                        {
                            WritePerfHint(PerfHints::HeapArgumentsModification, instr->m_func, instr->GetByteCodeOffset());
                        }
#endif
                        CannotAllocateArgumentsObjectOnStack();
                        return;
                    }
                }
                else if (dst->IsRegOpnd())
                {
                    if (this->currentBlock->loop && CurrentBlockData()->IsArgumentsOpnd(dst))
                    {
#ifdef PERF_HINT
                        if (PHASE_TRACE1(Js::PerfHintPhase))
                        {
                            WritePerfHint(PerfHints::HeapArgumentsModification, instr->m_func, instr->GetByteCodeOffset());
                        }
#endif
                        CannotAllocateArgumentsObjectOnStack();
                        return;
                    }
                    CurrentBlockData()->ClearArgumentsSym(dst->AsRegOpnd());
                }
            }
        }
        break;
    }
    return;
}

void
GlobOpt::MarkArgumentsUsedForBranch(IR::Instr * instr)
{
    // If it's a conditional branch instruction and the operand used for branching is one of the arguments
    // to the function, tag the m_argUsedForBranch of the functionBody so that it can be used later for inlining decisions.
    if (instr->IsBranchInstr() && !instr->AsBranchInstr()->IsUnconditional())
    {
        IR::BranchInstr * bInstr = instr->AsBranchInstr();
        IR::Opnd *src1 = bInstr->GetSrc1();
        IR::Opnd *src2 = bInstr->GetSrc2();
        // These are used because we don't want to rely on src1 or src2 to always be the register/constant
        IR::RegOpnd *regOpnd = nullptr;
        if (!src2 && (instr->m_opcode == Js::OpCode::BrFalse_A || instr->m_opcode == Js::OpCode::BrTrue_A) && src1->IsRegOpnd())
        {
            regOpnd = src1->AsRegOpnd();
        }
        // We need to check for (0===arg) and (arg===0); this is especially important since some minifiers
        // change all instances of one to the other.
        else if (src2 && src2->IsConstOpnd() && src1->IsRegOpnd())
        {
            regOpnd = src1->AsRegOpnd();
        }
        else if (src2 && src2->IsRegOpnd() && src1->IsConstOpnd())
        {
            regOpnd = src2->AsRegOpnd();
        }
        if (regOpnd != nullptr)
        {
            if (regOpnd->m_sym->IsSingleDef())
            {
                IR::Instr * defInst = regOpnd->m_sym->GetInstrDef();
                IR::Opnd *defSym = defInst->GetSrc1();
                if (defSym && defSym->IsSymOpnd() && defSym->AsSymOpnd()->m_sym->IsStackSym()
                    && defSym->AsSymOpnd()->m_sym->AsStackSym()->IsParamSlotSym())
                {
                    uint16 param = defSym->AsSymOpnd()->m_sym->AsStackSym()->GetParamSlotNum();

                    // We only support functions with 13 arguments to ensure optimal size of callSiteInfo
                    if (param < Js::Constants::MaximumArgumentCountForConstantArgumentInlining)
                    {
                        this->func->GetJITOutput()->SetArgUsedForBranch((uint8)param);
                    }
                }
            }
        }
    }
}

const InductionVariable*
GlobOpt::GetInductionVariable(SymID sym, Loop *loop)
{
    if (loop->inductionVariables)
    {
        for (auto it = loop->inductionVariables->GetIterator(); it.IsValid(); it.MoveNext())
        {
            InductionVariable* iv = &it.CurrentValueReference();
            if (!iv->IsChangeDeterminate() || !iv->IsChangeUnidirectional())
            {
                continue;
            }
            if (iv->Sym()->m_id == sym)
            {
                return iv;
            }
        }
    }
    return nullptr;
}

bool
GlobOpt::IsSymIDInductionVariable(SymID sym, Loop *loop)
{
    return GetInductionVariable(sym, loop) != nullptr;
}

SymID
GlobOpt::GetVarSymID(StackSym *sym)
{
    if (sym && sym->m_type != TyVar)
    {
        sym = sym->GetVarEquivSym(nullptr);
    }
    if (!sym)
    {
        return Js::Constants::InvalidSymID;
    }
    return sym->m_id;
}

bool
GlobOpt::IsAllowedForMemOpt(IR::Instr* instr, bool isMemset, IR::RegOpnd *baseOpnd, IR::Opnd *indexOpnd)
{
    Assert(instr);
    if (!baseOpnd || !indexOpnd)
    {
        return false;
    }
    Loop* loop = this->currentBlock->loop;

    const ValueType baseValueType(baseOpnd->GetValueType());
    const ValueType indexValueType(indexOpnd->GetValueType());

    // Validate the array and index types
    if (
        !indexValueType.IsInt() ||
            !(
                baseValueType.IsTypedIntOrFloatArray() ||
                baseValueType.IsArray()
            )
        )
    {
#if DBG_DUMP
        wchar indexValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
        indexValueType.ToString(indexValueTypeStr);
        wchar baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
        baseValueType.ToString(baseValueTypeStr);
        TRACE_MEMOP_VERBOSE(loop, instr, _u("Index[%s] or Array[%s] value type is invalid"), indexValueTypeStr, baseValueTypeStr);
#endif
        return false;
    }

    // The following is conservative and works around a bug in induction variable analysis.
    if (baseOpnd->IsArrayRegOpnd())
    {
        IR::ArrayRegOpnd *baseArrayOp = baseOpnd->AsArrayRegOpnd();
        bool hasBoundChecksRemoved = (
            baseArrayOp->EliminatedLowerBoundCheck() &&
            baseArrayOp->EliminatedUpperBoundCheck() &&
            !instr->extractedUpperBoundCheckWithoutHoisting &&
            !instr->loadedArrayHeadSegment &&
            !instr->loadedArrayHeadSegmentLength
            );
        if (!hasBoundChecksRemoved)
        {
            TRACE_MEMOP_VERBOSE(loop, instr, _u("Missing bounds check optimization"));
            return false;
        }
    }

    if (!baseValueType.IsTypedArray())
    {
        // Check if the instr can kill the value type of the array
        JsArrayKills arrayKills = CheckJsArrayKills(instr);
        if (arrayKills.KillsValueType(baseValueType))
        {
            TRACE_MEMOP_VERBOSE(loop, instr, _u("The array (s%d) can lose its value type"), GetVarSymID(baseOpnd->GetStackSym()));
            return false;
        }
    }

    // Process the Index Operand
    if (!this->OptIsInvariant(baseOpnd, this->currentBlock, loop, CurrentBlockData()->FindValue(baseOpnd->m_sym), false, true))
    {
        TRACE_MEMOP_VERBOSE(loop, instr, _u("Base (s%d) is not invariant"), GetVarSymID(baseOpnd->GetStackSym()));
        return false;
    }

    // Validate the index
    Assert(indexOpnd->GetStackSym());
    SymID indexSymID = GetVarSymID(indexOpnd->GetStackSym());
    const InductionVariable* iv = GetInductionVariable(indexSymID, loop);
    if (!iv)
    {
        // If the index is not an induction variable return
        TRACE_MEMOP_VERBOSE(loop, instr, _u("Index (s%d) is not an induction variable"), indexSymID);
        return false;
    }

    Assert(iv->IsChangeDeterminate() && iv->IsChangeUnidirectional());
    const IntConstantBounds & bounds = iv->ChangeBounds();

    if (loop->memOpInfo)
    {
        // Only accept induction variables that increments by 1
        Loop::InductionVariableChangeInfo inductionVariableChangeInfo = { 0, 0 };
        inductionVariableChangeInfo = loop->memOpInfo->inductionVariableChangeInfoMap->Lookup(indexSymID, inductionVariableChangeInfo);

        if (
            (bounds.LowerBound() != 1 && bounds.LowerBound() != -1) ||
            (bounds.UpperBound() != bounds.LowerBound()) ||
            inductionVariableChangeInfo.unroll > 1 // Must be 0 (not seen yet) or 1 (already seen)
        )
        {
            TRACE_MEMOP_VERBOSE(loop, instr, _u("The index does not change by 1: %d><%d, unroll=%d"), bounds.LowerBound(), bounds.UpperBound(), inductionVariableChangeInfo.unroll);
            return false;
        }

        // Check if the index is the same in all MemOp optimization in this loop
        if (!loop->memOpInfo->candidates->Empty())
        {
            Loop::MemOpCandidate* previousCandidate = loop->memOpInfo->candidates->Head();

            // All MemOp operations within the same loop must use the same index
            if (previousCandidate->index != indexSymID)
            {
                TRACE_MEMOP_VERBOSE(loop, instr, _u("The index is not the same as other MemOp in the loop"));
                return false;
            }
        }
    }

    return true;
}

bool
GlobOpt::CollectMemcopyLdElementI(IR::Instr *instr, Loop *loop)
{
    Assert(instr->GetSrc1()->IsIndirOpnd());

    IR::IndirOpnd *src1 = instr->GetSrc1()->AsIndirOpnd();
    IR::Opnd *indexOpnd = src1->GetIndexOpnd();
    IR::RegOpnd *baseOpnd = src1->GetBaseOpnd()->AsRegOpnd();
    SymID baseSymID = GetVarSymID(baseOpnd->GetStackSym());

    if (!IsAllowedForMemOpt(instr, false, baseOpnd, indexOpnd))
    {
        return false;
    }

    SymID inductionSymID = GetVarSymID(indexOpnd->GetStackSym());
    Assert(IsSymIDInductionVariable(inductionSymID, loop));

    loop->EnsureMemOpVariablesInitialized();
    bool isIndexPreIncr = loop->memOpInfo->inductionVariableChangeInfoMap->ContainsKey(inductionSymID);

    IR::Opnd * dst = instr->GetDst();
    if (!dst->IsRegOpnd() || !dst->AsRegOpnd()->GetStackSym()->IsSingleDef())
    {
        return false;
    }

    Loop::MemCopyCandidate* memcopyInfo = memcopyInfo = JitAnewStruct(this->func->GetTopFunc()->m_fg->alloc, Loop::MemCopyCandidate);
    memcopyInfo->ldBase = baseSymID;
    memcopyInfo->ldCount = 1;
    memcopyInfo->count = 0;
    memcopyInfo->bIndexAlreadyChanged = isIndexPreIncr;
    memcopyInfo->base = Js::Constants::InvalidSymID; //need to find the stElem first
    memcopyInfo->index = inductionSymID;
    memcopyInfo->transferSym = dst->AsRegOpnd()->GetStackSym();
    loop->memOpInfo->candidates->Prepend(memcopyInfo);
    return true;
}

bool
GlobOpt::CollectMemsetStElementI(IR::Instr *instr, Loop *loop)
{
    Assert(instr->GetDst()->IsIndirOpnd());
    IR::IndirOpnd *dst = instr->GetDst()->AsIndirOpnd();
    IR::Opnd *indexOp = dst->GetIndexOpnd();
    IR::RegOpnd *baseOp = dst->GetBaseOpnd()->AsRegOpnd();

    if (!IsAllowedForMemOpt(instr, true, baseOp, indexOp))
    {
        return false;
    }

    SymID baseSymID = GetVarSymID(baseOp->GetStackSym());

    IR::Opnd *srcDef = instr->GetSrc1();
    StackSym *srcSym = nullptr;
    if (srcDef->IsRegOpnd())
    {
        IR::RegOpnd* opnd = srcDef->AsRegOpnd();
        if (this->OptIsInvariant(opnd, this->currentBlock, loop, CurrentBlockData()->FindValue(opnd->m_sym), true, true))
        {
            srcSym = opnd->GetStackSym();
        }
    }

    BailoutConstantValue constant = {TyIllegal, 0};
    if (srcDef->IsFloatConstOpnd())
    {
        constant.InitFloatConstValue(srcDef->AsFloatConstOpnd()->m_value);
    }
    else if (srcDef->IsIntConstOpnd())
    {
        constant.InitIntConstValue(srcDef->AsIntConstOpnd()->GetValue(), srcDef->AsIntConstOpnd()->GetType());
    }
    else if (srcDef->IsAddrOpnd())
    {
        constant.InitVarConstValue(srcDef->AsAddrOpnd()->m_address);
    }
    else if(!srcSym)
    {
        TRACE_MEMOP_PHASE_VERBOSE(MemSet, loop, instr, _u("Source is not an invariant"));
        return false;
    }

    // Process the Index Operand
    Assert(indexOp->GetStackSym());
    SymID inductionSymID = GetVarSymID(indexOp->GetStackSym());
    Assert(IsSymIDInductionVariable(inductionSymID, loop));

    loop->EnsureMemOpVariablesInitialized();
    bool isIndexPreIncr = loop->memOpInfo->inductionVariableChangeInfoMap->ContainsKey(inductionSymID);

    Loop::MemSetCandidate* memsetInfo = JitAnewStruct(this->func->GetTopFunc()->m_fg->alloc, Loop::MemSetCandidate);
    memsetInfo->base = baseSymID;
    memsetInfo->index = inductionSymID;
    memsetInfo->constant = constant;
    memsetInfo->srcSym = srcSym;
    memsetInfo->count = 1;
    memsetInfo->bIndexAlreadyChanged = isIndexPreIncr;
    loop->memOpInfo->candidates->Prepend(memsetInfo);
    return true;
}

bool GlobOpt::CollectMemcopyStElementI(IR::Instr *instr, Loop *loop)
{
    if (!loop->memOpInfo || loop->memOpInfo->candidates->Empty())
    {
        // There is no ldElem matching this stElem
        return false;
    }

    Assert(instr->GetDst()->IsIndirOpnd());
    IR::IndirOpnd *dst = instr->GetDst()->AsIndirOpnd();
    IR::Opnd *indexOp = dst->GetIndexOpnd();
    IR::RegOpnd *baseOp = dst->GetBaseOpnd()->AsRegOpnd();
    SymID baseSymID = GetVarSymID(baseOp->GetStackSym());

    if (!instr->GetSrc1()->IsRegOpnd())
    {
        return false;
    }
    IR::RegOpnd* src1 = instr->GetSrc1()->AsRegOpnd();

    if (!src1->GetIsDead())
    {
        // This must be the last use of the register.
        // It will invalidate `var m = a[i]; b[i] = m;` but this is not a very interesting case.
        TRACE_MEMOP_PHASE_VERBOSE(MemCopy, loop, instr, _u("Source (s%d) is still alive after StElemI"), baseSymID);
        return false;
    }

    if (!IsAllowedForMemOpt(instr, false, baseOp, indexOp))
    {
        return false;
    }

    SymID srcSymID = GetVarSymID(src1->GetStackSym());

    // Prepare the memcopyCandidate entry
    Loop::MemOpCandidate* previousCandidate = loop->memOpInfo->candidates->Head();
    if (!previousCandidate->IsMemCopy())
    {
        return false;
    }
    Loop::MemCopyCandidate* memcopyInfo = previousCandidate->AsMemCopy();

    // The previous candidate has to have been created by the matching ldElem
    if (
        memcopyInfo->base != Js::Constants::InvalidSymID ||
        GetVarSymID(memcopyInfo->transferSym) != srcSymID
    )
    {
        TRACE_MEMOP_PHASE_VERBOSE(MemCopy, loop, instr, _u("No matching LdElem found (s%d)"), baseSymID);
        return false;
    }

    Assert(indexOp->GetStackSym());
    SymID inductionSymID = GetVarSymID(indexOp->GetStackSym());
    Assert(IsSymIDInductionVariable(inductionSymID, loop));
    bool isIndexPreIncr = loop->memOpInfo->inductionVariableChangeInfoMap->ContainsKey(inductionSymID);
    if (isIndexPreIncr != memcopyInfo->bIndexAlreadyChanged)
    {
        // The index changed between the load and the store
        TRACE_MEMOP_PHASE_VERBOSE(MemCopy, loop, instr, _u("Index value changed between ldElem and stElem"));
        return false;
    }

    // Consider: Can we remove the count field?
    memcopyInfo->count++;
    memcopyInfo->base = baseSymID;

    return true;
}

bool
GlobOpt::CollectMemOpLdElementI(IR::Instr *instr, Loop *loop)
{
    Assert(instr->m_opcode == Js::OpCode::LdElemI_A);
    return (!PHASE_OFF(Js::MemCopyPhase, this->func) && CollectMemcopyLdElementI(instr, loop));
}

bool
GlobOpt::CollectMemOpStElementI(IR::Instr *instr, Loop *loop)
{
    Assert(instr->m_opcode == Js::OpCode::StElemI_A || instr->m_opcode == Js::OpCode::StElemI_A_Strict);
    Assert(instr->GetSrc1());
    return (!PHASE_OFF(Js::MemSetPhase, this->func) && CollectMemsetStElementI(instr, loop)) ||
        (!PHASE_OFF(Js::MemCopyPhase, this->func) && CollectMemcopyStElementI(instr, loop));
}

bool
GlobOpt::CollectMemOpInfo(IR::Instr *instrBegin, IR::Instr *instr, Value *src1Val, Value *src2Val)
{
    Assert(this->currentBlock->loop);

    Loop *loop = this->currentBlock->loop;

    if (!loop->blockList.HasTwo())
    {
        //  We support memcopy and memset for loops which have only two blocks.
        return false;
    }

    if (loop->GetLoopFlags().isInterpreted && !loop->GetLoopFlags().memopMinCountReached)
    {
        TRACE_MEMOP_VERBOSE(loop, instr, _u("minimum loop count not reached"))
        loop->doMemOp = false;
        return false;
    }
    Assert(loop->doMemOp);

    bool isIncr = true, isChangedByOne = false;

    switch (instr->m_opcode)
    {
    case Js::OpCode::StElemI_A:
    case Js::OpCode::StElemI_A_Strict:
        if (!CollectMemOpStElementI(instr, loop))
        {
            loop->doMemOp = false;
            return false;
        }
        break;
    case Js::OpCode::LdElemI_A:
        if (!CollectMemOpLdElementI(instr, loop))
        {
            loop->doMemOp = false;
            return false;
        }
        break;
    case Js::OpCode::Decr_A:
        isIncr = false;
    case Js::OpCode::Incr_A:
        isChangedByOne = true;
        goto MemOpCheckInductionVariable;
    case Js::OpCode::Sub_I4:
    case Js::OpCode::Sub_A:
        isIncr = false;
    case Js::OpCode::Add_A:
    case Js::OpCode::Add_I4:
    {
MemOpCheckInductionVariable:
        StackSym *sym = instr->GetSrc1()->GetStackSym();
        if (!sym)
        {
            sym = instr->GetSrc2()->GetStackSym();
        }

        SymID inductionSymID = GetVarSymID(sym);

        if (IsSymIDInductionVariable(inductionSymID, this->currentBlock->loop))
        {
            if (!isChangedByOne)
            {
                IR::Opnd *src1, *src2;
                src1 = instr->GetSrc1();
                src2 = instr->GetSrc2();

                if (src2->IsRegOpnd())
                {
                    Value *val = CurrentBlockData()->FindValue(src2->AsRegOpnd()->m_sym);
                    if (val)
                    {
                        ValueInfo *vi = val->GetValueInfo();
                        int constValue;
                        if (vi && vi->TryGetIntConstantValue(&constValue))
                        {
                            if (constValue == 1)
                            {
                                isChangedByOne = true;
                            }
                        }
                    }
                }
                else if (src2->IsIntConstOpnd())
                {
                    if (src2->AsIntConstOpnd()->GetValue() == 1)
                    {
                        isChangedByOne = true;
                    }
                }
            }

            loop->EnsureMemOpVariablesInitialized();
            if (!isChangedByOne)
            {
                Loop::InductionVariableChangeInfo inductionVariableChangeInfo = { Js::Constants::InvalidLoopUnrollFactor, 0 };

                if (!loop->memOpInfo->inductionVariableChangeInfoMap->ContainsKey(inductionSymID))
                {
                    loop->memOpInfo->inductionVariableChangeInfoMap->Add(inductionSymID, inductionVariableChangeInfo);
                }
                else
                {
                    loop->memOpInfo->inductionVariableChangeInfoMap->Item(inductionSymID, inductionVariableChangeInfo);
                }
            }
            else
            {
                if (!loop->memOpInfo->inductionVariableChangeInfoMap->ContainsKey(inductionSymID))
                {
                    Loop::InductionVariableChangeInfo inductionVariableChangeInfo = { 1, isIncr };
                    loop->memOpInfo->inductionVariableChangeInfoMap->Add(inductionSymID, inductionVariableChangeInfo);
                }
                else
                {
                    Loop::InductionVariableChangeInfo inductionVariableChangeInfo = { 0, 0 };
                    inductionVariableChangeInfo = loop->memOpInfo->inductionVariableChangeInfoMap->Lookup(inductionSymID, inductionVariableChangeInfo);
                    inductionVariableChangeInfo.unroll++;
                    inductionVariableChangeInfo.isIncremental = isIncr;
                    loop->memOpInfo->inductionVariableChangeInfoMap->Item(inductionSymID, inductionVariableChangeInfo);
                }
            }
            break;
        }
        // Fallthrough if not an induction variable
    }
    default:
        FOREACH_INSTR_IN_RANGE(chkInstr, instrBegin->m_next, instr)
        {
            if (IsInstrInvalidForMemOp(chkInstr, loop, src1Val, src2Val))
            {
                loop->doMemOp = false;
                return false;
            }

            // Make sure this instruction doesn't use the memcopy transfer sym before it is checked by StElemI
            if (loop->memOpInfo && !loop->memOpInfo->candidates->Empty())
            {
                Loop::MemOpCandidate* prevCandidate = loop->memOpInfo->candidates->Head();
                if (prevCandidate->IsMemCopy())
                {
                    Loop::MemCopyCandidate* memcopyCandidate = prevCandidate->AsMemCopy();
                    if (memcopyCandidate->base == Js::Constants::InvalidSymID)
                    {
                        if (chkInstr->FindRegUse(memcopyCandidate->transferSym))
                        {
                            loop->doMemOp = false;
                            TRACE_MEMOP_PHASE_VERBOSE(MemCopy, loop, chkInstr, _u("Found illegal use of LdElemI value(s%d)"), GetVarSymID(memcopyCandidate->transferSym));
                            return false;
                        }
                    }
                }
            }
        }
        NEXT_INSTR_IN_RANGE;
    }

    return true;
}

bool
GlobOpt::IsInstrInvalidForMemOp(IR::Instr *instr, Loop *loop, Value *src1Val, Value *src2Val)
{
    // List of instruction that are valid with memop (ie: instr that gets removed if memop is emitted)
    if (
        this->currentBlock != loop->GetHeadBlock() &&
        !instr->IsLabelInstr() &&
        instr->IsRealInstr() &&
        instr->m_opcode != Js::OpCode::IncrLoopBodyCount &&
        instr->m_opcode != Js::OpCode::StLoopBodyCount &&
        instr->m_opcode != Js::OpCode::Ld_A &&
        instr->m_opcode != Js::OpCode::Ld_I4 &&
        !(instr->IsBranchInstr() && instr->AsBranchInstr()->IsUnconditional())
    )
    {
        TRACE_MEMOP_VERBOSE(loop, instr, _u("Instruction not accepted for memop"));
        return true;
    }

    // Check prev instr because it could have been added by an optimization and we won't see it here.
    if (OpCodeAttr::FastFldInstr(instr->m_opcode) || (instr->m_prev && OpCodeAttr::FastFldInstr(instr->m_prev->m_opcode)))
    {
        // Refuse any operations interacting with Fields
        TRACE_MEMOP_VERBOSE(loop, instr, _u("Field interaction detected"));
        return true;
    }

    if (Js::OpCodeUtil::GetOpCodeLayout(instr->m_opcode) == Js::OpLayoutType::ElementSlot)
    {
        // Refuse any operations interacting with slots
        TRACE_MEMOP_VERBOSE(loop, instr, _u("Slot interaction detected"));
        return true;
    }

    if (this->MayNeedBailOnImplicitCall(instr, src1Val, src2Val))
    {
        TRACE_MEMOP_VERBOSE(loop, instr, _u("Implicit call bailout detected"));
        return true;
    }

    return false;
}

void
GlobOpt::TryReplaceLdLen(IR::Instr *& instr)
{
    // Change LdFld on arrays, strings, and 'arguments' to LdLen when we're accessing the .length field
    if ((instr->GetSrc1() && instr->GetSrc1()->IsSymOpnd() && instr->m_opcode == Js::OpCode::ProfiledLdFld) || instr->m_opcode == Js::OpCode::LdFld || instr->m_opcode == Js::OpCode::ScopedLdFld)
    {
        IR::SymOpnd * opnd = instr->GetSrc1()->AsSymOpnd();
        Sym *sym = opnd->m_sym;
        if (sym->IsPropertySym())
        {
            PropertySym *originalPropertySym = sym->AsPropertySym();
            // only on .length
            if (this->lengthEquivBv != nullptr && this->lengthEquivBv->Test(originalPropertySym->m_id))
            {
                IR::RegOpnd* newopnd = IR::RegOpnd::New(originalPropertySym->m_stackSym, IRType::TyVar, instr->m_func);
                ValueInfo *const objectValueInfo = CurrentBlockData()->FindValue(originalPropertySym->m_stackSym)->GetValueInfo();
                // Only for things we'd emit a fast path for
                if (
                    objectValueInfo->IsLikelyAnyArray() ||
                    objectValueInfo->HasHadStringTag() ||
                    objectValueInfo->IsLikelyString() ||
                    newopnd->IsArgumentsObject() ||
                    (CurrentBlockData()->argObjSyms && CurrentBlockData()->IsArgumentsOpnd(newopnd))
                   )
                {
                    // We need to properly transfer over the information from the old operand, which is
                    // a SymOpnd, to the new one, which is a RegOpnd. Unfortunately, the types mean the
                    // normal copy methods won't work here, so we're going to directly copy data.
                    newopnd->SetIsJITOptimizedReg(opnd->GetIsJITOptimizedReg());
                    newopnd->SetValueType(objectValueInfo->Type());
                    newopnd->SetIsDead(opnd->GetIsDead());

                    // Now that we have the operand we need, we can go ahead and make the new instr.
                    IR::Instr *newinstr = IR::Instr::New(Js::OpCode::LdLen_A, instr->m_func);
                    instr->TransferTo(newinstr);
                    newinstr->UnlinkSrc1();
                    newinstr->SetSrc1(newopnd);
                    instr->InsertAfter(newinstr);
                    instr->Remove();
                    instr = newinstr;
                }
            }
        }
    }
}

IR::Instr *
GlobOpt::OptInstr(IR::Instr *&instr, bool* isInstrRemoved)
{
    Assert(instr->m_func->IsTopFunc() || instr->m_func->isGetterSetter || instr->m_func->callSiteIdInParentFunc != UINT16_MAX);

    IR::Opnd *src1, *src2;
    Value *src1Val = nullptr, *src2Val = nullptr, *dstVal = nullptr;
    Value *src1IndirIndexVal = nullptr, *dstIndirIndexVal = nullptr;
    IR::Instr *instrPrev = instr->m_prev;
    IR::Instr *instrNext = instr->m_next;

    if (instr->IsLabelInstr() && this->func->HasTry() && this->func->DoOptimizeTry())
    {
        this->currentRegion = instr->AsLabelInstr()->GetRegion();
        Assert(this->currentRegion);
    }

    if(PrepareForIgnoringIntOverflow(instr))
    {
        if(!IsLoopPrePass())
        {
            *isInstrRemoved = true;
            currentBlock->RemoveInstr(instr);
        }
        return instrNext;
    }

    if (!instr->IsRealInstr() || instr->IsByteCodeUsesInstr() || instr->m_opcode == Js::OpCode::Conv_Bool)
    {
        return instrNext;
    }

    if (instr->m_opcode == Js::OpCode::Yield)
    {
        // TODO[generators][ianhall]: Can this and the FillBailOutInfo call below be moved to after Src1 and Src2 so that Yield can be optimized right up to the actual yield?
        CurrentBlockData()->KillStateForGeneratorYield();
    }

    // Change LdFld on arrays, strings, and 'arguments' to LdLen when we're accessing the .length field
    this->TryReplaceLdLen(instr);

    // Consider: Do we ever get post-op bailout here, and if so is the FillBailOutInfo call in the right place?
    if (instr->HasBailOutInfo() && !this->IsLoopPrePass())
    {
        this->FillBailOutInfo(this->currentBlock, instr->GetBailOutInfo());
    }

    this->instrCountSinceLastCleanUp++;

    instr = this->PreOptPeep(instr);

    this->OptArguments(instr);

    //StackArguments Optimization - We bail out if the index is out of range of actuals.
    if ((instr->m_opcode == Js::OpCode::LdElemI_A || instr->m_opcode == Js::OpCode::TypeofElem) &&
        instr->DoStackArgsOpt(this->func) && !this->IsLoopPrePass())
    {
        GenerateBailAtOperation(&instr, IR::BailOnStackArgsOutOfActualsRange);
    }

#if DBG
    PropertySym *propertySymUseBefore = nullptr;
    Assert(this->byteCodeUses == nullptr);
    this->byteCodeUsesBeforeOpt->ClearAll();
    GlobOpt::TrackByteCodeSymUsed(instr, this->byteCodeUsesBeforeOpt, &propertySymUseBefore);
    Assert(noImplicitCallUsesToInsert->Count() == 0);
#endif

    this->ignoredIntOverflowForCurrentInstr = false;
    this->ignoredNegativeZeroForCurrentInstr = false;

    src1 = instr->GetSrc1();
    src2 = instr->GetSrc2();

    if (src1)
    {
        src1Val = this->OptSrc(src1, &instr, &src1IndirIndexVal);

        instr = this->SetTypeCheckBailOut(instr->GetSrc1(), instr, nullptr);

        if (src2)
        {
            src2Val = this->OptSrc(src2, &instr);
        }
    }
    if(instr->GetDst() && instr->GetDst()->IsIndirOpnd())
    {
        this->OptSrc(instr->GetDst(), &instr, &dstIndirIndexVal);
    }

    MarkArgumentsUsedForBranch(instr);
    CSEOptimize(this->currentBlock, &instr, &src1Val, &src2Val, &src1IndirIndexVal);
    OptimizeChecks(instr);
    OptArraySrc(&instr);
    OptNewScObject(&instr, src1Val);


    instr = this->OptPeep(instr, src1Val, src2Val);

    if (instr->m_opcode == Js::OpCode::Nop ||
        (instr->m_opcode == Js::OpCode::CheckThis &&
        instr->GetSrc1()->IsRegOpnd() &&
        instr->GetSrc1()->AsRegOpnd()->m_sym->m_isSafeThis))
    {
        instrNext = instr->m_next;
        InsertNoImplicitCallUses(instr);
        if (this->byteCodeUses)
        {
            this->InsertByteCodeUses(instr);
        }
        *isInstrRemoved = true;
        this->currentBlock->RemoveInstr(instr);
        return instrNext;
    }
    else if (instr->m_opcode == Js::OpCode::GetNewScObject && !this->IsLoopPrePass() && src1Val->GetValueInfo()->IsPrimitive())
    {
        // Constructor returned (src1) a primitive value, so fold this into "dst = Ld_A src2", where src2 is the new object that
        // was passed into the constructor as its 'this' parameter
        instr->FreeSrc1();
        instr->SetSrc1(instr->UnlinkSrc2());
        instr->m_opcode = Js::OpCode::Ld_A;
        src1Val = src2Val;
        src2Val = nullptr;
    }
    else if ((instr->m_opcode == Js::OpCode::TryCatch && this->func->DoOptimizeTry()) || (instr->m_opcode == Js::OpCode::TryFinally && this->func->DoOptimizeTry()))
    {
        ProcessTryHandler(instr);
    }
    else if (instr->m_opcode == Js::OpCode::BrOnException || instr->m_opcode == Js::OpCode::BrOnNoException)
    {
        if (this->ProcessExceptionHandlingEdges(instr))
            {
            *isInstrRemoved = true;
            return instrNext;
        }
    }

    bool isAlreadyTypeSpecialized = false;
    if (!IsLoopPrePass() && instr->HasBailOutInfo())
    {
        if (instr->GetBailOutKind() == IR::BailOutExpectingInteger)
        {
            isAlreadyTypeSpecialized = TypeSpecializeBailoutExpectedInteger(instr, src1Val, &dstVal);
        }
        else if (instr->GetBailOutKind() == IR::BailOutExpectingString)
        {
            if (instr->GetSrc1()->IsRegOpnd())
            {
                if (!src1Val || !src1Val->GetValueInfo()->IsLikelyString())
                {
                    // Disable SwitchOpt if the source is definitely not a string - This may be realized only in Globopt
                    Assert(IsSwitchOptEnabled());
                    throw Js::RejitException(RejitReason::DisableSwitchOptExpectingString);
                }
            }
        }
    }

    bool forceInvariantHoisting = false;
    const bool ignoreIntOverflowInRangeForInstr = instr->ignoreIntOverflowInRange; // Save it since the instr can change
    if (!isAlreadyTypeSpecialized)
    {
        bool redoTypeSpec;
        instr = this->TypeSpecialization(instr, &src1Val, &src2Val, &dstVal, &redoTypeSpec, &forceInvariantHoisting);

        if(redoTypeSpec && instr->m_opcode != Js::OpCode::Nop)
        {
            forceInvariantHoisting = false;
            instr = this->TypeSpecialization(instr, &src1Val, &src2Val, &dstVal, &redoTypeSpec, &forceInvariantHoisting);
            Assert(!redoTypeSpec);
        }
        if (instr->m_opcode == Js::OpCode::Nop)
        {
            InsertNoImplicitCallUses(instr);
            if (this->byteCodeUses)
            {
                this->InsertByteCodeUses(instr);
            }
            instrNext = instr->m_next;
            *isInstrRemoved = true;
            this->currentBlock->RemoveInstr(instr);
            return instrNext;
        }
    }

    if (ignoreIntOverflowInRangeForInstr)
    {
        VerifyIntSpecForIgnoringIntOverflow(instr);
    }

    // Track calls after any pre-op bailouts have been inserted before the call, because they will need to restore out params.
    // We don't inline in asmjs and hence we don't need to track calls in asmjs too, skipping this step for asmjs.
    if (!GetIsAsmJSFunc())
    {
        this->TrackCalls(instr);
    }

    if (instr->GetSrc1())
    {
        this->UpdateObjPtrValueType(instr->GetSrc1(), instr);
    }
    IR::Opnd *dst = instr->GetDst();

    if (dst)
    {
        // Copy prop dst uses and mark live/available type syms before tracking kills.
        CopyPropDstUses(dst, instr, src1Val);
    }

    // Track mark temp object before we process the dst so we can generate pre-op bailout
    instr = this->TrackMarkTempObject(instrPrev->m_next, instr);

    bool removed = OptTagChecks(instr);
    if (removed)
    {
        *isInstrRemoved = true;
        return instrNext;
    }

    dstVal = this->OptDst(&instr, dstVal, src1Val, src2Val, dstIndirIndexVal, src1IndirIndexVal);
    dst = instr->GetDst();

    instrNext = instr->m_next;
    if (dst)
    {
        if (this->func->HasTry() && this->func->DoOptimizeTry())
        {
            this->InsertToVarAtDefInTryRegion(instr, dst);
        }
        instr = this->SetTypeCheckBailOut(dst, instr, nullptr);
        this->UpdateObjPtrValueType(dst, instr);
    }

    BVSparse<JitArenaAllocator> instrByteCodeStackSymUsedAfter(this->alloc);
    PropertySym *propertySymUseAfter = nullptr;
    if (this->byteCodeUses != nullptr)
    {
        GlobOpt::TrackByteCodeSymUsed(instr, &instrByteCodeStackSymUsedAfter, &propertySymUseAfter);
    }
#if DBG
    else
    {
        GlobOpt::TrackByteCodeSymUsed(instr, &instrByteCodeStackSymUsedAfter, &propertySymUseAfter);
        instrByteCodeStackSymUsedAfter.Equal(this->byteCodeUsesBeforeOpt);
        Assert(propertySymUseAfter == propertySymUseBefore);
    }
#endif

    bool isHoisted = false;
    if (this->currentBlock->loop && !this->IsLoopPrePass())
    {
        isHoisted = this->TryHoistInvariant(instr, this->currentBlock, dstVal, src1Val, src2Val, true, false, forceInvariantHoisting);
    }

    src1 = instr->GetSrc1();
    if (!this->IsLoopPrePass() && src1)
    {
        // instr  const, nonConst   =>  canonicalize by swapping operands
        // This simplifies lowering. (somewhat machine dependent)
        // Note that because of Var overflows, src1 may not have been constant prop'd to an IntConst
        this->PreLowerCanonicalize(instr, &src1Val, &src2Val);
    }

    if (!PHASE_OFF(Js::MemOpPhase, this->func) &&
        !isHoisted &&
        !(instr->IsJitProfilingInstr()) &&
        this->currentBlock->loop && !IsLoopPrePass() &&
        !func->IsJitInDebugMode() &&
        (func->HasProfileInfo() && !func->GetReadOnlyProfileInfo()->IsMemOpDisabled()) &&
        this->currentBlock->loop->doMemOp)
    {
        CollectMemOpInfo(instrPrev, instr, src1Val, src2Val);
    }

    InsertNoImplicitCallUses(instr);
    if (this->byteCodeUses != nullptr)
    {
        // Optimization removed some uses from the instruction.
        // Need to insert fake uses so we can get the correct live register to restore in bailout.
        this->byteCodeUses->Minus(&instrByteCodeStackSymUsedAfter);
        if (this->propertySymUse == propertySymUseAfter)
        {
            this->propertySymUse = nullptr;
        }
        this->InsertByteCodeUses(instr);
    }

    if (!this->IsLoopPrePass() && !isHoisted && this->IsImplicitCallBailOutCurrentlyNeeded(instr, src1Val, src2Val))
    {
        IR::BailOutKind kind = IR::BailOutOnImplicitCalls;
        if(instr->HasBailOutInfo())
        {
            Assert(instr->GetBailOutInfo()->bailOutOffset == instr->GetByteCodeOffset());
            const IR::BailOutKind bailOutKind = instr->GetBailOutKind();
            if((bailOutKind & ~IR::BailOutKindBits) != IR::BailOutOnImplicitCallsPreOp)
            {
                Assert(!(bailOutKind & ~IR::BailOutKindBits));
                instr->SetBailOutKind(bailOutKind + IR::BailOutOnImplicitCallsPreOp);
            }
        }
        else if (instr->forcePreOpBailOutIfNeeded || this->isRecursiveCallOnLandingPad)
        {
            // We can't have a byte code reg slot as dst to generate a
            // pre-op implicit call after we have processed the dst.

            // Consider: This might miss an opportunity to use a copy prop sym to restore
            // some other byte code reg if the dst is that copy prop that we already killed.
            Assert(!instr->GetDst()
                || !instr->GetDst()->IsRegOpnd()
                || instr->GetDst()->AsRegOpnd()->GetIsJITOptimizedReg()
                || !instr->GetDst()->AsRegOpnd()->m_sym->HasByteCodeRegSlot());

            this->GenerateBailAtOperation(&instr, IR::BailOutOnImplicitCallsPreOp);
        }
        else
        {
            // Capture value of the bailout after the operation is done.
            this->GenerateBailAfterOperation(&instr, kind);
        }
    }

    if (instr->HasBailOutInfo() && !this->IsLoopPrePass())
    {
        GlobOptBlockData * globOptData = CurrentBlockData();
        globOptData->changedSyms->ClearAll();

        if (!this->changedSymsAfterIncBailoutCandidate->IsEmpty())
        {
            //
            // some symbols are changed after the values for current bailout have been
            // captured (GlobOpt::CapturedValues), need to restore such symbols as changed
            // for following incremental bailout construction, or we will miss capturing
            // values for later bailout
            //

            // swap changedSyms and changedSymsAfterIncBailoutCandidate
            // because both are from this->alloc
            BVSparse<JitArenaAllocator> * tempBvSwap = globOptData->changedSyms;
            globOptData->changedSyms = this->changedSymsAfterIncBailoutCandidate;
            this->changedSymsAfterIncBailoutCandidate = tempBvSwap;
        }

        globOptData->capturedValues = globOptData->capturedValuesCandidate;

        // null out capturedValuesCandicate to stop tracking symbols change for it
        globOptData->capturedValuesCandidate = nullptr;
    }

    return instrNext;
}

bool
GlobOpt::OptTagChecks(IR::Instr *instr)
{
    if (PHASE_OFF(Js::OptTagChecksPhase, this->func) || !this->DoTagChecks())
    {
        return false;
    }

    StackSym *stackSym = nullptr;
    IR::SymOpnd *symOpnd = nullptr;
    IR::RegOpnd *regOpnd = nullptr;

    switch(instr->m_opcode)
    {
    case Js::OpCode::LdFld:
    case Js::OpCode::LdMethodFld:
    case Js::OpCode::CheckFixedFld:
    case Js::OpCode::CheckPropertyGuardAndLoadType:
        symOpnd = instr->GetSrc1()->AsSymOpnd();
        stackSym = symOpnd->m_sym->AsPropertySym()->m_stackSym;
        break;

    case Js::OpCode::BailOnNotObject:
    case Js::OpCode::BailOnNotArray:
        if (instr->GetSrc1()->IsRegOpnd())
        {
            regOpnd = instr->GetSrc1()->AsRegOpnd();
            stackSym = regOpnd->m_sym;
        }
        break;

    case Js::OpCode::StFld:
        symOpnd = instr->GetDst()->AsSymOpnd();
        stackSym = symOpnd->m_sym->AsPropertySym()->m_stackSym;
        break;
    }

    if (stackSym)
    {
        Value *value = CurrentBlockData()->FindValue(stackSym);
        if (value)
        {
            ValueInfo *valInfo = value->GetValueInfo();
            if (valInfo->GetSymStore() && valInfo->GetSymStore()->IsStackSym() && valInfo->GetSymStore()->AsStackSym()->IsFromByteCodeConstantTable())
            {
                return false;
            }
            ValueType valueType = value->GetValueInfo()->Type();
            if (instr->m_opcode == Js::OpCode::BailOnNotObject)
            {
                if (valueType.CanBeTaggedValue())
                {
                    // We're not adding new information to the value other than changing the value type. Preserve any existing
                    // information and just change the value type.
                    ChangeValueType(nullptr, value, valueType.SetCanBeTaggedValue(false), true /*preserveSubClassInfo*/);
                    return false;
                }
                if (this->byteCodeUses)
                {
                    this->InsertByteCodeUses(instr);
                }
                this->currentBlock->RemoveInstr(instr);
                return true;
            }

            if (valueType.CanBeTaggedValue() &&
                !valueType.HasBeenNumber() &&
                (this->IsLoopPrePass() || !this->currentBlock->loop))
            {
                ValueType newValueType = valueType.SetCanBeTaggedValue(false);

                // Split out the tag check as a separate instruction.
                IR::Instr *bailOutInstr;
                bailOutInstr = IR::BailOutInstr::New(Js::OpCode::BailOnNotObject, IR::BailOutOnTaggedValue, instr, instr->m_func);
                if (!this->IsLoopPrePass())
                {
                    FillBailOutInfo(this->currentBlock, bailOutInstr->GetBailOutInfo());
                }
                IR::RegOpnd *srcOpnd = regOpnd;
                if (!srcOpnd)
                {
                    srcOpnd = IR::RegOpnd::New(stackSym, stackSym->GetType(), instr->m_func);
                    AnalysisAssert(symOpnd);
                    if (symOpnd->GetIsJITOptimizedReg())
                    {
                        srcOpnd->SetIsJITOptimizedReg(true);
                    }
                }
                bailOutInstr->SetSrc1(srcOpnd);
                bailOutInstr->GetSrc1()->SetValueType(valueType);
                instr->InsertBefore(bailOutInstr);

                if (symOpnd)
                {
                    symOpnd->SetPropertyOwnerValueType(newValueType);
                }
                else
                {
                    regOpnd->SetValueType(newValueType);
                }
                ChangeValueType(nullptr, value, newValueType, false);
            }
        }
    }

    return false;
}

bool
GlobOpt::TypeSpecializeBailoutExpectedInteger(IR::Instr* instr, Value* src1Val, Value** dstVal)
{
    bool isAlreadyTypeSpecialized = false;

    if(instr->GetSrc1()->IsRegOpnd())
    {
        if (!src1Val || !src1Val->GetValueInfo()->IsLikelyInt() || instr->GetSrc1()->AsRegOpnd()->m_sym->m_isNotInt)
        {
            Assert(IsSwitchOptEnabled());
            throw Js::RejitException(RejitReason::DisableSwitchOptExpectingInteger);
        }

        // Attach the BailOutExpectingInteger to FromVar and Remove the bail out info on the Ld_A (Begin Switch) instr.
        this->ToTypeSpecUse(instr, instr->GetSrc1(), this->currentBlock, src1Val, nullptr, TyInt32, IR::BailOutExpectingInteger, false, instr);

        //TypeSpecialize the dst of Ld_A
        TypeSpecializeIntDst(instr, instr->m_opcode, src1Val, src1Val, nullptr, IR::BailOutInvalid, INT32_MIN, INT32_MAX, dstVal);
        isAlreadyTypeSpecialized = true;
    }

    instr->ClearBailOutInfo();
    return isAlreadyTypeSpecialized;
}

Value*
GlobOpt::OptDst(
    IR::Instr ** pInstr,
    Value *dstVal,
    Value *src1Val,
    Value *src2Val,
    Value *dstIndirIndexVal,
    Value *src1IndirIndexVal)
{
    IR::Instr *&instr = *pInstr;
    IR::Opnd *opnd = instr->GetDst();

    if (opnd)
    {
        if (opnd->IsSymOpnd() && opnd->AsSymOpnd()->IsPropertySymOpnd())
        {
            this->FinishOptPropOp(instr, opnd->AsPropertySymOpnd());
        }
        else if (instr->m_opcode == Js::OpCode::StElemI_A ||
                 instr->m_opcode == Js::OpCode::StElemI_A_Strict ||
                 instr->m_opcode == Js::OpCode::InitComputedProperty)
        {
            this->KillObjectHeaderInlinedTypeSyms(this->currentBlock, false);
        }

        if (opnd->IsIndirOpnd() && !this->IsLoopPrePass())
        {
            IR::RegOpnd *baseOpnd = opnd->AsIndirOpnd()->GetBaseOpnd();
            const ValueType baseValueType(baseOpnd->GetValueType());
            if ((
                    baseValueType.IsLikelyNativeArray() ||
                #ifdef _M_IX86
                    (
                        !AutoSystemInfo::Data.SSE2Available() &&
                        baseValueType.IsLikelyObject() &&
                        (
                            baseValueType.GetObjectType() == ObjectType::Float32Array ||
                            baseValueType.GetObjectType() == ObjectType::Float64Array
                        )
                    )
                #else
                    false
                #endif
                ) &&
                instr->GetSrc1()->IsVar())
            {
                if(instr->m_opcode == Js::OpCode::StElemC)
                {
                    // StElemC has different code that handles native array conversion or missing value stores. Add a bailout
                    // for those cases.
                    Assert(baseValueType.IsLikelyNativeArray());
                    Assert(!instr->HasBailOutInfo());
                    GenerateBailAtOperation(&instr, IR::BailOutConventionalNativeArrayAccessOnly);
                }
                else if(instr->HasBailOutInfo())
                {
                    // The lowerer is not going to generate a fast path for this case. Remove any bailouts that require the fast
                    // path. Note that the removed bailouts should not be necessary for correctness. Bailout on native array
                    // conversion will be handled automatically as normal.
                    IR::BailOutKind bailOutKind = instr->GetBailOutKind();
                    if(bailOutKind & IR::BailOutOnArrayAccessHelperCall)
                    {
                        bailOutKind -= IR::BailOutOnArrayAccessHelperCall;
                    }
                    if(bailOutKind == IR::BailOutOnImplicitCallsPreOp)
                    {
                        bailOutKind -= IR::BailOutOnImplicitCallsPreOp;
                    }
                    if(bailOutKind)
                    {
                        instr->SetBailOutKind(bailOutKind);
                    }
                    else
                    {
                        instr->ClearBailOutInfo();
                    }
                }
            }
        }
    }

    this->ProcessKills(instr);

    if (opnd)
    {
        if (dstVal == nullptr)
        {
            dstVal = ValueNumberDst(pInstr, src1Val, src2Val);
        }
        if (this->IsLoopPrePass())
        {
            // Keep track of symbols defined in the loop.
            if (opnd->IsRegOpnd())
            {
                StackSym *symDst = opnd->AsRegOpnd()->m_sym;
                rootLoopPrePass->symsDefInLoop->Set(symDst->m_id);
            }
        }
        else if (dstVal)
        {
            opnd->SetValueType(dstVal->GetValueInfo()->Type());

            if(currentBlock->loop &&
                !IsLoopPrePass() &&
                (instr->m_opcode == Js::OpCode::Ld_A || instr->m_opcode == Js::OpCode::Ld_I4) &&
                instr->GetSrc1()->IsRegOpnd() &&
                !func->IsJitInDebugMode() &&
                func->DoGlobOptsForGeneratorFunc())
            {
                // Look for the following patterns:
                //
                // Pattern 1:
                //     s1[liveOnBackEdge] = s3[dead]
                //
                // Pattern 2:
                //     s3 = operation(s1[liveOnBackEdge], s2)
                //     s1[liveOnBackEdge] = s3
                //
                // In both patterns, s1 and s3 have the same value by the end. Prefer to use s1 as the sym store instead of s3
                // since s1 is live on back-edge, as otherwise, their lifetimes overlap, requiring two registers to hold the
                // value instead of one.
                do
                {
                    IR::RegOpnd *const src = instr->GetSrc1()->AsRegOpnd();
                    StackSym *srcVarSym = src->m_sym;
                    if(srcVarSym->IsTypeSpec())
                    {
                        srcVarSym = srcVarSym->GetVarEquivSym(nullptr);
                        Assert(srcVarSym);
                    }
                    if(dstVal->GetValueInfo()->GetSymStore() != srcVarSym)
                    {
                        break;
                    }

                    IR::RegOpnd *const dst = opnd->AsRegOpnd();
                    StackSym *dstVarSym = dst->m_sym;
                    if(dstVarSym->IsTypeSpec())
                    {
                        dstVarSym = dstVarSym->GetVarEquivSym(nullptr);
                        Assert(dstVarSym);
                    }
                    if(!currentBlock->loop->regAlloc.liveOnBackEdgeSyms->Test(dstVarSym->m_id))
                    {
                        break;
                    }

                    Value *const srcValue = CurrentBlockData()->FindValue(srcVarSym);
                    if(srcValue->GetValueNumber() != dstVal->GetValueNumber())
                    {
                        break;
                    }

                    if(!src->GetIsDead())
                    {
                        IR::Instr *const prevInstr = instr->GetPrevRealInstrOrLabel();
                        IR::Opnd *const prevDst = prevInstr->GetDst();
                        if(!prevDst ||
                            !src->IsEqualInternal(prevDst) ||
                            !(
                                (prevInstr->GetSrc1() && dst->IsEqual(prevInstr->GetSrc1())) ||
                                (prevInstr->GetSrc2() && dst->IsEqual(prevInstr->GetSrc2()))
                            ))
                        {
                            break;
                        }
                    }

                    this->SetSymStoreDirect(dstVal->GetValueInfo(), dstVarSym);
                } while(false);
            }
        }

        this->ValueNumberObjectType(opnd, instr);
    }

    this->CSEAddInstr(this->currentBlock, *pInstr, dstVal, src1Val, src2Val, dstIndirIndexVal, src1IndirIndexVal);

    return dstVal;
}

void
GlobOpt::CopyPropDstUses(IR::Opnd *opnd, IR::Instr *instr, Value *src1Val)
{
    if (opnd->IsSymOpnd())
    {
        IR::SymOpnd *symOpnd = opnd->AsSymOpnd();

        if (symOpnd->m_sym->IsPropertySym())
        {
            PropertySym * originalPropertySym = symOpnd->m_sym->AsPropertySym();

            Value *const objectValue = CurrentBlockData()->FindValue(originalPropertySym->m_stackSym);
            symOpnd->SetPropertyOwnerValueType(objectValue ? objectValue->GetValueInfo()->Type() : ValueType::Uninitialized);

            this->FieldHoistOptDst(instr, originalPropertySym, src1Val);
            PropertySym * sym = this->CopyPropPropertySymObj(symOpnd, instr);
            if (sym != originalPropertySym && !this->IsLoopPrePass())
            {
                // Consider: This doesn't detect hoistability of a property sym after object pointer copy prop
                // on loop prepass. But if it so happened that the property sym is hoisted, we might as well do so.
                this->FieldHoistOptDst(instr, sym, src1Val);
            }

        }
    }
}

void
GlobOpt::SetLoopFieldInitialValue(Loop *loop, IR::Instr *instr, PropertySym *propertySym, PropertySym *originalPropertySym)
{
    Value *initialValue = nullptr;
    StackSym *symStore;

    if (loop->allFieldsKilled || loop->fieldKilled->Test(originalPropertySym->m_id))
    {
        return;
    }
    Assert(!loop->fieldKilled->Test(propertySym->m_id));

    // Value already exists
    if (CurrentBlockData()->FindValue(propertySym))
    {
        return;
    }

    // If this initial value was already added, we would find in the current value table.
    Assert(!loop->initialValueFieldMap.TryGetValue(propertySym, &initialValue));

    // If propertySym is live in landingPad, we don't need an initial value.
    if (loop->landingPad->globOptData.liveFields->Test(propertySym->m_id))
    {
        return;
    }

    Value *landingPadObjPtrVal, *currentObjPtrVal;
    landingPadObjPtrVal = loop->landingPad->globOptData.FindValue(propertySym->m_stackSym);
    currentObjPtrVal = CurrentBlockData()->FindValue(propertySym->m_stackSym);
    if (!currentObjPtrVal || !landingPadObjPtrVal || currentObjPtrVal->GetValueNumber() != landingPadObjPtrVal->GetValueNumber())
    {
        // objPtr has a different value in the landing pad.
        return;
    }

    // The opnd's value type has not yet been initialized. Since the property sym doesn't have a value, it effectively has an
    // Uninitialized value type. Use the profiled value type from the instruction.
    const ValueType profiledValueType =
        instr->IsProfiledInstr() ? instr->AsProfiledInstr()->u.FldInfo().valueType : ValueType::Uninitialized;
    Assert(!profiledValueType.IsDefinite()); // Hence the values created here don't need to be tracked for kills
    initialValue = this->NewGenericValue(profiledValueType, propertySym);
    symStore = StackSym::New(this->func);

    initialValue->GetValueInfo()->SetSymStore(symStore);
    loop->initialValueFieldMap.Add(propertySym, initialValue->Copy(this->alloc, initialValue->GetValueNumber()));

    // Copy the initial value into the landing pad, but without a symStore
    Value *landingPadInitialValue = Value::New(this->alloc, initialValue->GetValueNumber(),
        ValueInfo::New(this->alloc, initialValue->GetValueInfo()->Type()));
    loop->landingPad->globOptData.SetValue(landingPadInitialValue, propertySym);
    loop->landingPad->globOptData.liveFields->Set(propertySym->m_id);

#if DBG_DUMP
    if (PHASE_TRACE(Js::FieldPREPhase, this->func))
    {
        Output::Print(_u("** TRACE:  Field PRE initial value for loop head #%d. Val:%d symStore:"),
            loop->GetHeadBlock()->GetBlockNum(), initialValue->GetValueNumber());
        symStore->Dump();
        Output::Print(_u("\n    Instr: "));
        instr->Dump();
    }
#endif

    // Add initial value to all the previous blocks in the loop.
    FOREACH_BLOCK_BACKWARD_IN_RANGE(block, this->currentBlock->GetPrev(), loop->GetHeadBlock())
    {
        if (block->GetDataUseCount() == 0)
        {
            // All successor blocks have been processed, no point in adding the value.
            continue;
        }
        Value *newValue = initialValue->Copy(this->alloc, initialValue->GetValueNumber());
        block->globOptData.SetValue(newValue, propertySym);
        block->globOptData.liveFields->Set(propertySym->m_id);
        block->globOptData.SetValue(newValue, symStore);
        block->globOptData.liveVarSyms->Set(symStore->m_id);
    } NEXT_BLOCK_BACKWARD_IN_RANGE;

    CurrentBlockData()->SetValue(initialValue, symStore);
    CurrentBlockData()->liveVarSyms->Set(symStore->m_id);
    CurrentBlockData()->liveFields->Set(propertySym->m_id);
}

// Examine src, apply copy prop and value number it
Value*
GlobOpt::OptSrc(IR::Opnd *opnd, IR::Instr * *pInstr, Value **indirIndexValRef, IR::IndirOpnd *parentIndirOpnd)
{
    IR::Instr * &instr = *pInstr;
    Assert(!indirIndexValRef || !*indirIndexValRef);
    Assert(
        parentIndirOpnd
            ? opnd == parentIndirOpnd->GetBaseOpnd() || opnd == parentIndirOpnd->GetIndexOpnd()
            : opnd == instr->GetSrc1() || opnd == instr->GetSrc2() || opnd == instr->GetDst() && opnd->IsIndirOpnd());

    Sym *sym;
    Value *val;
    PropertySym *originalPropertySym = nullptr;

    switch(opnd->GetKind())
    {
    case IR::OpndKindIntConst:
        val = this->GetIntConstantValue(opnd->AsIntConstOpnd()->AsInt32(), instr);
        opnd->SetValueType(val->GetValueInfo()->Type());
        return val;
    case IR::OpndKindInt64Const:
        val = this->GetIntConstantValue(opnd->AsInt64ConstOpnd()->GetValue(), instr);
        opnd->SetValueType(val->GetValueInfo()->Type());
        return val;
    case IR::OpndKindFloatConst:
    {
        const FloatConstType floatValue = opnd->AsFloatConstOpnd()->m_value;
        int32 int32Value;
        if(Js::JavascriptNumber::TryGetInt32Value(floatValue, &int32Value))
        {
            val = GetIntConstantValue(int32Value, instr);
        }
        else
        {
            val = NewFloatConstantValue(floatValue);
        }
        opnd->SetValueType(val->GetValueInfo()->Type());
        return val;
    }

    case IR::OpndKindAddr:
        {
            IR::AddrOpnd *addrOpnd = opnd->AsAddrOpnd();
            if (addrOpnd->m_isFunction)
            {
                AssertMsg(!PHASE_OFF(Js::FixedMethodsPhase, instr->m_func), "Fixed function address operand with fixed method calls phase disabled?");
                val = NewFixedFunctionValue((Js::JavascriptFunction *)addrOpnd->m_address, addrOpnd);
                opnd->SetValueType(val->GetValueInfo()->Type());
                return val;
            }
            else if (addrOpnd->IsVar() && Js::TaggedInt::Is(addrOpnd->m_address))
            {
                val = this->GetIntConstantValue(Js::TaggedInt::ToInt32(addrOpnd->m_address), instr);
                opnd->SetValueType(val->GetValueInfo()->Type());
                return val;
            }
            val = this->GetVarConstantValue(addrOpnd);
            return val;
        }

    case IR::OpndKindSym:
    {
        // Clear the opnd's value type up-front, so that this code cannot accidentally use the value type set from a previous
        // OptSrc on the same instruction (for instance, from an earlier loop prepass). The value type will be set from the
        // value if available, before returning from this function.
        opnd->SetValueType(ValueType::Uninitialized);

        sym = opnd->AsSymOpnd()->m_sym;

        // Don't create a new value for ArgSlots and don't copy prop them away.
        if (sym->IsStackSym() && sym->AsStackSym()->IsArgSlotSym())
        {
            return nullptr;
        }

        // Unless we have profile info, don't create a new value for ArgSlots and don't copy prop them away.
        if (sym->IsStackSym() && sym->AsStackSym()->IsParamSlotSym())
        {
            if (!instr->m_func->IsLoopBody() && instr->m_func->HasProfileInfo())
            {
                // Skip "this" pointer.
                int paramSlotNum = sym->AsStackSym()->GetParamSlotNum() - 2;
                if (paramSlotNum >= 0)
                {
                    const auto parameterType = instr->m_func->GetReadOnlyProfileInfo()->GetParameterInfo(static_cast<Js::ArgSlot>(paramSlotNum));
                    val = NewGenericValue(parameterType);
                    opnd->SetValueType(val->GetValueInfo()->Type());
                    return val;
                }
            }
            return nullptr;
        }

        if (!sym->IsPropertySym())
        {
            break;
        }
        originalPropertySym = sym->AsPropertySym();

        Value *const objectValue = CurrentBlockData()->FindValue(originalPropertySym->m_stackSym);
        opnd->AsSymOpnd()->SetPropertyOwnerValueType(
            objectValue ? objectValue->GetValueInfo()->Type() : ValueType::Uninitialized);

        if (!FieldHoistOptSrc(opnd->AsSymOpnd(), instr, originalPropertySym))
        {
            sym = this->CopyPropPropertySymObj(opnd->AsSymOpnd(), instr);

            // Consider: This doesn't detect hoistability of a property sym after object pointer copy prop
            // on loop prepass. But if it so happened that the property sym is hoisted, we might as well do so.
            if (originalPropertySym == sym || this->IsLoopPrePass() ||
                !FieldHoistOptSrc(opnd->AsSymOpnd(), instr, sym->AsPropertySym()))
            {
                if (!DoFieldCopyProp())
                {
                    if (opnd->AsSymOpnd()->IsPropertySymOpnd())
                    {
                        this->FinishOptPropOp(instr, opnd->AsPropertySymOpnd());
                    }
                    return nullptr;
                }
                switch (instr->m_opcode)
                {
                    // These need the symbolic reference to the field, don't copy prop the value of the field
                case Js::OpCode::DeleteFld:
                case Js::OpCode::DeleteRootFld:
                case Js::OpCode::DeleteFldStrict:
                case Js::OpCode::DeleteRootFldStrict:
                case Js::OpCode::ScopedDeleteFld:
                case Js::OpCode::ScopedDeleteFldStrict:
                case Js::OpCode::LdMethodFromFlags:
                case Js::OpCode::BrOnNoProperty:
                case Js::OpCode::BrOnHasProperty:
                case Js::OpCode::LdMethodFldPolyInlineMiss:
                case Js::OpCode::StSlotChkUndecl:
                    return nullptr;
                };

                if (instr->CallsGetter())
                {
                    return nullptr;
                }

                if (this->IsLoopPrePass() && this->DoFieldPRE(this->rootLoopPrePass))
                {
                    if (!this->prePassLoop->allFieldsKilled && !this->prePassLoop->fieldKilled->Test(sym->m_id))
                    {
                        this->SetLoopFieldInitialValue(this->rootLoopPrePass, instr, sym->AsPropertySym(), originalPropertySym);
                    }
                    if (this->IsPREInstrCandidateLoad(instr->m_opcode))
                    {
                        // Foreach property sym, remember the first instruction that loads it.
                        // Can this be done in one call?
                        if (!this->prePassInstrMap->ContainsKey(sym->m_id))
                        {
                            this->prePassInstrMap->AddNew(sym->m_id, instr);
                        }
                    }
                }
                break;
            }
        }

        // We field hoisted, we can continue as a reg.
        opnd = instr->GetSrc1();
    }
    case IR::OpndKindReg:
        // Clear the opnd's value type up-front, so that this code cannot accidentally use the value type set from a previous
        // OptSrc on the same instruction (for instance, from an earlier loop prepass). The value type will be set from the
        // value if available, before returning from this function.
        opnd->SetValueType(ValueType::Uninitialized);

        sym = opnd->AsRegOpnd()->m_sym;
        CurrentBlockData()->MarkTempLastUse(instr, opnd->AsRegOpnd());
        if (sym->AsStackSym()->IsTypeSpec())
        {
            sym = sym->AsStackSym()->GetVarEquivSym(this->func);
        }
        break;

    case IR::OpndKindIndir:
        this->OptimizeIndirUses(opnd->AsIndirOpnd(), &instr, indirIndexValRef);
        return nullptr;

    default:
        return nullptr;
    }

    val = CurrentBlockData()->FindValue(sym);

    if (val)
    {
        Assert(CurrentBlockData()->IsLive(sym) || (sym->IsPropertySym()));
        if (instr)
        {
            opnd = this->CopyProp(opnd, instr, val, parentIndirOpnd);
        }

        // Check if we freed the operand.
        if (opnd == nullptr)
        {
            return nullptr;
        }

        // In a loop prepass, determine stack syms that are used before they are defined in the root loop for which the prepass
        // is being done. This information is used to do type specialization conversions in the landing pad where appropriate.
        if(IsLoopPrePass() &&
            sym->IsStackSym() &&
            !rootLoopPrePass->symsUsedBeforeDefined->Test(sym->m_id) &&
            rootLoopPrePass->landingPad->globOptData.IsLive(sym) && !isAsmJSFunc) // no typespec in asmjs and hence skipping this
        {
            Value *const landingPadValue = rootLoopPrePass->landingPad->globOptData.FindValue(sym);
            if(landingPadValue && val->GetValueNumber() == landingPadValue->GetValueNumber())
            {
                rootLoopPrePass->symsUsedBeforeDefined->Set(sym->m_id);
                ValueInfo *landingPadValueInfo = landingPadValue->GetValueInfo();
                if(landingPadValueInfo->IsLikelyNumber())
                {
                    rootLoopPrePass->likelyNumberSymsUsedBeforeDefined->Set(sym->m_id);
                    if(DoAggressiveIntTypeSpec() ? landingPadValueInfo->IsLikelyInt() : landingPadValueInfo->IsInt())
                    {
                        // Can only force int conversions in the landing pad based on likely-int values if aggressive int type
                        // specialization is enabled.
                        rootLoopPrePass->likelyIntSymsUsedBeforeDefined->Set(sym->m_id);
                    }
                }

#ifdef ENABLE_SIMDJS
                // SIMD_JS
                // For uses before defs, we set likelySimd128*SymsUsedBeforeDefined bits for syms that have landing pad value info that allow type-spec to happen in the loop body.
                // The BV will be added to loop header if the backedge has a live matching type-spec value. We then compensate in the loop header to unbox the value.
                // This allows type-spec in the landing pad instead of boxing/unboxing on each iteration.
                if (Js::IsSimd128Opcode(instr->m_opcode))
                {
                    // Simd ops are strongly typed. We type-spec only if the type is likely/Definitely the expected type or if we have object which can come from merging different Simd types.
                    // Simd value must be initialized properly on all paths before the loop entry. Cannot be merged with Undefined/Null.
                    ThreadContext::SimdFuncSignature funcSignature;
                    instr->m_func->GetScriptContext()->GetThreadContext()->GetSimdFuncSignatureFromOpcode(instr->m_opcode, funcSignature);
                    Assert(funcSignature.valid);
                    ValueType expectedType = funcSignature.args[opnd == instr->GetSrc1() ? 0 : 1];

                    if (expectedType.IsSimd128Float32x4())
                    {
                        if  (
                                (landingPadValueInfo->IsLikelySimd128Float32x4() || (landingPadValueInfo->IsLikelyObject() && landingPadValueInfo->GetObjectType() == ObjectType::Object))
                                &&
                                !landingPadValueInfo->HasBeenUndefined() && !landingPadValueInfo->HasBeenNull()
                            )
                        {
                            rootLoopPrePass->likelySimd128F4SymsUsedBeforeDefined->Set(sym->m_id);
                        }
                    }
                    else if (expectedType.IsSimd128Int32x4())
                    {
                        if (
                            (landingPadValueInfo->IsLikelySimd128Int32x4() || (landingPadValueInfo->IsLikelyObject() && landingPadValueInfo->GetObjectType() == ObjectType::Object))
                            &&
                            !landingPadValueInfo->HasBeenUndefined() && !landingPadValueInfo->HasBeenNull()
                          )
                        {
                            rootLoopPrePass->likelySimd128I4SymsUsedBeforeDefined->Set(sym->m_id);
                        }
                    }
                }
                else if (instr->m_opcode == Js::OpCode::ExtendArg_A && opnd == instr->GetSrc1() && instr->GetDst()->GetValueType().IsSimd128())
                {
                    // Extended_Args for Simd ops are annotated with the expected type by the inliner. Use this info to find out if type-spec is supposed to happen.
                    ValueType expectedType = instr->GetDst()->GetValueType();

                    if ((landingPadValueInfo->IsLikelySimd128Float32x4() || (landingPadValueInfo->IsLikelyObject() && landingPadValueInfo->GetObjectType() == ObjectType::Object))
                        && expectedType.IsSimd128Float32x4())
                    {
                        rootLoopPrePass->likelySimd128F4SymsUsedBeforeDefined->Set(sym->m_id);
                    }
                    else if ((landingPadValueInfo->IsLikelySimd128Int32x4() || (landingPadValueInfo->IsLikelyObject() && landingPadValueInfo->GetObjectType() == ObjectType::Object))
                        && expectedType.IsSimd128Int32x4())
                    {
                        rootLoopPrePass->likelySimd128I4SymsUsedBeforeDefined->Set(sym->m_id);
                    }
                }
#endif
            }
        }
    }
    else if ((instr->TransfersSrcValue() || OpCodeAttr::CanCSE(instr->m_opcode)) && (opnd == instr->GetSrc1() || opnd == instr->GetSrc2()))
    {
        if (sym->IsPropertySym())
        {
            val = this->CreateFieldSrcValue(sym->AsPropertySym(), originalPropertySym, &opnd, instr);
        }
        else
        {
            val = this->NewGenericValue(ValueType::Uninitialized, opnd);
        }
    }

    if (opnd->IsSymOpnd() && opnd->AsSymOpnd()->IsPropertySymOpnd())
    {
        TryOptimizeInstrWithFixedDataProperty(&instr);
        this->FinishOptPropOp(instr, opnd->AsPropertySymOpnd());
    }

    if (val)
    {
        ValueType valueType(val->GetValueInfo()->Type());

        // This block uses local profiling data to optimize the case of a native array being passed to a function that fills it with other types. When the function is inlined
        // into different call paths which use different types this can cause a perf hit by performing unnecessary array conversions, so only perform this optimization when 
        // the function is not inlined.
        if (valueType.IsLikelyNativeArray() && !valueType.IsObject() && instr->IsProfiledInstr() && !instr->m_func->IsInlined())
        {
            // See if we have profile data for the array type
            IR::ProfiledInstr *const profiledInstr = instr->AsProfiledInstr();
            ValueType profiledArrayType;
            switch(instr->m_opcode)
            {
                case Js::OpCode::LdElemI_A:
                    if(instr->GetSrc1()->IsIndirOpnd() && opnd == instr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd())
                    {
                        profiledArrayType = profiledInstr->u.ldElemInfo->GetArrayType();
                    }
                    break;

                case Js::OpCode::StElemI_A:
                case Js::OpCode::StElemI_A_Strict:
                case Js::OpCode::StElemC:
                    if(instr->GetDst()->IsIndirOpnd() && opnd == instr->GetDst()->AsIndirOpnd()->GetBaseOpnd())
                    {
                        profiledArrayType = profiledInstr->u.stElemInfo->GetArrayType();
                    }
                    break;

                case Js::OpCode::LdLen_A:
                    if(instr->GetSrc1()->IsRegOpnd() && opnd == instr->GetSrc1())
                    {
                        profiledArrayType = profiledInstr->u.ldElemInfo->GetArrayType();
                    }
                    break;
            }
            if(profiledArrayType.IsLikelyObject() &&
                profiledArrayType.GetObjectType() == valueType.GetObjectType() &&
                (profiledArrayType.HasVarElements() || (valueType.HasIntElements() && profiledArrayType.HasFloatElements())))
            {
                // Merge array type we pulled from profile with type propagated by dataflow.
                valueType = valueType.Merge(profiledArrayType).SetHasNoMissingValues(valueType.HasNoMissingValues());
                ChangeValueType(this->currentBlock, CurrentBlockData()->FindValue(opnd->AsRegOpnd()->m_sym), valueType, false);
            }
        }
        opnd->SetValueType(valueType);

        if(!IsLoopPrePass() && opnd->IsSymOpnd() && valueType.IsDefinite())
        {
            if (opnd->AsSymOpnd()->m_sym->IsPropertySym())
            {
                // A property sym can only be guaranteed to have a definite value type when implicit calls are disabled from the
                // point where the sym was defined with the definite value type. Insert an instruction to indicate to the
                // dead-store pass that implicit calls need to be kept disabled until after this instruction.
                Assert(DoFieldCopyProp());
                CaptureNoImplicitCallUses(opnd, false, instr);
            }
        }
    }
    else
    {
        opnd->SetValueType(ValueType::Uninitialized);
    }

    return val;
}

/*
*   GlobOpt::TryOptimizeInstrWithFixedDataProperty
*       Converts Ld[Root]Fld instr to
*            * CheckFixedFld
*            * Dst = Ld_A <int Constant value>
*   This API assumes that the source operand is a Sym/PropertySym kind.
*/
void
GlobOpt::TryOptimizeInstrWithFixedDataProperty(IR::Instr ** const pInstr)
{
    Assert(pInstr);
    IR::Instr * &instr = *pInstr;
    IR::Opnd * src1 = instr->GetSrc1();
    Assert(src1 && src1->IsSymOpnd() && src1->AsSymOpnd()->IsPropertySymOpnd());

    if(PHASE_OFF(Js::UseFixedDataPropsPhase, instr->m_func))
    {
        return;
    }

    if (!this->IsLoopPrePass() && !this->isRecursiveCallOnLandingPad &&
        OpCodeAttr::CanLoadFixedFields(instr->m_opcode))
    {
        instr->TryOptimizeInstrWithFixedDataProperty(&instr, this);
    }
}

// Constant prop if possible, otherwise if this value already resides in another
// symbol, reuse this previous symbol. This should help register allocation.
IR::Opnd *
GlobOpt::CopyProp(IR::Opnd *opnd, IR::Instr *instr, Value *val, IR::IndirOpnd *parentIndirOpnd)
{
    Assert(
        parentIndirOpnd
            ? opnd == parentIndirOpnd->GetBaseOpnd() || opnd == parentIndirOpnd->GetIndexOpnd()
            : opnd == instr->GetSrc1() || opnd == instr->GetSrc2() || opnd == instr->GetDst() && opnd->IsIndirOpnd());

    if (this->IsLoopPrePass())
    {
        // Transformations are not legal in prepass...
        return opnd;
    }

    if (!this->func->DoGlobOptsForGeneratorFunc())
    {
        // Don't copy prop in generator functions because non-bytecode temps that span a yield
        // cannot be saved and restored by the current bail-out mechanics utilized by generator
        // yield/resume.
        // TODO[generators][ianhall]: Enable copy-prop at least for in between yields.
        return opnd;
    }

    if (instr->m_opcode == Js::OpCode::CheckFixedFld || instr->m_opcode == Js::OpCode::CheckPropertyGuardAndLoadType)
    {
        // Don't copy prop into CheckFixedFld or CheckPropertyGuardAndLoadType
        return opnd;
    }

    // Don't copy-prop link operands of ExtendedArgs
    if (instr->m_opcode == Js::OpCode::ExtendArg_A && opnd == instr->GetSrc2())
    {
        return opnd;
    }

    // Don't copy-prop operand of SIMD instr with ExtendedArg operands. Each instr should have its exclusive EA sequence.
    if (
            Js::IsSimd128Opcode(instr->m_opcode) &&
            instr->GetSrc1() != nullptr &&
            instr->GetSrc1()->IsRegOpnd() &&
            instr->GetSrc2() == nullptr
       )
    {
        StackSym *sym = instr->GetSrc1()->GetStackSym();
        if (sym && sym->IsSingleDef() && sym->GetInstrDef()->m_opcode == Js::OpCode::ExtendArg_A)
        {
                return opnd;
        }
    }

    ValueInfo *valueInfo = val->GetValueInfo();

    // Constant prop?
    int32 intConstantValue;
    int64 int64ConstantValue;
    if (valueInfo->TryGetIntConstantValue(&intConstantValue))
    {
        if (PHASE_OFF(Js::ConstPropPhase, this->func))
        {
            return opnd;
        }

        if ((
                instr->m_opcode == Js::OpCode::StElemI_A ||
                instr->m_opcode == Js::OpCode::StElemI_A_Strict ||
                instr->m_opcode == Js::OpCode::StElemC
            ) && instr->GetSrc1() == opnd)
        {
            // Disabling prop to src of native array store, because we were losing the chance to type specialize.
            // Is it possible to type specialize this src if we allow constants, etc., to be prop'd here?
            if (instr->GetDst()->AsIndirOpnd()->GetBaseOpnd()->GetValueType().IsLikelyNativeArray())
            {
                return opnd;
            }
        }

        if(opnd != instr->GetSrc1() && opnd != instr->GetSrc2())
        {
            if(PHASE_OFF(Js::IndirCopyPropPhase, instr->m_func))
            {
                return opnd;
            }

            // Const-prop an indir opnd's constant index into its offset
            IR::Opnd *srcs[] = { instr->GetSrc1(), instr->GetSrc2(), instr->GetDst() };
            for(int i = 0; i < sizeof(srcs) / sizeof(srcs[0]); ++i)
            {
                const auto src = srcs[i];
                if(!src || !src->IsIndirOpnd())
                {
                    continue;
                }

                const auto indir = src->AsIndirOpnd();
                if ((int64)indir->GetOffset() + intConstantValue > INT32_MAX)
                {
                    continue;
                }
                if(opnd == indir->GetIndexOpnd())
                {
                    Assert(indir->GetScale() == 0);
                    GOPT_TRACE_OPND(opnd, _u("Constant prop indir index into offset (value: %d)\n"), intConstantValue);
                    this->CaptureByteCodeSymUses(instr);
                    indir->SetOffset(indir->GetOffset() + intConstantValue);
                    indir->SetIndexOpnd(nullptr);
                }
            }

            return opnd;
        }

        if (Js::TaggedInt::IsOverflow(intConstantValue))
        {
            return opnd;
        }

        IR::Opnd *constOpnd;

        if (opnd->IsVar())
        {
            IR::AddrOpnd *addrOpnd = IR::AddrOpnd::New(Js::TaggedInt::ToVarUnchecked((int)intConstantValue), IR::AddrOpndKindConstantVar, instr->m_func);

            GOPT_TRACE_OPND(opnd, _u("Constant prop %d (value:%d)\n"), addrOpnd->m_address, intConstantValue);
            constOpnd = addrOpnd;
        }
        else
        {
            // Note: Jit loop body generates some i32 operands...
            Assert(opnd->IsInt32() || opnd->IsInt64() || opnd->IsUInt32());
            IRType opndType;
            IntConstType constVal;
            if (opnd->IsUInt32())
            {
                // avoid sign extension
                constVal = (uint32)intConstantValue;
                opndType = TyUint32;
            }
            else
            {
                constVal = intConstantValue;
                opndType = TyInt32;
            }
            IR::IntConstOpnd *intOpnd = IR::IntConstOpnd::New(constVal, opndType, instr->m_func);

            GOPT_TRACE_OPND(opnd, _u("Constant prop %d (value:%d)\n"), intOpnd->GetImmediateValue(instr->m_func), intConstantValue);
            constOpnd = intOpnd;
        }

#if ENABLE_DEBUG_CONFIG_OPTIONS
        //Need to update DumpFieldCopyPropTestTrace for every new opcode that is added for fieldcopyprop
        if(Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::FieldCopyPropPhase))
        {
            instr->DumpFieldCopyPropTestTrace();
        }
#endif

        this->CaptureByteCodeSymUses(instr);
        opnd = instr->ReplaceSrc(opnd, constOpnd);

        switch (instr->m_opcode)
        {
        case Js::OpCode::LdSlot:
        case Js::OpCode::LdSlotArr:
        case Js::OpCode::LdFld:
        case Js::OpCode::LdFldForTypeOf:
        case Js::OpCode::LdRootFldForTypeOf:
        case Js::OpCode::LdFldForCallApplyTarget:
        case Js::OpCode::LdRootFld:
        case Js::OpCode::LdMethodFld:
        case Js::OpCode::LdRootMethodFld:
        case Js::OpCode::LdMethodFromFlags:
        case Js::OpCode::ScopedLdMethodFld:
            instr->m_opcode = Js::OpCode::Ld_A;
        case Js::OpCode::Ld_A:
            {
                IR::Opnd * dst = instr->GetDst();
                if (dst->IsRegOpnd() && dst->AsRegOpnd()->m_sym->IsSingleDef())
                {
                    dst->AsRegOpnd()->m_sym->SetIsIntConst((int)intConstantValue);
                }
                break;
            }
        case Js::OpCode::ArgOut_A:
        case Js::OpCode::ArgOut_A_Inline:
        case Js::OpCode::ArgOut_A_FixupForStackArgs:
        case Js::OpCode::ArgOut_A_InlineBuiltIn:

            if (instr->GetDst()->IsRegOpnd())
            {
                Assert(instr->GetDst()->AsRegOpnd()->m_sym->m_isSingleDef);
                instr->GetDst()->AsRegOpnd()->m_sym->AsStackSym()->SetIsIntConst((int)intConstantValue);
            }
            else
            {
                instr->GetDst()->AsSymOpnd()->m_sym->AsStackSym()->SetIsIntConst((int)intConstantValue);
            }
            break;

        case Js::OpCode::TypeofElem:
            instr->m_opcode = Js::OpCode::Typeof;
            break;

        case Js::OpCode::StSlotChkUndecl:
            if (instr->GetSrc2() == opnd)
            {
                // Src2 here should refer to the same location as the Dst operand, which we need to keep live
                // due to the implicit read for ChkUndecl.
                instr->m_opcode = Js::OpCode::StSlot;
                instr->FreeSrc2();
                opnd = nullptr;
            }
            break;
        }
        return opnd;
    }
    else if (valueInfo->TryGetIntConstantValue(&int64ConstantValue, false))
    {
        if (PHASE_OFF(Js::ConstPropPhase, this->func) || !PHASE_ON(Js::Int64ConstPropPhase, this->func))
        {
            return opnd;
        }

        Assert(this->func->GetJITFunctionBody()->IsWasmFunction());
        if (this->func->GetJITFunctionBody()->IsWasmFunction() && opnd->IsInt64())
        {
            IR::Int64ConstOpnd *intOpnd = IR::Int64ConstOpnd::New(int64ConstantValue, opnd->GetType(), instr->m_func);
            GOPT_TRACE_OPND(opnd, _u("Constant prop %lld (value:%lld)\n"), intOpnd->GetImmediateValue(instr->m_func), int64ConstantValue);
            this->CaptureByteCodeSymUses(instr);
            opnd = instr->ReplaceSrc(opnd, intOpnd);
        }
        return opnd;
    }

    Sym *opndSym = nullptr;
    if (opnd->IsRegOpnd())
    {
        IR::RegOpnd *regOpnd = opnd->AsRegOpnd();
        opndSym = regOpnd->m_sym;
    }
    else if (opnd->IsSymOpnd())
    {
        IR::SymOpnd *symOpnd = opnd->AsSymOpnd();
        opndSym = symOpnd->m_sym;
    }
    if (!opndSym)
    {
        return opnd;
    }

    if (PHASE_OFF(Js::CopyPropPhase, this->func))
    {
        this->SetSymStoreDirect(valueInfo, opndSym);
        return opnd;
    }

    // We should have dealt with field hoist already
    Assert(!instr->TransfersSrcValue() || !opndSym->IsPropertySym() ||
        !this->IsHoistedPropertySym(opndSym->AsPropertySym()));

    StackSym *copySym = CurrentBlockData()->GetCopyPropSym(opndSym, val);
    if (copySym != nullptr)
    {
        // Copy prop.
        return CopyPropReplaceOpnd(instr, opnd, copySym, parentIndirOpnd);
    }
    else
    {
        if (valueInfo->GetSymStore() && instr->m_opcode == Js::OpCode::Ld_A && instr->GetDst()->IsRegOpnd()
            && valueInfo->GetSymStore() == instr->GetDst()->AsRegOpnd()->m_sym)
        {
            // Avoid resetting symStore after fieldHoisting:
            //  t1 = LdFld field            <- set symStore to fieldHoistSym
            //   fieldHoistSym = Ld_A t1    <- we're looking at t1 now, but want to copy-prop fieldHoistSym forward
            return opnd;
        }
        this->SetSymStoreDirect(valueInfo, opndSym);
    }
    return opnd;
}

IR::Opnd *
GlobOpt::CopyPropReplaceOpnd(IR::Instr * instr, IR::Opnd * opnd, StackSym * copySym, IR::IndirOpnd *parentIndirOpnd)
{
    Assert(
        parentIndirOpnd
            ? opnd == parentIndirOpnd->GetBaseOpnd() || opnd == parentIndirOpnd->GetIndexOpnd()
            : opnd == instr->GetSrc1() || opnd == instr->GetSrc2() || opnd == instr->GetDst() && opnd->IsIndirOpnd());
    Assert(CurrentBlockData()->IsLive(copySym));

    IR::RegOpnd *regOpnd;
    StackSym *newSym = copySym;

    GOPT_TRACE_OPND(opnd, _u("Copy prop s%d\n"), newSym->m_id);
#if ENABLE_DEBUG_CONFIG_OPTIONS
    //Need to update DumpFieldCopyPropTestTrace for every new opcode that is added for fieldcopyprop
    if(Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::FieldCopyPropPhase))
    {
        instr->DumpFieldCopyPropTestTrace();
    }
#endif

    this->CaptureByteCodeSymUses(instr);
    if (opnd->IsRegOpnd())
    {
        regOpnd = opnd->AsRegOpnd();
        regOpnd->m_sym = newSym;
        regOpnd->SetIsJITOptimizedReg(true);

        // The dead bit on the opnd is specific to the sym it is referencing. Since we replaced the sym, the bit is reset.
        regOpnd->SetIsDead(false);

        if(parentIndirOpnd)
        {
            return regOpnd;
        }
    }
    else
    {
        // If this is an object type specialized field load inside a loop, and it produces a type value which wasn't live
        // before, make sure the type check is left in the loop, because it may be the last type check in the loop protecting
        // other fields which are not hoistable and are lexically upstream in the loop.  If the check is not ultimately
        // needed, the dead store pass will remove it.
        if (this->currentBlock->loop != nullptr && opnd->IsSymOpnd() && opnd->AsSymOpnd()->IsPropertySymOpnd())
        {
            IR::PropertySymOpnd* propertySymOpnd = opnd->AsPropertySymOpnd();
            if (CheckIfPropOpEmitsTypeCheck(instr, propertySymOpnd))
            {
                // We only set guarded properties in the dead store pass, so they shouldn't be set here yet. If they were
                // we would need to move them from this operand to the operand which is being copy propagated.
                Assert(propertySymOpnd->GetGuardedPropOps() == nullptr);

                // We're creating a copy of this operand to be reused in the same spot in the flow, so we can copy all
                // flow sensitive fields.  However, we will do only a type check here (no property access) and only for
                // the sake of downstream instructions, so the flags pertaining to this property access are irrelevant.
                IR::PropertySymOpnd* checkObjTypeOpnd = CreateOpndForTypeCheckOnly(propertySymOpnd, instr->m_func);
                IR::Instr* checkObjTypeInstr = IR::Instr::New(Js::OpCode::CheckObjType, instr->m_func);
                checkObjTypeInstr->SetSrc1(checkObjTypeOpnd);
                checkObjTypeInstr->SetByteCodeOffset(instr);
                instr->InsertBefore(checkObjTypeInstr);

                // Since we inserted this instruction before the one that is being processed in natural flow, we must process
                // it for object type spec explicitly here.
                FinishOptPropOp(checkObjTypeInstr, checkObjTypeOpnd);
                Assert(!propertySymOpnd->IsTypeChecked());
                checkObjTypeInstr = this->SetTypeCheckBailOut(checkObjTypeOpnd, checkObjTypeInstr, nullptr);
                Assert(checkObjTypeInstr->HasBailOutInfo());

                if (this->currentBlock->loop && !this->IsLoopPrePass())
                {
                    // Try hoisting this checkObjType.
                    // But since this isn't the current instr being optimized, we need to play tricks with
                    // the byteCodeUse fields...
                    BVSparse<JitArenaAllocator> *currentBytecodeUses = this->byteCodeUses;
                    PropertySym * currentPropertySymUse = this->propertySymUse;
                    PropertySym * tempPropertySymUse = NULL;
                    this->byteCodeUses = NULL;
                    BVSparse<JitArenaAllocator> *tempByteCodeUse = JitAnew(this->tempAlloc, BVSparse<JitArenaAllocator>, this->tempAlloc);
#if DBG
                    BVSparse<JitArenaAllocator> *currentBytecodeUsesBeforeOpt = this->byteCodeUsesBeforeOpt;
                    this->byteCodeUsesBeforeOpt = tempByteCodeUse;
#endif
                    this->propertySymUse = NULL;
                    GlobOpt::TrackByteCodeSymUsed(checkObjTypeInstr, tempByteCodeUse, &tempPropertySymUse);

                    TryHoistInvariant(checkObjTypeInstr, this->currentBlock, NULL, CurrentBlockData()->FindValue(copySym), NULL, true);

                    this->byteCodeUses = currentBytecodeUses;
                    this->propertySymUse = currentPropertySymUse;
#if DBG
                    this->byteCodeUsesBeforeOpt = currentBytecodeUsesBeforeOpt;
#endif
                }
            }
        }

        if (opnd->IsSymOpnd() && opnd->GetIsDead())
        {
            // Take the property sym out of the live fields set
            this->EndFieldLifetime(opnd->AsSymOpnd());
        }
        regOpnd = IR::RegOpnd::New(newSym, opnd->GetType(), instr->m_func);
        regOpnd->SetIsJITOptimizedReg(true);
        instr->ReplaceSrc(opnd, regOpnd);
    }

    switch (instr->m_opcode)
    {
    case Js::OpCode::Ld_A:
        if (instr->GetDst()->IsRegOpnd() && instr->GetSrc1()->IsRegOpnd() &&
            instr->GetDst()->AsRegOpnd()->GetStackSym() == instr->GetSrc1()->AsRegOpnd()->GetStackSym())
        {
            this->InsertByteCodeUses(instr, true);
            instr->m_opcode = Js::OpCode::Nop;
        }
        break;

    case Js::OpCode::LdSlot:
    case Js::OpCode::LdSlotArr:
        if (instr->GetDst()->IsRegOpnd() && instr->GetSrc1()->IsRegOpnd() &&
            instr->GetDst()->AsRegOpnd()->GetStackSym() == instr->GetSrc1()->AsRegOpnd()->GetStackSym())
        {
            this->InsertByteCodeUses(instr, true);
            instr->m_opcode = Js::OpCode::Nop;
        }
        else
        {
            instr->m_opcode = Js::OpCode::Ld_A;
        }
        break;

    case Js::OpCode::StSlotChkUndecl:
        if (instr->GetSrc2()->IsRegOpnd())
        {
            // Src2 here should refer to the same location as the Dst operand, which we need to keep live
            // due to the implicit read for ChkUndecl.
            instr->m_opcode = Js::OpCode::StSlot;
            instr->FreeSrc2();
            return nullptr;
        }
        break;

    case Js::OpCode::LdFld:
    case Js::OpCode::LdFldForTypeOf:
    case Js::OpCode::LdRootFldForTypeOf:
    case Js::OpCode::LdFldForCallApplyTarget:
    case Js::OpCode::LdRootFld:
    case Js::OpCode::LdMethodFld:
    case Js::OpCode::LdRootMethodFld:
    case Js::OpCode::ScopedLdMethodFld:
        instr->m_opcode = Js::OpCode::Ld_A;
        break;

    case Js::OpCode::LdMethodFromFlags:
        // The bailout is checked on the loop top and we don't need to check bailout again in loop.
        instr->m_opcode = Js::OpCode::Ld_A;
        instr->ClearBailOutInfo();
        break;

    case Js::OpCode::TypeofElem:
        instr->m_opcode = Js::OpCode::Typeof;
        break;
    }
    CurrentBlockData()->MarkTempLastUse(instr, regOpnd);

    return regOpnd;
}

ValueNumber
GlobOpt::NewValueNumber()
{
    ValueNumber valueNumber = this->currentValue++;

    if (valueNumber == 0)
    {
        Js::Throw::OutOfMemory();
    }
    return valueNumber;
}

Value *GlobOpt::NewValue(ValueInfo *const valueInfo)
{
    return NewValue(NewValueNumber(), valueInfo);
}

Value *GlobOpt::NewValue(const ValueNumber valueNumber, ValueInfo *const valueInfo)
{
    Assert(valueInfo);

    return Value::New(alloc, valueNumber, valueInfo);
}

Value *GlobOpt::CopyValue(Value const *const value)
{
    return CopyValue(value, NewValueNumber());
}

Value *GlobOpt::CopyValue(Value const *const value, const ValueNumber valueNumber)
{
    Assert(value);

    return value->Copy(alloc, valueNumber);
}

Value *
GlobOpt::NewGenericValue(const ValueType valueType)
{
    return NewGenericValue(valueType, static_cast<IR::Opnd *>(nullptr));
}

Value *
GlobOpt::NewGenericValue(const ValueType valueType, IR::Opnd *const opnd)
{
    // Shouldn't assign a likely-int value to something that is definitely not an int
    Assert(!(valueType.IsLikelyInt() && opnd && opnd->IsRegOpnd() && opnd->AsRegOpnd()->m_sym->m_isNotInt));

    ValueInfo *valueInfo = ValueInfo::New(this->alloc, valueType);
    Value *val = NewValue(valueInfo);
    TrackNewValueForKills(val);

    CurrentBlockData()->InsertNewValue(val, opnd);
    return val;
}

Value *
GlobOpt::NewGenericValue(const ValueType valueType, Sym *const sym)
{
    ValueInfo *valueInfo = ValueInfo::New(this->alloc, valueType);
    Value *val = NewValue(valueInfo);
    TrackNewValueForKills(val);

    CurrentBlockData()->SetValue(val, sym);
    return val;
}

Value *
GlobOpt::GetIntConstantValue(const int32 intConst, IR::Instr * instr, IR::Opnd *const opnd)
{
    Value *value = nullptr;
    Value *const cachedValue = this->intConstantToValueMap->Lookup(intConst, nullptr);

    if(cachedValue)
    {
        // The cached value could be from a different block since this is a global (as opposed to a per-block) cache. Since
        // values are cloned for each block, we can't use the same value object. We also can't have two values with the same
        // number in one block, so we can't simply copy the cached value either. And finally, there is no deterministic and fast
        // way to determine if a value with the same value number exists for this block. So the best we can do with a global
        // cache is to check the sym-store's value in the current block to see if it has a value with the same number.
        // Otherwise, we have to create a new value with a new value number.
        Sym *const symStore = cachedValue->GetValueInfo()->GetSymStore();
        if (symStore && CurrentBlockData()->IsLive(symStore))
        {

            Value *const symStoreValue = CurrentBlockData()->FindValue(symStore);
            int32 symStoreIntConstantValue;
            if (symStoreValue &&
                symStoreValue->GetValueNumber() == cachedValue->GetValueNumber() &&
                symStoreValue->GetValueInfo()->TryGetIntConstantValue(&symStoreIntConstantValue) &&
                symStoreIntConstantValue == intConst)
            {
                value = symStoreValue;
            }
        }
    }

    if (!value)
    {
        value = NewIntConstantValue(intConst, instr, !Js::TaggedInt::IsOverflow(intConst));
    }

    return CurrentBlockData()->InsertNewValue(value, opnd);
}

Value *
GlobOpt::GetIntConstantValue(const int64 intConst, IR::Instr * instr, IR::Opnd *const opnd)
{
    Assert(instr->m_func->GetJITFunctionBody()->IsWasmFunction());

    Value *value = nullptr;
    Value *const cachedValue = this->int64ConstantToValueMap->Lookup(intConst, nullptr);

    if (cachedValue)
    {
        // The cached value could be from a different block since this is a global (as opposed to a per-block) cache. Since
        // values are cloned for each block, we can't use the same value object. We also can't have two values with the same
        // number in one block, so we can't simply copy the cached value either. And finally, there is no deterministic and fast
        // way to determine if a value with the same value number exists for this block. So the best we can do with a global
        // cache is to check the sym-store's value in the current block to see if it has a value with the same number.
        // Otherwise, we have to create a new value with a new value number.
        Sym *const symStore = cachedValue->GetValueInfo()->GetSymStore();
        if (symStore && this->currentBlock->globOptData.IsLive(symStore))
        {

            Value *const symStoreValue = this->currentBlock->globOptData.FindValue(symStore);
            int64 symStoreIntConstantValue;
            if (symStoreValue &&
                symStoreValue->GetValueNumber() == cachedValue->GetValueNumber() &&
                symStoreValue->GetValueInfo()->TryGetInt64ConstantValue(&symStoreIntConstantValue, false) &&
                symStoreIntConstantValue == intConst)
            {
                value = symStoreValue;
            }
        }
    }

    if (!value)
    {
        value = NewInt64ConstantValue(intConst, instr);
    }

    return this->currentBlock->globOptData.InsertNewValue(value, opnd);
}

Value *
GlobOpt::NewInt64ConstantValue(const int64 intConst, IR::Instr* instr)
{
    Value * value = NewValue(Int64ConstantValueInfo::New(this->alloc, intConst));
    this->int64ConstantToValueMap->Item(intConst, value);

    if (!value->GetValueInfo()->GetSymStore() &&
        (instr->m_opcode == Js::OpCode::LdC_A_I4 || instr->m_opcode == Js::OpCode::Ld_I4))
    {
        StackSym * sym = instr->GetDst()->GetStackSym();
        Assert(sym && !sym->IsTypeSpec());
        this->currentBlock->globOptData.SetValue(value, sym);
        this->currentBlock->globOptData.liveVarSyms->Set(sym->m_id);
    }
    return value;
}

Value *
GlobOpt::NewIntConstantValue(const int32 intConst, IR::Instr * instr, bool isTaggable)
{
    Value * value = NewValue(IntConstantValueInfo::New(this->alloc, intConst));
    this->intConstantToValueMap->Item(intConst, value);

    if (isTaggable &&
        !PHASE_OFF(Js::HoistConstIntPhase, this->func))
    {
        // When creating a new int constant value, make sure it gets a symstore. If the int const doesn't have a symstore,
        // any downstream instruction using the same int will have to create a new value (object) for the int.
        // This gets in the way of CSE.
        value = HoistConstantLoadAndPropagateValueBackward(Js::TaggedInt::ToVarUnchecked(intConst), instr, value);
        if (!value->GetValueInfo()->GetSymStore() &&
            (instr->m_opcode == Js::OpCode::LdC_A_I4 || instr->m_opcode == Js::OpCode::Ld_I4))
        {
            StackSym * sym = instr->GetDst()->GetStackSym();
            Assert(sym);
            if (sym->IsTypeSpec())
            {
                Assert(sym->IsInt32());
                StackSym * varSym = sym->GetVarEquivSym(instr->m_func);
                CurrentBlockData()->SetValue(value, varSym);
                CurrentBlockData()->liveInt32Syms->Set(varSym->m_id);
            }
            else
            {
                CurrentBlockData()->SetValue(value, sym);
                CurrentBlockData()->liveVarSyms->Set(sym->m_id);
            }
        }
    }
    return value;
}

ValueInfo *
GlobOpt::NewIntRangeValueInfo(const int32 min, const int32 max, const bool wasNegativeZeroPreventedByBailout)
{
    return ValueInfo::NewIntRangeValueInfo(this->alloc, min, max, wasNegativeZeroPreventedByBailout);
}

ValueInfo *GlobOpt::NewIntRangeValueInfo(
    const ValueInfo *const originalValueInfo,
    const int32 min,
    const int32 max) const
{
    Assert(originalValueInfo);

    ValueInfo *valueInfo;
    if(min == max)
    {
        // Since int constant values are const-propped, negative zero tracking does not track them, and so it's okay to ignore
        // 'wasNegativeZeroPreventedByBailout'
        valueInfo = IntConstantValueInfo::New(alloc, min);
    }
    else
    {
        valueInfo =
            IntRangeValueInfo::New(
                alloc,
                min,
                max,
                min <= 0 && max >= 0 && originalValueInfo->WasNegativeZeroPreventedByBailout());
    }
    valueInfo->SetSymStore(originalValueInfo->GetSymStore());
    return valueInfo;
}

Value *
GlobOpt::NewIntRangeValue(
    const int32 min,
    const int32 max,
    const bool wasNegativeZeroPreventedByBailout,
    IR::Opnd *const opnd)
{
    ValueInfo *valueInfo = this->NewIntRangeValueInfo(min, max, wasNegativeZeroPreventedByBailout);
    Value *val = NewValue(valueInfo);

    if (opnd)
    {
        GOPT_TRACE_OPND(opnd, _u("Range %d (0x%X) to %d (0x%X)\n"), min, min, max, max);
    }
    CurrentBlockData()->InsertNewValue(val, opnd);
    return val;
}

IntBoundedValueInfo *GlobOpt::NewIntBoundedValueInfo(
    const ValueInfo *const originalValueInfo,
    const IntBounds *const bounds) const
{
    Assert(originalValueInfo);
    bounds->Verify();

    IntBoundedValueInfo *const valueInfo =
        IntBoundedValueInfo::New(
            originalValueInfo->Type(),
            bounds,
            (
                bounds->ConstantLowerBound() <= 0 &&
                bounds->ConstantUpperBound() >= 0 &&
                originalValueInfo->WasNegativeZeroPreventedByBailout()
            ),
            alloc);
    valueInfo->SetSymStore(originalValueInfo->GetSymStore());
    return valueInfo;
}

Value *GlobOpt::NewIntBoundedValue(
    const ValueType valueType,
    const IntBounds *const bounds,
    const bool wasNegativeZeroPreventedByBailout,
    IR::Opnd *const opnd)
{
    Value *const value = NewValue(IntBoundedValueInfo::New(valueType, bounds, wasNegativeZeroPreventedByBailout, alloc));
    CurrentBlockData()->InsertNewValue(value, opnd);
    return value;
}

Value *
GlobOpt::NewFloatConstantValue(const FloatConstType floatValue, IR::Opnd *const opnd)
{
    FloatConstantValueInfo *valueInfo = FloatConstantValueInfo::New(this->alloc, floatValue);
    Value *val = NewValue(valueInfo);

    CurrentBlockData()->InsertNewValue(val, opnd);
    return val;
}

Value *
GlobOpt::GetVarConstantValue(IR::AddrOpnd *addrOpnd)
{
    bool isVar = addrOpnd->IsVar();
    bool isString = isVar && addrOpnd->m_localAddress && JITJavascriptString::Is(addrOpnd->m_localAddress);
    Value *val = nullptr;
    Value *cachedValue = nullptr;
    if(this->addrConstantToValueMap->TryGetValue(addrOpnd->m_address, &cachedValue))
    {
        // The cached value could be from a different block since this is a global (as opposed to a per-block) cache. Since
        // values are cloned for each block, we can't use the same value object. We also can't have two values with the same
        // number in one block, so we can't simply copy the cached value either. And finally, there is no deterministic and fast
        // way to determine if a value with the same value number exists for this block. So the best we can do with a global
        // cache is to check the sym-store's value in the current block to see if it has a value with the same number.
        // Otherwise, we have to create a new value with a new value number.
        Sym *symStore = cachedValue->GetValueInfo()->GetSymStore();
        if(symStore && CurrentBlockData()->IsLive(symStore))
        {
            Value *const symStoreValue = CurrentBlockData()->FindValue(symStore);
            if(symStoreValue && symStoreValue->GetValueNumber() == cachedValue->GetValueNumber())
            {
                ValueInfo *const symStoreValueInfo = symStoreValue->GetValueInfo();
                if(symStoreValueInfo->IsVarConstant() && symStoreValueInfo->AsVarConstant()->VarValue() == addrOpnd->m_address)
                {
                    val = symStoreValue;
                }
            }
        }
    }
    else if (isString)
    {
        JITJavascriptString* jsString = JITJavascriptString::FromVar(addrOpnd->m_localAddress);
        Js::InternalString internalString(jsString->GetString(), jsString->GetLength());
        if (this->stringConstantToValueMap->TryGetValue(internalString, &cachedValue))
        {
            Sym *symStore = cachedValue->GetValueInfo()->GetSymStore();
            if (symStore && CurrentBlockData()->IsLive(symStore))
            {
                Value *const symStoreValue = CurrentBlockData()->FindValue(symStore);
                if (symStoreValue && symStoreValue->GetValueNumber() == cachedValue->GetValueNumber())
                {
                    ValueInfo *const symStoreValueInfo = symStoreValue->GetValueInfo();
                    if (symStoreValueInfo->IsVarConstant())
                    {
                        JITJavascriptString * cachedString = JITJavascriptString::FromVar(symStoreValue->GetValueInfo()->AsVarConstant()->VarValue(true));
                        Js::InternalString cachedInternalString(cachedString->GetString(), cachedString->GetLength());
                        if (Js::InternalStringComparer::Equals(internalString, cachedInternalString))
                        {
                            val = symStoreValue;
                        }
                    }
                }
            }
        }
    }

    if(!val)
    {
        val = NewVarConstantValue(addrOpnd, isString);
    }

    addrOpnd->SetValueType(val->GetValueInfo()->Type());
    return val;
}

Value *
GlobOpt::NewVarConstantValue(IR::AddrOpnd *addrOpnd, bool isString)
{
    VarConstantValueInfo *valueInfo = VarConstantValueInfo::New(this->alloc, addrOpnd->m_address, addrOpnd->GetValueType(), false, addrOpnd->m_localAddress);
    Value * value = NewValue(valueInfo);
    this->addrConstantToValueMap->Item(addrOpnd->m_address, value);
    if (isString)
    {
        JITJavascriptString* jsString = JITJavascriptString::FromVar(addrOpnd->m_localAddress);
        Js::InternalString internalString(jsString->GetString(), jsString->GetLength());
        this->stringConstantToValueMap->Item(internalString, value);
    }
    return value;
}

Value *
GlobOpt::HoistConstantLoadAndPropagateValueBackward(Js::Var varConst, IR::Instr * origInstr, Value * value)
{
    if (this->IsLoopPrePass() ||
        ((this->currentBlock == this->func->m_fg->blockList) &&
        origInstr->TransfersSrcValue()))
    {
        return value;
    }

    // Only hoisting taggable int const loads for now. Could be extended to other constants (floats, strings, addr opnds) if we see some benefit.
    Assert(Js::TaggedInt::Is(varConst));

    // Insert a load of the constant at the top of the function
    StackSym *    dstSym = StackSym::New(this->func);
    IR::RegOpnd * constRegOpnd = IR::RegOpnd::New(dstSym, TyVar, this->func);
    IR::Instr *   loadInstr = IR::Instr::NewConstantLoad(constRegOpnd, (intptr_t)varConst, ValueType::GetInt(true), this->func);
    this->func->m_fg->blockList->GetFirstInstr()->InsertAfter(loadInstr);

    // Type-spec the load (Support for floats needs to be added when we start hoisting float constants).
    bool typeSpecedToInt = false;
    if (Js::TaggedInt::Is(varConst) && !IsTypeSpecPhaseOff(this->func))
    {
        typeSpecedToInt = true;
        loadInstr->m_opcode = Js::OpCode::Ld_I4;
        ToInt32Dst(loadInstr, loadInstr->GetDst()->AsRegOpnd(), this->currentBlock);
        loadInstr->GetDst()->GetStackSym()->SetIsConst();
    }
    else
    {
        CurrentBlockData()->liveVarSyms->Set(dstSym->m_id);
    }

    // Add the value (object) to the current block's symToValueMap and propagate the value backward to all relevant blocks so it is available on merges.
    value = CurrentBlockData()->InsertNewValue(value, constRegOpnd);

    BVSparse<JitArenaAllocator>* GlobOptBlockData::*bv;
    bv = typeSpecedToInt ? &GlobOptBlockData::liveInt32Syms : &GlobOptBlockData::liveVarSyms; // Will need to be expanded when we start hoisting float constants.
    if (this->currentBlock != this->func->m_fg->blockList)
    {
        for (InvariantBlockBackwardIterator it(this, this->currentBlock, this->func->m_fg->blockList, nullptr);
             it.IsValid();
             it.MoveNext())
        {
            BasicBlock * block = it.Block();
            (block->globOptData.*bv)->Set(dstSym->m_id);
            Assert(!block->globOptData.FindValue(dstSym));
            Value *const valueCopy = CopyValue(value, value->GetValueNumber());
            block->globOptData.SetValue(valueCopy, dstSym);
        }
    }

    return value;
}

Value *
GlobOpt::NewFixedFunctionValue(Js::JavascriptFunction *function, IR::AddrOpnd *addrOpnd)
{
    Assert(function != nullptr);

    Value *val = nullptr;
    Value *cachedValue = nullptr;
    if(this->addrConstantToValueMap->TryGetValue(addrOpnd->m_address, &cachedValue))
    {
        // The cached value could be from a different block since this is a global (as opposed to a per-block) cache. Since
        // values are cloned for each block, we can't use the same value object. We also can't have two values with the same
        // number in one block, so we can't simply copy the cached value either. And finally, there is no deterministic and fast
        // way to determine if a value with the same value number exists for this block. So the best we can do with a global
        // cache is to check the sym-store's value in the current block to see if it has a value with the same number.
        // Otherwise, we have to create a new value with a new value number.
        Sym *symStore = cachedValue->GetValueInfo()->GetSymStore();
        if(symStore && CurrentBlockData()->IsLive(symStore))
        {
            Value *const symStoreValue = CurrentBlockData()->FindValue(symStore);
            if(symStoreValue && symStoreValue->GetValueNumber() == cachedValue->GetValueNumber())
            {
                ValueInfo *const symStoreValueInfo = symStoreValue->GetValueInfo();
                if(symStoreValueInfo->IsVarConstant())
                {
                    VarConstantValueInfo *const symStoreVarConstantValueInfo = symStoreValueInfo->AsVarConstant();
                    if(symStoreVarConstantValueInfo->VarValue() == addrOpnd->m_address &&
                        symStoreVarConstantValueInfo->IsFunction())
                    {
                        val = symStoreValue;
                    }
                }
            }
        }
    }

    if(!val)
    {
        VarConstantValueInfo *valueInfo = VarConstantValueInfo::New(this->alloc, function, addrOpnd->GetValueType(), true, addrOpnd->m_localAddress);
        val = NewValue(valueInfo);
        this->addrConstantToValueMap->AddNew(addrOpnd->m_address, val);
    }

    CurrentBlockData()->InsertNewValue(val, addrOpnd);
    return val;
}

StackSym *GlobOpt::GetTaggedIntConstantStackSym(const int32 intConstantValue) const
{
    Assert(!Js::TaggedInt::IsOverflow(intConstantValue));

    return intConstantToStackSymMap->Lookup(intConstantValue, nullptr);
}

StackSym *GlobOpt::GetOrCreateTaggedIntConstantStackSym(const int32 intConstantValue) const
{
    StackSym *stackSym = GetTaggedIntConstantStackSym(intConstantValue);
    if(stackSym)
    {
        return stackSym;
    }

    stackSym = StackSym::New(TyVar,func);
    intConstantToStackSymMap->Add(intConstantValue, stackSym);
    return stackSym;
}

Sym *
GlobOpt::SetSymStore(ValueInfo *valueInfo, Sym *sym)
{
    if (sym->IsStackSym())
    {
        StackSym *stackSym = sym->AsStackSym();

        if (stackSym->IsTypeSpec())
        {
            stackSym = stackSym->GetVarEquivSym(this->func);
            sym = stackSym;
        }
    }
    if (valueInfo->GetSymStore() == nullptr || valueInfo->GetSymStore()->IsPropertySym())
    {
        SetSymStoreDirect(valueInfo, sym);
    }

    return sym;
}

void
GlobOpt::SetSymStoreDirect(ValueInfo * valueInfo, Sym * sym)
{
    Sym * prevSymStore = valueInfo->GetSymStore();
    if (prevSymStore && prevSymStore->IsStackSym() &&
        prevSymStore->AsStackSym()->HasByteCodeRegSlot())
    {
        CurrentBlockData()->SetChangedSym(prevSymStore->m_id);
    }
    valueInfo->SetSymStore(sym);
}

// Figure out the Value of this dst.
Value *
GlobOpt::ValueNumberDst(IR::Instr **pInstr, Value *src1Val, Value *src2Val)
{
    IR::Instr *&instr = *pInstr;
    IR::Opnd *dst = instr->GetDst();
    Value *dstVal = nullptr;
    Sym *sym;

    if (instr->CallsSetter())
    {
        return nullptr;
    }
    if (dst == nullptr)
    {
        return nullptr;
    }

    switch (dst->GetKind())
    {
    case IR::OpndKindSym:
        sym = dst->AsSymOpnd()->m_sym;
        break;

    case IR::OpndKindReg:
        sym = dst->AsRegOpnd()->m_sym;

        if (OpCodeAttr::TempNumberProducing(instr->m_opcode))
        {
            CurrentBlockData()->isTempSrc->Set(sym->m_id);
        }
        else if (OpCodeAttr::TempNumberTransfer(instr->m_opcode))
        {
            IR::Opnd *src1 = instr->GetSrc1();

            if (src1->IsRegOpnd() && CurrentBlockData()->isTempSrc->Test(src1->AsRegOpnd()->m_sym->m_id))
            {
                StackSym *src1Sym = src1->AsRegOpnd()->m_sym;
                // isTempSrc is used for marking isTempLastUse, which is used to generate AddLeftDead()
                // calls instead of the normal Add helpers. It tells the runtime that concats can use string
                // builders.
                // We need to be careful in the case where src1 points to a string builder and is getting aliased.
                // Clear the bit on src and dst of the transfer instr in this case, unless we can prove src1
                // isn't pointing at a string builder, like if it is single def and the def instr is not an Add,
                // but TempProducing.
                if (src1Sym->IsSingleDef() && src1Sym->m_instrDef->m_opcode != Js::OpCode::Add_A
                    && OpCodeAttr::TempNumberProducing(src1Sym->m_instrDef->m_opcode))
                {
                    CurrentBlockData()->isTempSrc->Set(sym->m_id);
                }
                else
                {
                    CurrentBlockData()->isTempSrc->Clear(src1->AsRegOpnd()->m_sym->m_id);
                    CurrentBlockData()->isTempSrc->Clear(sym->m_id);
                }
            }
            else
            {
                CurrentBlockData()->isTempSrc->Clear(sym->m_id);
            }
        }
        else
        {
            CurrentBlockData()->isTempSrc->Clear(sym->m_id);
        }
        break;

    case IR::OpndKindIndir:
        return nullptr;

    default:
        return nullptr;
    }

    int32 min1, max1, min2, max2, newMin, newMax;
    ValueInfo *src1ValueInfo = (src1Val ? src1Val->GetValueInfo() : nullptr);
    ValueInfo *src2ValueInfo = (src2Val ? src2Val->GetValueInfo() : nullptr);

    switch (instr->m_opcode)
    {
    case Js::OpCode::Conv_PrimStr:
        AssertMsg(instr->GetDst()->GetValueType().IsString(),
            "Creator of this instruction should have set the type");
        if (this->IsLoopPrePass() || src1ValueInfo == nullptr || !src1ValueInfo->IsPrimitive())
        {
            break;
        }
        instr->m_opcode = Js::OpCode::Conv_Str;
        // fall-through
    case Js::OpCode::Conv_Str:
    // This opcode is commented out since we don't track regex information in GlobOpt now.
    //case Js::OpCode::Coerce_Regex:
    case Js::OpCode::Coerce_Str:
        AssertMsg(instr->GetDst()->GetValueType().IsString(),
            "Creator of this instruction should have set the type");
        // fall-through
    case Js::OpCode::Coerce_StrOrRegex:
        // We don't set the ValueType of src1 for Coerce_StrOrRegex, hence skip the ASSERT
        if (this->IsLoopPrePass() || src1ValueInfo == nullptr || !src1ValueInfo->IsString())
        {
            break;
        }
        instr->m_opcode = Js::OpCode::Ld_A;
        // fall-through

    case Js::OpCode::BytecodeArgOutCapture:
    case Js::OpCode::InitConst:
    case Js::OpCode::LdAsmJsFunc:
    case Js::OpCode::Ld_A:
    case Js::OpCode::Ld_I4:

        // Propagate sym attributes across the reg copy.
        if (!this->IsLoopPrePass() && instr->GetSrc1()->IsRegOpnd())
        {
            if (dst->AsRegOpnd()->m_sym->IsSingleDef())
            {
                dst->AsRegOpnd()->m_sym->CopySymAttrs(instr->GetSrc1()->AsRegOpnd()->m_sym);
            }
        }

        if (instr->IsProfiledInstr())
        {
            const ValueType profiledValueType(instr->AsProfiledInstr()->u.FldInfo().valueType);
            if(!(
                    profiledValueType.IsLikelyInt() &&
                    (
                        (dst->IsRegOpnd() && dst->AsRegOpnd()->m_sym->m_isNotInt) ||
                        (instr->GetSrc1()->IsRegOpnd() && instr->GetSrc1()->AsRegOpnd()->m_sym->m_isNotInt)
                    )
                ))
            {
                if(!src1ValueInfo)
                {
                    dstVal = this->NewGenericValue(profiledValueType, dst);
                }
                else if(src1ValueInfo->IsUninitialized())
                {
                    if(IsLoopPrePass())
                    {
                        dstVal = this->NewGenericValue(profiledValueType, dst);
                    }
                    else
                    {
                        // Assuming the profile data gives more precise value types based on the path it took at runtime, we
                        // can improve the original value type.
                        src1ValueInfo->Type() = profiledValueType;
                        instr->GetSrc1()->SetValueType(profiledValueType);
                    }
                }
            }
        }

        if (dstVal == nullptr)
        {
            // Ld_A is just transferring the value
            dstVal = this->ValueNumberTransferDst(instr, src1Val);
        }

        break;

    case Js::OpCode::ExtendArg_A:
    {
        // SIMD_JS
        // We avoid transforming EAs to Lds to keep the IR shape consistent and avoid CSEing of EAs.
        // CSEOptimize only assigns a Value to the EA dst, and doesn't turn it to a Ld. If this happened, we shouldn't assign a new Value here.
        if (DoCSE())
        {
            IR::Opnd * currDst = instr->GetDst();
            Value * currDstVal = CurrentBlockData()->FindValue(currDst->GetStackSym());
            if (currDstVal != nullptr)
            {
                return currDstVal;
            }
        }
        break;
    }

    case Js::OpCode::CheckFixedFld:
        AssertMsg(false, "CheckFixedFld doesn't have a dst, so we should never get here");
        break;

    case Js::OpCode::LdSlot:
    case Js::OpCode::LdSlotArr:
    case Js::OpCode::LdFld:
    case Js::OpCode::LdFldForTypeOf:
    case Js::OpCode::LdFldForCallApplyTarget:
    // Do not transfer value type on ldFldForTypeOf to prevent copy-prop to LdRootFld in case the field doesn't exist since LdRootFldForTypeOf does not throw
    //case Js::OpCode::LdRootFldForTypeOf:
    case Js::OpCode::LdRootFld:
    case Js::OpCode::LdMethodFld:
    case Js::OpCode::LdRootMethodFld:
    case Js::OpCode::ScopedLdMethodFld:
    case Js::OpCode::LdMethodFromFlags:
        if (instr->IsProfiledInstr())
        {
            ValueType profiledValueType(instr->AsProfiledInstr()->u.FldInfo().valueType);
            if(!(profiledValueType.IsLikelyInt() && dst->IsRegOpnd() && dst->AsRegOpnd()->m_sym->m_isNotInt))
            {
                if(!src1ValueInfo)
                {
                    dstVal = this->NewGenericValue(profiledValueType, dst);
                }
                else if(src1ValueInfo->IsUninitialized())
                {
                    if(IsLoopPrePass() && (!dst->IsRegOpnd() || !dst->AsRegOpnd()->m_sym->IsSingleDef() || DoFieldHoisting()))
                    {
                        dstVal = this->NewGenericValue(profiledValueType, dst);
                    }
                    else
                    {
                        // Assuming the profile data gives more precise value types based on the path it took at runtime, we
                        // can improve the original value type.
                        src1ValueInfo->Type() = profiledValueType;
                        instr->GetSrc1()->SetValueType(profiledValueType);
                    }
                }
            }
        }
        if (dstVal == nullptr)
        {
            dstVal = this->ValueNumberTransferDst(instr, src1Val);
        }

        if(!this->IsLoopPrePass())
        {
            // We cannot transfer value if the field hasn't been copy prop'd because we don't generate
            // an implicit call bailout between those values if we don't have "live fields" unless, we are hoisting the field.
            PropertySym *propertySym = instr->GetSrc1()->AsSymOpnd()->m_sym->AsPropertySym();
            StackSym * fieldHoistSym;
            Loop * loop = this->FindFieldHoistStackSym(this->currentBlock->loop, propertySym->m_id, &fieldHoistSym, instr);
            ValueInfo *dstValueInfo = (dstVal ? dstVal->GetValueInfo() : nullptr);

            // Update symStore for field hoisting
            if (loop != nullptr && (dstValueInfo != nullptr))
            {
                this->SetSymStoreDirect(dstValueInfo, fieldHoistSym);
            }
            // Update symStore if it isn't a stackSym
            if (dstVal && (!dstValueInfo->GetSymStore() || !dstValueInfo->GetSymStore()->IsStackSym()))
            {
                Assert(dst->IsRegOpnd());
                this->SetSymStoreDirect(dstValueInfo, dst->AsRegOpnd()->m_sym);
            }
            if (src1Val != dstVal)
            {
                CurrentBlockData()->SetValue(dstVal, instr->GetSrc1());
            }
        }
        break;

    case Js::OpCode::LdC_A_R8:
    case Js::OpCode::LdC_A_I4:
    case Js::OpCode::ArgIn_A:
        dstVal = src1Val;
        break;

    case Js::OpCode::LdStr:
        if (src1Val == nullptr)
        {
            src1Val = NewGenericValue(ValueType::String, dst);
        }
        dstVal = src1Val;
        break;

    // LdElemUndef only assign undef if the field doesn't exist.
    // So we don't actually know what the value is, so we can't really copy prop it.
    //case Js::OpCode::LdElemUndef:
    case Js::OpCode::StSlot:
    case Js::OpCode::StSlotChkUndecl:
    case Js::OpCode::StFld:
    case Js::OpCode::StRootFld:
    case Js::OpCode::StFldStrict:
    case Js::OpCode::StRootFldStrict:
        if (DoFieldCopyProp())
        {
            if (src1Val == nullptr)
            {
                // src1 may have no value if it's not a valid var, e.g., NULL for let/const initialization.
                // Consider creating generic values for such things.
                return nullptr;
            }
            AssertMsg(!src2Val, "Bad src Values...");

            Assert(sym->IsPropertySym());
            SymID symId = sym->m_id;
            Assert(instr->m_opcode == Js::OpCode::StSlot || instr->m_opcode == Js::OpCode::StSlotChkUndecl || !CurrentBlockData()->liveFields->Test(symId));
            if (IsHoistablePropertySym(symId))
            {
                // We have changed the value of a hoistable field, load afterwards shouldn't get hoisted,
                // but we will still copy prop the pre-assign sym to it if we have a live value.
                Assert((instr->m_opcode == Js::OpCode::StSlot || instr->m_opcode == Js::OpCode::StSlotChkUndecl) && CurrentBlockData()->liveFields->Test(symId));
                CurrentBlockData()->hoistableFields->Clear(symId);
            }
            CurrentBlockData()->liveFields->Set(symId);
            if (!this->IsLoopPrePass() && dst->GetIsDead())
            {
                // Take the property sym out of the live fields set (with special handling for loops).
                this->EndFieldLifetime(dst->AsSymOpnd());
            }

            dstVal = this->ValueNumberTransferDst(instr, src1Val);
        }
        else
        {
            return nullptr;
        }
        break;

    case Js::OpCode::Conv_Num:
        if(src1ValueInfo->IsNumber())
        {
            dstVal = ValueNumberTransferDst(instr, src1Val);
        }
        else
        {
            return NewGenericValue(src1ValueInfo->Type().ToDefiniteAnyNumber(), dst);
        }
        break;

    case Js::OpCode::Not_A:
    {
        if (!src1Val || !src1ValueInfo->GetIntValMinMax(&min1, &max1, this->DoAggressiveIntTypeSpec()))
        {
            min1 = INT32_MIN;
            max1 = INT32_MAX;
        }

        this->PropagateIntRangeForNot(min1, max1, &newMin, &newMax);
        return CreateDstUntransferredIntValue(newMin, newMax, instr, src1Val, src2Val);
    }

    case Js::OpCode::Xor_A:
    case Js::OpCode::Or_A:
    case Js::OpCode::And_A:
    case Js::OpCode::Shl_A:
    case Js::OpCode::Shr_A:
    case Js::OpCode::ShrU_A:
    {
        if (!src1Val || !src1ValueInfo->GetIntValMinMax(&min1, &max1, this->DoAggressiveIntTypeSpec()))
        {
            min1 = INT32_MIN;
            max1 = INT32_MAX;
        }
        if (!src2Val || !src2ValueInfo->GetIntValMinMax(&min2, &max2, this->DoAggressiveIntTypeSpec()))
        {
            min2 = INT32_MIN;
            max2 = INT32_MAX;
        }

        if (instr->m_opcode == Js::OpCode::ShrU_A &&
            min1 < 0 &&
            IntConstantBounds(min2, max2).And_0x1f().Contains(0))
        {
            // Src1 may be too large to represent as a signed int32, and src2 may be zero.
            // Since the result can therefore be too large to represent as a signed int32,
            // include Number in the value type.
            return CreateDstUntransferredValue(
                ValueType::AnyNumber.SetCanBeTaggedValue(true), instr, src1Val, src2Val);
        }

        this->PropagateIntRangeBinary(instr, min1, max1, min2, max2, &newMin, &newMax);
        return CreateDstUntransferredIntValue(newMin, newMax, instr, src1Val, src2Val);
    }

    case Js::OpCode::Incr_A:
    case Js::OpCode::Decr_A:
    {
        ValueType valueType;
        if(src1Val)
        {
            valueType = src1Val->GetValueInfo()->Type().ToDefiniteAnyNumber();
        }
        else
        {
            valueType = ValueType::Number;
        }
        return CreateDstUntransferredValue(valueType, instr, src1Val, src2Val);
    }

    case Js::OpCode::Add_A:
    {
        ValueType valueType;
        if (src1Val && src1ValueInfo->IsLikelyNumber() && src2Val && src2ValueInfo->IsLikelyNumber())
        {
            if(src1ValueInfo->IsLikelyInt() && src2ValueInfo->IsLikelyInt())
            {
                // When doing aggressiveIntType, just assume the result is likely going to be int
                // if both input is int.
                const bool isLikelyTagged = src1ValueInfo->IsLikelyTaggedInt() && src2ValueInfo->IsLikelyTaggedInt();
                if(src1ValueInfo->IsNumber() && src2ValueInfo->IsNumber())
                {
                    // If both of them are numbers then we can definitely say that the result is a number.
                    valueType = ValueType::GetNumberAndLikelyInt(isLikelyTagged);
                }
                else
                {
                    // This is only likely going to be int but can be a string as well.
                    valueType = ValueType::GetInt(isLikelyTagged).ToLikely();
                }
            }
            else
            {
                // We can only be certain of any thing if both of them are numbers.
                // Otherwise, the result could be string.
                if (src1ValueInfo->IsNumber() && src2ValueInfo->IsNumber())
                {
                    if (src1ValueInfo->IsFloat() || src2ValueInfo->IsFloat())
                    {
                        // If one of them is a float, the result probably is a float instead of just int
                        // but should always be a number.
                        valueType = ValueType::Float;
                    }
                    else
                    {
                        // Could be int, could be number
                        valueType = ValueType::Number;
                    }
                }
                else if (src1ValueInfo->IsLikelyFloat() || src2ValueInfo->IsLikelyFloat())
                {
                    // Result is likely a float (but can be anything)
                    valueType = ValueType::Float.ToLikely();
                }
                else
                {
                    // Otherwise it is a likely int or float (but can be anything)
                    valueType = ValueType::Number.ToLikely();
                }
            }
        }
        else if((src1Val && src1ValueInfo->IsString()) || (src2Val && src2ValueInfo->IsString()))
        {
            // String + anything should always result in a string
            valueType = ValueType::String;
        }
        else if((src1Val && src1ValueInfo->IsNotString() && src1ValueInfo->IsPrimitive())
            && (src2Val && src2ValueInfo->IsNotString() && src2ValueInfo->IsPrimitive()))
        {
            // If src1 and src2 are not strings and primitive, add should yield a number.
            valueType = ValueType::Number;
        }
        else if((src1Val && src1ValueInfo->IsLikelyString()) || (src2Val && src2ValueInfo->IsLikelyString()))
        {
            // likelystring + anything should always result in a likelystring
            valueType = ValueType::String.ToLikely();
        }
        else
        {
            // Number or string. Could make the value a merge of Number and String, but Uninitialized is more useful at the moment.
            Assert(valueType.IsUninitialized());
        }

        return CreateDstUntransferredValue(valueType, instr, src1Val, src2Val);
    }

    case Js::OpCode::Div_A:
        {
            ValueType divValueType = GetDivValueType(instr, src1Val, src2Val, false);
            if (divValueType.IsLikelyInt() || divValueType.IsFloat())
            {
                return CreateDstUntransferredValue(divValueType, instr, src1Val, src2Val);
            }
        }
        // fall-through

    case Js::OpCode::Sub_A:
    case Js::OpCode::Mul_A:
    case Js::OpCode::Rem_A:
    {
        ValueType valueType;
        if( src1Val &&
            src1ValueInfo->IsLikelyInt() &&
            src2Val &&
            src2ValueInfo->IsLikelyInt() &&
            instr->m_opcode != Js::OpCode::Div_A)
        {
            const bool isLikelyTagged =
                src1ValueInfo->IsLikelyTaggedInt() && (src2ValueInfo->IsLikelyTaggedInt() || instr->m_opcode == Js::OpCode::Rem_A);
            if(src1ValueInfo->IsNumber() && src2ValueInfo->IsNumber())
            {
                valueType = ValueType::GetNumberAndLikelyInt(isLikelyTagged);
            }
            else
            {
                valueType = ValueType::GetInt(isLikelyTagged).ToLikely();
            }
        }
        else if ((src1Val && src1ValueInfo->IsLikelyFloat()) || (src2Val && src2ValueInfo->IsLikelyFloat()))
        {
            // This should ideally be NewNumberAndLikelyFloatValue since we know the result is a number but not sure if it will
            // be a float value. However, that Number/LikelyFloat value type doesn't exist currently and all the necessary
            // checks are done for float values (tagged int checks, etc.) so it's sufficient to just create a float value here.
            valueType = ValueType::Float;
        }
        else
        {
            valueType = ValueType::Number;
        }

        return CreateDstUntransferredValue(valueType, instr, src1Val, src2Val);
    }

    case Js::OpCode::CallI:
        Assert(dst->IsRegOpnd());
        return NewGenericValue(dst->AsRegOpnd()->GetValueType(), dst);

    case Js::OpCode::LdElemI_A:
    {
        dstVal = ValueNumberLdElemDst(pInstr, src1Val);
        const ValueType baseValueType(instr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->GetValueType());
        if( (
                baseValueType.IsLikelyNativeArray() ||
            #ifdef _M_IX86
                (
                    !AutoSystemInfo::Data.SSE2Available() &&
                    baseValueType.IsLikelyObject() &&
                    (
                        baseValueType.GetObjectType() == ObjectType::Float32Array ||
                        baseValueType.GetObjectType() == ObjectType::Float64Array
                    )
                )
            #else
                false
            #endif
            ) &&
            instr->GetDst()->IsVar() &&
            instr->HasBailOutInfo())
        {
            // The lowerer is not going to generate a fast path for this case. Remove any bailouts that require the fast
            // path. Note that the removed bailouts should not be necessary for correctness.
            IR::BailOutKind bailOutKind = instr->GetBailOutKind();
            if(bailOutKind & IR::BailOutOnArrayAccessHelperCall)
            {
                bailOutKind -= IR::BailOutOnArrayAccessHelperCall;
            }
            if(bailOutKind == IR::BailOutOnImplicitCallsPreOp)
            {
                bailOutKind -= IR::BailOutOnImplicitCallsPreOp;
            }
            if(bailOutKind)
            {
                instr->SetBailOutKind(bailOutKind);
            }
            else
            {
                instr->ClearBailOutInfo();
            }
        }
        return dstVal;
    }

    case Js::OpCode::LdMethodElem:
        // Not worth profiling this, just assume it's likely object (should be likely function but ValueType does not track
        // functions currently, so using ObjectType::Object instead)
        dstVal = NewGenericValue(ValueType::GetObject(ObjectType::Object).ToLikely(), dst);
        if(instr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->GetValueType().IsLikelyNativeArray() && instr->HasBailOutInfo())
        {
            // The lowerer is not going to generate a fast path for this case. Remove any bailouts that require the fast
            // path. Note that the removed bailouts should not be necessary for correctness.
            IR::BailOutKind bailOutKind = instr->GetBailOutKind();
            if(bailOutKind & IR::BailOutOnArrayAccessHelperCall)
            {
                bailOutKind -= IR::BailOutOnArrayAccessHelperCall;
            }
            if(bailOutKind == IR::BailOutOnImplicitCallsPreOp)
            {
                bailOutKind -= IR::BailOutOnImplicitCallsPreOp;
            }
            if(bailOutKind)
            {
                instr->SetBailOutKind(bailOutKind);
            }
            else
            {
                instr->ClearBailOutInfo();
            }
        }
        return dstVal;

    case Js::OpCode::StElemI_A:
    case Js::OpCode::StElemI_A_Strict:
        dstVal = this->ValueNumberTransferDst(instr, src1Val);
        break;

    case Js::OpCode::LdLen_A:
        if (instr->IsProfiledInstr())
        {
            const ValueType profiledValueType(instr->AsProfiledInstr()->u.ldElemInfo->GetElementType());
            if(!(profiledValueType.IsLikelyInt() && dst->AsRegOpnd()->m_sym->m_isNotInt))
            {
                return this->NewGenericValue(profiledValueType, dst);
            }
        }
        break;

    case Js::OpCode::BrOnEmpty:
    case Js::OpCode::BrOnNotEmpty:
        Assert(dst->IsRegOpnd());
        Assert(dst->GetValueType().IsString());
        return this->NewGenericValue(ValueType::String, dst);

    case Js::OpCode::IsInst:
    case Js::OpCode::LdTrue:
    case Js::OpCode::LdFalse:
        return this->NewGenericValue(ValueType::Boolean, dst);

    case Js::OpCode::LdUndef:
        return this->NewGenericValue(ValueType::Undefined, dst);

    case Js::OpCode::LdC_A_Null:
        return this->NewGenericValue(ValueType::Null, dst);

    case Js::OpCode::LdThis:
        if (!PHASE_OFF(Js::OptTagChecksPhase, this->func) &&
            (src1ValueInfo == nullptr || src1ValueInfo->IsUninitialized()))
        {
            return this->NewGenericValue(ValueType::GetObject(ObjectType::Object), dst);
        }
        break;

    case Js::OpCode::Typeof:
        return this->NewGenericValue(ValueType::String, dst);
    case Js::OpCode::InitLocalClosure:
        Assert(instr->GetDst());
        Assert(instr->GetDst()->IsRegOpnd());
        IR::RegOpnd *regOpnd = instr->GetDst()->AsRegOpnd();
        StackSym *opndStackSym = regOpnd->m_sym;
        Assert(opndStackSym != nullptr);
        ObjectSymInfo *objectSymInfo = opndStackSym->m_objectInfo;
        Assert(objectSymInfo != nullptr);
        for (PropertySym *localVarSlotList = objectSymInfo->m_propertySymList; localVarSlotList; localVarSlotList = localVarSlotList->m_nextInStackSymList)
        {
            this->slotSyms->Set(localVarSlotList->m_id);
        }
        break;
    }

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    if (Js::IsSimd128Opcode(instr->m_opcode) && !func->GetJITFunctionBody()->IsAsmJsMode())
    {
        ThreadContext::SimdFuncSignature simdFuncSignature;
        instr->m_func->GetScriptContext()->GetThreadContext()->GetSimdFuncSignatureFromOpcode(instr->m_opcode, simdFuncSignature);
        return this->NewGenericValue(simdFuncSignature.returnType, dst);
    }
#endif

    if (dstVal == nullptr)
    {
        return this->NewGenericValue(dst->GetValueType(), dst);
    }

    return CurrentBlockData()->SetValue(dstVal, dst);
}

Value *
GlobOpt::ValueNumberLdElemDst(IR::Instr **pInstr, Value *srcVal)
{
    IR::Instr *&instr = *pInstr;
    IR::Opnd *dst = instr->GetDst();
    Value *dstVal = nullptr;
    int32 newMin, newMax;
    ValueInfo *srcValueInfo = (srcVal ? srcVal->GetValueInfo() : nullptr);

    ValueType profiledElementType;
    if (instr->IsProfiledInstr())
    {
        profiledElementType = instr->AsProfiledInstr()->u.ldElemInfo->GetElementType();
        if(!(profiledElementType.IsLikelyInt() && dst->IsRegOpnd() && dst->AsRegOpnd()->m_sym->m_isNotInt) &&
            srcVal &&
            srcValueInfo->IsUninitialized())
        {
            if(IsLoopPrePass())
            {
                dstVal = NewGenericValue(profiledElementType, dst);
            }
            else
            {
                // Assuming the profile data gives more precise value types based on the path it took at runtime, we
                // can improve the original value type.
                srcValueInfo->Type() = profiledElementType;
                instr->GetSrc1()->SetValueType(profiledElementType);
            }
        }
    }

    IR::IndirOpnd *src = instr->GetSrc1()->AsIndirOpnd();
    const ValueType baseValueType(src->GetBaseOpnd()->GetValueType());
    if (instr->DoStackArgsOpt(this->func) ||
        !(
            baseValueType.IsLikelyOptimizedTypedArray() ||
            (baseValueType.IsLikelyNativeArray() && instr->IsProfiledInstr()) // Specialized native array lowering for LdElem requires that it is profiled.
        ) ||
        (!this->DoTypedArrayTypeSpec() && baseValueType.IsLikelyOptimizedTypedArray()) ||

        // Don't do type spec on native array with a history of accessing gaps, as this is a bailout
        (!this->DoNativeArrayTypeSpec() && baseValueType.IsLikelyNativeArray()) ||
        !ShouldExpectConventionalArrayIndexValue(src))
    {
        if(DoTypedArrayTypeSpec() && !IsLoopPrePass())
        {
            GOPT_TRACE_INSTR(instr, _u("Didn't specialize array access.\n"));
            if (PHASE_TRACE(Js::TypedArrayTypeSpecPhase, this->func))
            {
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                char baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
                baseValueType.ToString(baseValueTypeStr);
                Output::Print(_u("Typed Array Optimization:  function: %s (%s): instr: %s, base value type: %S, did not type specialize, because %s.\n"),
                    this->func->GetJITFunctionBody()->GetDisplayName(),
                    this->func->GetDebugNumberSet(debugStringBuffer),
                    Js::OpCodeUtil::GetOpCodeName(instr->m_opcode),
                    baseValueTypeStr,
                    instr->DoStackArgsOpt(this->func) ? _u("instruction uses the arguments object") :
                    baseValueType.IsLikelyOptimizedTypedArray() ? _u("index is negative or likely not int") : _u("of array type"));
                Output::Flush();
            }
        }

        if(!dstVal)
        {
            if(srcVal)
            {
                dstVal = this->ValueNumberTransferDst(instr, srcVal);
            }
            else
            {
                dstVal = NewGenericValue(profiledElementType, dst);
            }
        }
        return dstVal;
    }

    Assert(instr->GetSrc1()->IsIndirOpnd());

    IRType toType = TyVar;
    IR::BailOutKind bailOutKind = IR::BailOutConventionalTypedArrayAccessOnly;

    switch(baseValueType.GetObjectType())
    {
    case ObjectType::Int8Array:
    case ObjectType::Int8VirtualArray:
    case ObjectType::Int8MixedArray:
        newMin = Int8ConstMin;
        newMax = Int8ConstMax;
        goto IntArrayCommon;

    case ObjectType::Uint8Array:
    case ObjectType::Uint8VirtualArray:
    case ObjectType::Uint8MixedArray:
    case ObjectType::Uint8ClampedArray:
    case ObjectType::Uint8ClampedVirtualArray:
    case ObjectType::Uint8ClampedMixedArray:
        newMin = Uint8ConstMin;
        newMax = Uint8ConstMax;
        goto IntArrayCommon;

    case ObjectType::Int16Array:
    case ObjectType::Int16VirtualArray:
    case ObjectType::Int16MixedArray:
        newMin = Int16ConstMin;
        newMax = Int16ConstMax;
        goto IntArrayCommon;

    case ObjectType::Uint16Array:
    case ObjectType::Uint16VirtualArray:
    case ObjectType::Uint16MixedArray:
        newMin = Uint16ConstMin;
        newMax = Uint16ConstMax;
        goto IntArrayCommon;

    case ObjectType::Int32Array:
    case ObjectType::Int32VirtualArray:
    case ObjectType::Int32MixedArray:
    case ObjectType::Uint32Array: // int-specialized loads from uint32 arrays will bail out on values that don't fit in an int32
    case ObjectType::Uint32VirtualArray:
    case ObjectType::Uint32MixedArray:
    Int32Array:
        newMin = Int32ConstMin;
        newMax = Int32ConstMax;
        goto IntArrayCommon;

    IntArrayCommon:
        Assert(dst->IsRegOpnd());

        // If int type spec is disabled, it is ok to load int values as they can help float type spec, and merging int32 with float64 => float64.
        // But if float type spec is also disabled, we'll have problems because float64 merged with var => float64...
        if (!this->DoAggressiveIntTypeSpec() && !this->DoFloatTypeSpec())
        {
            if (!dstVal)
            {
                if (srcVal)
                {
                    dstVal = this->ValueNumberTransferDst(instr, srcVal);
                }
                else
                {
                    dstVal = NewGenericValue(profiledElementType, dst);
                }
            }
            return dstVal;
        }

        if (!this->IsLoopPrePass())
        {
            if (instr->HasBailOutInfo())
            {
                const IR::BailOutKind oldBailOutKind = instr->GetBailOutKind();
                Assert(
                    (
                        !(oldBailOutKind & ~IR::BailOutKindBits) ||
                        (oldBailOutKind & ~IR::BailOutKindBits) == IR::BailOutOnImplicitCallsPreOp
                        ) &&
                    !(oldBailOutKind & IR::BailOutKindBits & ~(IR::BailOutOnArrayAccessHelperCall | IR::BailOutMarkTempObject)));
                if (bailOutKind == IR::BailOutConventionalTypedArrayAccessOnly)
                {
                    // BailOutConventionalTypedArrayAccessOnly also bails out if the array access is outside the head
                    // segment bounds, and guarantees no implicit calls. Override the bailout kind so that the instruction
                    // bails out for the right reason.
                    instr->SetBailOutKind(
                        bailOutKind | (oldBailOutKind & (IR::BailOutKindBits - IR::BailOutOnArrayAccessHelperCall)));
                }
                else
                {
                    // BailOutConventionalNativeArrayAccessOnly by itself may generate a helper call, and may cause implicit
                    // calls to occur, so it must be merged in to eliminate generating the helper call
                    Assert(bailOutKind == IR::BailOutConventionalNativeArrayAccessOnly);
                    instr->SetBailOutKind(oldBailOutKind | bailOutKind);
                }
            }
            else
            {
                GenerateBailAtOperation(&instr, bailOutKind);
            }
        }
        TypeSpecializeIntDst(instr, instr->m_opcode, nullptr, nullptr, nullptr, bailOutKind, newMin, newMax, &dstVal);
        toType = TyInt32;
        break;

    case ObjectType::Float32Array:
    case ObjectType::Float32VirtualArray:
    case ObjectType::Float32MixedArray:
    case ObjectType::Float64Array:
    case ObjectType::Float64VirtualArray:
    case ObjectType::Float64MixedArray:
    Float64Array:
        Assert(dst->IsRegOpnd());
        
        // If float type spec is disabled, don't load float64 values
        if (!this->DoFloatTypeSpec())
        {
            if (!dstVal)
            {
                if (srcVal)
                {
                    dstVal = this->ValueNumberTransferDst(instr, srcVal);
                }
                else
                {
                    dstVal = NewGenericValue(profiledElementType, dst);
                }
            }
            return dstVal;
        }

        if (!this->IsLoopPrePass())
        {
            if (instr->HasBailOutInfo())
            {
                const IR::BailOutKind oldBailOutKind = instr->GetBailOutKind();
                Assert(
                    (
                        !(oldBailOutKind & ~IR::BailOutKindBits) ||
                        (oldBailOutKind & ~IR::BailOutKindBits) == IR::BailOutOnImplicitCallsPreOp
                        ) &&
                    !(oldBailOutKind & IR::BailOutKindBits & ~(IR::BailOutOnArrayAccessHelperCall | IR::BailOutMarkTempObject)));
                if (bailOutKind == IR::BailOutConventionalTypedArrayAccessOnly)
                {
                    // BailOutConventionalTypedArrayAccessOnly also bails out if the array access is outside the head
                    // segment bounds, and guarantees no implicit calls. Override the bailout kind so that the instruction
                    // bails out for the right reason.
                    instr->SetBailOutKind(
                        bailOutKind | (oldBailOutKind & (IR::BailOutKindBits - IR::BailOutOnArrayAccessHelperCall)));
                }
                else
                {
                    // BailOutConventionalNativeArrayAccessOnly by itself may generate a helper call, and may cause implicit
                    // calls to occur, so it must be merged in to eliminate generating the helper call
                    Assert(bailOutKind == IR::BailOutConventionalNativeArrayAccessOnly);
                    instr->SetBailOutKind(oldBailOutKind | bailOutKind);
                }
            }
            else
            {
                GenerateBailAtOperation(&instr, bailOutKind);
            }
        }
        TypeSpecializeFloatDst(instr, nullptr, nullptr, nullptr, &dstVal);
        toType = TyFloat64;
        break;

    default:
        Assert(baseValueType.IsLikelyNativeArray());
        bailOutKind = IR::BailOutConventionalNativeArrayAccessOnly;
        if(baseValueType.HasIntElements())
        {
            goto Int32Array;
        }
        Assert(baseValueType.HasFloatElements());
        goto Float64Array;
    }

    if(!dstVal)
    {
        dstVal = NewGenericValue(profiledElementType, dst);
    }

    Assert(toType != TyVar);

    GOPT_TRACE_INSTR(instr, _u("Type specialized array access.\n"));
    if (PHASE_TRACE(Js::TypedArrayTypeSpecPhase, this->func))
    {
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        char baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
        baseValueType.ToString(baseValueTypeStr);
        char dstValTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
        dstVal->GetValueInfo()->Type().ToString(dstValTypeStr);
        Output::Print(_u("Typed Array Optimization:  function: %s (%s): instr: %s, base value type: %S, type specialized to %s producing %S"),
            this->func->GetJITFunctionBody()->GetDisplayName(),
            this->func->GetDebugNumberSet(debugStringBuffer),
            Js::OpCodeUtil::GetOpCodeName(instr->m_opcode),
            baseValueTypeStr,
            toType == TyInt32 ? _u("int32") : _u("float64"),
            dstValTypeStr);
#if DBG_DUMP
        Output::Print(_u(" ("));
        dstVal->Dump();
        Output::Print(_u(").\n"));
#else
        Output::Print(_u(".\n"));
#endif
        Output::Flush();
    }

    return dstVal;
}

ValueType
GlobOpt::GetPrepassValueTypeForDst(
    const ValueType desiredValueType,
    IR::Instr *const instr,
    Value *const src1Value,
    Value *const src2Value,
    bool *const isValueInfoPreciseRef) const
{
    // Values with definite types can be created in the loop prepass only when it is guaranteed that the value type will be the
    // same on any iteration of the loop. The heuristics currently used are:
    //     - If the source sym is not live on the back-edge, then it acquires a new value for each iteration of the loop, so
    //       that value type can be definite
    //         - Consider: A better solution for this is to track values that originate in this loop, which can have definite value
    //           types. That catches more cases, should look into that in the future.
    //     - If the source sym has a constant value that doesn't change for the duration of the function
    //     - The operation always results in a definite value type. For instance, signed bitwise operations always result in an
    //       int32, conv_num and ++ always result in a number, etc.
    //         - For operations that always result in an int32, the resulting int range is precise only if the source syms pass
    //           the above heuristics. Otherwise, the range must be expanded to the full int32 range.

    Assert(IsLoopPrePass());
    Assert(instr);

    if(isValueInfoPreciseRef)
    {
        *isValueInfoPreciseRef = false;
    }

    if(!desiredValueType.IsDefinite())
    {
        return desiredValueType;
    }

    if((instr->GetSrc1() && !IsPrepassSrcValueInfoPrecise(instr->GetSrc1(), src1Value)) ||
       (instr->GetSrc2() && !IsPrepassSrcValueInfoPrecise(instr->GetSrc2(), src2Value)))
    {
        // If the desired value type is not precise, the value type of the destination is derived from the value types of the
        // sources. Since the value type of a source sym is not definite, the destination value type also cannot be definite.
        if(desiredValueType.IsInt() && OpCodeAttr::IsInt32(instr->m_opcode))
        {
            // The op always produces an int32, but not always a tagged int
            return ValueType::GetInt(desiredValueType.IsLikelyTaggedInt());
        }
        if(desiredValueType.IsNumber() && OpCodeAttr::ProducesNumber(instr->m_opcode))
        {
            // The op always produces a number, but not always an int
            return desiredValueType.ToDefiniteAnyNumber();
        }
        return desiredValueType.ToLikely();
    }

    if(isValueInfoPreciseRef)
    {
        // The produced value info is derived from the sources, which have precise value infos
        *isValueInfoPreciseRef = true;
    }
    return desiredValueType;
}

bool
GlobOpt::IsPrepassSrcValueInfoPrecise(IR::Opnd *const src, Value *const srcValue) const
{
    Assert(IsLoopPrePass());
    Assert(src);

    if(!src->IsRegOpnd() || !srcValue)
    {
        return false;
    }

    ValueInfo *const srcValueInfo = srcValue->GetValueInfo();
    if(!srcValueInfo->IsDefinite())
    {
        return false;
    }

    StackSym *srcSym = src->AsRegOpnd()->m_sym;
    Assert(!srcSym->IsTypeSpec());
    int32 intConstantValue;
    return
        srcSym->IsFromByteCodeConstantTable() ||
        (
            srcValueInfo->TryGetIntConstantValue(&intConstantValue) &&
            !Js::TaggedInt::IsOverflow(intConstantValue) &&
            GetTaggedIntConstantStackSym(intConstantValue) == srcSym
        ) ||
        !currentBlock->loop->regAlloc.liveOnBackEdgeSyms->Test(srcSym->m_id);
}

Value *GlobOpt::CreateDstUntransferredIntValue(
    const int32 min,
    const int32 max,
    IR::Instr *const instr,
    Value *const src1Value,
    Value *const src2Value)
{
    Assert(instr);
    Assert(instr->GetDst());

    Assert(OpCodeAttr::ProducesNumber(instr->m_opcode)
        || (instr->m_opcode == Js::OpCode::Add_A && src1Value->GetValueInfo()->IsNumber()
        && src2Value->GetValueInfo()->IsNumber()));

    ValueType valueType(ValueType::GetInt(IntConstantBounds(min, max).IsLikelyTaggable()));
    Assert(valueType.IsInt());
    bool isValueInfoPrecise;
    if(IsLoopPrePass())
    {
        valueType = GetPrepassValueTypeForDst(valueType, instr, src1Value, src2Value, &isValueInfoPrecise);
    }
    else
    {
        isValueInfoPrecise = true;
    }

    IR::Opnd *const dst = instr->GetDst();
    if(isValueInfoPrecise)
    {
        Assert(valueType == ValueType::GetInt(IntConstantBounds(min, max).IsLikelyTaggable()));
        Assert(!(dst->IsRegOpnd() && dst->AsRegOpnd()->m_sym->IsTypeSpec()));
        return NewIntRangeValue(min, max, false, dst);
    }
    return NewGenericValue(valueType, dst);
}

Value *
GlobOpt::CreateDstUntransferredValue(
    const ValueType desiredValueType,
    IR::Instr *const instr,
    Value *const src1Value,
    Value *const src2Value)
{
    Assert(instr);
    Assert(instr->GetDst());
    Assert(!desiredValueType.IsInt()); // use CreateDstUntransferredIntValue instead

    ValueType valueType(desiredValueType);
    if(IsLoopPrePass())
    {
        valueType = GetPrepassValueTypeForDst(valueType, instr, src1Value, src2Value);
    }
    return NewGenericValue(valueType, instr->GetDst());
}

Value *
GlobOpt::ValueNumberTransferDst(IR::Instr *const instr, Value * src1Val)
{
    Value *dstVal = this->IsLoopPrePass() ? this->ValueNumberTransferDstInPrepass(instr, src1Val) : src1Val;

    // Don't copy-prop a temp over a user symbol.  This is likely to extend the temp's lifetime, as the user symbol
    // is more likely to already have later references.
    // REVIEW: Enabling this does cause perf issues...
#if 0
    if (dstVal != src1Val)
    {
        return dstVal;
    }

    Sym *dstSym = dst->GetStackSym();

    if (dstVal && dstSym && dstSym->IsStackSym() && !dstSym->AsStackSym()->m_isBytecodeTmp)
    {
        Sym *dstValSym = dstVal->GetValueInfo()->GetSymStore();
        if (dstValSym && dstValSym->AsStackSym()->m_isBytecodeTmp /* src->GetIsDead()*/)
        {
            dstVal->GetValueInfo()->SetSymStore(dstSym);
        }
    }
#endif

    return dstVal;
}

bool
GlobOpt::IsSafeToTransferInPrePass(IR::Opnd *src, Value *srcValue)
{
    if (this->DoFieldHoisting())
    {
        return false;
    }

    if (src->IsRegOpnd())
    {
        StackSym *srcSym = src->AsRegOpnd()->m_sym;
        if (srcSym->IsFromByteCodeConstantTable())
        {
            return true;
        }

        ValueInfo *srcValueInfo = srcValue->GetValueInfo();

        int32 srcIntConstantValue;
        if (srcValueInfo->TryGetIntConstantValue(&srcIntConstantValue) && !Js::TaggedInt::IsOverflow(srcIntConstantValue)
            && GetTaggedIntConstantStackSym(srcIntConstantValue) == srcSym)
        {
            return true;
        }
    }

    return false;
}

Value *
GlobOpt::ValueNumberTransferDstInPrepass(IR::Instr *const instr, Value *const src1Val)
{
    Value *dstVal = nullptr;

    if (!src1Val)
    {
        return nullptr;
    }

    bool isValueInfoPrecise;
    ValueInfo *const src1ValueInfo = src1Val->GetValueInfo();

    // TODO: This conflicts with new values created by the type specialization code
    //       We should re-enable if we change that code to avoid the new values.
#if 0
    if (this->IsSafeToTransferInPrePass(instr->GetSrc1(), src1Val))
    {
        return src1Val;
    }


    if (this->IsPREInstrCandidateLoad(instr->m_opcode) && instr->GetDst())
    {
        StackSym *dstSym = instr->GetDst()->AsRegOpnd()->m_sym;

        for (Loop *curLoop = this->currentBlock->loop; curLoop; curLoop = curLoop->parent)
        {
            if (curLoop->fieldPRESymStore->Test(dstSym->m_id))
            {
                return src1Val;
            }
        }
    }

    if (!this->DoFieldHoisting())
    {
        if (instr->GetDst()->IsRegOpnd())
        {
            StackSym *stackSym = instr->GetDst()->AsRegOpnd()->m_sym;

            if (stackSym->IsSingleDef() || this->IsLive(stackSym, this->prePassLoop->landingPad))
            {
                IntConstantBounds src1IntConstantBounds;
                if (src1ValueInfo->TryGetIntConstantBounds(&src1IntConstantBounds) &&
                    !(
                        src1IntConstantBounds.LowerBound() == INT32_MIN &&
                        src1IntConstantBounds.UpperBound() == INT32_MAX
                        ))
                {
                    const ValueType valueType(
                        GetPrepassValueTypeForDst(src1ValueInfo->Type(), instr, src1Val, nullptr, &isValueInfoPrecise));
                    if (isValueInfoPrecise)
                    {
                        return src1Val;
                    }
                }
                else
                {
                    return src1Val;
                }
            }
        }
    }
#endif

    // Src1's value could change later in the loop, so the value wouldn't be the same for each
    // iteration.  Since we don't iterate over loops "while (!changed)", go conservative on the
    // first pass when transferring a value that is live on the back-edge.

    // In prepass we are going to copy the value but with a different value number
    // for aggressive int type spec.
    const ValueType valueType(GetPrepassValueTypeForDst(src1ValueInfo->Type(), instr, src1Val, nullptr, &isValueInfoPrecise));
    if(isValueInfoPrecise || (valueType == src1ValueInfo->Type() && src1ValueInfo->IsGeneric()))
    {
        Assert(valueType == src1ValueInfo->Type());
        dstVal = CopyValue(src1Val);
        TrackCopiedValueForKills(dstVal);
    }
    else
    {
        dstVal = NewGenericValue(valueType);
        dstVal->GetValueInfo()->SetSymStore(src1ValueInfo->GetSymStore());
    }

    return dstVal;
}

void
GlobOpt::PropagateIntRangeForNot(int32 minimum, int32 maximum, int32 *pNewMin, int32* pNewMax)
{
    int32 tmp;
    Int32Math::Not(minimum, pNewMin);
    *pNewMax = *pNewMin;
    Int32Math::Not(maximum, &tmp);
    *pNewMin = min(*pNewMin, tmp);
    *pNewMax = max(*pNewMax, tmp);
}

void
GlobOpt::PropagateIntRangeBinary(IR::Instr *instr, int32 min1, int32 max1,
    int32 min2, int32 max2, int32 *pNewMin, int32* pNewMax)
{
    int32 min, max, tmp, tmp2;

    min = INT32_MIN;
    max = INT32_MAX;

    switch (instr->m_opcode)
    {
    case Js::OpCode::Xor_A:
    case Js::OpCode::Or_A:
        // Find range with highest high order bit
        tmp = ::max((uint32)min1, (uint32)max1);
        tmp2 = ::max((uint32)min2, (uint32)max2);

        if ((uint32)tmp > (uint32)tmp2)
        {
            max = tmp;
        }
        else
        {
            max = tmp2;
        }

        if (max < 0)
        {
            min = INT32_MIN;  // REVIEW: conservative...
            max = INT32_MAX;
        }
        else
        {
            // Turn values like 0x1010 into 0x1111
            max = 1 << Math::Log2(max);
            max = (uint32)(max << 1) - 1;
            min = 0;
        }

        break;

    case Js::OpCode::And_A:

        if (min1 == INT32_MIN && min2 == INT32_MIN)
        {
            // Shortcut
            break;
        }

        // Find range with lowest higher bit
        tmp = ::max((uint32)min1, (uint32)max1);
        tmp2 = ::max((uint32)min2, (uint32)max2);

        if ((uint32)tmp < (uint32)tmp2)
        {
            min = min1;
            max = max1;
        }
        else
        {
            min = min2;
            max = max2;
        }

        // To compute max, look if min has higher high bit
        if ((uint32)min > (uint32)max)
        {
            max = min;
        }

        // If max is negative, max let's assume it could be -1, so result in MAX_INT
        if (max < 0)
        {
            max = INT32_MAX;
        }

        // If min is positive, the resulting min is zero
        if (min >= 0)
        {
            min = 0;
        }
        else
        {
            min = INT32_MIN;
        }
        break;

    case Js::OpCode::Shl_A:
        {
            // Shift count
            if (min2 != max2 && ((uint32)min2 > 0x1F || (uint32)max2 > 0x1F))
            {
                min2 = 0;
                max2 = 0x1F;
            }
            else
            {
                min2 &= 0x1F;
                max2 &= 0x1F;
            }

            int32 min1FreeTopBitCount = min1 ? (sizeof(int32) * 8) - (Math::Log2(min1) + 1) : (sizeof(int32) * 8);
            int32 max1FreeTopBitCount = max1 ? (sizeof(int32) * 8) - (Math::Log2(max1) + 1) : (sizeof(int32) * 8);
            if (min1FreeTopBitCount <= max2 || max1FreeTopBitCount <= max2)
            {
                // If the shift is going to touch the sign bit return the max range
                min = INT32_MIN;
                max = INT32_MAX;
            }
            else
            {
                // Compute max
                // Turn values like 0x1010 into 0x1111
                if (min1)
                {
                    min1 = 1 << Math::Log2(min1);
                    min1 = (min1 << 1) - 1;
                }
                if (max1)
                {
                    max1 = 1 << Math::Log2(max1);
                    max1 = (uint32)(max1 << 1) - 1;
                }

                if (max1 > 0)
                {
                    int32 nrTopBits = (sizeof(int32) * 8) - Math::Log2(max1);
                    if (nrTopBits < ::min(max2, 30))
                        max = INT32_MAX;
                    else
                        max = ::max((max1 << ::min(max2, 30)) & ~0x80000000, (min1 << min2) & ~0x80000000);
                }
                else
                {
                    max = (max1 << min2) & ~0x80000000;
                }
                // Compute min

                if (min1 < 0)
                {
                    min = ::min(min1 << max2, max1 << max2);
                }
                else
                {
                    min = ::min(min1 << min2, max1 << max2);
                }
                // Turn values like 0x1110 into 0x1000
                if (min)
                {
                    min = 1 << Math::Log2(min);
                }
            }
        }
        break;

    case Js::OpCode::Shr_A:
        // Shift count
        if (min2 != max2 && ((uint32)min2 > 0x1F || (uint32)max2 > 0x1F))
        {
            min2 = 0;
            max2 = 0x1F;
        }
        else
        {
            min2 &= 0x1F;
            max2 &= 0x1F;
        }

        // Compute max

        if (max1 < 0)
        {
            max = max1 >> max2;
        }
        else
        {
            max = max1 >> min2;
        }

        // Compute min

        if (min1 < 0)
        {
            min = min1 >> min2;
        }
        else
        {
            min = min1 >> max2;
        }
        break;

    case Js::OpCode::ShrU_A:

        // shift count is constant zero
        if ((min2 == max2) && (max2 & 0x1f) == 0)
        {
            // We can't encode uint32 result, so it has to be used as int32 only or the original value is positive.
            Assert(instr->ignoreIntOverflow || min1 >= 0);
            // We can transfer the signed int32 range.
            min = min1;
            max = max1;
            break;
        }

        const IntConstantBounds src2NewBounds = IntConstantBounds(min2, max2).And_0x1f();
        // Zero is only allowed if result is always a signed int32 or always used as a signed int32
        Assert(min1 >= 0 || instr->ignoreIntOverflow || !src2NewBounds.Contains(0));
        min2 = src2NewBounds.LowerBound();
        max2 = src2NewBounds.UpperBound();

        Assert(min2 <= max2);
        // zero shift count is only allowed if result is used as int32 and/or value is positive
        Assert(min2 > 0 || instr->ignoreIntOverflow || min1 >= 0);

        uint32 umin1 = (uint32)min1;
        uint32 umax1 = (uint32)max1;

        if (umin1 > umax1)
        {
            uint32 temp = umax1;
            umax1 = umin1;
            umin1 = temp;
        }

        Assert(min2 >= 0 && max2 < 32);

        // Compute max

        if (min1 < 0)
        {
            umax1 = UINT32_MAX;
        }
        max = umax1 >> min2;

        // Compute min

        if (min1 <= 0 && max1 >=0)
        {
            min = 0;
        }
        else
        {
            min = umin1 >> max2;
        }

        // We should be able to fit uint32 range as int32
        Assert(instr->ignoreIntOverflow || (min >= 0 && max >= 0) );
        if (min > max)
        {
            // can only happen if shift count can be zero
            Assert(min2 == 0 && (instr->ignoreIntOverflow || min1 >= 0));
            min = Int32ConstMin;
            max = Int32ConstMax;
        }

        break;
    }

    *pNewMin = min;
    *pNewMax = max;
}

IR::Instr *
GlobOpt::TypeSpecialization(
    IR::Instr *instr,
    Value **pSrc1Val,
    Value **pSrc2Val,
    Value **pDstVal,
    bool *redoTypeSpecRef,
    bool *const forceInvariantHoistingRef)
{
    Value *&src1Val = *pSrc1Val;
    Value *&src2Val = *pSrc2Val;
    *redoTypeSpecRef = false;
    Assert(!*forceInvariantHoistingRef);

    this->ignoredIntOverflowForCurrentInstr = false;
    this->ignoredNegativeZeroForCurrentInstr = false;

    // - Int32 values that can't be tagged are created as float constant values instead because a JavascriptNumber var is needed
    //   for that value at runtime. For the purposes of type specialization, recover the int32 values so that they will be
    //   treated as ints.
    // - If int overflow does not matter for the instruction, we can additionally treat uint32 values as int32 values because
    //   the value resulting from the operation will eventually be converted to int32 anyway
    Value *const src1OriginalVal = src1Val;
    Value *const src2OriginalVal = src2Val;

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    if (TypeSpecializeSimd128(instr, pSrc1Val, pSrc2Val, pDstVal))
    {
        return instr;
    }
#endif

    if(!instr->ShouldCheckForIntOverflow())
    {
        if(src1Val && src1Val->GetValueInfo()->IsFloatConstant())
        {
            int32 int32Value;
            bool isInt32;
            if(Js::JavascriptNumber::TryGetInt32OrUInt32Value(
                    src1Val->GetValueInfo()->AsFloatConstant()->FloatValue(),
                    &int32Value,
                    &isInt32))
            {
                src1Val = GetIntConstantValue(int32Value, instr);
                if(!isInt32)
                {
                    this->ignoredIntOverflowForCurrentInstr = true;
                }
            }
        }
        if(src2Val && src2Val->GetValueInfo()->IsFloatConstant())
        {
            int32 int32Value;
            bool isInt32;
            if(Js::JavascriptNumber::TryGetInt32OrUInt32Value(
                    src2Val->GetValueInfo()->AsFloatConstant()->FloatValue(),
                    &int32Value,
                    &isInt32))
            {
                src2Val = GetIntConstantValue(int32Value, instr);
                if(!isInt32)
                {
                    this->ignoredIntOverflowForCurrentInstr = true;
                }
            }
        }
    }
    const AutoRestoreVal autoRestoreSrc1Val(src1OriginalVal, &src1Val);
    const AutoRestoreVal autoRestoreSrc2Val(src2OriginalVal, &src2Val);

    if (src1Val && instr->GetSrc2() == nullptr)
    {
        // Unary
        // Note make sure that native array StElemI gets to TypeSpecializeStElem. Do this for typed arrays, too?
        int32 intConstantValue;
        if (!this->IsLoopPrePass() &&
            !instr->IsBranchInstr() &&
            src1Val->GetValueInfo()->TryGetIntConstantValue(&intConstantValue) &&
            !(
                // Nothing to fold for element stores. Go into type specialization to see if they can at least be specialized.
                instr->m_opcode == Js::OpCode::StElemI_A ||
                instr->m_opcode == Js::OpCode::StElemI_A_Strict ||
                instr->m_opcode == Js::OpCode::StElemC ||
                instr->m_opcode == Js::OpCode::MultiBr ||
                instr->m_opcode == Js::OpCode::InlineArrayPop
            ))
        {
            if (OptConstFoldUnary(&instr, intConstantValue, src1Val == src1OriginalVal, pDstVal))
            {
                return instr;
            }
        }
        else if (this->TypeSpecializeUnary(
                    &instr,
                    &src1Val,
                    pDstVal,
                    src1OriginalVal,
                    redoTypeSpecRef,
                    forceInvariantHoistingRef))
        {
            return instr;
        }
        else if(*redoTypeSpecRef)
        {
            return instr;
        }
    }
    else if (instr->GetSrc2() && !instr->IsBranchInstr())
    {
        // Binary
        if (!this->IsLoopPrePass())
        {
            if (GetIsAsmJSFunc())
            {
                if (CONFIG_FLAG(WasmFold))
                {
                    bool success = instr->GetSrc1()->IsInt64() ?
                        this->OptConstFoldBinaryWasm<int64>(&instr, src1Val, src2Val, pDstVal) :
                        this->OptConstFoldBinaryWasm<int>(&instr, src1Val, src2Val, pDstVal);

                    if (success)
                    {
                        return instr;
                    }
                }
            }
            else
            {
                // OptConstFoldBinary doesn't do type spec, so only deal with things we are sure are int (IntConstant and IntRange)
                // and not just likely ints  TypeSpecializeBinary will deal with type specializing them and fold them again
                IntConstantBounds src1IntConstantBounds, src2IntConstantBounds;
                if (src1Val && src1Val->GetValueInfo()->TryGetIntConstantBounds(&src1IntConstantBounds))
                {
                    if (src2Val && src2Val->GetValueInfo()->TryGetIntConstantBounds(&src2IntConstantBounds))
                    {
                        if (this->OptConstFoldBinary(&instr, src1IntConstantBounds, src2IntConstantBounds, pDstVal))
                        {
                            return instr;
                        }
                    }
                }
            }
        }
    }
    if (instr->GetSrc2() && this->TypeSpecializeBinary(&instr, pSrc1Val, pSrc2Val, pDstVal, src1OriginalVal, src2OriginalVal, redoTypeSpecRef))
    {
        if (!this->IsLoopPrePass() &&
            instr->m_opcode != Js::OpCode::Nop &&
            instr->m_opcode != Js::OpCode::Br &&    // We may have const fold a branch

            // Cannot const-peep if the result of the operation is required for a bailout check
            !(instr->HasBailOutInfo() && instr->GetBailOutKind() & IR::BailOutOnResultConditions))
        {
            if (src1Val && src1Val->GetValueInfo()->HasIntConstantValue())
            {
                if (this->OptConstPeep(instr, instr->GetSrc1(), pDstVal, src1Val->GetValueInfo()))
                {
                    return instr;
                }
            }
            else if (src2Val && src2Val->GetValueInfo()->HasIntConstantValue())
            {
                if (this->OptConstPeep(instr, instr->GetSrc2(), pDstVal, src2Val->GetValueInfo()))
                {
                    return instr;
                }
            }
        }
        return instr;
    }
    else if(*redoTypeSpecRef)
    {
        return instr;
    }

    if (instr->IsBranchInstr() && !this->IsLoopPrePass())
    {
        if (this->OptConstFoldBranch(instr, src1Val, src2Val, pDstVal))
        {
            return instr;
        }
    }
    // We didn't type specialize, make sure the srcs are unspecialized
    IR::Opnd *src1 = instr->GetSrc1();
    if (src1)
    {
        instr = this->ToVarUses(instr, src1, false, src1Val);

        IR::Opnd *src2 = instr->GetSrc2();
        if (src2)
        {
            instr = this->ToVarUses(instr, src2, false, src2Val);
        }
    }

    IR::Opnd *dst = instr->GetDst();
    if (dst)
    {
        instr = this->ToVarUses(instr, dst, true, nullptr);

        // Handling for instructions other than built-ins that may require only dst type specialization
        // should be added here.
        if(OpCodeAttr::IsInlineBuiltIn(instr->m_opcode) && !GetIsAsmJSFunc()) // don't need to do typespec for asmjs
        {
            this->TypeSpecializeInlineBuiltInDst(&instr, pDstVal);
            return instr;
        }

        // Clear the int specialized bit on the dst.
        if (dst->IsRegOpnd())
        {
            IR::RegOpnd *dstRegOpnd = dst->AsRegOpnd();
            if (!dstRegOpnd->m_sym->IsTypeSpec())
            {
                this->ToVarRegOpnd(dstRegOpnd, this->currentBlock);
            }
            else if (dstRegOpnd->m_sym->IsInt32())
            {
                this->ToInt32Dst(instr, dstRegOpnd, this->currentBlock);
            }
            else if (dstRegOpnd->m_sym->IsUInt32() && GetIsAsmJSFunc())
            {
                this->ToUInt32Dst(instr, dstRegOpnd, this->currentBlock);
            }
            else if (dstRegOpnd->m_sym->IsFloat64())
            {
                this->ToFloat64Dst(instr, dstRegOpnd, this->currentBlock);
            }
        }
        else if (dst->IsSymOpnd() && dst->AsSymOpnd()->m_sym->IsStackSym())
        {
            this->ToVarStackSym(dst->AsSymOpnd()->m_sym->AsStackSym(), this->currentBlock);
        }
    }

    return instr;
}

bool
GlobOpt::OptConstPeep(IR::Instr *instr, IR::Opnd *constSrc, Value **pDstVal, ValueInfo *valuInfo)
{
    int32 value;
    IR::Opnd *src;
    IR::Opnd *nonConstSrc = (constSrc == instr->GetSrc1() ? instr->GetSrc2() : instr->GetSrc1());

    // Try to find the value from value info first
    if (valuInfo->TryGetIntConstantValue(&value))
    {
    }
    else if (constSrc->IsAddrOpnd())
    {
        IR::AddrOpnd *addrOpnd = constSrc->AsAddrOpnd();
#ifdef _M_X64
        Assert(addrOpnd->IsVar() || Math::FitsInDWord((size_t)addrOpnd->m_address));
#else
        Assert(sizeof(value) == sizeof(addrOpnd->m_address));
#endif

        if (addrOpnd->IsVar())
        {
            value = Js::TaggedInt::ToInt32(addrOpnd->m_address);
        }
        else
        {
            // We asserted that the address will fit in a DWORD above
            value = ::Math::PointerCastToIntegral<int32>(constSrc->AsAddrOpnd()->m_address);
        }
    }
    else if (constSrc->IsIntConstOpnd())
    {
        value = constSrc->AsIntConstOpnd()->AsInt32();
    }
    else
    {
        return false;
    }

    switch(instr->m_opcode)
    {
        // Can't do all Add_A because of string concats.
        // Sub_A cannot be transformed to a NEG_A because 0 - 0 != -0
    case Js::OpCode::Add_A:
        src = nonConstSrc;

        if (!src->GetValueType().IsInt())
        {
            // 0 + -0  != -0
            // "Foo" + 0 != "Foo
            return false;
        }
        // fall-through

    case Js::OpCode::Add_I4:
        if (value != 0)
        {
            return false;
        }
        if (constSrc == instr->GetSrc1())
        {
            src = instr->GetSrc2();
        }
        else
        {
            src = instr->GetSrc1();
        }
        break;

    case Js::OpCode::Mul_A:
    case Js::OpCode::Mul_I4:
        if (value == 0)
        {
            // -0 * 0 != 0
            return false;
        }
        else if (value == 1)
        {
            src = nonConstSrc;
        }
        else
        {
            return false;
        }
        break;

    case Js::OpCode::Div_A:
        if (value == 1 && constSrc == instr->GetSrc2())
        {
            src = instr->GetSrc1();
        }
        else
        {
            return false;
        }
        break;

    case Js::OpCode::Or_I4:
        if (value == -1)
        {
            src = constSrc;
        }
        else if (value == 0)
        {
            src = nonConstSrc;
        }
        else
        {
            return false;
        }
        break;

    case Js::OpCode::And_I4:
        if (value == -1)
        {
            src = nonConstSrc;
        }
        else if (value == 0)
        {
            src = constSrc;
        }
        else
        {
            return false;
        }
        break;

    case Js::OpCode::Shl_I4:
    case Js::OpCode::ShrU_I4:
    case Js::OpCode::Shr_I4:
        if (value != 0 || constSrc != instr->GetSrc2())
        {
            return false;
        }
        src = instr->GetSrc1();
        break;

    default:
        return false;
    }

    this->CaptureByteCodeSymUses(instr);

    if (src == instr->GetSrc1())
    {
        instr->FreeSrc2();
    }
    else
    {
        Assert(src == instr->GetSrc2());
        instr->ReplaceSrc1(instr->UnlinkSrc2());
    }

    instr->m_opcode = Js::OpCode::Ld_A;

    return true;
}

Js::Var // TODO: michhol OOP JIT, shouldn't play with Vars
GlobOpt::GetConstantVar(IR::Opnd *opnd, Value *val)
{
    ValueInfo *valueInfo = val->GetValueInfo();

    if (valueInfo->IsVarConstant() && valueInfo->IsPrimitive())
    {
        return valueInfo->AsVarConstant()->VarValue();
    }
    if (opnd->IsAddrOpnd())
    {
        IR::AddrOpnd *addrOpnd = opnd->AsAddrOpnd();
        if (addrOpnd->IsVar())
        {
            return addrOpnd->m_address;
        }
    }
    else if (opnd->IsIntConstOpnd())
    {
        if (!Js::TaggedInt::IsOverflow(opnd->AsIntConstOpnd()->AsInt32()))
        {
            return Js::TaggedInt::ToVarUnchecked(opnd->AsIntConstOpnd()->AsInt32());
        }
    }
    else if (opnd->IsRegOpnd() && opnd->AsRegOpnd()->m_sym->IsSingleDef())
    {
        if (valueInfo->IsBoolean())
        {
            IR::Instr * defInstr = opnd->AsRegOpnd()->m_sym->GetInstrDef();

            if (defInstr->m_opcode != Js::OpCode::Ld_A || !defInstr->GetSrc1()->IsAddrOpnd())
            {
                return nullptr;
            }
            Assert(defInstr->GetSrc1()->AsAddrOpnd()->IsVar());
            return defInstr->GetSrc1()->AsAddrOpnd()->m_address;
        }
        else if (valueInfo->IsUndefined())
        {
            return (Js::Var)this->func->GetScriptContextInfo()->GetUndefinedAddr();
        }
        else if (valueInfo->IsNull())
        {
            return (Js::Var)this->func->GetScriptContextInfo()->GetNullAddr();
        }
    }

    return nullptr;
}

bool BoolAndIntStaticAndTypeMismatch(Value* src1Val, Value* src2Val, Js::Var src1Var, Js::Var src2Var)
{
    ValueInfo *src1ValInfo = src1Val->GetValueInfo();
    ValueInfo *src2ValInfo = src2Val->GetValueInfo();
    return (src1ValInfo->IsNumber() && src1Var && src2ValInfo->IsBoolean() && src1Var != Js::TaggedInt::ToVarUnchecked(0) && src1Var != Js::TaggedInt::ToVarUnchecked(1)) ||
        (src2ValInfo->IsNumber() && src2Var && src1ValInfo->IsBoolean() && src2Var != Js::TaggedInt::ToVarUnchecked(0) && src2Var != Js::TaggedInt::ToVarUnchecked(1));
}

bool
GlobOpt::OptConstFoldBranch(IR::Instr *instr, Value *src1Val, Value*src2Val, Value **pDstVal)
{
    if (!src1Val)
    {
        return false;
    }

    int64 left64, right64;
    Js::Var src1Var = this->GetConstantVar(instr->GetSrc1(), src1Val);

    Js::Var src2Var = nullptr;

    if (instr->GetSrc2())
    {
        if (!src2Val)
        {
            return false;
        }

        src2Var = this->GetConstantVar(instr->GetSrc2(), src2Val);
    }

    // Make sure GetConstantVar only returns primitives.
    // TODO: OOP JIT, enabled these asserts
    //Assert(!src1Var || !Js::JavascriptOperators::IsObject(src1Var));
    //Assert(!src2Var || !Js::JavascriptOperators::IsObject(src2Var));

    BOOL result;
    int32 constVal;
    switch (instr->m_opcode)
    {
#define BRANCH(OPCODE,CMP,TYPE,UNSIGNEDNESS) \
    case Js::OpCode::##OPCODE: \
        if (src1Val->GetValueInfo()->TryGetInt64ConstantValue(&left64, UNSIGNEDNESS) && \
            src2Val->GetValueInfo()->TryGetInt64ConstantValue(&right64, UNSIGNEDNESS)) \
        { \
            result = (TYPE)left64 CMP (TYPE)right64; \
        } \
        else \
        { \
            return false; \
        } \
        break;

    BRANCH(BrEq_I4, == , int64, false)
    BRANCH(BrGe_I4, >= , int64, false)
    BRANCH(BrGt_I4, >, int64, false)
    BRANCH(BrLt_I4, <, int64, false)
    BRANCH(BrLe_I4, <= , int64, false)
    BRANCH(BrNeq_I4, != , int64, false)
    BRANCH(BrUnGe_I4, >= , uint64, true)
    BRANCH(BrUnGt_I4, >, uint64, true)
    BRANCH(BrUnLt_I4, <, uint64, true)
    BRANCH(BrUnLe_I4, <= , uint64, true)
    case Js::OpCode::BrEq_A:
    case Js::OpCode::BrNotNeq_A:
        if (!src1Var || !src2Var)
        {
            if (BoolAndIntStaticAndTypeMismatch(src1Val, src2Val, src1Var, src2Var))
            {
                    result = false;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (func->IsOOPJIT() || !CONFIG_FLAG(OOPJITMissingOpts))
            {
                // TODO: OOP JIT, const folding
                return false;
            }
            result = Js::JavascriptOperators::Equal(src1Var, src2Var, this->func->GetScriptContext());
        }
        break;
    case Js::OpCode::BrNeq_A:
    case Js::OpCode::BrNotEq_A:
        if (!src1Var || !src2Var)
        {
            if (BoolAndIntStaticAndTypeMismatch(src1Val, src2Val, src1Var, src2Var))
            {
                result = true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (func->IsOOPJIT() || !CONFIG_FLAG(OOPJITMissingOpts))
            {
                // TODO: OOP JIT, const folding
                return false;
            }
            result = Js::JavascriptOperators::NotEqual(src1Var, src2Var, this->func->GetScriptContext());
        }
        break;
    case Js::OpCode::BrSrEq_A:
    case Js::OpCode::BrSrNotNeq_A:
        if (!src1Var || !src2Var)
        {
            ValueInfo *src1ValInfo = src1Val->GetValueInfo();
            ValueInfo *src2ValInfo = src2Val->GetValueInfo();
            if (
                (src1ValInfo->IsUndefined() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenUndefined()) ||
                (src1ValInfo->IsNull() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenNull()) ||
                (src1ValInfo->IsBoolean() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenBoolean()) ||
                (src1ValInfo->IsNumber() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenNumber()) ||
                (src1ValInfo->IsString() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenString()) ||

                (src2ValInfo->IsUndefined() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenUndefined()) ||
                (src2ValInfo->IsNull() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenNull()) ||
                (src2ValInfo->IsBoolean() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenBoolean()) ||
                (src2ValInfo->IsNumber() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenNumber()) ||
                (src2ValInfo->IsString() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenString())
               )
            {
                result = false;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (func->IsOOPJIT() || !CONFIG_FLAG(OOPJITMissingOpts))
            {
                // TODO: OOP JIT, const folding
                return false;
            }
            result = Js::JavascriptOperators::StrictEqual(src1Var, src2Var, this->func->GetScriptContext());
        }
        break;

    case Js::OpCode::BrSrNeq_A:
    case Js::OpCode::BrSrNotEq_A:
        if (!src1Var || !src2Var)
        {
            ValueInfo *src1ValInfo = src1Val->GetValueInfo();
            ValueInfo *src2ValInfo = src2Val->GetValueInfo();
            if (
                (src1ValInfo->IsUndefined() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenUndefined()) ||
                (src1ValInfo->IsNull() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenNull()) ||
                (src1ValInfo->IsBoolean() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenBoolean()) ||
                (src1ValInfo->IsNumber() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenNumber()) ||
                (src1ValInfo->IsString() && src2ValInfo->IsDefinite() && !src2ValInfo->HasBeenString()) ||

                (src2ValInfo->IsUndefined() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenUndefined()) ||
                (src2ValInfo->IsNull() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenNull()) ||
                (src2ValInfo->IsBoolean() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenBoolean()) ||
                (src2ValInfo->IsNumber() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenNumber()) ||
                (src2ValInfo->IsString() && src1ValInfo->IsDefinite() && !src1ValInfo->HasBeenString())
               )
            {
                result = true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (func->IsOOPJIT() || !CONFIG_FLAG(OOPJITMissingOpts))
            {
                // TODO: OOP JIT, const folding
                return false;
            }
            result = Js::JavascriptOperators::NotStrictEqual(src1Var, src2Var, this->func->GetScriptContext());
        }
        break;

    case Js::OpCode::BrFalse_A:
    case Js::OpCode::BrTrue_A:
    {
        ValueInfo *const src1ValueInfo = src1Val->GetValueInfo();
        if(src1ValueInfo->IsNull() || src1ValueInfo->IsUndefined())
        {
            result = instr->m_opcode == Js::OpCode::BrFalse_A;
            break;
        }
        if(src1ValueInfo->IsObject() && src1ValueInfo->GetObjectType() > ObjectType::Object)
        {
            // Specific object types that are tracked are equivalent to 'true'
            result = instr->m_opcode == Js::OpCode::BrTrue_A;
            break;
        }

        if (func->IsOOPJIT() || !CONFIG_FLAG(OOPJITMissingOpts))
        {
            // TODO: OOP JIT, const folding
            return false;
        }
        if (!src1Var)
        {
            return false;
        }
        result = Js::JavascriptConversion::ToBoolean(src1Var, this->func->GetScriptContext());
        if(instr->m_opcode == Js::OpCode::BrFalse_A)
        {
            result = !result;
        }
        break;
    }
    case Js::OpCode::BrFalse_I4:
        // this path would probably work outside of asm.js, but we should verify that if we ever hit this scenario
        Assert(GetIsAsmJSFunc());
        constVal = 0;
        if (src1Val->GetValueInfo()->TryGetIntConstantValue(&constVal) && constVal != 0)
        {
            instr->FreeSrc1();
            if (instr->GetSrc2())
            {
                instr->FreeSrc2();
            }
            instr->m_opcode = Js::OpCode::Nop;
            return true;
        }
        return false;

    default:
        return false;
#undef BRANCH
    }

    this->OptConstFoldBr(!!result, instr);

    return true;
}

bool
GlobOpt::OptConstFoldUnary(
    IR::Instr * *pInstr,
    const int32 intConstantValue,
    const bool isUsingOriginalSrc1Value,
    Value **pDstVal)
{
    IR::Instr * &instr = *pInstr;
    int32 value = 0;
    IR::Opnd *constOpnd;
    bool isInt = true;
    bool doSetDstVal = true;
    FloatConstType fValue = 0.0;

    if (!DoConstFold())
    {
        return false;
    }

    if (instr->GetDst() && !instr->GetDst()->IsRegOpnd())
    {
        return false;
    }

    switch(instr->m_opcode)
    {
    case Js::OpCode::Neg_A:
        if (intConstantValue == 0)
        {
            // Could fold to -0.0
            return false;
        }

        if (Int32Math::Neg(intConstantValue, &value))
        {
            return false;
        }
        break;

    case Js::OpCode::Not_A:
        Int32Math::Not(intConstantValue, &value);
        break;

    case Js::OpCode::Ld_A:
        if (instr->HasBailOutInfo())
        {
            //The profile data for switch expr can be string and in GlobOpt we realize it is an int.
            if(instr->GetBailOutKind() == IR::BailOutExpectingString)
            {
                throw Js::RejitException(RejitReason::DisableSwitchOptExpectingString);
            }
            Assert(instr->GetBailOutKind() == IR::BailOutExpectingInteger);
            instr->ClearBailOutInfo();
        }
        value = intConstantValue;
        if(isUsingOriginalSrc1Value)
        {
            doSetDstVal = false;  // Let OptDst do it by copying src1Val
        }
        break;

    case Js::OpCode::Conv_Num:
    case Js::OpCode::LdC_A_I4:
        value = intConstantValue;
        if(isUsingOriginalSrc1Value)
        {
            doSetDstVal = false;  // Let OptDst do it by copying src1Val
        }
        break;

    case Js::OpCode::Incr_A:
        if (Int32Math::Inc(intConstantValue, &value))
        {
            return false;
        }
        break;

    case Js::OpCode::Decr_A:
        if (Int32Math::Dec(intConstantValue, &value))
        {
            return false;
        }
        break;

    case Js::OpCode::InlineMathAcos:
        fValue = Js::Math::Acos((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathAsin:
        fValue = Js::Math::Asin((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathAtan:
        fValue = Js::Math::Atan((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathCos:
        fValue = Js::Math::Cos((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathExp:
        fValue = Js::Math::Exp((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathLog:
        fValue = Js::Math::Log((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathSin:
        fValue = Js::Math::Sin((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathSqrt:
        fValue = ::sqrt((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathTan:
        fValue = ::tan((double)intConstantValue);
        isInt = false;
        break;

    case Js::OpCode::InlineMathFround:
        fValue = (double) (float) intConstantValue;
        isInt = false;
        break;

    case Js::OpCode::InlineMathAbs:
        if (intConstantValue == INT32_MIN)
        {
            if (instr->GetDst()->IsInt32())
            {
                // if dst is an int (e.g. in asm.js), we should coerce it, not convert to float
                value = static_cast<int32>(2147483648U);
            }
            else
            {
                // Rejit with AggressiveIntTypeSpecDisabled for Math.abs(INT32_MIN) because it causes dst
                // to be float type which could be different with previous type spec result in LoopPrePass
                throw Js::RejitException(RejitReason::AggressiveIntTypeSpecDisabled);
            }
        }
        else
        {
            value = ::abs(intConstantValue);
        }
        break;

    case Js::OpCode::InlineMathClz:
        DWORD clz;
        if (_BitScanReverse(&clz, intConstantValue))
        {
            value = 31 - clz;
        }
        else
        {
            value = 32;
        }
        instr->ClearBailOutInfo();
        break;

    case Js::OpCode::Ctz:
        Assert(func->GetJITFunctionBody()->IsWasmFunction());
        Assert(!instr->HasBailOutInfo());
        DWORD ctz;
        if (_BitScanForward(&ctz, intConstantValue))
        {
            value = ctz;
        }
        else
        {
            value = 32;
        }
        break;

    case Js::OpCode::InlineMathFloor:
        value = intConstantValue;
        instr->ClearBailOutInfo();
        break;

    case Js::OpCode::InlineMathCeil:
        value = intConstantValue;
        instr->ClearBailOutInfo();
        break;

    case Js::OpCode::InlineMathRound:
        value = intConstantValue;
        instr->ClearBailOutInfo();
        break;
    case Js::OpCode::ToVar:
        if (Js::TaggedInt::IsOverflow(intConstantValue))
        {
            return false;
        }
        else
        {
            value = intConstantValue;
            instr->ClearBailOutInfo();
            break;
        }
    default:
        return false;
    }

    this->CaptureByteCodeSymUses(instr);

    Assert(!instr->HasBailOutInfo()); // If we are, in fact, successful in constant folding the instruction, there is no point in having the bailoutinfo around anymore.
                                      // Make sure that it is cleared if it was initially present.
    if (!isInt)
    {
        value = (int32)fValue;
        if (fValue == (double)value)
        {
            isInt = true;
        }
    }
    if (isInt)
    {
        constOpnd = IR::IntConstOpnd::New(value, TyInt32, instr->m_func);
        GOPT_TRACE(_u("Constant folding to %d\n"), value);
    }
    else
    {
        constOpnd = IR::FloatConstOpnd::New(fValue, TyFloat64, instr->m_func);
        GOPT_TRACE(_u("Constant folding to %f\n"), fValue);
    }
    instr->ReplaceSrc1(constOpnd);

    this->OptSrc(constOpnd, &instr);

    IR::Opnd *dst = instr->GetDst();
    Assert(dst->IsRegOpnd());

    StackSym *dstSym = dst->AsRegOpnd()->m_sym;

    if (isInt)
    {
        if (dstSym->IsSingleDef())
        {
            dstSym->SetIsIntConst(value);
        }

        if (doSetDstVal)
        {
            *pDstVal = GetIntConstantValue(value, instr, dst);
        }

        if (IsTypeSpecPhaseOff(this->func))
        {
            instr->m_opcode = Js::OpCode::LdC_A_I4;
            this->ToVarRegOpnd(dst->AsRegOpnd(), this->currentBlock);
        }
        else
        {
            instr->m_opcode = Js::OpCode::Ld_I4;
            this->ToInt32Dst(instr, dst->AsRegOpnd(), this->currentBlock);

            StackSym * currDstSym = instr->GetDst()->AsRegOpnd()->m_sym;
            if (currDstSym->IsSingleDef())
            {
                currDstSym->SetIsIntConst(value);
            }
        }
    }
    else
    {
        *pDstVal = NewFloatConstantValue(fValue, dst);

        if (IsTypeSpecPhaseOff(this->func))
        {
            instr->m_opcode = Js::OpCode::LdC_A_R8;
            this->ToVarRegOpnd(dst->AsRegOpnd(), this->currentBlock);
        }
        else
        {
            instr->m_opcode = Js::OpCode::LdC_F8_R8;
            this->ToFloat64Dst(instr, dst->AsRegOpnd(), this->currentBlock);
        }
    }
    return true;
}

//------------------------------------------------------------------------------------------------------
// Type specialization
//------------------------------------------------------------------------------------------------------
bool
GlobOpt::IsWorthSpecializingToInt32DueToSrc(IR::Opnd *const src, Value *const val)
{
    Assert(src);
    Assert(val);
    ValueInfo *valueInfo = val->GetValueInfo();
    Assert(valueInfo->IsLikelyInt());

    // If it is not known that the operand is definitely an int, the operand is not already type-specialized, and it's not live
    // in the loop landing pad (if we're in a loop), it's probably not worth type-specializing this instruction. The common case
    // where type-specializing this would be bad is where the operations are entirely on properties or array elements, where the
    // ratio of FromVars and ToVars to the number of actual operations is high, and the conversions would dominate the time
    // spent. On the other hand, if we're using a function formal parameter more than once, it would probably be worth
    // type-specializing it, hence the IsDead check on the operands.
    return
        valueInfo->IsInt() ||
        valueInfo->HasIntConstantValue(true) ||
        !src->GetIsDead() ||
        !src->IsRegOpnd() ||
        CurrentBlockData()->IsInt32TypeSpecialized(src->AsRegOpnd()->m_sym) ||
        (this->currentBlock->loop && this->currentBlock->loop->landingPad->globOptData.IsLive(src->AsRegOpnd()->m_sym));
}

bool
GlobOpt::IsWorthSpecializingToInt32DueToDst(IR::Opnd *const dst)
{
    Assert(dst);

    const auto sym = dst->AsRegOpnd()->m_sym;
    return
        CurrentBlockData()->IsInt32TypeSpecialized(sym) ||
        (this->currentBlock->loop && this->currentBlock->loop->landingPad->globOptData.IsLive(sym));
}

bool
GlobOpt::IsWorthSpecializingToInt32(IR::Instr *const instr, Value *const src1Val, Value *const src2Val)
{
    Assert(instr);

    const auto src1 = instr->GetSrc1();
    const auto src2 = instr->GetSrc2();

    // In addition to checking each operand and the destination, if for any reason we only have to do a maximum of two
    // conversions instead of the worst-case 3 conversions, it's probably worth specializing.
    if (IsWorthSpecializingToInt32DueToSrc(src1, src1Val) ||
        (src2Val && IsWorthSpecializingToInt32DueToSrc(src2, src2Val)))
    {
        return true;
    }

    IR::Opnd *dst = instr->GetDst();
    if (!dst || IsWorthSpecializingToInt32DueToDst(dst))
    {
        return true;
    }

    if (dst->IsEqual(src1) || (src2Val && (dst->IsEqual(src2) || src1->IsEqual(src2))))
    {
        return true;
    }

    IR::Instr *instrNext = instr->GetNextRealInstrOrLabel();

    // Skip useless Ld_A's
    do
    {
        switch (instrNext->m_opcode)
        {
        case Js::OpCode::Ld_A:
            if (!dst->IsEqual(instrNext->GetSrc1()))
            {
                goto done;
            }
            dst = instrNext->GetDst();
            break;
        case Js::OpCode::LdFld:
        case Js::OpCode::LdRootFld:
        case Js::OpCode::LdRootFldForTypeOf:
        case Js::OpCode::LdFldForTypeOf:
        case Js::OpCode::LdElemI_A:
        case Js::OpCode::ByteCodeUses:
            break;
        default:
            goto done;
        }

        instrNext = instrNext->GetNextRealInstrOrLabel();
    } while (true);
done:

    // If the next instr could also be type specialized, then it is probably worth it.
    if ((instrNext->GetSrc1() && dst->IsEqual(instrNext->GetSrc1())) || (instrNext->GetSrc2() && dst->IsEqual(instrNext->GetSrc2())))
    {
        switch (instrNext->m_opcode)
        {
        case Js::OpCode::Add_A:
        case Js::OpCode::Sub_A:
        case Js::OpCode::Mul_A:
        case Js::OpCode::Div_A:
        case Js::OpCode::Rem_A:
        case Js::OpCode::Xor_A:
        case Js::OpCode::And_A:
        case Js::OpCode::Or_A:
        case Js::OpCode::Shl_A:
        case Js::OpCode::Shr_A:
        case Js::OpCode::Incr_A:
        case Js::OpCode::Decr_A:
        case Js::OpCode::Neg_A:
        case Js::OpCode::Not_A:
        case Js::OpCode::Conv_Num:
        case Js::OpCode::BrEq_I4:
        case Js::OpCode::BrTrue_I4:
        case Js::OpCode::BrFalse_I4:
        case Js::OpCode::BrGe_I4:
        case Js::OpCode::BrGt_I4:
        case Js::OpCode::BrLt_I4:
        case Js::OpCode::BrLe_I4:
        case Js::OpCode::BrNeq_I4:
            return true;
        }
    }

    return false;
}

bool
GlobOpt::TypeSpecializeNumberUnary(IR::Instr *instr, Value *src1Val, Value **pDstVal)
{
    Assert(src1Val->GetValueInfo()->IsNumber());

    if (this->IsLoopPrePass())
    {
        return false;
    }

    switch (instr->m_opcode)
    {
    case Js::OpCode::Conv_Num:
        // Optimize Conv_Num away since we know this is a number
        instr->m_opcode = Js::OpCode::Ld_A;
        return false;
    }

    return false;
}

bool
GlobOpt::TypeSpecializeUnary(
    IR::Instr **pInstr,
    Value **pSrc1Val,
    Value **pDstVal,
    Value *const src1OriginalVal,
    bool *redoTypeSpecRef,
    bool *const forceInvariantHoistingRef)
{
    Assert(pSrc1Val);
    Value *&src1Val = *pSrc1Val;
    Assert(src1Val);

    // We don't need to do typespec for asmjs
    if (IsTypeSpecPhaseOff(this->func) || GetIsAsmJSFunc())
    {
        return false;
    }

    IR::Instr *&instr = *pInstr;
    int32 min, max;

    // Inline built-ins explicitly specify how srcs/dst must be specialized.
    if (OpCodeAttr::IsInlineBuiltIn(instr->m_opcode))
    {
        TypeSpecializeInlineBuiltInUnary(pInstr, &src1Val, pDstVal, src1OriginalVal, redoTypeSpecRef);
        return true;
    }

    // Consider: If type spec wasn't completely done, make sure that we don't type-spec the dst 2nd time.
    if(instr->m_opcode == Js::OpCode::LdLen_A && TypeSpecializeLdLen(&instr, &src1Val, pDstVal, forceInvariantHoistingRef))
    {
        return true;
    }

    if (!src1Val->GetValueInfo()->GetIntValMinMax(&min, &max, this->DoAggressiveIntTypeSpec()))
    {
        src1Val = src1OriginalVal;
        if (src1Val->GetValueInfo()->IsLikelyFloat())
        {
            // Try to type specialize to float
            return this->TypeSpecializeFloatUnary(pInstr, src1Val, pDstVal);
        }
        else if (src1Val->GetValueInfo()->IsNumber())
        {
            return TypeSpecializeNumberUnary(instr, src1Val, pDstVal);
        }
        return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
    }

    return this->TypeSpecializeIntUnary(pInstr, &src1Val, pDstVal, min, max, src1OriginalVal, redoTypeSpecRef);
}

// Returns true if the built-in requested type specialization, and no further action needed,
// otherwise returns false.
void
GlobOpt::TypeSpecializeInlineBuiltInUnary(IR::Instr **pInstr, Value **pSrc1Val, Value **pDstVal, Value *const src1OriginalVal, bool *redoTypeSpecRef)
{
    IR::Instr *&instr = *pInstr;

    Assert(pSrc1Val);
    Value *&src1Val = *pSrc1Val;

    Assert(OpCodeAttr::IsInlineBuiltIn(instr->m_opcode));

    Js::BuiltinFunction builtInId = Js::JavascriptLibrary::GetBuiltInInlineCandidateId(instr->m_opcode);   // From actual instr, not profile based.
    Assert(builtInId != Js::BuiltinFunction::None);

    // Consider using different bailout for float/int FromVars, so that when the arg cannot be converted to number we don't disable
    //       type spec for other parts of the big function but rather just don't inline that built-in instr.
    //       E.g. could do that if the value is not likelyInt/likelyFloat.

    Js::BuiltInFlags builtInFlags = Js::JavascriptLibrary::GetFlagsForBuiltIn(builtInId);
    bool areAllArgsAlwaysFloat = (builtInFlags & Js::BuiltInFlags::BIF_Args) == Js::BuiltInFlags::BIF_TypeSpecUnaryToFloat;
    if (areAllArgsAlwaysFloat)
    {
        // InlineMathAcos, InlineMathAsin, InlineMathAtan, InlineMathCos, InlineMathExp, InlineMathLog, InlineMathSin, InlineMathSqrt, InlineMathTan.
        Assert(this->DoFloatTypeSpec());

        // Type-spec the src.
        src1Val = src1OriginalVal;
        bool retVal = this->TypeSpecializeFloatUnary(pInstr, src1Val, pDstVal, /* skipDst = */ true);
        AssertMsg(retVal, "For inline built-ins the args have to be type-specialized to float, but something failed during the process.");

        // Type-spec the dst.
        this->TypeSpecializeFloatDst(instr, nullptr, src1Val, nullptr, pDstVal);
    }
    else if (instr->m_opcode == Js::OpCode::InlineMathAbs)
    {
        // Consider the case when the value is unknown - because of bailout in abs we may disable type spec for the whole function which is too much.
        // First, try int.
        int minVal, maxVal;
        bool shouldTypeSpecToInt = src1Val->GetValueInfo()->GetIntValMinMax(&minVal, &maxVal, /* doAggressiveIntTypeSpec = */ true);
        if (shouldTypeSpecToInt)
        {
            Assert(this->DoAggressiveIntTypeSpec());
            bool retVal = this->TypeSpecializeIntUnary(pInstr, &src1Val, pDstVal, minVal, maxVal, src1OriginalVal, redoTypeSpecRef, true);
            AssertMsg(retVal, "For inline built-ins the args have to be type-specialized (int), but something failed during the process.");

            if (!this->IsLoopPrePass())
            {
                // Create bailout for INT_MIN which does not have corresponding int value on the positive side.
                // Check int range: if we know the range is out of overflow, we do not need the bail out at all.
                if (minVal == INT32_MIN)
                {
                    GenerateBailAtOperation(&instr, IR::BailOnIntMin);
                }
            }

            // Account for ::abs(INT_MIN) == INT_MIN (which is less than 0).
            maxVal = ::max(
                ::abs(Int32Math::NearestInRangeTo(minVal, INT_MIN + 1, INT_MAX)),
                ::abs(Int32Math::NearestInRangeTo(maxVal, INT_MIN + 1, INT_MAX)));
            minVal = minVal >= 0 ? minVal : 0;
            this->TypeSpecializeIntDst(instr, instr->m_opcode, nullptr, src1Val, nullptr, IR::BailOutInvalid, minVal, maxVal, pDstVal);
        }
        else
        {
            // If we couldn't do int, do float.
            Assert(this->DoFloatTypeSpec());
            src1Val = src1OriginalVal;
            bool retVal = this->TypeSpecializeFloatUnary(pInstr, src1Val, pDstVal, true);
            AssertMsg(retVal, "For inline built-ins the args have to be type-specialized (float), but something failed during the process.");

            this->TypeSpecializeFloatDst(instr, nullptr, src1Val, nullptr, pDstVal);
        }
    }
    else if (instr->m_opcode == Js::OpCode::InlineMathFloor || instr->m_opcode == Js::OpCode::InlineMathCeil || instr->m_opcode == Js::OpCode::InlineMathRound)
    {
        // Type specialize src to float
        src1Val = src1OriginalVal;
        bool retVal = this->TypeSpecializeFloatUnary(pInstr, src1Val, pDstVal, /* skipDst = */ true);
        AssertMsg(retVal, "For inline Math.floor and Math.ceil the src has to be type-specialized to float, but something failed during the process.");

        // Type specialize dst to int
        this->TypeSpecializeIntDst(
            instr,
            instr->m_opcode,
            nullptr,
            src1Val,
            nullptr,
            IR::BailOutInvalid,
            INT32_MIN,
            INT32_MAX,
            pDstVal);
    }
    else if(instr->m_opcode == Js::OpCode::InlineArrayPop)
    {
        IR::Opnd *const thisOpnd = instr->GetSrc1();

        Assert(thisOpnd);

        // Ensure src1 (Array) is a var
        this->ToVarUses(instr, thisOpnd, false, src1Val);

        if(!this->IsLoopPrePass() && thisOpnd->GetValueType().IsLikelyNativeArray())
        {
            // We bail out, if there is illegal access or a mismatch in the Native array type that is optimized for, during the run time.
            GenerateBailAtOperation(&instr, IR::BailOutConventionalNativeArrayAccessOnly);
        }

        if(!instr->GetDst())
        {
            return;
        }

        // Try Type Specializing the element (return item from Pop) based on the array's profile data.
        if(thisOpnd->GetValueType().IsLikelyNativeIntArray())
        {
            this->TypeSpecializeIntDst(instr, instr->m_opcode, nullptr, nullptr, nullptr, IR::BailOutInvalid, INT32_MIN, INT32_MAX, pDstVal);
        }
        else if(thisOpnd->GetValueType().IsLikelyNativeFloatArray())
        {
            this->TypeSpecializeFloatDst(instr, nullptr, nullptr, nullptr, pDstVal);
        }
        else
        {
            // We reached here so the Element is not yet type specialized. Ensure element is a var
            if(instr->GetDst()->IsRegOpnd())
            {
                this->ToVarRegOpnd(instr->GetDst()->AsRegOpnd(), currentBlock);
            }
        }
    }
    else if (instr->m_opcode == Js::OpCode::InlineMathClz)
    {
        Assert(this->DoAggressiveIntTypeSpec());
        Assert(this->DoLossyIntTypeSpec());
        //Type specialize to int
        bool retVal = this->TypeSpecializeIntUnary(pInstr, &src1Val, pDstVal, INT32_MIN, INT32_MAX, src1OriginalVal, redoTypeSpecRef);
        AssertMsg(retVal, "For clz32, the arg has to be type-specialized to int.");
    }
    else
    {
        AssertMsg(FALSE, "Unsupported built-in!");
    }
}

void
GlobOpt::TypeSpecializeInlineBuiltInBinary(IR::Instr **pInstr, Value *src1Val, Value* src2Val, Value **pDstVal, Value *const src1OriginalVal, Value *const src2OriginalVal)
{
    IR::Instr *&instr = *pInstr;
    Assert(OpCodeAttr::IsInlineBuiltIn(instr->m_opcode));

    switch(instr->m_opcode)
    {
        case Js::OpCode::InlineMathAtan2:
        {
            Js::BuiltinFunction builtInId = Js::JavascriptLibrary::GetBuiltInInlineCandidateId(instr->m_opcode);   // From actual instr, not profile based.
            Js::BuiltInFlags builtInFlags = Js::JavascriptLibrary::GetFlagsForBuiltIn(builtInId);

            bool areAllArgsAlwaysFloat = (builtInFlags & Js::BuiltInFlags::BIF_TypeSpecAllToFloat) != 0;
            Assert(areAllArgsAlwaysFloat);
            Assert(this->DoFloatTypeSpec());

            // Type-spec the src1, src2 and dst.
            src1Val = src1OriginalVal;
            src2Val = src2OriginalVal;
            bool retVal = this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
            AssertMsg(retVal, "For pow and atnan2 the args have to be type-specialized to float, but something failed during the process.");

            break;
        }

        case Js::OpCode::InlineMathPow:
        {
#ifndef _M_ARM
            if (src2Val->GetValueInfo()->IsLikelyInt())
            {
                bool lossy = false;

                this->ToInt32(instr, instr->GetSrc2(), this->currentBlock, src2Val, nullptr, lossy);

                IR::Opnd* src1 = instr->GetSrc1();
                int32 valueMin, valueMax;
                if (src1Val->GetValueInfo()->IsLikelyInt() &&
                    this->DoPowIntIntTypeSpec() &&
                    src2Val->GetValueInfo()->GetIntValMinMax(&valueMin, &valueMax, this->DoAggressiveIntTypeSpec()) &&
                    valueMin >= 0)

                {
                    this->ToInt32(instr, src1, this->currentBlock, src1Val, nullptr, lossy);
                    this->TypeSpecializeIntDst(instr, instr->m_opcode, nullptr, src1Val, src2Val, IR::BailOutInvalid, INT32_MIN, INT32_MAX, pDstVal);

                    if(!this->IsLoopPrePass())
                    {
                        GenerateBailAtOperation(&instr, IR::BailOutOnPowIntIntOverflow);
                    }
                }
                else
                {
                    this->ToFloat64(instr, src1, this->currentBlock, src1Val, nullptr, IR::BailOutPrimitiveButString);
                    TypeSpecializeFloatDst(instr, nullptr, src1Val, src2Val, pDstVal);
                }
            }
            else
            {
#endif
                this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
#ifndef _M_ARM
            }
#endif
            break;
        }

        case Js::OpCode::InlineMathImul:
        {
            Assert(this->DoAggressiveIntTypeSpec());
            Assert(this->DoLossyIntTypeSpec());

            //Type specialize to int
            bool retVal = this->TypeSpecializeIntBinary(pInstr, src1Val, src2Val, pDstVal, INT32_MIN, INT32_MAX, false /* skipDst */);

            AssertMsg(retVal, "For imul, the args have to be type-specialized to int but something failed during the process.");
            break;
        }

        case Js::OpCode::InlineMathMin:
        case Js::OpCode::InlineMathMax:
        {
            if(src1Val->GetValueInfo()->IsLikelyInt() && src2Val->GetValueInfo()->IsLikelyInt())
            {
                // Compute resulting range info
                int32 min1, max1, min2, max2, newMin, newMax;

                Assert(this->DoAggressiveIntTypeSpec());
                src1Val->GetValueInfo()->GetIntValMinMax(&min1, &max1, this->DoAggressiveIntTypeSpec());
                src2Val->GetValueInfo()->GetIntValMinMax(&min2, &max2, this->DoAggressiveIntTypeSpec());
                if (instr->m_opcode == Js::OpCode::InlineMathMin)
                {
                    newMin = min(min1, min2);
                    newMax = min(max1, max2);
                }
                else
                {
                    Assert(instr->m_opcode == Js::OpCode::InlineMathMax);
                    newMin = max(min1, min2);
                    newMax = max(max1, max2);
                }
                // Type specialize to int
                bool retVal = this->TypeSpecializeIntBinary(pInstr, src1Val, src2Val, pDstVal, newMin, newMax, false /* skipDst */);
                AssertMsg(retVal, "For min and max, the args have to be type-specialized to int if any one of the sources is an int, but something failed during the process.");
            }

            // Couldn't type specialize to int, type specialize to float
            else
            {
                Assert(this->DoFloatTypeSpec());
                src1Val = src1OriginalVal;
                src2Val = src2OriginalVal;
                bool retVal = this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
                AssertMsg(retVal, "For min and max, the args have to be type-specialized to float if any one of the sources is a float, but something failed during the process.");
            }
            break;
        }
        case Js::OpCode::InlineArrayPush:
        {
            IR::Opnd *const thisOpnd = instr->GetSrc1();

            Assert(thisOpnd);

            if(instr->GetDst() && instr->GetDst()->IsRegOpnd())
            {
                // Set the dst as live here, as the built-ins return early from the TypeSpecialization functions - before the dst is marked as live.
                // Also, we are not specializing the dst separately and we are skipping the dst to be handled when we specialize the instruction above.
                this->ToVarRegOpnd(instr->GetDst()->AsRegOpnd(), currentBlock);
            }

            // Ensure src1 (Array) is a var
            this->ToVarUses(instr, thisOpnd, false, src1Val);

            if(!this->IsLoopPrePass())
            {
                if(thisOpnd->GetValueType().IsLikelyNativeArray())
                {
                    // We bail out, if there is illegal access or a mismatch in the Native array type that is optimized for, during run time.
                    GenerateBailAtOperation(&instr, IR::BailOutConventionalNativeArrayAccessOnly);
                }
                else
                {
                    GenerateBailAtOperation(&instr, IR::BailOutOnImplicitCallsPreOp);
                }
            }

            // Try Type Specializing the element based on the array's profile data.
            if(thisOpnd->GetValueType().IsLikelyNativeFloatArray())
            {
                src1Val = src1OriginalVal;
                src2Val = src2OriginalVal;
            }
            if((thisOpnd->GetValueType().IsLikelyNativeIntArray() && this->TypeSpecializeIntBinary(pInstr, src1Val, src2Val, pDstVal, INT32_MIN, INT32_MAX, true))
                || (thisOpnd->GetValueType().IsLikelyNativeFloatArray() && this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal)))
            {
                break;
            }

            // The Element is not yet type specialized. Ensure element is a var
            this->ToVarUses(instr, instr->GetSrc2(), false, src2Val);
            break;
        }
    }
}

void
GlobOpt::TypeSpecializeInlineBuiltInDst(IR::Instr **pInstr, Value **pDstVal)
{
    IR::Instr *&instr = *pInstr;
    Assert(OpCodeAttr::IsInlineBuiltIn(instr->m_opcode));

    if (instr->m_opcode == Js::OpCode::InlineMathRandom)
    {
        Assert(this->DoFloatTypeSpec());
        // Type specialize dst to float
        this->TypeSpecializeFloatDst(instr, nullptr, nullptr, nullptr, pDstVal);
    }
}

bool
GlobOpt::TryTypeSpecializeUnaryToFloatHelper(IR::Instr** pInstr, Value** pSrc1Val, Value* const src1OriginalVal, Value **pDstVal)
{
    // It has been determined that this instruction cannot be int-specialized. We need to determine whether to attempt to
    // float-specialize the instruction, or leave it unspecialized.
#if !INT32VAR
    Value*& src1Val = *pSrc1Val;
    if(src1Val->GetValueInfo()->IsLikelyUntaggedInt())
    {
        // An input range is completely outside the range of an int31. Even if the operation may overflow, it is
        // unlikely to overflow on these operations, so we leave it unspecialized on 64-bit platforms. However, on
        // 32-bit platforms, the value is untaggable and will be a JavascriptNumber, which is significantly slower to
        // use in an unspecialized operation compared to a tagged int. So, try to float-specialize the instruction.
        src1Val = src1OriginalVal;
        return this->TypeSpecializeFloatUnary(pInstr, src1Val, pDstVal);
    }
#endif
    return false;
}

bool
GlobOpt::TypeSpecializeIntBinary(IR::Instr **pInstr, Value *src1Val, Value *src2Val, Value **pDstVal, int32 min, int32 max, bool skipDst /* = false */)
{
    // Consider moving the code for int type spec-ing binary functions here.
    IR::Instr *&instr = *pInstr;
    bool lossy = false;

    if(OpCodeAttr::IsInlineBuiltIn(instr->m_opcode))
    {
        if(instr->m_opcode == Js::OpCode::InlineArrayPush)
        {
            int32 intConstantValue;
            bool isIntConstMissingItem = src2Val->GetValueInfo()->TryGetIntConstantValue(&intConstantValue);

            if(isIntConstMissingItem)
            {
                isIntConstMissingItem = Js::SparseArraySegment<int>::IsMissingItem(&intConstantValue);
            }

            // Don't specialize if the element is not likelyInt or an IntConst which is a missing item value.
            if(!(src2Val->GetValueInfo()->IsLikelyInt()) || isIntConstMissingItem)
            {
                return false;
            }
            // We don't want to specialize both the source operands, though it is a binary instr.
            IR::Opnd * elementOpnd = instr->GetSrc2();
            this->ToInt32(instr, elementOpnd, this->currentBlock, src2Val, nullptr, lossy);
        }
        else
        {
            IR::Opnd *src1 = instr->GetSrc1();
            this->ToInt32(instr, src1, this->currentBlock, src1Val, nullptr, lossy);

            IR::Opnd *src2 = instr->GetSrc2();
            this->ToInt32(instr, src2, this->currentBlock, src2Val, nullptr, lossy);
        }

        if(!skipDst)
        {
            IR::Opnd *dst = instr->GetDst();
            if (dst)
            {
                TypeSpecializeIntDst(instr, instr->m_opcode, nullptr, src1Val, src2Val, IR::BailOutInvalid, min, max, pDstVal);
            }
        }
        return true;
    }
    else
    {
        AssertMsg(false, "Yet to move code for other binary functions here");
        return false;
    }
}

bool
GlobOpt::TypeSpecializeIntUnary(
    IR::Instr **pInstr,
    Value **pSrc1Val,
    Value **pDstVal,
    int32 min,
    int32 max,
    Value *const src1OriginalVal,
    bool *redoTypeSpecRef,
    bool skipDst /* = false */)
{
    IR::Instr *&instr = *pInstr;

    Assert(pSrc1Val);
    Value *&src1Val = *pSrc1Val;

    bool isTransfer = false;
    Js::OpCode opcode;
    int32 newMin, newMax;
    bool lossy = false;
    IR::BailOutKind bailOutKind = IR::BailOutInvalid;
    bool ignoredIntOverflow = this->ignoredIntOverflowForCurrentInstr;
    bool ignoredNegativeZero = false;
    bool checkTypeSpecWorth = false;

    if(instr->GetSrc1()->IsRegOpnd() && instr->GetSrc1()->AsRegOpnd()->m_sym->m_isNotInt)
    {
        return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
    }

    AddSubConstantInfo addSubConstantInfo;

    switch(instr->m_opcode)
    {
    case Js::OpCode::Ld_A:
        if (instr->GetSrc1()->IsRegOpnd())
        {
            StackSym *sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
            if (CurrentBlockData()->IsInt32TypeSpecialized(sym) == false)
            {
                // Type specializing an Ld_A isn't worth it, unless the src
                // is already type specialized.
                return false;
            }
        }
        newMin = min;
        newMax = max;
        opcode = Js::OpCode::Ld_I4;
        isTransfer = true;
        break;

    case Js::OpCode::Conv_Num:
        newMin = min;
        newMax = max;
        opcode = Js::OpCode::Ld_I4;
        isTransfer = true;
        break;

    case Js::OpCode::LdC_A_I4:
        newMin = newMax = instr->GetSrc1()->AsIntConstOpnd()->AsInt32();
        opcode = Js::OpCode::Ld_I4;
        break;

    case Js::OpCode::Neg_A:
        if (min <= 0 && max >= 0)
        {
            if(instr->ShouldCheckForNegativeZero())
            {
                // -0 matters since the sym is not a local, or is used in a way in which -0 would differ from +0
                if(!DoAggressiveIntTypeSpec())
                {
                    // May result in -0
                    // Consider adding a dynamic check for src1 == 0
                    return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                }
                if(min == 0 && max == 0)
                {
                    // Always results in -0
                    return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                }
                bailOutKind |= IR::BailOutOnNegativeZero;
            }
            else
            {
                ignoredNegativeZero = true;
            }
        }
        if (Int32Math::Neg(min, &newMax))
        {
            if(instr->ShouldCheckForIntOverflow())
            {
                if(!DoAggressiveIntTypeSpec())
                {
                    // May overflow
                    return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                }
                if(min == max)
                {
                    // Always overflows
                    return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                }
                bailOutKind |= IR::BailOutOnOverflow;
                newMax = INT32_MAX;
            }
            else
            {
                ignoredIntOverflow = true;
            }
        }
        if (Int32Math::Neg(max, &newMin))
        {
            if(instr->ShouldCheckForIntOverflow())
            {
                if(!DoAggressiveIntTypeSpec())
                {
                    // May overflow
                    return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                }
                bailOutKind |= IR::BailOutOnOverflow;
                newMin = INT32_MAX;
            }
            else
            {
                ignoredIntOverflow = true;
            }
        }
        if(!instr->ShouldCheckForIntOverflow() && newMin > newMax)
        {
            // When ignoring overflow, the range needs to account for overflow. Since MIN_INT is the only int32 value that
            // overflows on Neg, and the value resulting from overflow is also MIN_INT, if calculating only the new min or new
            // max overflowed but not both, then the new min will be greater than the new max. In that case we need to consider
            // the full range of int32s as possible resulting values.
            newMin = INT32_MIN;
            newMax = INT32_MAX;
        }
        opcode = Js::OpCode::Neg_I4;
        checkTypeSpecWorth = true;
        break;

    case Js::OpCode::Not_A:
        if(!DoLossyIntTypeSpec())
        {
            return false;
        }
        this->PropagateIntRangeForNot(min, max, &newMin, &newMax);
        opcode = Js::OpCode::Not_I4;
        lossy = true;
        break;

    case Js::OpCode::Incr_A:
        do // while(false)
        {
            const auto CannotOverflowBasedOnRelativeBounds = [&]()
            {
                const ValueInfo *const src1ValueInfo = src1Val->GetValueInfo();
                return
                    (src1ValueInfo->IsInt() || DoAggressiveIntTypeSpec()) &&
                    src1ValueInfo->IsIntBounded() &&
                    src1ValueInfo->AsIntBounded()->Bounds()->AddCannotOverflowBasedOnRelativeBounds(1);
            };

            if (Int32Math::Inc(min, &newMin))
            {
                if(CannotOverflowBasedOnRelativeBounds())
                {
                    newMin = INT32_MAX;
                }
                else if(instr->ShouldCheckForIntOverflow())
                {
                    // Always overflows
                    return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                }
                else
                {
                    // When ignoring overflow, the range needs to account for overflow. For any Add or Sub, since overflow
                    // causes the value to wrap around, and we don't have a way to specify a lower and upper range of ints,
                    // we use the full range of int32s.
                    ignoredIntOverflow = true;
                    newMin = INT32_MIN;
                    newMax = INT32_MAX;
                    break;
                }
            }
            if (Int32Math::Inc(max, &newMax))
            {
                if(CannotOverflowBasedOnRelativeBounds())
                {
                    newMax = INT32_MAX;
                }
                else if(instr->ShouldCheckForIntOverflow())
                {
                    if(!DoAggressiveIntTypeSpec())
                    {
                        // May overflow
                        return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                    }
                    bailOutKind |= IR::BailOutOnOverflow;
                    newMax = INT32_MAX;
                }
                else
                {
                    // See comment about ignoring overflow above
                    ignoredIntOverflow = true;
                    newMin = INT32_MIN;
                    newMax = INT32_MAX;
                    break;
                }
            }
        } while(false);

        if(!ignoredIntOverflow && instr->GetSrc1()->IsRegOpnd())
        {
            addSubConstantInfo.Set(instr->GetSrc1()->AsRegOpnd()->m_sym, src1Val, min == max, 1);
        }

        opcode = Js::OpCode::Add_I4;
        if (!this->IsLoopPrePass())
        {
            instr->SetSrc2(IR::IntConstOpnd::New(1, TyInt32, instr->m_func));
        }
        checkTypeSpecWorth = true;
        break;

    case Js::OpCode::Decr_A:
        do // while(false)
        {
            const auto CannotOverflowBasedOnRelativeBounds = [&]()
            {
                const ValueInfo *const src1ValueInfo = src1Val->GetValueInfo();
                return
                    (src1ValueInfo->IsInt() || DoAggressiveIntTypeSpec()) &&
                    src1ValueInfo->IsIntBounded() &&
                    src1ValueInfo->AsIntBounded()->Bounds()->SubCannotOverflowBasedOnRelativeBounds(1);
            };

            if (Int32Math::Dec(max, &newMax))
            {
                if(CannotOverflowBasedOnRelativeBounds())
                {
                    newMax = INT32_MIN;
                }
                else if(instr->ShouldCheckForIntOverflow())
                {
                    // Always overflows
                    return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                }
                else
                {
                    // When ignoring overflow, the range needs to account for overflow. For any Add or Sub, since overflow
                    // causes the value to wrap around, and we don't have a way to specify a lower and upper range of ints, we
                    // use the full range of int32s.
                    ignoredIntOverflow = true;
                    newMin = INT32_MIN;
                    newMax = INT32_MAX;
                    break;
                }
            }
            if (Int32Math::Dec(min, &newMin))
            {
                if(CannotOverflowBasedOnRelativeBounds())
                {
                    newMin = INT32_MIN;
                }
                else if(instr->ShouldCheckForIntOverflow())
                {
                    if(!DoAggressiveIntTypeSpec())
                    {
                        // May overflow
                        return TryTypeSpecializeUnaryToFloatHelper(pInstr, &src1Val, src1OriginalVal, pDstVal);
                    }
                    bailOutKind |= IR::BailOutOnOverflow;
                    newMin = INT32_MIN;
                }
                else
                {
                    // See comment about ignoring overflow above
                    ignoredIntOverflow = true;
                    newMin = INT32_MIN;
                    newMax = INT32_MAX;
                    break;
                }
            }
        } while(false);

        if(!ignoredIntOverflow && instr->GetSrc1()->IsRegOpnd())
        {
            addSubConstantInfo.Set(instr->GetSrc1()->AsRegOpnd()->m_sym, src1Val, min == max, -1);
        }

        opcode = Js::OpCode::Sub_I4;
        if (!this->IsLoopPrePass())
        {
            instr->SetSrc2(IR::IntConstOpnd::New(1, TyInt32, instr->m_func));
        }
        checkTypeSpecWorth = true;
        break;

    case Js::OpCode::BrFalse_A:
    case Js::OpCode::BrTrue_A:
    {
        if(DoConstFold() && !IsLoopPrePass() && TryOptConstFoldBrFalse(instr, src1Val, min, max))
        {
            return true;
        }

        bool specialize = true;
        if (!src1Val->GetValueInfo()->HasIntConstantValue() && instr->GetSrc1()->IsRegOpnd())
        {
            StackSym *sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
            if (CurrentBlockData()->IsInt32TypeSpecialized(sym) == false)
            {
                // Type specializing a BrTrue_A/BrFalse_A isn't worth it, unless the src
                // is already type specialized
                specialize = false;
            }
        }
        if(instr->m_opcode == Js::OpCode::BrTrue_A)
        {
            UpdateIntBoundsForNotEqualBranch(src1Val, nullptr, 0);
            opcode = Js::OpCode::BrTrue_I4;
        }
        else
        {
            UpdateIntBoundsForEqualBranch(src1Val, nullptr, 0);
            opcode = Js::OpCode::BrFalse_I4;
        }
        if(!specialize)
        {
            return false;
        }

        newMin = 2; newMax = 1;  // We'll assert if we make a range where min > max
        break;
    }

    case Js::OpCode::MultiBr:
        newMin = min;
        newMax = max;
        opcode = instr->m_opcode;
        break;

    case Js::OpCode::StElemI_A:
    case Js::OpCode::StElemI_A_Strict:
    case Js::OpCode::StElemC:
        if(instr->GetDst()->AsIndirOpnd()->GetBaseOpnd()->GetValueType().IsLikelyAnyArrayWithNativeFloatValues())
        {
            src1Val = src1OriginalVal;
        }
        return TypeSpecializeStElem(pInstr, src1Val, pDstVal);

    case Js::OpCode::NewScArray:
    case Js::OpCode::NewScArrayWithMissingValues:
    case Js::OpCode::InitFld:
    case Js::OpCode::InitRootFld:
    case Js::OpCode::StSlot:
    case Js::OpCode::StSlotChkUndecl:
#if !FLOATVAR
    case Js::OpCode::StSlotBoxTemp:
#endif
    case Js::OpCode::StFld:
    case Js::OpCode::StRootFld:
    case Js::OpCode::StFldStrict:
    case Js::OpCode::StRootFldStrict:
    case Js::OpCode::ArgOut_A:
    case Js::OpCode::ArgOut_A_Inline:
    case Js::OpCode::ArgOut_A_FixupForStackArgs:
    case Js::OpCode::ArgOut_A_Dynamic:
    case Js::OpCode::ArgOut_A_FromStackArgs:
    case Js::OpCode::ArgOut_A_SpreadArg:
    // For this one we need to implement type specialization
    //case Js::OpCode::ArgOut_A_InlineBuiltIn:
    case Js::OpCode::Ret:
    case Js::OpCode::LdElemUndef:
    case Js::OpCode::LdElemUndefScoped:
        return false;

    default:
        if (OpCodeAttr::IsInlineBuiltIn(instr->m_opcode))
        {
            newMin = min;
            newMax = max;
            opcode = instr->m_opcode;
            break; // Note: we must keep checkTypeSpecWorth = false to make sure we never return false from this function.
        }
        return false;
    }

    // If this instruction is in a range of instructions where int overflow does not matter, we will still specialize it (won't
    // leave it unspecialized based on heuristics), since it is most likely worth specializing, and the dst value needs to be
    // guaranteed to be an int
    if(checkTypeSpecWorth &&
        !ignoredIntOverflow &&
        !ignoredNegativeZero &&
        instr->ShouldCheckForIntOverflow() &&
        !IsWorthSpecializingToInt32(instr, src1Val))
    {
        // Even though type specialization is being skipped since it may not be worth it, the proper value should still be
        // maintained so that the result may be type specialized later. An int value is not created for the dst in any of
        // the following cases.
        // - A bailout check is necessary to specialize this instruction. The bailout check is what guarantees the result to be
        //   an int, but since we're not going to specialize this instruction, there won't be a bailout check.
        // - Aggressive int type specialization is disabled and we're in a loop prepass. We're conservative on dst values in
        //   that case, especially if the dst sym is live on the back-edge.
        if(bailOutKind == IR::BailOutInvalid &&
            instr->GetDst() &&
            (DoAggressiveIntTypeSpec() || !this->IsLoopPrePass()))
        {
            *pDstVal = CreateDstUntransferredIntValue(newMin, newMax, instr, src1Val, nullptr);
        }

        if(instr->GetSrc2())
        {
            instr->FreeSrc2();
        }
        return false;
    }

    this->ignoredIntOverflowForCurrentInstr = ignoredIntOverflow;
    this->ignoredNegativeZeroForCurrentInstr = ignoredNegativeZero;

    {
        // Try CSE again before modifying the IR, in case some attributes are required for successful CSE
        Value *src1IndirIndexVal = nullptr;
        Value *src2Val = nullptr;
        if(CSEOptimize(currentBlock, &instr, &src1Val, &src2Val, &src1IndirIndexVal, true /* intMathExprOnly */))
        {
            *redoTypeSpecRef = true;
            return false;
        }
    }

    const Js::OpCode originalOpCode = instr->m_opcode;
    if (!this->IsLoopPrePass())
    {
        // No re-write on prepass
        instr->m_opcode = opcode;
    }

    Value *src1ValueToSpecialize = src1Val;
    if(lossy)
    {
        // Lossy conversions to int32 must be done based on the original source values. For instance, if one of the values is a
        // float constant with a value that fits in a uint32 but not an int32, and the instruction can ignore int overflow, the
        // source value for the purposes of int specialization would have been changed to an int constant value by ignoring
        // overflow. If we were to specialize the sym using the int constant value, it would be treated as a lossless
        // conversion, but since there may be subsequent uses of the same float constant value that may not ignore overflow,
        // this must be treated as a lossy conversion by specializing the sym using the original float constant value.
        src1ValueToSpecialize = src1OriginalVal;
    }

    // Make sure the srcs are specialized
    IR::Opnd *src1 = instr->GetSrc1();
    this->ToInt32(instr, src1, this->currentBlock, src1ValueToSpecialize, nullptr, lossy);

    if(bailOutKind != IR::BailOutInvalid && !this->IsLoopPrePass())
    {
        GenerateBailAtOperation(&instr, bailOutKind);
    }

    if (!skipDst)
    {
        IR::Opnd *dst = instr->GetDst();
        if (dst)
        {
            AssertMsg(!(isTransfer && !this->IsLoopPrePass()) || min == newMin && max == newMax, "If this is just a copy, old/new min/max should be the same");
            TypeSpecializeIntDst(
                instr,
                originalOpCode,
                isTransfer ? src1Val : nullptr,
                src1Val,
                nullptr,
                bailOutKind,
                newMin,
                newMax,
                pDstVal,
                addSubConstantInfo.HasInfo() ? &addSubConstantInfo : nullptr);
        }
    }

    if(bailOutKind == IR::BailOutInvalid)
    {
        GOPT_TRACE(_u("Type specialized to INT\n"));
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::AggressiveIntTypeSpecPhase))
        {
            Output::Print(_u("Type specialized to INT: "));
            Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
        }
#endif
    }
    else
    {
        GOPT_TRACE(_u("Type specialized to INT with bailout on:\n"));
        if(bailOutKind & IR::BailOutOnOverflow)
        {
            GOPT_TRACE(_u("    Overflow\n"));
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::AggressiveIntTypeSpecPhase))
            {
                Output::Print(_u("Type specialized to INT with bailout (%S): "), "Overflow");
                Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
            }
#endif
        }
        if(bailOutKind & IR::BailOutOnNegativeZero)
        {
            GOPT_TRACE(_u("    Zero\n"));
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::AggressiveIntTypeSpecPhase))
            {
                Output::Print(_u("Type specialized to INT with bailout (%S): "), "Zero");
                Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
            }
#endif
        }
    }

    return true;
}

void
GlobOpt::TypeSpecializeIntDst(IR::Instr* instr, Js::OpCode originalOpCode, Value* valToTransfer, Value *const src1Value, Value *const src2Value, const IR::BailOutKind bailOutKind, int32 newMin, int32 newMax, Value** pDstVal, const AddSubConstantInfo *const addSubConstantInfo)
{
    this->TypeSpecializeIntDst(instr, originalOpCode, valToTransfer, src1Value, src2Value, bailOutKind, ValueType::GetInt(IntConstantBounds(newMin, newMax).IsLikelyTaggable()), newMin, newMax, pDstVal, addSubConstantInfo);
}

void
GlobOpt::TypeSpecializeIntDst(IR::Instr* instr, Js::OpCode originalOpCode, Value* valToTransfer, Value *const src1Value, Value *const src2Value, const IR::BailOutKind bailOutKind, ValueType valueType, Value** pDstVal, const AddSubConstantInfo *const addSubConstantInfo)
{
    this->TypeSpecializeIntDst(instr, originalOpCode, valToTransfer, src1Value, src2Value, bailOutKind, valueType, 0, 0, pDstVal, addSubConstantInfo);
}

void
GlobOpt::TypeSpecializeIntDst(IR::Instr* instr, Js::OpCode originalOpCode, Value* valToTransfer, Value *const src1Value, Value *const src2Value, const IR::BailOutKind bailOutKind, ValueType valueType, int32 newMin, int32 newMax, Value** pDstVal, const AddSubConstantInfo *const addSubConstantInfo)
{
    Assert(valueType.IsInt() || (valueType.IsNumber() && valueType.IsLikelyInt() && newMin == 0 && newMax == 0));
    Assert(!valToTransfer || valToTransfer == src1Value);
    Assert(!addSubConstantInfo || addSubConstantInfo->HasInfo());

    IR::Opnd *dst = instr->GetDst();
    Assert(dst);

    bool isValueInfoPrecise;
    if(IsLoopPrePass())
    {
        valueType = GetPrepassValueTypeForDst(valueType, instr, src1Value, src2Value, &isValueInfoPrecise);
    }
    else
    {
        isValueInfoPrecise = true;
    }

    // If dst has a circular reference in a loop, it probably won't get specialized. Don't mark the dst as type-specialized on
    // the pre-pass. With aggressive int spec though, it will take care of bailing out if necessary so there's no need to assume
    // that the dst will be a var even if it's live on the back-edge. Also if the op always produces an int32, then there's no
    // ambiguity in the dst's value type even in the prepass.
    if (!DoAggressiveIntTypeSpec() && this->IsLoopPrePass() && !valueType.IsInt())
    {
        if (dst->IsRegOpnd())
        {
            this->ToVarRegOpnd(dst->AsRegOpnd(), this->currentBlock);
        }
        return;
    }

    const IntBounds *dstBounds = nullptr;
    if(addSubConstantInfo && !addSubConstantInfo->SrcValueIsLikelyConstant() && DoTrackRelativeIntBounds())
    {
        Assert(!ignoredIntOverflowForCurrentInstr);

        // Track bounds for add or sub with a constant. For instance, consider (b = a + 2). The value of 'b' should track that
        // it is equal to (the value of 'a') + 2. Additionally, the value of 'b' should inherit the bounds of 'a', offset by
        // the constant value.
        if(!valueType.IsInt() || !isValueInfoPrecise)
        {
            newMin = INT32_MIN;
            newMax = INT32_MAX;
        }
        dstBounds =
            IntBounds::Add(
                addSubConstantInfo->SrcValue(),
                addSubConstantInfo->Offset(),
                isValueInfoPrecise,
                IntConstantBounds(newMin, newMax),
                alloc);
    }

    // Src1's value could change later in the loop, so the value wouldn't be the same for each
    // iteration.  Since we don't iterate over loops "while (!changed)", go conservative on the
    // pre-pass.
    if (valToTransfer)
    {
        // If this is just a copy, no need for creating a new value.
        Assert(!addSubConstantInfo);
        *pDstVal = this->ValueNumberTransferDst(instr, valToTransfer);
        CurrentBlockData()->InsertNewValue(*pDstVal, dst);
    }
    else if (valueType.IsInt() && isValueInfoPrecise)
    {
        bool wasNegativeZeroPreventedByBailout = false;
        if(newMin <= 0 && newMax >= 0)
        {
            switch(originalOpCode)
            {
                case Js::OpCode::Add_A:
                    // -0 + -0 == -0
                    Assert(src1Value);
                    Assert(src2Value);
                    wasNegativeZeroPreventedByBailout =
                        src1Value->GetValueInfo()->WasNegativeZeroPreventedByBailout() &&
                        src2Value->GetValueInfo()->WasNegativeZeroPreventedByBailout();
                    break;

                case Js::OpCode::Sub_A:
                    // -0 - 0 == -0
                    Assert(src1Value);
                    wasNegativeZeroPreventedByBailout = src1Value->GetValueInfo()->WasNegativeZeroPreventedByBailout();
                    break;

                case Js::OpCode::Neg_A:
                case Js::OpCode::Mul_A:
                case Js::OpCode::Div_A:
                case Js::OpCode::Rem_A:
                    wasNegativeZeroPreventedByBailout = !!(bailOutKind & IR::BailOutOnNegativeZero);
                    break;
            }
        }

        *pDstVal =
            dstBounds
                ? NewIntBoundedValue(valueType, dstBounds, wasNegativeZeroPreventedByBailout, nullptr)
                : NewIntRangeValue(newMin, newMax, wasNegativeZeroPreventedByBailout, nullptr);
    }
    else
    {
        *pDstVal = dstBounds ? NewIntBoundedValue(valueType, dstBounds, false, nullptr) : NewGenericValue(valueType);
    }

    if(addSubConstantInfo || updateInductionVariableValueNumber)
    {
        TrackIntSpecializedAddSubConstant(instr, addSubConstantInfo, *pDstVal, !!dstBounds);
    }

    CurrentBlockData()->SetValue(*pDstVal, dst);

    AssertMsg(dst->IsRegOpnd(), "What else?");
    this->ToInt32Dst(instr, dst->AsRegOpnd(), this->currentBlock);
}

bool
GlobOpt::TypeSpecializeBinary(IR::Instr **pInstr, Value **pSrc1Val, Value **pSrc2Val, Value **pDstVal, Value *const src1OriginalVal, Value *const src2OriginalVal, bool *redoTypeSpecRef)
{
    IR::Instr *&instr = *pInstr;
    int32 min1 = INT32_MIN, max1 = INT32_MAX, min2 = INT32_MIN, max2 = INT32_MAX, newMin, newMax, tmp;
    Js::OpCode opcode;
    Value *&src1Val = *pSrc1Val;
    Value *&src2Val = *pSrc2Val;

    // We don't need to do typespec for asmjs
    if (IsTypeSpecPhaseOff(this->func) || GetIsAsmJSFunc())
    {
        return false;
    }

    if (OpCodeAttr::IsInlineBuiltIn(instr->m_opcode))
    {
        this->TypeSpecializeInlineBuiltInBinary(pInstr, src1Val, src2Val, pDstVal, src1OriginalVal, src2OriginalVal);
        return true;
    }

    if (src1Val)
    {
        src1Val->GetValueInfo()->GetIntValMinMax(&min1, &max1, this->DoAggressiveIntTypeSpec());
    }
    if (src2Val)
    {
        src2Val->GetValueInfo()->GetIntValMinMax(&min2, &max2, this->DoAggressiveIntTypeSpec());
    }

    // Type specialize binary operators to int32

    bool src1Lossy = true;
    bool src2Lossy = true;
    IR::BailOutKind bailOutKind = IR::BailOutInvalid;
    bool ignoredIntOverflow = this->ignoredIntOverflowForCurrentInstr;
    bool ignoredNegativeZero = false;
    bool skipSrc2 = false;
    bool skipDst = false;
    bool needsBoolConv = false;
    AddSubConstantInfo addSubConstantInfo;

    switch (instr->m_opcode)
    {
    case Js::OpCode::Or_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        this->PropagateIntRangeBinary(instr, min1, max1, min2, max2, &newMin, &newMax);
        opcode = Js::OpCode::Or_I4;
        break;

    case Js::OpCode::And_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        this->PropagateIntRangeBinary(instr, min1, max1, min2, max2, &newMin, &newMax);
        opcode = Js::OpCode::And_I4;
        break;

    case Js::OpCode::Xor_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        this->PropagateIntRangeBinary(instr, min1, max1, min2, max2, &newMin, &newMax);
        opcode = Js::OpCode::Xor_I4;
        break;

    case Js::OpCode::Shl_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        this->PropagateIntRangeBinary(instr, min1, max1, min2, max2, &newMin, &newMax);
        opcode = Js::OpCode::Shl_I4;
        break;

    case Js::OpCode::Shr_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        this->PropagateIntRangeBinary(instr, min1, max1, min2, max2, &newMin, &newMax);
        opcode = Js::OpCode::Shr_I4;
        break;

    case Js::OpCode::ShrU_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        if (min1 < 0 && IntConstantBounds(min2, max2).And_0x1f().Contains(0))
        {
            // Src1 may be too large to represent as a signed int32, and src2 may be zero. Unless the resulting value is only
            // used as a signed int32 (hence allowing us to ignore the result's sign), don't specialize the instruction.
            if (!instr->ignoreIntOverflow)
                return false;
            ignoredIntOverflow = true;
        }
        this->PropagateIntRangeBinary(instr, min1, max1, min2, max2, &newMin, &newMax);
        opcode = Js::OpCode::ShrU_I4;
        break;

    case Js::OpCode::BrUnLe_A:
        // Folding the branch based on bounds will attempt a lossless int32 conversion of the sources if they are not definitely
        // int already, so require that both sources are likely int for folding.
        if (DoConstFold() &&
            !IsLoopPrePass() &&
            TryOptConstFoldBrUnsignedGreaterThan(instr, false, src1Val, min1, max1, src2Val, min2, max2))
        {
            return true;
        }

        if (min1 >= 0 && min2 >= 0)
        {
            // Only handle positive values since this is unsigned...

            // Bounds are tracked only for likely int values. Only likely int values may have bounds that are not the defaults
            // (INT32_MIN, INT32_MAX), so we're good.
            Assert(src1Val);
            Assert(src1Val->GetValueInfo()->IsLikelyInt());
            Assert(src2Val);
            Assert(src2Val->GetValueInfo()->IsLikelyInt());

            UpdateIntBoundsForLessThanOrEqualBranch(src1Val, src2Val);
        }

        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = newMax = 0;
        opcode = Js::OpCode::BrUnLe_I4;
        break;

    case Js::OpCode::BrUnLt_A:
        // Folding the branch based on bounds will attempt a lossless int32 conversion of the sources if they are not definitely
        // int already, so require that both sources are likely int for folding.
        if (DoConstFold() &&
            !IsLoopPrePass() &&
            TryOptConstFoldBrUnsignedLessThan(instr, true, src1Val, min1, max1, src2Val, min2, max2))
        {
            return true;
        }

        if (min1 >= 0 && min2 >= 0)
        {
            // Only handle positive values since this is unsigned...

            // Bounds are tracked only for likely int values. Only likely int values may have bounds that are not the defaults
            // (INT32_MIN, INT32_MAX), so we're good.
            Assert(src1Val);
            Assert(src1Val->GetValueInfo()->IsLikelyInt());
            Assert(src2Val);
            Assert(src2Val->GetValueInfo()->IsLikelyInt());

            UpdateIntBoundsForLessThanBranch(src1Val, src2Val);
        }

        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = newMax = 0;
        opcode = Js::OpCode::BrUnLt_I4;
        break;

    case Js::OpCode::BrUnGe_A:
        // Folding the branch based on bounds will attempt a lossless int32 conversion of the sources if they are not definitely
        // int already, so require that both sources are likely int for folding.
        if (DoConstFold() &&
            !IsLoopPrePass() &&
            TryOptConstFoldBrUnsignedLessThan(instr, false, src1Val, min1, max1, src2Val, min2, max2))
        {
            return true;
        }

        if (min1 >= 0 && min2 >= 0)
        {
            // Only handle positive values since this is unsigned...

            // Bounds are tracked only for likely int values. Only likely int values may have bounds that are not the defaults
            // (INT32_MIN, INT32_MAX), so we're good.
            Assert(src1Val);
            Assert(src1Val->GetValueInfo()->IsLikelyInt());
            Assert(src2Val);
            Assert(src2Val->GetValueInfo()->IsLikelyInt());

            UpdateIntBoundsForGreaterThanOrEqualBranch(src1Val, src2Val);
        }

        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = newMax = 0;
        opcode = Js::OpCode::BrUnGe_I4;
        break;

    case Js::OpCode::BrUnGt_A:
        // Folding the branch based on bounds will attempt a lossless int32 conversion of the sources if they are not definitely
        // int already, so require that both sources are likely int for folding.
        if (DoConstFold() &&
            !IsLoopPrePass() &&
            TryOptConstFoldBrUnsignedGreaterThan(instr, true, src1Val, min1, max1, src2Val, min2, max2))
        {
            return true;
        }

        if (min1 >= 0 && min2 >= 0)
        {
            // Only handle positive values since this is unsigned...

            // Bounds are tracked only for likely int values. Only likely int values may have bounds that are not the defaults
            // (INT32_MIN, INT32_MAX), so we're good.
            Assert(src1Val);
            Assert(src1Val->GetValueInfo()->IsLikelyInt());
            Assert(src2Val);
            Assert(src2Val->GetValueInfo()->IsLikelyInt());

            UpdateIntBoundsForGreaterThanBranch(src1Val, src2Val);
        }

        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = newMax = 0;
        opcode = Js::OpCode::BrUnGt_I4;
        break;

    case Js::OpCode::CmUnLe_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = 0;
        newMax = 1;
        opcode = Js::OpCode::CmUnLe_I4;
        needsBoolConv = true;
        break;

    case Js::OpCode::CmUnLt_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = 0;
        newMax = 1;
        opcode = Js::OpCode::CmUnLt_I4;
        needsBoolConv = true;
        break;

    case Js::OpCode::CmUnGe_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = 0;
        newMax = 1;
        opcode = Js::OpCode::CmUnGe_I4;
        needsBoolConv = true;
        break;

    case Js::OpCode::CmUnGt_A:
        if (!DoLossyIntTypeSpec())
        {
            return false;
        }
        newMin = 0;
        newMax = 1;
        opcode = Js::OpCode::CmUnGt_I4;
        needsBoolConv = true;
        break;

    case Js::OpCode::Expo_A:
        {
            src1Val = src1OriginalVal;
            src2Val = src2OriginalVal;
            return this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
        }

    case Js::OpCode::Div_A:
        {
            ValueType specializedValueType = GetDivValueType(instr, src1Val, src2Val, true);
            if (specializedValueType.IsFloat())
            {
                // Either result is float or 1/x  or cst1/cst2 where cst1%cst2 != 0
                // Note: We should really constant fold cst1%cst2...
                src1Val = src1OriginalVal;
                src2Val = src2OriginalVal;
                return this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
            }
#ifdef _M_ARM
            if (!AutoSystemInfo::Data.ArmDivAvailable())
            {
                return false;
            }
#endif
            if (specializedValueType.IsInt())
            {
                if (max2 == 0x80000000 || (min2 == 0 && max2 == 00))
                {
                    return false;
                }

                if (min1 == 0x80000000 && min2 <= -1 && max2 >= -1)
                {
                    // Prevent integer overflow, as div by zero or MIN_INT / -1 will throw an exception
                    // Or we know we are dividing by zero (which is weird to have because the profile data
                    // say we got an int)
                    bailOutKind = IR::BailOutOnDivOfMinInt;
                }

                src1Lossy = false;          // Detect -0 on the sources
                src2Lossy = false;

                opcode = Js::OpCode::Div_I4;
                Assert(!instr->GetSrc1()->IsUnsigned());
                bailOutKind |= IR::BailOnDivResultNotInt;
                if (max2 >= 0 && min2 <= 0)
                {
                    // Need to check for divide by zero if the denominator range includes 0
                    bailOutKind |= IR::BailOutOnDivByZero;
                }


                if (max1 >= 0 && min1 <= 0)
                {
                    // Numerator contains 0 so the result contains 0
                    newMin = 0;
                    newMax = 0;

                    if (min2 < 0)
                    {
                        // Denominator may be negative, so the result could be negative 0
                        if (instr->ShouldCheckForNegativeZero())
                        {
                            bailOutKind |= IR::BailOutOnNegativeZero;
                        }
                        else
                        {
                            ignoredNegativeZero = true;
                        }
                    }
                }
                else
                {
                    // Initialize to invalid value, one of the condition below will update it correctly
                    newMin = INT_MAX;
                    newMax = INT_MIN;
                }

                // Deal with the positive and negative range separately for both the numerator and the denominator,
                // and integrate to the overall min and max.

                // If the result is positive (positive/positive or negative/negative):
                //      The min should be the  smallest magnitude numerator   (positive_Min1 | negative_Max1)
                //                divided by   ---------------------------------------------------------------
                //                             largest magnitude denominator  (positive_Max2 | negative_Min2)
                //
                //      The max should be the  largest magnitude numerator    (positive_Max1 | negative_Max1)
                //                divided by   ---------------------------------------------------------------
                //                             smallest magnitude denominator (positive_Min2 | negative_Max2)

                // If the result is negative (positive/negative or positive/negative):
                //      The min should be the  largest magnitude numerator    (positive_Max1 | negative_Min1)
                //                divided by   ---------------------------------------------------------------
                //                             smallest magnitude denominator (negative_Max2 | positive_Min2)
                //
                //      The max should be the  smallest magnitude numerator   (positive_Min1 | negative_Max1)
                //                divided by   ---------------------------------------------------------------
                //                             largest magnitude denominator  (negative_Min2 | positive_Max2)

                // Consider: The range can be slightly more precise if we take care of the rounding

                if (max1 > 0)
                {
                    // Take only the positive numerator range
                    int32 positive_Min1 = max(1, min1);
                    int32 positive_Max1 = max1;
                    if (max2 > 0)
                    {
                        // Take only the positive denominator range
                        int32 positive_Min2 = max(1, min2);
                        int32 positive_Max2 = max2;

                        // Positive / Positive
                        int32 quadrant1_Min = positive_Min1 <= positive_Max2? 1 : positive_Min1 / positive_Max2;
                        int32 quadrant1_Max = positive_Max1 <= positive_Min2? 1 : positive_Max1 / positive_Min2;

                        Assert(1 <= quadrant1_Min && quadrant1_Min <= quadrant1_Max);

                        // The result should positive
                        newMin = min(newMin, quadrant1_Min);
                        newMax = max(newMax, quadrant1_Max);
                    }

                    if (min2 < 0)
                    {
                        // Take only the negative denominator range
                        int32 negative_Min2 = min2;
                        int32 negative_Max2 = min(-1, max2);

                        // Positive / Negative
                        int32 quadrant2_Min = -positive_Max1 >= negative_Max2? -1 : positive_Max1 / negative_Max2;
                        int32 quadrant2_Max = -positive_Min1 >= negative_Min2? -1 : positive_Min1 / negative_Min2;

                        // The result should negative
                        Assert(quadrant2_Min <= quadrant2_Max && quadrant2_Max <= -1);

                        newMin = min(newMin, quadrant2_Min);
                        newMax = max(newMax, quadrant2_Max);
                    }
                }
                if (min1 < 0)
                {
                    // Take only the native numerator range
                    int32 negative_Min1 = min1;
                    int32 negative_Max1 = min(-1, max1);

                    if (max2 > 0)
                    {
                        // Take only the positive denominator range
                        int32 positive_Min2 = max(1, min2);
                        int32 positive_Max2 = max2;

                        // Negative / Positive
                        int32 quadrant4_Min = negative_Min1 >= -positive_Min2? -1 : negative_Min1 / positive_Min2;
                        int32 quadrant4_Max = negative_Max1 >= -positive_Max2? -1 : negative_Max1 / positive_Max2;

                        // The result should negative
                        Assert(quadrant4_Min <= quadrant4_Max && quadrant4_Max <= -1);

                        newMin = min(newMin, quadrant4_Min);
                        newMax = max(newMax, quadrant4_Max);
                    }

                    if (min2 < 0)
                    {

                        // Take only the negative denominator range
                        int32 negative_Min2 = min2;
                        int32 negative_Max2 = min(-1, max2);

                        int32 quadrant3_Min;
                        int32 quadrant3_Max;
                        // Negative / Negative
                        if (negative_Max1 == 0x80000000 && negative_Min2 == -1)
                        {
                            quadrant3_Min = negative_Max1 >= negative_Min2? 1 : (negative_Max1+1) / negative_Min2;
                        }
                        else
                        {
                            quadrant3_Min = negative_Max1 >= negative_Min2? 1 : negative_Max1 / negative_Min2;
                        }
                        if (negative_Min1 == 0x80000000 && negative_Max2 == -1)
                        {
                            quadrant3_Max = negative_Min1 >= negative_Max2? 1 : (negative_Min1+1) / negative_Max2;
                        }
                        else
                        {
                            quadrant3_Max = negative_Min1 >= negative_Max2? 1 : negative_Min1 / negative_Max2;
                        }

                        // The result should positive
                        Assert(1 <= quadrant3_Min && quadrant3_Min <= quadrant3_Max);

                        newMin = min(newMin, quadrant3_Min);
                        newMax = max(newMax, quadrant3_Max);
                    }
                }
                Assert(newMin <= newMax);
                // Continue to int type spec
                break;
            }
        }
        // fall-through
    default:
        {
            const bool involesLargeInt32 =
                (src1Val && src1Val->GetValueInfo()->IsLikelyUntaggedInt()) ||
                (src2Val && src2Val->GetValueInfo()->IsLikelyUntaggedInt());
            const auto trySpecializeToFloat =
                [&](const bool mayOverflow) -> bool
            {
                // It has been determined that this instruction cannot be int-specialized. Need to determine whether to attempt
                // to float-specialize the instruction, or leave it unspecialized.
                if((involesLargeInt32
#if INT32VAR
                    && mayOverflow
#endif
                   ) || (instr->m_opcode == Js::OpCode::Mul_A && !this->DoAggressiveMulIntTypeSpec())
                  )
                {
                    // An input range is completely outside the range of an int31 and the operation is likely to overflow.
                    // Additionally, on 32-bit platforms, the value is untaggable and will be a JavascriptNumber, which is
                    // significantly slower to use in an unspecialized operation compared to a tagged int. So, try to
                    // float-specialize the instruction.
                    src1Val = src1OriginalVal;
                    src2Val = src2OriginalVal;
                    return TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
                }
                return false;
            };

            if (instr->m_opcode != Js::OpCode::ArgOut_A_InlineBuiltIn)
            {
                if ((src1Val && src1Val->GetValueInfo()->IsLikelyFloat()) || (src2Val && src2Val->GetValueInfo()->IsLikelyFloat()))
                {
                    // Try to type specialize to float
                    src1Val = src1OriginalVal;
                    src2Val = src2OriginalVal;
                    return this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
                }

                if (src1Val == nullptr ||
                    src2Val == nullptr ||
                    !src1Val->GetValueInfo()->IsLikelyInt() ||
                    !src2Val->GetValueInfo()->IsLikelyInt() ||
                    (
                        !DoAggressiveIntTypeSpec() &&
                        (
                            !(src1Val->GetValueInfo()->IsInt() || CurrentBlockData()->IsSwitchInt32TypeSpecialized(instr)) ||
                            !src2Val->GetValueInfo()->IsInt()
                        )
                    ) ||
                    (instr->GetSrc1()->IsRegOpnd() && instr->GetSrc1()->AsRegOpnd()->m_sym->m_isNotInt) ||
                    (instr->GetSrc2()->IsRegOpnd() && instr->GetSrc2()->AsRegOpnd()->m_sym->m_isNotInt))
                {
                    return trySpecializeToFloat(true);
                }
            }

            // Try to type specialize to int32

            // If one of the values is a float constant with a value that fits in a uint32 but not an int32,
            // and the instruction can ignore int overflow, the source value for the purposes of int specialization
            // would have been changed to an int constant value by ignoring overflow. But, the conversion is still lossy.
            if (!(src1OriginalVal && src1OriginalVal->GetValueInfo()->IsFloatConstant() && src1Val && src1Val->GetValueInfo()->HasIntConstantValue()))
            {
                src1Lossy = false;
            }
            if (!(src2OriginalVal && src2OriginalVal->GetValueInfo()->IsFloatConstant() && src2Val && src2Val->GetValueInfo()->HasIntConstantValue()))
            {
                src2Lossy = false;
            }

            switch(instr->m_opcode)
            {
            case Js::OpCode::ArgOut_A_InlineBuiltIn:
                // If the src is already type-specialized, if we don't type-specialize ArgOut_A_InlineBuiltIn instr, we'll get additional ToVar.
                //   So, to avoid that, type-specialize the ArgOut_A_InlineBuiltIn instr.
                // Else we don't need to type-specialize the instr, we are fine with src being Var.
                if (instr->GetSrc1()->IsRegOpnd())
                {
                    StackSym *sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
                    if (CurrentBlockData()->IsInt32TypeSpecialized(sym))
                    {
                        opcode = instr->m_opcode;
                        skipDst = true;                 // We should keep dst as is, otherwise the link opnd for next ArgOut/InlineBuiltInStart would be broken.
                        skipSrc2 = true;                // src2 is linkOpnd. We don't need to type-specialize it.
                        newMin = min1; newMax = max1;   // Values don't matter, these are unused.
                        goto LOutsideSwitch;            // Continue to int-type-specialize.
                    }
                    else if (CurrentBlockData()->IsFloat64TypeSpecialized(sym))
                    {
                        src1Val = src1OriginalVal;
                        src2Val = src2OriginalVal;
                        return this->TypeSpecializeFloatBinary(instr, src1Val, src2Val, pDstVal);
                    }
#ifdef ENABLE_SIMDJS
                    else if (CurrentBlockData()->IsSimd128F4TypeSpecialized(sym))
                    {
                        // SIMD_JS
                        // We should be already using the SIMD type-spec sym. See TypeSpecializeSimd128.
                        Assert(IRType_IsSimd128(instr->GetSrc1()->GetType()));
                    }
#endif
                }
                return false;

            case Js::OpCode::Add_A:
                do // while(false)
                {
                    const auto CannotOverflowBasedOnRelativeBounds = [&](int32 *const constantValueRef)
                    {
                        Assert(constantValueRef);

                        if(min2 == max2 &&
                            src1Val->GetValueInfo()->IsIntBounded() &&
                            src1Val->GetValueInfo()->AsIntBounded()->Bounds()->AddCannotOverflowBasedOnRelativeBounds(min2))
                        {
                            *constantValueRef = min2;
                            return true;
                        }
                        else if(
                            min1 == max1 &&
                            src2Val->GetValueInfo()->IsIntBounded() &&
                            src2Val->GetValueInfo()->AsIntBounded()->Bounds()->AddCannotOverflowBasedOnRelativeBounds(min1))
                        {
                            *constantValueRef = min1;
                            return true;
                        }
                        return false;
                    };

                    if (Int32Math::Add(min1, min2, &newMin))
                    {
                        int32 constantSrcValue;
                        if(CannotOverflowBasedOnRelativeBounds(&constantSrcValue))
                        {
                            newMin = constantSrcValue >= 0 ? INT32_MAX : INT32_MIN;
                        }
                        else if(instr->ShouldCheckForIntOverflow())
                        {
                            if(involesLargeInt32 || !DoAggressiveIntTypeSpec())
                            {
                                // May overflow
                                return trySpecializeToFloat(true);
                            }
                            bailOutKind |= IR::BailOutOnOverflow;
                            newMin = min1 < 0 ? INT32_MIN : INT32_MAX;
                        }
                        else
                        {
                            // When ignoring overflow, the range needs to account for overflow. For any Add or Sub, since
                            // overflow causes the value to wrap around, and we don't have a way to specify a lower and upper
                            // range of ints, we use the full range of int32s.
                            ignoredIntOverflow = true;
                            newMin = INT32_MIN;
                            newMax = INT32_MAX;
                            break;
                        }
                    }
                    if (Int32Math::Add(max1, max2, &newMax))
                    {
                        int32 constantSrcValue;
                        if(CannotOverflowBasedOnRelativeBounds(&constantSrcValue))
                        {
                            newMax = constantSrcValue >= 0 ? INT32_MAX : INT32_MIN;
                        }
                        else if(instr->ShouldCheckForIntOverflow())
                        {
                            if(involesLargeInt32 || !DoAggressiveIntTypeSpec())
                            {
                                // May overflow
                                return trySpecializeToFloat(true);
                            }
                            bailOutKind |= IR::BailOutOnOverflow;
                            newMax = max1 < 0 ? INT32_MIN : INT32_MAX;
                        }
                        else
                        {
                            // See comment about ignoring overflow above
                            ignoredIntOverflow = true;
                            newMin = INT32_MIN;
                            newMax = INT32_MAX;
                            break;
                        }
                    }
                    if(bailOutKind & IR::BailOutOnOverflow)
                    {
                        Assert(bailOutKind == IR::BailOutOnOverflow);
                        Assert(instr->ShouldCheckForIntOverflow());
                        int32 temp;
                        if(Int32Math::Add(
                            Int32Math::NearestInRangeTo(0, min1, max1),
                            Int32Math::NearestInRangeTo(0, min2, max2),
                            &temp))
                        {
                            // Always overflows
                            return trySpecializeToFloat(true);
                        }
                    }
                } while(false);

                if (!this->IsLoopPrePass() && newMin == newMax && bailOutKind == IR::BailOutInvalid)
                {
                    // Take care of Add with zero here, since we know we're dealing with 2 numbers.
                    this->CaptureByteCodeSymUses(instr);
                    IR::Opnd *src;
                    bool isAddZero = true;
                    int32 intConstantValue;
                    if (src1Val->GetValueInfo()->TryGetIntConstantValue(&intConstantValue) && intConstantValue == 0)
                    {
                        src = instr->UnlinkSrc2();
                        instr->FreeSrc1();
                    }
                    else if (src2Val->GetValueInfo()->TryGetIntConstantValue(&intConstantValue) && intConstantValue == 0)
                    {
                        src = instr->UnlinkSrc1();
                        instr->FreeSrc2();
                    }
                    else
                    {
                        // This should have been handled by const folding, unless:
                        // - A source's value was substituted with a different value here, which is after const folding happened
                        // - A value is not definitely int, but once converted to definite int, it would be zero due to a
                        //   condition in the source code such as if(a === 0). Ideally, we would specialize the sources and
                        //   remove the add, but doesn't seem too important for now.
                        Assert(
                            !DoConstFold() ||
                            src1Val != src1OriginalVal ||
                            src2Val != src2OriginalVal ||
                            !src1Val->GetValueInfo()->IsInt() ||
                            !src2Val->GetValueInfo()->IsInt());
                        isAddZero = false;
                        src = nullptr;
                    }
                    if (isAddZero)
                    {
                        IR::Instr *newInstr = IR::Instr::New(Js::OpCode::Ld_A, instr->UnlinkDst(), src, instr->m_func);
                        newInstr->SetByteCodeOffset(instr);

                        instr->m_opcode = Js::OpCode::Nop;
                        this->currentBlock->InsertInstrAfter(newInstr, instr);
                        return true;
                    }
                }

                if(!ignoredIntOverflow)
                {
                    if(min2 == max2 &&
                        (!IsLoopPrePass() || IsPrepassSrcValueInfoPrecise(instr->GetSrc2(), src2Val)) &&
                        instr->GetSrc1()->IsRegOpnd())
                    {
                        addSubConstantInfo.Set(instr->GetSrc1()->AsRegOpnd()->m_sym, src1Val, min1 == max1, min2);
                    }
                    else if(
                        min1 == max1 &&
                        (!IsLoopPrePass() || IsPrepassSrcValueInfoPrecise(instr->GetSrc1(), src1Val)) &&
                        instr->GetSrc2()->IsRegOpnd())
                    {
                        addSubConstantInfo.Set(instr->GetSrc2()->AsRegOpnd()->m_sym, src2Val, min2 == max2, min1);
                    }
                }

                opcode = Js::OpCode::Add_I4;
                break;

            case Js::OpCode::Sub_A:
                do // while(false)
                {
                    const auto CannotOverflowBasedOnRelativeBounds = [&]()
                    {
                        return
                            min2 == max2 &&
                            src1Val->GetValueInfo()->IsIntBounded() &&
                            src1Val->GetValueInfo()->AsIntBounded()->Bounds()->SubCannotOverflowBasedOnRelativeBounds(min2);
                    };

                    if (Int32Math::Sub(min1, max2, &newMin))
                    {
                        if(CannotOverflowBasedOnRelativeBounds())
                        {
                            Assert(min2 == max2);
                            newMin = min2 >= 0 ? INT32_MIN : INT32_MAX;
                        }
                        else if(instr->ShouldCheckForIntOverflow())
                        {
                            if(involesLargeInt32 || !DoAggressiveIntTypeSpec())
                            {
                                // May overflow
                                return trySpecializeToFloat(true);
                            }
                            bailOutKind |= IR::BailOutOnOverflow;
                            newMin = min1 < 0 ? INT32_MIN : INT32_MAX;
                        }
                        else
                        {
                            // When ignoring overflow, the range needs to account for overflow. For any Add or Sub, since overflow
                            // causes the value to wrap around, and we don't have a way to specify a lower and upper range of ints,
                            // we use the full range of int32s.
                            ignoredIntOverflow = true;
                            newMin = INT32_MIN;
                            newMax = INT32_MAX;
                            break;
                        }
                    }
                    if (Int32Math::Sub(max1, min2, &newMax))
                    {
                        if(CannotOverflowBasedOnRelativeBounds())
                        {
                            Assert(min2 == max2);
                            newMax = min2 >= 0 ? INT32_MIN: INT32_MAX;
                        }
                        else if(instr->ShouldCheckForIntOverflow())
                        {
                            if(involesLargeInt32 || !DoAggressiveIntTypeSpec())
                            {
                                // May overflow
                                return trySpecializeToFloat(true);
                            }
                            bailOutKind |= IR::BailOutOnOverflow;
                            newMax = max1 < 0 ? INT32_MIN : INT32_MAX;
                        }
                        else
                        {
                            // See comment about ignoring overflow above
                            ignoredIntOverflow = true;
                            newMin = INT32_MIN;
                            newMax = INT32_MAX;
                            break;
                        }
                    }
                    if(bailOutKind & IR::BailOutOnOverflow)
                    {
                        Assert(bailOutKind == IR::BailOutOnOverflow);
                        Assert(instr->ShouldCheckForIntOverflow());
                        int32 temp;
                        if(Int32Math::Sub(
                            Int32Math::NearestInRangeTo(-1, min1, max1),
                            Int32Math::NearestInRangeTo(0, min2, max2),
                            &temp))
                        {
                            // Always overflows
                            return trySpecializeToFloat(true);
                        }
                    }
                } while(false);

                if(!ignoredIntOverflow &&
                    min2 == max2 &&
                    min2 != INT32_MIN &&
                    (!IsLoopPrePass() || IsPrepassSrcValueInfoPrecise(instr->GetSrc2(), src2Val)) &&
                    instr->GetSrc1()->IsRegOpnd())
                {
                    addSubConstantInfo.Set(instr->GetSrc1()->AsRegOpnd()->m_sym, src1Val, min1 == max1, -min2);
                }

                opcode = Js::OpCode::Sub_I4;
                break;

            case Js::OpCode::Mul_A:
            {
                if (Int32Math::Mul(min1, min2, &newMin))
                {
                    if (involesLargeInt32 || !DoAggressiveMulIntTypeSpec() || !DoAggressiveIntTypeSpec())
                    {
                        // May overflow
                        return trySpecializeToFloat(true);
                    }
                    bailOutKind |= IR::BailOutOnMulOverflow;
                    newMin = (min1 < 0) ^ (min2 < 0) ? INT32_MIN : INT32_MAX;
                }
                newMax = newMin;
                if (Int32Math::Mul(max1, max2, &tmp))
                {
                    if (involesLargeInt32 || !DoAggressiveMulIntTypeSpec() || !DoAggressiveIntTypeSpec())
                    {
                        // May overflow
                        return trySpecializeToFloat(true);
                    }
                    bailOutKind |= IR::BailOutOnMulOverflow;
                    tmp = (max1 < 0) ^ (max2 < 0) ? INT32_MIN : INT32_MAX;
                }
                newMin = min(newMin, tmp);
                newMax = max(newMax, tmp);
                if (Int32Math::Mul(min1, max2, &tmp))
                {
                    if (involesLargeInt32 || !DoAggressiveMulIntTypeSpec() || !DoAggressiveIntTypeSpec())
                    {
                        // May overflow
                        return trySpecializeToFloat(true);
                    }
                    bailOutKind |= IR::BailOutOnMulOverflow;
                    tmp = (min1 < 0) ^ (max2 < 0) ? INT32_MIN : INT32_MAX;
                }
                newMin = min(newMin, tmp);
                newMax = max(newMax, tmp);
                if (Int32Math::Mul(max1, min2, &tmp))
                {
                    if (involesLargeInt32 || !DoAggressiveMulIntTypeSpec() || !DoAggressiveIntTypeSpec())
                    {
                        // May overflow
                        return trySpecializeToFloat(true);
                    }
                    bailOutKind |= IR::BailOutOnMulOverflow;
                    tmp = (max1 < 0) ^ (min2 < 0) ? INT32_MIN : INT32_MAX;
                }
                newMin = min(newMin, tmp);
                newMax = max(newMax, tmp);
                if (bailOutKind & IR::BailOutOnMulOverflow)
                {
                    // CSE only if two MULs have the same overflow check behavior.
                    // Currently this is set to be ignore int32 overflow, but not 53-bit, or int32 overflow matters.
                    if (!instr->ShouldCheckFor32BitOverflow() && instr->ShouldCheckForNon32BitOverflow())
                    {
                        // If we allow int to overflow then there can be anything in the resulting int
                        newMin = IntConstMin;
                        newMax = IntConstMax;
                        ignoredIntOverflow = true;
                    }

                    int32 temp, overflowValue;
                    if (Int32Math::Mul(
                        Int32Math::NearestInRangeTo(0, min1, max1),
                        Int32Math::NearestInRangeTo(0, min2, max2),
                        &temp,
                        &overflowValue))
                    {
                        Assert(instr->ignoreOverflowBitCount >= 32);
                        int overflowMatters = 64 - instr->ignoreOverflowBitCount;
                        if (!ignoredIntOverflow ||
                            // Use shift to check high bits in case its negative
                            ((overflowValue << overflowMatters) >> overflowMatters) != overflowValue
                            )
                        {
                            // Always overflows
                            return trySpecializeToFloat(true);
                        }
                    }

                }

                if (newMin <= 0 && newMax >= 0 &&                   // New range crosses zero
                    (min1 < 0 || min2 < 0) &&                       // An operand's range contains a negative integer
                    !(min1 > 0 || min2 > 0) &&                      // Neither operand's range contains only positive integers
                    !instr->GetSrc1()->IsEqual(instr->GetSrc2()))   // The operands don't have the same value
                {
                    if (instr->ShouldCheckForNegativeZero())
                    {
                        // -0 matters since the sym is not a local, or is used in a way in which -0 would differ from +0
                        if (!DoAggressiveIntTypeSpec())
                        {
                            // May result in -0
                            return trySpecializeToFloat(false);
                        }
                        if (((min1 == 0 && max1 == 0) || (min2 == 0 && max2 == 0)) && (max1 < 0 || max2 < 0))
                        {
                            // Always results in -0
                            return trySpecializeToFloat(false);
                        }
                        bailOutKind |= IR::BailOutOnNegativeZero;
                    }
                    else
                    {
                        ignoredNegativeZero = true;
                    }
                }
                opcode = Js::OpCode::Mul_I4;
                break;
            }
            case Js::OpCode::Rem_A:
            {
                IR::Opnd* src2 = instr->GetSrc2();
                if (!this->IsLoopPrePass() && min2 == max2 && min1 >= 0)
                {
                    int32 value = min2;

                    if (value == (1 << Math::Log2(value)) && src2->IsAddrOpnd())
                    {
                        Assert(src2->AsAddrOpnd()->IsVar());
                        instr->m_opcode = Js::OpCode::And_A;
                        src2->AsAddrOpnd()->SetAddress(Js::TaggedInt::ToVarUnchecked(value - 1),
                            IR::AddrOpndKindConstantVar);
                        *pSrc2Val = GetIntConstantValue(value - 1, instr);
                        src2Val = *pSrc2Val;
                        return this->TypeSpecializeBinary(&instr, pSrc1Val, pSrc2Val, pDstVal, src1OriginalVal, src2Val, redoTypeSpecRef);
                    }
                }
#ifdef _M_ARM
                if (!AutoSystemInfo::Data.ArmDivAvailable())
                {
                    return false;
                }
#endif
                if (min1 < 0)
                {
                    // The most negative it can be is min1, unless limited by min2/max2
                    int32 negMaxAbs2;
                    if (min2 == INT32_MIN)
                    {
                        negMaxAbs2 = INT32_MIN;
                    }
                    else
                    {
                        negMaxAbs2 = -max(abs(min2), abs(max2)) + 1;
                    }
                    newMin = max(min1, negMaxAbs2);
                }
                else
                {
                    newMin = 0;
                }

                bool isModByPowerOf2 = (instr->IsProfiledInstr() && instr->m_func->HasProfileInfo() &&
                    instr->m_func->GetReadOnlyProfileInfo()->IsModulusOpByPowerOf2(static_cast<Js::ProfileId>(instr->AsProfiledInstr()->u.profileId)));

                if(isModByPowerOf2)
                {
                    Assert(bailOutKind == IR::BailOutInvalid);
                    bailOutKind = IR::BailOnModByPowerOf2;
                    newMin = 0;
                }
                else
                {
                    if (min2 <= 0 && max2 >= 0)
                    {
                        // Consider: We could handle the zero case with a check and bailout...
                        return false;
                    }

                    if (min1 == 0x80000000 && (min2 <= -1 && max2 >= -1))
                    {
                        // Prevent integer overflow, as div by zero or MIN_INT / -1 will throw an exception
                        return false;
                    }
                    if (min1 < 0)
                    {
                        if(instr->ShouldCheckForNegativeZero())
                        {
                            if (!DoAggressiveIntTypeSpec())
                            {
                                return false;
                            }
                            bailOutKind |= IR::BailOutOnNegativeZero;
                        }
                        else
                        {
                            ignoredNegativeZero = true;
                        }
                    }
                }
                {
                    int32 absMax2;
                    if (min2 == INT32_MIN)
                    {
                        // abs(INT32_MIN) == INT32_MAX because of overflow
                        absMax2 = INT32_MAX;
                    }
                    else
                    {
                        absMax2 = max(abs(min2), abs(max2)) - 1;
                    }
                    newMax = min(absMax2, max(max1, 0));
                    newMax = max(newMin, newMax);
                }
                opcode = Js::OpCode::Rem_I4;
                Assert(!instr->GetSrc1()->IsUnsigned());
                break;
            }

            case Js::OpCode::CmEq_A:
            case Js::OpCode::CmSrEq_A:
                if (!IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val))
                {
                    return false;
                }
                newMin = 0;
                newMax = 1;
                opcode = Js::OpCode::CmEq_I4;
                needsBoolConv = true;
                break;

            case Js::OpCode::CmNeq_A:
            case Js::OpCode::CmSrNeq_A:
                if (!IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val))
                {
                    return false;
                }
                newMin = 0;
                newMax = 1;
                opcode = Js::OpCode::CmNeq_I4;
                needsBoolConv = true;
                break;

            case Js::OpCode::CmLe_A:
                if (!IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val))
                {
                    return false;
                }
                newMin = 0;
                newMax = 1;
                opcode = Js::OpCode::CmLe_I4;
                needsBoolConv = true;
                break;

            case Js::OpCode::CmLt_A:
                if (!IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val))
                {
                    return false;
                }
                newMin = 0;
                newMax = 1;
                opcode = Js::OpCode::CmLt_I4;
                needsBoolConv = true;
                break;

            case Js::OpCode::CmGe_A:
                if (!IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val))
                {
                    return false;
                }
                newMin = 0;
                newMax = 1;
                opcode = Js::OpCode::CmGe_I4;
                needsBoolConv = true;
                break;

            case Js::OpCode::CmGt_A:
                if (!IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val))
                {
                    return false;
                }
                newMin = 0;
                newMax = 1;
                opcode = Js::OpCode::CmGt_I4;
                needsBoolConv = true;
                break;

            case Js::OpCode::BrSrEq_A:
            case Js::OpCode::BrEq_A:
            case Js::OpCode::BrNotNeq_A:
            case Js::OpCode::BrSrNotNeq_A:
            {
                if(DoConstFold() &&
                    !IsLoopPrePass() &&
                    TryOptConstFoldBrEqual(instr, true, src1Val, min1, max1, src2Val, min2, max2))
                {
                    return true;
                }

                const bool specialize = IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val);
                UpdateIntBoundsForEqualBranch(src1Val, src2Val);
                if(!specialize)
                {
                    return false;
                }

                opcode = Js::OpCode::BrEq_I4;
                // We'll get a warning if we don't assign a value to these...
                // We'll assert if we use them and make a range where min > max
                newMin = 2; newMax = 1;
                break;
            }

            case Js::OpCode::BrSrNeq_A:
            case Js::OpCode::BrNeq_A:
            case Js::OpCode::BrSrNotEq_A:
            case Js::OpCode::BrNotEq_A:
            {
                if(DoConstFold() &&
                    !IsLoopPrePass() &&
                    TryOptConstFoldBrEqual(instr, false, src1Val, min1, max1, src2Val, min2, max2))
                {
                    return true;
                }

                const bool specialize = IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val);
                UpdateIntBoundsForNotEqualBranch(src1Val, src2Val);
                if(!specialize)
                {
                    return false;
                }

                opcode = Js::OpCode::BrNeq_I4;
                // We'll get a warning if we don't assign a value to these...
                // We'll assert if we use them and make a range where min > max
                newMin = 2; newMax = 1;
                break;
            }

            case Js::OpCode::BrGt_A:
            case Js::OpCode::BrNotLe_A:
            {
                if(DoConstFold() &&
                    !IsLoopPrePass() &&
                    TryOptConstFoldBrGreaterThan(instr, true, src1Val, min1, max1, src2Val, min2, max2))
                {
                    return true;
                }

                const bool specialize = IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val);
                UpdateIntBoundsForGreaterThanBranch(src1Val, src2Val);
                if(!specialize)
                {
                    return false;
                }

                opcode = Js::OpCode::BrGt_I4;
                // We'll get a warning if we don't assign a value to these...
                // We'll assert if we use them and make a range where min > max
                newMin = 2; newMax = 1;
                break;
            }

            case Js::OpCode::BrGe_A:
            case Js::OpCode::BrNotLt_A:
            {
                if(DoConstFold() &&
                    !IsLoopPrePass() &&
                    TryOptConstFoldBrGreaterThanOrEqual(instr, true, src1Val, min1, max1, src2Val, min2, max2))
                {
                    return true;
                }

                const bool specialize = IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val);
                UpdateIntBoundsForGreaterThanOrEqualBranch(src1Val, src2Val);
                if(!specialize)
                {
                    return false;
                }

                opcode = Js::OpCode::BrGe_I4;
                // We'll get a warning if we don't assign a value to these...
                // We'll assert if we use them and make a range where min > max
                newMin = 2; newMax = 1;
                break;
            }

            case Js::OpCode::BrLt_A:
            case Js::OpCode::BrNotGe_A:
            {
                if(DoConstFold() &&
                    !IsLoopPrePass() &&
                    TryOptConstFoldBrGreaterThanOrEqual(instr, false, src1Val, min1, max1, src2Val, min2, max2))
                {
                    return true;
                }

                const bool specialize = IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val);
                UpdateIntBoundsForLessThanBranch(src1Val, src2Val);
                if(!specialize)
                {
                    return false;
                }

                opcode = Js::OpCode::BrLt_I4;
                // We'll get a warning if we don't assign a value to these...
                // We'll assert if we use them and make a range where min > max
                newMin = 2; newMax = 1;
                break;
            }

            case Js::OpCode::BrLe_A:
            case Js::OpCode::BrNotGt_A:
            {
                if(DoConstFold() &&
                    !IsLoopPrePass() &&
                    TryOptConstFoldBrGreaterThan(instr, false, src1Val, min1, max1, src2Val, min2, max2))
                {
                    return true;
                }

                const bool specialize = IsWorthSpecializingToInt32Branch(instr, src1Val, src2Val);
                UpdateIntBoundsForLessThanOrEqualBranch(src1Val, src2Val);
                if(!specialize)
                {
                    return false;
                }

                opcode = Js::OpCode::BrLe_I4;
                // We'll get a warning if we don't assign a value to these...
                // We'll assert if we use them and make a range where min > max
                newMin = 2; newMax = 1;
                break;
            }

            default:
                return false;
            }

            // If this instruction is in a range of instructions where int overflow does not matter, we will still specialize it
            // (won't leave it unspecialized based on heuristics), since it is most likely worth specializing, and the dst value
            // needs to be guaranteed to be an int
            if(!ignoredIntOverflow &&
                !ignoredNegativeZero &&
                !needsBoolConv &&
                instr->ShouldCheckForIntOverflow() &&
                !IsWorthSpecializingToInt32(instr, src1Val, src2Val))
            {
                // Even though type specialization is being skipped since it may not be worth it, the proper value should still be
                // maintained so that the result may be type specialized later. An int value is not created for the dst in any of
                // the following cases.
                // - A bailout check is necessary to specialize this instruction. The bailout check is what guarantees the result to
                //   be an int, but since we're not going to specialize this instruction, there won't be a bailout check.
                // - Aggressive int type specialization is disabled and we're in a loop prepass. We're conservative on dst values in
                //   that case, especially if the dst sym is live on the back-edge.
                if(bailOutKind == IR::BailOutInvalid &&
                    instr->GetDst() &&
                    src1Val->GetValueInfo()->IsInt() &&
                    src2Val->GetValueInfo()->IsInt() &&
                    (DoAggressiveIntTypeSpec() || !this->IsLoopPrePass()))
                {
                    *pDstVal = CreateDstUntransferredIntValue(newMin, newMax, instr, src1Val, src2Val);
                }
                return false;
            }
        } // case default
    } // switch

LOutsideSwitch:
    this->ignoredIntOverflowForCurrentInstr = ignoredIntOverflow;
    this->ignoredNegativeZeroForCurrentInstr = ignoredNegativeZero;

    {
        // Try CSE again before modifying the IR, in case some attributes are required for successful CSE
        Value *src1IndirIndexVal = nullptr;
        if(CSEOptimize(currentBlock, &instr, &src1Val, &src2Val, &src1IndirIndexVal, true /* intMathExprOnly */))
        {
            *redoTypeSpecRef = true;
            return false;
        }
    }

    const Js::OpCode originalOpCode = instr->m_opcode;
    if (!this->IsLoopPrePass())
    {
        // No re-write on prepass
        instr->m_opcode = opcode;
    }

    Value *src1ValueToSpecialize = src1Val, *src2ValueToSpecialize = src2Val;
    // Lossy conversions to int32 must be done based on the original source values. For instance, if one of the values is a
    // float constant with a value that fits in a uint32 but not an int32, and the instruction can ignore int overflow, the
    // source value for the purposes of int specialization would have been changed to an int constant value by ignoring
    // overflow. If we were to specialize the sym using the int constant value, it would be treated as a lossless
    // conversion, but since there may be subsequent uses of the same float constant value that may not ignore overflow,
    // this must be treated as a lossy conversion by specializing the sym using the original float constant value.
    if(src1Lossy)
    {
        src1ValueToSpecialize = src1OriginalVal;
    }
    if (src2Lossy)
    {
        src2ValueToSpecialize = src2OriginalVal;
    }

    // Make sure the srcs are specialized
    IR::Opnd* src1 = instr->GetSrc1();
    this->ToInt32(instr, src1, this->currentBlock, src1ValueToSpecialize, nullptr, src1Lossy);

    if (!skipSrc2)
    {
        IR::Opnd* src2 = instr->GetSrc2();
        this->ToInt32(instr, src2, this->currentBlock, src2ValueToSpecialize, nullptr, src2Lossy);
    }

    if(bailOutKind != IR::BailOutInvalid && !this->IsLoopPrePass())
    {
        GenerateBailAtOperation(&instr, bailOutKind);
    }

    if (!skipDst && instr->GetDst())
    {
        if (needsBoolConv)
        {
            IR::RegOpnd *varDst;
            if (this->IsLoopPrePass())
            {
                varDst = instr->GetDst()->AsRegOpnd();
                this->ToVarRegOpnd(varDst, this->currentBlock);
            }
            else
            {
                // Generate:
                // t1.i = CmCC t2.i, t3.i
                // t1.v = Conv_bool t1.i
                //
                // If the only uses of t1 are ints, the conv_bool will get dead-stored

                TypeSpecializeIntDst(instr, originalOpCode, nullptr, src1Val, src2Val, bailOutKind, newMin, newMax, pDstVal);
                IR::RegOpnd *intDst = instr->GetDst()->AsRegOpnd();
                intDst->SetIsJITOptimizedReg(true);

                varDst = IR::RegOpnd::New(intDst->m_sym->GetVarEquivSym(this->func), TyVar, this->func);
                IR::Instr *convBoolInstr = IR::Instr::New(Js::OpCode::Conv_Bool, varDst, intDst, this->func);
                // In some cases (e.g. unsigned compare peep code), a comparison will use variables
                // other than the ones initially intended for it, if we can determine that we would
                // arrive at the same result. This means that we get a ByteCodeUses operation after
                // the actual comparison. Since Inserting the Conv_bool just after the compare, and
                // just before the ByteCodeUses, would cause issues later on with register lifetime
                // calculation, we want to insert the Conv_bool after the whole compare instruction
                // block.
                IR::Instr *putAfter = instr;
                while (putAfter->m_next && putAfter->m_next->m_opcode == Js::OpCode::ByteCodeUses)
                {
                    putAfter = putAfter->m_next;
                }
                putAfter->InsertAfter(convBoolInstr);
                convBoolInstr->SetByteCodeOffset(instr);

                this->ToVarRegOpnd(varDst, this->currentBlock);
                CurrentBlockData()->liveInt32Syms->Set(varDst->m_sym->m_id);
                CurrentBlockData()->liveLossyInt32Syms->Set(varDst->m_sym->m_id);
            }
            *pDstVal = this->NewGenericValue(ValueType::Boolean, varDst);
        }
        else
        {
            TypeSpecializeIntDst(
                instr,
                originalOpCode,
                nullptr,
                src1Val,
                src2Val,
                bailOutKind,
                newMin,
                newMax,
                pDstVal,
                addSubConstantInfo.HasInfo() ? &addSubConstantInfo : nullptr);
        }
    }

    if(bailOutKind == IR::BailOutInvalid)
    {
        GOPT_TRACE(_u("Type specialized to INT\n"));
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::AggressiveIntTypeSpecPhase))
        {
            Output::Print(_u("Type specialized to INT: "));
            Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
        }
#endif
    }
    else
    {
        GOPT_TRACE(_u("Type specialized to INT with bailout on:\n"));
        if(bailOutKind & (IR::BailOutOnOverflow | IR::BailOutOnMulOverflow) )
        {
            GOPT_TRACE(_u("    Overflow\n"));
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::AggressiveIntTypeSpecPhase))
            {
                Output::Print(_u("Type specialized to INT with bailout (%S): "), "Overflow");
                Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
            }
#endif
        }
        if(bailOutKind & IR::BailOutOnNegativeZero)
        {
            GOPT_TRACE(_u("    Zero\n"));
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::AggressiveIntTypeSpecPhase))
            {
                Output::Print(_u("Type specialized to INT with bailout (%S): "), "Zero");
                Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
            }
#endif
        }
    }

    return true;
}

bool
GlobOpt::IsWorthSpecializingToInt32Branch(IR::Instr const * instr, Value const * src1Val, Value const * src2Val) const
{
    if (!src1Val->GetValueInfo()->HasIntConstantValue() && instr->GetSrc1()->IsRegOpnd())
    {
        StackSym const *sym1 = instr->GetSrc1()->AsRegOpnd()->m_sym;
        if (CurrentBlockData()->IsInt32TypeSpecialized(sym1) == false)
        {
            if (!src2Val->GetValueInfo()->HasIntConstantValue() && instr->GetSrc2()->IsRegOpnd())
            {
                StackSym const *sym2 = instr->GetSrc2()->AsRegOpnd()->m_sym;
                if (CurrentBlockData()->IsInt32TypeSpecialized(sym2) == false)
                {
                    // Type specializing a Br itself isn't worth it, unless one src
                    // is already type specialized
                    return false;
                }
            }
        }
    }
    return true;
}

bool
GlobOpt::TryOptConstFoldBrFalse(
    IR::Instr *const instr,
    Value *const srcValue,
    const int32 min,
    const int32 max)
{
    Assert(instr);
    Assert(instr->m_opcode == Js::OpCode::BrFalse_A || instr->m_opcode == Js::OpCode::BrTrue_A);
    Assert(srcValue);

    if(!(DoAggressiveIntTypeSpec() ? srcValue->GetValueInfo()->IsLikelyInt() : srcValue->GetValueInfo()->IsInt()))
    {
        return false;
    }
    if(ValueInfo::IsEqualTo(srcValue, min, max, nullptr, 0, 0))
    {
        OptConstFoldBr(instr->m_opcode == Js::OpCode::BrFalse_A, instr, srcValue);
        return true;
    }
    if(ValueInfo::IsNotEqualTo(srcValue, min, max, nullptr, 0, 0))
    {
        OptConstFoldBr(instr->m_opcode == Js::OpCode::BrTrue_A, instr, srcValue);
        return true;
    }
    return false;
}

bool
GlobOpt::TryOptConstFoldBrEqual(
    IR::Instr *const instr,
    const bool branchOnEqual,
    Value *const src1Value,
    const int32 min1,
    const int32 max1,
    Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    Assert(instr);
    Assert(src1Value);
    Assert(DoAggressiveIntTypeSpec() ? src1Value->GetValueInfo()->IsLikelyInt() : src1Value->GetValueInfo()->IsInt());
    Assert(src2Value);
    Assert(DoAggressiveIntTypeSpec() ? src2Value->GetValueInfo()->IsLikelyInt() : src2Value->GetValueInfo()->IsInt());

    if(ValueInfo::IsEqualTo(src1Value, min1, max1, src2Value, min2, max2))
    {
        OptConstFoldBr(branchOnEqual, instr, src1Value, src2Value);
        return true;
    }
    if(ValueInfo::IsNotEqualTo(src1Value, min1, max1, src2Value, min2, max2))
    {
        OptConstFoldBr(!branchOnEqual, instr, src1Value, src2Value);
        return true;
    }
    return false;
}

bool
GlobOpt::TryOptConstFoldBrGreaterThan(
    IR::Instr *const instr,
    const bool branchOnGreaterThan,
    Value *const src1Value,
    const int32 min1,
    const int32 max1,
    Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    Assert(instr);
    Assert(src1Value);
    Assert(DoAggressiveIntTypeSpec() ? src1Value->GetValueInfo()->IsLikelyInt() : src1Value->GetValueInfo()->IsInt());
    Assert(src2Value);
    Assert(DoAggressiveIntTypeSpec() ? src2Value->GetValueInfo()->IsLikelyInt() : src2Value->GetValueInfo()->IsInt());

    if(ValueInfo::IsGreaterThan(src1Value, min1, max1, src2Value, min2, max2))
    {
        OptConstFoldBr(branchOnGreaterThan, instr, src1Value, src2Value);
        return true;
    }
    if(ValueInfo::IsLessThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2))
    {
        OptConstFoldBr(!branchOnGreaterThan, instr, src1Value, src2Value);
        return true;
    }
    return false;
}

bool
GlobOpt::TryOptConstFoldBrGreaterThanOrEqual(
    IR::Instr *const instr,
    const bool branchOnGreaterThanOrEqual,
    Value *const src1Value,
    const int32 min1,
    const int32 max1,
    Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    Assert(instr);
    Assert(src1Value);
    Assert(DoAggressiveIntTypeSpec() ? src1Value->GetValueInfo()->IsLikelyInt() : src1Value->GetValueInfo()->IsInt());
    Assert(src2Value);
    Assert(DoAggressiveIntTypeSpec() ? src2Value->GetValueInfo()->IsLikelyInt() : src2Value->GetValueInfo()->IsInt());

    if(ValueInfo::IsGreaterThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2))
    {
        OptConstFoldBr(branchOnGreaterThanOrEqual, instr, src1Value, src2Value);
        return true;
    }
    if(ValueInfo::IsLessThan(src1Value, min1, max1, src2Value, min2, max2))
    {
        OptConstFoldBr(!branchOnGreaterThanOrEqual, instr, src1Value, src2Value);
        return true;
    }
    return false;
}

bool
GlobOpt::TryOptConstFoldBrUnsignedLessThan(
    IR::Instr *const instr,
    const bool branchOnLessThan,
    Value *const src1Value,
    const int32 min1,
    const int32 max1,
    Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    Assert(DoConstFold());
    Assert(!IsLoopPrePass());

    if(!src1Value ||
        !src2Value ||
        !(
            DoAggressiveIntTypeSpec()
                ? src1Value->GetValueInfo()->IsLikelyInt() && src2Value->GetValueInfo()->IsLikelyInt()
                : src1Value->GetValueInfo()->IsInt() && src2Value->GetValueInfo()->IsInt()
        ))
    {
        return false;
    }

    uint uMin1 = (min1 < 0 ? (max1 < 0 ? min((uint)min1, (uint)max1) : 0) : min1);
    uint uMax1 = max((uint)min1, (uint)max1);
    uint uMin2 = (min2 < 0 ? (max2 < 0 ? min((uint)min2, (uint)max2) : 0) : min2);
    uint uMax2 = max((uint)min2, (uint)max2);

    if (uMax1 < uMin2)
    {
        // Range 1 is always lesser than Range 2
        OptConstFoldBr(branchOnLessThan, instr, src1Value, src2Value);
        return true;
    }
    if (uMin1 >= uMax2)
    {
        // Range 2 is always lesser than Range 1
        OptConstFoldBr(!branchOnLessThan, instr, src1Value, src2Value);
        return true;
    }
    return false;
}

bool
GlobOpt::TryOptConstFoldBrUnsignedGreaterThan(
    IR::Instr *const instr,
    const bool branchOnGreaterThan,
    Value *const src1Value,
    const int32 min1,
    const int32 max1,
    Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    Assert(DoConstFold());
    Assert(!IsLoopPrePass());

    if(!src1Value ||
        !src2Value ||
        !(
            DoAggressiveIntTypeSpec()
                ? src1Value->GetValueInfo()->IsLikelyInt() && src2Value->GetValueInfo()->IsLikelyInt()
                : src1Value->GetValueInfo()->IsInt() && src2Value->GetValueInfo()->IsInt()
        ))
    {
        return false;
    }

    uint uMin1 = (min1 < 0 ? (max1 < 0 ? min((uint)min1, (uint)max1) : 0) : min1);
    uint uMax1 = max((uint)min1, (uint)max1);
    uint uMin2 = (min2 < 0 ? (max2 < 0 ? min((uint)min2, (uint)max2) : 0) : min2);
    uint uMax2 = max((uint)min2, (uint)max2);

    if (uMin1 > uMax2)
    {
        // Range 1 is always greater than Range 2
        OptConstFoldBr(branchOnGreaterThan, instr, src1Value, src2Value);
        return true;
    }
    if (uMax1 <= uMin2)
    {
        // Range 2 is always greater than Range 1
        OptConstFoldBr(!branchOnGreaterThan, instr, src1Value, src2Value);
        return true;
    }
    return false;
}

void
GlobOpt::SetPathDependentInfo(const bool conditionToBranch, const PathDependentInfo &info)
{
    Assert(this->currentBlock->GetSuccList()->Count() == 2);
    IR::Instr * fallthrough = this->currentBlock->GetNext()->GetFirstInstr();
    FOREACH_SLISTBASECOUNTED_ENTRY(FlowEdge*, edge, this->currentBlock->GetSuccList())
    {
        if (conditionToBranch == (edge->GetSucc()->GetFirstInstr() != fallthrough))
        {
            edge->SetPathDependentInfo(info, alloc);
            return;
        }
    }
    NEXT_SLISTBASECOUNTED_ENTRY;

    Assert(false);
}

PathDependentInfoToRestore
GlobOpt::UpdatePathDependentInfo(PathDependentInfo *const info)
{
    Assert(info);

    if(!info->HasInfo())
    {
        return PathDependentInfoToRestore();
    }

    decltype(&GlobOpt::UpdateIntBoundsForEqual) UpdateIntBoundsForLeftValue, UpdateIntBoundsForRightValue;
    switch(info->Relationship())
    {
        case PathDependentRelationship::Equal:
            UpdateIntBoundsForLeftValue = &GlobOpt::UpdateIntBoundsForEqual;
            UpdateIntBoundsForRightValue = &GlobOpt::UpdateIntBoundsForEqual;
            break;

        case PathDependentRelationship::NotEqual:
            UpdateIntBoundsForLeftValue = &GlobOpt::UpdateIntBoundsForNotEqual;
            UpdateIntBoundsForRightValue = &GlobOpt::UpdateIntBoundsForNotEqual;
            break;

        case PathDependentRelationship::GreaterThanOrEqual:
            UpdateIntBoundsForLeftValue = &GlobOpt::UpdateIntBoundsForGreaterThanOrEqual;
            UpdateIntBoundsForRightValue = &GlobOpt::UpdateIntBoundsForLessThanOrEqual;
            break;

        case PathDependentRelationship::GreaterThan:
            UpdateIntBoundsForLeftValue = &GlobOpt::UpdateIntBoundsForGreaterThan;
            UpdateIntBoundsForRightValue = &GlobOpt::UpdateIntBoundsForLessThan;
            break;

        case PathDependentRelationship::LessThanOrEqual:
            UpdateIntBoundsForLeftValue = &GlobOpt::UpdateIntBoundsForLessThanOrEqual;
            UpdateIntBoundsForRightValue = &GlobOpt::UpdateIntBoundsForGreaterThanOrEqual;
            break;

        case PathDependentRelationship::LessThan:
            UpdateIntBoundsForLeftValue = &GlobOpt::UpdateIntBoundsForLessThan;
            UpdateIntBoundsForRightValue = &GlobOpt::UpdateIntBoundsForGreaterThan;
            break;

        default:
            Assert(false);
            __assume(false);
    }

    ValueInfo *leftValueInfo = info->LeftValue()->GetValueInfo();
    IntConstantBounds leftConstantBounds;
    AssertVerify(leftValueInfo->TryGetIntConstantBounds(&leftConstantBounds, true));

    ValueInfo *rightValueInfo;
    IntConstantBounds rightConstantBounds;
    if(info->RightValue())
    {
        rightValueInfo = info->RightValue()->GetValueInfo();
        AssertVerify(rightValueInfo->TryGetIntConstantBounds(&rightConstantBounds, true));
    }
    else
    {
        rightValueInfo = nullptr;
        rightConstantBounds = IntConstantBounds(info->RightConstantValue(), info->RightConstantValue());
    }

    ValueInfo *const newLeftValueInfo =
        (this->*UpdateIntBoundsForLeftValue)(
            info->LeftValue(),
            leftConstantBounds,
            info->RightValue(),
            rightConstantBounds,
            true);
    if(newLeftValueInfo)
    {
        ChangeValueInfo(nullptr, info->LeftValue(), newLeftValueInfo);
        AssertVerify(newLeftValueInfo->TryGetIntConstantBounds(&leftConstantBounds, true));
    }
    else
    {
        leftValueInfo = nullptr;
    }

    ValueInfo *const newRightValueInfo =
        (this->*UpdateIntBoundsForRightValue)(
            info->RightValue(),
            rightConstantBounds,
            info->LeftValue(),
            leftConstantBounds,
            true);
    if(newRightValueInfo)
    {
        ChangeValueInfo(nullptr, info->RightValue(), newRightValueInfo);
    }
    else
    {
        rightValueInfo = nullptr;
    }

    return PathDependentInfoToRestore(leftValueInfo, rightValueInfo);
}

void
GlobOpt::RestorePathDependentInfo(PathDependentInfo *const info, const PathDependentInfoToRestore infoToRestore)
{
    Assert(info);

    if(infoToRestore.LeftValueInfo())
    {
        Assert(info->LeftValue());
        ChangeValueInfo(nullptr, info->LeftValue(), infoToRestore.LeftValueInfo());
    }

    if(infoToRestore.RightValueInfo())
    {
        Assert(info->RightValue());
        ChangeValueInfo(nullptr, info->RightValue(), infoToRestore.RightValueInfo());
    }
}

bool
GlobOpt::TypeSpecializeFloatUnary(IR::Instr **pInstr, Value *src1Val, Value **pDstVal, bool skipDst /* = false */)
{
    IR::Instr *&instr = *pInstr;
    IR::Opnd *src1;
    IR::Opnd *dst;
    Js::OpCode opcode = instr->m_opcode;
    Value *valueToTransfer = nullptr;

    Assert(src1Val && src1Val->GetValueInfo()->IsLikelyNumber() || OpCodeAttr::IsInlineBuiltIn(instr->m_opcode));

    if (!this->DoFloatTypeSpec())
    {
        return false;
    }

    // For inline built-ins we need to do type specialization. Check upfront to avoid duplicating same case labels.
    if (!OpCodeAttr::IsInlineBuiltIn(instr->m_opcode))
    {
        switch (opcode)
        {
        case Js::OpCode::ArgOut_A_InlineBuiltIn:
            skipDst = true;
            // fall-through

        case Js::OpCode::Ld_A:
        case Js::OpCode::BrTrue_A:
        case Js::OpCode::BrFalse_A:
            if (instr->GetSrc1()->IsRegOpnd())
            {
                StackSym *sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
                if (CurrentBlockData()->IsFloat64TypeSpecialized(sym) == false)
                {
                    // Type specializing an Ld_A isn't worth it, unless the src
                    // is already type specialized
                    return false;
                }
            }
            if (instr->m_opcode == Js::OpCode::Ld_A)
            {
                valueToTransfer = src1Val;
            }
            break;
        case Js::OpCode::Neg_A:
            break;

        case Js::OpCode::Conv_Num:
            Assert(src1Val);
            opcode = Js::OpCode::Ld_A;
            valueToTransfer = src1Val;
            if (!src1Val->GetValueInfo()->IsNumber())
            {
                StackSym *sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
                valueToTransfer = NewGenericValue(ValueType::Float, instr->GetDst()->GetStackSym());
                if (CurrentBlockData()->IsFloat64TypeSpecialized(sym) == false)
                {
                    // Set the dst as a nonDeadStore. We want to keep the Ld_A to prevent the FromVar from
                    // being dead-stored, as it could cause implicit calls.
                    dst = instr->GetDst();
                    dst->AsRegOpnd()->m_dontDeadStore = true;
                }
            }
            break;

        case Js::OpCode::StElemI_A:
        case Js::OpCode::StElemI_A_Strict:
        case Js::OpCode::StElemC:
            return TypeSpecializeStElem(pInstr, src1Val, pDstVal);

        default:
            return false;
        }
    }

    // Make sure the srcs are specialized
    src1 = instr->GetSrc1();

    // Use original val when calling toFloat64 as this is what we'll use to try hoisting the fromVar if we're in a loop.
    this->ToFloat64(instr, src1, this->currentBlock, src1Val, nullptr, IR::BailOutPrimitiveButString);

    if (!skipDst)
    {
        dst = instr->GetDst();
        if (dst)
        {
            this->TypeSpecializeFloatDst(instr, valueToTransfer, src1Val, nullptr, pDstVal);
            if (!this->IsLoopPrePass())
            {
                instr->m_opcode = opcode;
            }
        }
    }

    GOPT_TRACE_INSTR(instr, _u("Type specialized to FLOAT: "));

#if ENABLE_DEBUG_CONFIG_OPTIONS
    if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::FloatTypeSpecPhase))
    {
        Output::Print(_u("Type specialized to FLOAT: "));
        Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
    }
#endif

    return true;
}

// Unconditionally type-spec dst to float.
void
GlobOpt::TypeSpecializeFloatDst(IR::Instr *instr, Value *valToTransfer, Value *const src1Value, Value *const src2Value, Value **pDstVal)
{
    IR::Opnd* dst = instr->GetDst();
    Assert(dst);

    AssertMsg(dst->IsRegOpnd(), "What else?");
    this->ToFloat64Dst(instr, dst->AsRegOpnd(), this->currentBlock);

    if(valToTransfer)
    {
        *pDstVal = this->ValueNumberTransferDst(instr, valToTransfer);
        CurrentBlockData()->InsertNewValue(*pDstVal, dst);
    }
    else
    {
        *pDstVal = CreateDstUntransferredValue(ValueType::Float, instr, src1Value, src2Value);
    }
}

#ifdef ENABLE_SIMDJS
void
GlobOpt::TypeSpecializeSimd128Dst(IRType type, IR::Instr *instr, Value *valToTransfer, Value *const src1Value, Value **pDstVal)
{
    IR::Opnd* dst = instr->GetDst();
    Assert(dst);

    AssertMsg(dst->IsRegOpnd(), "What else?");
    this->ToSimd128Dst(type, instr, dst->AsRegOpnd(), this->currentBlock);

    if (valToTransfer)
    {
        *pDstVal = this->ValueNumberTransferDst(instr, valToTransfer);
        CurrentBlockData()->InsertNewValue(*pDstVal, dst);
    }
    else
    {
        *pDstVal = NewGenericValue(GetValueTypeFromIRType(type), instr->GetDst());
    }
}
#endif

bool
GlobOpt::TypeSpecializeLdLen(
    IR::Instr * *const instrRef,
    Value * *const src1ValueRef,
    Value * *const dstValueRef,
    bool *const forceInvariantHoistingRef)
{
    Assert(instrRef);
    IR::Instr *&instr = *instrRef;
    Assert(instr);
    Assert(instr->m_opcode == Js::OpCode::LdLen_A);
    Assert(src1ValueRef);
    Value *&src1Value = *src1ValueRef;
    Assert(dstValueRef);
    Value *&dstValue = *dstValueRef;
    Assert(forceInvariantHoistingRef);
    bool &forceInvariantHoisting = *forceInvariantHoistingRef;

    if(!DoLdLenIntSpec(instr, instr->GetSrc1()->GetValueType()))
    {
        return false;
    }

    IR::BailOutKind bailOutKind = IR::BailOutOnIrregularLength;
    if(!IsLoopPrePass())
    {
        IR::RegOpnd *const baseOpnd = instr->GetSrc1()->AsRegOpnd();
        if(baseOpnd->IsArrayRegOpnd())
        {
            StackSym *const lengthSym = baseOpnd->AsArrayRegOpnd()->LengthSym();
            if(lengthSym)
            {
                CaptureByteCodeSymUses(instr);
                instr->m_opcode = Js::OpCode::Ld_I4;
                instr->ReplaceSrc1(IR::RegOpnd::New(lengthSym, lengthSym->GetType(), func));
                instr->ClearBailOutInfo();

                // Find the hoisted length value
                Value *const lengthValue = CurrentBlockData()->FindValue(lengthSym);
                Assert(lengthValue);
                src1Value = lengthValue;
                ValueInfo *const lengthValueInfo = lengthValue->GetValueInfo();
                Assert(lengthValueInfo->GetSymStore() != lengthSym);
                IntConstantBounds lengthConstantBounds;
                AssertVerify(lengthValueInfo->TryGetIntConstantBounds(&lengthConstantBounds));
                Assert(lengthConstantBounds.LowerBound() >= 0);

                // Int-specialize, and transfer the value to the dst
                TypeSpecializeIntDst(
                    instr,
                    Js::OpCode::LdLen_A,
                    src1Value,
                    src1Value,
                    nullptr,
                    bailOutKind,
                    lengthConstantBounds.LowerBound(),
                    lengthConstantBounds.UpperBound(),
                    &dstValue);

                // Try to force hoisting the Ld_I4 so that the length will have an invariant sym store that can be
                // copy-propped. Invariant hoisting does not automatically hoist Ld_I4.
                forceInvariantHoisting = true;
                return true;
            }
        }

        if (instr->HasBailOutInfo())
        {
            Assert(instr->GetBailOutKind() == IR::BailOutMarkTempObject);
            bailOutKind = IR::BailOutOnIrregularLength | IR::BailOutMarkTempObject;
            instr->SetBailOutKind(bailOutKind);
        }
        else
        {
            Assert(bailOutKind == IR::BailOutOnIrregularLength);
            GenerateBailAtOperation(&instr, bailOutKind);
        }
    }

    TypeSpecializeIntDst(
        instr,
        Js::OpCode::LdLen_A,
        nullptr,
        nullptr,
        nullptr,
        bailOutKind,
        0,
        INT32_MAX,
        &dstValue);
    return true;
}

bool
GlobOpt::TypeSpecializeFloatBinary(IR::Instr *instr, Value *src1Val, Value *src2Val, Value **pDstVal)
{
    IR::Opnd *src1;
    IR::Opnd *src2;
    IR::Opnd *dst;
    bool allowUndefinedOrNullSrc1 = true;
    bool allowUndefinedOrNullSrc2 = true;

    bool skipSrc1 = false;
    bool skipSrc2 = false;
    bool skipDst = false;

    if (!this->DoFloatTypeSpec())
    {
        return false;
    }

    // For inline built-ins we need to do type specialization. Check upfront to avoid duplicating same case labels.
    if (!OpCodeAttr::IsInlineBuiltIn(instr->m_opcode))
    {
        switch (instr->m_opcode)
        {
        case Js::OpCode::Sub_A:
        case Js::OpCode::Mul_A:
        case Js::OpCode::Div_A:
        case Js::OpCode::Expo_A:
            // Avoid if one source is known not to be a number.
            if (src1Val->GetValueInfo()->IsNotNumber() || src2Val->GetValueInfo()->IsNotNumber())
            {
                return false;
            }
            break;

        case Js::OpCode::BrSrEq_A:
        case Js::OpCode::BrSrNeq_A:
        case Js::OpCode::BrEq_A:
        case Js::OpCode::BrNeq_A:
        case Js::OpCode::BrSrNotEq_A:
        case Js::OpCode::BrNotEq_A:
        case Js::OpCode::BrSrNotNeq_A:
        case Js::OpCode::BrNotNeq_A:
            // Avoid if one source is known not to be a number.
            if (src1Val->GetValueInfo()->IsNotNumber() || src2Val->GetValueInfo()->IsNotNumber())
            {
                return false;
            }
            // Undef == Undef, but +Undef != +Undef
            // 0.0 != null, but 0.0 == +null
            //
            // So Bailout on anything but numbers for both src1 and src2
            allowUndefinedOrNullSrc1 = false;
            allowUndefinedOrNullSrc2 = false;
            break;

        case Js::OpCode::BrGt_A:
        case Js::OpCode::BrGe_A:
        case Js::OpCode::BrLt_A:
        case Js::OpCode::BrLe_A:
        case Js::OpCode::BrNotGt_A:
        case Js::OpCode::BrNotGe_A:
        case Js::OpCode::BrNotLt_A:
        case Js::OpCode::BrNotLe_A:
            // Avoid if one source is known not to be a number.
            if (src1Val->GetValueInfo()->IsNotNumber() || src2Val->GetValueInfo()->IsNotNumber())
            {
                return false;
            }
            break;

        case Js::OpCode::Add_A:
            // For Add, we need both sources to be Numbers, otherwise it could be a string concat
            if (!src1Val || !src2Val || !(src1Val->GetValueInfo()->IsLikelyNumber() && src2Val->GetValueInfo()->IsLikelyNumber()))
            {
                return false;
            }
            break;

        case Js::OpCode::ArgOut_A_InlineBuiltIn:
            skipSrc2 = true;
            skipDst = true;
            break;

        default:
            return false;
        }
    }
    else
    {
        switch (instr->m_opcode)
        {
            case Js::OpCode::InlineArrayPush:
                bool isFloatConstMissingItem = src2Val->GetValueInfo()->IsFloatConstant();

                if(isFloatConstMissingItem)
                {
                    FloatConstType floatValue = src2Val->GetValueInfo()->AsFloatConstant()->FloatValue();
                    isFloatConstMissingItem = Js::SparseArraySegment<double>::IsMissingItem(&floatValue);
                }
                // Don't specialize if the element is not likelyNumber - we will surely bailout
                if(!(src2Val->GetValueInfo()->IsLikelyNumber()) || isFloatConstMissingItem)
                {
                    return false;
                }
                // Only specialize the Second source - element
                skipSrc1 = true;
                skipDst = true;
                allowUndefinedOrNullSrc2 = false;
                break;
        }
    }

    // Make sure the srcs are specialized
    if(!skipSrc1)
    {
        src1 = instr->GetSrc1();
        this->ToFloat64(instr, src1, this->currentBlock, src1Val, nullptr, (allowUndefinedOrNullSrc1 ? IR::BailOutPrimitiveButString : IR::BailOutNumberOnly));
    }

    if (!skipSrc2)
    {
        src2 = instr->GetSrc2();
        this->ToFloat64(instr, src2, this->currentBlock, src2Val, nullptr, (allowUndefinedOrNullSrc2 ? IR::BailOutPrimitiveButString : IR::BailOutNumberOnly));
    }

    if (!skipDst)
    {
        dst = instr->GetDst();

        if (dst)
        {
            *pDstVal = CreateDstUntransferredValue(ValueType::Float, instr, src1Val, src2Val);

            AssertMsg(dst->IsRegOpnd(), "What else?");
            this->ToFloat64Dst(instr, dst->AsRegOpnd(), this->currentBlock);
        }
    }

    GOPT_TRACE_INSTR(instr, _u("Type specialized to FLOAT: "));

#if ENABLE_DEBUG_CONFIG_OPTIONS
    if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::FloatTypeSpecPhase))
    {
        Output::Print(_u("Type specialized to FLOAT: "));
        Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
    }
#endif

    return true;
}

bool
GlobOpt::TypeSpecializeStElem(IR::Instr ** pInstr, Value *src1Val, Value **pDstVal)
{
    IR::Instr *&instr = *pInstr;

    IR::RegOpnd *baseOpnd = instr->GetDst()->AsIndirOpnd()->GetBaseOpnd();
    ValueType baseValueType(baseOpnd->GetValueType());
    if (instr->DoStackArgsOpt(this->func) ||
        (!this->DoTypedArrayTypeSpec() && baseValueType.IsLikelyOptimizedTypedArray()) ||
        (!this->DoNativeArrayTypeSpec() && baseValueType.IsLikelyNativeArray()) ||
        !(baseValueType.IsLikelyOptimizedTypedArray() || baseValueType.IsLikelyNativeArray()))
    {
        GOPT_TRACE_INSTR(instr, _u("Didn't type specialize array access, because typed array type specialization is disabled, or base is not an optimized typed array.\n"));
        if (PHASE_TRACE(Js::TypedArrayTypeSpecPhase, this->func))
        {
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
            char baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
            baseValueType.ToString(baseValueTypeStr);
            Output::Print(_u("Typed Array Optimization:  function: %s (%s): instr: %s, base value type: %S, did not specialize because %s.\n"),
                this->func->GetJITFunctionBody()->GetDisplayName(),
                this->func->GetDebugNumberSet(debugStringBuffer),
                Js::OpCodeUtil::GetOpCodeName(instr->m_opcode),
                baseValueTypeStr,
                instr->DoStackArgsOpt(this->func) ?
                    _u("instruction uses the arguments object") :
                    _u("typed array type specialization is disabled, or base is not an optimized typed array"));
            Output::Flush();
        }
        return false;
    }

    Assert(instr->GetSrc1()->IsRegOpnd() || (src1Val && src1Val->GetValueInfo()->HasIntConstantValue()));

    StackSym *sym = instr->GetSrc1()->IsRegOpnd() ? instr->GetSrc1()->AsRegOpnd()->m_sym : nullptr;

    // Only type specialize the source of store element if the source symbol is already type specialized to int or float.
    if (sym)
    {
        if (baseValueType.IsLikelyNativeArray())
        {
            // Gently coerce these src's into native if it seems likely to work.
            // Otherwise we can't use the fast path to store.
            // But don't try to put a float-specialized number into an int array this way.
            if (!(
                    CurrentBlockData()->IsInt32TypeSpecialized(sym) ||
                    (
                        src1Val &&
                        (
                            DoAggressiveIntTypeSpec()
                                ? src1Val->GetValueInfo()->IsLikelyInt()
                                : src1Val->GetValueInfo()->IsInt()
                        )
                    )
                ))
            {
                if (!(
                        CurrentBlockData()->IsFloat64TypeSpecialized(sym) ||
                        (src1Val && src1Val->GetValueInfo()->IsLikelyNumber())
                    ) ||
                    baseValueType.HasIntElements())
                {
                    return false;
                }
            }
        }
        else if (!CurrentBlockData()->IsInt32TypeSpecialized(sym) && !CurrentBlockData()->IsFloat64TypeSpecialized(sym))
        {
            GOPT_TRACE_INSTR(instr, _u("Didn't specialize array access, because src is not type specialized.\n"));
            if (PHASE_TRACE(Js::TypedArrayTypeSpecPhase, this->func))
            {
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                char baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
                baseValueType.ToString(baseValueTypeStr);
                Output::Print(_u("Typed Array Optimization:  function: %s (%s): instr: %s, base value type: %S, did not specialize because src is not specialized.\n"),
                              this->func->GetJITFunctionBody()->GetDisplayName(),
                              this->func->GetDebugNumberSet(debugStringBuffer),
                              Js::OpCodeUtil::GetOpCodeName(instr->m_opcode),
                              baseValueTypeStr);
                Output::Flush();
            }

            return false;
        }
    }

    int32 src1IntConstantValue;
    if(baseValueType.IsLikelyNativeIntArray() && src1Val && src1Val->GetValueInfo()->TryGetIntConstantValue(&src1IntConstantValue))
    {
        if(Js::SparseArraySegment<int32>::IsMissingItem(&src1IntConstantValue))
        {
            return false;
        }
    }

    // Note: doing ToVarUses to make sure we do get the int32 version of the index before trying to access its value in
    // ShouldExpectConventionalArrayIndexValue. Not sure why that never gave us a problem before.
    Assert(instr->GetDst()->IsIndirOpnd());
    IR::IndirOpnd *dst = instr->GetDst()->AsIndirOpnd();

    // Make sure we use the int32 version of the index operand symbol, if available.  Otherwise, ensure the var symbol is live (by
    // potentially inserting a ToVar).
    this->ToVarUses(instr, dst, /* isDst = */ true, nullptr);

    if (!ShouldExpectConventionalArrayIndexValue(dst))
    {
        GOPT_TRACE_INSTR(instr, _u("Didn't specialize array access, because index is negative or likely not int.\n"));
        if (PHASE_TRACE(Js::TypedArrayTypeSpecPhase, this->func))
        {
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
            char baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
            baseValueType.ToString(baseValueTypeStr);
            Output::Print(_u("Typed Array Optimization:  function: %s (%s): instr: %s, base value type: %S, did not specialize because index is negative or likely not int.\n"),
                this->func->GetJITFunctionBody()->GetDisplayName(),
                this->func->GetDebugNumberSet(debugStringBuffer),
                Js::OpCodeUtil::GetOpCodeName(instr->m_opcode),
                baseValueTypeStr);
            Output::Flush();
        }
        return false;
    }

    IRType          toType = TyVar;
    bool            isLossyAllowed = true;
    IR::BailOutKind arrayBailOutKind = IR::BailOutConventionalTypedArrayAccessOnly;

    switch(baseValueType.GetObjectType())
    {
    case ObjectType::Int8Array:
    case ObjectType::Uint8Array:
    case ObjectType::Int16Array:
    case ObjectType::Uint16Array:
    case ObjectType::Int32Array:
    case ObjectType::Int8VirtualArray:
    case ObjectType::Uint8VirtualArray:
    case ObjectType::Int16VirtualArray:
    case ObjectType::Uint16VirtualArray:
    case ObjectType::Int32VirtualArray:
    case ObjectType::Int8MixedArray:
    case ObjectType::Uint8MixedArray:
    case ObjectType::Int16MixedArray:
    case ObjectType::Uint16MixedArray:
    case ObjectType::Int32MixedArray:
    Int32Array:
        if (this->DoAggressiveIntTypeSpec() || this->DoFloatTypeSpec())
        {
            toType = TyInt32;
        }
        break;

    case ObjectType::Uint32Array:
    case ObjectType::Uint32VirtualArray:
    case ObjectType::Uint32MixedArray:
        // Uint32Arrays may store values that overflow int32.  If the value being stored comes from a symbol that's
        // already losslessly type specialized to int32, we'll use it.  Otherwise, if we only have a float64 specialized
        // value, we don't want to force bailout if it doesn't fit in int32.  Instead, we'll emit conversion in the
        // lowerer, and handle overflow, if necessary.
        if (!sym || CurrentBlockData()->IsInt32TypeSpecialized(sym))
        {
            toType = TyInt32;
        }
        else if (CurrentBlockData()->IsFloat64TypeSpecialized(sym))
        {
            toType = TyFloat64;
        }
        break;

    case ObjectType::Float32Array:
    case ObjectType::Float64Array:
    case ObjectType::Float32VirtualArray:
    case ObjectType::Float32MixedArray:
    case ObjectType::Float64VirtualArray:
    case ObjectType::Float64MixedArray:
    Float64Array:
    if (this->DoFloatTypeSpec())
    {
         toType = TyFloat64;
    }
    break;

    case ObjectType::Uint8ClampedArray:
    case ObjectType::Uint8ClampedVirtualArray:
    case ObjectType::Uint8ClampedMixedArray:
        // Uint8ClampedArray requires rounding (as opposed to truncation) of floating point values. If source symbol is
        // float type specialized, type specialize this instruction to float as well, and handle rounding in the
        // lowerer.
            if (!sym || CurrentBlockData()->IsInt32TypeSpecialized(sym))
            {
                toType = TyInt32;
                isLossyAllowed = false;
            }
            else if (CurrentBlockData()->IsFloat64TypeSpecialized(sym))
            {
                toType = TyFloat64;
            }
        break;

    default:
        Assert(baseValueType.IsLikelyNativeArray());
        isLossyAllowed = false;
        arrayBailOutKind = IR::BailOutConventionalNativeArrayAccessOnly;
        if(baseValueType.HasIntElements())
        {
            goto Int32Array;
        }
        Assert(baseValueType.HasFloatElements());
        goto Float64Array;
    }

    if (toType != TyVar)
    {
        GOPT_TRACE_INSTR(instr, _u("Type specialized array access.\n"));
        if (PHASE_TRACE(Js::TypedArrayTypeSpecPhase, this->func))
        {
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
            char baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
            baseValueType.ToString(baseValueTypeStr);
            Output::Print(_u("Typed Array Optimization:  function: %s (%s): instr: %s, base value type: %S, type specialized to %s.\n"),
                this->func->GetJITFunctionBody()->GetDisplayName(),
                this->func->GetDebugNumberSet(debugStringBuffer),
                Js::OpCodeUtil::GetOpCodeName(instr->m_opcode),
                baseValueTypeStr,
                toType == TyInt32 ? _u("int32") : _u("float64"));
            Output::Flush();
        }

        IR::BailOutKind bailOutKind = ((toType == TyInt32) ? IR::BailOutIntOnly : IR::BailOutNumberOnly);
        this->ToTypeSpecUse(instr, instr->GetSrc1(), this->currentBlock, src1Val, nullptr, toType, bailOutKind, /* lossy = */ isLossyAllowed);

        if (!this->IsLoopPrePass())
        {
            bool bConvertToBailoutInstr = true;
            // Definite StElemC doesn't need bailout, because it can't fail or cause conversion.
            if (instr->m_opcode == Js::OpCode::StElemC && baseValueType.IsObject())
            {
                if (baseValueType.HasIntElements())
                {
                    //Native int array requires a missing element check & bailout
                    int32 min = INT32_MIN;
                    int32 max = INT32_MAX;

                    if (src1Val->GetValueInfo()->GetIntValMinMax(&min, &max, false))
                    {
                        bConvertToBailoutInstr = ((min <= Js::JavascriptNativeIntArray::MissingItem) && (max >= Js::JavascriptNativeIntArray::MissingItem));
                    }
                }
                else
                {
                    bConvertToBailoutInstr = false;
                }
            }

            if (bConvertToBailoutInstr)
            {
                if(instr->HasBailOutInfo())
                {
                    const IR::BailOutKind oldBailOutKind = instr->GetBailOutKind();
                    Assert(
                        (
                            !(oldBailOutKind & ~IR::BailOutKindBits) ||
                            (oldBailOutKind & ~IR::BailOutKindBits) == IR::BailOutOnImplicitCallsPreOp
                        ) &&
                        !(oldBailOutKind & IR::BailOutKindBits & ~(IR::BailOutOnArrayAccessHelperCall | IR::BailOutMarkTempObject)));
                    if(arrayBailOutKind == IR::BailOutConventionalTypedArrayAccessOnly)
                    {
                        // BailOutConventionalTypedArrayAccessOnly also bails out if the array access is outside the head
                        // segment bounds, and guarantees no implicit calls. Override the bailout kind so that the instruction
                        // bails out for the right reason.
                        instr->SetBailOutKind(
                            arrayBailOutKind | (oldBailOutKind & (IR::BailOutKindBits - IR::BailOutOnArrayAccessHelperCall)));
                    }
                    else
                    {
                        // BailOutConventionalNativeArrayAccessOnly by itself may generate a helper call, and may cause implicit
                        // calls to occur, so it must be merged in to eliminate generating the helper call.
                        Assert(arrayBailOutKind == IR::BailOutConventionalNativeArrayAccessOnly);
                        instr->SetBailOutKind(oldBailOutKind | arrayBailOutKind);
                    }
                }
                else
                {
                    GenerateBailAtOperation(&instr, arrayBailOutKind);
                }
            }
        }
    }
    else
    {
        GOPT_TRACE_INSTR(instr, _u("Didn't specialize array access, because the source was not already specialized.\n"));
        if (PHASE_TRACE(Js::TypedArrayTypeSpecPhase, this->func))
        {
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
            char baseValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
            baseValueType.ToString(baseValueTypeStr);
            Output::Print(_u("Typed Array Optimization:  function: %s (%s): instr: %s, base value type: %S, did not type specialize, because of array type.\n"),
                this->func->GetJITFunctionBody()->GetDisplayName(),
                this->func->GetDebugNumberSet(debugStringBuffer),
                Js::OpCodeUtil::GetOpCodeName(instr->m_opcode),
                baseValueTypeStr);
            Output::Flush();
        }
    }

    return toType != TyVar;
}

IR::Instr *
GlobOpt::ToVarUses(IR::Instr *instr, IR::Opnd *opnd, bool isDst, Value *val)
{
    Sym *sym;

    switch (opnd->GetKind())
    {
    case IR::OpndKindReg:
        if (!isDst && !CurrentBlockData()->liveVarSyms->Test(opnd->AsRegOpnd()->m_sym->m_id))
        {
            instr = this->ToVar(instr, opnd->AsRegOpnd(), this->currentBlock, val, true);
        }
        break;

    case IR::OpndKindSym:
        sym = opnd->AsSymOpnd()->m_sym;

        if (sym->IsPropertySym() && !CurrentBlockData()->liveVarSyms->Test(sym->AsPropertySym()->m_stackSym->m_id)
            && sym->AsPropertySym()->m_stackSym->IsVar())
        {
            StackSym *propertyBase = sym->AsPropertySym()->m_stackSym;
            IR::RegOpnd *newOpnd = IR::RegOpnd::New(propertyBase, TyVar, instr->m_func);
            instr = this->ToVar(instr, newOpnd, this->currentBlock, CurrentBlockData()->FindValue(propertyBase), true);
        }
        break;

    case IR::OpndKindIndir:
        IR::RegOpnd *baseOpnd = opnd->AsIndirOpnd()->GetBaseOpnd();
        if (!CurrentBlockData()->liveVarSyms->Test(baseOpnd->m_sym->m_id))
        {
            instr = this->ToVar(instr, baseOpnd, this->currentBlock, CurrentBlockData()->FindValue(baseOpnd->m_sym), true);
        }
        IR::RegOpnd *indexOpnd = opnd->AsIndirOpnd()->GetIndexOpnd();
        if (indexOpnd && !indexOpnd->m_sym->IsTypeSpec())
        {
            if((indexOpnd->GetValueType().IsInt()
                    ? !IsTypeSpecPhaseOff(func)
                    : indexOpnd->GetValueType().IsLikelyInt() && DoAggressiveIntTypeSpec()) && !GetIsAsmJSFunc()) // typespec is disabled for asmjs
            {
                StackSym *const indexVarSym = indexOpnd->m_sym;
                Value *const indexValue = CurrentBlockData()->FindValue(indexVarSym);
                Assert(indexValue);
                Assert(indexValue->GetValueInfo()->IsLikelyInt());

                ToInt32(instr, indexOpnd, currentBlock, indexValue, opnd->AsIndirOpnd(), false);
                Assert(indexValue->GetValueInfo()->IsInt());

                if(!IsLoopPrePass())
                {
                    indexOpnd = opnd->AsIndirOpnd()->GetIndexOpnd();
                    if(indexOpnd)
                    {
                        Assert(indexOpnd->m_sym->IsTypeSpec());
                        IntConstantBounds indexConstantBounds;
                        AssertVerify(indexValue->GetValueInfo()->TryGetIntConstantBounds(&indexConstantBounds));
                        if(ValueInfo::IsGreaterThanOrEqualTo(
                                indexValue,
                                indexConstantBounds.LowerBound(),
                                indexConstantBounds.UpperBound(),
                                nullptr,
                                0,
                                0))
                        {
                            indexOpnd->SetType(TyUint32);
                        }
                    }
                }
            }
            else if (!CurrentBlockData()->liveVarSyms->Test(indexOpnd->m_sym->m_id))
            {
                instr = this->ToVar(instr, indexOpnd, this->currentBlock, CurrentBlockData()->FindValue(indexOpnd->m_sym), true);
            }
        }
        break;
    }

    return instr;
}

IR::Instr *
GlobOpt::ToVar(IR::Instr *instr, IR::RegOpnd *regOpnd, BasicBlock *block, Value *value, bool needsUpdate)
{
    IR::Instr *newInstr;
    StackSym *varSym = regOpnd->m_sym;

    if (IsTypeSpecPhaseOff(this->func))
    {
        return instr;
    }

    if (this->IsLoopPrePass())
    {
        block->globOptData.liveVarSyms->Set(varSym->m_id);
        return instr;
    }

    if (block->globOptData.liveVarSyms->Test(varSym->m_id))
    {
        // Already live, nothing to do
        return instr;
    }

    if (!varSym->IsVar())
    {
        Assert(!varSym->IsTypeSpec());
        // Leave non-vars alone.
        return instr;
    }

    Assert(block->globOptData.IsTypeSpecialized(varSym));

    if (!value)
    {
        value = block->globOptData.FindValue(varSym);
    }

    ValueInfo *valueInfo = value ? value->GetValueInfo() : nullptr;
    if(valueInfo && valueInfo->IsInt())
    {
        // If two syms have the same value, one is lossy-int-specialized, and then the other is int-specialized, the value
        // would have been updated to definitely int. Upon using the lossy-int-specialized sym later, it would be flagged as
        // lossy while the value is definitely int. Since the bit-vectors are based on the sym and not the value, update the
        // lossy state.
        block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id);
    }

    IRType fromType = TyIllegal;
    StackSym *typeSpecSym = nullptr;

    if (block->globOptData.liveInt32Syms->Test(varSym->m_id) && !block->globOptData.liveLossyInt32Syms->Test(varSym->m_id))
    {
        fromType = TyInt32;
        typeSpecSym = varSym->GetInt32EquivSym(this->func);
        Assert(valueInfo);
        Assert(valueInfo->IsInt());
    }
    else if (block->globOptData.liveFloat64Syms->Test(varSym->m_id))
    {

        fromType = TyFloat64;
        typeSpecSym = varSym->GetFloat64EquivSym(this->func);

        // Ensure that all bailout FromVars that generate a value for this type-specialized sym will bail out on any non-number
        // value, even ones that have already been generated before. Float-specialized non-number values cannot be converted
        // back to Var since they will not go back to the original non-number value. The dead-store pass will update the bailout
        // kind on already-generated FromVars based on this bit.
        typeSpecSym->m_requiresBailOnNotNumber = true;

        // A previous float conversion may have used BailOutPrimitiveButString, which does not change the value type to say
        // definitely float, since it can also be a non-string primitive. The convert back to Var though, will cause that
        // bailout kind to be changed to BailOutNumberOnly in the dead-store phase, so from the point of the initial conversion
        // to float, that the value is definitely number. Since we don't know where the FromVar is, change the value type here.
        if(valueInfo)
        {
            if(!valueInfo->IsNumber())
            {
                valueInfo = valueInfo->SpecializeToFloat64(alloc);
                ChangeValueInfo(block, value, valueInfo);
                regOpnd->SetValueType(valueInfo->Type());
            }
        }
        else
        {
            value = NewGenericValue(ValueType::Float);
            valueInfo = value->GetValueInfo();
            block->globOptData.SetValue(value, varSym);
            regOpnd->SetValueType(valueInfo->Type());
        }
    }
    else
    {
#ifdef ENABLE_SIMDJS
        // SIMD_JS
        Assert(block->globOptData.IsLiveAsSimd128(varSym));
        if (block->globOptData.IsLiveAsSimd128F4(varSym))
        {
            fromType = TySimd128F4;
        }
        else
        {
            Assert(block->globOptData.IsLiveAsSimd128I4(varSym));
            fromType = TySimd128I4;
        }

        if (valueInfo)
        {
            if (fromType == TySimd128F4 && !valueInfo->Type().IsSimd128Float32x4())
            {
                valueInfo = valueInfo->SpecializeToSimd128F4(alloc);
                ChangeValueInfo(block, value, valueInfo);
                regOpnd->SetValueType(valueInfo->Type());
            }
            else if (fromType == TySimd128I4 && !valueInfo->Type().IsSimd128Int32x4())
            {
                if (!valueInfo->Type().IsSimd128Int32x4())
                {
                    valueInfo = valueInfo->SpecializeToSimd128I4(alloc);
                    ChangeValueInfo(block, value, valueInfo);
                    regOpnd->SetValueType(valueInfo->Type());
                }
            }
        }
        else
        {
            ValueType valueType = fromType == TySimd128F4 ? ValueType::GetSimd128(ObjectType::Simd128Float32x4) : ValueType::GetSimd128(ObjectType::Simd128Int32x4);
            value = NewGenericValue(valueType);
            valueInfo = value->GetValueInfo();
            block->globOptData.SetValue(value, varSym);
            regOpnd->SetValueType(valueInfo->Type());
        }

        ValueType valueType = valueInfo->Type();

        // Should be definite if type-specialized
        Assert(valueType.IsSimd128());

        typeSpecSym = varSym->GetSimd128EquivSym(fromType, this->func);
#else
        Assert(UNREACHED);
#endif
    }

    AssertOrFailFast(valueInfo);

    int32 intConstantValue;
    if (valueInfo->TryGetIntConstantValue(&intConstantValue))
    {
        // Lower will tag or create a number directly
        newInstr = IR::Instr::New(Js::OpCode::LdC_A_I4, regOpnd,
            IR::IntConstOpnd::New(intConstantValue, TyInt32, instr->m_func), instr->m_func);
    }
    else
    {
        IR::RegOpnd * regNew = IR::RegOpnd::New(typeSpecSym, fromType, instr->m_func);
        Js::OpCode opcode = Js::OpCode::ToVar;
        regNew->SetIsJITOptimizedReg(true);

        newInstr = IR::Instr::New(opcode, regOpnd, regNew, instr->m_func);
    }
    newInstr->SetByteCodeOffset(instr);
    newInstr->GetDst()->AsRegOpnd()->SetIsJITOptimizedReg(true);

    ValueType valueType = valueInfo->Type();
    if(fromType == TyInt32)
    {
    #if !INT32VAR // All 32-bit ints are taggable on 64-bit architectures
        IntConstantBounds constantBounds;
        AssertVerify(valueInfo->TryGetIntConstantBounds(&constantBounds));
        if(constantBounds.IsTaggable())
    #endif
        {
            // The value is within the taggable range, so set the opnd value types to TaggedInt to avoid the overflow check
            valueType = ValueType::GetTaggedInt();
        }
    }
    newInstr->GetDst()->SetValueType(valueType);
    newInstr->GetSrc1()->SetValueType(valueType);

    IR::Instr *insertAfterInstr = instr->m_prev;
    if (instr == block->GetLastInstr() &&
        (instr->IsBranchInstr() || instr->m_opcode == Js::OpCode::BailTarget))
    {
        // Don't insert code between the branch and the preceding ByteCodeUses instrs...
        while(insertAfterInstr->m_opcode == Js::OpCode::ByteCodeUses)
        {
            insertAfterInstr = insertAfterInstr->m_prev;
        }
    }
    block->InsertInstrAfter(newInstr, insertAfterInstr);

    block->globOptData.liveVarSyms->Set(varSym->m_id);

    GOPT_TRACE_OPND(regOpnd, _u("Converting to var\n"));

    if (block->loop)
    {
        Assert(!this->IsLoopPrePass());
        this->TryHoistInvariant(newInstr, block, value, value, nullptr, false);
    }

    if (needsUpdate)
    {
        // Make sure that the kill effect of the ToVar instruction is tracked and that the kill of a property
        // type is reflected in the current instruction.
        this->ProcessKills(newInstr);
        this->ValueNumberObjectType(newInstr->GetDst(), newInstr);

        if (instr->GetSrc1() && instr->GetSrc1()->IsSymOpnd() && instr->GetSrc1()->AsSymOpnd()->IsPropertySymOpnd())
        {
            // Reprocess the load source. We need to reset the PropertySymOpnd fields first.
            IR::PropertySymOpnd *propertySymOpnd = instr->GetSrc1()->AsPropertySymOpnd();
            if (propertySymOpnd->IsTypeCheckSeqCandidate())
            {
                propertySymOpnd->SetTypeChecked(false);
                propertySymOpnd->SetTypeAvailable(false);
                propertySymOpnd->SetWriteGuardChecked(false);
            }

            this->FinishOptPropOp(instr, propertySymOpnd);
            instr = this->SetTypeCheckBailOut(instr->GetSrc1(), instr, nullptr);
        }
    }

    return instr;
}

IR::Instr *
GlobOpt::ToInt32(IR::Instr *instr, IR::Opnd *opnd, BasicBlock *block, Value *val, IR::IndirOpnd *indir, bool lossy)
{
    return this->ToTypeSpecUse(instr, opnd, block, val, indir, TyInt32, IR::BailOutIntOnly, lossy);
}

IR::Instr *
GlobOpt::ToFloat64(IR::Instr *instr, IR::Opnd *opnd, BasicBlock *block, Value *val, IR::IndirOpnd *indir, IR::BailOutKind bailOutKind)
{
    return this->ToTypeSpecUse(instr, opnd, block, val, indir, TyFloat64, bailOutKind);
}

IR::Instr *
GlobOpt::ToTypeSpecUse(IR::Instr *instr, IR::Opnd *opnd, BasicBlock *block, Value *val, IR::IndirOpnd *indir, IRType toType, IR::BailOutKind bailOutKind, bool lossy, IR::Instr *insertBeforeInstr)
{
    Assert(bailOutKind != IR::BailOutInvalid);
    IR::Instr *newInstr;

    if (!val && opnd->IsRegOpnd())
    {
        val = block->globOptData.FindValue(opnd->AsRegOpnd()->m_sym);
    }

    ValueInfo *valueInfo = val ? val->GetValueInfo() : nullptr;
    bool needReplaceSrc = false;
    bool updateBlockLastInstr = false;

    if (instr)
    {
        needReplaceSrc = true;
        if (!insertBeforeInstr)
        {
            insertBeforeInstr = instr;
        }
    }
    else if (!insertBeforeInstr)
    {
        // Insert it at the end of the block
        insertBeforeInstr = block->GetLastInstr();
        if (insertBeforeInstr->IsBranchInstr() || insertBeforeInstr->m_opcode == Js::OpCode::BailTarget)
        {
            // Don't insert code between the branch and the preceding ByteCodeUses instrs...
            while(insertBeforeInstr->m_prev->m_opcode == Js::OpCode::ByteCodeUses)
            {
                insertBeforeInstr = insertBeforeInstr->m_prev;
            }
        }
        else
        {
            insertBeforeInstr = insertBeforeInstr->m_next;
            updateBlockLastInstr = true;
        }
    }

    // Int constant values will be propagated into the instruction. For ArgOut_A_InlineBuiltIn, there's no benefit from
    // const-propping, so those are excluded.
    if (opnd->IsRegOpnd() &&
        !(
            valueInfo &&
            (valueInfo->HasIntConstantValue() || valueInfo->IsFloatConstant()) &&
            (!instr || instr->m_opcode != Js::OpCode::ArgOut_A_InlineBuiltIn)
        ))
    {
        IR::RegOpnd *regSrc = opnd->AsRegOpnd();
        StackSym *varSym = regSrc->m_sym;
        Js::OpCode opcode = Js::OpCode::FromVar;

        if (varSym->IsTypeSpec() || !block->globOptData.liveVarSyms->Test(varSym->m_id))
        {
            // Conversion between int32 and float64
            if (varSym->IsTypeSpec())
            {
                varSym = varSym->GetVarEquivSym(this->func);
            }
            opcode = Js::OpCode::Conv_Prim;
        }

        Assert(block->globOptData.liveVarSyms->Test(varSym->m_id) || block->globOptData.IsTypeSpecialized(varSym));

        StackSym *typeSpecSym = nullptr;
        BOOL isLive = FALSE;
        BVSparse<JitArenaAllocator> *livenessBv = nullptr;

        if(valueInfo && valueInfo->IsInt())
        {
            // If two syms have the same value, one is lossy-int-specialized, and then the other is int-specialized, the value
            // would have been updated to definitely int. Upon using the lossy-int-specialized sym later, it would be flagged as
            // lossy while the value is definitely int. Since the bit-vectors are based on the sym and not the value, update the
            // lossy state.
            block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id);
        }

        if (toType == TyInt32)
        {
            // Need to determine whether the conversion is actually lossy or lossless. If the value is an int, then it's a
            // lossless conversion despite the type of conversion requested. The liveness of the converted int32 sym needs to be
            // set to reflect the actual type of conversion done. Also, a lossless conversion needs the value to determine
            // whether the conversion may need to bail out.
            Assert(valueInfo);
            if(valueInfo->IsInt())
            {
                lossy = false;
            }
            else
            {
                Assert(IsLoopPrePass() || !block->globOptData.IsInt32TypeSpecialized(varSym));
            }

            livenessBv = block->globOptData.liveInt32Syms;
            isLive = livenessBv->Test(varSym->m_id) && (lossy || !block->globOptData.liveLossyInt32Syms->Test(varSym->m_id));
            if (this->IsLoopPrePass())
            {
                if(!isLive)
                {
                    livenessBv->Set(varSym->m_id);
                    if(lossy)
                    {
                        block->globOptData.liveLossyInt32Syms->Set(varSym->m_id);
                    }
                    else
                    {
                        block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id);
                    }
                }
                if(!lossy)
                {
                    Assert(bailOutKind == IR::BailOutIntOnly || bailOutKind == IR::BailOutExpectingInteger);
                    valueInfo = valueInfo->SpecializeToInt32(alloc);
                    ChangeValueInfo(nullptr, val, valueInfo);
                    if(needReplaceSrc)
                    {
                        opnd->SetValueType(valueInfo->Type());
                    }
                }
                return instr;
            }

            typeSpecSym = varSym->GetInt32EquivSym(this->func);

            if (!isLive)
            {
                if (!opnd->IsVar() ||
                    !block->globOptData.liveVarSyms->Test(varSym->m_id) ||
                    (block->globOptData.liveFloat64Syms->Test(varSym->m_id) && valueInfo && valueInfo->IsLikelyFloat()))
                {
                    Assert(block->globOptData.liveFloat64Syms->Test(varSym->m_id));

                    if(!lossy && !valueInfo->IsInt())
                    {
                        // Shouldn't try to do a lossless conversion from float64 to int32 when the value is not known to be an
                        // int. There are cases where we need more than two passes over loops to flush out all dependencies.
                        // It's possible for the loop prepass to think that a sym s1 remains an int because it acquires the
                        // value of another sym s2 that is an int in the prepass at that time. However, s2 can become a float
                        // later in the loop body, in which case s1 would become a float on the second iteration of the loop. By
                        // that time, we would have already committed to having s1 live as a lossless int on entry into the
                        // loop, and we end up having to compensate by doing a lossless conversion from float to int, which will
                        // need a bailout and will most likely bail out.
                        //
                        // If s2 becomes a var instead of a float, then the compensation is legal although not ideal. After
                        // enough bailouts, rejit would be triggered with aggressive int type spec turned off. For the
                        // float-to-int conversion though, there's no point in emitting a bailout because we already know that
                        // the value is a float and has high probability of bailing out (whereas a var has a chance to be a
                        // tagged int), and so currently lossless conversion from float to int with bailout is not supported.
                        //
                        // So, treating this case as a compile-time bailout. The exception will trigger the jit work item to be
                        // restarted with aggressive int type specialization disabled.
                        if(bailOutKind == IR::BailOutExpectingInteger)
                        {
                            Assert(IsSwitchOptEnabled());
                            throw Js::RejitException(RejitReason::DisableSwitchOptExpectingInteger);
                        }
                        else
                        {
                            Assert(DoAggressiveIntTypeSpec());
                            if(PHASE_TRACE(Js::BailOutPhase, this->func))
                            {
                                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                                Output::Print(
                                    _u("BailOut (compile-time): function: %s (%s) varSym: "),
                                    this->func->GetJITFunctionBody()->GetDisplayName(),
                                    this->func->GetDebugNumberSet(debugStringBuffer),
                                    varSym->m_id);
    #if DBG_DUMP
                                varSym->Dump();
    #else
                                Output::Print(_u("s%u"), varSym->m_id);
    #endif
                                if(varSym->HasByteCodeRegSlot())
                                {
                                    Output::Print(_u(" byteCodeReg: R%u"), varSym->GetByteCodeRegSlot());
                                }
                                Output::Print(_u(" (lossless conversion from float64 to int32)\n"));
                                Output::Flush();
                            }

                            if(!DoAggressiveIntTypeSpec())
                            {
                                // Aggressive int type specialization is already off for some reason. Prevent trying to rejit again
                                // because it won't help and the same thing will happen again. Just abort jitting this function.
                                if(PHASE_TRACE(Js::BailOutPhase, this->func))
                                {
                                    Output::Print(_u("    Aborting JIT because AggressiveIntTypeSpec is already off\n"));
                                    Output::Flush();
                                }
                                throw Js::OperationAbortedException();
                            }

                            throw Js::RejitException(RejitReason::AggressiveIntTypeSpecDisabled);
                        }
                    }

                    if(opnd->IsVar())
                    {
                        regSrc->SetType(TyFloat64);
                        regSrc->m_sym = varSym->GetFloat64EquivSym(this->func);
                        opcode = Js::OpCode::Conv_Prim;
                    }
                    else
                    {
                        Assert(regSrc->IsFloat64());
                        Assert(regSrc->m_sym->IsFloat64());
                        Assert(opcode == Js::OpCode::Conv_Prim);
                    }
                }
            }
            GOPT_TRACE_OPND(regSrc, _u("Converting to int32\n"));
        }
        else if (toType == TyFloat64)
        {
            // float64
            typeSpecSym = varSym->GetFloat64EquivSym(this->func);
            if(!IsLoopPrePass() && typeSpecSym->m_requiresBailOnNotNumber && block->globOptData.IsFloat64TypeSpecialized(varSym))
            {
                // This conversion is already protected by a BailOutNumberOnly bailout (or at least it will be after the
                // dead-store phase). Since 'requiresBailOnNotNumber' is not flow-based, change the value to definitely float.
                if(valueInfo)
                {
                    if(!valueInfo->IsNumber())
                    {
                        valueInfo = valueInfo->SpecializeToFloat64(alloc);
                        ChangeValueInfo(block, val, valueInfo);
                        opnd->SetValueType(valueInfo->Type());
                    }
                }
                else
                {
                    val = NewGenericValue(ValueType::Float);
                    valueInfo = val->GetValueInfo();
                    block->globOptData.SetValue(val, varSym);
                    opnd->SetValueType(valueInfo->Type());
                }
            }

            if(bailOutKind == IR::BailOutNumberOnly)
            {
                if(!IsLoopPrePass())
                {
                    // Ensure that all bailout FromVars that generate a value for this type-specialized sym will bail out on any
                    // non-number value, even ones that have already been generated before. The dead-store pass will update the
                    // bailout kind on already-generated FromVars based on this bit.
                    typeSpecSym->m_requiresBailOnNotNumber = true;
                }
            }
            else if(typeSpecSym->m_requiresBailOnNotNumber)
            {
                Assert(bailOutKind == IR::BailOutPrimitiveButString);
                bailOutKind = IR::BailOutNumberOnly;
            }

            livenessBv = block->globOptData.liveFloat64Syms;
            isLive = livenessBv->Test(varSym->m_id);
            if (this->IsLoopPrePass())
            {
                if(!isLive)
                {
                    livenessBv->Set(varSym->m_id);
                }

                if (this->OptIsInvariant(opnd, block, this->prePassLoop, val, false, true))
                {
                    this->prePassLoop->forceFloat64SymsOnEntry->Set(varSym->m_id);
                }
                else
                {
                    Sym *symStore = (valueInfo ? valueInfo->GetSymStore() : NULL);
                    if (symStore && symStore != varSym
                        && this->OptIsInvariant(symStore, block, this->prePassLoop, block->globOptData.FindValue(symStore), false, true))
                    {
                        // If symStore is assigned to sym and we want sym to be type-specialized, for symStore to be specialized
                        // outside the loop.
                        this->prePassLoop->forceFloat64SymsOnEntry->Set(symStore->m_id);
                    }
                }

                if(bailOutKind == IR::BailOutNumberOnly)
                {
                    if(valueInfo)
                    {
                        valueInfo = valueInfo->SpecializeToFloat64(alloc);
                        ChangeValueInfo(block, val, valueInfo);
                    }
                    else
                    {
                        val = NewGenericValue(ValueType::Float);
                        valueInfo = val->GetValueInfo();
                        block->globOptData.SetValue(val, varSym);
                    }
                    if(needReplaceSrc)
                    {
                        opnd->SetValueType(valueInfo->Type());
                    }
                }
                return instr;
            }

            if (!isLive && regSrc->IsVar())
            {
                if (!block->globOptData.liveVarSyms->Test(varSym->m_id) ||
                    (
                        block->globOptData.liveInt32Syms->Test(varSym->m_id) &&
                        !block->globOptData.liveLossyInt32Syms->Test(varSym->m_id) &&
                        valueInfo &&
                        valueInfo->IsLikelyInt()
                    ))
                {
                    Assert(block->globOptData.liveInt32Syms->Test(varSym->m_id));
                    Assert(!block->globOptData.liveLossyInt32Syms->Test(varSym->m_id)); // Shouldn't try to convert a lossy int32 to anything
                    regSrc->SetType(TyInt32);
                    regSrc->m_sym = varSym->GetInt32EquivSym(this->func);
                    opcode = Js::OpCode::Conv_Prim;
                }
            }
            GOPT_TRACE_OPND(regSrc, _u("Converting to float64\n"));
        }
#ifdef ENABLE_SIMDJS
        else
        {
            // SIMD_JS
            Assert(IRType_IsSimd128(toType));

            // Get or create type-spec sym
            typeSpecSym = varSym->GetSimd128EquivSym(toType, this->func);

            if (!IsLoopPrePass() && block->globOptData.IsSimd128TypeSpecialized(toType, varSym))
            {
                // Consider: Is this needed ? Shouldn't this have been done at previous FromVar since the simd128 sym is alive ?
                if (valueInfo)
                {
                    if (!valueInfo->IsSimd128(toType))
                    {
                        valueInfo = valueInfo->SpecializeToSimd128(toType, alloc);
                        ChangeValueInfo(block, val, valueInfo);
                        opnd->SetValueType(valueInfo->Type());
                    }
                }
                else
                {
                    val = NewGenericValue(GetValueTypeFromIRType(toType));
                    valueInfo = val->GetValueInfo();
                    block->globOptData.SetValue(val, varSym);
                    opnd->SetValueType(valueInfo->Type());
                }
            }

            livenessBv = block->globOptData.GetSimd128LivenessBV(toType);
            isLive = livenessBv->Test(varSym->m_id);
            if (this->IsLoopPrePass())
            {
                // FromVar Hoisting
                BVSparse<Memory::JitArenaAllocator> * forceSimd128SymsOnEntry;
                forceSimd128SymsOnEntry = \
                    toType == TySimd128F4 ? this->prePassLoop->forceSimd128F4SymsOnEntry : this->prePassLoop->forceSimd128I4SymsOnEntry;
                if (!isLive)
                {
                    livenessBv->Set(varSym->m_id);
                }

                // Be aggressive with hoisting only if value is always initialized to SIMD type before entering loop.
                // This reduces the chance that the FromVar gets executed while the specialized instruction in the loop is not. Leading to unnecessary excessive bailouts.
                if (val && !val->GetValueInfo()->HasBeenUndefined() &&  !val->GetValueInfo()->HasBeenNull() &&
                    this->OptIsInvariant(opnd, block, this->prePassLoop, val, false, true))
                {
                    forceSimd128SymsOnEntry->Set(varSym->m_id);
                }
                else
                {
                    Sym *symStore = (valueInfo ? valueInfo->GetSymStore() : NULL);
                    Value * value = symStore ? block->globOptData.FindValue(symStore) : nullptr;

                    if (symStore && symStore != varSym
                        && value
                        && !value->GetValueInfo()->HasBeenUndefined() && !value->GetValueInfo()->HasBeenNull()
                        && this->OptIsInvariant(symStore, block, this->prePassLoop, value, true, true))
                    {
                        // If symStore is assigned to sym and we want sym to be type-specialized, for symStore to be specialized
                        // outside the loop.
                        forceSimd128SymsOnEntry->Set(symStore->m_id);
                    }
                }

                Assert(bailOutKind == IR::BailOutSimd128F4Only || bailOutKind == IR::BailOutSimd128I4Only);

                // We are in loop prepass, we haven't propagated the value info to the src. Do it now.
                if (valueInfo)
                {
                    valueInfo = valueInfo->SpecializeToSimd128(toType, alloc);
                    ChangeValueInfo(block, val, valueInfo);
                }
                else
                {
                    val = NewGenericValue(GetValueTypeFromIRType(toType));
                    valueInfo = val->GetValueInfo();
                    block->globOptData.SetValue(val, varSym);
                }
                if (needReplaceSrc)
                {
                    opnd->SetValueType(valueInfo->Type());
                }
                return instr;
            }
            GOPT_TRACE_OPND(regSrc, _u("Converting to Simd128\n"));
        }
#endif
        bool needLoad = false;

        if (needReplaceSrc)
        {
            bool wasDead = regSrc->GetIsDead();
            // needReplaceSrc means we are type specializing a use, and need to replace the src on the instr
            if (!isLive)
            {
                needLoad = true;
                // ReplaceSrc will delete it.
                regSrc = regSrc->Copy(instr->m_func)->AsRegOpnd();
            }
            IR::RegOpnd * regNew = IR::RegOpnd::New(typeSpecSym, toType, instr->m_func);
            if(valueInfo)
            {
                regNew->SetValueType(valueInfo->Type());
                regNew->m_wasNegativeZeroPreventedByBailout = valueInfo->WasNegativeZeroPreventedByBailout();
            }
            regNew->SetIsDead(wasDead);
            regNew->SetIsJITOptimizedReg(true);

            this->CaptureByteCodeSymUses(instr);
            if (indir == nullptr)
            {
                instr->ReplaceSrc(opnd, regNew);
            }
            else
            {
                indir->ReplaceIndexOpnd(regNew);
            }
            opnd = regNew;

            if (!needLoad)
            {
                Assert(isLive);
                return instr;
            }
        }
        else
        {
            // We just need to insert a load of a type spec sym
            if(isLive)
            {
                return instr;
            }

            // Insert it before the specified instruction
            instr = insertBeforeInstr;
        }


        IR::RegOpnd *regDst = IR::RegOpnd::New(typeSpecSym, toType, instr->m_func);
        bool isBailout = false;
        bool isHoisted = false;
        bool isInLandingPad = (block->next && !block->next->isDeleted && block->next->isLoopHeader);

        if (isInLandingPad)
        {
            Loop *loop = block->next->loop;
            Assert(loop && loop->landingPad == block);
            Assert(loop->bailOutInfo);
        }

        if (opcode == Js::OpCode::FromVar)
        {

            if (toType == TyInt32)
            {
                Assert(valueInfo);
                if (lossy)
                {
                    if (!valueInfo->IsPrimitive() && !block->globOptData.IsTypeSpecialized(varSym))
                    {
                        // Lossy conversions to int32 on non-primitive values may have implicit calls to toString or valueOf, which
                        // may be overridden to have a side effect. The side effect needs to happen every time the conversion is
                        // supposed to happen, so the resulting lossy int32 value cannot be reused. Bail out on implicit calls.
                        Assert(DoLossyIntTypeSpec());

                        bailOutKind = IR::BailOutOnNotPrimitive;
                        isBailout = true;
                    }
                }
                else if (!valueInfo->IsInt())
                {
                    // The operand is likely an int (hence the request to convert to int), so bail out if it's not an int. Only
                    // bail out if a lossless conversion to int is requested. Lossy conversions to int such as in (a | 0) don't
                    // need to bail out.
                    if (bailOutKind == IR::BailOutExpectingInteger)
                    {
                        Assert(IsSwitchOptEnabled());
                    }
                    else
                    {
                        Assert(DoAggressiveIntTypeSpec());
                    }

                    isBailout = true;
                }
            }
            else if (toType == TyFloat64 &&
                (!valueInfo || !valueInfo->IsNumber()))
            {
                // Bailout if converting vars to float if we can't prove they are floats:
                //      x = str + float;  -> need to bailout if str is a string
                //
                //      x = obj * 0.1;
                //      y = obj * 0.2;  -> if obj has valueof, we'll only call valueof once on the FromVar conversion...
                Assert(bailOutKind != IR::BailOutInvalid);
                isBailout = true;
            }
#ifdef ENABLE_SIMDJS
            else if (IRType_IsSimd128(toType) &&
                (!valueInfo || !valueInfo->IsSimd128(toType)))
            {
                Assert(toType == TySimd128F4 && bailOutKind == IR::BailOutSimd128F4Only
                    || toType == TySimd128I4 && bailOutKind == IR::BailOutSimd128I4Only);
                isBailout = true;
            }
#endif
        }

        if (isBailout)
        {
            if (isInLandingPad)
            {
                Loop *loop = block->next->loop;
                this->EnsureBailTarget(loop);
                instr = loop->bailOutInfo->bailOutInstr;
                updateBlockLastInstr = false;
                newInstr = IR::BailOutInstr::New(opcode, bailOutKind, loop->bailOutInfo, instr->m_func);
                newInstr->SetDst(regDst);
                newInstr->SetSrc1(regSrc);
            }
            else
            {
                newInstr = IR::BailOutInstr::New(opcode, regDst, regSrc, bailOutKind, instr, instr->m_func);
            }
        }
        else
        {
            newInstr = IR::Instr::New(opcode, regDst, regSrc, instr->m_func);
        }

        newInstr->SetByteCodeOffset(instr);
        instr->InsertBefore(newInstr);
        if (updateBlockLastInstr)
        {
            block->SetLastInstr(newInstr);
        }
        regDst->SetIsJITOptimizedReg(true);
        newInstr->GetSrc1()->AsRegOpnd()->SetIsJITOptimizedReg(true);

        ValueInfo *const oldValueInfo = valueInfo;
        if(valueInfo)
        {
            newInstr->GetSrc1()->SetValueType(valueInfo->Type());
        }
        if(isBailout)
        {
            Assert(opcode == Js::OpCode::FromVar);
            if(toType == TyInt32)
            {
                Assert(valueInfo);
                if(!lossy)
                {
                    Assert(bailOutKind == IR::BailOutIntOnly || bailOutKind == IR::BailOutExpectingInteger);
                    valueInfo = valueInfo->SpecializeToInt32(alloc, isPerformingLoopBackEdgeCompensation);
                    ChangeValueInfo(nullptr, val, valueInfo);

                    int32 intConstantValue;
                    if(indir && needReplaceSrc && valueInfo->TryGetIntConstantValue(&intConstantValue))
                    {
                        // A likely-int value can have constant bounds due to conditional branches narrowing its range. Now that
                        // the sym has been proven to be an int, the likely-int value, after specialization, will be constant.
                        // Replace the index opnd in the indir with an offset.
                        Assert(opnd == indir->GetIndexOpnd());
                        Assert(indir->GetScale() == 0);
                        indir->UnlinkIndexOpnd()->Free(instr->m_func);
                        opnd = nullptr;
                        indir->SetOffset(intConstantValue);
                    }
                }
            }
            else if (toType == TyFloat64)
            {
                if(bailOutKind == IR::BailOutNumberOnly)
                {
                    if(valueInfo)
                    {
                        valueInfo = valueInfo->SpecializeToFloat64(alloc);
                        ChangeValueInfo(block, val, valueInfo);
                    }
                    else
                    {
                        val = NewGenericValue(ValueType::Float);
                        valueInfo = val->GetValueInfo();
                        block->globOptData.SetValue(val, varSym);
                    }
                }
            }
            else
            {
                Assert(IRType_IsSimd128(toType));
                if (valueInfo)
                {
                    valueInfo = valueInfo->SpecializeToSimd128(toType, alloc);
                    ChangeValueInfo(block, val, valueInfo);
                }
                else
                {
                    val = NewGenericValue(GetValueTypeFromIRType(toType));
                    valueInfo = val->GetValueInfo();
                    block->globOptData.SetValue(val, varSym);
                }
            }
        }

        if(valueInfo)
        {
            newInstr->GetDst()->SetValueType(valueInfo->Type());
            if(needReplaceSrc && opnd)
            {
                opnd->SetValueType(valueInfo->Type());
            }
        }

        if (block->loop)
        {
            Assert(!this->IsLoopPrePass());
            isHoisted = this->TryHoistInvariant(newInstr, block, val, val, nullptr, false, lossy, false, bailOutKind);
        }

        if (isBailout)
        {
            if (!isHoisted && !isInLandingPad)
            {
                if(valueInfo)
                {
                    // Since this is a pre-op bailout, the old value info should be used for the purposes of bailout. For
                    // instance, the value info could be LikelyInt but with a constant range. Once specialized to int, the value
                    // info would be an int constant. However, the int constant is only guaranteed if the value is actually an
                    // int, which this conversion is verifying, so bailout cannot assume the constant value.
                    if(oldValueInfo)
                    {
                        val->SetValueInfo(oldValueInfo);
                    }
                    else
                    {
                        block->globOptData.ClearSymValue(varSym);
                    }
                }

                // Fill in bail out info if the FromVar is a bailout instr, and it wasn't hoisted as invariant.
                // If it was hoisted, the invariant code will fill out the bailout info with the loop landing pad bailout info.
                this->FillBailOutInfo(block, newInstr->GetBailOutInfo());

                if(valueInfo)
                {
                    // Restore the new value info after filling the bailout info
                    if(oldValueInfo)
                    {
                        val->SetValueInfo(valueInfo);
                    }
                    else
                    {
                        block->globOptData.SetValue(val, varSym);
                    }
                }
            }
        }

        // Now that we've captured the liveness in the bailout info, we can mark this as live.
        // This type specialized sym isn't live if the FromVar bails out.
        livenessBv->Set(varSym->m_id);
        if(toType == TyInt32)
        {
            if(lossy)
            {
                block->globOptData.liveLossyInt32Syms->Set(varSym->m_id);
            }
            else
            {
                block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id);
            }
        }
    }
    else
    {
        Assert(valueInfo);
        if(opnd->IsRegOpnd() && valueInfo->IsInt())
        {
            // If two syms have the same value, one is lossy-int-specialized, and then the other is int-specialized, the value
            // would have been updated to definitely int. Upon using the lossy-int-specialized sym later, it would be flagged as
            // lossy while the value is definitely int. Since the bit-vectors are based on the sym and not the value, update the
            // lossy state.
            block->globOptData.liveLossyInt32Syms->Clear(opnd->AsRegOpnd()->m_sym->m_id);
            if(toType == TyInt32)
            {
                lossy = false;
            }
        }

        if (this->IsLoopPrePass())
        {
            if(opnd->IsRegOpnd())
            {
                StackSym *const sym = opnd->AsRegOpnd()->m_sym;
                if(toType == TyInt32)
                {
                    Assert(!sym->IsTypeSpec());
                    block->globOptData.liveInt32Syms->Set(sym->m_id);
                    if(lossy)
                    {
                        block->globOptData.liveLossyInt32Syms->Set(sym->m_id);
                    }
                    else
                    {
                        block->globOptData.liveLossyInt32Syms->Clear(sym->m_id);
                    }
                }
                else
                {
                    Assert(toType == TyFloat64);
                    AnalysisAssert(instr);
                    StackSym *const varSym = sym->IsTypeSpec() ? sym->GetVarEquivSym(instr->m_func) : sym;
                    block->globOptData.liveFloat64Syms->Set(varSym->m_id);
                }
            }
            return instr;
        }

        if (!needReplaceSrc)
        {
            instr = insertBeforeInstr;
        }

        IR::Opnd *constOpnd;
        int32 intConstantValue;
        if(valueInfo->TryGetIntConstantValue(&intConstantValue))
        {
            if(toType == TyInt32)
            {
                constOpnd = IR::IntConstOpnd::New(intConstantValue, TyInt32, instr->m_func);
            }
            else
            {
                Assert(toType == TyFloat64);
                constOpnd = IR::FloatConstOpnd::New(static_cast<FloatConstType>(intConstantValue), TyFloat64, instr->m_func);
            }
        }
        else if(valueInfo->IsFloatConstant())
        {
            const FloatConstType floatValue = valueInfo->AsFloatConstant()->FloatValue();
            if(toType == TyInt32)
            {
                Assert(lossy);
                constOpnd =
                    IR::IntConstOpnd::New(
                        Js::JavascriptMath::ToInt32(floatValue),
                        TyInt32,
                        instr->m_func);
            }
            else
            {
                Assert(toType == TyFloat64);
                constOpnd = IR::FloatConstOpnd::New(floatValue, TyFloat64, instr->m_func);
            }
        }
        else
        {
            Assert(opnd->IsVar());
            Assert(opnd->IsAddrOpnd());
            AssertMsg(opnd->AsAddrOpnd()->IsVar(), "We only expect to see addr that are var before lower.");

            // Don't need to capture uses, we are only replacing an addr opnd
            if(toType == TyInt32)
            {
                constOpnd = IR::IntConstOpnd::New(Js::TaggedInt::ToInt32(opnd->AsAddrOpnd()->m_address), TyInt32, instr->m_func);
            }
            else
            {
                Assert(toType == TyFloat64);
                constOpnd = IR::FloatConstOpnd::New(Js::TaggedInt::ToDouble(opnd->AsAddrOpnd()->m_address), TyFloat64, instr->m_func);
            }
        }

        if (toType == TyInt32)
        {
            if (needReplaceSrc)
            {
                CaptureByteCodeSymUses(instr);
                if(indir)
                {
                    Assert(opnd == indir->GetIndexOpnd());
                    Assert(indir->GetScale() == 0);
                    indir->UnlinkIndexOpnd()->Free(instr->m_func);
                    indir->SetOffset(constOpnd->AsIntConstOpnd()->AsInt32());
                }
                else
                {
                    instr->ReplaceSrc(opnd, constOpnd);
                }
            }
            else
            {
                StackSym *varSym = opnd->AsRegOpnd()->m_sym;
                if(varSym->IsTypeSpec())
                {
                    varSym = varSym->GetVarEquivSym(nullptr);
                    Assert(varSym);
                }
                if(block->globOptData.liveInt32Syms->TestAndSet(varSym->m_id))
                {
                    Assert(!!block->globOptData.liveLossyInt32Syms->Test(varSym->m_id) == lossy);
                }
                else
                {
                    if(lossy)
                    {
                        block->globOptData.liveLossyInt32Syms->Set(varSym->m_id);
                    }

                    StackSym *int32Sym = varSym->GetInt32EquivSym(instr->m_func);
                    IR::RegOpnd *int32Reg = IR::RegOpnd::New(int32Sym, TyInt32, instr->m_func);
                    int32Reg->SetIsJITOptimizedReg(true);
                    newInstr = IR::Instr::New(Js::OpCode::Ld_I4, int32Reg, constOpnd, instr->m_func);
                    newInstr->SetByteCodeOffset(instr);
                    instr->InsertBefore(newInstr);
                    if (updateBlockLastInstr)
                    {
                        block->SetLastInstr(newInstr);
                    }
                }
            }
        }
        else
        {
            StackSym *floatSym;

            bool newFloatSym = false;
            StackSym* varSym;
            if (opnd->IsRegOpnd())
            {
                varSym = opnd->AsRegOpnd()->m_sym;
                if (varSym->IsTypeSpec())
                {
                    varSym = varSym->GetVarEquivSym(nullptr);
                    Assert(varSym);
                }
                floatSym = varSym->GetFloat64EquivSym(instr->m_func);
            }
            else
            {
                varSym = block->globOptData.GetCopyPropSym(nullptr, val);
                // If there is no float 64 type specialized sym for this - create a new sym.
                if(!varSym || !block->globOptData.IsFloat64TypeSpecialized(varSym))
                {
                    // Clear the symstore to ensure it's set below to this new symbol
                    this->SetSymStoreDirect(val->GetValueInfo(), nullptr);
                    varSym = StackSym::New(TyVar, instr->m_func);
                    newFloatSym = true;
                }
                floatSym = varSym->GetFloat64EquivSym(instr->m_func);
            }

            IR::RegOpnd *floatReg = IR::RegOpnd::New(floatSym, TyFloat64, instr->m_func);
            floatReg->SetIsJITOptimizedReg(true);

            // If the value is not live - let's load it.
            if(!block->globOptData.liveFloat64Syms->TestAndSet(varSym->m_id))
            {
                newInstr = IR::Instr::New(Js::OpCode::LdC_F8_R8, floatReg, constOpnd, instr->m_func);
                newInstr->SetByteCodeOffset(instr);
                instr->InsertBefore(newInstr);
                if (updateBlockLastInstr)
                {
                    block->SetLastInstr(newInstr);
                }
                if(newFloatSym)
                {
                    block->globOptData.SetValue(val, varSym);
                }

                // Src is always invariant, but check if the dst is, and then hoist.
                if (block->loop &&
                    (
                        (newFloatSym && block->loop->CanHoistInvariants()) ||
                        this->OptIsInvariant(floatReg, block, block->loop, val, false, false)
                    ))
                {
                    Assert(!this->IsLoopPrePass());
                    this->OptHoistInvariant(newInstr, block, block->loop, val, val, nullptr, false);
                }
            }

            if (needReplaceSrc)
            {
                CaptureByteCodeSymUses(instr);
                instr->ReplaceSrc(opnd, floatReg);
            }

        }

        return instr;
    }

    return newInstr;
}

void
GlobOpt::ToVarRegOpnd(IR::RegOpnd *dst, BasicBlock *block)
{
    ToVarStackSym(dst->m_sym, block);
}

void
GlobOpt::ToVarStackSym(StackSym *varSym, BasicBlock *block)
{
    //added another check for sym , in case of asmjs there is mostly no var syms and hence added a new check to see if it is the primary sym
    Assert(!varSym->IsTypeSpec());

    block->globOptData.liveVarSyms->Set(varSym->m_id);
    block->globOptData.liveInt32Syms->Clear(varSym->m_id);
    block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id);
    block->globOptData.liveFloat64Syms->Clear(varSym->m_id);

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    block->globOptData.liveSimd128F4Syms->Clear(varSym->m_id);
    block->globOptData.liveSimd128I4Syms->Clear(varSym->m_id);
#endif
}

void
GlobOpt::ToInt32Dst(IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block)
{
    StackSym *varSym = dst->m_sym;
    Assert(!varSym->IsTypeSpec());

    if (!this->IsLoopPrePass() && varSym->IsVar())
    {
        StackSym *int32Sym = varSym->GetInt32EquivSym(instr->m_func);

        // Use UnlinkDst / SetDst to make sure isSingleDef is tracked properly,
        // since we'll just be hammering the symbol.
        dst = instr->UnlinkDst()->AsRegOpnd();
        dst->m_sym = int32Sym;
        dst->SetType(TyInt32);
        instr->SetDst(dst);
    }

    block->globOptData.liveInt32Syms->Set(varSym->m_id);
    block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id); // The store makes it lossless
    block->globOptData.liveVarSyms->Clear(varSym->m_id);
    block->globOptData.liveFloat64Syms->Clear(varSym->m_id);

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    block->globOptData.liveSimd128F4Syms->Clear(varSym->m_id);
    block->globOptData.liveSimd128I4Syms->Clear(varSym->m_id);
#endif
}

void
GlobOpt::ToUInt32Dst(IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block)
{
    // We should be calling only for asmjs function
    Assert(GetIsAsmJSFunc());
    StackSym *varSym = dst->m_sym;
    Assert(!varSym->IsTypeSpec());
    block->globOptData.liveInt32Syms->Set(varSym->m_id);
    block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id); // The store makes it lossless
    block->globOptData.liveVarSyms->Clear(varSym->m_id);
    block->globOptData.liveFloat64Syms->Clear(varSym->m_id);

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    block->globOptData.liveSimd128F4Syms->Clear(varSym->m_id);
    block->globOptData.liveSimd128I4Syms->Clear(varSym->m_id);
#endif
}

void
GlobOpt::ToFloat64Dst(IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block)
{
    StackSym *varSym = dst->m_sym;
    Assert(!varSym->IsTypeSpec());

    if (!this->IsLoopPrePass() && varSym->IsVar())
    {
        StackSym *float64Sym = varSym->GetFloat64EquivSym(this->func);

        // Use UnlinkDst / SetDst to make sure isSingleDef is tracked properly,
        // since we'll just be hammering the symbol.
        dst = instr->UnlinkDst()->AsRegOpnd();
        dst->m_sym = float64Sym;
        dst->SetType(TyFloat64);
        instr->SetDst(dst);
    }

    block->globOptData.liveFloat64Syms->Set(varSym->m_id);
    block->globOptData.liveVarSyms->Clear(varSym->m_id);
    block->globOptData.liveInt32Syms->Clear(varSym->m_id);
    block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id);

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    block->globOptData.liveSimd128F4Syms->Clear(varSym->m_id);
    block->globOptData.liveSimd128I4Syms->Clear(varSym->m_id);
#endif
}

#ifdef ENABLE_SIMDJS
// SIMD_JS
void
GlobOpt::ToSimd128Dst(IRType toType, IR::Instr *instr, IR::RegOpnd *dst, BasicBlock *block)
{
    StackSym *varSym = dst->m_sym;
    Assert(!varSym->IsTypeSpec());
    BVSparse<JitArenaAllocator> * livenessBV = block->globOptData.GetSimd128LivenessBV(toType);

    Assert(livenessBV);

    if (!this->IsLoopPrePass() && varSym->IsVar())
    {
        StackSym *simd128Sym = varSym->GetSimd128EquivSym(toType, this->func);

        // Use UnlinkDst / SetDst to make sure isSingleDef is tracked properly,
        // since we'll just be hammering the symbol.
        dst = instr->UnlinkDst()->AsRegOpnd();
        dst->m_sym = simd128Sym;
        dst->SetType(toType);
        instr->SetDst(dst);
    }

    block->globOptData.liveFloat64Syms->Clear(varSym->m_id);
    block->globOptData.liveVarSyms->Clear(varSym->m_id);
    block->globOptData.liveInt32Syms->Clear(varSym->m_id);
    block->globOptData.liveLossyInt32Syms->Clear(varSym->m_id);

    // SIMD_JS
    block->globOptData.liveSimd128F4Syms->Clear(varSym->m_id);
    block->globOptData.liveSimd128I4Syms->Clear(varSym->m_id);

    livenessBV->Set(varSym->m_id);
}
#endif

static void SetIsConstFlag(StackSym* dstSym, int64 value)
{
    Assert(dstSym);
    dstSym->SetIsInt64Const();
}

static void SetIsConstFlag(StackSym* dstSym, int value)
{
    Assert(dstSym);
    dstSym->SetIsIntConst(value);
}

static IR::Opnd* CreateIntConstOpnd(IR::Instr* instr, int64 value) 
{
    return (IR::Opnd*)IR::Int64ConstOpnd::New(value, instr->GetDst()->GetType(), instr->m_func);
}

static IR::Opnd* CreateIntConstOpnd(IR::Instr* instr, int value)
{
    IntConstType constVal;
    if (instr->GetDst()->IsUnsigned())
    {
        // we should zero extend in case of uint
        constVal = (uint32)value;
    }
    else
    {
        constVal = value;
    }
    return (IR::Opnd*)IR::IntConstOpnd::New(constVal, instr->GetDst()->GetType(), instr->m_func);
}

template <typename T>
IR::Opnd* GlobOpt::ReplaceWConst(IR::Instr **pInstr, T value, Value **pDstVal)
{
    IR::Instr * &instr = *pInstr;
    IR::Opnd * constOpnd = CreateIntConstOpnd(instr, value);

    instr->ReplaceSrc1(constOpnd);
    instr->FreeSrc2();

    this->OptSrc(constOpnd, &instr);

    IR::Opnd *dst = instr->GetDst();
    StackSym *dstSym = dst->AsRegOpnd()->m_sym;
    if (dstSym->IsSingleDef())
    {
        SetIsConstFlag(dstSym, value);
    }

    GOPT_TRACE_INSTR(instr, _u("Constant folding to %d: \n"), value);
    *pDstVal = GetIntConstantValue(value, instr, dst);
    return dst;
}

template <typename T>
bool GlobOpt::OptConstFoldBinaryWasm(
    IR::Instr** pInstr,
    const Value* src1,
    const Value* src2,
    Value **pDstVal)
{
    IR::Instr* &instr = *pInstr;

    if (!DoConstFold())
    {
        return false;
    }

    T src1IntConstantValue, src2IntConstantValue;
    if (!src1 || !src1->GetValueInfo()->TryGetIntConstantValue(&src1IntConstantValue, false) || //a bit sketchy: false for int32 means likelyInt = false 
        !src2 || !src2->GetValueInfo()->TryGetIntConstantValue(&src2IntConstantValue, false)    //and unsigned = false for int64
        )
    {
        return false;
    }

    int64 tmpValueOut;
    if (!instr->BinaryCalculatorT<T>(src1IntConstantValue, src2IntConstantValue, &tmpValueOut, func->GetJITFunctionBody()->IsWasmFunction()))
    {
        return false;
    }

    this->CaptureByteCodeSymUses(instr);

    IR::Opnd *dst = (instr->GetDst()->IsInt64()) ? //dst can be int32 for int64 comparison operators
        ReplaceWConst(pInstr, tmpValueOut, pDstVal) :
        ReplaceWConst(pInstr, (int)tmpValueOut, pDstVal);

    instr->m_opcode = Js::OpCode::Ld_I4;
    this->ToInt32Dst(instr, dst->AsRegOpnd(), this->currentBlock);
    return true;
}

bool
GlobOpt::OptConstFoldBinary(
    IR::Instr * *pInstr,
    const IntConstantBounds &src1IntConstantBounds,
    const IntConstantBounds &src2IntConstantBounds,
    Value **pDstVal)
{
    IR::Instr * &instr = *pInstr;
    int32 value;
    IR::IntConstOpnd *constOpnd;

    if (!DoConstFold())
    {
        return false;
    }

    int32 src1IntConstantValue = -1;
    int32 src2IntConstantValue = -1;

    int32 src1MaxIntConstantValue = -1;
    int32 src2MaxIntConstantValue = -1;
    int32 src1MinIntConstantValue = -1;
    int32 src2MinIntConstantValue = -1;

    if (instr->IsBranchInstr())
    {
        src1MinIntConstantValue = src1IntConstantBounds.LowerBound();
        src1MaxIntConstantValue = src1IntConstantBounds.UpperBound();
        src2MinIntConstantValue = src2IntConstantBounds.LowerBound();
        src2MaxIntConstantValue = src2IntConstantBounds.UpperBound();
    }
    else if (src1IntConstantBounds.IsConstant() && src2IntConstantBounds.IsConstant())
    {
        src1IntConstantValue = src1IntConstantBounds.LowerBound();
        src2IntConstantValue = src2IntConstantBounds.LowerBound();
    }
    else
    {
        return false;
    }

    IntConstType tmpValueOut;
    if (!instr->BinaryCalculator(src1IntConstantValue, src2IntConstantValue, &tmpValueOut)
        || !Math::FitsInDWord(tmpValueOut))
    {
        return false;
    }

    value = (int32)tmpValueOut;

    this->CaptureByteCodeSymUses(instr);
    constOpnd = IR::IntConstOpnd::New(value, TyInt32, instr->m_func);
    instr->ReplaceSrc1(constOpnd);
    instr->FreeSrc2();

    this->OptSrc(constOpnd, &instr);

    IR::Opnd *dst = instr->GetDst();
    Assert(dst->IsRegOpnd());

    StackSym *dstSym = dst->AsRegOpnd()->m_sym;

    if (dstSym->IsSingleDef())
    {
        dstSym->SetIsIntConst(value);
    }

    GOPT_TRACE_INSTR(instr, _u("Constant folding to %d: \n"), value);

    *pDstVal = GetIntConstantValue(value, instr, dst);

    if (IsTypeSpecPhaseOff(this->func))
    {
        instr->m_opcode = Js::OpCode::LdC_A_I4;
        this->ToVarRegOpnd(dst->AsRegOpnd(), this->currentBlock);
    }
    else
    {
        instr->m_opcode = Js::OpCode::Ld_I4;
        this->ToInt32Dst(instr, dst->AsRegOpnd(), this->currentBlock);
    }

    return true;
}

void
GlobOpt::OptConstFoldBr(bool test, IR::Instr *instr, Value * src1Val, Value * src2Val)
{
    GOPT_TRACE_INSTR(instr, _u("Constant folding to branch: "));
    BasicBlock *deadBlock;

    if (src1Val)
    {
        this->ToInt32(instr, instr->GetSrc1(), this->currentBlock, src1Val, nullptr, false);
    }

    if (src2Val)
    {
        this->ToInt32(instr, instr->GetSrc2(), this->currentBlock, src2Val, nullptr, false);
    }

    this->CaptureByteCodeSymUses(instr);

    if (test)
    {
        instr->m_opcode = Js::OpCode::Br;

        instr->FreeSrc1();
        if(instr->GetSrc2())
        {
            instr->FreeSrc2();
        }
        deadBlock = instr->m_next->AsLabelInstr()->GetBasicBlock();
    }
    else
    {
        AssertMsg(instr->m_next->IsLabelInstr(), "Next instr of branch should be a label...");
        if(instr->AsBranchInstr()->IsMultiBranch())
        {
            return;
        }
        deadBlock = instr->AsBranchInstr()->GetTarget()->GetBasicBlock();
        instr->FreeSrc1();
        if(instr->GetSrc2())
        {
            instr->FreeSrc2();
        }
        instr->m_opcode = Js::OpCode::Nop;
    }

    // Loop back edge: we would have already decremented data use count for the tail block when we processed the loop header.
    if (!(this->currentBlock->loop && this->currentBlock->loop->GetHeadBlock() == deadBlock))
    {
        this->currentBlock->DecrementDataUseCount();
    }

    this->currentBlock->RemoveDeadSucc(deadBlock, this->func->m_fg);

    if (deadBlock->GetPredList()->Count() == 0)
    {
        deadBlock->SetDataUseCount(0);
    }
}

void
GlobOpt::ChangeValueType(
    BasicBlock *const block,
    Value *const value,
    const ValueType newValueType,
    const bool preserveSubclassInfo,
    const bool allowIncompatibleType) const
{
    Assert(value);
    // Why are we trying to change the value type of the type sym value? Asserting here to make sure we don't deep copy the type sym's value info.
    Assert(!value->GetValueInfo()->IsJsType());

    ValueInfo *const valueInfo = value->GetValueInfo();
    const ValueType valueType(valueInfo->Type());
    if(valueType == newValueType && (preserveSubclassInfo || valueInfo->IsGeneric()))
    {
        return;
    }

    // ArrayValueInfo has information specific to the array type, so make sure that doesn't change
    Assert(
        !preserveSubclassInfo ||
        !valueInfo->IsArrayValueInfo() ||
        newValueType.IsObject() && newValueType.GetObjectType() == valueInfo->GetObjectType());

    Assert(!valueInfo->GetSymStore() || !valueInfo->GetSymStore()->IsStackSym() || !valueInfo->GetSymStore()->AsStackSym()->IsFromByteCodeConstantTable());

    ValueInfo *const newValueInfo =
        preserveSubclassInfo
            ? valueInfo->Copy(alloc)
            : valueInfo->CopyWithGenericStructureKind(alloc);
    newValueInfo->Type() = newValueType;
    ChangeValueInfo(block, value, newValueInfo, allowIncompatibleType);
}

void
GlobOpt::ChangeValueInfo(BasicBlock *const block, Value *const value, ValueInfo *const newValueInfo, const bool allowIncompatibleType, const bool compensated) const
{
    Assert(value);
    Assert(newValueInfo);

    // The value type must be changed to something more specific or something more generic. For instance, it would be changed to
    // something more specific if the current value type is LikelyArray and checks have been done to ensure that it's an array,
    // and it would be changed to something more generic if a call kills the Array value type and it must be treated as
    // LikelyArray going forward.

    // There are cases where we change the type because of different profile information, and because of rejit, these profile information
    // may conflict. Need to allow incompatible type in those cause. However, the old type should be indefinite.
    Assert((allowIncompatibleType && !value->GetValueInfo()->IsDefinite()) ||
        AreValueInfosCompatible(newValueInfo, value->GetValueInfo()));

    // ArrayValueInfo has information specific to the array type, so make sure that doesn't change
    Assert(
        !value->GetValueInfo()->IsArrayValueInfo() ||
        !newValueInfo->IsArrayValueInfo() ||
        newValueInfo->GetObjectType() == value->GetValueInfo()->GetObjectType());

    if(block)
    {
        TrackValueInfoChangeForKills(block, value, newValueInfo, compensated);
    }
    value->SetValueInfo(newValueInfo);
}

bool
GlobOpt::AreValueInfosCompatible(const ValueInfo *const v0, const ValueInfo *const v1) const
{
    Assert(v0);
    Assert(v1);

    if(v0->IsUninitialized() || v1->IsUninitialized())
    {
        return true;
    }

    const bool doAggressiveIntTypeSpec = DoAggressiveIntTypeSpec();
    if(doAggressiveIntTypeSpec && (v0->IsInt() || v1->IsInt()))
    {
        // Int specialization in some uncommon loop cases involving dependencies, needs to allow specializing values of
        // arbitrary types, even values that are definitely not int, to compensate for aggressive assumptions made by a loop
        // prepass
        return true;
    }
    if ((v0->Type()).IsMixedTypedArrayPair(v1->Type()) || (v1->Type()).IsMixedTypedArrayPair(v0->Type()))
    {
        return true;
    }
    const bool doFloatTypeSpec = DoFloatTypeSpec();
    if(doFloatTypeSpec && (v0->IsFloat() || v1->IsFloat()))
    {
        // Float specialization allows specializing values of arbitrary types, even values that are definitely not float
        return true;
    }

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    if (SIMD128_TYPE_SPEC_FLAG && v0->Type().IsSimd128())
    {
        // We only type-spec Undefined values, Objects (possibly merged SIMD values), or actual SIMD values.

        if (v1->Type().IsLikelyUndefined() || v1->Type().IsLikelyNull())
        {
            return true;
        }

        if (v1->Type().IsLikelyObject() && v1->Type().GetObjectType() == ObjectType::Object)
        {
            return true;
        }

        if (v1->Type().IsSimd128())
        {
            return v0->Type().GetObjectType() == v1->Type().GetObjectType();
        }
    }
#endif

    const bool doArrayMissingValueCheckHoist = DoArrayMissingValueCheckHoist();
    const bool doNativeArrayTypeSpec = DoNativeArrayTypeSpec();
    const auto AreValueTypesCompatible = [=](const ValueType t0, const ValueType t1)
    {
        return
            t0.IsSubsetOf(t1, doAggressiveIntTypeSpec, doFloatTypeSpec, doArrayMissingValueCheckHoist, doNativeArrayTypeSpec) ||
            t1.IsSubsetOf(t0, doAggressiveIntTypeSpec, doFloatTypeSpec, doArrayMissingValueCheckHoist, doNativeArrayTypeSpec);
    };

    const ValueType t0(v0->Type().ToDefinite()), t1(v1->Type().ToDefinite());
    if(t0.IsLikelyObject() && t1.IsLikelyObject())
    {
        // Check compatibility for the primitive portions and the object portions of the value types separately
        if(AreValueTypesCompatible(t0.ToDefiniteObject(), t1.ToDefiniteObject()) &&
            (
                !t0.HasBeenPrimitive() ||
                !t1.HasBeenPrimitive() ||
                AreValueTypesCompatible(t0.ToDefinitePrimitiveSubset(), t1.ToDefinitePrimitiveSubset())
            ))
        {
            return true;
        }
    }
    else if(AreValueTypesCompatible(t0, t1))
    {
        return true;
    }

    const FloatConstantValueInfo *floatConstantValueInfo;
    const ValueInfo *likelyIntValueinfo;
    if(v0->IsFloatConstant() && v1->IsLikelyInt())
    {
        floatConstantValueInfo = v0->AsFloatConstant();
        likelyIntValueinfo = v1;
    }
    else if(v0->IsLikelyInt() && v1->IsFloatConstant())
    {
        floatConstantValueInfo = v1->AsFloatConstant();
        likelyIntValueinfo = v0;
    }
    else
    {
        return false;
    }

    // A float constant value with a value that is actually an int is a subset of a likely-int value.
    // Ideally, we should create an int constant value for this up front, such that IsInt() also returns true. There
    // were other issues with that, should see if that can be done.
    int32 int32Value;
    return
        Js::JavascriptNumber::TryGetInt32Value(floatConstantValueInfo->FloatValue(), &int32Value) &&
        (!likelyIntValueinfo->IsLikelyTaggedInt() || !Js::TaggedInt::IsOverflow(int32Value));
}

#if DBG
void
GlobOpt::VerifyArrayValueInfoForTracking(
    const ValueInfo *const valueInfo,
    const bool isJsArray,
    const BasicBlock *const block,
    const bool ignoreKnownImplicitCalls) const
{
    Assert(valueInfo);
    Assert(valueInfo->IsAnyOptimizedArray());
    Assert(isJsArray == valueInfo->IsArrayOrObjectWithArray());
    Assert(!isJsArray == valueInfo->IsOptimizedTypedArray());
    Assert(block);

    Loop *implicitCallsLoop;
    if(block->next && !block->next->isDeleted && block->next->isLoopHeader)
    {
        // Since a loop's landing pad does not have user code, determine whether disabling implicit calls is allowed in the
        // landing pad based on the loop for which this block is the landing pad.
        implicitCallsLoop = block->next->loop;
        Assert(implicitCallsLoop);
        Assert(implicitCallsLoop->landingPad == block);
    }
    else
    {
        implicitCallsLoop = block->loop;
    }

    Assert(
        !isJsArray ||
        DoArrayCheckHoist(valueInfo->Type(), implicitCallsLoop) ||
        (
            ignoreKnownImplicitCalls &&
            !(implicitCallsLoop ? ImplicitCallFlagsAllowOpts(implicitCallsLoop) : ImplicitCallFlagsAllowOpts(func))
        ));
    Assert(!(isJsArray && valueInfo->HasNoMissingValues() && !DoArrayMissingValueCheckHoist()));
    Assert(
        !(
            valueInfo->IsArrayValueInfo() &&
            (
                valueInfo->AsArrayValueInfo()->HeadSegmentSym() ||
                valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym()
            ) &&
            !DoArraySegmentHoist(valueInfo->Type())
        ));
#if 0
    // We can't assert here that there is only a head segment length sym if hoisting is allowed in the current block,
    // because we may have propagated the sym forward out of a loop, and hoisting may be allowed inside but not
    // outside the loop.
    Assert(
        isJsArray ||
        !valueInfo->IsArrayValueInfo() ||
        !valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym() ||
        DoTypedArraySegmentLengthHoist(implicitCallsLoop) ||
        ignoreKnownImplicitCalls ||
        (implicitCallsLoop ? ImplicitCallFlagsAllowOpts(implicitCallsLoop) : ImplicitCallFlagsAllowOpts(func))
        );
#endif
    Assert(
        !(
            isJsArray &&
            valueInfo->IsArrayValueInfo() &&
            valueInfo->AsArrayValueInfo()->LengthSym() &&
            !DoArrayLengthHoist()
        ));
}
#endif

void
GlobOpt::TrackNewValueForKills(Value *const value)
{
    Assert(value);

    if(!value->GetValueInfo()->IsAnyOptimizedArray())
    {
        return;
    }

    DoTrackNewValueForKills(value);
}

void
GlobOpt::DoTrackNewValueForKills(Value *const value)
{
    Assert(value);

    ValueInfo *const valueInfo = value->GetValueInfo();
    Assert(valueInfo->IsAnyOptimizedArray());
    Assert(!valueInfo->IsArrayValueInfo());

    // The value and value info here are new, so it's okay to modify the value info in-place
    Assert(!valueInfo->GetSymStore());

    const bool isJsArray = valueInfo->IsArrayOrObjectWithArray();
    Assert(!isJsArray == valueInfo->IsOptimizedTypedArray());

    Loop *implicitCallsLoop;
    if(currentBlock->next && !currentBlock->next->isDeleted && currentBlock->next->isLoopHeader)
    {
        // Since a loop's landing pad does not have user code, determine whether disabling implicit calls is allowed in the
        // landing pad based on the loop for which this block is the landing pad.
        implicitCallsLoop = currentBlock->next->loop;
        Assert(implicitCallsLoop);
        Assert(implicitCallsLoop->landingPad == currentBlock);
    }
    else
    {
        implicitCallsLoop = currentBlock->loop;
    }

    if(isJsArray)
    {
        if(!DoArrayCheckHoist(valueInfo->Type(), implicitCallsLoop))
        {
            // Array opts are disabled for this value type, so treat it as an indefinite value type going forward
            valueInfo->Type() = valueInfo->Type().ToLikely();
            return;
        }

        if(valueInfo->HasNoMissingValues() && !DoArrayMissingValueCheckHoist())
        {
            valueInfo->Type() = valueInfo->Type().SetHasNoMissingValues(false);
        }
    }

#if DBG
    VerifyArrayValueInfoForTracking(valueInfo, isJsArray, currentBlock);
#endif

    if(!isJsArray)
    {
        return;
    }

    // Can't assume going forward that it will definitely be an array without disabling implicit calls, because the
    // array may be transformed into an ES5 array. Since array opts are enabled, implicit calls can be disabled, and we can
    // treat it as a definite value type going forward, but the value needs to be tracked so that something like a call can
    // revert the value type to a likely version.
    CurrentBlockData()->valuesToKillOnCalls->Add(value);
}

void
GlobOpt::TrackCopiedValueForKills(Value *const value)
{
    Assert(value);

    if(!value->GetValueInfo()->IsAnyOptimizedArray())
    {
        return;
    }

    DoTrackCopiedValueForKills(value);
}

void
GlobOpt::DoTrackCopiedValueForKills(Value *const value)
{
    Assert(value);

    ValueInfo *const valueInfo = value->GetValueInfo();
    Assert(valueInfo->IsAnyOptimizedArray());

    const bool isJsArray = valueInfo->IsArrayOrObjectWithArray();
    Assert(!isJsArray == valueInfo->IsOptimizedTypedArray());

#if DBG
    VerifyArrayValueInfoForTracking(valueInfo, isJsArray, currentBlock);
#endif

    if(!isJsArray && !(valueInfo->IsArrayValueInfo() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym()))
    {
        return;
    }

    // Can't assume going forward that it will definitely be an array without disabling implicit calls, because the
    // array may be transformed into an ES5 array. Since array opts are enabled, implicit calls can be disabled, and we can
    // treat it as a definite value type going forward, but the value needs to be tracked so that something like a call can
    // revert the value type to a likely version.
    CurrentBlockData()->valuesToKillOnCalls->Add(value);
}

void
GlobOpt::TrackMergedValueForKills(
    Value *const value,
    GlobOptBlockData *const blockData,
    BVSparse<JitArenaAllocator> *const mergedValueTypesTrackedForKills) const
{
    Assert(value);

    if(!value->GetValueInfo()->IsAnyOptimizedArray())
    {
        return;
    }

    DoTrackMergedValueForKills(value, blockData, mergedValueTypesTrackedForKills);
}

void
GlobOpt::DoTrackMergedValueForKills(
    Value *const value,
    GlobOptBlockData *const blockData,
    BVSparse<JitArenaAllocator> *const mergedValueTypesTrackedForKills) const
{
    Assert(value);
    Assert(blockData);

    ValueInfo *valueInfo = value->GetValueInfo();
    Assert(valueInfo->IsAnyOptimizedArray());

    const bool isJsArray = valueInfo->IsArrayOrObjectWithArray();
    Assert(!isJsArray == valueInfo->IsOptimizedTypedArray());

#if DBG
    VerifyArrayValueInfoForTracking(valueInfo, isJsArray, currentBlock, true);
#endif

    if(!isJsArray && !(valueInfo->IsArrayValueInfo() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym()))
    {
        return;
    }

    // Can't assume going forward that it will definitely be an array without disabling implicit calls, because the
    // array may be transformed into an ES5 array. Since array opts are enabled, implicit calls can be disabled, and we can
    // treat it as a definite value type going forward, but the value needs to be tracked so that something like a call can
    // revert the value type to a likely version.
    if(!mergedValueTypesTrackedForKills || !mergedValueTypesTrackedForKills->TestAndSet(value->GetValueNumber()))
    {
        blockData->valuesToKillOnCalls->Add(value);
    }
}

void
GlobOpt::TrackValueInfoChangeForKills(BasicBlock *const block, Value *const value, ValueInfo *const newValueInfo, const bool compensated) const
{
    Assert(block);
    Assert(value);
    Assert(newValueInfo);

    ValueInfo *const oldValueInfo = value->GetValueInfo();

#if DBG
    if(oldValueInfo->IsAnyOptimizedArray())
    {
        VerifyArrayValueInfoForTracking(oldValueInfo, oldValueInfo->IsArrayOrObjectWithArray(), block, compensated);
    }
#endif

    const bool trackOldValueInfo =
        oldValueInfo->IsArrayOrObjectWithArray() ||
        (
            oldValueInfo->IsOptimizedTypedArray() &&
            oldValueInfo->IsArrayValueInfo() &&
            oldValueInfo->AsArrayValueInfo()->HeadSegmentLengthSym()
        );
    Assert(trackOldValueInfo == block->globOptData.valuesToKillOnCalls->ContainsKey(value));

#if DBG
    if(newValueInfo->IsAnyOptimizedArray())
    {
        VerifyArrayValueInfoForTracking(newValueInfo, newValueInfo->IsArrayOrObjectWithArray(), block, compensated);
    }
#endif

    const bool trackNewValueInfo =
        newValueInfo->IsArrayOrObjectWithArray() ||
        (
            newValueInfo->IsOptimizedTypedArray() &&
            newValueInfo->IsArrayValueInfo() &&
            newValueInfo->AsArrayValueInfo()->HeadSegmentLengthSym()
        );

    if(trackOldValueInfo == trackNewValueInfo)
    {
        return;
    }

    if(trackNewValueInfo)
    {
        block->globOptData.valuesToKillOnCalls->Add(value);
    }
    else
    {
        block->globOptData.valuesToKillOnCalls->Remove(value);
    }
}

void
GlobOpt::ProcessValueKills(IR::Instr *const instr)
{
    Assert(instr);

    ValueSet *const valuesToKillOnCalls = CurrentBlockData()->valuesToKillOnCalls;
    if(!IsLoopPrePass() && valuesToKillOnCalls->Count() == 0)
    {
        return;
    }

    const JsArrayKills kills = CheckJsArrayKills(instr);
    Assert(!kills.KillsArrayHeadSegments() || kills.KillsArrayHeadSegmentLengths());

    if(IsLoopPrePass())
    {
        rootLoopPrePass->jsArrayKills = rootLoopPrePass->jsArrayKills.Merge(kills);
        Assert(
            !rootLoopPrePass->parent ||
            rootLoopPrePass->jsArrayKills.AreSubsetOf(rootLoopPrePass->parent->jsArrayKills));
        if(kills.KillsAllArrays())
        {
            rootLoopPrePass->needImplicitCallBailoutChecksForJsArrayCheckHoist = false;
        }

        if(valuesToKillOnCalls->Count() == 0)
        {
            return;
        }
    }

    if(kills.KillsAllArrays())
    {
        Assert(kills.KillsTypedArrayHeadSegmentLengths());

        // - Calls need to kill the value types of values in the following list. For instance, calls can transform a JS array
        //   into an ES5 array, so any definitely-array value types need to be killed. Update the value types.
        // - Calls also need to kill typed array head segment lengths. A typed array's array buffer may be transferred to a web
        //   worker, in which case the typed array's length is set to zero.
        for(auto it = valuesToKillOnCalls->GetIterator(); it.IsValid(); it.MoveNext())
        {
            Value *const value = it.CurrentValue();
            ValueInfo *const valueInfo = value->GetValueInfo();
            Assert(
                valueInfo->IsArrayOrObjectWithArray() ||
                valueInfo->IsOptimizedTypedArray() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym());
            if(valueInfo->IsArrayOrObjectWithArray())
            {
                ChangeValueType(nullptr, value, valueInfo->Type().ToLikely(), false);
                continue;
            }
            ChangeValueInfo(
                nullptr,
                value,
                valueInfo->AsArrayValueInfo()->Copy(alloc, true, false /* copyHeadSegmentLength */, true));
        }
        valuesToKillOnCalls->Clear();
        return;
    }

    if(kills.KillsArraysWithNoMissingValues())
    {
        // Some operations may kill arrays with no missing values in unlikely circumstances. Convert their value types to likely
        // versions so that the checks have to be redone.
        for(auto it = valuesToKillOnCalls->GetIteratorWithRemovalSupport(); it.IsValid(); it.MoveNext())
        {
            Value *const value = it.CurrentValue();
            ValueInfo *const valueInfo = value->GetValueInfo();
            Assert(
                valueInfo->IsArrayOrObjectWithArray() ||
                valueInfo->IsOptimizedTypedArray() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym());
            if(!valueInfo->IsArrayOrObjectWithArray() || !valueInfo->HasNoMissingValues())
            {
                continue;
            }
            ChangeValueType(nullptr, value, valueInfo->Type().ToLikely(), false);
            it.RemoveCurrent();
        }
    }

    if(kills.KillsNativeArrays())
    {
        // Some operations may kill native arrays in (what should be) unlikely circumstances. Convert their value types to
        // likely versions so that the checks have to be redone.
        for(auto it = valuesToKillOnCalls->GetIteratorWithRemovalSupport(); it.IsValid(); it.MoveNext())
        {
            Value *const value = it.CurrentValue();
            ValueInfo *const valueInfo = value->GetValueInfo();
            Assert(
                valueInfo->IsArrayOrObjectWithArray() ||
                valueInfo->IsOptimizedTypedArray() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym());
            if(!valueInfo->IsArrayOrObjectWithArray() || valueInfo->HasVarElements())
            {
                continue;
            }
            ChangeValueType(nullptr, value, valueInfo->Type().ToLikely(), false);
            it.RemoveCurrent();
        }
    }

    const bool likelyKillsJsArraysWithNoMissingValues = IsOperationThatLikelyKillsJsArraysWithNoMissingValues(instr);
    if(!kills.KillsArrayHeadSegmentLengths())
    {
        Assert(!kills.KillsArrayHeadSegments());
        if(!likelyKillsJsArraysWithNoMissingValues && !kills.KillsArrayLengths())
        {
            return;
        }
    }

    for(auto it = valuesToKillOnCalls->GetIterator(); it.IsValid(); it.MoveNext())
    {
        Value *const value = it.CurrentValue();
        ValueInfo *valueInfo = value->GetValueInfo();
        Assert(
            valueInfo->IsArrayOrObjectWithArray() ||
            valueInfo->IsOptimizedTypedArray() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym());
        if(!valueInfo->IsArrayOrObjectWithArray())
        {
            continue;
        }

        if(likelyKillsJsArraysWithNoMissingValues && valueInfo->HasNoMissingValues())
        {
            ChangeValueType(nullptr, value, valueInfo->Type().SetHasNoMissingValues(false), true);
            valueInfo = value->GetValueInfo();
        }

        if(!valueInfo->IsArrayValueInfo())
        {
            continue;
        }

        ArrayValueInfo *const arrayValueInfo = valueInfo->AsArrayValueInfo();
        const bool removeHeadSegment = kills.KillsArrayHeadSegments() && arrayValueInfo->HeadSegmentSym();
        const bool removeHeadSegmentLength = kills.KillsArrayHeadSegmentLengths() && arrayValueInfo->HeadSegmentLengthSym();
        const bool removeLength = kills.KillsArrayLengths() && arrayValueInfo->LengthSym();
        if(removeHeadSegment || removeHeadSegmentLength || removeLength)
        {
            ChangeValueInfo(
                nullptr,
                value,
                arrayValueInfo->Copy(alloc, !removeHeadSegment, !removeHeadSegmentLength, !removeLength));
            valueInfo = value->GetValueInfo();
        }
    }
}

void
GlobOpt::ProcessValueKills(BasicBlock *const block, GlobOptBlockData *const blockData)
{
    Assert(block);
    Assert(blockData);

    ValueSet *const valuesToKillOnCalls = blockData->valuesToKillOnCalls;
    if(!IsLoopPrePass() && valuesToKillOnCalls->Count() == 0)
    {
        return;
    }

    // If the current block or loop has implicit calls, kill all definitely-array value types, as using that info will cause
    // implicit calls to be disabled, resulting in unnecessary bailouts
    const bool killValuesOnImplicitCalls =
        (block->loop ? !this->ImplicitCallFlagsAllowOpts(block->loop) : !this->ImplicitCallFlagsAllowOpts(func));
    if (!killValuesOnImplicitCalls)
    {
        return;
    }

    if(IsLoopPrePass() && block->loop == rootLoopPrePass)
    {
        AnalysisAssert(rootLoopPrePass);

        for (Loop * loop = rootLoopPrePass; loop != nullptr; loop = loop->parent)
        {
            loop->jsArrayKills.SetKillsAllArrays();
        }
        Assert(!rootLoopPrePass->parent || rootLoopPrePass->jsArrayKills.AreSubsetOf(rootLoopPrePass->parent->jsArrayKills));

        if(valuesToKillOnCalls->Count() == 0)
        {
            return;
        }
    }

    for(auto it = valuesToKillOnCalls->GetIterator(); it.IsValid(); it.MoveNext())
    {
        Value *const value = it.CurrentValue();
        ValueInfo *const valueInfo = value->GetValueInfo();
        Assert(
            valueInfo->IsArrayOrObjectWithArray() ||
            valueInfo->IsOptimizedTypedArray() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym());
        if(valueInfo->IsArrayOrObjectWithArray())
        {
            ChangeValueType(nullptr, value, valueInfo->Type().ToLikely(), false);
            continue;
        }
        ChangeValueInfo(
            nullptr,
            value,
            valueInfo->AsArrayValueInfo()->Copy(alloc, true, false /* copyHeadSegmentLength */, true));
    }
    valuesToKillOnCalls->Clear();
}

void
GlobOpt::ProcessValueKillsForLoopHeaderAfterBackEdgeMerge(BasicBlock *const block, GlobOptBlockData *const blockData)
{
    Assert(block);
    Assert(block->isLoopHeader);
    Assert(blockData);

    ValueSet *const valuesToKillOnCalls = blockData->valuesToKillOnCalls;
    if(valuesToKillOnCalls->Count() == 0)
    {
        return;
    }

    const JsArrayKills loopKills(block->loop->jsArrayKills);
    for(auto it = valuesToKillOnCalls->GetIteratorWithRemovalSupport(); it.IsValid(); it.MoveNext())
    {
        Value *const value = it.CurrentValue();
        ValueInfo *valueInfo = value->GetValueInfo();
        Assert(
            valueInfo->IsArrayOrObjectWithArray() ||
            valueInfo->IsOptimizedTypedArray() && valueInfo->AsArrayValueInfo()->HeadSegmentLengthSym());

        const bool isJsArray = valueInfo->IsArrayOrObjectWithArray();
        Assert(!isJsArray == valueInfo->IsOptimizedTypedArray());

        if(isJsArray ? loopKills.KillsValueType(valueInfo->Type()) : loopKills.KillsTypedArrayHeadSegmentLengths())
        {
            // Hoisting array checks and other related things for this type is disabled for the loop due to the kill, as
            // compensation code is currently not added on back-edges. When merging values from a back-edge, the array value
            // type cannot be definite, as that may require adding compensation code on the back-edge if the optimization pass
            // chooses to not optimize the array.
            if(isJsArray)
            {
                ChangeValueType(nullptr, value, valueInfo->Type().ToLikely(), false);
            }
            else
            {
                ChangeValueInfo(
                    nullptr,
                    value,
                    valueInfo->AsArrayValueInfo()->Copy(alloc, true, false /* copyHeadSegmentLength */, true));
            }
            it.RemoveCurrent();
            continue;
        }

        if(!isJsArray || !valueInfo->IsArrayValueInfo())
        {
            continue;
        }

        // Similarly, if the loop contains an operation that kills JS array segments, don't make the segment or other related
        // syms available initially inside the loop
        ArrayValueInfo *const arrayValueInfo = valueInfo->AsArrayValueInfo();
        const bool removeHeadSegment = loopKills.KillsArrayHeadSegments() && arrayValueInfo->HeadSegmentSym();
        const bool removeHeadSegmentLength = loopKills.KillsArrayHeadSegmentLengths() && arrayValueInfo->HeadSegmentLengthSym();
        const bool removeLength = loopKills.KillsArrayLengths() && arrayValueInfo->LengthSym();
        if(removeHeadSegment || removeHeadSegmentLength || removeLength)
        {
            ChangeValueInfo(
                nullptr,
                value,
                arrayValueInfo->Copy(alloc, !removeHeadSegment, !removeHeadSegmentLength, !removeLength));
            valueInfo = value->GetValueInfo();
        }
    }
}

bool
GlobOpt::NeedBailOnImplicitCallForLiveValues(BasicBlock const * const block, const bool isForwardPass) const
{
    if(isForwardPass)
    {
        return block->globOptData.valuesToKillOnCalls->Count() != 0;
    }

    if(block->noImplicitCallUses->IsEmpty())
    {
        Assert(block->noImplicitCallNoMissingValuesUses->IsEmpty());
        Assert(block->noImplicitCallNativeArrayUses->IsEmpty());
        Assert(block->noImplicitCallJsArrayHeadSegmentSymUses->IsEmpty());
        Assert(block->noImplicitCallArrayLengthSymUses->IsEmpty());
        return false;
    }

    return true;
}

IR::Instr*
GlobOpt::CreateBoundsCheckInstr(IR::Opnd* lowerBound, IR::Opnd* upperBound, int offset, Func* func)
{
    IR::Instr* instr = IR::Instr::New(Js::OpCode::BoundCheck, func);
    return AttachBoundsCheckData(instr, lowerBound, upperBound, offset);
}

IR::Instr*
GlobOpt::CreateBoundsCheckInstr(IR::Opnd* lowerBound, IR::Opnd* upperBound, int offset, IR::BailOutKind bailoutkind, BailOutInfo* bailoutInfo, Func * func)
{
    IR::Instr* instr = IR::BailOutInstr::New(Js::OpCode::BoundCheck, bailoutkind, bailoutInfo, func);
    return AttachBoundsCheckData(instr, lowerBound, upperBound, offset);
}

IR::Instr*
GlobOpt::AttachBoundsCheckData(IR::Instr* instr, IR::Opnd* lowerBound, IR::Opnd* upperBound, int offset)
{
    instr->SetSrc1(lowerBound);
    instr->SetSrc2(upperBound);
    if (offset != 0)
    {
        instr->SetDst(IR::IntConstOpnd::New(offset, TyInt32, instr->m_func));
    }
    return instr;
}

void
GlobOpt::OptArraySrc(IR::Instr * *const instrRef)
{
    Assert(instrRef);
    IR::Instr *&instr = *instrRef;
    Assert(instr);

    IR::Instr *baseOwnerInstr;
    IR::IndirOpnd *baseOwnerIndir;
    IR::RegOpnd *baseOpnd;
    bool isProfilableLdElem, isProfilableStElem;
    bool isLoad, isStore;
    bool needsHeadSegment, needsHeadSegmentLength, needsLength, needsBoundChecks;
    switch(instr->m_opcode)
    {
        // SIMD_JS
        case Js::OpCode::Simd128_LdArr_F4:
        case Js::OpCode::Simd128_LdArr_I4:
            // no type-spec for Asm.js
            if (this->GetIsAsmJSFunc())
            {
                return;
            }
            // fall through
        case Js::OpCode::LdElemI_A:
        case Js::OpCode::LdMethodElem:
            if(!instr->GetSrc1()->IsIndirOpnd())
            {
                return;
            }
            baseOwnerInstr = nullptr;
            baseOwnerIndir = instr->GetSrc1()->AsIndirOpnd();
            baseOpnd = baseOwnerIndir->GetBaseOpnd();
            isProfilableLdElem = instr->m_opcode == Js::OpCode::LdElemI_A; // LdMethodElem is currently not profiled
            isProfilableLdElem |= Js::IsSimd128Load(instr->m_opcode);
            needsBoundChecks = needsHeadSegmentLength = needsHeadSegment = isLoad = true;
            needsLength = isStore = isProfilableStElem = false;
            break;

        // SIMD_JS
        case Js::OpCode::Simd128_StArr_F4:
        case Js::OpCode::Simd128_StArr_I4:
            if (this->GetIsAsmJSFunc())
            {
                return;
            }
            // fall through
        case Js::OpCode::StElemI_A:
        case Js::OpCode::StElemI_A_Strict:
        case Js::OpCode::StElemC:
            if(!instr->GetDst()->IsIndirOpnd())
            {
                return;
            }
            baseOwnerInstr = nullptr;
            baseOwnerIndir = instr->GetDst()->AsIndirOpnd();
            baseOpnd = baseOwnerIndir->GetBaseOpnd();
            needsBoundChecks = isProfilableStElem = instr->m_opcode != Js::OpCode::StElemC;
            isProfilableStElem |= Js::IsSimd128Store(instr->m_opcode);
            needsHeadSegmentLength = needsHeadSegment = isStore = true;
            needsLength = isLoad = isProfilableLdElem = false;
            break;

        case Js::OpCode::InlineArrayPush:
        case Js::OpCode::InlineArrayPop:
        {
            baseOwnerInstr = instr;
            baseOwnerIndir = nullptr;
            IR::Opnd * thisOpnd = instr->GetSrc1();

            // Return if it not a LikelyArray or Object with Array - No point in doing array check elimination.
            if(!thisOpnd->IsRegOpnd() || !thisOpnd->GetValueType().IsLikelyArrayOrObjectWithArray())
            {
                return;
            }
            baseOpnd = thisOpnd->AsRegOpnd();

            isLoad = instr->m_opcode == Js::OpCode::InlineArrayPop;
            isStore = instr->m_opcode == Js::OpCode::InlineArrayPush;
            needsLength = needsHeadSegmentLength = needsHeadSegment = true;
            needsBoundChecks = isProfilableLdElem = isProfilableStElem = false;
            break;
        }

        case Js::OpCode::LdLen_A:
            if(!instr->GetSrc1()->IsRegOpnd())
            {
                return;
            }
            baseOwnerInstr = instr;
            baseOwnerIndir = nullptr;
            baseOpnd = instr->GetSrc1()->AsRegOpnd();
            if(baseOpnd->GetValueType().IsLikelyObject() &&
                baseOpnd->GetValueType().GetObjectType() == ObjectType::ObjectWithArray)
            {
                return;
            }
            needsLength = true;
            needsBoundChecks =
                needsHeadSegmentLength =
                needsHeadSegment =
                isStore =
                isLoad =
                isProfilableStElem =
                isProfilableLdElem = false;
            break;

        default:
            return;
    }
    Assert(!(baseOwnerInstr && baseOwnerIndir));
    Assert(!needsHeadSegmentLength || needsHeadSegment);

    if(baseOwnerIndir && !IsLoopPrePass())
    {
        // Since this happens before type specialization, make sure that any necessary conversions are done, and that the index
        // is int-specialized if possible such that the const flags are correct.
        ToVarUses(instr, baseOwnerIndir, baseOwnerIndir == instr->GetDst(), nullptr);
    }

    if(isProfilableStElem && !IsLoopPrePass())
    {
        // If the dead-store pass decides to add the bailout kind IR::BailOutInvalidatedArrayHeadSegment, and the fast path is
        // generated, it may bail out before the operation is done, so this would need to be a pre-op bailout.
        if(instr->HasBailOutInfo())
        {
            Assert(
                instr->GetByteCodeOffset() != Js::Constants::NoByteCodeOffset &&
                instr->GetBailOutInfo()->bailOutOffset <= instr->GetByteCodeOffset());

            const IR::BailOutKind bailOutKind = instr->GetBailOutKind();
            Assert(
                !(bailOutKind & ~IR::BailOutKindBits) ||
                (bailOutKind & ~IR::BailOutKindBits) == IR::BailOutOnImplicitCallsPreOp);
            if(!(bailOutKind & ~IR::BailOutKindBits))
            {
                instr->SetBailOutKind(bailOutKind + IR::BailOutOnImplicitCallsPreOp);
            }
        }
        else
        {
            GenerateBailAtOperation(&instr, IR::BailOutOnImplicitCallsPreOp);
        }
    }

    Value *const baseValue = CurrentBlockData()->FindValue(baseOpnd->m_sym);
    if(!baseValue)
    {
        return;
    }
    ValueInfo *baseValueInfo = baseValue->GetValueInfo();
    ValueType baseValueType(baseValueInfo->Type());

    baseOpnd->SetValueType(baseValueType);
    if(!baseValueType.IsLikelyAnyOptimizedArray() ||
        !DoArrayCheckHoist(baseValueType, currentBlock->loop, instr) ||
        (baseOwnerIndir && !ShouldExpectConventionalArrayIndexValue(baseOwnerIndir)))
    {
        return;
    }

    const bool isLikelyJsArray = !baseValueType.IsLikelyTypedArray();
    Assert(isLikelyJsArray == baseValueType.IsLikelyArrayOrObjectWithArray());
    Assert(!isLikelyJsArray == baseValueType.IsLikelyOptimizedTypedArray());
    if(!isLikelyJsArray && instr->m_opcode == Js::OpCode::LdMethodElem)
    {
        // Fast path is not generated in this case since the subsequent call will throw
        return;
    }

    ValueType newBaseValueType(baseValueType.ToDefiniteObject());
    if(isLikelyJsArray && newBaseValueType.HasNoMissingValues() && !DoArrayMissingValueCheckHoist())
    {
        newBaseValueType = newBaseValueType.SetHasNoMissingValues(false);
    }
    Assert((newBaseValueType == baseValueType) == baseValueType.IsObject());

    ArrayValueInfo *baseArrayValueInfo = nullptr;
    const auto UpdateValue = [&](StackSym *newHeadSegmentSym, StackSym *newHeadSegmentLengthSym, StackSym *newLengthSym)
    {
        Assert(baseValueType.GetObjectType() == newBaseValueType.GetObjectType());
        Assert(newBaseValueType.IsObject());
        Assert(baseValueType.IsLikelyArray() || !newLengthSym);

        if(!(newHeadSegmentSym || newHeadSegmentLengthSym || newLengthSym))
        {
            // We're not adding new information to the value other than changing the value type. Preserve any existing
            // information and just change the value type.
            ChangeValueType(currentBlock, baseValue, newBaseValueType, true);
            return;
        }

        // Merge the new syms into the value while preserving any existing information, and change the value type
        if(baseArrayValueInfo)
        {
            if(!newHeadSegmentSym)
            {
                newHeadSegmentSym = baseArrayValueInfo->HeadSegmentSym();
            }
            if(!newHeadSegmentLengthSym)
            {
                newHeadSegmentLengthSym = baseArrayValueInfo->HeadSegmentLengthSym();
            }
            if(!newLengthSym)
            {
                newLengthSym = baseArrayValueInfo->LengthSym();
            }
            Assert(
                !baseArrayValueInfo->HeadSegmentSym() ||
                newHeadSegmentSym == baseArrayValueInfo->HeadSegmentSym());
            Assert(
                !baseArrayValueInfo->HeadSegmentLengthSym() ||
                newHeadSegmentLengthSym == baseArrayValueInfo->HeadSegmentLengthSym());
            Assert(!baseArrayValueInfo->LengthSym() || newLengthSym == baseArrayValueInfo->LengthSym());
        }
        ArrayValueInfo *const newBaseArrayValueInfo =
            ArrayValueInfo::New(
                alloc,
                newBaseValueType,
                newHeadSegmentSym,
                newHeadSegmentLengthSym,
                newLengthSym,
                baseValueInfo->GetSymStore());
        ChangeValueInfo(currentBlock, baseValue, newBaseArrayValueInfo);
    };

    if(IsLoopPrePass())
    {
        if(newBaseValueType != baseValueType)
        {
            UpdateValue(nullptr, nullptr, nullptr);
        }

        // For javascript arrays and objects with javascript arrays:
        //   - Implicit calls need to be disabled and calls cannot be allowed in the loop since the array vtable may be changed
        //     into an ES5 array.
        // For typed arrays:
        //   - A typed array's array buffer may be transferred to a web worker as part of an implicit call, in which case the
        //     typed array's length is set to zero. Implicit calls need to be disabled if the typed array's head segment length
        //     is going to be loaded and used later.
        // Since we don't know if the loop has kills after this instruction, the kill information may not be complete. If a kill
        // is found later, this information will be updated to not require disabling implicit calls.
        if(!(
                isLikelyJsArray
                    ? rootLoopPrePass->jsArrayKills.KillsValueType(newBaseValueType)
                    : rootLoopPrePass->jsArrayKills.KillsTypedArrayHeadSegmentLengths()
            ))
        {
            rootLoopPrePass->needImplicitCallBailoutChecksForJsArrayCheckHoist = true;
        }
        return;
    }

    if(baseValueInfo->IsArrayValueInfo())
    {
        baseArrayValueInfo = baseValueInfo->AsArrayValueInfo();
    }

    const bool doArrayChecks = !baseValueType.IsObject();
    const bool doArraySegmentHoist = DoArraySegmentHoist(baseValueType) && instr->m_opcode != Js::OpCode::StElemC;
    const bool headSegmentIsAvailable = baseArrayValueInfo && baseArrayValueInfo->HeadSegmentSym();
    const bool doHeadSegmentLoad = doArraySegmentHoist && needsHeadSegment && !headSegmentIsAvailable;
    const bool doArraySegmentLengthHoist =
        doArraySegmentHoist && (isLikelyJsArray || DoTypedArraySegmentLengthHoist(currentBlock->loop));
    const bool headSegmentLengthIsAvailable = baseArrayValueInfo && baseArrayValueInfo->HeadSegmentLengthSym();
    const bool doHeadSegmentLengthLoad =
        doArraySegmentLengthHoist &&
        (needsHeadSegmentLength || (!isLikelyJsArray && needsLength)) &&
        !headSegmentLengthIsAvailable;
    const bool lengthIsAvailable = baseArrayValueInfo && baseArrayValueInfo->LengthSym();
    const bool doLengthLoad =
        DoArrayLengthHoist() &&
        needsLength &&
        !lengthIsAvailable &&
        baseValueType.IsLikelyArray() &&
        DoLdLenIntSpec(instr->m_opcode == Js::OpCode::LdLen_A ? instr : nullptr, baseValueType);
    StackSym *const newHeadSegmentSym = doHeadSegmentLoad ? StackSym::New(TyMachPtr, instr->m_func) : nullptr;
    StackSym *const newHeadSegmentLengthSym = doHeadSegmentLengthLoad ? StackSym::New(TyUint32, instr->m_func) : nullptr;
    StackSym *const newLengthSym = doLengthLoad ? StackSym::New(TyUint32, instr->m_func) : nullptr;

    bool canBailOutOnArrayAccessHelperCall;

    if (Js::IsSimd128LoadStore(instr->m_opcode))
    {
        // SIMD_JS
        // simd load/store never call helper
        canBailOutOnArrayAccessHelperCall = true;
    }
    else
    {
        canBailOutOnArrayAccessHelperCall = (isProfilableLdElem || isProfilableStElem) &&
        DoEliminateArrayAccessHelperCall() &&
        !(
            instr->IsProfiledInstr() &&
            (
                isProfilableLdElem
                    ? instr->AsProfiledInstr()->u.ldElemInfo->LikelyNeedsHelperCall()
                    : instr->AsProfiledInstr()->u.stElemInfo->LikelyNeedsHelperCall()
            )
         );
    }

    bool doExtractBoundChecks = false, eliminatedLowerBoundCheck = false, eliminatedUpperBoundCheck = false;
    StackSym *indexVarSym = nullptr;
    Value *indexValue = nullptr;
    IntConstantBounds indexConstantBounds;
    Value *headSegmentLengthValue = nullptr;
    IntConstantBounds headSegmentLengthConstantBounds;

#if ENABLE_FAST_ARRAYBUFFER
    if (baseValueType.IsLikelyOptimizedVirtualTypedArray() && !Js::IsSimd128LoadStore(instr->m_opcode) /*Always extract bounds for SIMD */)
    {
        if (isProfilableStElem ||
            !instr->IsDstNotAlwaysConvertedToInt32() ||
            ( (baseValueType.GetObjectType() == ObjectType::Float32VirtualArray ||
              baseValueType.GetObjectType() == ObjectType::Float64VirtualArray) &&
              !instr->IsDstNotAlwaysConvertedToNumber()
            )
           )
        {
            // Unless we're in asm.js (where it is guaranteed that virtual typed array accesses cannot read/write beyond 4GB),
            // check the range of the index to make sure we won't access beyond the reserved memory beforing eliminating bounds
            // checks in jitted code.
            if (!GetIsAsmJSFunc() && baseOwnerIndir)
            {
                IR::RegOpnd * idxOpnd = baseOwnerIndir->GetIndexOpnd();
                if (idxOpnd)
                {
                    StackSym * idxSym = idxOpnd->m_sym->IsTypeSpec() ? idxOpnd->m_sym->GetVarEquivSym(nullptr) : idxOpnd->m_sym;
                    Value * idxValue = CurrentBlockData()->FindValue(idxSym);
                    IntConstantBounds idxConstantBounds;
                    if (idxValue && idxValue->GetValueInfo()->TryGetIntConstantBounds(&idxConstantBounds))
                    {
                        BYTE indirScale = Lowerer::GetArrayIndirScale(baseValueType);
                        int32 upperBound = idxConstantBounds.UpperBound();
                        int32 lowerBound = idxConstantBounds.LowerBound();
                        if (lowerBound >= 0 && ((static_cast<uint64>(upperBound) << indirScale) < MAX_ASMJS_ARRAYBUFFER_LENGTH))
                        {
                            eliminatedLowerBoundCheck = true;
                            eliminatedUpperBoundCheck = true;
                            canBailOutOnArrayAccessHelperCall = false;
                        }
                    }
                }
            }
            else
            {
                if (!baseOwnerIndir)
                {
                    Assert(instr->m_opcode == Js::OpCode::InlineArrayPush ||
                        instr->m_opcode == Js::OpCode::InlineArrayPop ||
                        instr->m_opcode == Js::OpCode::LdLen_A);
                }
                eliminatedLowerBoundCheck = true;
                eliminatedUpperBoundCheck = true;
                canBailOutOnArrayAccessHelperCall = false;
            }
        }
    }
#endif

    if(needsBoundChecks && DoBoundCheckElimination())
    {
        AnalysisAssert(baseOwnerIndir);
        Assert(needsHeadSegmentLength);

        // Bound checks can be separated from the instruction only if it can bail out instead of making a helper call when a
        // bound check fails. And only if it would bail out, can we use a bound check to eliminate redundant bound checks later
        // on that path.
        doExtractBoundChecks = (headSegmentLengthIsAvailable || doHeadSegmentLengthLoad) && canBailOutOnArrayAccessHelperCall;

        do
        {
            // Get the index value
            IR::RegOpnd *const indexOpnd = baseOwnerIndir->GetIndexOpnd();
            if(indexOpnd)
            {
                StackSym *const indexSym = indexOpnd->m_sym;
                if(indexSym->IsTypeSpec())
                {
                    Assert(indexSym->IsInt32());
                    indexVarSym = indexSym->GetVarEquivSym(nullptr);
                    Assert(indexVarSym);
                    indexValue = CurrentBlockData()->FindValue(indexVarSym);
                    Assert(indexValue);
                    AssertVerify(indexValue->GetValueInfo()->TryGetIntConstantBounds(&indexConstantBounds));
                    Assert(indexOpnd->GetType() == TyInt32 || indexOpnd->GetType() == TyUint32);
                    Assert(
                        (indexOpnd->GetType() == TyUint32) ==
                        ValueInfo::IsGreaterThanOrEqualTo(
                            indexValue,
                            indexConstantBounds.LowerBound(),
                            indexConstantBounds.UpperBound(),
                            nullptr,
                            0,
                            0));
                    if(indexOpnd->GetType() == TyUint32)
                    {
                        eliminatedLowerBoundCheck = true;
                    }
                }
                else
                {
                    doExtractBoundChecks = false; // Bound check instruction operates only on int-specialized operands
                    indexValue = CurrentBlockData()->FindValue(indexSym);
                    if(!indexValue || !indexValue->GetValueInfo()->TryGetIntConstantBounds(&indexConstantBounds))
                    {
                        break;
                    }
                    if(ValueInfo::IsGreaterThanOrEqualTo(
                            indexValue,
                            indexConstantBounds.LowerBound(),
                            indexConstantBounds.UpperBound(),
                            nullptr,
                            0,
                            0))
                    {
                        eliminatedLowerBoundCheck = true;
                    }
                }
                if(!eliminatedLowerBoundCheck &&
                    ValueInfo::IsLessThan(
                        indexValue,
                        indexConstantBounds.LowerBound(),
                        indexConstantBounds.UpperBound(),
                        nullptr,
                        0,
                        0))
                {
                    eliminatedUpperBoundCheck = true;
                    doExtractBoundChecks = false;
                    break;
                }
            }
            else
            {
                const int32 indexConstantValue = baseOwnerIndir->GetOffset();
                if(indexConstantValue < 0)
                {
                    eliminatedUpperBoundCheck = true;
                    doExtractBoundChecks = false;
                    break;
                }
                if(indexConstantValue == INT32_MAX)
                {
                    eliminatedLowerBoundCheck = true;
                    doExtractBoundChecks = false;
                    break;
                }
                indexConstantBounds = IntConstantBounds(indexConstantValue, indexConstantValue);
                eliminatedLowerBoundCheck = true;
            }

            if(!headSegmentLengthIsAvailable)
            {
                break;
            }
            headSegmentLengthValue = CurrentBlockData()->FindValue(baseArrayValueInfo->HeadSegmentLengthSym());
            if(!headSegmentLengthValue)
            {
                if(doExtractBoundChecks)
                {
                    headSegmentLengthConstantBounds = IntConstantBounds(0, Js::SparseArraySegmentBase::MaxLength);
                }
                break;
            }
            AssertVerify(headSegmentLengthValue->GetValueInfo()->TryGetIntConstantBounds(&headSegmentLengthConstantBounds));

            if (ValueInfo::IsLessThanOrEqualTo(
                    indexValue,
                    indexConstantBounds.LowerBound(),
                    indexConstantBounds.UpperBound(),
                    headSegmentLengthValue,
                    headSegmentLengthConstantBounds.LowerBound(),
                    headSegmentLengthConstantBounds.UpperBound(),
                    GetBoundCheckOffsetForSimd(newBaseValueType, instr, -1)
                    ))
            {
                eliminatedUpperBoundCheck = true;
                if(eliminatedLowerBoundCheck)
                {
                    doExtractBoundChecks = false;
                }
            }
        } while(false);
    }

    if(doArrayChecks || doHeadSegmentLoad || doHeadSegmentLengthLoad || doLengthLoad || doExtractBoundChecks)
    {
        // Find the loops out of which array checks and head segment loads need to be hoisted
        Loop *hoistChecksOutOfLoop = nullptr;
        Loop *hoistHeadSegmentLoadOutOfLoop = nullptr;
        Loop *hoistHeadSegmentLengthLoadOutOfLoop = nullptr;
        Loop *hoistLengthLoadOutOfLoop = nullptr;
        if(doArrayChecks || doHeadSegmentLoad || doHeadSegmentLengthLoad || doLengthLoad)
        {
            for(Loop *loop = currentBlock->loop; loop; loop = loop->parent)
            {
                const JsArrayKills loopKills(loop->jsArrayKills);
                Value *baseValueInLoopLandingPad = nullptr;
                if((isLikelyJsArray && loopKills.KillsValueType(newBaseValueType)) ||
                    !OptIsInvariant(baseOpnd->m_sym, currentBlock, loop, baseValue, true, true, &baseValueInLoopLandingPad) ||
                    !(doArrayChecks || baseValueInLoopLandingPad->GetValueInfo()->IsObject()))
                {
                    break;
                }

                // The value types should be the same, except:
                //     - The value type in the landing pad is a type that can merge to a specific object type. Typically, these
                //       cases will use BailOnNoProfile, but that can be disabled due to excessive bailouts. Those value types
                //       merge aggressively to the other side's object type, so the value type may have started off as
                //       Uninitialized, [Likely]Undefined|Null, [Likely]UninitializedObject, etc., and changed in the loop to an
                //       array type during a prepass.
                //     - StElems in the loop can kill the no-missing-values info.
                //     - The native array type may be made more conservative based on profile data by an instruction in the loop.
                Assert(
                    baseValueInLoopLandingPad->GetValueInfo()->CanMergeToSpecificObjectType() ||
                    baseValueInLoopLandingPad->GetValueInfo()->Type().SetCanBeTaggedValue(false) ==
                        baseValueType.SetCanBeTaggedValue(false) ||
                    baseValueInLoopLandingPad->GetValueInfo()->Type().SetHasNoMissingValues(false).SetCanBeTaggedValue(false) ==
                        baseValueType.SetHasNoMissingValues(false).SetCanBeTaggedValue(false) ||
                    baseValueInLoopLandingPad->GetValueInfo()->Type().SetHasNoMissingValues(false).ToLikely().SetCanBeTaggedValue(false) ==
                        baseValueType.SetHasNoMissingValues(false).SetCanBeTaggedValue(false) ||
                    (
                        baseValueInLoopLandingPad->GetValueInfo()->Type().IsLikelyNativeArray() &&
                        baseValueInLoopLandingPad->GetValueInfo()->Type().Merge(baseValueType).SetHasNoMissingValues(false).SetCanBeTaggedValue(false) ==
                            baseValueType.SetHasNoMissingValues(false).SetCanBeTaggedValue(false)
                    ));

                if(doArrayChecks)
                {
                    hoistChecksOutOfLoop = loop;
                }

                if(isLikelyJsArray && loopKills.KillsArrayHeadSegments())
                {
                    Assert(loopKills.KillsArrayHeadSegmentLengths());
                    if(!(doArrayChecks || doLengthLoad))
                    {
                        break;
                    }
                }
                else
                {
                    if(doHeadSegmentLoad || headSegmentIsAvailable)
                    {
                        // If the head segment is already available, we may need to rehoist the value including other
                        // information. So, need to track the loop out of which the head segment length can be hoisted even if
                        // the head segment length is not being loaded here.
                        hoistHeadSegmentLoadOutOfLoop = loop;
                    }

                    if(isLikelyJsArray
                            ? loopKills.KillsArrayHeadSegmentLengths()
                            : loopKills.KillsTypedArrayHeadSegmentLengths())
                    {
                        if(!(doArrayChecks || doHeadSegmentLoad || doLengthLoad))
                        {
                            break;
                        }
                    }
                    else if(doHeadSegmentLengthLoad || headSegmentLengthIsAvailable)
                    {
                        // If the head segment length is already available, we may need to rehoist the value including other
                        // information. So, need to track the loop out of which the head segment length can be hoisted even if
                        // the head segment length is not being loaded here.
                        hoistHeadSegmentLengthLoadOutOfLoop = loop;
                    }
                }

                if(isLikelyJsArray && loopKills.KillsArrayLengths())
                {
                    if(!(doArrayChecks || doHeadSegmentLoad || doHeadSegmentLengthLoad))
                    {
                        break;
                    }
                }
                else if(doLengthLoad || lengthIsAvailable)
                {
                    // If the length is already available, we may need to rehoist the value including other information. So,
                    // need to track the loop out of which the head segment length can be hoisted even if the length is not
                    // being loaded here.
                    hoistLengthLoadOutOfLoop = loop;
                }
            }
        }

        IR::Instr *insertBeforeInstr = instr->GetInsertBeforeByteCodeUsesInstr();
        const auto InsertInstrInLandingPad = [&](IR::Instr *const instr, Loop *const hoistOutOfLoop)
        {
            if(hoistOutOfLoop->bailOutInfo->bailOutInstr)
            {
                instr->SetByteCodeOffset(hoistOutOfLoop->bailOutInfo->bailOutInstr);
                hoistOutOfLoop->bailOutInfo->bailOutInstr->InsertBefore(instr);
            }
            else
            {
                instr->SetByteCodeOffset(hoistOutOfLoop->landingPad->GetLastInstr());
                hoistOutOfLoop->landingPad->InsertAfter(instr);
            }
        };

        BailOutInfo *shareableBailOutInfo = nullptr;
        IR::Instr *shareableBailOutInfoOriginalOwner = nullptr;
        const auto ShareBailOut = [&]()
        {
            Assert(shareableBailOutInfo);
            if(shareableBailOutInfo->bailOutInstr != shareableBailOutInfoOriginalOwner)
            {
                return;
            }

            Assert(shareableBailOutInfoOriginalOwner->GetBailOutInfo() == shareableBailOutInfo);
            IR::Instr *const sharedBailOut = shareableBailOutInfoOriginalOwner->ShareBailOut();
            Assert(sharedBailOut->GetBailOutInfo() == shareableBailOutInfo);
            shareableBailOutInfoOriginalOwner = nullptr;
            sharedBailOut->Unlink();
            insertBeforeInstr->InsertBefore(sharedBailOut);
            insertBeforeInstr = sharedBailOut;
        };

        if(doArrayChecks)
        {
            TRACE_TESTTRACE_PHASE_INSTR(Js::ArrayCheckHoistPhase, instr, _u("Separating array checks with bailout\n"));

            IR::Instr *bailOnNotArray = IR::Instr::New(Js::OpCode::BailOnNotArray, instr->m_func);
            bailOnNotArray->SetSrc1(baseOpnd);
            bailOnNotArray->GetSrc1()->SetIsJITOptimizedReg(true);
            const IR::BailOutKind bailOutKind =
                newBaseValueType.IsLikelyNativeArray() ? IR::BailOutOnNotNativeArray : IR::BailOutOnNotArray;
            if(hoistChecksOutOfLoop)
            {
                Assert(!(isLikelyJsArray && hoistChecksOutOfLoop->jsArrayKills.KillsValueType(newBaseValueType)));

                TRACE_PHASE_INSTR(
                    Js::ArrayCheckHoistPhase,
                    instr,
                    _u("Hoisting array checks with bailout out of loop %u to landing pad block %u\n"),
                    hoistChecksOutOfLoop->GetLoopNumber(),
                    hoistChecksOutOfLoop->landingPad->GetBlockNum());
                TESTTRACE_PHASE_INSTR(Js::ArrayCheckHoistPhase, instr, _u("Hoisting array checks with bailout out of loop\n"));

                Assert(hoistChecksOutOfLoop->bailOutInfo);
                EnsureBailTarget(hoistChecksOutOfLoop);
                InsertInstrInLandingPad(bailOnNotArray, hoistChecksOutOfLoop);
                bailOnNotArray = bailOnNotArray->ConvertToBailOutInstr(hoistChecksOutOfLoop->bailOutInfo, bailOutKind);
            }
            else
            {
                bailOnNotArray->SetByteCodeOffset(instr);
                insertBeforeInstr->InsertBefore(bailOnNotArray);
                GenerateBailAtOperation(&bailOnNotArray, bailOutKind);
                shareableBailOutInfo = bailOnNotArray->GetBailOutInfo();
                shareableBailOutInfoOriginalOwner = bailOnNotArray;
            }

            baseValueType = newBaseValueType;
            baseOpnd->SetValueType(newBaseValueType);
        }

        if(doLengthLoad)
        {
            Assert(baseValueType.IsArray());
            Assert(newLengthSym);

            TRACE_TESTTRACE_PHASE_INSTR(Js::Phase::ArrayLengthHoistPhase, instr, _u("Separating array length load\n"));

            // Create an initial value for the length
            CurrentBlockData()->liveVarSyms->Set(newLengthSym->m_id);
            Value *const lengthValue = NewIntRangeValue(0, INT32_MAX, false);
            CurrentBlockData()->SetValue(lengthValue, newLengthSym);

            // SetValue above would have set the sym store to newLengthSym. This sym won't be used for copy-prop though, so
            // remove it as the sym store.
            this->SetSymStoreDirect(lengthValue->GetValueInfo(), nullptr);

            // length = [array + offsetOf(length)]
            IR::Instr *const loadLength =
                IR::Instr::New(
                    Js::OpCode::LdIndir,
                    IR::RegOpnd::New(newLengthSym, newLengthSym->GetType(), instr->m_func),
                    IR::IndirOpnd::New(
                        baseOpnd,
                        Js::JavascriptArray::GetOffsetOfLength(),
                        newLengthSym->GetType(),
                        instr->m_func),
                    instr->m_func);
            loadLength->GetDst()->SetIsJITOptimizedReg(true);
            loadLength->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->SetIsJITOptimizedReg(true);

            // BailOnNegative length (BailOutOnIrregularLength)
            IR::Instr *bailOnIrregularLength = IR::Instr::New(Js::OpCode::BailOnNegative, instr->m_func);
            bailOnIrregularLength->SetSrc1(loadLength->GetDst());

            const IR::BailOutKind bailOutKind = IR::BailOutOnIrregularLength;
            if(hoistLengthLoadOutOfLoop)
            {
                Assert(!hoistLengthLoadOutOfLoop->jsArrayKills.KillsArrayLengths());

                TRACE_PHASE_INSTR(
                    Js::Phase::ArrayLengthHoistPhase,
                    instr,
                    _u("Hoisting array length load out of loop %u to landing pad block %u\n"),
                    hoistLengthLoadOutOfLoop->GetLoopNumber(),
                    hoistLengthLoadOutOfLoop->landingPad->GetBlockNum());
                TESTTRACE_PHASE_INSTR(Js::Phase::ArrayLengthHoistPhase, instr, _u("Hoisting array length load out of loop\n"));

                Assert(hoistLengthLoadOutOfLoop->bailOutInfo);
                EnsureBailTarget(hoistLengthLoadOutOfLoop);
                InsertInstrInLandingPad(loadLength, hoistLengthLoadOutOfLoop);
                InsertInstrInLandingPad(bailOnIrregularLength, hoistLengthLoadOutOfLoop);
                bailOnIrregularLength =
                    bailOnIrregularLength->ConvertToBailOutInstr(hoistLengthLoadOutOfLoop->bailOutInfo, bailOutKind);

                // Hoist the length value
                for(InvariantBlockBackwardIterator it(
                        this,
                        currentBlock,
                        hoistLengthLoadOutOfLoop->landingPad,
                        baseOpnd->m_sym,
                        baseValue->GetValueNumber());
                    it.IsValid();
                    it.MoveNext())
                {
                    BasicBlock *const block = it.Block();
                    block->globOptData.liveVarSyms->Set(newLengthSym->m_id);
                    Assert(!block->globOptData.FindValue(newLengthSym));
                    Value *const lengthValueCopy = CopyValue(lengthValue, lengthValue->GetValueNumber());
                    block->globOptData.SetValue(lengthValueCopy, newLengthSym);
                    this->SetSymStoreDirect(lengthValueCopy->GetValueInfo(), nullptr);
                }
            }
            else
            {
                loadLength->SetByteCodeOffset(instr);
                insertBeforeInstr->InsertBefore(loadLength);
                bailOnIrregularLength->SetByteCodeOffset(instr);
                insertBeforeInstr->InsertBefore(bailOnIrregularLength);
                if(shareableBailOutInfo)
                {
                    ShareBailOut();
                    bailOnIrregularLength = bailOnIrregularLength->ConvertToBailOutInstr(shareableBailOutInfo, bailOutKind);
                }
                else
                {
                    GenerateBailAtOperation(&bailOnIrregularLength, bailOutKind);
                    shareableBailOutInfo = bailOnIrregularLength->GetBailOutInfo();
                    shareableBailOutInfoOriginalOwner = bailOnIrregularLength;
                }
            }
        }

        const auto InsertHeadSegmentLoad = [&]()
        {
            TRACE_TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Separating array segment load\n"));

            Assert(newHeadSegmentSym);
            IR::RegOpnd *const headSegmentOpnd =
                IR::RegOpnd::New(newHeadSegmentSym, newHeadSegmentSym->GetType(), instr->m_func);
            headSegmentOpnd->SetIsJITOptimizedReg(true);
            IR::RegOpnd *const jitOptimizedBaseOpnd = baseOpnd->Copy(instr->m_func)->AsRegOpnd();
            jitOptimizedBaseOpnd->SetIsJITOptimizedReg(true);
            IR::Instr *loadObjectArray;
            if(baseValueType.GetObjectType() == ObjectType::ObjectWithArray)
            {
                loadObjectArray =
                    IR::Instr::New(
                        Js::OpCode::LdIndir,
                        headSegmentOpnd,
                        IR::IndirOpnd::New(
                            jitOptimizedBaseOpnd,
                            Js::DynamicObject::GetOffsetOfObjectArray(),
                            jitOptimizedBaseOpnd->GetType(),
                            instr->m_func),
                        instr->m_func);
            }
            else
            {
                loadObjectArray = nullptr;
            }
            IR::Instr *const loadHeadSegment =
                IR::Instr::New(
                    Js::OpCode::LdIndir,
                    headSegmentOpnd,
                    IR::IndirOpnd::New(
                        loadObjectArray ? headSegmentOpnd : jitOptimizedBaseOpnd,
                        Lowerer::GetArrayOffsetOfHeadSegment(baseValueType),
                        headSegmentOpnd->GetType(),
                        instr->m_func),
                    instr->m_func);
            if(hoistHeadSegmentLoadOutOfLoop)
            {
                Assert(!(isLikelyJsArray && hoistHeadSegmentLoadOutOfLoop->jsArrayKills.KillsArrayHeadSegments()));

                TRACE_PHASE_INSTR(
                    Js::ArraySegmentHoistPhase,
                    instr,
                    _u("Hoisting array segment load out of loop %u to landing pad block %u\n"),
                    hoistHeadSegmentLoadOutOfLoop->GetLoopNumber(),
                    hoistHeadSegmentLoadOutOfLoop->landingPad->GetBlockNum());
                TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Hoisting array segment load out of loop\n"));

                if(loadObjectArray)
                {
                    InsertInstrInLandingPad(loadObjectArray, hoistHeadSegmentLoadOutOfLoop);
                }
                InsertInstrInLandingPad(loadHeadSegment, hoistHeadSegmentLoadOutOfLoop);
            }
            else
            {
                if(loadObjectArray)
                {
                    loadObjectArray->SetByteCodeOffset(instr);
                    insertBeforeInstr->InsertBefore(loadObjectArray);
                }
                loadHeadSegment->SetByteCodeOffset(instr);
                insertBeforeInstr->InsertBefore(loadHeadSegment);
                instr->loadedArrayHeadSegment = true;
            }
        };

        if(doHeadSegmentLoad && isLikelyJsArray)
        {
            // For javascript arrays, the head segment is required to load the head segment length
            InsertHeadSegmentLoad();
        }

        if(doHeadSegmentLengthLoad)
        {
            Assert(!isLikelyJsArray || newHeadSegmentSym || baseArrayValueInfo && baseArrayValueInfo->HeadSegmentSym());
            Assert(newHeadSegmentLengthSym);
            Assert(!headSegmentLengthValue);

            TRACE_TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Separating array segment length load\n"));

            // Create an initial value for the head segment length
            CurrentBlockData()->liveVarSyms->Set(newHeadSegmentLengthSym->m_id);
            headSegmentLengthValue = NewIntRangeValue(0, Js::SparseArraySegmentBase::MaxLength, false);
            headSegmentLengthConstantBounds = IntConstantBounds(0, Js::SparseArraySegmentBase::MaxLength);
            CurrentBlockData()->SetValue(headSegmentLengthValue, newHeadSegmentLengthSym);

            // SetValue above would have set the sym store to newHeadSegmentLengthSym. This sym won't be used for copy-prop
            // though, so remove it as the sym store.
            this->SetSymStoreDirect(headSegmentLengthValue->GetValueInfo(), nullptr);

            StackSym *const headSegmentSym =
                isLikelyJsArray
                    ? newHeadSegmentSym ? newHeadSegmentSym : baseArrayValueInfo->HeadSegmentSym()
                    : nullptr;
            IR::Instr *const loadHeadSegmentLength =
                IR::Instr::New(
                    Js::OpCode::LdIndir,
                    IR::RegOpnd::New(newHeadSegmentLengthSym, newHeadSegmentLengthSym->GetType(), instr->m_func),
                    IR::IndirOpnd::New(
                        isLikelyJsArray ? IR::RegOpnd::New(headSegmentSym, headSegmentSym->GetType(), instr->m_func) : baseOpnd,
                        isLikelyJsArray
                            ? Js::SparseArraySegmentBase::GetOffsetOfLength()
                            : Lowerer::GetArrayOffsetOfLength(baseValueType),
                        newHeadSegmentLengthSym->GetType(),
                        instr->m_func),
                    instr->m_func);
            loadHeadSegmentLength->GetDst()->SetIsJITOptimizedReg(true);
            loadHeadSegmentLength->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->SetIsJITOptimizedReg(true);

            // We don't check the head segment length for negative (very large uint32) values. For JS arrays, the bound checks
            // cover that. For typed arrays, we currently don't allocate array buffers with more than 1 GB elements.
            if(hoistHeadSegmentLengthLoadOutOfLoop)
            {
                Assert(
                    !(
                        isLikelyJsArray
                            ? hoistHeadSegmentLengthLoadOutOfLoop->jsArrayKills.KillsArrayHeadSegmentLengths()
                            : hoistHeadSegmentLengthLoadOutOfLoop->jsArrayKills.KillsTypedArrayHeadSegmentLengths()
                    ));

                TRACE_PHASE_INSTR(
                    Js::ArraySegmentHoistPhase,
                    instr,
                    _u("Hoisting array segment length load out of loop %u to landing pad block %u\n"),
                    hoistHeadSegmentLengthLoadOutOfLoop->GetLoopNumber(),
                    hoistHeadSegmentLengthLoadOutOfLoop->landingPad->GetBlockNum());
                TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Hoisting array segment length load out of loop\n"));

                InsertInstrInLandingPad(loadHeadSegmentLength, hoistHeadSegmentLengthLoadOutOfLoop);

                // Hoist the head segment length value
                for(InvariantBlockBackwardIterator it(
                        this,
                        currentBlock,
                        hoistHeadSegmentLengthLoadOutOfLoop->landingPad,
                        baseOpnd->m_sym,
                        baseValue->GetValueNumber());
                    it.IsValid();
                    it.MoveNext())
                {
                    BasicBlock *const block = it.Block();
                    block->globOptData.liveVarSyms->Set(newHeadSegmentLengthSym->m_id);
                    Assert(!block->globOptData.FindValue(newHeadSegmentLengthSym));
                    Value *const headSegmentLengthValueCopy =
                        CopyValue(headSegmentLengthValue, headSegmentLengthValue->GetValueNumber());
                    block->globOptData.SetValue(headSegmentLengthValueCopy, newHeadSegmentLengthSym);
                    this->SetSymStoreDirect(headSegmentLengthValueCopy->GetValueInfo(), nullptr);
                }
            }
            else
            {
                loadHeadSegmentLength->SetByteCodeOffset(instr);
                insertBeforeInstr->InsertBefore(loadHeadSegmentLength);
                instr->loadedArrayHeadSegmentLength = true;
            }
        }

        if(doExtractBoundChecks)
        {
            Assert(!(eliminatedLowerBoundCheck && eliminatedUpperBoundCheck));
            Assert(baseOwnerIndir);
            Assert(!baseOwnerIndir->GetIndexOpnd() || baseOwnerIndir->GetIndexOpnd()->m_sym->IsTypeSpec());
            Assert(doHeadSegmentLengthLoad || headSegmentLengthIsAvailable);
            Assert(canBailOutOnArrayAccessHelperCall);
            Assert(!isStore || instr->m_opcode == Js::OpCode::StElemI_A || instr->m_opcode == Js::OpCode::StElemI_A_Strict || Js::IsSimd128LoadStore(instr->m_opcode));

            StackSym *const headSegmentLengthSym =
                headSegmentLengthIsAvailable ? baseArrayValueInfo->HeadSegmentLengthSym() : newHeadSegmentLengthSym;
            Assert(headSegmentLengthSym);
            Assert(headSegmentLengthValue);

            ArrayLowerBoundCheckHoistInfo lowerBoundCheckHoistInfo;
            ArrayUpperBoundCheckHoistInfo upperBoundCheckHoistInfo;
            bool failedToUpdateCompatibleLowerBoundCheck = false, failedToUpdateCompatibleUpperBoundCheck = false;
            if(DoBoundCheckHoist())
            {
                if(indexVarSym)
                {
                    TRACE_PHASE_INSTR_VERBOSE(
                        Js::Phase::BoundCheckHoistPhase,
                        instr,
                        _u("Determining array bound check hoistability for index s%u\n"),
                        indexVarSym->m_id);
                }
                else
                {
                    TRACE_PHASE_INSTR_VERBOSE(
                        Js::Phase::BoundCheckHoistPhase,
                        instr,
                        _u("Determining array bound check hoistability for index %d\n"),
                        indexConstantBounds.LowerBound());
                }

                DetermineArrayBoundCheckHoistability(
                    !eliminatedLowerBoundCheck,
                    !eliminatedUpperBoundCheck,
                    lowerBoundCheckHoistInfo,
                    upperBoundCheckHoistInfo,
                    isLikelyJsArray,
                    indexVarSym,
                    indexValue,
                    indexConstantBounds,
                    headSegmentLengthSym,
                    headSegmentLengthValue,
                    headSegmentLengthConstantBounds,
                    hoistHeadSegmentLengthLoadOutOfLoop,
                    failedToUpdateCompatibleLowerBoundCheck,
                    failedToUpdateCompatibleUpperBoundCheck);

#ifdef ENABLE_SIMDJS
                // SIMD_JS
                UpdateBoundCheckHoistInfoForSimd(upperBoundCheckHoistInfo, newBaseValueType, instr);
#endif
            }

            if(!eliminatedLowerBoundCheck)
            {
                eliminatedLowerBoundCheck = true;

                Assert(indexVarSym);
                Assert(baseOwnerIndir->GetIndexOpnd());
                Assert(indexValue);

                ArrayLowerBoundCheckHoistInfo &hoistInfo = lowerBoundCheckHoistInfo;
                if(hoistInfo.HasAnyInfo())
                {
                    BasicBlock *hoistBlock;
                    if(hoistInfo.CompatibleBoundCheckBlock())
                    {
                        hoistBlock = hoistInfo.CompatibleBoundCheckBlock();

                        TRACE_PHASE_INSTR(
                            Js::Phase::BoundCheckHoistPhase,
                            instr,
                            _u("Hoisting array lower bound check into existing bound check instruction in block %u\n"),
                            hoistBlock->GetBlockNum());
                        TESTTRACE_PHASE_INSTR(
                            Js::Phase::BoundCheckHoistPhase,
                            instr,
                            _u("Hoisting array lower bound check into existing bound check instruction\n"));
                    }
                    else
                    {
                        Assert(hoistInfo.Loop());
                        BasicBlock *const landingPad = hoistInfo.Loop()->landingPad;
                        hoistBlock = landingPad;

                        StackSym *indexIntSym;
                        if(hoistInfo.IndexSym() && hoistInfo.IndexSym()->IsVar())
                        {
                            if(!landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()))
                            {
                                // Int-specialize the index sym, as the BoundCheck instruction requires int operands. Specialize
                                // it in this block if it is invariant, as the conversion will be hoisted along with value
                                // updates.
                                BasicBlock *specializationBlock = hoistInfo.Loop()->landingPad;
                                IR::Instr *specializeBeforeInstr = nullptr;
                                if(!CurrentBlockData()->IsInt32TypeSpecialized(hoistInfo.IndexSym()) &&
                                    OptIsInvariant(
                                        hoistInfo.IndexSym(),
                                        currentBlock,
                                        hoistInfo.Loop(),
                                        CurrentBlockData()->FindValue(hoistInfo.IndexSym()),
                                        false,
                                        true))
                                {
                                    specializationBlock = currentBlock;
                                    specializeBeforeInstr = insertBeforeInstr;
                                }
                                Assert(tempBv->IsEmpty());
                                tempBv->Set(hoistInfo.IndexSym()->m_id);
                                ToInt32(tempBv, specializationBlock, false, specializeBeforeInstr);
                                tempBv->ClearAll();
                                Assert(landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()));
                            }
                            indexIntSym = hoistInfo.IndexSym()->GetInt32EquivSym(nullptr);
                            Assert(indexIntSym);
                        }
                        else
                        {
                            indexIntSym = hoistInfo.IndexSym();
                            Assert(!indexIntSym || indexIntSym->GetType() == TyInt32 || indexIntSym->GetType() == TyUint32);
                        }

                        // The info in the landing pad may be better than the info in the current block due to changes made to
                        // the index sym inside the loop. Check if the bound check we intend to hoist is unnecessary in the
                        // landing pad.
                        if(!ValueInfo::IsLessThanOrEqualTo(
                                nullptr,
                                0,
                                0,
                                hoistInfo.IndexValue(),
                                hoistInfo.IndexConstantBounds().LowerBound(),
                                hoistInfo.IndexConstantBounds().UpperBound(),
                                hoistInfo.Offset()))
                        {
                            Assert(hoistInfo.IndexSym());
                            Assert(hoistInfo.Loop()->bailOutInfo);
                            EnsureBailTarget(hoistInfo.Loop());

                            if(hoistInfo.LoopCount())
                            {
                                // Generate the loop count and loop count based bound that will be used for the bound check
                                if(!hoistInfo.LoopCount()->HasBeenGenerated())
                                {
                                    GenerateLoopCount(hoistInfo.Loop(), hoistInfo.LoopCount());
                                }
                                GenerateSecondaryInductionVariableBound(
                                    hoistInfo.Loop(),
                                    indexVarSym->GetInt32EquivSym(nullptr),
                                    hoistInfo.LoopCount(),
                                    hoistInfo.MaxMagnitudeChange(),
                                    hoistInfo.IndexSym());
                            }

                            IR::Opnd* lowerBound = IR::IntConstOpnd::New(0, TyInt32, instr->m_func, true);
                            IR::Opnd* upperBound = IR::RegOpnd::New(indexIntSym, TyInt32, instr->m_func);
                            upperBound->SetIsJITOptimizedReg(true);

                            // 0 <= indexSym + offset (src1 <= src2 + dst)
                            IR::Instr *const boundCheck = CreateBoundsCheckInstr(
                                lowerBound,
                                upperBound,
                                hoistInfo.Offset(),
                                hoistInfo.IsLoopCountBasedBound()
                                    ? IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck
                                    : IR::BailOutOnFailedHoistedBoundCheck,
                                hoistInfo.Loop()->bailOutInfo,
                                hoistInfo.Loop()->bailOutInfo->bailOutFunc);
                            InsertInstrInLandingPad(boundCheck, hoistInfo.Loop());

                            TRACE_PHASE_INSTR(
                                Js::Phase::BoundCheckHoistPhase,
                                instr,
                                _u("Hoisting array lower bound check out of loop %u to landing pad block %u, as (0 <= s%u + %d)\n"),
                                hoistInfo.Loop()->GetLoopNumber(),
                                landingPad->GetBlockNum(),
                                hoistInfo.IndexSym()->m_id,
                                hoistInfo.Offset());
                            TESTTRACE_PHASE_INSTR(
                                Js::Phase::BoundCheckHoistPhase,
                                instr,
                                _u("Hoisting array lower bound check out of loop\n"));

                            // Record the bound check instruction as available
                            const IntBoundCheck boundCheckInfo(
                                ZeroValueNumber,
                                hoistInfo.IndexValueNumber(),
                                boundCheck,
                                landingPad);
                            {
                                const bool added = CurrentBlockData()->availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                                Assert(added || failedToUpdateCompatibleLowerBoundCheck);
                            }
                            for(InvariantBlockBackwardIterator it(this, currentBlock, landingPad, nullptr);
                                it.IsValid();
                                it.MoveNext())
                            {
                                const bool added = it.Block()->globOptData.availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                                Assert(added || failedToUpdateCompatibleLowerBoundCheck);
                            }
                        }
                    }

                    // Update values of the syms involved in the bound check to reflect the bound check
                    if(hoistBlock != currentBlock && hoistInfo.IndexSym() && hoistInfo.Offset() != INT32_MIN)
                    {
                        for(InvariantBlockBackwardIterator it(
                                this,
                                currentBlock->next,
                                hoistBlock,
                                hoistInfo.IndexSym(),
                                hoistInfo.IndexValueNumber());
                            it.IsValid();
                            it.MoveNext())
                        {
                            Value *const value = it.InvariantSymValue();
                            IntConstantBounds constantBounds;
                            AssertVerify(value->GetValueInfo()->TryGetIntConstantBounds(&constantBounds, true));

                            ValueInfo *const newValueInfo =
                                UpdateIntBoundsForGreaterThanOrEqual(
                                    value,
                                    constantBounds,
                                    nullptr,
                                    IntConstantBounds(-hoistInfo.Offset(), -hoistInfo.Offset()),
                                    false);
                            if(newValueInfo)
                            {
                                ChangeValueInfo(nullptr, value, newValueInfo);
                                if(it.Block() == currentBlock && value == indexValue)
                                {
                                    AssertVerify(newValueInfo->TryGetIntConstantBounds(&indexConstantBounds));
                                }
                            }
                        }
                    }
                }
                else
                {
                    IR::Opnd* lowerBound = IR::IntConstOpnd::New(0, TyInt32, instr->m_func, true);
                    IR::Opnd* upperBound = baseOwnerIndir->GetIndexOpnd();
                    upperBound->SetIsJITOptimizedReg(true);
                    const int offset = 0;

                    IR::Instr *boundCheck;
                    if(shareableBailOutInfo)
                    {
                        ShareBailOut();
                        boundCheck = CreateBoundsCheckInstr(
                            lowerBound,
                            upperBound,
                            offset,
                            IR::BailOutOnArrayAccessHelperCall,
                            shareableBailOutInfo,
                            shareableBailOutInfo->bailOutFunc);
                    }
                    else
                    {
                        boundCheck = CreateBoundsCheckInstr(
                            lowerBound,
                            upperBound,
                            offset,
                            instr->m_func);
                    }


                    boundCheck->SetByteCodeOffset(instr);
                    insertBeforeInstr->InsertBefore(boundCheck);
                    if(!shareableBailOutInfo)
                    {
                        GenerateBailAtOperation(&boundCheck, IR::BailOutOnArrayAccessHelperCall);
                        shareableBailOutInfo = boundCheck->GetBailOutInfo();
                        shareableBailOutInfoOriginalOwner = boundCheck;
                    }

                    TRACE_PHASE_INSTR(
                        Js::Phase::BoundCheckEliminationPhase,
                        instr,
                        _u("Separating array lower bound check, as (0 <= s%u)\n"),
                        indexVarSym->m_id);
                    TESTTRACE_PHASE_INSTR(
                        Js::Phase::BoundCheckEliminationPhase,
                        instr,
                        _u("Separating array lower bound check\n"));

                    if(DoBoundCheckHoist())
                    {
                        // Record the bound check instruction as available
                        const bool added =
                            CurrentBlockData()->availableIntBoundChecks->AddNew(
                                IntBoundCheck(ZeroValueNumber, indexValue->GetValueNumber(), boundCheck, currentBlock)) >= 0;
                        Assert(added || failedToUpdateCompatibleLowerBoundCheck);
                    }
                }

                // Update the index value to reflect the bound check
                ValueInfo *const newValueInfo =
                    UpdateIntBoundsForGreaterThanOrEqual(
                        indexValue,
                        indexConstantBounds,
                        nullptr,
                        IntConstantBounds(0, 0),
                        false);
                if(newValueInfo)
                {
                    ChangeValueInfo(nullptr, indexValue, newValueInfo);
                    AssertVerify(newValueInfo->TryGetIntConstantBounds(&indexConstantBounds));
                }
            }

            if(!eliminatedUpperBoundCheck)
            {
                eliminatedUpperBoundCheck = true;

                ArrayUpperBoundCheckHoistInfo &hoistInfo = upperBoundCheckHoistInfo;
                if(hoistInfo.HasAnyInfo())
                {
                    BasicBlock *hoistBlock;
                    if(hoistInfo.CompatibleBoundCheckBlock())
                    {
                        hoistBlock = hoistInfo.CompatibleBoundCheckBlock();

                        TRACE_PHASE_INSTR(
                            Js::Phase::BoundCheckHoistPhase,
                            instr,
                            _u("Hoisting array upper bound check into existing bound check instruction in block %u\n"),
                            hoistBlock->GetBlockNum());
                        TESTTRACE_PHASE_INSTR(
                            Js::Phase::BoundCheckHoistPhase,
                            instr,
                            _u("Hoisting array upper bound check into existing bound check instruction\n"));
                    }
                    else
                    {
                        Assert(hoistInfo.Loop());
                        BasicBlock *const landingPad = hoistInfo.Loop()->landingPad;
                        hoistBlock = landingPad;

                        StackSym *indexIntSym;
                        if(hoistInfo.IndexSym() && hoistInfo.IndexSym()->IsVar())
                        {
                            if(!landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()))
                            {
                                // Int-specialize the index sym, as the BoundCheck instruction requires int operands. Specialize it
                                // in this block if it is invariant, as the conversion will be hoisted along with value updates.
                                BasicBlock *specializationBlock = hoistInfo.Loop()->landingPad;
                                IR::Instr *specializeBeforeInstr = nullptr;
                                if(!CurrentBlockData()->IsInt32TypeSpecialized(hoistInfo.IndexSym()) &&
                                    OptIsInvariant(
                                        hoistInfo.IndexSym(),
                                        currentBlock,
                                        hoistInfo.Loop(),
                                        CurrentBlockData()->FindValue(hoistInfo.IndexSym()),
                                        false,
                                        true))
                                {
                                    specializationBlock = currentBlock;
                                    specializeBeforeInstr = insertBeforeInstr;
                                }
                                Assert(tempBv->IsEmpty());
                                tempBv->Set(hoistInfo.IndexSym()->m_id);
                                ToInt32(tempBv, specializationBlock, false, specializeBeforeInstr);
                                tempBv->ClearAll();
                                Assert(landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()));
                            }
                            indexIntSym = hoistInfo.IndexSym()->GetInt32EquivSym(nullptr);
                            Assert(indexIntSym);
                        }
                        else
                        {
                            indexIntSym = hoistInfo.IndexSym();
                            Assert(!indexIntSym || indexIntSym->GetType() == TyInt32 || indexIntSym->GetType() == TyUint32);
                        }

                        // The info in the landing pad may be better than the info in the current block due to changes made to the
                        // index sym inside the loop. Check if the bound check we intend to hoist is unnecessary in the landing pad.
                        if(!ValueInfo::IsLessThanOrEqualTo(
                                hoistInfo.IndexValue(),
                                hoistInfo.IndexConstantBounds().LowerBound(),
                                hoistInfo.IndexConstantBounds().UpperBound(),
                                hoistInfo.HeadSegmentLengthValue(),
                                hoistInfo.HeadSegmentLengthConstantBounds().LowerBound(),
                                hoistInfo.HeadSegmentLengthConstantBounds().UpperBound(),
                                hoistInfo.Offset()))
                        {
                            Assert(hoistInfo.Loop()->bailOutInfo);
                            EnsureBailTarget(hoistInfo.Loop());

                            if(hoistInfo.LoopCount())
                            {
                                // Generate the loop count and loop count based bound that will be used for the bound check
                                if(!hoistInfo.LoopCount()->HasBeenGenerated())
                                {
                                    GenerateLoopCount(hoistInfo.Loop(), hoistInfo.LoopCount());
                                }
                                GenerateSecondaryInductionVariableBound(
                                    hoistInfo.Loop(),
                                    indexVarSym->GetInt32EquivSym(nullptr),
                                    hoistInfo.LoopCount(),
                                    hoistInfo.MaxMagnitudeChange(),
                                    hoistInfo.IndexSym());
                            }

                            IR::Opnd* lowerBound = indexIntSym
                                ? static_cast<IR::Opnd *>(IR::RegOpnd::New(indexIntSym, TyInt32, instr->m_func))
                                : IR::IntConstOpnd::New(
                                    hoistInfo.IndexConstantBounds().LowerBound(),
                                    TyInt32,
                                    instr->m_func);
                            lowerBound->SetIsJITOptimizedReg(true);
                            IR::Opnd* upperBound = IR::RegOpnd::New(headSegmentLengthSym, headSegmentLengthSym->GetType(), instr->m_func);
                            upperBound->SetIsJITOptimizedReg(true);

                            // indexSym <= headSegmentLength + offset (src1 <= src2 + dst)
                            IR::Instr *const boundCheck = CreateBoundsCheckInstr(
                                lowerBound,
                                upperBound,
                                hoistInfo.Offset(),
                                hoistInfo.IsLoopCountBasedBound()
                                    ? IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck
                                    : IR::BailOutOnFailedHoistedBoundCheck,
                                hoistInfo.Loop()->bailOutInfo,
                                hoistInfo.Loop()->bailOutInfo->bailOutFunc);

                            InsertInstrInLandingPad(boundCheck, hoistInfo.Loop());

                            if(indexIntSym)
                            {
                                TRACE_PHASE_INSTR(
                                    Js::Phase::BoundCheckHoistPhase,
                                    instr,
                                    _u("Hoisting array upper bound check out of loop %u to landing pad block %u, as (s%u <= s%u + %d)\n"),
                                    hoistInfo.Loop()->GetLoopNumber(),
                                    landingPad->GetBlockNum(),
                                    hoistInfo.IndexSym()->m_id,
                                    headSegmentLengthSym->m_id,
                                    hoistInfo.Offset());
                            }
                            else
                            {
                                TRACE_PHASE_INSTR(
                                    Js::Phase::BoundCheckHoistPhase,
                                    instr,
                                    _u("Hoisting array upper bound check out of loop %u to landing pad block %u, as (%d <= s%u + %d)\n"),
                                    hoistInfo.Loop()->GetLoopNumber(),
                                    landingPad->GetBlockNum(),
                                    hoistInfo.IndexConstantBounds().LowerBound(),
                                    headSegmentLengthSym->m_id,
                                    hoistInfo.Offset());
                            }
                            TESTTRACE_PHASE_INSTR(
                                Js::Phase::BoundCheckHoistPhase,
                                instr,
                                _u("Hoisting array upper bound check out of loop\n"));

                            // Record the bound check instruction as available
                            const IntBoundCheck boundCheckInfo(
                                hoistInfo.IndexValue() ? hoistInfo.IndexValueNumber() : ZeroValueNumber,
                                hoistInfo.HeadSegmentLengthValue()->GetValueNumber(),
                                boundCheck,
                                landingPad);
                            {
                                const bool added = CurrentBlockData()->availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                                Assert(added || failedToUpdateCompatibleUpperBoundCheck);
                            }
                            for(InvariantBlockBackwardIterator it(this, currentBlock, landingPad, nullptr);
                                it.IsValid();
                                it.MoveNext())
                            {
                                const bool added = it.Block()->globOptData.availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                                Assert(added || failedToUpdateCompatibleUpperBoundCheck);
                            }
                        }
                    }

                    // Update values of the syms involved in the bound check to reflect the bound check
                    Assert(!hoistInfo.Loop() || hoistBlock != currentBlock);
                    if(hoistBlock != currentBlock)
                    {
                        for(InvariantBlockBackwardIterator it(this, currentBlock->next, hoistBlock, nullptr);
                            it.IsValid();
                            it.MoveNext())
                        {
                            BasicBlock *const block = it.Block();

                            Value *leftValue;
                            IntConstantBounds leftConstantBounds;
                            if(hoistInfo.IndexSym())
                            {
                                leftValue = block->globOptData.FindValue(hoistInfo.IndexSym());
                                if(!leftValue || leftValue->GetValueNumber() != hoistInfo.IndexValueNumber())
                                {
                                    continue;
                                }
                                AssertVerify(leftValue->GetValueInfo()->TryGetIntConstantBounds(&leftConstantBounds, true));
                            }
                            else
                            {
                                leftValue = nullptr;
                                leftConstantBounds = hoistInfo.IndexConstantBounds();
                            }

                            Value *const rightValue = block->globOptData.FindValue(headSegmentLengthSym);
                            if(!rightValue)
                            {
                                continue;
                            }
                            Assert(rightValue->GetValueNumber() ==  headSegmentLengthValue->GetValueNumber());
                            IntConstantBounds rightConstantBounds;
                            AssertVerify(rightValue->GetValueInfo()->TryGetIntConstantBounds(&rightConstantBounds));

                            ValueInfo *const newValueInfoForLessThanOrEqual =
                                UpdateIntBoundsForLessThanOrEqual(
                                    leftValue,
                                    leftConstantBounds,
                                    rightValue,
                                    rightConstantBounds,
                                    hoistInfo.Offset(),
                                    false);
                            if (newValueInfoForLessThanOrEqual)
                            {
                                ChangeValueInfo(nullptr, leftValue, newValueInfoForLessThanOrEqual);
                                AssertVerify(newValueInfoForLessThanOrEqual->TryGetIntConstantBounds(&leftConstantBounds, true));
                                if(block == currentBlock && leftValue == indexValue)
                                {
                                    Assert(newValueInfoForLessThanOrEqual->IsInt());
                                    indexConstantBounds = leftConstantBounds;
                                }
                            }
                            if(hoistInfo.Offset() != INT32_MIN)
                            {
                                ValueInfo *const newValueInfoForGreaterThanOrEqual =
                                    UpdateIntBoundsForGreaterThanOrEqual(
                                        rightValue,
                                        rightConstantBounds,
                                        leftValue,
                                        leftConstantBounds,
                                        -hoistInfo.Offset(),
                                        false);
                                if (newValueInfoForGreaterThanOrEqual)
                                {
                                    ChangeValueInfo(nullptr, rightValue, newValueInfoForGreaterThanOrEqual);
                                    if(block == currentBlock)
                                    {
                                        Assert(rightValue == headSegmentLengthValue);
                                        AssertVerify(newValueInfoForGreaterThanOrEqual->TryGetIntConstantBounds(&headSegmentLengthConstantBounds));
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    IR::Opnd* lowerBound = baseOwnerIndir->GetIndexOpnd()
                        ? static_cast<IR::Opnd *>(baseOwnerIndir->GetIndexOpnd())
                        : IR::IntConstOpnd::New(baseOwnerIndir->GetOffset(), TyInt32, instr->m_func);
                    lowerBound->SetIsJITOptimizedReg(true);
                    IR::Opnd* upperBound = IR::RegOpnd::New(headSegmentLengthSym, headSegmentLengthSym->GetType(), instr->m_func);
                    upperBound->SetIsJITOptimizedReg(true);
                    const int offset = GetBoundCheckOffsetForSimd(newBaseValueType, instr, -1);
                    IR::Instr *boundCheck;

                    // index <= headSegmentLength - 1 (src1 <= src2 + dst)
                    if (shareableBailOutInfo)
                    {
                        ShareBailOut();
                        boundCheck = CreateBoundsCheckInstr(
                            lowerBound,
                            upperBound,
                            offset,
                            IR::BailOutOnArrayAccessHelperCall,
                            shareableBailOutInfo,
                            shareableBailOutInfo->bailOutFunc);
                    }
                    else
                    {
                        boundCheck = CreateBoundsCheckInstr(
                            lowerBound,
                            upperBound,
                            offset,
                            instr->m_func);
                    }

                    boundCheck->SetByteCodeOffset(instr);
                    insertBeforeInstr->InsertBefore(boundCheck);
                    if(!shareableBailOutInfo)
                    {
                        GenerateBailAtOperation(&boundCheck, IR::BailOutOnArrayAccessHelperCall);
                        shareableBailOutInfo = boundCheck->GetBailOutInfo();
                        shareableBailOutInfoOriginalOwner = boundCheck;
                    }
                    instr->extractedUpperBoundCheckWithoutHoisting = true;

                    if(baseOwnerIndir->GetIndexOpnd())
                    {
                        TRACE_PHASE_INSTR(
                            Js::Phase::BoundCheckEliminationPhase,
                            instr,
                            _u("Separating array upper bound check, as (s%u < s%u)\n"),
                            indexVarSym->m_id,
                            headSegmentLengthSym->m_id);
                    }
                    else
                    {
                        TRACE_PHASE_INSTR(
                            Js::Phase::BoundCheckEliminationPhase,
                            instr,
                            _u("Separating array upper bound check, as (%d < s%u)\n"),
                            baseOwnerIndir->GetOffset(),
                            headSegmentLengthSym->m_id);
                    }
                    TESTTRACE_PHASE_INSTR(
                        Js::Phase::BoundCheckEliminationPhase,
                        instr,
                        _u("Separating array upper bound check\n"));

                    if(DoBoundCheckHoist())
                    {
                        // Record the bound check instruction as available
                        const bool added =
                            CurrentBlockData()->availableIntBoundChecks->AddNew(
                                IntBoundCheck(
                                    indexValue ? indexValue->GetValueNumber() : ZeroValueNumber,
                                    headSegmentLengthValue->GetValueNumber(),
                                    boundCheck,
                                    currentBlock)) >= 0;
                        Assert(added || failedToUpdateCompatibleUpperBoundCheck);
                    }
                }

                // Update the index and head segment length values to reflect the bound check
                ValueInfo *newValueInfo =
                    UpdateIntBoundsForLessThan(
                        indexValue,
                        indexConstantBounds,
                        headSegmentLengthValue,
                        headSegmentLengthConstantBounds,
                        false);
                if(newValueInfo)
                {
                    ChangeValueInfo(nullptr, indexValue, newValueInfo);
                    AssertVerify(newValueInfo->TryGetIntConstantBounds(&indexConstantBounds));
                }
                newValueInfo =
                    UpdateIntBoundsForGreaterThan(
                        headSegmentLengthValue,
                        headSegmentLengthConstantBounds,
                        indexValue,
                        indexConstantBounds,
                        false);
                if(newValueInfo)
                {
                    ChangeValueInfo(nullptr, headSegmentLengthValue, newValueInfo);
                }
            }
        }

        if(doHeadSegmentLoad && !isLikelyJsArray)
        {
            // For typed arrays, load the length first, followed by the bound checks, and then load the head segment. This
            // allows the length sym to become dead by the time of the head segment load, freeing up the register for use by the
            // head segment sym.
            InsertHeadSegmentLoad();
        }

        if(doArrayChecks || doHeadSegmentLoad || doHeadSegmentLengthLoad || doLengthLoad)
        {
            UpdateValue(newHeadSegmentSym, newHeadSegmentLengthSym, newLengthSym);
            baseValueInfo = baseValue->GetValueInfo();
            baseArrayValueInfo = baseValueInfo->IsArrayValueInfo() ? baseValueInfo->AsArrayValueInfo() : nullptr;

            // Iterate up to the root loop's landing pad until all necessary value info is updated
            uint hoistItemCount =
                static_cast<uint>(!!hoistChecksOutOfLoop) +
                !!hoistHeadSegmentLoadOutOfLoop +
                !!hoistHeadSegmentLengthLoadOutOfLoop +
                !!hoistLengthLoadOutOfLoop;
            if(hoistItemCount != 0)
            {
                Loop *rootLoop = nullptr;
                for(Loop *loop = currentBlock->loop; loop; loop = loop->parent)
                {
                    rootLoop = loop;
                }
                Assert(rootLoop);

                ValueInfo *valueInfoToHoist = baseValueInfo;
                bool removeHeadSegment, removeHeadSegmentLength, removeLength;
                if(baseArrayValueInfo)
                {
                    removeHeadSegment = baseArrayValueInfo->HeadSegmentSym() && !hoistHeadSegmentLoadOutOfLoop;
                    removeHeadSegmentLength =
                        baseArrayValueInfo->HeadSegmentLengthSym() && !hoistHeadSegmentLengthLoadOutOfLoop;
                    removeLength = baseArrayValueInfo->LengthSym() && !hoistLengthLoadOutOfLoop;
                }
                else
                {
                    removeLength = removeHeadSegmentLength = removeHeadSegment = false;
                }
                for(InvariantBlockBackwardIterator it(
                        this,
                        currentBlock,
                        rootLoop->landingPad,
                        baseOpnd->m_sym,
                        baseValue->GetValueNumber());
                    it.IsValid();
                    it.MoveNext())
                {
                    if(removeHeadSegment || removeHeadSegmentLength || removeLength)
                    {
                        // Remove information that shouldn't be there anymore, from the value info
                        valueInfoToHoist =
                            valueInfoToHoist->AsArrayValueInfo()->Copy(
                                alloc,
                                !removeHeadSegment,
                                !removeHeadSegmentLength,
                                !removeLength);
                        removeLength = removeHeadSegmentLength = removeHeadSegment = false;
                    }

                    BasicBlock *const block = it.Block();
                    Value *const blockBaseValue = it.InvariantSymValue();
                    HoistInvariantValueInfo(valueInfoToHoist, blockBaseValue, block);

                    // See if we have completed hoisting value info for one of the items
                    if(hoistChecksOutOfLoop && block == hoistChecksOutOfLoop->landingPad)
                    {
                        // All other items depend on array checks, so we can just stop here
                        hoistChecksOutOfLoop = nullptr;
                        break;
                    }
                    if(hoistHeadSegmentLoadOutOfLoop && block == hoistHeadSegmentLoadOutOfLoop->landingPad)
                    {
                        hoistHeadSegmentLoadOutOfLoop = nullptr;
                        if(--hoistItemCount == 0)
                            break;
                        if(valueInfoToHoist->IsArrayValueInfo() && valueInfoToHoist->AsArrayValueInfo()->HeadSegmentSym())
                            removeHeadSegment = true;
                    }
                    if(hoistHeadSegmentLengthLoadOutOfLoop && block == hoistHeadSegmentLengthLoadOutOfLoop->landingPad)
                    {
                        hoistHeadSegmentLengthLoadOutOfLoop = nullptr;
                        if(--hoistItemCount == 0)
                            break;
                        if(valueInfoToHoist->IsArrayValueInfo() && valueInfoToHoist->AsArrayValueInfo()->HeadSegmentLengthSym())
                            removeHeadSegmentLength = true;
                    }
                    if(hoistLengthLoadOutOfLoop && block == hoistLengthLoadOutOfLoop->landingPad)
                    {
                        hoistLengthLoadOutOfLoop = nullptr;
                        if(--hoistItemCount == 0)
                            break;
                        if(valueInfoToHoist->IsArrayValueInfo() && valueInfoToHoist->AsArrayValueInfo()->LengthSym())
                            removeLength = true;
                    }
                }
            }
        }
    }

    IR::ArrayRegOpnd *baseArrayOpnd;
    if(baseArrayValueInfo)
    {
        // Update the opnd to include the associated syms
        baseArrayOpnd =
            baseArrayValueInfo->CreateOpnd(
                baseOpnd,
                needsHeadSegment,
                needsHeadSegmentLength || (!isLikelyJsArray && needsLength),
                needsLength,
                eliminatedLowerBoundCheck,
                eliminatedUpperBoundCheck,
                instr->m_func);
        if(baseOwnerInstr)
        {
            Assert(baseOwnerInstr->GetSrc1() == baseOpnd);
            baseOwnerInstr->ReplaceSrc1(baseArrayOpnd);
        }
        else
        {
            Assert(baseOwnerIndir);
            Assert(baseOwnerIndir->GetBaseOpnd() == baseOpnd);
            baseOwnerIndir->ReplaceBaseOpnd(baseArrayOpnd);
        }
        baseOpnd = baseArrayOpnd;
    }
    else
    {
        baseArrayOpnd = nullptr;
    }

    if(isLikelyJsArray)
    {
        // Insert an instruction to indicate to the dead-store pass that implicit calls need to be kept disabled until this
        // instruction. Operations other than LdElem and StElem don't benefit much from arrays having no missing values, so
        // no need to ensure that the array still has no missing values. For a particular array, if none of the accesses
        // benefit much from the no-missing-values information, it may be beneficial to avoid checking for no missing
        // values, especially in the case for a single array access, where the cost of the check could be relatively
        // significant. An StElem has to do additional checks in the common path if the array may have missing values, and
        // a StElem that operates on an array that has no missing values is more likely to keep the no-missing-values info
        // on the array more precise, so it still benefits a little from the no-missing-values info.
        CaptureNoImplicitCallUses(baseOpnd, isLoad || isStore);
    }
    else if(baseArrayOpnd && baseArrayOpnd->HeadSegmentLengthSym())
    {
        // A typed array's array buffer may be transferred to a web worker as part of an implicit call, in which case the typed
        // array's length is set to zero. Insert an instruction to indicate to the dead-store pass that implicit calls need to
        // be disabled until this instruction.
        IR::RegOpnd *const headSegmentLengthOpnd =
            IR::RegOpnd::New(
                baseArrayOpnd->HeadSegmentLengthSym(),
                baseArrayOpnd->HeadSegmentLengthSym()->GetType(),
                instr->m_func);
        const IR::AutoReuseOpnd autoReuseHeadSegmentLengthOpnd(headSegmentLengthOpnd, instr->m_func);
        CaptureNoImplicitCallUses(headSegmentLengthOpnd, false);
    }

    const auto OnEliminated = [&](const Js::Phase phase, const char *const eliminatedLoad)
    {
        TRACE_TESTTRACE_PHASE_INSTR(phase, instr, _u("Eliminating array %S\n"), eliminatedLoad);
    };

    OnEliminated(Js::Phase::ArrayCheckHoistPhase, "checks");
    if(baseArrayOpnd)
    {
        if(baseArrayOpnd->HeadSegmentSym())
        {
            OnEliminated(Js::Phase::ArraySegmentHoistPhase, "head segment load");
        }
        if(baseArrayOpnd->HeadSegmentLengthSym())
        {
            OnEliminated(Js::Phase::ArraySegmentHoistPhase, "head segment length load");
        }
        if(baseArrayOpnd->LengthSym())
        {
            OnEliminated(Js::Phase::ArrayLengthHoistPhase, "length load");
        }
        if(baseArrayOpnd->EliminatedLowerBoundCheck())
        {
            OnEliminated(Js::Phase::BoundCheckEliminationPhase, "lower bound check");
        }
        if(baseArrayOpnd->EliminatedUpperBoundCheck())
        {
            OnEliminated(Js::Phase::BoundCheckEliminationPhase, "upper bound check");
        }
    }

    if(!canBailOutOnArrayAccessHelperCall)
    {
        return;
    }

    // Bail out instead of generating a helper call. This helps to remove the array reference when the head segment and head
    // segment length are available, reduces code size, and allows bound checks to be separated.
    if(instr->HasBailOutInfo())
    {
        const IR::BailOutKind bailOutKind = instr->GetBailOutKind();
        Assert(
            !(bailOutKind & ~IR::BailOutKindBits) ||
            (bailOutKind & ~IR::BailOutKindBits) == IR::BailOutOnImplicitCallsPreOp);
        instr->SetBailOutKind(bailOutKind & IR::BailOutKindBits | IR::BailOutOnArrayAccessHelperCall);
    }
    else
    {
        GenerateBailAtOperation(&instr, IR::BailOutOnArrayAccessHelperCall);
    }
}

void
GlobOpt::CaptureNoImplicitCallUses(
    IR::Opnd *opnd,
    const bool usesNoMissingValuesInfo,
    IR::Instr *const includeCurrentInstr)
{
    Assert(!IsLoopPrePass());
    Assert(noImplicitCallUsesToInsert);
    Assert(opnd);

    // The opnd may be deleted later, so make a copy to ensure it is alive for inserting NoImplicitCallUses later
    opnd = opnd->Copy(func);

    if(!usesNoMissingValuesInfo)
    {
        const ValueType valueType(opnd->GetValueType());
        if(valueType.IsArrayOrObjectWithArray() && valueType.HasNoMissingValues())
        {
            // Inserting NoImplicitCallUses for an opnd with a definitely-array-with-no-missing-values value type means that the
            // instruction following it uses the information that the array has no missing values in some way, for instance, it
            // may omit missing value checks. Based on that, the dead-store phase in turn ensures that the necessary bailouts
            // are inserted to ensure that the array still has no missing values until the following instruction. Since
            // 'usesNoMissingValuesInfo' is false, change the value type to indicate to the dead-store phase that the following
            // instruction does not use the no-missing-values information.
            opnd->SetValueType(valueType.SetHasNoMissingValues(false));
        }
    }

    if(includeCurrentInstr)
    {
        IR::Instr *const noImplicitCallUses =
            IR::PragmaInstr::New(Js::OpCode::NoImplicitCallUses, 0, includeCurrentInstr->m_func);
        noImplicitCallUses->SetSrc1(opnd);
        noImplicitCallUses->GetSrc1()->SetIsJITOptimizedReg(true);
        includeCurrentInstr->InsertAfter(noImplicitCallUses);
        return;
    }

    noImplicitCallUsesToInsert->Add(opnd);
}

void
GlobOpt::InsertNoImplicitCallUses(IR::Instr *const instr)
{
    Assert(noImplicitCallUsesToInsert);

    const int n = noImplicitCallUsesToInsert->Count();
    if(n == 0)
    {
        return;
    }

    IR::Instr *const insertBeforeInstr = instr->GetInsertBeforeByteCodeUsesInstr();
    for(int i = 0; i < n;)
    {
        IR::Instr *const noImplicitCallUses = IR::PragmaInstr::New(Js::OpCode::NoImplicitCallUses, 0, instr->m_func);
        noImplicitCallUses->SetSrc1(noImplicitCallUsesToInsert->Item(i));
        noImplicitCallUses->GetSrc1()->SetIsJITOptimizedReg(true);
        ++i;
        if(i < n)
        {
            noImplicitCallUses->SetSrc2(noImplicitCallUsesToInsert->Item(i));
            noImplicitCallUses->GetSrc2()->SetIsJITOptimizedReg(true);
            ++i;
        }
        noImplicitCallUses->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(noImplicitCallUses);
    }
    noImplicitCallUsesToInsert->Clear();
}

void
GlobOpt::PrepareLoopArrayCheckHoist()
{
    if(IsLoopPrePass() || !currentBlock->loop || !currentBlock->isLoopHeader || !currentBlock->loop->parent)
    {
        return;
    }

    if(currentBlock->loop->parent->needImplicitCallBailoutChecksForJsArrayCheckHoist)
    {
        // If the parent loop is an array check elimination candidate, so is the current loop. Even though the current loop may
        // not have array accesses, if the parent loop hoists array checks, the current loop also needs implicit call checks.
        currentBlock->loop->needImplicitCallBailoutChecksForJsArrayCheckHoist = true;
    }
}

JsArrayKills
GlobOpt::CheckJsArrayKills(IR::Instr *const instr)
{
    Assert(instr);

    JsArrayKills kills;
    if(instr->UsesAllFields())
    {
        // Calls can (but are unlikely to) change a javascript array into an ES5 array, which may have different behavior for
        // index properties.
        kills.SetKillsAllArrays();
        return kills;
    }

    const bool doArrayMissingValueCheckHoist = DoArrayMissingValueCheckHoist();
    const bool doNativeArrayTypeSpec = DoNativeArrayTypeSpec();
    const bool doArraySegmentHoist = DoArraySegmentHoist(ValueType::GetObject(ObjectType::Array));
    Assert(doArraySegmentHoist == DoArraySegmentHoist(ValueType::GetObject(ObjectType::ObjectWithArray)));
    const bool doArrayLengthHoist = DoArrayLengthHoist();
    if(!doArrayMissingValueCheckHoist && !doNativeArrayTypeSpec && !doArraySegmentHoist && !doArrayLengthHoist)
    {
        return kills;
    }

    // The following operations may create missing values in an array in an unlikely circumstance. Even though they don't kill
    // the fact that the 'this' parameter is an array (when implicit calls are disabled), we don't have a way to say the value
    // type is definitely array but it likely has no missing values. So, these will kill the definite value type as well, making
    // it likely array, such that the array checks will have to be redone.
    const bool useValueTypes = !IsLoopPrePass(); // Source value types are not guaranteed to be correct in a loop prepass
    switch(instr->m_opcode)
    {
        case Js::OpCode::StElemI_A:
        case Js::OpCode::StElemI_A_Strict:
        {
            Assert(instr->GetDst());
            if(!instr->GetDst()->IsIndirOpnd())
            {
                break;
            }
            const ValueType baseValueType =
                useValueTypes ? instr->GetDst()->AsIndirOpnd()->GetBaseOpnd()->GetValueType() : ValueType::Uninitialized;
            if(useValueTypes && baseValueType.IsNotArrayOrObjectWithArray())
            {
                break;
            }
            if(instr->IsProfiledInstr())
            {
                const Js::StElemInfo *const stElemInfo = instr->AsProfiledInstr()->u.stElemInfo;
                if(doArraySegmentHoist && stElemInfo->LikelyStoresOutsideHeadSegmentBounds())
                {
                    kills.SetKillsArrayHeadSegments();
                    kills.SetKillsArrayHeadSegmentLengths();
                }
                if(doArrayLengthHoist &&
                    !(useValueTypes && baseValueType.IsNotArray()) &&
                    stElemInfo->LikelyStoresOutsideArrayBounds())
                {
                    kills.SetKillsArrayLengths();
                }
            }
            break;
        }

        case Js::OpCode::DeleteElemI_A:
        case Js::OpCode::DeleteElemIStrict_A:
            Assert(instr->GetSrc1());
            if(!instr->GetSrc1()->IsIndirOpnd() ||
                (useValueTypes && instr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->GetValueType().IsNotArrayOrObjectWithArray()))
            {
                break;
            }
            if(doArrayMissingValueCheckHoist)
            {
                kills.SetKillsArraysWithNoMissingValues();
            }
            if(doArraySegmentHoist)
            {
                kills.SetKillsArrayHeadSegmentLengths();
            }
            break;

        case Js::OpCode::StFld:
        case Js::OpCode::StFldStrict:
        {
            Assert(instr->GetDst());

            if(!doArraySegmentHoist && !doArrayLengthHoist)
            {
                break;
            }

            IR::SymOpnd *const symDst = instr->GetDst()->AsSymOpnd();
            if(!symDst->IsPropertySymOpnd())
            {
                break;
            }

            IR::PropertySymOpnd *const dst = symDst->AsPropertySymOpnd();
            if(dst->m_sym->AsPropertySym()->m_propertyId != Js::PropertyIds::length)
            {
                break;
            }

            if(useValueTypes && dst->GetPropertyOwnerValueType().IsNotArray())
            {
                // Setting the 'length' property of an object that is not an array, even if it has an internal array, does
                // not kill the head segment or head segment length of any arrays.
                break;
            }

            if(doArraySegmentHoist)
            {
                kills.SetKillsArrayHeadSegmentLengths();
            }
            if(doArrayLengthHoist)
            {
                kills.SetKillsArrayLengths();
            }
            break;
        }

        case Js::OpCode::InlineArrayPush:
        {
            Assert(instr->GetSrc2());
            IR::Opnd *const arrayOpnd = instr->GetSrc1();
            Assert(arrayOpnd);

            const ValueType arrayValueType(arrayOpnd->GetValueType());

            if(!arrayOpnd->IsRegOpnd() || (useValueTypes && arrayValueType.IsNotArrayOrObjectWithArray()))
            {
                break;
            }

            if(doArrayMissingValueCheckHoist)
            {
                kills.SetKillsArraysWithNoMissingValues();
            }

            if(doArraySegmentHoist)
            {
                kills.SetKillsArrayHeadSegments();
                kills.SetKillsArrayHeadSegmentLengths();
            }

            if(doArrayLengthHoist && !(useValueTypes && arrayValueType.IsNotArray()))
            {
                kills.SetKillsArrayLengths();
            }

            // Don't kill NativeArray, if there is no mismatch between array's type and element's type.
            if(doNativeArrayTypeSpec &&
               !(useValueTypes && arrayValueType.IsNativeArray() &&
                    ((arrayValueType.IsLikelyNativeIntArray() && instr->GetSrc2()->IsInt32()) ||
                     (arrayValueType.IsLikelyNativeFloatArray() && instr->GetSrc2()->IsFloat()))
                ) &&
               !(useValueTypes && arrayValueType.IsNotNativeArray()))
            {
                kills.SetKillsNativeArrays();
            }

            break;
        }

        case Js::OpCode::InlineArrayPop:
        {
            IR::Opnd *const arrayOpnd = instr->GetSrc1();
            Assert(arrayOpnd);

            const ValueType arrayValueType(arrayOpnd->GetValueType());
            if(!arrayOpnd->IsRegOpnd() || (useValueTypes && arrayValueType.IsNotArrayOrObjectWithArray()))
            {
                break;
            }

            if(doArraySegmentHoist)
            {
                kills.SetKillsArrayHeadSegmentLengths();
            }

            if(doArrayLengthHoist && !(useValueTypes && arrayValueType.IsNotArray()))
            {
                kills.SetKillsArrayLengths();
            }
            break;
        }

        case Js::OpCode::CallDirect:
        {
            Assert(instr->GetSrc1());

            // Find the 'this' parameter and check if it's possible for it to be an array
            IR::Opnd *const arrayOpnd = instr->FindCallArgumentOpnd(1);
            Assert(arrayOpnd);
            const ValueType arrayValueType(arrayOpnd->GetValueType());
            if(!arrayOpnd->IsRegOpnd() || (useValueTypes && arrayValueType.IsNotArrayOrObjectWithArray()))
            {
                break;
            }

            const IR::JnHelperMethod helperMethod = instr->GetSrc1()->AsHelperCallOpnd()->m_fnHelper;
            if(doArrayMissingValueCheckHoist)
            {
                switch(helperMethod)
                {
                    case IR::HelperArray_Reverse:
                    case IR::HelperArray_Shift:
                    case IR::HelperArray_Splice:
                    case IR::HelperArray_Unshift:
                        kills.SetKillsArraysWithNoMissingValues();
                        break;
                }
            }

            if(doArraySegmentHoist)
            {
                switch(helperMethod)
                {
                    case IR::HelperArray_Reverse:
                    case IR::HelperArray_Shift:
                    case IR::HelperArray_Splice:
                    case IR::HelperArray_Unshift:
                        kills.SetKillsArrayHeadSegments();
                        kills.SetKillsArrayHeadSegmentLengths();
                        break;
                }
            }

            if(doArrayLengthHoist && !(useValueTypes && arrayValueType.IsNotArray()))
            {
                switch(helperMethod)
                {
                    case IR::HelperArray_Shift:
                    case IR::HelperArray_Splice:
                    case IR::HelperArray_Unshift:
                        kills.SetKillsArrayLengths();
                        break;
                }
            }

            if(doNativeArrayTypeSpec && !(useValueTypes && arrayValueType.IsNotNativeArray()))
            {
                switch(helperMethod)
                {
                    case IR::HelperArray_Reverse:
                    case IR::HelperArray_Shift:
                    case IR::HelperArray_Slice:
                    // Currently not inlined.
                    //case IR::HelperArray_Sort:
                    case IR::HelperArray_Splice:
                    case IR::HelperArray_Unshift:
                        kills.SetKillsNativeArrays();
                        break;
                }
            }
            break;
        }
    }

    return kills;
}

GlobOptBlockData const * GlobOpt::CurrentBlockData() const
{
    return &this->currentBlock->globOptData;
}

GlobOptBlockData * GlobOpt::CurrentBlockData()
{
    return &this->currentBlock->globOptData;
}

bool
GlobOpt::IsOperationThatLikelyKillsJsArraysWithNoMissingValues(IR::Instr *const instr)
{
    // StElem is profiled with information indicating whether it will likely create a missing value in the array. In that case,
    // we prefer to kill the no-missing-values information in the value so that we don't bail out in a likely circumstance.
    return
        (instr->m_opcode == Js::OpCode::StElemI_A || instr->m_opcode == Js::OpCode::StElemI_A_Strict) &&
        DoArrayMissingValueCheckHoist() &&
        instr->IsProfiledInstr() &&
        instr->AsProfiledInstr()->u.stElemInfo->LikelyCreatesMissingValue();
}

bool
GlobOpt::NeedBailOnImplicitCallForArrayCheckHoist(BasicBlock const * const block, const bool isForwardPass) const
{
    Assert(block);
    return isForwardPass && block->loop && block->loop->needImplicitCallBailoutChecksForJsArrayCheckHoist;
}

bool
GlobOpt::PrepareForIgnoringIntOverflow(IR::Instr *const instr)
{
    Assert(instr);

    const bool isBoundary = instr->m_opcode == Js::OpCode::NoIntOverflowBoundary;

    // Update the instruction's "int overflow matters" flag based on whether we are currently allowing ignoring int overflows.
    // Some operations convert their srcs to int32s, those can still ignore int overflow.
    if(instr->ignoreIntOverflowInRange)
    {
        instr->ignoreIntOverflowInRange = !intOverflowCurrentlyMattersInRange || OpCodeAttr::IsInt32(instr->m_opcode);
    }

    if(!intOverflowDoesNotMatterRange)
    {
        Assert(intOverflowCurrentlyMattersInRange);

        // There are no more ranges of instructions where int overflow does not matter, in this block.
        return isBoundary;
    }

    if(instr == intOverflowDoesNotMatterRange->LastInstr())
    {
        Assert(isBoundary);

        // Reached the last instruction in the range
        intOverflowCurrentlyMattersInRange = true;
        intOverflowDoesNotMatterRange = intOverflowDoesNotMatterRange->Next();
        return isBoundary;
    }

    if(!intOverflowCurrentlyMattersInRange)
    {
        return isBoundary;
    }

    if(instr != intOverflowDoesNotMatterRange->FirstInstr())
    {
        // Have not reached the next range
        return isBoundary;
    }

    Assert(isBoundary);

    // This is the first instruction in a range of instructions where int overflow does not matter. There can be many inputs to
    // instructions in the range, some of which are inputs to the range itself (that is, the values are not defined in the
    // range). Ignoring int overflow is only valid for int operations, so we need to ensure that all inputs to the range are
    // int (not "likely int") before ignoring any overflows in the range. Ensuring that a sym with a "likely int" value is an
    // int requires a bail-out. These bail-out check need to happen before any overflows are ignored, otherwise it's too late.
    // The backward pass tracked all inputs into the range. Iterate over them and verify the values, and insert lossless
    // conversions to int as necessary, before the first instruction in the range. If for any reason all values cannot be
    // guaranteed to be ints, the optimization will be disabled for this range.
    intOverflowCurrentlyMattersInRange = false;
    {
        BVSparse<JitArenaAllocator> tempBv1(tempAlloc);
        BVSparse<JitArenaAllocator> tempBv2(tempAlloc);

        {
            // Just renaming the temp BVs for this section to indicate how they're used so that it makes sense
            BVSparse<JitArenaAllocator> &symsToExclude = tempBv1;
            BVSparse<JitArenaAllocator> &symsToInclude = tempBv2;
#if DBG_DUMP
            SymID couldNotConvertSymId = 0;
#endif
            FOREACH_BITSET_IN_SPARSEBV(id, intOverflowDoesNotMatterRange->SymsRequiredToBeInt())
            {
                Sym *const sym = func->m_symTable->Find(id);
                Assert(sym);

                // Some instructions with property syms are also tracked by the backward pass, and may be included in the range
                // (LdSlot for instance). These property syms don't get their values until either copy-prop resolves a value for
                // them, or a new value is created once the use of the property sym is reached. In either case, we're not that
                // far yet, so we need to find the future value of the property sym by evaluating copy-prop in reverse.
                Value *const value = sym->IsStackSym() ? CurrentBlockData()->FindValue(sym) : CurrentBlockData()->FindFuturePropertyValue(sym->AsPropertySym());
                if(!value)
                {
#if DBG_DUMP
                    couldNotConvertSymId = id;
#endif
                    intOverflowCurrentlyMattersInRange = true;
                    BREAK_BITSET_IN_SPARSEBV;
                }

                const bool isInt32OrUInt32Float =
                    value->GetValueInfo()->IsFloatConstant() &&
                    Js::JavascriptNumber::IsInt32OrUInt32(value->GetValueInfo()->AsFloatConstant()->FloatValue());
                if(value->GetValueInfo()->IsInt() || isInt32OrUInt32Float)
                {
                    if(!IsLoopPrePass())
                    {
                        // Input values that are already int can be excluded from int-specialization. We can treat unsigned
                        // int32 values as int32 values (ignoring the overflow), since the values will only be used inside the
                        // range where overflow does not matter.
                        symsToExclude.Set(sym->m_id);
                    }
                    continue;
                }

                if(!DoAggressiveIntTypeSpec() || !value->GetValueInfo()->IsLikelyInt())
                {
                    // When aggressive int specialization is off, syms with "likely int" values cannot be forced to int since
                    // int bail-out checks are not allowed in that mode. Similarly, with aggressive int specialization on, it
                    // wouldn't make sense to force non-"likely int" values to int since it would almost guarantee a bail-out at
                    // runtime. In both cases, just disable ignoring overflow for this range.
#if DBG_DUMP
                    couldNotConvertSymId = id;
#endif
                    intOverflowCurrentlyMattersInRange = true;
                    BREAK_BITSET_IN_SPARSEBV;
                }

                if(IsLoopPrePass())
                {
                    // The loop prepass does not modify bit-vectors. Since it doesn't add bail-out checks, it also does not need
                    // to specialize anything up-front. It only needs to be consistent in how it determines whether to allow
                    // ignoring overflow for a range, based on the values of inputs into the range.
                    continue;
                }

                // Since input syms are tracked in the backward pass, where there is no value tracking, it will not be aware of
                // copy-prop. If a copy-prop sym is available, it will be used instead, so exclude the original sym and include
                // the copy-prop sym for specialization.
                StackSym *const copyPropSym = CurrentBlockData()->GetCopyPropSym(sym, value);
                if(copyPropSym)
                {
                    symsToExclude.Set(sym->m_id);
                    Assert(!symsToExclude.Test(copyPropSym->m_id));

                    const bool needsToBeLossless =
                        !intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt()->Test(sym->m_id);
                    if(intOverflowDoesNotMatterRange->SymsRequiredToBeInt()->Test(copyPropSym->m_id) ||
                        symsToInclude.TestAndSet(copyPropSym->m_id))
                    {
                        // The copy-prop sym is already included
                        if(needsToBeLossless)
                        {
                            // The original sym needs to be lossless, so make the copy-prop sym lossless as well.
                            intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt()->Clear(copyPropSym->m_id);
                        }
                    }
                    else if(!needsToBeLossless)
                    {
                        // The copy-prop sym was not included before, and the original sym can be lossy, so make it lossy.
                        intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt()->Set(copyPropSym->m_id);
                    }
                }
                else if(!sym->IsStackSym())
                {
                    // Only stack syms can be converted to int, and copy-prop syms are stack syms. If a copy-prop sym was not
                    // found for the property sym, we can't ignore overflows in this range.
#if DBG_DUMP
                    couldNotConvertSymId = id;
#endif
                    intOverflowCurrentlyMattersInRange = true;
                    BREAK_BITSET_IN_SPARSEBV;
                }
            } NEXT_BITSET_IN_SPARSEBV;

            if(intOverflowCurrentlyMattersInRange)
            {
#if DBG_DUMP
                if(PHASE_TRACE(Js::TrackCompoundedIntOverflowPhase, func) && !IsLoopPrePass())
                {
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                    Output::Print(
                        _u("TrackCompoundedIntOverflow - Top function: %s (%s), Phase: %s, Block: %u, Disabled ignoring overflows\n"),
                        func->GetJITFunctionBody()->GetDisplayName(),
                        func->GetDebugNumberSet(debugStringBuffer),
                        Js::PhaseNames[Js::ForwardPhase],
                        currentBlock->GetBlockNum());
                    Output::Print(_u("    Input sym could not be turned into an int:   %u\n"), couldNotConvertSymId);
                    Output::Print(_u("    First instr: "));
                    instr->m_next->Dump();
                    Output::Flush();
                }
#endif
                intOverflowDoesNotMatterRange = intOverflowDoesNotMatterRange->Next();
                return isBoundary;
            }

            if(IsLoopPrePass())
            {
                return isBoundary;
            }

            // Update the syms to specialize after enumeration
            intOverflowDoesNotMatterRange->SymsRequiredToBeInt()->Minus(&symsToExclude);
            intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt()->Minus(&symsToExclude);
            intOverflowDoesNotMatterRange->SymsRequiredToBeInt()->Or(&symsToInclude);
        }

        {
            // Exclude syms that are already live as lossless int32, and exclude lossy conversions of syms that are already live
            // as lossy int32.
            //     symsToExclude = liveInt32Syms - liveLossyInt32Syms                   // syms live as lossless int
            //     lossySymsToExclude = symsRequiredToBeLossyInt & liveLossyInt32Syms;  // syms we want as lossy int that are already live as lossy int
            //     symsToExclude |= lossySymsToExclude
            //     symsRequiredToBeInt -= symsToExclude
            //     symsRequiredToBeLossyInt -= symsToExclude
            BVSparse<JitArenaAllocator> &symsToExclude = tempBv1;
            BVSparse<JitArenaAllocator> &lossySymsToExclude = tempBv2;
            symsToExclude.Minus(CurrentBlockData()->liveInt32Syms, CurrentBlockData()->liveLossyInt32Syms);
            lossySymsToExclude.And(
                intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt(),
                CurrentBlockData()->liveLossyInt32Syms);
            symsToExclude.Or(&lossySymsToExclude);
            intOverflowDoesNotMatterRange->SymsRequiredToBeInt()->Minus(&symsToExclude);
            intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt()->Minus(&symsToExclude);
        }

#if DBG
        {
            // Verify that the syms to be converted are live
            //     liveSyms = liveInt32Syms | liveFloat64Syms | liveVarSyms
            //     deadSymsRequiredToBeInt = symsRequiredToBeInt - liveSyms
            BVSparse<JitArenaAllocator> &liveSyms = tempBv1;
            BVSparse<JitArenaAllocator> &deadSymsRequiredToBeInt = tempBv2;
            liveSyms.Or(CurrentBlockData()->liveInt32Syms, CurrentBlockData()->liveFloat64Syms);
            liveSyms.Or(CurrentBlockData()->liveVarSyms);
            deadSymsRequiredToBeInt.Minus(intOverflowDoesNotMatterRange->SymsRequiredToBeInt(), &liveSyms);
            Assert(deadSymsRequiredToBeInt.IsEmpty());
        }
#endif
    }

    // Int-specialize the syms before the first instruction of the range (the current instruction)
    intOverflowDoesNotMatterRange->SymsRequiredToBeInt()->Minus(intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt());

#if DBG_DUMP
    if(PHASE_TRACE(Js::TrackCompoundedIntOverflowPhase, func))
    {
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        Output::Print(
            _u("TrackCompoundedIntOverflow - Top function: %s (%s), Phase: %s, Block: %u\n"),
            func->GetJITFunctionBody()->GetDisplayName(),
            func->GetDebugNumberSet(debugStringBuffer),
            Js::PhaseNames[Js::ForwardPhase],
            currentBlock->GetBlockNum());
        Output::Print(_u("    Input syms to be int-specialized (lossless): "));
        intOverflowDoesNotMatterRange->SymsRequiredToBeInt()->Dump();
        Output::Print(_u("    Input syms to be converted to int (lossy):   "));
        intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt()->Dump();
        Output::Print(_u("    First instr: "));
        instr->m_next->Dump();
        Output::Flush();
    }
#endif

    ToInt32(intOverflowDoesNotMatterRange->SymsRequiredToBeInt(), currentBlock, false /* lossy */, instr);
    ToInt32(intOverflowDoesNotMatterRange->SymsRequiredToBeLossyInt(), currentBlock, true /* lossy */, instr);
    return isBoundary;
}

void
GlobOpt::VerifyIntSpecForIgnoringIntOverflow(IR::Instr *const instr)
{
    if(intOverflowCurrentlyMattersInRange || IsLoopPrePass())
    {
        return;
    }

    Assert(instr->m_opcode != Js::OpCode::Mul_I4 ||
        (instr->m_opcode == Js::OpCode::Mul_I4 && !instr->ShouldCheckFor32BitOverflow() && instr->ShouldCheckForNon32BitOverflow() ));

    // Instructions that are marked as "overflow doesn't matter" in the range must guarantee that they operate on int values and
    // result in int values, for ignoring overflow to be valid. So, int-specialization is required for such instructions in the
    // range. Ld_A is an exception because it only specializes if the src sym is available as a required specialized sym, and it
    // doesn't generate bailouts or cause ignoring int overflow to be invalid.
    // MULs are allowed to start a region and have BailOutInfo since they will bailout on non-32 bit overflow.
    if(instr->m_opcode == Js::OpCode::Ld_A ||
       ((!instr->HasBailOutInfo() || instr->m_opcode == Js::OpCode::Mul_I4) &&
        (!instr->GetDst() || instr->GetDst()->IsInt32()) &&
        (!instr->GetSrc1() || instr->GetSrc1()->IsInt32()) &&
        (!instr->GetSrc2() || instr->GetSrc2()->IsInt32())))
    {
        return;
    }

    if (!instr->HasBailOutInfo() && !instr->HasAnySideEffects())
    {
        return;
    }

    // This can happen for Neg_A if it needs to bail out on negative zero, and perhaps other cases as well. It's too late to fix
    // the problem (overflows may already be ignored), so handle it by bailing out at compile-time and disabling tracking int
    // overflow.
    Assert(!func->IsTrackCompoundedIntOverflowDisabled());

    if(PHASE_TRACE(Js::BailOutPhase, this->func))
    {
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        Output::Print(
            _u("BailOut (compile-time): function: %s (%s) instr: "),
            func->GetJITFunctionBody()->GetDisplayName(),
            func->GetDebugNumberSet(debugStringBuffer));
#if DBG_DUMP
        instr->Dump();
#else
        Output::Print(_u("%s "), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
#endif
        Output::Print(_u("(overflow does not matter but could not int-spec or needed bailout)\n"));
        Output::Flush();
    }

    if(func->IsTrackCompoundedIntOverflowDisabled())
    {
        // Tracking int overflows is already off for some reason. Prevent trying to rejit again because it won't help and the
        // same thing will happen again and cause an infinite loop. Just abort jitting this function.
        if(PHASE_TRACE(Js::BailOutPhase, this->func))
        {
            Output::Print(_u("    Aborting JIT because TrackIntOverflow is already off\n"));
            Output::Flush();
        }
        throw Js::OperationAbortedException();
    }

    throw Js::RejitException(RejitReason::TrackIntOverflowDisabled);
}

// It makes lowering easier if it can assume that the first src is never a constant,
// at least for commutative operators. For non-commutative, just hoist the constant.
void
GlobOpt::PreLowerCanonicalize(IR::Instr *instr, Value **pSrc1Val, Value **pSrc2Val)
{
    IR::Opnd *dst = instr->GetDst();
    IR::Opnd *src1 = instr->GetSrc1();
    IR::Opnd *src2 = instr->GetSrc2();

    if (src1->IsImmediateOpnd())
    {
        // Swap for dst, src
    }
    else if (src2 && dst && src2->IsRegOpnd())
    {
        if (src2->GetIsDead() && !src1->GetIsDead() && !src1->IsEqual(dst))
        {
            // Swap if src2 is dead, as the reg can be reuse for the dst for opEqs like on x86 (ADD r1, r2)
        }
        else if (src2->IsEqual(dst))
        {
            // Helps lowering of opEqs
        }
        else
        {
            return;
        }
        // Make sure we don't swap 2 srcs with valueOf calls.
        if (OpCodeAttr::OpndHasImplicitCall(instr->m_opcode))
        {
            if (instr->IsBranchInstr())
            {
                if (!src1->GetValueType().IsPrimitive() || !src2->GetValueType().IsPrimitive())
                {
                    return;
                }
            }
            else if (!src1->GetValueType().IsPrimitive() && !src2->GetValueType().IsPrimitive())
            {
                return;
            }
        }
    }
    else
    {
        return;
    }
    Js::OpCode opcode = instr->m_opcode;
    switch (opcode)
    {
    case Js::OpCode::And_A:
    case Js::OpCode::Mul_A:
    case Js::OpCode::Or_A:
    case Js::OpCode::Xor_A:
    case Js::OpCode::And_I4:
    case Js::OpCode::Mul_I4:
    case Js::OpCode::Or_I4:
    case Js::OpCode::Xor_I4:
    case Js::OpCode::Add_I4:
swap_srcs:
        if (!instr->GetSrc2()->IsImmediateOpnd())
        {
            instr->m_opcode = opcode;
            instr->SwapOpnds();

            Value *tempVal = *pSrc1Val;
            *pSrc1Val = *pSrc2Val;
            *pSrc2Val = tempVal;
            return;
        }
        break;

    case Js::OpCode::BrSrEq_A:
    case Js::OpCode::BrSrNotNeq_A:
    case Js::OpCode::BrEq_I4:
        goto swap_srcs;

    case Js::OpCode::BrSrNeq_A:
    case Js::OpCode::BrNeq_A:
    case Js::OpCode::BrSrNotEq_A:
    case Js::OpCode::BrNotEq_A:
    case Js::OpCode::BrNeq_I4:
        goto swap_srcs;

    case Js::OpCode::BrGe_A:
        opcode = Js::OpCode::BrLe_A;
        goto swap_srcs;

    case Js::OpCode::BrNotGe_A:
        opcode = Js::OpCode::BrNotLe_A;
        goto swap_srcs;

    case Js::OpCode::BrGe_I4:
        opcode = Js::OpCode::BrLe_I4;
        goto swap_srcs;

    case Js::OpCode::BrGt_A:
        opcode = Js::OpCode::BrLt_A;
        goto swap_srcs;

    case Js::OpCode::BrNotGt_A:
        opcode = Js::OpCode::BrNotLt_A;
        goto swap_srcs;

    case Js::OpCode::BrGt_I4:
        opcode = Js::OpCode::BrLt_I4;
        goto swap_srcs;

    case Js::OpCode::BrLe_A:
        opcode = Js::OpCode::BrGe_A;
        goto swap_srcs;

    case Js::OpCode::BrNotLe_A:
        opcode = Js::OpCode::BrNotGe_A;
        goto swap_srcs;

    case Js::OpCode::BrLe_I4:
        opcode = Js::OpCode::BrGe_I4;
        goto swap_srcs;

    case Js::OpCode::BrLt_A:
        opcode = Js::OpCode::BrGt_A;
        goto swap_srcs;

    case Js::OpCode::BrNotLt_A:
        opcode = Js::OpCode::BrNotGt_A;
        goto swap_srcs;

    case Js::OpCode::BrLt_I4:
        opcode = Js::OpCode::BrGt_I4;
        goto swap_srcs;

    case Js::OpCode::BrEq_A:
    case Js::OpCode::BrNotNeq_A:
    case Js::OpCode::CmEq_A:
    case Js::OpCode::CmNeq_A:
        // this == "" not the same as "" == this...
        if (!src1->IsImmediateOpnd() && (!src1->GetValueType().IsPrimitive() || !src2->GetValueType().IsPrimitive()))
        {
            return;
        }
        goto swap_srcs;

    case Js::OpCode::CmGe_A:
        if (!src1->IsImmediateOpnd() && (!src1->GetValueType().IsPrimitive() || !src2->GetValueType().IsPrimitive()))
        {
            return;
        }
        opcode = Js::OpCode::CmLe_A;
        goto swap_srcs;

    case Js::OpCode::CmGt_A:
        if (!src1->IsImmediateOpnd() && (!src1->GetValueType().IsPrimitive() || !src2->GetValueType().IsPrimitive()))
        {
            return;
        }
        opcode = Js::OpCode::CmLt_A;
        goto swap_srcs;

    case Js::OpCode::CmLe_A:
        if (!src1->IsImmediateOpnd() && (!src1->GetValueType().IsPrimitive() || !src2->GetValueType().IsPrimitive()))
        {
            return;
        }
        opcode = Js::OpCode::CmGe_A;
        goto swap_srcs;

    case Js::OpCode::CmLt_A:
        if (!src1->IsImmediateOpnd() && (!src1->GetValueType().IsPrimitive() || !src2->GetValueType().IsPrimitive()))
        {
            return;
        }
        opcode = Js::OpCode::CmGt_A;
        goto swap_srcs;

    case Js::OpCode::CallI:
    case Js::OpCode::CallIFixed:
    case Js::OpCode::NewScObject:
    case Js::OpCode::NewScObjectSpread:
    case Js::OpCode::NewScObjArray:
    case Js::OpCode::NewScObjArraySpread:
    case Js::OpCode::NewScObjectNoCtor:
        // Don't insert load to register if the function operand is a fixed function.
        if (instr->HasFixedFunctionAddressTarget())
        {
            return;
        }
        break;

        // Can't do add because <32 + "Hello"> isn't equal to <"Hello" + 32>
        // Lower can do the swap. Other op-codes listed below don't need immediate source hoisting, as the fast paths handle it,
        // or the lowering handles the hoisting.
    case Js::OpCode::Add_A:
        if (src1->IsFloat())
        {
            goto swap_srcs;
        }
        return;

    case Js::OpCode::Sub_I4:
    case Js::OpCode::Neg_I4:
    case Js::OpCode::Not_I4:
    case Js::OpCode::NewScFunc:
    case Js::OpCode::NewScGenFunc:
    case Js::OpCode::NewScArray:
    case Js::OpCode::NewScIntArray:
    case Js::OpCode::NewScFltArray:
    case Js::OpCode::NewScArrayWithMissingValues:
    case Js::OpCode::NewRegEx:
    case Js::OpCode::Ld_A:
    case Js::OpCode::Ld_I4:
    case Js::OpCode::ThrowRuntimeError:
    case Js::OpCode::TrapIfMinIntOverNegOne:
    case Js::OpCode::TrapIfTruncOverflow:
    case Js::OpCode::TrapIfZero:
    case Js::OpCode::FromVar:
    case Js::OpCode::Conv_Prim:
    case Js::OpCode::LdC_A_I4:
    case Js::OpCode::LdStr:
    case Js::OpCode::InitFld:
    case Js::OpCode::InitRootFld:
    case Js::OpCode::StartCall:
    case Js::OpCode::ArgOut_A:
    case Js::OpCode::ArgOut_A_Inline:
    case Js::OpCode::ArgOut_A_Dynamic:
    case Js::OpCode::ArgOut_A_FromStackArgs:
    case Js::OpCode::ArgOut_A_InlineBuiltIn:
    case Js::OpCode::ArgOut_A_InlineSpecialized:
    case Js::OpCode::ArgOut_A_SpreadArg:
    case Js::OpCode::InlineeEnd:
    case Js::OpCode::EndCallForPolymorphicInlinee:
    case Js::OpCode::InlineeMetaArg:
    case Js::OpCode::InlineBuiltInEnd:
    case Js::OpCode::InlineNonTrackingBuiltInEnd:
    case Js::OpCode::CallHelper:
    case Js::OpCode::LdElemUndef:
    case Js::OpCode::LdElemUndefScoped:
    case Js::OpCode::RuntimeTypeError:
    case Js::OpCode::RuntimeReferenceError:
    case Js::OpCode::Ret:
    case Js::OpCode::NewScObjectSimple:
    case Js::OpCode::NewScObjectLiteral:
    case Js::OpCode::StFld:
    case Js::OpCode::StRootFld:
    case Js::OpCode::StSlot:
    case Js::OpCode::StSlotChkUndecl:
    case Js::OpCode::StElemC:
    case Js::OpCode::StArrSegElemC:
    case Js::OpCode::StElemI_A:
    case Js::OpCode::StElemI_A_Strict:
    case Js::OpCode::CallDirect:
    case Js::OpCode::BrNotHasSideEffects:
    case Js::OpCode::NewConcatStrMulti:
    case Js::OpCode::NewConcatStrMultiBE:
    case Js::OpCode::ExtendArg_A:
#ifdef ENABLE_DOM_FAST_PATH
    case Js::OpCode::DOMFastPathGetter:
    case Js::OpCode::DOMFastPathSetter:
#endif
    case Js::OpCode::NewScopeSlots:
    case Js::OpCode::NewScopeSlotsWithoutPropIds:
    case Js::OpCode::NewStackScopeSlots:
    case Js::OpCode::IsInst:
    case Js::OpCode::BailOnEqual:
    case Js::OpCode::BailOnNotEqual:
    case Js::OpCode::StArrViewElem:
        return;
    }

    if (!src1->IsImmediateOpnd())
    {
        return;
    }

    // The fast paths or lowering of the remaining instructions may not support handling immediate opnds for the first src. The
    // immediate src1 is hoisted here into a separate instruction.
    if (src1->IsIntConstOpnd())
    {
        IR::Instr *newInstr = instr->HoistSrc1(Js::OpCode::Ld_I4);
        ToInt32Dst(newInstr, newInstr->GetDst()->AsRegOpnd(), this->currentBlock);
    }
    else if (src1->IsInt64ConstOpnd())
    {
        instr->HoistSrc1(Js::OpCode::Ld_I4);
    }
    else
    {
        instr->HoistSrc1(Js::OpCode::Ld_A);
    }
    src1 = instr->GetSrc1();
    src1->AsRegOpnd()->m_sym->SetIsConst();
}

// Clear the ValueMap pf the values invalidated by this instr.
void
GlobOpt::ProcessKills(IR::Instr *instr)
{
    this->ProcessFieldKills(instr);
    this->ProcessValueKills(instr);
    this->ProcessArrayValueKills(instr);
}

bool
GlobOpt::OptIsInvariant(IR::Opnd *src, BasicBlock *block, Loop *loop, Value *srcVal, bool isNotTypeSpecConv, bool allowNonPrimitives)
{
    if(!loop->CanHoistInvariants())
    {
        return false;
    }

    Sym *sym;

    switch(src->GetKind())
    {
    case IR::OpndKindAddr:
    case IR::OpndKindFloatConst:
    case IR::OpndKindIntConst:
        return true;

    case IR::OpndKindReg:
        sym = src->AsRegOpnd()->m_sym;
        break;

    case IR::OpndKindSym:
        sym = src->AsSymOpnd()->m_sym;
        if (src->AsSymOpnd()->IsPropertySymOpnd())
        {
            if (src->AsSymOpnd()->AsPropertySymOpnd()->IsTypeChecked())
            {
                // We do not handle hoisting these yet.  We might be hoisting this across the instr with the type check protecting this one.
                // And somehow, the dead-store pass now removes the type check on that instr later on...
                // For CheckFixedFld, there is no benefit hoisting these if they don't have a type check as they won't generate code.
                return false;
            }
        }

        break;

    case IR::OpndKindHelperCall:
        // Helper calls, like the private slot getter, can be invariant.
        // Consider moving more math builtin to invariant?
        return HelperMethodAttributes::IsInVariant(src->AsHelperCallOpnd()->m_fnHelper);

    default:
        return false;
    }
    return OptIsInvariant(sym, block, loop, srcVal, isNotTypeSpecConv, allowNonPrimitives);
}

bool
GlobOpt::OptIsInvariant(Sym *sym, BasicBlock *block, Loop *loop, Value *srcVal, bool isNotTypeSpecConv, bool allowNonPrimitives, Value **loopHeadValRef)
{
    Value *localLoopHeadVal;
    if(!loopHeadValRef)
    {
        loopHeadValRef = &localLoopHeadVal;
    }
    Value *&loopHeadVal = *loopHeadValRef;
    loopHeadVal = nullptr;

    if(!loop->CanHoistInvariants())
    {
        return false;
    }

    if (sym->IsStackSym())
    {
        if (sym->AsStackSym()->IsTypeSpec())
        {
            StackSym *varSym = sym->AsStackSym()->GetVarEquivSym(this->func);
            // Make sure the int32/float64 version of this is available.
            // Note: We could handle this by converting the src, but usually the
            // conversion is hoistable if this is hoistable anyway.
            // In some weird cases it may not be however, so we'll bail out.
            if (sym->AsStackSym()->IsInt32())
            {
                Assert(block->globOptData.liveInt32Syms->Test(varSym->m_id));
                if (!loop->landingPad->globOptData.liveInt32Syms->Test(varSym->m_id) ||
                    (loop->landingPad->globOptData.liveLossyInt32Syms->Test(varSym->m_id) &&
                    !block->globOptData.liveLossyInt32Syms->Test(varSym->m_id)))
                {
                    // Either the int32 sym is not live in the landing pad, or it's lossy in the landing pad and the
                    // instruction's block is using the lossless version. In either case, the instruction cannot be hoisted
                    // without doing a conversion of this operand.
                    return false;
                }
            }
            else if (sym->AsStackSym()->IsFloat64())
            {
                if (!loop->landingPad->globOptData.liveFloat64Syms->Test(varSym->m_id))
                {
                    return false;
                }
            }
#ifdef ENABLE_SIMDJS
            else
            {
                Assert(sym->AsStackSym()->IsSimd128());
                if (!loop->landingPad->globOptData.liveSimd128F4Syms->Test(varSym->m_id) && !loop->landingPad->globOptData.liveSimd128I4Syms->Test(varSym->m_id))
                {
                    return false;
                }
            }
#endif
            sym = sym->AsStackSym()->GetVarEquivSym(this->func);
        }
        else
        {
            // Make sure the var version of this is available.
            // Note: We could handle this by converting the src, but usually the
            // conversion is hoistable if this is hoistable anyway.
            // In some weird cases it may not be however, so we'll bail out.
            if (!loop->landingPad->globOptData.liveVarSyms->Test(sym->m_id))
            {
                return false;
            }
        }
    }
    else if (sym->IsPropertySym())
    {
        if (!loop->landingPad->globOptData.liveFields->Test(sym->m_id))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // We rely on having a value.
    if (srcVal == NULL)
    {
        return false;
    }

    // A symbol is invariant if its current value is the same as it was upon entering the loop.
    loopHeadVal = loop->landingPad->globOptData.FindValue(sym);
    if (loopHeadVal == NULL || loopHeadVal->GetValueNumber() != srcVal->GetValueNumber())
    {
        return false;
    }

    // Can't hoist non-primitives, unless we have safeguards against valueof/tostring.  Additionally, we need to consider
    // the value annotations on the source *before* the loop: if we hoist this instruction outside the loop, we can't
    // necessarily rely on type annotations added (and enforced) earlier in the loop's body.
    //
    // It might look as though !loopHeadVal->GetValueInfo()->IsPrimitive() implies
    // !loop->landingPad->globOptData.IsTypeSpecialized(sym), but it turns out that this is not always the case.  We
    // encountered a test case in which we had previously hoisted a FromVar (to float 64) instruction, but its bailout code was
    // BailoutPrimitiveButString, rather than BailoutNumberOnly, which would have allowed us to conclude that the dest was
    // definitely a float64.  Instead, it was only *likely* a float64, causing IsPrimitive to return false.
    if (!allowNonPrimitives && !loopHeadVal->GetValueInfo()->IsPrimitive() && !loop->landingPad->globOptData.IsTypeSpecialized(sym))
    {
        return false;
    }

    if(!isNotTypeSpecConv && loop->symsDefInLoop->Test(sym->m_id))
    {
        // Typically, a sym is considered invariant if it has the same value in the current block and in the loop landing pad.
        // The sym may have had a different value earlier in the loop or on the back-edge, but as long as it's reassigned to its
        // value outside the loop, it would be considered invariant in this block. Consider that case:
        //     s1 = s2[invariant]
        //     <loop start>
        //         s1 = s2[invariant]
        //                              // s1 now has the same value as in the landing pad, and is considered invariant
        //         s1 += s3
        //                              // s1 is not invariant here, or on the back-edge
        //         ++s3                 // s3 is not invariant, so the add above cannot be hoisted
        //     <loop end>
        //
        // A problem occurs at the point of (s1 += s3) when:
        //     - At (s1 = s2) inside the loop, s1 was made to be the sym store of that value. This by itself is legal, because
        //       after that transfer, s1 and s2 have the same value.
        //     - (s1 += s3) is type-specialized but s1 is not specialized in the loop header. This happens when s1 is not
        //       specialized entering the loop, and since s1 is not used before it's defined in the loop, it's not specialized
        //       on back-edges.
        //
        // With that, at (s1 += s3), the conversion of s1 to the type-specialized version would be hoisted because s1 is
        // invariant just before that instruction. Since this add is specialized, the specialized version of the sym is modified
        // in the loop without a reassignment at (s1 = s2) inside the loop, and (s1 += s3) would then use an incorrect value of
        // s1 (it would use the value of s1 from the previous loop iteration, instead of using the value of s2).
        //
        // The problem here, is that we cannot hoist the conversion of s1 into its specialized version across the assignment
        // (s1 = s2) inside the loop. So for the purposes of type specialization, don't consider a sym invariant if it has a def
        // inside the loop.
        return false;
    }

    // For values with an int range, require additionally that the range is the same as in the landing pad, as the range may
    // have been changed on this path based on branches, and int specialization and invariant hoisting may rely on the range
    // being the same. For type spec conversions, only require that if the value is an int constant in the current block, that
    // it is also an int constant with the same value in the landing pad. Other range differences don't matter for type spec.
    IntConstantBounds srcIntConstantBounds, loopHeadIntConstantBounds;
    if(srcVal->GetValueInfo()->TryGetIntConstantBounds(&srcIntConstantBounds) &&
        (isNotTypeSpecConv || srcIntConstantBounds.IsConstant()) &&
        (
            !loopHeadVal->GetValueInfo()->TryGetIntConstantBounds(&loopHeadIntConstantBounds) ||
            loopHeadIntConstantBounds.LowerBound() != srcIntConstantBounds.LowerBound() ||
            loopHeadIntConstantBounds.UpperBound() != srcIntConstantBounds.UpperBound()
        ))
    {
        return false;
    }

    // If the loopHeadVal is primitive, the current value should be as well.  This really should be
    // srcVal->GetValueInfo()->IsPrimitive() instead of IsLikelyPrimitive, but this stronger assertion
    // doesn't hold in some cases when this method is called out of the array code.
    Assert((!loopHeadVal->GetValueInfo()->IsPrimitive()) || srcVal->GetValueInfo()->IsLikelyPrimitive());

    return true;
}

bool
GlobOpt::OptIsInvariant(
    IR::Instr *instr,
    BasicBlock *block,
    Loop *loop,
    Value *src1Val,
    Value *src2Val,
    bool isNotTypeSpecConv,
    const bool forceInvariantHoisting)
{
    if (!loop->CanHoistInvariants())
    {
        return false;
    }
    if (!OpCodeAttr::CanCSE(instr->m_opcode))
    {
        return false;
    }

    bool allowNonPrimitives = !OpCodeAttr::OpndHasImplicitCall(instr->m_opcode);

    switch(instr->m_opcode)
    {
        // Can't legally hoist these
    case Js::OpCode::LdLen_A:
        return false;

        //Can't Hoist BailOnNotStackArgs, as it is necessary as InlineArgsOptimization relies on this opcode
        //to decide whether to throw rejit exception or not.
    case Js::OpCode::BailOnNotStackArgs:
        return false;

        // Usually not worth hoisting these
    case Js::OpCode::LdStr:
    case Js::OpCode::Ld_A:
    case Js::OpCode::Ld_I4:
    case Js::OpCode::LdC_A_I4:
        if(!forceInvariantHoisting)
        {
            return false;
        }
        break;

        // Can't hoist these outside the function it's for. The LdArgumentsFromFrame for an inlinee depends on the inlinee meta arg
        // that holds the arguments object, which is only initialized at the start of the inlinee. So, can't hoist this outside the
        // inlinee.
    case Js::OpCode::LdArgumentsFromFrame:
        if(instr->m_func != loop->GetFunc())
        {
            return false;
        }
        break;

    case Js::OpCode::FromVar:
        if (instr->HasBailOutInfo())
        {
            allowNonPrimitives = true;
        }
        break;
    case Js::OpCode::CheckObjType:
        // Bug 11712101: If the operand is a field, ensure that its containing object type is invariant
        // before hoisting -- that is, don't hoist a CheckObjType over a DeleteFld on that object.
        // (CheckObjType only checks the operand and its immediate parent, so we don't need to go
        // any farther up the object graph.)
        Assert(instr->GetSrc1());
        PropertySym *propertySym = instr->GetSrc1()->AsPropertySymOpnd()->GetPropertySym();
        if (propertySym->HasObjectTypeSym()) {
            StackSym *objectTypeSym = propertySym->GetObjectTypeSym();
            if (!this->OptIsInvariant(objectTypeSym, block, loop, this->CurrentBlockData()->FindValue(objectTypeSym), true, true)) {
                return false;
            }
        }

        break;
    }

    IR::Opnd *dst = instr->GetDst();

    if (dst && !dst->IsRegOpnd())
    {
        return false;
    }

    IR::Opnd *src1 = instr->GetSrc1();

    if (src1)
    {
        if (!this->OptIsInvariant(src1, block, loop, src1Val, isNotTypeSpecConv, allowNonPrimitives))
        {
            return false;
        }

        IR::Opnd *src2 = instr->GetSrc2();

        if (src2)
        {
            if (!this->OptIsInvariant(src2, block, loop, src2Val, isNotTypeSpecConv, allowNonPrimitives))
            {
                return false;
            }
        }
    }

    return true;
}

bool
GlobOpt::OptDstIsInvariant(IR::RegOpnd *dst)
{
    StackSym *dstSym = dst->m_sym;
    if (dstSym->IsTypeSpec())
    {
        // The type-specialized sym may be single def, but not the original...
        dstSym = dstSym->GetVarEquivSym(this->func);
    }

    return (dstSym->m_isSingleDef);
}

void
GlobOpt::OptHoistUpdateValueType(
    Loop* loop,
    IR::Instr* instr,
    IR::Opnd* srcOpnd,
    Value* opndVal)
{
    if (opndVal == nullptr || instr->m_opcode == Js::OpCode::FromVar)
    {
        return;
    }

    Sym* opndSym = srcOpnd->GetSym();;

    if (opndSym)
    {
        BasicBlock* landingPad = loop->landingPad;
        Value* opndValueInLandingPad = landingPad->globOptData.FindValue(opndSym);
        Assert(opndVal->GetValueNumber() == opndValueInLandingPad->GetValueNumber());

        ValueType opndValueTypeInLandingPad = opndValueInLandingPad->GetValueInfo()->Type();

        if (srcOpnd->GetValueType() != opndValueTypeInLandingPad)
        {
            if (instr->m_opcode == Js::OpCode::SetConcatStrMultiItemBE)
            {
                Assert(!opndValueTypeInLandingPad.IsString());
                Assert(instr->GetDst());

                IR::RegOpnd* strOpnd = IR::RegOpnd::New(TyVar, instr->m_func);
                strOpnd->SetValueType(ValueType::String);
                strOpnd->SetValueTypeFixed();
                IR::Instr* convPrimStrInstr =
                    IR::Instr::New(Js::OpCode::Conv_PrimStr, strOpnd, srcOpnd->Use(instr->m_func), instr->m_func);
                instr->ReplaceSrc(srcOpnd, strOpnd);

                if (loop->bailOutInfo->bailOutInstr)
                {
                    loop->bailOutInfo->bailOutInstr->InsertBefore(convPrimStrInstr);
                }
                else
                {
                    landingPad->InsertAfter(convPrimStrInstr);
                }
            }

            srcOpnd->SetValueType(opndValueTypeInLandingPad);
        }


        if (opndSym->IsPropertySym())
        {
            // Also fix valueInfo on objPtr
            StackSym* opndObjPtrSym = opndSym->AsPropertySym()->m_stackSym;
            Value* opndObjPtrSymValInLandingPad = landingPad->globOptData.FindValue(opndObjPtrSym);
            ValueInfo* opndObjPtrSymValueInfoInLandingPad = opndObjPtrSymValInLandingPad->GetValueInfo();

            srcOpnd->AsSymOpnd()->SetPropertyOwnerValueType(opndObjPtrSymValueInfoInLandingPad->Type());
        }
    }
}

void
GlobOpt::OptHoistInvariant(
    IR::Instr *instr,
    BasicBlock *block,
    Loop *loop,
    Value *dstVal,
    Value *const src1Val,
    Value *const src2Val,
    bool isNotTypeSpecConv,
    bool lossy,
    IR::BailOutKind bailoutKind)
{
    BasicBlock *landingPad = loop->landingPad;

    IR::Opnd* src1 = instr->GetSrc1();
    if (src1)
    {
        // We are hoisting this instruction possibly past other uses, which might invalidate the last use info. Clear it.
        OptHoistUpdateValueType(loop, instr, src1, src1Val);

        if (src1->IsRegOpnd())
        {
            src1->AsRegOpnd()->m_isTempLastUse = false;
        }

        IR::Opnd* src2 = instr->GetSrc2();
        if (src2)
        {
            OptHoistUpdateValueType(loop, instr, src2, src2Val);

            if (src2->IsRegOpnd())
            {
                src2->AsRegOpnd()->m_isTempLastUse = false;
            }
        }
    }

    IR::RegOpnd *dst = instr->GetDst() ? instr->GetDst()->AsRegOpnd() : nullptr;
    if(dst)
    {
        switch (instr->m_opcode)
        {
        case Js::OpCode::CmEq_I4:
        case Js::OpCode::CmNeq_I4:
        case Js::OpCode::CmLt_I4:
        case Js::OpCode::CmLe_I4:
        case Js::OpCode::CmGt_I4:
        case Js::OpCode::CmGe_I4:
        case Js::OpCode::CmUnLt_I4:
        case Js::OpCode::CmUnLe_I4:
        case Js::OpCode::CmUnGt_I4:
        case Js::OpCode::CmUnGe_I4:
            // These operations are a special case. They generate a lossy int value, and the var sym is initialized using
            // Conv_Bool. A sym cannot be live only as a lossy int sym, the var needs to be live as well since the lossy int
            // sym cannot be used to convert to var. We don't know however, whether the Conv_Bool will be hoisted. The idea
            // currently is that the sym is only used on the path in which it is initialized inside the loop. So, don't
            // hoist any liveness info for the dst.
            if (!this->GetIsAsmJSFunc())
            {
                lossy = true;
            }
            break;

        case Js::OpCode::FromVar:
        {
            StackSym* src1StackSym = IR::RegOpnd::TryGetStackSym(instr->GetSrc1());

            if (instr->HasBailOutInfo())
            {
                IR::BailOutKind instrBailoutKind = instr->GetBailOutKind();
#ifdef ENABLE_SIMDJS
                Assert(instrBailoutKind == IR::BailOutIntOnly ||
                    instrBailoutKind == IR::BailOutExpectingInteger ||
                    instrBailoutKind == IR::BailOutOnNotPrimitive ||
                    instrBailoutKind == IR::BailOutNumberOnly ||
                    instrBailoutKind == IR::BailOutPrimitiveButString ||
                    instrBailoutKind == IR::BailOutSimd128F4Only ||
                    instrBailoutKind == IR::BailOutSimd128I4Only);
#else
                Assert(instrBailoutKind == IR::BailOutIntOnly ||
                    instrBailoutKind == IR::BailOutExpectingInteger ||
                    instrBailoutKind == IR::BailOutOnNotPrimitive ||
                    instrBailoutKind == IR::BailOutNumberOnly ||
                    instrBailoutKind == IR::BailOutPrimitiveButString);
#endif
            }
            else if (src1StackSym && bailoutKind != IR::BailOutInvalid)
            {
                // We may be hoisting FromVar from a region where it didn't need a bailout (src1 had a definite value type) to a region
                // where it would. In such cases, the FromVar needs a bailout based on the value type of src1 in its new position.
                Assert(!src1StackSym->IsTypeSpec());
                Value* landingPadSrc1val = landingPad->globOptData.FindValue(src1StackSym);
                Assert(src1Val->GetValueNumber() == landingPadSrc1val->GetValueNumber());

                ValueInfo *src1ValueInfo = src1Val->GetValueInfo();
                ValueInfo *landingPadSrc1ValueInfo = landingPadSrc1val->GetValueInfo();
                IRType dstType = dst->GetType();

                const auto AddBailOutToFromVar = [&]()
                {
                    instr->GetSrc1()->SetValueType(landingPadSrc1val->GetValueInfo()->Type());
                    EnsureBailTarget(loop);
                    if (block->IsLandingPad())
                    {
                        instr = instr->ConvertToBailOutInstr(instr, bailoutKind, loop->bailOutInfo->bailOutOffset);
                    }
                    else
                    {
                        instr = instr->ConvertToBailOutInstr(instr, bailoutKind);
                    }
                };

                // A definite type in the source position and not a definite type in the destination (landing pad)
                // and no bailout on the instruction; we should put a bailout on the hoisted instruction.
                if (dstType == TyInt32)
                {
                    if (lossy)
                    {
                        if ((src1ValueInfo->IsPrimitive() || block->globOptData.IsTypeSpecialized(src1StackSym)) &&                // didn't need a lossy type spec bailout in the source block
                            (!landingPadSrc1ValueInfo->IsPrimitive() && !landingPad->globOptData.IsTypeSpecialized(src1StackSym))) // needs a lossy type spec bailout in the landing pad
                        {
                            bailoutKind = IR::BailOutOnNotPrimitive;
                            AddBailOutToFromVar();
                        }
                    }
                    else if (src1ValueInfo->IsInt() && !landingPadSrc1ValueInfo->IsInt())
                    {
                        AddBailOutToFromVar();
                    }
                }
                else if ((dstType == TyFloat64 && src1ValueInfo->IsNumber() && !landingPadSrc1ValueInfo->IsNumber()) ||
                    (IRType_IsSimd128(dstType) && src1ValueInfo->IsSimd128() && !landingPadSrc1ValueInfo->IsSimd128()))
                {
                    AddBailOutToFromVar();
                }
            }

            break;
        }
        }

        if (dstVal == NULL)
        {
            dstVal = this->NewGenericValue(ValueType::Uninitialized, dst);
        }

        // ToVar/FromVar don't need a new dst because it has to be invariant if their src is invariant.
        bool dstDoesntNeedLoad = (!isNotTypeSpecConv && instr->m_opcode != Js::OpCode::LdC_A_I4);

        StackSym *varSym = dst->m_sym;

        if (varSym->IsTypeSpec())
        {
            varSym = varSym->GetVarEquivSym(this->func);
        }

        Value *const landingPadDstVal = loop->landingPad->globOptData.FindValue(varSym);
        if(landingPadDstVal
                ? dstVal->GetValueNumber() != landingPadDstVal->GetValueNumber()
                : loop->symsDefInLoop->Test(varSym->m_id))
        {
            // We need a temp for FromVar/ToVar if dst changes in the loop.
            dstDoesntNeedLoad = false;
        }

        if (!dstDoesntNeedLoad && this->OptDstIsInvariant(dst) == false)
        {
            // Keep dst in place, hoist instr using a new dst.
            instr->UnlinkDst();

            // Set type specialization info correctly for this new sym
            StackSym *copyVarSym;
            IR::RegOpnd *copyReg;
            if (dst->m_sym->IsTypeSpec())
            {
                copyVarSym = StackSym::New(TyVar, instr->m_func);
                StackSym *copySym = copyVarSym;
                if (dst->m_sym->IsInt32())
                {
                    if(lossy)
                    {
                        // The new sym would only be live as a lossy int since we're only hoisting the store to the int version
                        // of the sym, and cannot be converted to var. It is not legal to have a sym only live as a lossy int,
                        // so don't update liveness info for this sym.
                    }
                    else
                    {
                        block->globOptData.liveInt32Syms->Set(copyVarSym->m_id);
                    }
                    copySym = copySym->GetInt32EquivSym(instr->m_func);
                }
                else if (dst->m_sym->IsFloat64())
                {
                    block->globOptData.liveFloat64Syms->Set(copyVarSym->m_id);
                    copySym = copySym->GetFloat64EquivSym(instr->m_func);
                }
#ifdef ENABLE_SIMDJS
                else if (dst->IsSimd128())
                {
                    // SIMD_JS
                    if (dst->IsSimd128F4())
                    {
                        block->globOptData.liveSimd128F4Syms->Set(copyVarSym->m_id);
                        copySym = copySym->GetSimd128F4EquivSym(instr->m_func);
                    }
                    else
                    {
                        Assert(dst->IsSimd128I4());
                        block->globOptData.liveSimd128I4Syms->Set(copyVarSym->m_id);
                        copySym = copySym->GetSimd128I4EquivSym(instr->m_func);
                    }
                }
#endif
                copyReg = IR::RegOpnd::New(copySym, copySym->GetType(), instr->m_func);
            }
            else
            {
                copyReg = IR::RegOpnd::New(dst->GetType(), instr->m_func);
                copyVarSym = copyReg->m_sym;
                block->globOptData.liveVarSyms->Set(copyVarSym->m_id);
            }

            copyReg->SetValueType(dst->GetValueType());
            IR::Instr *copyInstr = IR::Instr::New(Js::OpCode::Ld_A, dst, copyReg, instr->m_func);
            copyInstr->SetByteCodeOffset(instr);
            instr->SetDst(copyReg);
            instr->InsertBefore(copyInstr);

            dst->m_sym->m_mayNotBeTempLastUse = true;

            if (instr->GetSrc1() && instr->GetSrc1()->IsImmediateOpnd())
            {
                // Propagate IsIntConst if appropriate
                switch(instr->m_opcode)
                {
                case Js::OpCode::Ld_A:
                case Js::OpCode::Ld_I4:
                case Js::OpCode::LdC_A_I4:
                    copyReg->m_sym->SetIsConst();
                    break;
                }
            }

            ValueInfo *dstValueInfo = dstVal->GetValueInfo();
            if((!dstValueInfo->GetSymStore() || dstValueInfo->GetSymStore() == varSym) && !lossy)
            {
                // The destination's value may have been transferred from one of the invariant sources, in which case we should
                // keep the sym store intact, as that sym will likely have a better lifetime than this new copy sym. For
                // instance, if we're inside a conditioned block, because we don't make the copy sym live and set its value in
                // all preceding blocks, this sym would not be live after exiting this block, causing this value to not
                // participate in copy-prop after this block.
                this->SetSymStoreDirect(dstValueInfo, copyVarSym);
            }

            block->globOptData.InsertNewValue(dstVal, copyReg);
            dst = copyReg;
        }
    }

    // Move to landing pad
    block->UnlinkInstr(instr);

    if (loop->bailOutInfo->bailOutInstr)
    {
        loop->bailOutInfo->bailOutInstr->InsertBefore(instr);
    }
    else
    {
        landingPad->InsertAfter(instr);
    }

    GlobOpt::MarkNonByteCodeUsed(instr);

    if (instr->HasBailOutInfo() || instr->HasAuxBailOut())
    {
        Assert(loop->bailOutInfo);
        EnsureBailTarget(loop);

        // Copy bailout info of loop top.
        if (instr->ReplaceBailOutInfo(loop->bailOutInfo))
        {
            // if the old bailout is deleted, reset capturedvalues cached in block
            block->globOptData.capturedValues = nullptr;
            block->globOptData.capturedValuesCandidate = nullptr;
        }
    }

    if(!dst)
    {
        return;
    }

    // The bailout info's liveness for the dst sym is not updated in loop landing pads because bailout instructions previously
    // hoisted into the loop's landing pad may bail out before the current type of the dst sym became live (perhaps due to this
    // instruction). Since the landing pad will have a shared bailout point, the bailout info cannot assume that the current
    // type of the dst sym was live during every bailout hoisted into the landing pad.

    StackSym *const dstSym = dst->m_sym;
    StackSym *const dstVarSym = dstSym->IsTypeSpec() ? dstSym->GetVarEquivSym(nullptr) : dstSym;
    Assert(dstVarSym);
    if(isNotTypeSpecConv || !loop->landingPad->globOptData.IsLive(dstVarSym))
    {
        // A new dst is being hoisted, or the same single-def dst that would not be live before this block. So, make it live and
        // update the value info with the same value info in this block.

        if(lossy)
        {
            // This is a lossy conversion to int. The instruction was given a new dst specifically for hoisting, so this new dst
            // will not be live as a var before this block. A sym cannot be live only as a lossy int sym, the var needs to be
            // live as well since the lossy int sym cannot be used to convert to var. Since the var version of the sym is not
            // going to be initialized, don't hoist any liveness info for the dst. The sym is only going to be used on the path
            // in which it is initialized inside the loop.
            Assert(dstSym->IsTypeSpec());
            Assert(dstSym->IsInt32());
            return;
        }

        // Check if the dst value was transferred from the src. If so, the value transfer needs to be replicated.
        bool isTransfer = dstVal == src1Val;

        StackSym *transferValueOfSym = nullptr;
        if(isTransfer)
        {
            Assert(instr->GetSrc1());
            if(instr->GetSrc1()->IsRegOpnd())
            {
                StackSym *src1Sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
                if(src1Sym->IsTypeSpec())
                {
                    src1Sym = src1Sym->GetVarEquivSym(nullptr);
                    Assert(src1Sym);
                }
                if(dstVal == block->globOptData.FindValue(src1Sym))
                {
                    transferValueOfSym = src1Sym;
                }
            }
        }

        // SIMD_JS
        if (instr->m_opcode == Js::OpCode::ExtendArg_A)
        {
            // Check if we should have CSE'ed this EA
            Assert(instr->GetSrc1());

            // If the dstVal symstore is not the dst itself, then we copied the Value from another expression.
            if (dstVal->GetValueInfo()->GetSymStore() != instr->GetDst()->GetStackSym())
            {
                isTransfer = true;
                transferValueOfSym = dstVal->GetValueInfo()->GetSymStore()->AsStackSym();
            }
        }

        const ValueNumber dstValueNumber = dstVal->GetValueNumber();
        ValueNumber dstNewValueNumber = InvalidValueNumber;
        for(InvariantBlockBackwardIterator it(this, block, loop->landingPad, nullptr); it.IsValid(); it.MoveNext())
        {
            BasicBlock *const hoistBlock = it.Block();
            GlobOptBlockData &hoistBlockData = hoistBlock->globOptData;

            Assert(!hoistBlockData.IsLive(dstVarSym));
            hoistBlockData.MakeLive(dstSym, lossy);

            Value *newDstValue;
            do
            {
                if(isTransfer)
                {
                    if(transferValueOfSym)
                    {
                        newDstValue = hoistBlockData.FindValue(transferValueOfSym);
                        if(newDstValue && newDstValue->GetValueNumber() == dstValueNumber)
                        {
                            break;
                        }
                    }

                    // It's a transfer, but we don't have a sym whose value number matches in the target block. Use a new value
                    // number since we don't know if there is already a value with the current number for the target block.
                    if(dstNewValueNumber == InvalidValueNumber)
                    {
                        dstNewValueNumber = NewValueNumber();
                    }
                    newDstValue = CopyValue(dstVal, dstNewValueNumber);
                    break;
                }

                newDstValue = CopyValue(dstVal, dstValueNumber);
            } while(false);

            hoistBlockData.SetValue(newDstValue, dstVarSym);
        }
        return;
    }

#if DBG
    if(instr->GetSrc1()->IsRegOpnd()) // Type spec conversion may load a constant into a dst sym
    {
        StackSym *const srcSym = instr->GetSrc1()->AsRegOpnd()->m_sym;
        Assert(srcSym != dstSym); // Type spec conversion must be changing the type, so the syms must be different
        StackSym *const srcVarSym = srcSym->IsTypeSpec() ? srcSym->GetVarEquivSym(nullptr) : srcSym;
        Assert(srcVarSym == dstVarSym); // Type spec conversion must be between variants of the same var sym
    }
#endif

    bool changeValueType = false, changeValueTypeToInt = false;
    if(dstSym->IsTypeSpec())
    {
        if(dst->IsInt32())
        {
            if(!lossy)
            {
                Assert(
                    !instr->HasBailOutInfo() ||
                    instr->GetBailOutKind() == IR::BailOutIntOnly ||
                    instr->GetBailOutKind() == IR::BailOutExpectingInteger);
                changeValueType = changeValueTypeToInt = true;
            }
        }
        else if (dst->IsFloat64())
        {
            if(instr->HasBailOutInfo() && instr->GetBailOutKind() == IR::BailOutNumberOnly)
            {
                changeValueType = true;
            }
        }
#ifdef ENABLE_SIMDJS
        else
        {
            // SIMD_JS
            Assert(dst->IsSimd128());
            if (instr->HasBailOutInfo() &&
                (instr->GetBailOutKind() == IR::BailOutSimd128F4Only || instr->GetBailOutKind() == IR::BailOutSimd128I4Only))
            {
                changeValueType = true;
            }
        }
#endif
    }

    ValueInfo *previousValueInfoBeforeUpdate = nullptr, *previousValueInfoAfterUpdate = nullptr;
    for(InvariantBlockBackwardIterator it(
            this,
            block,
            loop->landingPad,
            dstVarSym,
            dstVal->GetValueNumber());
        it.IsValid();
        it.MoveNext())
    {
        BasicBlock *const hoistBlock = it.Block();
        GlobOptBlockData &hoistBlockData = hoistBlock->globOptData;

    #if DBG
        // TODO: There are some odd cases with field hoisting where the sym is invariant in only part of the loop and the info
        // does not flow through all blocks. Un-comment the verification below after PRE replaces field hoisting.

        //// Verify that the src sym is live as the required type, and that the conversion is valid
        //Assert(IsLive(dstVarSym, &hoistBlockData));
        //if(instr->GetSrc1()->IsRegOpnd())
        //{
        //    IR::RegOpnd *const src = instr->GetSrc1()->AsRegOpnd();
        //    StackSym *const srcSym = instr->GetSrc1()->AsRegOpnd()->m_sym;
        //    if(srcSym->IsTypeSpec())
        //    {
        //        if(src->IsInt32())
        //        {
        //            Assert(hoistBlockData.liveInt32Syms->Test(dstVarSym->m_id));
        //            Assert(!hoistBlockData.liveLossyInt32Syms->Test(dstVarSym->m_id)); // shouldn't try to convert a lossy int32 to anything
        //        }
        //        else
        //        {
        //            Assert(src->IsFloat64());
        //            Assert(hoistBlockData.liveFloat64Syms->Test(dstVarSym->m_id));
        //            if(dstSym->IsTypeSpec() && dst->IsInt32())
        //            {
        //                Assert(lossy); // shouldn't try to do a lossless conversion from float64 to int32
        //            }
        //        }
        //    }
        //    else
        //    {
        //        Assert(hoistBlockData.liveVarSyms->Test(dstVarSym->m_id));
        //    }
        //}
        //if(dstSym->IsTypeSpec() && dst->IsInt32())
        //{
        //    // If the sym is already specialized as required in the block to which we are attempting to hoist the conversion,
        //    // that info should have flowed into this block
        //    if(lossy)
        //    {
        //        Assert(!hoistBlockData.liveInt32Syms->Test(dstVarSym->m_id));
        //    }
        //    else
        //    {
        //        Assert(!IsInt32TypeSpecialized(dstVarSym, hoistBlock));
        //    }
        //}
    #endif

        hoistBlockData.MakeLive(dstSym, lossy);

        if(!changeValueType)
        {
            continue;
        }

        Value *const hoistBlockValue = it.InvariantSymValue();
        ValueInfo *const hoistBlockValueInfo = hoistBlockValue->GetValueInfo();
        if(hoistBlockValueInfo == previousValueInfoBeforeUpdate)
        {
            if(hoistBlockValueInfo != previousValueInfoAfterUpdate)
            {
                HoistInvariantValueInfo(previousValueInfoAfterUpdate, hoistBlockValue, hoistBlock);
            }
        }
        else
        {
            previousValueInfoBeforeUpdate = hoistBlockValueInfo;
            ValueInfo *const newValueInfo =
                changeValueTypeToInt
                    ? hoistBlockValueInfo->SpecializeToInt32(alloc)
                    : hoistBlockValueInfo->SpecializeToFloat64(alloc);
            previousValueInfoAfterUpdate = newValueInfo;
            ChangeValueInfo(changeValueTypeToInt ? nullptr : hoistBlock, hoistBlockValue, newValueInfo);
        }
    }
}

bool
GlobOpt::TryHoistInvariant(
    IR::Instr *instr,
    BasicBlock *block,
    Value *dstVal,
    Value *src1Val,
    Value *src2Val,
    bool isNotTypeSpecConv,
    const bool lossy,
    const bool forceInvariantHoisting,
    IR::BailOutKind bailoutKind)
{
    Assert(!this->IsLoopPrePass());

    if (OptIsInvariant(instr, block, block->loop, src1Val, src2Val, isNotTypeSpecConv, forceInvariantHoisting))
    {
#if DBG
        if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::InvariantsPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId()))
        {
            Output::Print(_u(" **** INVARIANT  ***   "));
            instr->Dump();
        }
#endif
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::InvariantsPhase))
        {
            Output::Print(_u(" **** INVARIANT  ***   "));
            Output::Print(_u("%s \n"), Js::OpCodeUtil::GetOpCodeName(instr->m_opcode));
        }
#endif
        Loop *loop = block->loop;

        // Try hoisting from to outer most loop
        while (loop->parent && OptIsInvariant(instr, block, loop->parent, src1Val, src2Val, isNotTypeSpecConv, forceInvariantHoisting))
        {
            loop = loop->parent;
        }

        // Record the byte code use here since we are going to move this instruction up
        if (isNotTypeSpecConv)
        {
            InsertNoImplicitCallUses(instr);
            this->CaptureByteCodeSymUses(instr);
            this->InsertByteCodeUses(instr, true);
        }
#if DBG
        else
        {
            PropertySym *propertySymUse = NULL;
            NoRecoverMemoryJitArenaAllocator tempAllocator(_u("BE-GlobOpt-Temp"), this->alloc->GetPageAllocator(), Js::Throw::OutOfMemory);
            BVSparse<JitArenaAllocator> * tempByteCodeUse = JitAnew(&tempAllocator, BVSparse<JitArenaAllocator>, &tempAllocator);
            GlobOpt::TrackByteCodeSymUsed(instr, tempByteCodeUse, &propertySymUse);
            Assert(tempByteCodeUse->Count() == 0 && propertySymUse == NULL);
        }
#endif
        OptHoistInvariant(instr, block, loop, dstVal, src1Val, src2Val, isNotTypeSpecConv, lossy, bailoutKind);
        return true;
    }

    return false;
}

InvariantBlockBackwardIterator::InvariantBlockBackwardIterator(
    GlobOpt *const globOpt,
    BasicBlock *const exclusiveBeginBlock,
    BasicBlock *const inclusiveEndBlock,
    StackSym *const invariantSym,
    const ValueNumber invariantSymValueNumber)
    : globOpt(globOpt),
    exclusiveEndBlock(inclusiveEndBlock->prev),
    invariantSym(invariantSym),
    invariantSymValueNumber(invariantSymValueNumber),
    block(exclusiveBeginBlock)
#if DBG
    ,
    inclusiveEndBlock(inclusiveEndBlock)
#endif
{
    Assert(exclusiveBeginBlock);
    Assert(inclusiveEndBlock);
    Assert(!inclusiveEndBlock->isDeleted);
    Assert(exclusiveBeginBlock != inclusiveEndBlock);
    Assert(!invariantSym == (invariantSymValueNumber == InvalidValueNumber));

    MoveNext();
}

bool
InvariantBlockBackwardIterator::IsValid() const
{
    return block != exclusiveEndBlock;
}

void
InvariantBlockBackwardIterator::MoveNext()
{
    Assert(IsValid());

    while(true)
    {
    #if DBG
        BasicBlock *const previouslyIteratedBlock = block;
    #endif
        block = block->prev;
        if(!IsValid())
        {
            Assert(previouslyIteratedBlock == inclusiveEndBlock);
            break;
        }

        if(block->isDeleted)
        {
            continue;
        }

        if(!block->globOptData.HasData())
        {
            // This block's info has already been merged with all of its successors
            continue;
        }

        if(!invariantSym)
        {
            break;
        }

        invariantSymValue = block->globOptData.FindValue(invariantSym);
        if(!invariantSymValue || invariantSymValue->GetValueNumber() != invariantSymValueNumber)
        {
            // BailOnNoProfile and throw blocks are not moved outside loops. A sym table cleanup on these paths may delete the
            // values. Field hoisting also has some odd cases where the hoisted stack sym is invariant in only part of the loop.
            continue;
        }
        break;
    }
}

BasicBlock *
InvariantBlockBackwardIterator::Block() const
{
    Assert(IsValid());
    return block;
}

Value *
InvariantBlockBackwardIterator::InvariantSymValue() const
{
    Assert(IsValid());
    Assert(invariantSym);

    return invariantSymValue;
}

void
GlobOpt::HoistInvariantValueInfo(
    ValueInfo *const invariantValueInfoToHoist,
    Value *const valueToUpdate,
    BasicBlock *const targetBlock)
{
    Assert(invariantValueInfoToHoist);
    Assert(valueToUpdate);
    Assert(targetBlock);

    // Why are we trying to change the value type of the type sym value? Asserting here to make sure we don't deep copy the type sym's value info.
    Assert(!invariantValueInfoToHoist->IsJsType());

    Sym *const symStore = valueToUpdate->GetValueInfo()->GetSymStore();
    ValueInfo *newValueInfo;
    if(invariantValueInfoToHoist->GetSymStore() == symStore)
    {
        newValueInfo = invariantValueInfoToHoist;
    }
    else
    {
        newValueInfo = invariantValueInfoToHoist->Copy(alloc);
        this->SetSymStoreDirect(newValueInfo, symStore);
    }
    ChangeValueInfo(targetBlock, valueToUpdate, newValueInfo);
}

// static
bool
GlobOpt::DoInlineArgsOpt(Func const * func)
{
    Func const * topFunc = func->GetTopFunc();
    Assert(topFunc != func);
    bool doInlineArgsOpt =
        !PHASE_OFF(Js::InlineArgsOptPhase, topFunc) &&
        !func->GetHasCalls() &&
        !func->GetHasUnoptimizedArgumentsAccess() &&
        func->m_canDoInlineArgsOpt;
    return doInlineArgsOpt;
}

bool
GlobOpt::IsSwitchOptEnabled(Func const * func)
{
    Assert(func->IsTopFunc());
    return !PHASE_OFF(Js::SwitchOptPhase, func) && !func->IsSwitchOptDisabled() && !IsTypeSpecPhaseOff(func)
        && func->DoGlobOpt() && !func->HasTry();
}

bool
GlobOpt::DoConstFold() const
{
    return !PHASE_OFF(Js::ConstFoldPhase, func);
}

bool
GlobOpt::IsTypeSpecPhaseOff(Func const *func)
{
    return PHASE_OFF(Js::TypeSpecPhase, func) || func->IsJitInDebugMode() || !func->DoGlobOptsForGeneratorFunc();
}

bool
GlobOpt::DoTypeSpec() const
{
    return doTypeSpec;
}

bool
GlobOpt::DoAggressiveIntTypeSpec(Func const * func)
{
    return
        !PHASE_OFF(Js::AggressiveIntTypeSpecPhase, func) &&
        !IsTypeSpecPhaseOff(func) &&
        !func->IsAggressiveIntTypeSpecDisabled();
}

bool
GlobOpt::DoAggressiveIntTypeSpec() const
{
    return doAggressiveIntTypeSpec;
}

bool
GlobOpt::DoAggressiveMulIntTypeSpec() const
{
    return doAggressiveMulIntTypeSpec;
}

bool
GlobOpt::DoDivIntTypeSpec() const
{
    return doDivIntTypeSpec;
}

// static
bool
GlobOpt::DoLossyIntTypeSpec(Func const * func)
{
    return
        !PHASE_OFF(Js::LossyIntTypeSpecPhase, func) &&
        !IsTypeSpecPhaseOff(func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsLossyIntTypeSpecDisabled());
}

bool
GlobOpt::DoLossyIntTypeSpec() const
{
    return doLossyIntTypeSpec;
}

// static
bool
GlobOpt::DoFloatTypeSpec(Func const * func)
{
    return
        !PHASE_OFF(Js::FloatTypeSpecPhase, func) &&
        !IsTypeSpecPhaseOff(func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsFloatTypeSpecDisabled()) &&
        AutoSystemInfo::Data.SSE2Available();
}

bool
GlobOpt::DoFloatTypeSpec() const
{
    return doFloatTypeSpec;
}

bool
GlobOpt::DoStringTypeSpec(Func const * func)
{
    return !PHASE_OFF(Js::StringTypeSpecPhase, func) && !IsTypeSpecPhaseOff(func);
}

// static
bool
GlobOpt::DoTypedArrayTypeSpec(Func const * func)
{
    return !PHASE_OFF(Js::TypedArrayTypeSpecPhase, func) &&
        !IsTypeSpecPhaseOff(func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsTypedArrayTypeSpecDisabled(func->IsLoopBody()))
#if defined(_M_IX86)
        && AutoSystemInfo::Data.SSE2Available()
#endif
        ;
}

// static
bool
GlobOpt::DoNativeArrayTypeSpec(Func const * func)
{
    return !PHASE_OFF(Js::NativeArrayPhase, func) &&
        !IsTypeSpecPhaseOff(func)
#if defined(_M_IX86)
        && AutoSystemInfo::Data.SSE2Available()
#endif
        ;
}

bool
GlobOpt::DoArrayCheckHoist(Func const * const func)
{
    Assert(func->IsTopFunc());
    return
        !PHASE_OFF(Js::ArrayCheckHoistPhase, func) &&
        !func->IsArrayCheckHoistDisabled() &&
        !func->IsJitInDebugMode() && // StElemI fast path is not allowed when in debug mode, so it cannot have bailout
        func->DoGlobOptsForGeneratorFunc();
}

bool
GlobOpt::DoArrayCheckHoist() const
{
    return doArrayCheckHoist;
}

bool
GlobOpt::DoArrayCheckHoist(const ValueType baseValueType, Loop* loop, IR::Instr const * const instr) const
{
    if(!DoArrayCheckHoist() || (instr && !IsLoopPrePass() && instr->DoStackArgsOpt(func)))
    {
        return false;
    }

    if(!baseValueType.IsLikelyArrayOrObjectWithArray() ||
        (loop ? ImplicitCallFlagsAllowOpts(loop) : ImplicitCallFlagsAllowOpts(func)))
    {
        return true;
    }

    // The function or loop does not allow disabling implicit calls, which is required to eliminate redundant JS array checks
#if DBG_DUMP
    if((((loop ? loop->GetImplicitCallFlags() : func->m_fg->implicitCallFlags) & ~Js::ImplicitCall_External) == 0) &&
        Js::Configuration::Global.flags.Trace.IsEnabled(Js::HostOptPhase))
    {
        Output::Print(_u("DoArrayCheckHoist disabled for JS arrays because of external: "));
        func->DumpFullFunctionName();
        Output::Print(_u("\n"));
        Output::Flush();
    }
#endif
    return false;
}

bool
GlobOpt::DoArrayMissingValueCheckHoist(Func const * const func)
{
    return
        DoArrayCheckHoist(func) &&
        !PHASE_OFF(Js::ArrayMissingValueCheckHoistPhase, func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsArrayMissingValueCheckHoistDisabled(func->IsLoopBody()));
}

bool
GlobOpt::DoArrayMissingValueCheckHoist() const
{
    return doArrayMissingValueCheckHoist;
}

bool
GlobOpt::DoArraySegmentHoist(const ValueType baseValueType, Func const * const func)
{
    Assert(baseValueType.IsLikelyAnyOptimizedArray());

    if(!DoArrayCheckHoist(func) || PHASE_OFF(Js::ArraySegmentHoistPhase, func))
    {
        return false;
    }

    if(!baseValueType.IsLikelyArrayOrObjectWithArray())
    {
        return true;
    }

    return
        !PHASE_OFF(Js::JsArraySegmentHoistPhase, func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsJsArraySegmentHoistDisabled(func->IsLoopBody()));
}

bool
GlobOpt::DoArraySegmentHoist(const ValueType baseValueType) const
{
    Assert(baseValueType.IsLikelyAnyOptimizedArray());
    return baseValueType.IsLikelyArrayOrObjectWithArray() ? doJsArraySegmentHoist : doArraySegmentHoist;
}

bool
GlobOpt::DoTypedArraySegmentLengthHoist(Loop *const loop) const
{
    if(!DoArraySegmentHoist(ValueType::GetObject(ObjectType::Int32Array)))
    {
        return false;
    }

    if(loop ? ImplicitCallFlagsAllowOpts(loop) : ImplicitCallFlagsAllowOpts(func))
    {
        return true;
    }

    // The function or loop does not allow disabling implicit calls, which is required to eliminate redundant typed array
    // segment length loads.
#if DBG_DUMP
    if((((loop ? loop->GetImplicitCallFlags() : func->m_fg->implicitCallFlags) & ~Js::ImplicitCall_External) == 0) &&
        Js::Configuration::Global.flags.Trace.IsEnabled(Js::HostOptPhase))
    {
        Output::Print(_u("DoArraySegmentLengthHoist disabled for typed arrays because of external: "));
        func->DumpFullFunctionName();
        Output::Print(_u("\n"));
        Output::Flush();
    }
#endif
    return false;
}

bool
GlobOpt::DoArrayLengthHoist(Func const * const func)
{
    return
        DoArrayCheckHoist(func) &&
        !PHASE_OFF(Js::Phase::ArrayLengthHoistPhase, func) &&
        (!func->HasProfileInfo() || !func->GetReadOnlyProfileInfo()->IsArrayLengthHoistDisabled(func->IsLoopBody()));
}

bool
GlobOpt::DoArrayLengthHoist() const
{
    return doArrayLengthHoist;
}

bool
GlobOpt::DoEliminateArrayAccessHelperCall(Func *const func)
{
    return DoArrayCheckHoist(func);
}

bool
GlobOpt::DoEliminateArrayAccessHelperCall() const
{
    return doEliminateArrayAccessHelperCall;
}

bool
GlobOpt::DoLdLenIntSpec(IR::Instr * const instr, const ValueType baseValueType)
{
    Assert(!instr || instr->m_opcode == Js::OpCode::LdLen_A);
    Assert(!instr || instr->GetDst());
    Assert(!instr || instr->GetSrc1());

    if(PHASE_OFF(Js::LdLenIntSpecPhase, func) ||
        IsTypeSpecPhaseOff(func) ||
        (func->HasProfileInfo() && func->GetReadOnlyProfileInfo()->IsLdLenIntSpecDisabled()) ||
        (instr && !IsLoopPrePass() && instr->DoStackArgsOpt(func)))
    {
        return false;
    }

    if(instr &&
        instr->IsProfiledInstr() &&
        (
            !instr->AsProfiledInstr()->u.ldElemInfo->GetElementType().IsLikelyInt() ||
            instr->GetDst()->AsRegOpnd()->m_sym->m_isNotInt
        ))
    {
        return false;
    }

    Assert(!instr || baseValueType == instr->GetSrc1()->GetValueType());
    return
        baseValueType.HasBeenString() ||
        (baseValueType.IsLikelyAnyOptimizedArray() && baseValueType.GetObjectType() != ObjectType::ObjectWithArray);
}

bool
GlobOpt::DoPathDependentValues() const
{
    return !PHASE_OFF(Js::Phase::PathDependentValuesPhase, func);
}

bool
GlobOpt::DoTrackRelativeIntBounds() const
{
    return doTrackRelativeIntBounds;
}

bool
GlobOpt::DoBoundCheckElimination() const
{
    return doBoundCheckElimination;
}

bool
GlobOpt::DoBoundCheckHoist() const
{
    return doBoundCheckHoist;
}

bool
GlobOpt::DoLoopCountBasedBoundCheckHoist() const
{
    return doLoopCountBasedBoundCheckHoist;
}

bool
GlobOpt::DoPowIntIntTypeSpec() const
{
    return doPowIntIntTypeSpec;
}

bool
GlobOpt::DoTagChecks() const
{
    return doTagChecks;
}

bool
GlobOpt::TrackArgumentsObject()
{
    if (PHASE_OFF(Js::StackArgOptPhase, this->func))
    {
        this->CannotAllocateArgumentsObjectOnStack();
        return false;
    }

    return func->GetHasStackArgs();
}

void
GlobOpt::CannotAllocateArgumentsObjectOnStack()
{
    func->SetHasStackArgs(false);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (PHASE_TESTTRACE(Js::StackArgOptPhase, this->func))
    {
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        Output::Print(_u("Stack args disabled for function %s(%s)\n"), func->GetJITFunctionBody()->GetDisplayName(), func->GetDebugNumberSet(debugStringBuffer));
        Output::Flush();
    }
#endif
}

IR::Instr *
GlobOpt::PreOptPeep(IR::Instr *instr)
{
    if (OpCodeAttr::HasDeadFallThrough(instr->m_opcode))
    {
        switch (instr->m_opcode)
        {
            case Js::OpCode::BailOnNoProfile:
            {
                // Handle BailOnNoProfile
                if (instr->HasBailOutInfo())
                {
                    if (!this->prePassLoop)
                    {
                        FillBailOutInfo(this->currentBlock, instr->GetBailOutInfo());
                    }
                    // Already processed.
                    return instr;
                }

                // Convert to bailout instr
                IR::Instr *nextBytecodeOffsetInstr = instr->GetNextRealInstrOrLabel();
                while(nextBytecodeOffsetInstr->GetByteCodeOffset() == Js::Constants::NoByteCodeOffset)
                {
                    nextBytecodeOffsetInstr = nextBytecodeOffsetInstr->GetNextRealInstrOrLabel();
                    Assert(!nextBytecodeOffsetInstr->IsLabelInstr());
                }
                instr = instr->ConvertToBailOutInstr(nextBytecodeOffsetInstr, IR::BailOutOnNoProfile);
                instr->ClearByteCodeOffset();
                instr->SetByteCodeOffset(nextBytecodeOffsetInstr);

                if (!this->currentBlock->loop)
                {
                    FillBailOutInfo(this->currentBlock, instr->GetBailOutInfo());
                }
                else
                {
                    Assert(this->prePassLoop);
                }
                break;
            }
            case Js::OpCode::BailOnException:
            {
                Assert(
                    (
                        this->func->HasTry() && this->func->DoOptimizeTry() &&
                        instr->m_prev->m_opcode == Js::OpCode::Catch &&
                        instr->m_prev->m_prev->IsLabelInstr() &&
                        instr->m_prev->m_prev->AsLabelInstr()->GetRegion()->GetType() == RegionType::RegionTypeCatch
                    )
                    ||
                    (
                        this->func->HasFinally() && this->func->DoOptimizeTry() &&
                        instr->m_prev->AsLabelInstr() &&
                        instr->m_prev->AsLabelInstr()->GetRegion()->GetType() == RegionType::RegionTypeFinally
                    )
                );
                break;
            }
            case Js::OpCode::BailOnEarlyExit:
            {
                Assert(this->func->HasFinally() && this->func->DoOptimizeTry());
                break;
            }
            default:
            {
                if(this->currentBlock->loop && !this->IsLoopPrePass())
                {
                    return instr;
                }
                break;
            }
        }
        RemoveCodeAfterNoFallthroughInstr(instr);
    }

    return instr;
}

void
GlobOpt::RemoveCodeAfterNoFallthroughInstr(IR::Instr *instr)
{
    if (instr != this->currentBlock->GetLastInstr())
    {
        // Remove dead code after bailout
        IR::Instr *instrDead = instr->m_next;
        IR::Instr *instrNext;

        for (; instrDead != this->currentBlock->GetLastInstr(); instrDead = instrNext)
        {
            instrNext = instrDead->m_next;
            if (instrNext->m_opcode == Js::OpCode::FunctionExit)
            {
                break;
            }
            this->func->m_fg->RemoveInstr(instrDead, this);
        }
        IR::Instr *instrNextBlock = instrDead->m_next;
        this->func->m_fg->RemoveInstr(instrDead, this);

        this->currentBlock->SetLastInstr(instrNextBlock->m_prev);
    }

    // Cleanup dead successors
    FOREACH_SUCCESSOR_BLOCK_EDITING(deadBlock, this->currentBlock, iter)
    {
        this->currentBlock->RemoveDeadSucc(deadBlock, this->func->m_fg);
        if (this->currentBlock->GetDataUseCount() > 0)
        {
            this->currentBlock->DecrementDataUseCount();
        }
    } NEXT_SUCCESSOR_BLOCK_EDITING;
}

void
GlobOpt::ProcessTryHandler(IR::Instr* instr)
{
    Assert(instr->m_next->IsLabelInstr() && instr->m_next->AsLabelInstr()->GetRegion()->GetType() == RegionType::RegionTypeTry);

    Region* tryRegion = instr->m_next->AsLabelInstr()->GetRegion();
    BVSparse<JitArenaAllocator> * writeThroughSymbolsSet = tryRegion->writeThroughSymbolsSet;

    ToVar(writeThroughSymbolsSet, this->currentBlock);
}

bool
GlobOpt::ProcessExceptionHandlingEdges(IR::Instr* instr)
{
    Assert(instr->m_opcode == Js::OpCode::BrOnException || instr->m_opcode == Js::OpCode::BrOnNoException);

    if (instr->m_opcode == Js::OpCode::BrOnException)
    {
        if (instr->AsBranchInstr()->GetTarget()->GetRegion()->GetType() == RegionType::RegionTypeCatch)
        {
            // BrOnException was added to model flow from try region to the catch region to assist
            // the backward pass in propagating bytecode upward exposed info from the catch block
            // to the try, and to handle break blocks. Removing it here as it has served its purpose
            // and keeping it around might also have unintended effects while merging block data for
            // the catch block's predecessors.
            // Note that the Deadstore pass will still be able to propagate bytecode upward exposed info
            // because it doesn't skip dead blocks for that.
            this->RemoveFlowEdgeToCatchBlock(instr);
            this->currentBlock->RemoveInstr(instr);
            return true;
        }
        else
        {
            // We add BrOnException from a finally region to early exit, remove that since it has served its purpose
            return this->RemoveFlowEdgeToFinallyOnExceptionBlock(instr);
        }
    }
    else if (instr->m_opcode == Js::OpCode::BrOnNoException)
    {
        if (instr->AsBranchInstr()->GetTarget()->GetRegion()->GetType() == RegionType::RegionTypeCatch)
        {
            this->RemoveFlowEdgeToCatchBlock(instr);
        }
        else
        {
            this->RemoveFlowEdgeToFinallyOnExceptionBlock(instr);
        }
    }
    return false;
}

void
GlobOpt::InsertToVarAtDefInTryRegion(IR::Instr * instr, IR::Opnd * dstOpnd)
{
    if (this->currentRegion->GetType() == RegionTypeTry && dstOpnd->IsRegOpnd() && dstOpnd->AsRegOpnd()->m_sym->HasByteCodeRegSlot())
    {
        StackSym * sym = dstOpnd->AsRegOpnd()->m_sym;
        if (sym->IsVar())
        {
            return;
        }

        StackSym * varSym = sym->GetVarEquivSym(nullptr);
        if (this->currentRegion->writeThroughSymbolsSet->Test(varSym->m_id))
        {
            IR::RegOpnd * regOpnd = IR::RegOpnd::New(varSym, IRType::TyVar, instr->m_func);
            this->ToVar(instr->m_next, regOpnd, this->currentBlock, NULL, false);
        }
    }
}

void
GlobOpt::RemoveFlowEdgeToCatchBlock(IR::Instr * instr)
{
    Assert(instr->IsBranchInstr());

    BasicBlock * catchBlock = nullptr;
    BasicBlock * predBlock = nullptr;
    if (instr->m_opcode == Js::OpCode::BrOnException)
    {
        catchBlock = instr->AsBranchInstr()->GetTarget()->GetBasicBlock();
        predBlock = this->currentBlock;
    }
    else
    {
        Assert(instr->m_opcode == Js::OpCode::BrOnNoException);
        IR::Instr * nextInstr = instr->GetNextRealInstrOrLabel();
        Assert(nextInstr->IsLabelInstr());
        IR::LabelInstr * nextLabel = nextInstr->AsLabelInstr();

        if (nextLabel->GetRegion() && nextLabel->GetRegion()->GetType() == RegionTypeCatch)
        {
            catchBlock = nextLabel->GetBasicBlock();
            predBlock = this->currentBlock;
        }
        else
        {
            Assert(nextLabel->m_next->IsBranchInstr() && nextLabel->m_next->AsBranchInstr()->IsUnconditional());
            BasicBlock * nextBlock = nextLabel->GetBasicBlock();
            IR::BranchInstr * branchToCatchBlock = nextLabel->m_next->AsBranchInstr();
            IR::LabelInstr * catchBlockLabel = branchToCatchBlock->GetTarget();
            Assert(catchBlockLabel->GetRegion()->GetType() == RegionTypeCatch);
            catchBlock = catchBlockLabel->GetBasicBlock();
            predBlock = nextBlock;
        }
    }

    Assert(catchBlock);
    Assert(predBlock);
    if (this->func->m_fg->FindEdge(predBlock, catchBlock))
    {
        predBlock->RemoveDeadSucc(catchBlock, this->func->m_fg);
        if (predBlock == this->currentBlock)
        {
            predBlock->DecrementDataUseCount();
        }
    }
}

bool
GlobOpt::RemoveFlowEdgeToFinallyOnExceptionBlock(IR::Instr * instr)
{
    Assert(instr->IsBranchInstr());

    if (instr->m_opcode == Js::OpCode::BrOnNoException && instr->AsBranchInstr()->m_brFinallyToEarlyExit)
    {
        // We add edge from finally to early exit block
        // We should not remove this edge
        // If a loop has continue, and we add edge in finally to continue
        // Break block removal can move all continues inside the loop to branch to the continue added within finally
        // If we get rid of this edge, then loop may loose all backedges
        // Ideally, doing tail duplication before globopt would enable us to remove these edges, but since we do it after globopt, keep it this way for now
        // See test1() in core/test/tryfinallytests.js
        return false;
    }

    BasicBlock * finallyBlock = nullptr;
    BasicBlock * predBlock = nullptr;
    if (instr->m_opcode == Js::OpCode::BrOnException)
    {
        finallyBlock = instr->AsBranchInstr()->GetTarget()->GetBasicBlock();
        predBlock = this->currentBlock;
    }
    else
    {
        Assert(instr->m_opcode == Js::OpCode::BrOnNoException);
        IR::Instr * nextInstr = instr->GetNextRealInstrOrLabel();
        Assert(nextInstr->IsLabelInstr());
        IR::LabelInstr * nextLabel = nextInstr->AsLabelInstr();

        if (nextLabel->GetRegion() && nextLabel->GetRegion()->GetType() == RegionTypeFinally)
        {
            finallyBlock = nextLabel->GetBasicBlock();
            predBlock = this->currentBlock;
        }
        else
        {
            if (!(nextLabel->m_next->IsBranchInstr() && nextLabel->m_next->AsBranchInstr()->IsUnconditional()))
            {
                return false;
            }

            BasicBlock * nextBlock = nextLabel->GetBasicBlock();
            IR::BranchInstr * branchTofinallyBlockOrEarlyExit = nextLabel->m_next->AsBranchInstr();
            IR::LabelInstr * finallyBlockLabelOrEarlyExitLabel = branchTofinallyBlockOrEarlyExit->GetTarget();
            finallyBlock = finallyBlockLabelOrEarlyExitLabel->GetBasicBlock();
            predBlock = nextBlock;
        }
    }

    Assert(finallyBlock && predBlock);

        if (this->func->m_fg->FindEdge(predBlock, finallyBlock))
        {
            predBlock->RemoveDeadSucc(finallyBlock, this->func->m_fg);

        if (instr->m_opcode == Js::OpCode::BrOnException)
        {
            this->currentBlock->RemoveInstr(instr);
        }

        if (finallyBlock->GetFirstInstr()->AsLabelInstr()->IsUnreferenced())
        {
            // Traverse predBlocks of finallyBlock, if any of the preds have a different region, set m_hasNonBranchRef to true
            // If not, this label can get eliminated and an incorrect region from the predecessor can get propagated in lowered code
            // See test3() in tryfinallytests.js

            Region * finallyRegion = finallyBlock->GetFirstInstr()->AsLabelInstr()->GetRegion();
            FOREACH_PREDECESSOR_BLOCK(pred, finallyBlock)
            {
                Region * predRegion = pred->GetFirstInstr()->AsLabelInstr()->GetRegion();
                if (predRegion != finallyRegion)
                {
                    finallyBlock->GetFirstInstr()->AsLabelInstr()->m_hasNonBranchRef = true;
                }
            } NEXT_PREDECESSOR_BLOCK;
        }

            if (predBlock == this->currentBlock)
            {
                predBlock->DecrementDataUseCount();
            }
        }

    return true;
}

IR::Instr *
GlobOpt::OptPeep(IR::Instr *instr, Value *src1Val, Value *src2Val)
{
    IR::Opnd *dst, *src1, *src2;

    if (this->IsLoopPrePass())
    {
        return instr;
    }

    switch (instr->m_opcode)
    {
        case Js::OpCode::DeadBrEqual:
        case Js::OpCode::DeadBrRelational:
        case Js::OpCode::DeadBrSrEqual:
            src1 = instr->GetSrc1();
            src2 = instr->GetSrc2();

            // These branches were turned into dead branches because they were unnecessary (branch to next, ...).
            // The DeadBr are necessary in case the evaluation of the sources have side-effects.
            // If we know for sure the srcs are primitive or have been type specialized, we don't need these instructions
            if (((src1Val && src1Val->GetValueInfo()->IsPrimitive()) || (src1->IsRegOpnd() && CurrentBlockData()->IsTypeSpecialized(src1->AsRegOpnd()->m_sym))) &&
                ((src2Val && src2Val->GetValueInfo()->IsPrimitive()) || (src2->IsRegOpnd() && CurrentBlockData()->IsTypeSpecialized(src2->AsRegOpnd()->m_sym))))
            {
                this->CaptureByteCodeSymUses(instr);
                instr->m_opcode = Js::OpCode::Nop;
            }
            break;
        case Js::OpCode::DeadBrOnHasProperty:
            src1 = instr->GetSrc1();

            if (((src1Val && src1Val->GetValueInfo()->IsPrimitive()) || (src1->IsRegOpnd() && CurrentBlockData()->IsTypeSpecialized(src1->AsRegOpnd()->m_sym))))
            {
                this->CaptureByteCodeSymUses(instr);
                instr->m_opcode = Js::OpCode::Nop;
            }
            break;
        case Js::OpCode::Ld_A:
        case Js::OpCode::Ld_I4:
            src1 = instr->GetSrc1();
            dst = instr->GetDst();

            if (dst->IsRegOpnd() && dst->IsEqual(src1))
            {
                dst = instr->UnlinkDst();
                if (!dst->GetIsJITOptimizedReg())
                {
                    IR::ByteCodeUsesInstr *bytecodeUse = IR::ByteCodeUsesInstr::New(instr);
                    bytecodeUse->SetDst(dst);
                    instr->InsertAfter(bytecodeUse);
                }
                instr->FreeSrc1();
                instr->m_opcode = Js::OpCode::Nop;
            }
            break;
    }
    return instr;
}

void
GlobOpt::OptimizeIndirUses(IR::IndirOpnd *indirOpnd, IR::Instr * *pInstr, Value **indirIndexValRef)
{
    IR::Instr * &instr = *pInstr;
    Assert(!indirIndexValRef || !*indirIndexValRef);

    // Update value types and copy-prop the base
    OptSrc(indirOpnd->GetBaseOpnd(), &instr, nullptr, indirOpnd);

    IR::RegOpnd *indexOpnd = indirOpnd->GetIndexOpnd();
    if (!indexOpnd)
    {
        return;
    }

    // Update value types and copy-prop the index
    Value *indexVal = OptSrc(indexOpnd, &instr, nullptr, indirOpnd);
    if(indirIndexValRef)
    {
        *indirIndexValRef = indexVal;
    }
}

bool
GlobOpt::IsPREInstrCandidateLoad(Js::OpCode opcode)
{
    switch (opcode)
    {
    case Js::OpCode::LdFld:
    case Js::OpCode::LdFldForTypeOf:
    case Js::OpCode::LdRootFld:
    case Js::OpCode::LdRootFldForTypeOf:
    case Js::OpCode::LdMethodFld:
    case Js::OpCode::LdRootMethodFld:
    case Js::OpCode::LdSlot:
    case Js::OpCode::LdSlotArr:
        return true;
    }

    return false;
}

bool
GlobOpt::IsPREInstrCandidateStore(Js::OpCode opcode)
{
    switch (opcode)
    {
    case Js::OpCode::StFld:
    case Js::OpCode::StRootFld:
    case Js::OpCode::StSlot:
        return true;
    }

    return false;
}

bool
GlobOpt::ImplicitCallFlagsAllowOpts(Loop *loop)
{
    return loop->GetImplicitCallFlags() != Js::ImplicitCall_HasNoInfo &&
        (((loop->GetImplicitCallFlags() & ~Js::ImplicitCall_Accessor) | Js::ImplicitCall_None) == Js::ImplicitCall_None);
}

bool
GlobOpt::ImplicitCallFlagsAllowOpts(Func const *func)
{
    return func->m_fg->implicitCallFlags != Js::ImplicitCall_HasNoInfo &&
        (((func->m_fg->implicitCallFlags & ~Js::ImplicitCall_Accessor) | Js::ImplicitCall_None) == Js::ImplicitCall_None);
}

#if DBG_DUMP
void
GlobOpt::Dump() const
{
    this->DumpSymToValueMap();
}

void
GlobOpt::DumpSymToValueMap(BasicBlock const * block) const
{
    Output::Print(_u("\n*** SymToValueMap ***\n"));
    block->globOptData.DumpSymToValueMap();
}

void
GlobOpt::DumpSymToValueMap() const
{
    DumpSymToValueMap(this->currentBlock);
}

void
GlobOpt::DumpSymVal(int index)
{
    SymID id = index;
    extern Func *CurrentFunc;
    Sym *sym = this->func->m_symTable->Find(id);

    AssertMsg(sym, "Sym not found!!!");

    Output::Print(_u("Sym: "));
    sym->Dump();

    Output::Print(_u("\t\tValueNumber: "));
    Value * pValue = CurrentBlockData()->FindValueFromMapDirect(sym->m_id);
    pValue->Dump();

    Output::Print(_u("\n"));
}

void
GlobOpt::Trace(BasicBlock * block, bool before) const 
{
    bool globOptTrace = Js::Configuration::Global.flags.Trace.IsEnabled(Js::GlobOptPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());
    bool typeSpecTrace = Js::Configuration::Global.flags.Trace.IsEnabled(Js::TypeSpecPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());
    bool floatTypeSpecTrace = Js::Configuration::Global.flags.Trace.IsEnabled(Js::FloatTypeSpecPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());
    bool fieldHoistTrace = Js::Configuration::Global.flags.Trace.IsEnabled(Js::FieldHoistPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());
    bool fieldCopyPropTrace = fieldHoistTrace || Js::Configuration::Global.flags.Trace.IsEnabled(Js::FieldCopyPropPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());
    bool objTypeSpecTrace = Js::Configuration::Global.flags.Trace.IsEnabled(Js::ObjTypeSpecPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());
    bool valueTableTrace = Js::Configuration::Global.flags.Trace.IsEnabled(Js::ValueTablePhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());
    bool fieldPRETrace = Js::Configuration::Global.flags.Trace.IsEnabled(Js::FieldPREPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId());

    bool anyTrace = globOptTrace || typeSpecTrace || floatTypeSpecTrace || fieldCopyPropTrace || fieldHoistTrace || objTypeSpecTrace || valueTableTrace || fieldPRETrace;

    if (!anyTrace)
    {
        return;
    }

    if (fieldPRETrace && this->IsLoopPrePass())
    {
        if (block->isLoopHeader && before)
        {
            Output::Print(_u("====  Loop Prepass block header #%-3d,  Visiting Loop block head #%-3d\n"),
                this->prePassLoop->GetHeadBlock()->GetBlockNum(), block->GetBlockNum());
        }
    }

    if (!typeSpecTrace && !floatTypeSpecTrace && !valueTableTrace && !Js::Configuration::Global.flags.Verbose)
    {
        return;
    }

    if (before)
    {
        Output::Print(_u("========================================================================\n"));
        Output::Print(_u("Begin OptBlock: Block #%-3d"), block->GetBlockNum());
        if (block->loop)
        {
            Output::Print(_u("     Loop block header:%-3d  currentLoop block head:%-3d   %s"),
                block->loop->GetHeadBlock()->GetBlockNum(),
                this->prePassLoop ? this->prePassLoop->GetHeadBlock()->GetBlockNum() : 0,
                this->IsLoopPrePass() ? _u("PrePass") : _u(""));
        }
        Output::Print(_u("\n"));
    }
    else
    {
        Output::Print(_u("-----------------------------------------------------------------------\n"));
        Output::Print(_u("After OptBlock: Block #%-3d\n"), block->GetBlockNum());
    }
    if ((typeSpecTrace || floatTypeSpecTrace) && !block->globOptData.liveVarSyms->IsEmpty())
    {
        Output::Print(_u("    Live var syms: "));
        block->globOptData.liveVarSyms->Dump();
    }
    if (typeSpecTrace && !block->globOptData.liveInt32Syms->IsEmpty())
    {
        Assert(this->tempBv->IsEmpty());
        this->tempBv->Minus(block->globOptData.liveInt32Syms, block->globOptData.liveLossyInt32Syms);
        if(!this->tempBv->IsEmpty())
        {
            Output::Print(_u("    Int32 type specialized (lossless) syms: "));
            this->tempBv->Dump();
        }
        this->tempBv->ClearAll();
        if(!block->globOptData.liveLossyInt32Syms->IsEmpty())
        {
            Output::Print(_u("    Int32 converted (lossy) syms: "));
            block->globOptData.liveLossyInt32Syms->Dump();
        }
    }
    if (floatTypeSpecTrace && !block->globOptData.liveFloat64Syms->IsEmpty())
    {
        Output::Print(_u("    Float64 type specialized syms: "));
        block->globOptData.liveFloat64Syms->Dump();
    }
    if ((fieldCopyPropTrace || objTypeSpecTrace) && this->DoFieldCopyProp(block->loop) && !block->globOptData.liveFields->IsEmpty())
    {
        Output::Print(_u("    Live field syms: "));
        block->globOptData.liveFields->Dump();
    }
    if ((fieldHoistTrace || objTypeSpecTrace) && this->DoFieldHoisting(block->loop) && HasHoistableFields(block))
    {
        Output::Print(_u("    Hoistable field sym: "));
        block->globOptData.hoistableFields->Dump();
    }
    if (objTypeSpecTrace || valueTableTrace)
    {
        Output::Print(_u("    Value table:\n"));
        block->globOptData.DumpSymToValueMap();
    }

    if (before)
    {
        Output::Print(_u("-----------------------------------------------------------------------\n")); \
    }

    Output::Flush();
}

void
GlobOpt::TraceSettings() const
{
    Output::Print(_u("GlobOpt Settings:\r\n"));
    Output::Print(_u("    FloatTypeSpec: %s\r\n"), this->DoFloatTypeSpec() ? _u("enabled") : _u("disabled"));
    Output::Print(_u("    AggressiveIntTypeSpec: %s\r\n"), this->DoAggressiveIntTypeSpec() ? _u("enabled") : _u("disabled"));
    Output::Print(_u("    LossyIntTypeSpec: %s\r\n"), this->DoLossyIntTypeSpec() ? _u("enabled") : _u("disabled"));
    Output::Print(_u("    ArrayCheckHoist: %s\r\n"),  this->func->IsArrayCheckHoistDisabled() ? _u("disabled") : _u("enabled"));
    Output::Print(_u("    ImplicitCallFlags: %s\r\n"), Js::DynamicProfileInfo::GetImplicitCallFlagsString(this->func->m_fg->implicitCallFlags));
    for (Loop * loop = this->func->m_fg->loopList; loop != NULL; loop = loop->next)
    {
        Output::Print(_u("        loop: %d, ImplicitCallFlags: %s\r\n"), loop->GetLoopNumber(),
            Js::DynamicProfileInfo::GetImplicitCallFlagsString(loop->GetImplicitCallFlags()));
    }

    Output::Flush();
}
#endif  // DBG_DUMP

IR::Instr *
GlobOpt::TrackMarkTempObject(IR::Instr * instrStart, IR::Instr * instrLast)
{
    if (!this->func->GetHasMarkTempObjects())
    {
        return instrLast;
    }
    IR::Instr * instr = instrStart;
    IR::Instr * instrEnd = instrLast->m_next;
    IR::Instr * lastInstr = nullptr;
    GlobOptBlockData& globOptData = *CurrentBlockData();
    do
    {
        bool mayNeedBailOnImplicitCallsPreOp = !this->IsLoopPrePass()
            && instr->HasAnyImplicitCalls()
            && globOptData.maybeTempObjectSyms != nullptr;
        if (mayNeedBailOnImplicitCallsPreOp)
        {
            IR::Opnd * src1 = instr->GetSrc1();
            if (src1)
            {
                instr = GenerateBailOutMarkTempObjectIfNeeded(instr, src1, false);
                IR::Opnd * src2 = instr->GetSrc2();
                if (src2)
                {
                    instr = GenerateBailOutMarkTempObjectIfNeeded(instr, src2, false);
                }
            }
        }

        IR::Opnd *dst = instr->GetDst();
        if (dst)
        {
            if (dst->IsRegOpnd())
            {
                TrackTempObjectSyms(instr, dst->AsRegOpnd());
            }
            else if (mayNeedBailOnImplicitCallsPreOp)
            {
                instr = GenerateBailOutMarkTempObjectIfNeeded(instr, dst, true);
            }
        }

        lastInstr = instr;
        instr = instr->m_next;
    }
    while (instr != instrEnd);
    return lastInstr;
}

void
GlobOpt::TrackTempObjectSyms(IR::Instr * instr, IR::RegOpnd * opnd)
{
    // If it is marked as dstIsTempObject, we should have mark temped it, or type specialized it to Ld_I4.
    Assert(!instr->dstIsTempObject || ObjectTempVerify::CanMarkTemp(instr, nullptr));
    GlobOptBlockData& globOptData = *CurrentBlockData();
    bool canStoreTemp = false;
    bool maybeTemp = false;
    if (OpCodeAttr::TempObjectProducing(instr->m_opcode))
    {
        maybeTemp = instr->dstIsTempObject;

        // We have to make sure that lower will always generate code to do stack allocation
        // before we can store any other stack instance onto it. Otherwise, we would not
        // walk object to box the stack property.
        canStoreTemp = instr->dstIsTempObject && ObjectTemp::CanStoreTemp(instr);
    }
    else if (OpCodeAttr::TempObjectTransfer(instr->m_opcode))
    {
        // Need to check both sources, GetNewScObject has two srcs for transfer.
        // No need to get var equiv sym here as transfer of type spec value does not transfer a mark temp object.
        maybeTemp = globOptData.maybeTempObjectSyms && (
            (instr->GetSrc1()->IsRegOpnd() && globOptData.maybeTempObjectSyms->Test(instr->GetSrc1()->AsRegOpnd()->m_sym->m_id))
            || (instr->GetSrc2() && instr->GetSrc2()->IsRegOpnd() && globOptData.maybeTempObjectSyms->Test(instr->GetSrc2()->AsRegOpnd()->m_sym->m_id)));

        canStoreTemp = globOptData.canStoreTempObjectSyms && (
            (instr->GetSrc1()->IsRegOpnd() && globOptData.canStoreTempObjectSyms->Test(instr->GetSrc1()->AsRegOpnd()->m_sym->m_id))
            && (!instr->GetSrc2() || (instr->GetSrc2()->IsRegOpnd() && globOptData.canStoreTempObjectSyms->Test(instr->GetSrc2()->AsRegOpnd()->m_sym->m_id))));

        Assert(!canStoreTemp || instr->dstIsTempObject);
        Assert(!maybeTemp || instr->dstIsTempObject);
    }

    // Need to get the var equiv sym as assignment of type specialized sym kill the var sym value anyway.
    StackSym * sym = opnd->m_sym;
    if (!sym->IsVar())
    {
        sym = sym->GetVarEquivSym(nullptr);
        if (sym == nullptr)
        {
            return;
        }
    }

    SymID symId = sym->m_id;
    if (maybeTemp)
    {
        // Only var sym should be temp objects
        Assert(opnd->m_sym == sym);

        if (globOptData.maybeTempObjectSyms == nullptr)
        {
            globOptData.maybeTempObjectSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
        }
        globOptData.maybeTempObjectSyms->Set(symId);

        if (canStoreTemp)
        {
            if (instr->m_opcode == Js::OpCode::NewScObjectLiteral && !this->IsLoopPrePass())
            {
                // For object literal, we install the final type up front.
                // If there are bailout before we finish initializing all the fields, we need to
                // zero out the rest if we stack allocate the literal, so that the boxing would not
                // try to box trash pointer in the properties.

                // Although object Literal initialization can be done lexically, BailOnNoProfile may cause some path
                // to disappear. Do it is flow base make it easier to stop propagate those entries.

                IR::IntConstOpnd * propertyArrayIdOpnd = instr->GetSrc1()->AsIntConstOpnd();
                const Js::PropertyIdArray * propIds = instr->m_func->GetJITFunctionBody()->ReadPropertyIdArrayFromAuxData(propertyArrayIdOpnd->AsUint32());

                // Duplicates are removed by parser
                Assert(!propIds->hadDuplicates);

                if (globOptData.stackLiteralInitFldDataMap == nullptr)
                {
                    globOptData.stackLiteralInitFldDataMap = JitAnew(alloc, StackLiteralInitFldDataMap, alloc);
                }
                else
                {
                    Assert(!globOptData.stackLiteralInitFldDataMap->ContainsKey(sym));
                }
                StackLiteralInitFldData data = { propIds, 0};
                globOptData.stackLiteralInitFldDataMap->AddNew(sym, data);
            }

            if (globOptData.canStoreTempObjectSyms == nullptr)
            {
                globOptData.canStoreTempObjectSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
            }
            globOptData.canStoreTempObjectSyms->Set(symId);
        }
        else if (globOptData.canStoreTempObjectSyms)
        {
            globOptData.canStoreTempObjectSyms->Clear(symId);
        }
    }
    else
    {
        Assert(!canStoreTemp);
        if (globOptData.maybeTempObjectSyms)
        {
            if (globOptData.canStoreTempObjectSyms)
            {
                globOptData.canStoreTempObjectSyms->Clear(symId);
            }
            globOptData.maybeTempObjectSyms->Clear(symId);
        }
        else
        {
            Assert(!globOptData.canStoreTempObjectSyms);
        }

        // The symbol is being assigned to, the sym shouldn't still be in the stackLiteralInitFldDataMap
        Assert(this->IsLoopPrePass() ||
            globOptData.stackLiteralInitFldDataMap == nullptr
            || globOptData.stackLiteralInitFldDataMap->Count() == 0
            || !globOptData.stackLiteralInitFldDataMap->ContainsKey(sym));
    }
}

IR::Instr *
GlobOpt::GenerateBailOutMarkTempObjectIfNeeded(IR::Instr * instr, IR::Opnd * opnd, bool isDst)
{
    Assert(opnd);
    Assert(isDst == (opnd == instr->GetDst()));
    Assert(opnd != instr->GetDst() || !opnd->IsRegOpnd());
    Assert(!this->IsLoopPrePass());
    Assert(instr->HasAnyImplicitCalls());

    // Only dst reg opnd opcode or ArgOut_A should have dstIsTempObject marked
    Assert(!isDst || !instr->dstIsTempObject || instr->m_opcode == Js::OpCode::ArgOut_A);

    // Post-op implicit call shouldn't have installed yet
    Assert(!instr->HasBailOutInfo() || (instr->GetBailOutKind() & IR::BailOutKindBits) != IR::BailOutOnImplicitCalls);

    GlobOptBlockData& globOptData = *CurrentBlockData();
    Assert(globOptData.maybeTempObjectSyms != nullptr);

    IR::PropertySymOpnd * propertySymOpnd = nullptr;
    StackSym * stackSym = ObjectTemp::GetStackSym(opnd, &propertySymOpnd);

    // It is okay to not get the var equiv sym here, as use of a type specialized sym is not use of the temp object
    // so no need to add mark temp bailout.
    // TempObjectSysm doesn't contain any type spec sym, so we will get false here for all type spec sym.
    if (stackSym && globOptData.maybeTempObjectSyms->Test(stackSym->m_id))
    {
        if (instr->HasBailOutInfo())
        {
            instr->SetBailOutKind(instr->GetBailOutKind() | IR::BailOutMarkTempObject);
        }
        else
        {
            // On insert the pre op bailout if it is not Direct field access do nothing, don't check the dst yet.
            // SetTypeCheckBailout will clear this out if it is direct field access.
            if (isDst
                || (instr->m_opcode == Js::OpCode::FromVar && !opnd->GetValueType().IsPrimitive())
                || propertySymOpnd == nullptr
                || !propertySymOpnd->IsTypeCheckProtected())
            {
                this->GenerateBailAtOperation(&instr, IR::BailOutMarkTempObject);
            }
        }

        if (!opnd->IsRegOpnd() && (!isDst || (globOptData.canStoreTempObjectSyms && globOptData.canStoreTempObjectSyms->Test(stackSym->m_id))))
        {
            // If this opnd is a dst, that means that the object pointer is a stack object,
            // and we can store temp object/number on it.
            // If the opnd is a src, that means that the object pointer may be a stack object
            // so the load may be a temp object/number and we need to track its use.

            // Don't mark start of indir as can store temp, because we don't actually know
            // what it is assigning to.
            if (!isDst || !opnd->IsIndirOpnd())
            {
                opnd->SetCanStoreTemp();
            }

            if (propertySymOpnd)
            {
                // Track initfld of stack literals
                if (isDst && instr->m_opcode == Js::OpCode::InitFld)
                {
                    const Js::PropertyId propertyId = propertySymOpnd->m_sym->AsPropertySym()->m_propertyId;

                    // We don't need to track numeric properties init
                    if (!this->func->GetThreadContextInfo()->IsNumericProperty(propertyId))
                    {
                        DebugOnly(bool found = false);
                        globOptData.stackLiteralInitFldDataMap->RemoveIf(stackSym,
                            [&](StackSym * key, StackLiteralInitFldData & data)
                        {
                            DebugOnly(found = true);
                            Assert(key == stackSym);
                            Assert(data.currentInitFldCount < data.propIds->count);

                            if (data.propIds->elements[data.currentInitFldCount] != propertyId)
                            {
#if DBG
                                bool duplicate = false;
                                for (uint i = 0; i < data.currentInitFldCount; i++)
                                {
                                    if (data.propIds->elements[i] == propertyId)
                                    {
                                        duplicate = true;
                                        break;
                                    }
                                }
                                Assert(duplicate);
#endif
                                // duplicate initialization
                                return false;
                            }
                            bool finished = (++data.currentInitFldCount == data.propIds->count);
#if DBG
                            if (finished)
                            {
                                // We can still track the finished stack literal InitFld lexically.
                                this->finishedStackLiteralInitFld->Set(stackSym->m_id);
                            }
#endif
                            return finished;
                        });
                        // We might still see InitFld even we have finished with all the property Id because
                        // of duplicate entries at the end
                        Assert(found || finishedStackLiteralInitFld->Test(stackSym->m_id));
                    }
                }
            }
        }
    }
    return instr;
}

LoopCount *
GlobOpt::GetOrGenerateLoopCountForMemOp(Loop *loop)
{
    LoopCount *loopCount = loop->loopCount;

    if (loopCount && !loopCount->HasGeneratedLoopCountSym())
    {
        Assert(loop->bailOutInfo);
        EnsureBailTarget(loop);
        GenerateLoopCountPlusOne(loop, loopCount);
    }

    return loopCount;
}

IR::Opnd *
GlobOpt::GenerateInductionVariableChangeForMemOp(Loop *loop, byte unroll, IR::Instr *insertBeforeInstr)
{
    LoopCount *loopCount = loop->loopCount;
    IR::Opnd *sizeOpnd = nullptr;
    Assert(loopCount);
    Assert(loop->memOpInfo->inductionVariableOpndPerUnrollMap);
    if (loop->memOpInfo->inductionVariableOpndPerUnrollMap->TryGetValue(unroll, &sizeOpnd))
    {
        return sizeOpnd;
    }

    Func *localFunc = loop->GetFunc();

    const auto InsertInstr = [&](IR::Instr *instr)
    {
        if (insertBeforeInstr == nullptr)
        {
            loop->landingPad->InsertAfter(instr);
        }
        else
        {
            insertBeforeInstr->InsertBefore(instr);
        }
    };

    if (loopCount->LoopCountMinusOneSym())
    {
        IRType type = loopCount->LoopCountSym()->GetType();

        // Loop count is off by one, so add one
        IR::RegOpnd *loopCountOpnd = IR::RegOpnd::New(loopCount->LoopCountSym(), type, localFunc);
        sizeOpnd = loopCountOpnd;

        if (unroll != 1)
        {
            sizeOpnd = IR::RegOpnd::New(TyUint32, this->func);

            IR::Opnd *unrollOpnd = IR::IntConstOpnd::New(unroll, type, localFunc);

            InsertInstr(IR::Instr::New(Js::OpCode::Mul_I4,
                sizeOpnd,
                loopCountOpnd,
                unrollOpnd,
                localFunc));

        }
    }
    else
    {
        uint size = (loopCount->LoopCountMinusOneConstantValue() + 1)  * unroll;
        sizeOpnd = IR::IntConstOpnd::New(size, IRType::TyUint32, localFunc);
    }
    loop->memOpInfo->inductionVariableOpndPerUnrollMap->Add(unroll, sizeOpnd);
    return sizeOpnd;
}

IR::RegOpnd*
GlobOpt::GenerateStartIndexOpndForMemop(Loop *loop, IR::Opnd *indexOpnd, IR::Opnd *sizeOpnd, bool isInductionVariableChangeIncremental, bool bIndexAlreadyChanged, IR::Instr *insertBeforeInstr)
{
    IR::RegOpnd *startIndexOpnd = nullptr;
    Func *localFunc = loop->GetFunc();
    IRType type = indexOpnd->GetType();

    const int cacheIndex = ((int)isInductionVariableChangeIncremental << 1) | (int)bIndexAlreadyChanged;
    if (loop->memOpInfo->startIndexOpndCache[cacheIndex])
    {
        return loop->memOpInfo->startIndexOpndCache[cacheIndex];
    }
    const auto InsertInstr = [&](IR::Instr *instr)
    {
        if (insertBeforeInstr == nullptr)
        {
            loop->landingPad->InsertAfter(instr);
        }
        else
        {
            insertBeforeInstr->InsertBefore(instr);
        }
    };

    startIndexOpnd = IR::RegOpnd::New(type, localFunc);

    // If the 2 are different we can simply use indexOpnd
    if (isInductionVariableChangeIncremental != bIndexAlreadyChanged)
    {
        InsertInstr(IR::Instr::New(Js::OpCode::Ld_A,
                                   startIndexOpnd,
                                   indexOpnd,
                                   localFunc));
    }
    else
    {
        // Otherwise add 1 to it
        InsertInstr(IR::Instr::New(Js::OpCode::Add_I4,
                                   startIndexOpnd,
                                   indexOpnd,
                                   IR::IntConstOpnd::New(1, type, localFunc, true),
                                   localFunc));
    }

    if (!isInductionVariableChangeIncremental)
    {
        InsertInstr(IR::Instr::New(Js::OpCode::Sub_I4,
                                   startIndexOpnd,
                                   startIndexOpnd,
                                   sizeOpnd,
                                   localFunc));
    }

    loop->memOpInfo->startIndexOpndCache[cacheIndex] = startIndexOpnd;
    return startIndexOpnd;
}

IR::Instr*
GlobOpt::FindUpperBoundsCheckInstr(IR::Instr* fromInstr)
{
    IR::Instr *upperBoundCheck = fromInstr;
    do
    {
        upperBoundCheck = upperBoundCheck->m_prev;
        Assert(upperBoundCheck);
        Assert(!upperBoundCheck->IsLabelInstr());
    } while (upperBoundCheck->m_opcode != Js::OpCode::BoundCheck);
    return upperBoundCheck;
}

IR::Instr*
GlobOpt::FindArraySegmentLoadInstr(IR::Instr* fromInstr)
{
    IR::Instr *headSegmentLengthLoad = fromInstr;
    do
    {
        headSegmentLengthLoad = headSegmentLengthLoad->m_prev;
        Assert(headSegmentLengthLoad);
        Assert(!headSegmentLengthLoad->IsLabelInstr());
    } while (headSegmentLengthLoad->m_opcode != Js::OpCode::LdIndir);
    return headSegmentLengthLoad;
}

void
GlobOpt::RemoveMemOpSrcInstr(IR::Instr* memopInstr, IR::Instr* srcInstr, BasicBlock* block)
{
    Assert(srcInstr && (srcInstr->m_opcode == Js::OpCode::LdElemI_A || srcInstr->m_opcode == Js::OpCode::StElemI_A || srcInstr->m_opcode == Js::OpCode::StElemI_A_Strict));
    Assert(memopInstr && (memopInstr->m_opcode == Js::OpCode::Memcopy || memopInstr->m_opcode == Js::OpCode::Memset));
    Assert(block);
    const bool isDst = srcInstr->m_opcode == Js::OpCode::StElemI_A || srcInstr->m_opcode == Js::OpCode::StElemI_A_Strict;
    IR::RegOpnd* opnd = (isDst ? memopInstr->GetDst() : memopInstr->GetSrc1())->AsIndirOpnd()->GetBaseOpnd();
    IR::ArrayRegOpnd* arrayOpnd = opnd->IsArrayRegOpnd() ? opnd->AsArrayRegOpnd() : nullptr;

    IR::Instr* topInstr = srcInstr;
    if (srcInstr->extractedUpperBoundCheckWithoutHoisting)
    {
        IR::Instr *upperBoundCheck = FindUpperBoundsCheckInstr(srcInstr);
        Assert(upperBoundCheck && upperBoundCheck != srcInstr);
        topInstr = upperBoundCheck;
    }
    if (srcInstr->loadedArrayHeadSegmentLength && arrayOpnd && arrayOpnd->HeadSegmentLengthSym())
    {
        IR::Instr *arrayLoadSegmentHeadLength = FindArraySegmentLoadInstr(topInstr);
        Assert(arrayLoadSegmentHeadLength);
        topInstr = arrayLoadSegmentHeadLength;
        arrayOpnd->RemoveHeadSegmentLengthSym();
    }
    if (srcInstr->loadedArrayHeadSegment && arrayOpnd && arrayOpnd->HeadSegmentSym())
    {
        IR::Instr *arrayLoadSegmentHead = FindArraySegmentLoadInstr(topInstr);
        Assert(arrayLoadSegmentHead);
        topInstr = arrayLoadSegmentHead;
        arrayOpnd->RemoveHeadSegmentSym();
    }

    // If no bounds check are present, simply look up for instruction added for instrumentation
    if(topInstr == srcInstr)
    {
        bool checkPrev = true;
        while (checkPrev)
        {
            switch (topInstr->m_prev->m_opcode)
            {
            case Js::OpCode::BailOnNotArray:
            case Js::OpCode::NoImplicitCallUses:
            case Js::OpCode::ByteCodeUses:
                topInstr = topInstr->m_prev;
                checkPrev = !!topInstr->m_prev;
                break;
            default:
                checkPrev = false;
                break;
            }
        }
    }

    while (topInstr != srcInstr)
    {
        IR::Instr* removeInstr = topInstr;
        topInstr = topInstr->m_next;
        Assert(
            removeInstr->m_opcode == Js::OpCode::BailOnNotArray ||
            removeInstr->m_opcode == Js::OpCode::NoImplicitCallUses ||
            removeInstr->m_opcode == Js::OpCode::ByteCodeUses ||
            removeInstr->m_opcode == Js::OpCode::LdIndir ||
            removeInstr->m_opcode == Js::OpCode::BoundCheck
        );
        if (removeInstr->m_opcode != Js::OpCode::ByteCodeUses)
        {
            block->RemoveInstr(removeInstr);
        }
    }
    this->ConvertToByteCodeUses(srcInstr);
}

void
GlobOpt::GetMemOpSrcInfo(Loop* loop, IR::Instr* instr, IR::RegOpnd*& base, IR::RegOpnd*& index, IRType& arrayType)
{
    Assert(instr && (instr->m_opcode == Js::OpCode::LdElemI_A || instr->m_opcode == Js::OpCode::StElemI_A || instr->m_opcode == Js::OpCode::StElemI_A_Strict));
    IR::Opnd* arrayOpnd = instr->m_opcode == Js::OpCode::LdElemI_A ? instr->GetSrc1() : instr->GetDst();
    Assert(arrayOpnd->IsIndirOpnd());

    IR::IndirOpnd* indirArrayOpnd = arrayOpnd->AsIndirOpnd();
    IR::RegOpnd* baseOpnd = (IR::RegOpnd*)indirArrayOpnd->GetBaseOpnd();
    IR::RegOpnd* indexOpnd = (IR::RegOpnd*)indirArrayOpnd->GetIndexOpnd();
    Assert(baseOpnd);
    Assert(indexOpnd);

    // Process Out Params
    base = baseOpnd;
    index = indexOpnd;
    arrayType = indirArrayOpnd->GetType();
}

void
GlobOpt::EmitMemop(Loop * loop, LoopCount *loopCount, const MemOpEmitData* emitData)
{
    Assert(emitData);
    Assert(emitData->candidate);
    Assert(emitData->stElemInstr);
    Assert(emitData->stElemInstr->m_opcode == Js::OpCode::StElemI_A || emitData->stElemInstr->m_opcode == Js::OpCode::StElemI_A_Strict);
    IR::BailOutKind bailOutKind = emitData->bailOutKind;

    const byte unroll = emitData->inductionVar.unroll;
    Assert(unroll == 1);
    const bool isInductionVariableChangeIncremental = emitData->inductionVar.isIncremental;
    const bool bIndexAlreadyChanged = emitData->candidate->bIndexAlreadyChanged;

    IR::RegOpnd *baseOpnd = nullptr;
    IR::RegOpnd *indexOpnd = nullptr;
    IRType dstType;
    GetMemOpSrcInfo(loop, emitData->stElemInstr, baseOpnd, indexOpnd, dstType);

    Func *localFunc = loop->GetFunc();

    // Handle bailout info
    EnsureBailTarget(loop);
    Assert(bailOutKind != IR::BailOutInvalid);

    // Keep only Array bits bailOuts. Consider handling these bailouts instead of simply ignoring them
    bailOutKind &= IR::BailOutForArrayBits;

    // Add our custom bailout to handle Op_MemCopy return value.
    bailOutKind |= IR::BailOutOnMemOpError;
    BailOutInfo *const bailOutInfo = loop->bailOutInfo;
    Assert(bailOutInfo);

    IR::Instr *insertBeforeInstr = bailOutInfo->bailOutInstr;
    Assert(insertBeforeInstr);
    IR::Opnd *sizeOpnd = GenerateInductionVariableChangeForMemOp(loop, unroll, insertBeforeInstr);
    IR::RegOpnd *startIndexOpnd = GenerateStartIndexOpndForMemop(loop, indexOpnd, sizeOpnd, isInductionVariableChangeIncremental, bIndexAlreadyChanged, insertBeforeInstr);
    IR::IndirOpnd* dstOpnd = IR::IndirOpnd::New(baseOpnd, startIndexOpnd, dstType, localFunc);

    IR::Opnd *src1;
    const bool isMemset = emitData->candidate->IsMemSet();

    // Get the source according to the memop type
    if (isMemset)
    {
        MemSetEmitData* data = (MemSetEmitData*)emitData;
        const Loop::MemSetCandidate* candidate = data->candidate->AsMemSet();
        if (candidate->srcSym)
        {
            IR::RegOpnd* regSrc = IR::RegOpnd::New(candidate->srcSym, candidate->srcSym->GetType(), func);
            regSrc->SetIsJITOptimizedReg(true);
            src1 = regSrc;
        }
        else
        {
            src1 = IR::AddrOpnd::New(candidate->constant.ToVar(localFunc), IR::AddrOpndKindConstantAddress, localFunc);
        }
    }
    else
    {
        Assert(emitData->candidate->IsMemCopy());

        MemCopyEmitData* data = (MemCopyEmitData*)emitData;
        Assert(data->ldElemInstr);
        Assert(data->ldElemInstr->m_opcode == Js::OpCode::LdElemI_A);

        IR::RegOpnd *srcBaseOpnd = nullptr;
        IR::RegOpnd *srcIndexOpnd = nullptr;
        IRType srcType;
        GetMemOpSrcInfo(loop, data->ldElemInstr, srcBaseOpnd, srcIndexOpnd, srcType);
        Assert(GetVarSymID(srcIndexOpnd->GetStackSym()) == GetVarSymID(indexOpnd->GetStackSym()));

        src1 = IR::IndirOpnd::New(srcBaseOpnd, startIndexOpnd, srcType, localFunc);
    }

    // Generate memcopy
    IR::Instr* memopInstr = IR::BailOutInstr::New(isMemset ? Js::OpCode::Memset : Js::OpCode::Memcopy, bailOutKind, bailOutInfo, localFunc);
    memopInstr->SetDst(dstOpnd);
    memopInstr->SetSrc1(src1);
    memopInstr->SetSrc2(sizeOpnd);
    insertBeforeInstr->InsertBefore(memopInstr);

#if DBG_DUMP
    if (DO_MEMOP_TRACE())
    {
        char valueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
        baseOpnd->GetValueType().ToString(valueTypeStr);
        const int loopCountBufSize = 16;
        char16 loopCountBuf[loopCountBufSize];
        if (loopCount->LoopCountMinusOneSym())
        {
            swprintf_s(loopCountBuf, _u("s%u"), loopCount->LoopCountMinusOneSym()->m_id);
        }
        else
        {
            swprintf_s(loopCountBuf, _u("%u"), loopCount->LoopCountMinusOneConstantValue() + 1);
        }
        if (isMemset)
        {
            const Loop::MemSetCandidate* candidate = emitData->candidate->AsMemSet();
            const int constBufSize = 32;
            char16 constBuf[constBufSize];
            if (candidate->srcSym)
            {
                swprintf_s(constBuf, _u("s%u"), candidate->srcSym->m_id);
            }
            else
            {
                switch (candidate->constant.type)
                {
                case TyInt8:
                case TyInt16:
                case TyInt32:
                case TyInt64:
                    swprintf_s(constBuf, sizeof(IntConstType) == 8 ? _u("%lld") : _u("%d"), candidate->constant.u.intConst.value);
                    break;
                case TyFloat32:
                case TyFloat64:
                    swprintf_s(constBuf, _u("%.4f"), candidate->constant.u.floatConst.value);
                    break;
                case TyVar:
                    swprintf_s(constBuf, sizeof(Js::Var) == 8 ? _u("0x%.16llX") : _u("0x%.8X"), candidate->constant.u.varConst.value);
                    break;
                default:
                    AssertMsg(false, "Unsupported constant type");
                    swprintf_s(constBuf, _u("Unknown"));
                    break;
                }
            }
            TRACE_MEMOP_PHASE(MemSet, loop, emitData->stElemInstr,
                              _u("ValueType: %S, Base: s%u, Index: s%u, Constant: %s, LoopCount: %s, IsIndexChangedBeforeUse: %d"),
                              valueTypeStr,
                              candidate->base,
                              candidate->index,
                              constBuf,
                              loopCountBuf,
                              bIndexAlreadyChanged);
        }
        else
        {
            const Loop::MemCopyCandidate* candidate = emitData->candidate->AsMemCopy();
            TRACE_MEMOP_PHASE(MemCopy, loop, emitData->stElemInstr,
                              _u("ValueType: %S, StBase: s%u, Index: s%u, LdBase: s%u, LoopCount: %s, IsIndexChangedBeforeUse: %d"),
                              valueTypeStr,
                              candidate->base,
                              candidate->index,
                              candidate->ldBase,
                              loopCountBuf,
                              bIndexAlreadyChanged);
        }

    }
#endif

    RemoveMemOpSrcInstr(memopInstr, emitData->stElemInstr, emitData->block);
    if (!isMemset)
    {
        RemoveMemOpSrcInstr(memopInstr, ((MemCopyEmitData*)emitData)->ldElemInstr, emitData->block);
    }
}

bool
GlobOpt::InspectInstrForMemSetCandidate(Loop* loop, IR::Instr* instr, MemSetEmitData* emitData, bool& errorInInstr)
{
    Assert(emitData && emitData->candidate && emitData->candidate->IsMemSet());
    Loop::MemSetCandidate* candidate = (Loop::MemSetCandidate*)emitData->candidate;
    if (instr->m_opcode == Js::OpCode::StElemI_A || instr->m_opcode == Js::OpCode::StElemI_A_Strict)
    {
        if (instr->GetDst()->IsIndirOpnd()
            && (GetVarSymID(instr->GetDst()->AsIndirOpnd()->GetBaseOpnd()->GetStackSym()) == candidate->base)
            && (GetVarSymID(instr->GetDst()->AsIndirOpnd()->GetIndexOpnd()->GetStackSym()) == candidate->index)
            )
        {
            Assert(instr->IsProfiledInstr());
            emitData->stElemInstr = instr;
            emitData->bailOutKind = instr->GetBailOutKind();
            return true;
        }
        TRACE_MEMOP_PHASE_VERBOSE(MemSet, loop, instr, _u("Orphan StElemI_A detected"));
        errorInInstr = true;
    }
    else if (instr->m_opcode == Js::OpCode::LdElemI_A)
    {
        TRACE_MEMOP_PHASE_VERBOSE(MemSet, loop, instr, _u("Orphan LdElemI_A detected"));
        errorInInstr = true;
    }
    return false;
}

bool
GlobOpt::InspectInstrForMemCopyCandidate(Loop* loop, IR::Instr* instr, MemCopyEmitData* emitData, bool& errorInInstr)
{
    Assert(emitData && emitData->candidate && emitData->candidate->IsMemCopy());
    Loop::MemCopyCandidate* candidate = (Loop::MemCopyCandidate*)emitData->candidate;
    if (instr->m_opcode == Js::OpCode::StElemI_A || instr->m_opcode == Js::OpCode::StElemI_A_Strict)
    {
        if (
            instr->GetDst()->IsIndirOpnd() &&
            (GetVarSymID(instr->GetDst()->AsIndirOpnd()->GetBaseOpnd()->GetStackSym()) == candidate->base) &&
            (GetVarSymID(instr->GetDst()->AsIndirOpnd()->GetIndexOpnd()->GetStackSym()) == candidate->index)
            )
        {
            Assert(instr->IsProfiledInstr());
            emitData->stElemInstr = instr;
            emitData->bailOutKind = instr->GetBailOutKind();
            // Still need to find the LdElem
            return false;
        }
        TRACE_MEMOP_PHASE_VERBOSE(MemCopy, loop, instr, _u("Orphan StElemI_A detected"));
        errorInInstr = true;
    }
    else if (instr->m_opcode == Js::OpCode::LdElemI_A)
    {
        if (
            emitData->stElemInstr &&
            instr->GetSrc1()->IsIndirOpnd() &&
            (GetVarSymID(instr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->GetStackSym()) == candidate->ldBase) &&
            (GetVarSymID(instr->GetSrc1()->AsIndirOpnd()->GetIndexOpnd()->GetStackSym()) == candidate->index)
            )
        {
            Assert(instr->IsProfiledInstr());
            emitData->ldElemInstr = instr;
            ValueType stValueType = emitData->stElemInstr->GetDst()->AsIndirOpnd()->GetBaseOpnd()->GetValueType();
            ValueType ldValueType = emitData->ldElemInstr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->GetValueType();
            if (stValueType != ldValueType)
            {
#if DBG_DUMP
                char16 stValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
                stValueType.ToString(stValueTypeStr);
                char16 ldValueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
                ldValueType.ToString(ldValueTypeStr);
                TRACE_MEMOP_PHASE_VERBOSE(MemCopy, loop, instr, _u("for mismatch in Load(%s) and Store(%s) value type"), ldValueTypeStr, stValueTypeStr);
#endif
                errorInInstr = true;
                return false;
            }
            // We found both instruction for this candidate
            return true;
        }
        TRACE_MEMOP_PHASE_VERBOSE(MemCopy, loop, instr, _u("Orphan LdElemI_A detected"));
        errorInInstr = true;
    }
    return false;
}

// The caller is responsible to free the memory allocated between inOrderEmitData[iEmitData -> end]
bool
GlobOpt::ValidateMemOpCandidates(Loop * loop, _Out_writes_(iEmitData) MemOpEmitData** inOrderEmitData, int& iEmitData)
{
    AnalysisAssert(iEmitData == (int)loop->memOpInfo->candidates->Count());
    // We iterate over the second block of the loop only. MemOp Works only if the loop has exactly 2 blocks
    Assert(loop->blockList.HasTwo());

    Loop::MemOpList::Iterator iter(loop->memOpInfo->candidates);
    BasicBlock* bblock = loop->blockList.Head()->next;
    Loop::MemOpCandidate* candidate = nullptr;
    MemOpEmitData* emitData = nullptr;

    // Iterate backward because the list of candidate is reversed
    FOREACH_INSTR_BACKWARD_IN_BLOCK(instr, bblock)
    {
        if (!candidate)
        {
            // Time to check next candidate
            if (!iter.Next())
            {
                // We have been through the whole list of candidates, finish
                break;
            }
            candidate = iter.Data();
            if (!candidate)
            {
                continue;
            }

            // Common check for memset and memcopy
            Loop::InductionVariableChangeInfo inductionVariableChangeInfo = { 0, 0 };

            // Get the inductionVariable changeInfo
            if (!loop->memOpInfo->inductionVariableChangeInfoMap->TryGetValue(candidate->index, &inductionVariableChangeInfo))
            {
                TRACE_MEMOP_VERBOSE(loop, nullptr, _u("MemOp skipped (s%d): no induction variable"), candidate->base);
                return false;
            }

            if (inductionVariableChangeInfo.unroll != candidate->count)
            {
                TRACE_MEMOP_VERBOSE(loop, nullptr, _u("MemOp skipped (s%d): not matching unroll count"), candidate->base);
                return false;
            }

            if (candidate->IsMemSet())
            {
                Assert(!PHASE_OFF(Js::MemSetPhase, this->func));
                emitData = JitAnew(this->alloc, MemSetEmitData);
            }
            else
            {
                Assert(!PHASE_OFF(Js::MemCopyPhase, this->func));
                // Specific check for memcopy
                Assert(candidate->IsMemCopy());
                Loop::MemCopyCandidate* memcopyCandidate = candidate->AsMemCopy();

                if (memcopyCandidate->base == Js::Constants::InvalidSymID
                    || memcopyCandidate->ldBase == Js::Constants::InvalidSymID
                    || (memcopyCandidate->ldCount != memcopyCandidate->count))
                {
                    TRACE_MEMOP_PHASE(MemCopy, loop, nullptr, _u("(s%d): not matching ldElem and stElem"), candidate->base);
                    return false;
                }
                emitData = JitAnew(this->alloc, MemCopyEmitData);
            }
            Assert(emitData);
            emitData->block = bblock;
            emitData->inductionVar = inductionVariableChangeInfo;
            emitData->candidate = candidate;
        }
        bool errorInInstr = false;
        bool candidateFound = candidate->IsMemSet() ?
            InspectInstrForMemSetCandidate(loop, instr, (MemSetEmitData*)emitData, errorInInstr)
            : InspectInstrForMemCopyCandidate(loop, instr, (MemCopyEmitData*)emitData, errorInInstr);
        if (errorInInstr)
        {
            JitAdelete(this->alloc, emitData);
            return false;
        }
        if (candidateFound)
        {
            AnalysisAssert(iEmitData > 0);
            if (iEmitData == 0)
            {
                // Explicit for OACR
                break;
            }
            inOrderEmitData[--iEmitData] = emitData;
            candidate = nullptr;
            emitData = nullptr;

        }
    } NEXT_INSTR_BACKWARD_IN_BLOCK;

    if (iter.IsValid())
    {
        TRACE_MEMOP(loop, nullptr, _u("Candidates not found in loop while validating"));
        return false;
    }
    return true;
}

void
GlobOpt::ProcessMemOp()
{
    FOREACH_LOOP_IN_FUNC_EDITING(loop, this->func)
    {
        if (HasMemOp(loop))
        {
            const int candidateCount = loop->memOpInfo->candidates->Count();
            Assert(candidateCount > 0);

            LoopCount * loopCount = GetOrGenerateLoopCountForMemOp(loop);

            // If loopCount is not available we can not continue with memop
            if (!loopCount || !(loopCount->LoopCountMinusOneSym() || loopCount->LoopCountMinusOneConstantValue()))
            {
                TRACE_MEMOP(loop, nullptr, _u("MemOp skipped for no loop count"));
                loop->doMemOp = false;
                loop->memOpInfo->candidates->Clear();
                continue;
            }

            // The list is reversed, check them and place them in order in the following array
            MemOpEmitData** inOrderCandidates = JitAnewArray(this->alloc, MemOpEmitData*, candidateCount);
            int i = candidateCount;
            if (ValidateMemOpCandidates(loop, inOrderCandidates, i))
            {
                Assert(i == 0);

                // Process the valid MemOp candidate in order.
                for (; i < candidateCount; ++i)
                {
                    // Emit
                    EmitMemop(loop, loopCount, inOrderCandidates[i]);
                    JitAdelete(this->alloc, inOrderCandidates[i]);
                }
            }
            else
            {
                Assert(i != 0);
                for (; i < candidateCount; ++i)
                {
                    JitAdelete(this->alloc, inOrderCandidates[i]);
                }

                // One of the memop candidates did not validate. Do not emit for this loop.
                loop->doMemOp = false;
                loop->memOpInfo->candidates->Clear();
            }

            // Free memory
            JitAdeleteArray(this->alloc, candidateCount, inOrderCandidates);
        }
    } NEXT_LOOP_EDITING;
}
