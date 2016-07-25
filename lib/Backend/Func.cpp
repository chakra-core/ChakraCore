//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "Base/EtwTrace.h"
#include "Base/ScriptContextProfiler.h"
#ifdef VTUNE_PROFILING
#include "Base/VTuneChakraProfile.h"
#endif

Func::Func(JitArenaAllocator *alloc, JITTimeWorkItem * workItem,
    ThreadContextInfo * threadContextInfo,
    ScriptContextInfo * scriptContextInfo,
    JITOutputIDL * outputData,
    const FunctionJITRuntimeInfo *const runtimeInfo,
    JITTimePolymorphicInlineCacheInfo * const polymorphicInlineCacheInfo, CodeGenAllocators *const codeGenAllocators,
    CodeGenNumberAllocator * numberAllocator,
    Js::ScriptContextProfiler *const codeGenProfiler, const bool isBackgroundJIT, Func * parentFunc,
    uint postCallByteCodeOffset, Js::RegSlot returnValueRegSlot, const bool isInlinedConstructor,
    Js::ProfileId callSiteIdInParentFunc, bool isGetterSetter) :
    m_alloc(alloc),
    m_workItem(workItem),
    m_output(outputData),
    m_threadContextInfo(threadContextInfo),
    m_scriptContextInfo(scriptContextInfo),
    m_runtimeInfo(runtimeInfo),
    m_polymorphicInlineCacheInfo(polymorphicInlineCacheInfo),
    m_codeGenAllocators(codeGenAllocators),
    m_inlineeId(0),
    pinnedTypeRefs(nullptr),
    singleTypeGuards(nullptr),
    equivalentTypeGuards(nullptr),
    propertyGuardsByPropertyId(nullptr),
    ctorCachesByPropertyId(nullptr),
    callSiteToArgumentsOffsetFixupMap(nullptr),
    indexedPropertyGuardCount(0),
    propertiesWrittenTo(nullptr),
    lazyBailoutProperties(alloc),
    anyPropertyMayBeWrittenTo(false),
#ifdef PROFILE_EXEC
    m_codeGenProfiler(codeGenProfiler),
#endif
    m_isBackgroundJIT(isBackgroundJIT),
    m_cloner(nullptr),
    m_cloneMap(nullptr),
    m_loopParamSym(nullptr),
    m_funcObjSym(nullptr),
    m_localClosureSym(nullptr),
    m_paramClosureSym(nullptr),
    m_localFrameDisplaySym(nullptr),
    m_bailoutReturnValueSym(nullptr),
    m_hasBailedOutSym(nullptr),
    m_inlineeFrameStartSym(nullptr),
    m_regsUsed(0),
    m_fg(nullptr),
    m_labelCount(0),
    m_argSlotsForFunctionsCalled(0),
    m_isLeaf(false),
    m_hasCalls(false),
    m_hasInlineArgsOpt(false),
    m_canDoInlineArgsOpt(true),
    m_doFastPaths(false),
    hasBailout(false),
    hasBailoutInEHRegion(false),
    hasInstrNumber(false),
    maintainByteCodeOffset(true),
    frameSize(0),
    parentFunc(parentFunc),
    argObjSyms(nullptr),
    m_nonTempLocalVars(nullptr),
    hasAnyStackNestedFunc(false),
    hasMarkTempObjects(false),
    postCallByteCodeOffset(postCallByteCodeOffset),
    maxInlineeArgOutCount(0),
    returnValueRegSlot(returnValueRegSlot),
    firstActualStackOffset(-1),
    m_localVarSlotsOffset(Js::Constants::InvalidOffset),
    m_hasLocalVarChangedOffset(Js::Constants::InvalidOffset),
    actualCount((Js::ArgSlot) - 1),
    tryCatchNestingLevel(0),
    m_localStackHeight(0),
    tempSymDouble(nullptr),
    tempSymBool(nullptr),
    hasInlinee(false),
    thisOrParentInlinerHasArguments(false),
    hasStackArgs(false),
    hasNonSimpleParams(false),
    hasUnoptimizedArgumentsAcccess(false),
    hasApplyTargetInlining(false),
    hasImplicitCalls(false),
    hasTempObjectProducingInstr(false),
    isInlinedConstructor(isInlinedConstructor),
    numberAllocator(numberAllocator),
    loopCount(0),
    callSiteIdInParentFunc(callSiteIdInParentFunc),
    isGetterSetter(isGetterSetter),
    frameInfo(nullptr),
    isTJLoopBody(false),
    isFlowGraphValid(false),
#if DBG
    m_callSiteCount(0),
#endif
    stackNestedFunc(false),
    stackClosure(false)
#if defined(_M_ARM32_OR_ARM64)
    , m_ArgumentsOffset(0)
    , m_epilogLabel(nullptr)
#endif
    , m_funcStartLabel(nullptr)
    , m_funcEndLabel(nullptr)
#ifdef _M_X64
    , m_prologEncoder(alloc)
#endif
#if DBG
    , hasCalledSetDoFastPaths(false)
    , allowRemoveBailOutArgInstr(false)
    , currentPhases(alloc)
    , isPostLower(false)
    , isPostRegAlloc(false)
    , isPostPeeps(false)
    , isPostLayout(false)
    , isPostFinalLower(false)
    , vtableMap(nullptr)
#endif
    , m_yieldOffsetResumeLabelList(nullptr)
    , m_bailOutNoSaveLabel(nullptr)
    , constantAddressRegOpnd(alloc)
    , lastConstantAddressRegLoadInstr(nullptr)
    , m_totalJumpTableSizeInBytesForSwitchStatements(0)
    , slotArrayCheckTable(nullptr)
    , frameDisplayCheckTable(nullptr)
    , stackArgWithFormalsTracker(nullptr)
    , argInsCount(0)
    , m_globalObjTypeSpecFldInfoArray(nullptr)
{

    Assert(this->IsInlined() == !!runtimeInfo);

    if (this->IsTopFunc())
    {
        outputData->writeableEPData.hasJittedStackClosure = false;
        // TODO: (michhol) validate initial values
        outputData->writeableEPData.localVarSlotsOffset = 0;
        outputData->writeableEPData.localVarChangedOffset = 0;
    }

    if (this->IsInlined())
    {
        m_inlineeId = ++(GetTopFunc()->m_inlineeId);
    }
    m_jnFunction = nullptr;
    bool doStackNestedFunc = GetJITFunctionBody()->DoStackNestedFunc();

    bool doStackClosure = GetJITFunctionBody()->DoStackClosure() && !PHASE_OFF(Js::FrameDisplayFastPathPhase, this) && !PHASE_OFF(Js::StackClosurePhase, this);
    Assert(!doStackClosure || doStackNestedFunc);
    this->stackClosure = doStackClosure && this->IsTopFunc();
    if (this->stackClosure)
    {
        // TODO: calculate on runtime side?
        m_output.SetHasJITStackClosure();
    }

    if (GetJITFunctionBody()->DoBackendArgumentsOptimization() && !GetJITFunctionBody()->HasTry())
    {
        // doBackendArgumentsOptimization bit is set when there is no eval inside a function
        // as determined by the bytecode generator.
        SetHasStackArgs(true);
    }

    if (m_workItem->Type() == JsFunctionType)
    {
        if (doStackNestedFunc && GetJITFunctionBody()->GetNestedCount() != 0 &&
            this->GetTopFunc()->m_workItem->Type() != JsLoopBodyWorkItemType) // make sure none of the functions inlined in a jitted loop body allocate nested functions on the stack
        {
            Assert(!(this->IsJitInDebugMode() && !GetJITFunctionBody()->IsLibraryCode()));
            stackNestedFunc = true;
            this->GetTopFunc()->hasAnyStackNestedFunc = true;
        }
    }
    else
    {
        Assert(m_workItem->IsLoopBody());
    }

    if (GetJITFunctionBody()->HasOrParentHasArguments() || parentFunc && parentFunc->thisOrParentInlinerHasArguments)
    {
        thisOrParentInlinerHasArguments = true;
    }

    if (parentFunc == nullptr)
    {
        inlineDepth = 0;
        m_symTable = JitAnew(alloc, SymTable);
        m_symTable->Init(this);
        m_symTable->SetStartingID(static_cast<SymID>(workItem->GetJITFunctionBody()->GetLocalsCount() + 1));

        Assert(Js::Constants::NoByteCodeOffset == postCallByteCodeOffset);
        Assert(Js::Constants::NoRegister == returnValueRegSlot);

#if defined(_M_IX86) ||  defined(_M_X64)
        if (HasArgumentSlot())
        {
            // Pre-allocate the single argument slot we'll reserve for the arguments object.
            // For ARM, the argument slot is not part of the local but part of the register saves
            m_localStackHeight = MachArgsSlotOffset;
        }
#endif
    }
    else
    {
        inlineDepth = parentFunc->inlineDepth + 1;
        Assert(Js::Constants::NoByteCodeOffset != postCallByteCodeOffset);
    }

    this->constructorCacheCount = 0;
    this->constructorCaches = AnewArrayZ(this->m_alloc, JITTimeConstructorCache*, GetJITFunctionBody()->GetProfiledCallSiteCount());

#if DBG_DUMP
    m_codeSize = -1;
#endif

#if defined(_M_X64)
    m_spillSize = -1;
    m_argsSize = -1;
    m_savedRegSize = -1;
#endif

    if (this->IsJitInDebugMode())
    {
        m_nonTempLocalVars = Anew(this->m_alloc, BVSparse<JitArenaAllocator>, this->m_alloc);
    }

    if (GetJITFunctionBody()->IsGenerator())
    {
        m_yieldOffsetResumeLabelList = YieldOffsetResumeLabelList::New(this->m_alloc);
    }

    if (this->IsTopFunc())
    {
        m_globalObjTypeSpecFldInfoArray = JitAnewArrayZ(this->m_alloc, JITObjTypeSpecFldInfo*, GetWorkItem()->GetJITTimeInfo()->GetGlobalObjTypeSpecFldInfoCount());
    }

    for (uint i = 0; i < GetJITFunctionBody()->GetInlineCacheCount(); ++i)
    {
        JITObjTypeSpecFldInfo * info = GetWorkItem()->GetJITTimeInfo()->GetObjTypeSpecFldInfo(i);
        if (info != nullptr)
        {
            Assert(info->GetObjTypeSpecFldId() < GetTopFunc()->GetWorkItem()->GetJITTimeInfo()->GetGlobalObjTypeSpecFldInfoCount());
            GetTopFunc()->m_globalObjTypeSpecFldInfoArray[info->GetObjTypeSpecFldId()] = info;
        }
    }

    canHoistConstantAddressLoad = !PHASE_OFF(Js::HoistConstAddrPhase, this);
}

bool
Func::IsLoopBodyInTry() const
{
    return IsLoopBody() && m_workItem->GetLoopHeader()->isInTry;
}

/* static */
void
Func::Codegen(JitArenaAllocator *alloc, JITTimeWorkItem * workItem,
    ThreadContextInfo * threadContextInfo,
    ScriptContextInfo * scriptContextInfo,
    JITOutputIDL * outputData,
    const FunctionJITRuntimeInfo *const runtimeInfo,
    JITTimePolymorphicInlineCacheInfo * const polymorphicInlineCacheInfo, CodeGenAllocators *const codeGenAllocators,
    CodeGenNumberAllocator * numberAllocator,
    Js::ScriptContextProfiler *const codeGenProfiler, const bool isBackgroundJIT)
{
    bool rejit;
    do
    {
        Func func(alloc, workItem, threadContextInfo,
            scriptContextInfo, outputData, runtimeInfo,
            polymorphicInlineCacheInfo, codeGenAllocators, numberAllocator,
            codeGenProfiler, isBackgroundJIT);
        try
        {
            func.TryCodegen();
            rejit = false;
        }
        catch (Js::RejitException ex)
        {
            // The work item needs to be rejitted, likely due to some optimization that was too aggressive
            if (ex.Reason() == RejitReason::AggressiveIntTypeSpecDisabled)
            {
                workItem->GetJITFunctionBody()->GetProfileInfo()->DisableAggressiveIntTypeSpec(func.IsLoopBody());
                outputData->disableAggressiveIntTypeSpec = TRUE;
            }
            else if (ex.Reason() == RejitReason::InlineApplyDisabled)
            {
                workItem->GetJITFunctionBody()->DisableInlineApply();
                outputData->disableInlineApply = FALSE;
            }
            else if (ex.Reason() == RejitReason::InlineSpreadDisabled)
            {
                workItem->GetJITFunctionBody()->DisableInlineSpread();
                outputData->disableInlineSpread = FALSE;
            }
            else if (ex.Reason() == RejitReason::DisableStackArgOpt)
            {
                workItem->GetJITFunctionBody()->GetProfileInfo()->DisableStackArgOpt();
                outputData->disableStackArgOpt = TRUE;
            }
            else if (ex.Reason() == RejitReason::DisableSwitchOptExpectingInteger ||
                ex.Reason() == RejitReason::DisableSwitchOptExpectingString)
            {
                workItem->GetJITFunctionBody()->GetProfileInfo()->DisableSwitchOpt();
                outputData->disableSwitchOpt = TRUE;
            }
            else
            {
                Assert(ex.Reason() == RejitReason::TrackIntOverflowDisabled);
                workItem->GetJITFunctionBody()->GetProfileInfo()->DisableTrackCompoundedIntOverflow();
                outputData->disableTrackCompoundedIntOverflow = TRUE;
            }

            if (PHASE_TRACE(Js::ReJITPhase, &func))
            {
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                Output::Print(
                    _u("Rejit (compile-time): function: %s (%s) reason: %S\n"),
                    workItem->GetJITFunctionBody()->GetDisplayName(),
                    workItem->GetJITTimeInfo()->GetDebugNumberSet(debugStringBuffer),
                    ex.ReasonName());
            }

            rejit = true;
        }
        // Either the entry point has a reference to the number now, or we failed to code gen and we
        // don't need to numbers, we can flush the completed page now.
        //
        // If the number allocator is NULL then we are shutting down the thread context and so too the
        // code generator. The number allocator must be freed before the recycler (and thus before the
        // code generator) so we can't and don't need to flush it.

        // TODO: OOP JIT, allocator cleanup
    } while (rejit);
}

///----------------------------------------------------------------------------
///
/// Func::TryCodegen
///
///     Attempt to Codegen this function.
///
///----------------------------------------------------------------------------
void
Func::TryCodegen()
{
    Assert(!IsJitInDebugMode() || !GetJITFunctionBody()->HasTry());

    BEGIN_CODEGEN_PHASE(this, Js::BackEndPhase);
    {
        // IRBuilder

        BEGIN_CODEGEN_PHASE(this, Js::IRBuilderPhase);

        if (GetJITFunctionBody()->IsAsmJsMode())
        {
            IRBuilderAsmJs asmIrBuilder(this);
            asmIrBuilder.Build();
        }
        else
        {
            IRBuilder irBuilder(this);
            irBuilder.Build();
        }

        END_CODEGEN_PHASE(this, Js::IRBuilderPhase);

#ifdef IR_VIEWER
        IRtoJSObjectBuilder::DumpIRtoGlobalObject(this, Js::IRBuilderPhase);
#endif /* IR_VIEWER */

        BEGIN_CODEGEN_PHASE(this, Js::InlinePhase);

        InliningHeuristics heuristics(GetWorkItem()->GetJITTimeInfo(), this->IsLoopBody());
        Inline inliner(this, heuristics);
        inliner.Optimize();

        END_CODEGEN_PHASE(this, Js::InlinePhase);

        // FlowGraph
        {
            // Scope for FlowGraph arena
            NoRecoverMemoryJitArenaAllocator fgAlloc(_u("BE-FlowGraph"), m_alloc->GetPageAllocator(), Js::Throw::OutOfMemory);

            BEGIN_CODEGEN_PHASE(this, Js::FGBuildPhase);

            this->m_fg = FlowGraph::New(this, &fgAlloc);
            this->m_fg->Build();

            END_CODEGEN_PHASE(this, Js::FGBuildPhase);

            // Global Optimization and Type Specialization
            BEGIN_CODEGEN_PHASE(this, Js::GlobOptPhase);

            GlobOpt globOpt(this);
            globOpt.Optimize();

            END_CODEGEN_PHASE(this, Js::GlobOptPhase);

            // Delete flowGraph now
            this->m_fg->Destroy();
            this->m_fg = nullptr;
        }

#ifdef IR_VIEWER
        IRtoJSObjectBuilder::DumpIRtoGlobalObject(this, Js::GlobOptPhase);
#endif /* IR_VIEWER */

        // Lowering
        Lowerer lowerer(this);
        BEGIN_CODEGEN_PHASE(this, Js::LowererPhase);
        lowerer.Lower();
        END_CODEGEN_PHASE(this, Js::LowererPhase);

#ifdef IR_VIEWER
        IRtoJSObjectBuilder::DumpIRtoGlobalObject(this, Js::LowererPhase);
#endif /* IR_VIEWER */

        // Encode constants

        Security security(this);

        BEGIN_CODEGEN_PHASE(this, Js::EncodeConstantsPhase)
        security.EncodeLargeConstants();
        END_CODEGEN_PHASE(this, Js::EncodeConstantsPhase);
        if (GetJITFunctionBody()->DoInterruptProbe())
        {
            BEGIN_CODEGEN_PHASE(this, Js::InterruptProbePhase)
            lowerer.DoInterruptProbes();
            END_CODEGEN_PHASE(this, Js::InterruptProbePhase)
        }

        // Register Allocation

        BEGIN_CODEGEN_PHASE(this, Js::RegAllocPhase);

        LinearScan linearScan(this);
        linearScan.RegAlloc();

        END_CODEGEN_PHASE(this, Js::RegAllocPhase);

#ifdef IR_VIEWER
        IRtoJSObjectBuilder::DumpIRtoGlobalObject(this, Js::RegAllocPhase);
#endif /* IR_VIEWER */

        // Peephole optimizations

        BEGIN_CODEGEN_PHASE(this, Js::PeepsPhase);

        Peeps peeps(this);
        peeps.PeepFunc();

        END_CODEGEN_PHASE(this, Js::PeepsPhase);

        // Layout

        BEGIN_CODEGEN_PHASE(this, Js::LayoutPhase);

        SimpleLayout layout(this);
        layout.Layout();

        END_CODEGEN_PHASE(this, Js::LayoutPhase);

        if (this->HasTry() && this->hasBailoutInEHRegion)
        {
            BEGIN_CODEGEN_PHASE(this, Js::EHBailoutPatchUpPhase);
            lowerer.EHBailoutPatchUp();
            END_CODEGEN_PHASE(this, Js::EHBailoutPatchUpPhase);
        }

        // Insert NOPs (moving this before prolog/epilog for AMD64 and possibly ARM).
        BEGIN_CODEGEN_PHASE(this, Js::InsertNOPsPhase);
        security.InsertNOPs();
        END_CODEGEN_PHASE(this, Js::InsertNOPsPhase);

        // Prolog/Epilog
        BEGIN_CODEGEN_PHASE(this, Js::PrologEpilogPhase);
        if (GetJITFunctionBody()->IsAsmJsMode())
        {
            lowerer.LowerPrologEpilogAsmJs();
        }
        else
        {
            lowerer.LowerPrologEpilog();
        }
        END_CODEGEN_PHASE(this, Js::PrologEpilogPhase);

        BEGIN_CODEGEN_PHASE(this, Js::FinalLowerPhase);
        lowerer.FinalLower();
        END_CODEGEN_PHASE(this, Js::FinalLowerPhase);

        // Encoder
        BEGIN_CODEGEN_PHASE(this, Js::EncoderPhase);

        Encoder encoder(this);
        encoder.Encode();

        END_CODEGEN_PHASE_NO_DUMP(this, Js::EncoderPhase);

#ifdef IR_VIEWER
        IRtoJSObjectBuilder::DumpIRtoGlobalObject(this, Js::EncoderPhase);
#endif /* IR_VIEWER */

    }
    END_CODEGEN_PHASE(this, Js::BackEndPhase);

#if DBG_DUMP
    if (Js::Configuration::Global.flags.IsEnabled(Js::AsmDumpModeFlag))
    {
        FILE * oldFile = 0;
        FILE * asmFile = GetScriptContext()->GetNativeCodeGenerator()->asmFile;
        if (asmFile)
        {
            oldFile = Output::SetFile(asmFile);
        }

        this->Dump(IRDumpFlags_AsmDumpMode);

        Output::Flush();

        if (asmFile)
        {
            FILE *openedFile = Output::SetFile(oldFile);
            Assert(openedFile == asmFile);
        }
    }
#endif


    auto dataAllocator = this->GetNativeCodeDataAllocator();
    if (dataAllocator->allocCount > 0)
    {
        // fill in the fixup list by scanning the memory
        // todo: this should be done while generating code
        
        NativeCodeData::DataChunk *chunk = (NativeCodeData::DataChunk*)dataAllocator->chunkList;
        NativeCodeData::DataChunk *next1 = chunk;
        while (next1)
        {
            if (next1->fixupFunc) 
            {
                next1->fixupFunc(next1->data, chunk);
            }

#if DBG
            NativeCodeData::DataChunk *next2 = chunk;
            while (next2)
            {
                for (unsigned int i = 0; i < next1->len / sizeof(void*); i++)
                {
                    if (((void**)next1->data)[i] == (void*)next2->data)
                    {
                        NativeCodeData::VerifyExistFixupEntry((void*)next2->data, &((void**)next1->data)[i], next1->data);
                        //NativeCodeData::AddFixupEntry((void*)next2->data, &((void**)next1->data)[i], next1->data, chunk);
                    }
                }
                next2 = next2->next;
            }
#endif
            next1 = next1->next;
        }
        ////
        

        JITOutputIDL* jitOutputData = m_output.GetOutputData();

        jitOutputData->nativeDataFixupTable = (NativeDataFixupTable*)midl_user_allocate(offsetof(NativeDataFixupTable, fixupRecords) + sizeof(NativeDataFixupRecord)* (dataAllocator->allocCount));
        jitOutputData->nativeDataFixupTable->count = dataAllocator->allocCount;

        jitOutputData->buffer = (NativeDataBuffer*)midl_user_allocate(offsetof(NativeDataBuffer, data) + dataAllocator->totalSize);
        jitOutputData->buffer->len = dataAllocator->totalSize;

        unsigned int len = 0;
        unsigned int count = 0;
        next1 = chunk;
        while (next1)
        {
            memcpy(jitOutputData->buffer->data + len, next1->data, next1->len);
            len += next1->len;

            jitOutputData->nativeDataFixupTable->fixupRecords[count].index = next1->allocIndex;
            jitOutputData->nativeDataFixupTable->fixupRecords[count].length = next1->len;
            jitOutputData->nativeDataFixupTable->fixupRecords[count].startOffset = next1->offset;
            jitOutputData->nativeDataFixupTable->fixupRecords[count].updateList = next1->fixupList;

            count++;
            next1 = next1->next;
        }

#if DBG
        if (PHASE_TRACE1(Js::NativeCodeDataPhase))
        {
            Output::Print(L"NativeCodeData Server Buffer: %p, len: %x, chunk head: %p\n", jitOutputData->buffer->data, jitOutputData->buffer->len, chunk);
        }
#endif
    }

}

///----------------------------------------------------------------------------
/// Func::StackAllocate
///     Allocate stack space of given size.
///----------------------------------------------------------------------------
int32
Func::StackAllocate(int size)
{
    Assert(this->IsTopFunc());

    int32 offset;

#ifdef MD_GROW_LOCALS_AREA_UP
    // Locals have positive offsets and are allocated from bottom to top.
    m_localStackHeight = Math::Align(m_localStackHeight, min(size, MachStackAlignment));

    offset = m_localStackHeight;
    m_localStackHeight += size;
#else
    // Locals have negative offsets and are allocated from top to bottom.
    m_localStackHeight += size;
    m_localStackHeight = Math::Align(m_localStackHeight, min(size, MachStackAlignment));

    offset = -m_localStackHeight;
#endif

    return offset;
}

///----------------------------------------------------------------------------
///
/// Func::StackAllocate
///
///     Allocate stack space for this symbol.
///
///----------------------------------------------------------------------------

int32
Func::StackAllocate(StackSym *stackSym, int size)
{
    Assert(size > 0);
    if (stackSym->IsArgSlotSym() || stackSym->IsParamSlotSym() || stackSym->IsAllocated())
    {
        return stackSym->m_offset;
    }
    Assert(stackSym->m_offset == 0);
    stackSym->m_allocated = true;
    stackSym->m_offset = StackAllocate(size);

    return stackSym->m_offset;
}

void
Func::SetArgOffset(StackSym *stackSym, int32 offset)
{
    AssertMsg(offset >= 0, "Why is the offset, negative?");
    stackSym->m_offset = offset;
    stackSym->m_allocated = true;
}

///
/// Ensures that local var slots are created, if the function has locals.
///     Allocate stack space for locals used for debugging
///     (for local non-temp vars we write-through memory so that locals inspection can make use of that.).
//      On stack, after local slots we allocate space for metadata (in particular, whether any the locals was changed in debugger).
///
void
Func::EnsureLocalVarSlots()
{
    Assert(IsJitInDebugMode());

    if (!this->HasLocalVarSlotCreated())
    {
        uint32 localSlotCount = GetJITFunctionBody()->GetNonTempLocalVarCount();
        if (localSlotCount && m_localVarSlotsOffset == Js::Constants::InvalidOffset)
        {
            // Allocate the slots.
            int32 size = localSlotCount * GetDiagLocalSlotSize();
            m_localVarSlotsOffset = StackAllocate(size);
            m_hasLocalVarChangedOffset = StackAllocate(max(1, MachStackAlignment)); // Can't alloc less than StackAlignment bytes.

            Assert(m_workItem->Type() == JsFunctionType);

            m_output.SetVarSlotsOffset(AdjustOffsetValue(m_localVarSlotsOffset));
            m_output.SetVarChangedOffset(AdjustOffsetValue(m_hasLocalVarChangedOffset));
        }
    }
}

void Func::SetFirstArgOffset(IR::Instr* inlineeStart)
{
    Assert(inlineeStart->m_func == this);
    Assert(!IsTopFunc());
    int32 lastOffset;

    IR::Instr* arg = inlineeStart->GetNextArg();
    const auto lastArgOutStackSym = arg->GetDst()->AsSymOpnd()->m_sym->AsStackSym();
    lastOffset = lastArgOutStackSym->m_offset;
    Assert(lastArgOutStackSym->m_isSingleDef);
    const auto secondLastArgOutOpnd = lastArgOutStackSym->m_instrDef->GetSrc2();
    if (secondLastArgOutOpnd->IsSymOpnd())
    {
        const auto secondLastOffset = secondLastArgOutOpnd->AsSymOpnd()->m_sym->AsStackSym()->m_offset;
        if (secondLastOffset > lastOffset)
        {
            lastOffset = secondLastOffset;
        }
    }
    lastOffset += MachPtr;
    int32 firstActualStackOffset = lastOffset - ((this->actualCount + Js::Constants::InlineeMetaArgCount) * MachPtr);
    Assert((this->firstActualStackOffset == -1) || (this->firstActualStackOffset == firstActualStackOffset));
    this->firstActualStackOffset = firstActualStackOffset;
}

int32
Func::GetLocalVarSlotOffset(int32 slotId)
{
    this->EnsureLocalVarSlots();
    Assert(m_localVarSlotsOffset != Js::Constants::InvalidOffset);

    int32 slotOffset = slotId * GetDiagLocalSlotSize();

    return m_localVarSlotsOffset + slotOffset;
}

void Func::OnAddSym(Sym* sym)
{
    Assert(sym);
    if (this->IsJitInDebugMode() && this->IsNonTempLocalVar(sym->m_id))
    {
        Assert(m_nonTempLocalVars);
        m_nonTempLocalVars->Set(sym->m_id);
    }
}

///
/// Returns offset of the flag (1 byte) whether any local was changed (in debugger).
/// If the function does not have any locals, returns -1.
///
int32
Func::GetHasLocalVarChangedOffset()
{
    this->EnsureLocalVarSlots();
    return m_hasLocalVarChangedOffset;
}

bool
Func::IsJitInDebugMode()
{
    return m_workItem->IsJitInDebugMode();
}

bool
Func::IsNonTempLocalVar(uint32 slotIndex)
{
    return GetJITFunctionBody()->IsNonTempLocalVar(slotIndex);
}

int32
Func::AdjustOffsetValue(int32 offset)
{
#ifdef MD_GROW_LOCALS_AREA_UP
        return -(offset + BailOutInfo::StackSymBias);
#else
        // Stack offset are negative, includes the PUSH EBP and return address
        return offset - (2 * MachPtr);
#endif
}

#ifdef MD_GROW_LOCALS_AREA_UP
// Note: this is called during jit-compile when we finalize bail out record.
void
Func::AjustLocalVarSlotOffset()
{
    if (GetJITFunctionBody()->GetNonTempLocalVarCount())
    {
        // Turn positive SP-relative base locals offset into negative frame-pointer-relative offset
        // This is changing value for restoring the locals when read due to locals inspection.

        int localsOffset = m_localVarSlotsOffset - (m_localStackHeight + m_ArgumentsOffset);
        int valueChangeOffset = m_hasLocalVarChangedOffset - (m_localStackHeight + m_ArgumentsOffset);

        Js::FunctionEntryPointInfo * entryPointInfo = static_cast<Js::FunctionEntryPointInfo*>(this->m_workItem->GetEntryPoint());
        Assert(entryPointInfo != nullptr);

        entryPointInfo->localVarSlotsOffset = localsOffset;
        entryPointInfo->localVarChangedOffset = valueChangeOffset;
    }
}
#endif

bool
Func::DoGlobOptsForGeneratorFunc() const
{
    // Disable GlobOpt optimizations for generators initially. Will visit and enable each one by one.
    return !GetJITFunctionBody()->IsGenerator();
}

bool
Func::DoSimpleJitDynamicProfile() const
{
    return IsSimpleJit() && !PHASE_OFF(Js::SimpleJitDynamicProfilePhase, GetTopFunc()) && !CONFIG_FLAG(NewSimpleJit);
}

void
Func::SetDoFastPaths()
{
    // Make sure we only call this once!
    Assert(!this->hasCalledSetDoFastPaths);

    bool doFastPaths = false;

    if(!PHASE_OFF(Js::FastPathPhase, this) && (!IsSimpleJit() || CONFIG_FLAG(NewSimpleJit)))
    {
        doFastPaths = true;
    }

    this->m_doFastPaths = doFastPaths;
#ifdef DBG
    this->hasCalledSetDoFastPaths = true;
#endif
}

#ifdef _M_ARM

RegNum
Func::GetLocalsPointer() const
{
#ifdef DBG
    if (Js::Configuration::Global.flags.IsEnabled(Js::ForceLocalsPtrFlag))
    {
        return ALT_LOCALS_PTR;
    }
#endif

    if (GetJITFunctionBody()->HasTry())
    {
        return ALT_LOCALS_PTR;
    }

    return RegSP;
}

#endif

void Func::AddSlotArrayCheck(IR::SymOpnd *fieldOpnd)
{
    if (PHASE_OFF(Js::ClosureRangeCheckPhase, this))
    {
        return;
    }

    Assert(IsTopFunc());
    if (this->slotArrayCheckTable == nullptr)
    {
        this->slotArrayCheckTable = SlotArrayCheckTable::New(m_alloc, 4);
    }

    PropertySym *propertySym = fieldOpnd->m_sym->AsPropertySym();
    uint32 slot = propertySym->m_propertyId;
    uint32 *pSlotId = this->slotArrayCheckTable->FindOrInsert(slot, propertySym->m_stackSym->m_id);

    if (pSlotId && (*pSlotId == (uint32)-1 || *pSlotId < slot))
    {
        *pSlotId = propertySym->m_propertyId;
    }
}

void Func::AddFrameDisplayCheck(IR::SymOpnd *fieldOpnd, uint32 slotId)
{
    if (PHASE_OFF(Js::ClosureRangeCheckPhase, this))
    {
        return;
    }

    Assert(IsTopFunc());
    if (this->frameDisplayCheckTable == nullptr)
    {
        this->frameDisplayCheckTable = FrameDisplayCheckTable::New(m_alloc, 4);
    }

    PropertySym *propertySym = fieldOpnd->m_sym->AsPropertySym();
    FrameDisplayCheckRecord **record = this->frameDisplayCheckTable->FindOrInsertNew(propertySym->m_stackSym->m_id);
    if (*record == nullptr)
    {
        *record = JitAnew(m_alloc, FrameDisplayCheckRecord);
    }

    uint32 frameDisplaySlot = propertySym->m_propertyId;
    if ((*record)->table == nullptr || (*record)->slotId < frameDisplaySlot)
    {
        (*record)->slotId = frameDisplaySlot;
    }

    if (slotId != (uint32)-1)
    {
        if ((*record)->table == nullptr)
        {
            (*record)->table = SlotArrayCheckTable::New(m_alloc, 4);
        }
        uint32 *pSlotId = (*record)->table->FindOrInsert(slotId, frameDisplaySlot);
        if (pSlotId && *pSlotId < slotId)
        {
            *pSlotId = slotId;
        }
    }
}

void Func::InitLocalClosureSyms()
{
    Assert(this->m_localClosureSym == nullptr);

    // Allocate stack space for closure pointers. Do this only if we're jitting for stack closures, and
    // tell bailout that these are not byte code symbols so that we don't try to encode them in the bailout record,
    // as they don't have normal lifetimes.
    Js::RegSlot regSlot = GetJITFunctionBody()->GetLocalClosureReg();
    if (regSlot != Js::Constants::NoRegister)
    {
        this->m_localClosureSym =
            StackSym::FindOrCreate(static_cast<SymID>(regSlot),
                                   this->DoStackFrameDisplay() ? (Js::RegSlot)-1 : regSlot,
                                   this);
    }

    regSlot = this->GetJITFunctionBody()->GetParamClosureReg();
    if (regSlot != Js::Constants::NoRegister)
    {
        Assert(this->GetParamClosureSym() == nullptr && !this->GetJITFunctionBody()->IsParamAndBodyScopeMerged());
        this->m_paramClosureSym =
            StackSym::FindOrCreate(static_cast<SymID>(regSlot),
                this->DoStackFrameDisplay() ? (Js::RegSlot) - 1 : regSlot,
                this);
    }

    regSlot = GetJITFunctionBody()->GetLocalFrameDisplayReg();
    if (regSlot != Js::Constants::NoRegister)
    {
        this->m_localFrameDisplaySym =
            StackSym::FindOrCreate(static_cast<SymID>(regSlot),
                                   this->DoStackFrameDisplay() ? (Js::RegSlot)-1 : regSlot,
                                   this);
    }
}

bool Func::CanAllocInPreReservedHeapPageSegment ()
{
#ifdef _CONTROL_FLOW_GUARD
    return PHASE_FORCE1(Js::PreReservedHeapAllocPhase) || (!PHASE_OFF1(Js::PreReservedHeapAllocPhase) &&
        !IsJitInDebugMode() && !GetScriptContext()->IsScriptContextInDebugMode() && GetScriptContext()->GetThreadContext()->IsCFGEnabled()
#if _M_IX86
        && m_workItem->GetJitMode() == ExecutionMode::FullJit && GetCodeGenAllocators()->canCreatePreReservedSegment);
#elif _M_X64
        && true);
#else
        && false); //Not yet implemented for architectures other than x86 and amd64.
#endif  //_M_ARCH
#else
    return false;
#endif//_CONTROL_FLOW_GUARD
}

///----------------------------------------------------------------------------
///
/// Func::GetInstrCount
///
///     Returns the number of instrs.
///     Note: It counts all instrs for now, including labels, etc.
///
///----------------------------------------------------------------------------
uint32
Func::GetInstrCount()
{
    uint instrCount = 0;

    FOREACH_INSTR_IN_FUNC(instr, this)
    {
        instrCount++;
    }NEXT_INSTR_IN_FUNC;

    return instrCount;
}

///----------------------------------------------------------------------------
///
/// Func::NumberInstrs
///
///     Number each instruction in order of appearance in the function.
///
///----------------------------------------------------------------------------
void
Func::NumberInstrs()
{
#if DBG_DUMP
    Assert(this->IsTopFunc());
    Assert(!this->hasInstrNumber);
    this->hasInstrNumber = true;
#endif
    uint instrCount = 1;

    FOREACH_INSTR_IN_FUNC(instr, this)
    {
        instr->SetNumber(instrCount++);
    }
    NEXT_INSTR_IN_FUNC;
}

///----------------------------------------------------------------------------
///
/// Func::IsInPhase
///
/// Determines whether the function is currently in the provided phase
///
///----------------------------------------------------------------------------
#if DBG
bool
Func::IsInPhase(Js::Phase tag)
{
    return this->GetTopFunc()->currentPhases.Contains(tag);
}
#endif

///----------------------------------------------------------------------------
///
/// Func::BeginPhase
///
/// Takes care of the profiler
///
///----------------------------------------------------------------------------
void
Func::BeginPhase(Js::Phase tag)
{
#ifdef DBG
    this->GetTopFunc()->currentPhases.Push(tag);
#endif

#ifdef PROFILE_EXEC
    AssertMsg((this->m_codeGenProfiler != nullptr) == Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag),
        "Profiler tag is supplied but the profiler pointer is NULL");
    if (this->m_codeGenProfiler)
    {
        this->m_codeGenProfiler->ProfileBegin(tag);
    }
#endif
}

///----------------------------------------------------------------------------
///
/// Func::EndPhase
///
/// Takes care of the profiler and dumper
///
///----------------------------------------------------------------------------
void
Func::EndProfiler(Js::Phase tag)
{
#ifdef DBG
    Assert(this->GetTopFunc()->currentPhases.Count() > 0);
    Js::Phase popped = this->GetTopFunc()->currentPhases.Pop();
    Assert(tag == popped);
#endif

#ifdef PROFILE_EXEC
    AssertMsg((this->m_codeGenProfiler != nullptr) == Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag),
        "Profiler tag is supplied but the profiler pointer is NULL");
    if (this->m_codeGenProfiler)
    {
        this->m_codeGenProfiler->ProfileEnd(tag);
    }
#endif
}

void
Func::EndPhase(Js::Phase tag, bool dump)
{
    this->EndProfiler(tag);
#if DBG_DUMP
    if(dump && (PHASE_DUMP(tag, this)
        || PHASE_DUMP(Js::BackEndPhase, this)))
    {
        Output::Print(_u("-----------------------------------------------------------------------------\n"));

        if (IsLoopBody())
        {
            Output::Print(_u("************   IR after %s (%S) Loop %d ************\n"),
                Js::PhaseNames[tag],
                ExecutionModeName(m_workItem->GetJitMode()),
                m_workItem->GetLoopNumber());
        }
        else
        {
            Output::Print(_u("************   IR after %s (%S)  ************\n"),
                Js::PhaseNames[tag],
                ExecutionModeName(m_workItem->GetJitMode()));
        }
        this->Dump(Js::Configuration::Global.flags.AsmDiff? IRDumpFlags_AsmDumpMode : IRDumpFlags_None);
    }
#endif

#if DBG
    if (tag == Js::LowererPhase)
    {
        Assert(!this->isPostLower);
        this->isPostLower = true;
    }
    else if (tag == Js::RegAllocPhase)
    {
        Assert(!this->isPostRegAlloc);
        this->isPostRegAlloc = true;
    }
    else if (tag == Js::PeepsPhase)
    {
        Assert(this->isPostLower && !this->isPostLayout);
        this->isPostPeeps = true;
    }
    else if (tag == Js::LayoutPhase)
    {
        Assert(this->isPostPeeps && !this->isPostLayout);
        this->isPostLayout = true;
    }
    else if (tag == Js::FinalLowerPhase)
    {
        Assert(this->isPostLayout && !this->isPostFinalLower);
        this->isPostFinalLower = true;
    }
    if (this->isPostLower)
    {
#ifndef _M_ARM    // Need to verify ARM is clean.
        DbCheckPostLower dbCheck(this);

        dbCheck.Check();
#endif
    }
#endif
}

Func const *
Func::GetTopFunc() const
{
    Func const * func = this;
    while (!func->IsTopFunc())
    {
        func = func->parentFunc;
    }
    return func;
}

Func *
Func::GetTopFunc()
{
    Func * func = this;
    while (!func->IsTopFunc())
    {
        func = func->parentFunc;
    }
    return func;
}

StackSym *
Func::EnsureLoopParamSym()
{
    if (this->m_loopParamSym == nullptr)
    {
        this->m_loopParamSym = StackSym::New(TyMachPtr, this);
    }
    return this->m_loopParamSym;
}

void
Func::UpdateMaxInlineeArgOutCount(uint inlineeArgOutCount)
{
    if (maxInlineeArgOutCount < inlineeArgOutCount)
    {
        maxInlineeArgOutCount = inlineeArgOutCount;
    }
}

void
Func::BeginClone(Lowerer * lowerer, JitArenaAllocator *alloc)
{
    Assert(this->IsTopFunc());
    AssertMsg(m_cloner == nullptr, "Starting new clone while one is in progress");
    m_cloner = JitAnew(alloc, Cloner, lowerer, alloc);
    if (m_cloneMap == nullptr)
    {
         m_cloneMap = JitAnew(alloc, InstrMap, alloc, 7);
    }
}

void
Func::EndClone()
{
    Assert(this->IsTopFunc());
    if (m_cloner)
    {
        m_cloner->Finish();
        JitAdelete(m_cloner->alloc, m_cloner);
        m_cloner = nullptr;
    }
}

IR::SymOpnd *
Func::GetInlineeOpndAtOffset(int32 offset)
{
    Assert(IsInlinee());

    StackSym *stackSym = CreateInlineeStackSym();
    this->SetArgOffset(stackSym, stackSym->m_offset + offset);
    Assert(stackSym->m_offset >= 0);

    return IR::SymOpnd::New(stackSym, 0, TyMachReg, this);
}

StackSym *
Func::CreateInlineeStackSym()
{
    // Make sure this is an inlinee and that GlobOpt has initialized the offset
    // in the inlinee's frame.
    Assert(IsInlinee());
    Assert(m_inlineeFrameStartSym->m_offset != -1);

    StackSym *stackSym = m_symTable->GetArgSlotSym((Js::ArgSlot)-1);
    stackSym->m_isInlinedArgSlot = true;
    stackSym->m_offset = m_inlineeFrameStartSym->m_offset;
    stackSym->m_allocated = true;

    return stackSym;
}

uint16
Func::GetArgUsedForBranch() const
{
    // this value can change while JITing, so or these together
    return GetJITFunctionBody()->GetArgUsedForBranch() | GetJITOutput()->GetArgUsedForBranch();
}

intptr_t
Func::GetJittedLoopIterationsSinceLastBailoutAddress() const
{
    Assert(this->m_workItem->Type() == JsLoopBodyWorkItemType);

    return m_workItem->GetJittedLoopIterationsSinceLastBailoutAddr();
}

intptr_t
Func::GetWeakFuncRef() const
{
    // TODO: OOP JIT figure out if this can be null

    return m_workItem->GetJITTimeInfo()->GetWeakFuncRef();
}

intptr_t
Func::GetRuntimeInlineCache(const uint index) const
{
    if(m_runtimeInfo != nullptr && m_runtimeInfo->HasClonedInlineCaches())
    {
        intptr_t inlineCache = m_runtimeInfo->GetClonedInlineCache(index);
        if(inlineCache)
        {
            return inlineCache;
        }
    }

    return GetJITFunctionBody()->GetInlineCache(index);
}

JITTimePolymorphicInlineCache *
Func::GetRuntimePolymorphicInlineCache(const uint index) const
{
    if (this->m_polymorphicInlineCacheInfo && this->m_polymorphicInlineCacheInfo->HasInlineCaches())
    {
        return this->m_polymorphicInlineCacheInfo->GetInlineCache(index);
    }
    return nullptr;
}

byte
Func::GetPolyCacheUtilToInitialize(const uint index) const
{
    return this->GetRuntimePolymorphicInlineCache(index) ? this->GetPolyCacheUtil(index) : PolymorphicInlineCacheUtilizationMinValue;
}

byte
Func::GetPolyCacheUtil(const uint index) const
{
    return this->m_polymorphicInlineCacheInfo->GetUtil(index);
}

JITObjTypeSpecFldInfo*
Func::GetObjTypeSpecFldInfo(const uint index) const
{
    if (GetJITFunctionBody()->GetInlineCacheCount() == 0)
    {
        Assert(UNREACHED);
        return nullptr;
    }

    return GetWorkItem()->GetJITTimeInfo()->GetObjTypeSpecFldInfo(index);
}

JITObjTypeSpecFldInfo*
Func::GetGlobalObjTypeSpecFldInfo(uint propertyInfoId) const
{
    Assert(propertyInfoId < GetTopFunc()->GetWorkItem()->GetJITTimeInfo()->GetGlobalObjTypeSpecFldInfoCount());
    return GetTopFunc()->m_globalObjTypeSpecFldInfoArray[propertyInfoId];
}

void
Func::EnsurePinnedTypeRefs()
{
    if (this->pinnedTypeRefs == nullptr)
    {
        this->pinnedTypeRefs = JitAnew(this->m_alloc, TypeRefSet, this->m_alloc);
    }
}

void
Func::PinTypeRef(void* typeRef)
{
    EnsurePinnedTypeRefs();
    this->pinnedTypeRefs->AddNew(typeRef);
}

void
Func::EnsureSingleTypeGuards()
{
    if (this->singleTypeGuards == nullptr)
    {
        this->singleTypeGuards = JitAnew(this->m_alloc, TypePropertyGuardDictionary, this->m_alloc);
    }
}

Js::JitTypePropertyGuard*
Func::GetOrCreateSingleTypeGuard(intptr_t typeAddr)
{
    EnsureSingleTypeGuards();

    Js::JitTypePropertyGuard* guard;
    if (!this->singleTypeGuards->TryGetValue(typeAddr, &guard))
    {
        // Property guards are allocated by NativeCodeData::Allocator so that their lifetime extends as long as the EntryPointInfo is alive.
        guard = NativeCodeDataNewNoFixup(GetNativeCodeDataAllocator(), Js::JitTypePropertyGuard, typeAddr, this->indexedPropertyGuardCount++);
        this->singleTypeGuards->Add(typeAddr, guard);
    }
    else
    {
        Assert(guard->GetTypeAddr() == typeAddr);
    }

    return guard;
}

void
Func::EnsureEquivalentTypeGuards()
{
    if (this->equivalentTypeGuards == nullptr)
    {
        this->equivalentTypeGuards = JitAnew(this->m_alloc, EquivalentTypeGuardList, this->m_alloc);
    }
}

Js::JitEquivalentTypeGuard*
Func::CreateEquivalentTypeGuard(JITTypeHolder type, uint32 objTypeSpecFldId)
{
    EnsureEquivalentTypeGuards();

    Js::JitEquivalentTypeGuard* guard = NativeCodeDataNew(GetNativeCodeDataAllocator(), Js::JitEquivalentTypeGuard, type.t->GetAddr(), this->indexedPropertyGuardCount++, objTypeSpecFldId);

    // If we want to hard code the address of the cache, we will need to go back to allocating it from the native code data allocator.
    // We would then need to maintain consistency (double write) to both the recycler allocated cache and the one on the heap.
    Js::EquivalentTypeCache* cache = nullptr;
    if (this->IsOOPJIT())
    {
        cache = JitAnewZ(this->m_alloc, Js::EquivalentTypeCache);
    }
    else
    {
        cache = NativeCodeDataNewZ(GetTransferDataAllocator(), Js::EquivalentTypeCache);
    }
    guard->SetCache(cache);

    // Give the cache a back-pointer to the guard so that the guard can be cleared at runtime if necessary.
    cache->SetGuard(guard);
    this->equivalentTypeGuards->Prepend(guard);

    return guard;
}

void
Func::EnsurePropertyGuardsByPropertyId()
{
    if (this->propertyGuardsByPropertyId == nullptr)
    {
        this->propertyGuardsByPropertyId = JitAnew(this->m_alloc, PropertyGuardByPropertyIdMap, this->m_alloc);
    }
}

void
Func::EnsureCtorCachesByPropertyId()
{
    if (this->ctorCachesByPropertyId == nullptr)
    {
        this->ctorCachesByPropertyId = JitAnew(this->m_alloc, CtorCachesByPropertyIdMap, this->m_alloc);
    }
}

void
Func::LinkGuardToPropertyId(Js::PropertyId propertyId, Js::JitIndexedPropertyGuard* guard)
{
    Assert(guard != nullptr);
    Assert(guard->GetValue() != NULL);

    Assert(this->propertyGuardsByPropertyId != nullptr);

    IndexedPropertyGuardSet* set;
    if (!this->propertyGuardsByPropertyId->TryGetValue(propertyId, &set))
    {
        set = JitAnew(this->m_alloc, IndexedPropertyGuardSet, this->m_alloc);
        this->propertyGuardsByPropertyId->Add(propertyId, set);
    }

    set->Item(guard);
}

void
Func::LinkCtorCacheToPropertyId(Js::PropertyId propertyId, JITTimeConstructorCache* cache)
{
    Assert(cache != nullptr);
    Assert(this->ctorCachesByPropertyId != nullptr);

    CtorCacheSet* set;
    if (!this->ctorCachesByPropertyId->TryGetValue(propertyId, &set))
    {
        set = JitAnew(this->m_alloc, CtorCacheSet, this->m_alloc);
        this->ctorCachesByPropertyId->Add(propertyId, set);
    }

    set->Item(cache->GetRuntimeCacheAddr());
}

JITTimeConstructorCache* Func::GetConstructorCache(const Js::ProfileId profiledCallSiteId)
{
    Assert(profiledCallSiteId < GetJITFunctionBody()->GetProfiledCallSiteCount());
    Assert(this->constructorCaches != nullptr);
    return this->constructorCaches[profiledCallSiteId];
}

void Func::SetConstructorCache(const Js::ProfileId profiledCallSiteId, JITTimeConstructorCache* constructorCache)
{
    Assert(profiledCallSiteId < GetJITFunctionBody()->GetProfiledCallSiteCount());
    Assert(constructorCache != nullptr);
    Assert(this->constructorCaches != nullptr);
    Assert(this->constructorCaches[profiledCallSiteId] == nullptr);
    this->constructorCacheCount++;
    this->constructorCaches[profiledCallSiteId] = constructorCache;
}

void Func::EnsurePropertiesWrittenTo()
{
    if (this->propertiesWrittenTo == nullptr)
    {
        this->propertiesWrittenTo = JitAnew(this->m_alloc, PropertyIdSet, this->m_alloc);
    }
}

void Func::EnsureCallSiteToArgumentsOffsetFixupMap()
{
    if (this->callSiteToArgumentsOffsetFixupMap == nullptr)
    {
        this->callSiteToArgumentsOffsetFixupMap = JitAnew(this->m_alloc, CallSiteToArgumentsOffsetFixupMap, this->m_alloc);
    }
}

IR::LabelInstr *
Func::GetFuncStartLabel()
{
    return m_funcStartLabel;
}

IR::LabelInstr *
Func::EnsureFuncStartLabel()
{
    if(m_funcStartLabel == nullptr)
    {
        m_funcStartLabel = IR::LabelInstr::New( Js::OpCode::Label, this );
    }
    return m_funcStartLabel;
}

IR::LabelInstr *
Func::GetFuncEndLabel()
{
    return m_funcEndLabel;
}

IR::LabelInstr *
Func::EnsureFuncEndLabel()
{
    if(m_funcEndLabel == nullptr)
    {
        m_funcEndLabel = IR::LabelInstr::New( Js::OpCode::Label, this );
    }
    return m_funcEndLabel;
}

void
Func::EnsureStackArgWithFormalsTracker()
{
    if (stackArgWithFormalsTracker == nullptr)
    {
        stackArgWithFormalsTracker = JitAnew(m_alloc, StackArgWithFormalsTracker, m_alloc);
    }
}

BOOL
Func::IsFormalsArraySym(SymID symId)
{
    if (stackArgWithFormalsTracker == nullptr || stackArgWithFormalsTracker->GetFormalsArraySyms() == nullptr)
    {
        return false;
    }
    return stackArgWithFormalsTracker->GetFormalsArraySyms()->Test(symId);
}

void 
Func::TrackFormalsArraySym(SymID symId)
{
    EnsureStackArgWithFormalsTracker();
    stackArgWithFormalsTracker->SetFormalsArraySyms(symId);
}

void 
Func::TrackStackSymForFormalIndex(Js::ArgSlot formalsIndex, StackSym * sym)
{
    EnsureStackArgWithFormalsTracker();
    Js::ArgSlot formalsCount = GetJITFunctionBody()->GetInParamsCount() - 1;
    stackArgWithFormalsTracker->SetStackSymInFormalsIndexMap(sym, formalsIndex, formalsCount);
}

StackSym * 
Func::GetStackSymForFormal(Js::ArgSlot formalsIndex)
{
    if (stackArgWithFormalsTracker == nullptr || stackArgWithFormalsTracker->GetFormalsIndexToStackSymMap() == nullptr)
    {
        return nullptr;
    }

    Js::ArgSlot formalsCount = GetJITFunctionBody()->GetInParamsCount() - 1;
    StackSym ** formalsIndexToStackSymMap = stackArgWithFormalsTracker->GetFormalsIndexToStackSymMap();
    AssertMsg(formalsIndex < formalsCount, "OutOfRange ? ");
    return formalsIndexToStackSymMap[formalsIndex];
}

bool
Func::HasStackSymForFormal(Js::ArgSlot formalsIndex)
{
    if (stackArgWithFormalsTracker == nullptr || stackArgWithFormalsTracker->GetFormalsIndexToStackSymMap() == nullptr)
    {
        return false;
    }
    return GetStackSymForFormal(formalsIndex) != nullptr;
}

void
Func::SetScopeObjSym(StackSym * sym)
{
    EnsureStackArgWithFormalsTracker();
    stackArgWithFormalsTracker->SetScopeObjSym(sym);
}

StackSym* 
Func::GetScopeObjSym()
{
    if (stackArgWithFormalsTracker == nullptr)
    {
        return nullptr;
    }
    return stackArgWithFormalsTracker->GetScopeObjSym();
}

BVSparse<JitArenaAllocator> * 
StackArgWithFormalsTracker::GetFormalsArraySyms()
{
    return formalsArraySyms;
}

void
StackArgWithFormalsTracker::SetFormalsArraySyms(SymID symId)
{
    if (formalsArraySyms == nullptr)
    {
        formalsArraySyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    }
    formalsArraySyms->Set(symId);
}

StackSym ** 
StackArgWithFormalsTracker::GetFormalsIndexToStackSymMap()
{
    return formalsIndexToStackSymMap;
}

void 
StackArgWithFormalsTracker::SetStackSymInFormalsIndexMap(StackSym * sym, Js::ArgSlot formalsIndex, Js::ArgSlot formalsCount)
{
    if(formalsIndexToStackSymMap == nullptr)
    {
        formalsIndexToStackSymMap = JitAnewArrayZ(alloc, StackSym*, formalsCount);
    }
    AssertMsg(formalsIndex < formalsCount, "Out of range ?");
    formalsIndexToStackSymMap[formalsIndex] = sym;
}

void 
StackArgWithFormalsTracker::SetScopeObjSym(StackSym * sym)
{
    m_scopeObjSym = sym;
}

StackSym * 
StackArgWithFormalsTracker::GetScopeObjSym()
{
    return m_scopeObjSym;
}


void
Cloner::AddInstr(IR::Instr * instrOrig, IR::Instr * instrClone)
{
    if (!this->instrFirst)
    {
        this->instrFirst = instrClone;
    }
    this->instrLast = instrClone;
}

void
Cloner::Finish()
{
    this->RetargetClonedBranches();
    if (this->lowerer)
    {
        lowerer->LowerRange(this->instrFirst, this->instrLast, false, false);
    }
}

void
Cloner::RetargetClonedBranches()
{
    if (!this->fRetargetClonedBranch)
    {
        return;
    }

    FOREACH_INSTR_IN_RANGE(instr, this->instrFirst, this->instrLast)
    {
        if (instr->IsBranchInstr())
        {
            instr->AsBranchInstr()->RetargetClonedBranch();
        }
    }
    NEXT_INSTR_IN_RANGE;
}

IR::IndirOpnd * Func::GetConstantAddressIndirOpnd(intptr_t address, IR::AddrOpndKind kind, IRType type, Js::OpCode loadOpCode)
{
    Assert(this->GetTopFunc() == this);
    if (!canHoistConstantAddressLoad)
    {
        // We can't hoist constant address load after lower, as we can't mark the sym as
        // live on back edge
        return nullptr;
    }
    int offset = 0;
    IR::RegOpnd ** foundRegOpnd = this->constantAddressRegOpnd.Find([address, &offset](IR::RegOpnd * regOpnd)
    {
        Assert(regOpnd->m_sym->IsSingleDef());
        void * curr = regOpnd->m_sym->m_instrDef->GetSrc1()->AsAddrOpnd()->m_address;
        ptrdiff_t diff = (intptr_t)address - (intptr_t)curr;
        if (!Math::FitsInDWord(diff))
        {
            return false;
        }

        offset = (int)diff;
        return true;
    });

    IR::RegOpnd * addressRegOpnd;
    if (foundRegOpnd != nullptr)
    {
        addressRegOpnd = *foundRegOpnd;
    }
    else
    {
        Assert(offset == 0);
        addressRegOpnd = IR::RegOpnd::New(TyMachPtr, this);
        IR::Instr *const newInstr =
            IR::Instr::New(
            loadOpCode,
            addressRegOpnd,
            IR::AddrOpnd::New(address, kind, this, true),
            this);
        this->constantAddressRegOpnd.Prepend(addressRegOpnd);

        IR::Instr * insertBeforeInstr = this->lastConstantAddressRegLoadInstr;
        if (insertBeforeInstr == nullptr)
        {
            insertBeforeInstr = this->GetFunctionEntryInsertionPoint();
            this->lastConstantAddressRegLoadInstr = newInstr;
        }
        insertBeforeInstr->InsertBefore(newInstr);
    }
    IR::IndirOpnd * indirOpnd =  IR::IndirOpnd::New(addressRegOpnd, offset, type, this, true);
#if DBG_DUMP
    // TODO: michhol make intptr_t
    indirOpnd->SetAddrKind(kind, (void*)address);
#endif
    return indirOpnd;
}

void Func::MarkConstantAddressSyms(BVSparse<JitArenaAllocator> * bv)
{
    Assert(this->GetTopFunc() == this);
    this->constantAddressRegOpnd.Iterate([bv](IR::RegOpnd * regOpnd)
    {
        bv->Set(regOpnd->m_sym->m_id);
    });
}

IR::Instr *
Func::GetFunctionEntryInsertionPoint()
{
    Assert(this->GetTopFunc() == this);
    IR::Instr * insertInsert = this->lastConstantAddressRegLoadInstr;
    if (insertInsert != nullptr)
    {
        return insertInsert->m_next;
    }

    insertInsert = this->m_headInstr;

    if (this->HasTry())
    {
        // Insert it inside the root region
        insertInsert = insertInsert->m_next;
        Assert(insertInsert->IsLabelInstr() && insertInsert->AsLabelInstr()->GetRegion()->GetType() == RegionTypeRoot);
    }

    return insertInsert->m_next;
}

Js::Var
Func::AllocateNumber(double value)
{
    Js::Var number = nullptr;
#if FLOATVAR
    number = Js::JavascriptNumber::NewCodeGenInstance(GetNumberAllocator(), (double)value, GetScriptContext());
#else
    if (!IsOOPJIT()) // in-proc jit
    {
        number = Js::JavascriptNumber::NewCodeGenInstance(GetNumberAllocator(), (double)value, GetScriptContext());
    }
    else // OOP JIT
    {
        GetXProcNumberAllocator()->AllocateNumber(this->GetThreadContextInfo()->GetProcessHandle(),
            value,
            (Js::StaticType*)this->GetScriptContextInfo()->GetNumberTypeStaticAddr(),
            (void*)this->GetScriptContextInfo()->GetVTableAddress(VTableValue::VtableJavascriptNumber));
    }
#endif

    return number;
}

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
void
Func::DumpFullFunctionName()
{
    wchar_t debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

    Output::Print(L"Function %s (%s)", GetJITFunctionBody()->GetDisplayName(), GetDebugNumberSet(debugStringBuffer));
}
#endif

#if DBG_DUMP
///----------------------------------------------------------------------------
///
/// Func::DumpHeader
///
///----------------------------------------------------------------------------
void
Func::DumpHeader()
{
    Output::Print(_u("-----------------------------------------------------------------------------\n"));

    DumpFullFunctionName();

    Output::SkipToColumn(50);
    Output::Print(_u("Instr Count:%d"), GetInstrCount());

    if(m_codeSize > 0)
    {
        Output::Print(_u("\t\tSize:%d\n\n"), m_codeSize);
    }
    else
    {
        Output::Print(_u("\n\n"));
    }
}

///----------------------------------------------------------------------------
///
/// Func::Dump
///
///----------------------------------------------------------------------------
void
Func::Dump(IRDumpFlags flags)
{
    this->DumpHeader();

    FOREACH_INSTR_IN_FUNC(instr, this)
    {
        instr->DumpGlobOptInstrString();
        instr->Dump(flags);
    }NEXT_INSTR_IN_FUNC;

    Output::Flush();
}

void
Func::Dump()
{
    this->Dump(IRDumpFlags_None);
}
#endif

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
LPCSTR
Func::GetVtableName(INT_PTR address)
{
#if DBG
    if (vtableMap == nullptr)
    {
        vtableMap = VirtualTableRegistry::CreateVtableHashMap(this->m_alloc);
    };
    LPCSTR name = vtableMap->Lookup(address, nullptr);
    if (name)
    {
         if (strncmp(name, "class ", _countof("class ") - 1) == 0)
         {
             name += _countof("class ") - 1;
         }
    }
    return name;
#else
    return "";
#endif
}
#endif

#if DBG_DUMP | defined(VTUNE_PROFILING)
bool Func::DoRecordNativeMap() const
{
#if defined(VTUNE_PROFILING)
    if (VTuneChakraProfile::isJitProfilingActive)
    {
        return true;
    }
#endif
#if DBG_DUMP
    return PHASE_DUMP(Js::EncoderPhase, this) && Js::Configuration::Global.flags.Verbose;
#else
    return false;
#endif
}
#endif
