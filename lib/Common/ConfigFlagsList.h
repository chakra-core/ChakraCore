//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#if defined(PHASE) || defined(PHASE_DEFAULT_ON) || defined(PHASE_DEFAULT_OFF)
#ifndef PHASE
#define PHASE(name)
#endif
#ifndef PHASE_DEFAULT_ON
#define PHASE_DEFAULT_ON PHASE
#endif
#ifndef PHASE_DEFAULT_OFF
#define PHASE_DEFAULT_OFF PHASE
#endif
PHASE(All)
    PHASE(BGJit)
    PHASE(Module)
    PHASE(LibInit)
        PHASE(JsLibInit)
    PHASE(Parse)
        PHASE(RegexCompile)
        PHASE_DEFAULT_ON(DeferParse)
        PHASE(Redeferral)
        PHASE(DeferEventHandlers)
        PHASE(FunctionSourceInfoParse)
        PHASE(StringTemplateParse)
        PHASE(CreateParserState)
        PHASE(SkipNestedDeferred)
        PHASE(CacheScopeInfoNames)
        PHASE(ScanAhead)
        PHASE_DEFAULT_OFF(ParallelParse)
        PHASE(EarlyReferenceErrors)
        PHASE(EarlyErrorOnAssignToCall)
        PHASE(BgParse)
    PHASE(ByteCode)
        PHASE(CachedScope)
        PHASE(StackFunc)
        PHASE(StackClosure)
        PHASE(DisableStackFuncOnDeferredEscape)
        PHASE(DelayCapture)
        PHASE(DebuggerScope)
        PHASE(ByteCodeSerialization)
            PHASE(VariableIntEncoding)
        PHASE(NativeCodeSerialization)
        PHASE(OptimizeBlockScope)
    PHASE(Delay)
        PHASE(Speculation)
        PHASE(GatherCodeGenData)
    PHASE_DEFAULT_ON(Wasm)
        // Wasm frontend
        PHASE(WasmBytecode) // Supports -off,-dump,-trace,-profile
        PHASE(WasmReader) // Support -trace,-profile
        PHASE(WasmSection) // Supports -trace
        PHASE(WasmOpCodeDistribution) // Support -dump
        // Wasm features per functions
        PHASE_DEFAULT_ON(WasmDeferred)
        PHASE_DEFAULT_OFF(WasmValidatePrejit)
        PHASE(WasmInOut) // Trace input and output of wasm calls
        PHASE(WasmMemWrites) // Trace memory writes
    PHASE(Asmjs)
        PHASE(AsmjsTmpRegisterAllocation)
        PHASE(AsmjsEncoder)
        PHASE(AsmjsInterpreter)
        PHASE(AsmJsJITTemplate)
        PHASE_DEFAULT_OFF(AsmjsFunctionEntry)
        PHASE(AsmjsInterpreterStack)
        PHASE(AsmjsEntryPointInfo)
        PHASE(AsmjsCallDebugBreak)
    PHASE(BackEnd)
        PHASE(IRBuilder)
            PHASE(SwitchOpt)
            PHASE(BailOnNoProfile)
            PHASE(BackendConcatExprOpt)
            PHASE(ClosureRangeCheck)
            PHASE(ClosureRegCheck)
        PHASE(Inline)
            PHASE(InlineRecursive)
            PHASE(InlineAtEveryCaller)      //Inlines a function, say, foo at every caller of foo. Doesn't guarantee all the calls within foo are inlined too.
            PHASE(InlineTree)               //Inlines every function within a top function, say, foo (which needs to be top function) Note: -force:inline achieves the effect of both -force:InlineTree & -force:InlineAtEveryCaller
            PHASE(TryAggressiveInlining)
            PHASE(InlineConstructors)
            PHASE(InlineBuiltIn)
            PHASE(InlineInJitLoopBody)
            PHASE(InlineAccessors)
            PHASE(InlineGetters)
            PHASE(InlineSetters)
            PHASE(InlineApply)
            PHASE(InlineApplyTarget)
            PHASE(InlineApplyWithoutArrayArg)
            PHASE(InlineAnyCallApplyTarget)
            PHASE(BailOutOnNotStackArgs)
            PHASE(InlineCall)
            PHASE(InlineCallTarget)
            PHASE(PartialPolymorphicInline)
            PHASE(PolymorphicInline)
            PHASE(PolymorphicInlineFixedMethods)
            PHASE(InlineOutsideLoops)
            PHASE(InlineFunctionsWithLoops)
            PHASE(EliminateArgoutForInlinee)
            PHASE(InlineBuiltInCaller)
            PHASE(InlineArgsOpt)
                PHASE(RemoveInlineFrame)
            PHASE(InlinerConstFold)
            PHASE_DEFAULT_ON(InlineCallbacks)
    PHASE(ExecBOIFastPath)
        PHASE(FGBuild)
            PHASE(OptimizeTryFinally)
            PHASE(RemoveBreakBlock)
            PHASE(TailDup)
        PHASE(FGPeeps)
        PHASE(GlobOpt)
            PHASE(PathDepBranchFolding)
            PHASE(OptimizeTryCatch)
            PHASE_DEFAULT_ON(CaptureByteCodeRegUse)
            PHASE(Backward)
                PHASE(TrackIntUsage)
                PHASE(TrackNegativeZero)
                PHASE(TypedArrayVirtual)
                PHASE(TrackIntOverflow)
                PHASE(TrackCompoundedIntOverflow)
            PHASE(Forward)
                PHASE(ValueTable)
                PHASE(ValueNumbering)
                PHASE(AVTInPrePass)
                PHASE(PathDependentValues)
                    PHASE(TrackRelativeIntBounds)
                        PHASE(BoundCheckElimination)
                            PHASE(BoundCheckHoist)
                                PHASE(LoopCountBasedBoundCheckHoist)
                PHASE(CopyProp)
                    PHASE(ObjPtrCopyProp)
                PHASE(ConstProp)
                PHASE(Int64ConstProp)
                PHASE(ConstFold)
                PHASE(CSE)
                PHASE(HoistConstInt)
                PHASE(TypeSpec)
                PHASE(AggressiveIntTypeSpec)
                PHASE(AggressiveMulIntTypeSpec)
                PHASE(LossyIntTypeSpec)
                PHASE(FloatTypeSpec)
                PHASE(StringTypeSpec)
                PHASE(InductionVars)
                PHASE(Invariants)
                PHASE(FieldCopyProp)
                PHASE(FieldPRE)
                PHASE(MakeObjSymLiveInLandingPad)
                PHASE(HostOpt)
                PHASE(ObjTypeSpec)
                    PHASE(ObjTypeSpecNewObj)
                    PHASE(ObjTypeSpecIsolatedFldOps)
                    PHASE(ObjTypeSpecIsolatedFldOpsWithBailOut)
                    PHASE(ObjTypeSpecStore)
                    PHASE(EquivObjTypeSpec)
                    PHASE(EquivObjTypeSpecByDefault)
                    PHASE(TraceObjTypeSpecTypeGuards)
                    PHASE(TraceObjTypeSpecWriteGuards)
                    PHASE(LiveOutFields)
                    PHASE(DisabledObjTypeSpec)
                    PHASE(DepolymorphizeInlinees)
                    PHASE(ReuseAuxSlotPtr)
                    PHASE(PolyEquivTypeGuard)
                    PHASE(DeadStoreTypeChecksOnStores)
                    #if DBG
                        PHASE(SimulatePolyCacheWithOneTypeForFunction)
                    #endif
                PHASE(CheckThis)
                PHASE(StackArgOpt)
                PHASE(StackArgFormalsOpt)
                PHASE(StackArgLenConstOpt)
                PHASE(IndirCopyProp)
                PHASE(ArrayCheckHoist)
                    PHASE(ArrayMissingValueCheckHoist)
                    PHASE(ArraySegmentHoist)
                        PHASE(JsArraySegmentHoist)
                    PHASE(ArrayLengthHoist)
                    PHASE(EliminateArrayAccessHelperCall)
                PHASE(NativeArray)
                    PHASE(NativeNewScArray)
                    PHASE(NativeArrayConversion)
                    PHASE(CopyOnAccessArray)
                    PHASE(NativeArrayLeafSegment)
                PHASE(TypedArrayTypeSpec)
                PHASE(LdLenIntSpec)
                PHASE(FixDataProps)
                PHASE(FixMethodProps)
                PHASE(FixAccessorProps)
                PHASE(FixDataVarProps)
                PHASE(UseFixedDataProps)
                PHASE(UseFixedDataPropsInInliner)
                PHASE(LazyBailout)
                    PHASE(LazyBailoutOnImplicitCalls)
                    PHASE(LazyFixedDataBailout)
                    PHASE(LazyFixedTypeBailout)
                PHASE(FixedMethods)
                    PHASE(FEFixedMethods)
                    PHASE(FixedFieldGuardCheck)
                    PHASE(FixedNewObj)
                        PHASE(JitAllocNewObj)
                    PHASE(FixedCtorInlining)
                    PHASE(FixedCtorCalls)
                    PHASE(FixedScriptMethodInlining)
                    PHASE(FixedScriptMethodCalls)
                    PHASE(FixedBuiltInMethodInlining)
                    PHASE(FixedBuiltInMethodCalls)
                    PHASE(SplitNewScObject)
                PHASE(OptTagChecks)
                PHASE(MemOp)
                    PHASE(MemSet)
                    PHASE(MemCopy)
                PHASE(IncrementalBailout)
            PHASE(DeadStore)
                PHASE(ReverseCopyProp)
                PHASE(MarkTemp)
                    PHASE(MarkTempNumber)
                    PHASE(MarkTempObject)
                    PHASE(MarkTempNumberOnTempObject)
                PHASE(SpeculationPropagationAnalysis)
        PHASE(DumpGlobOptInstr) // Print the Globopt instr string in post lower dumps
        PHASE(Lowerer)
            PHASE(FastPath)
                PHASE(LoopFastPath)
                PHASE(MathFastPath)
                PHASE(Atom)
                    PHASE(MulStrengthReduction)
                    PHASE(AgenPeeps)
                PHASE(BranchFastPath)
                PHASE(CallFastPath)
                PHASE(BitopsFastPath)
                PHASE(OtherFastPath)
                PHASE(ObjectFastPath)
                PHASE(ProfileBasedFldFastPath)
                PHASE(AddFldFastPath)
                PHASE(RootObjectFldFastPath)
                PHASE(ArrayLiteralFastPath)
                PHASE(ArrayCtorFastPath)
                PHASE(NewScopeSlotFastPath)
                PHASE(FrameDisplayFastPath)
                PHASE(HoistMarkTempInit)
                PHASE(HoistConstAddr)
            PHASE(JitWriteBarrier)
            PHASE(PreLowererPeeps)
            PHASE(CFGInJit)
            PHASE(TypedArray)
            PHASE(TracePinnedTypes)
        PHASE(InterruptProbe)
        PHASE(EncodeConstants)
        PHASE(RegAlloc)
            PHASE(Liveness)
                PHASE(RegParams)
            PHASE(LinearScan)
                PHASE(OpHelperRegOpt)
                PHASE(StackPack)
                PHASE(SecondChance)
                PHASE(RegionUseCount)
                PHASE(RegHoistLoads)
                PHASE(ClearRegLoopExit)
        PHASE(Peeps)
        PHASE(Layout)
        PHASE(EHBailoutPatchUp)
        PHASE(FinalLower)
        PHASE(PrologEpilog)
        PHASE(InsertNOPs)
        PHASE(Encoder)
            PHASE(Assembly)
            PHASE(Emitter)
            PHASE(DebugBreak)
#if defined(_M_IX86) || defined(_M_X64)
            PHASE(BrShorten)
                PHASE(LoopAlign)
#endif

#ifdef RECYCLER_WRITE_BARRIER
#if DBG_DUMP
    PHASE(SWB)
#endif
#endif
    PHASE(Run)
        PHASE(Interpreter)
        PHASE(EvalCompile)
            PHASE(FastIndirectEval)
        PHASE(IdleDecommit)
        PHASE(IdleCollect)
        PHASE(Marshal)
        PHASE(MemoryAllocation)
#ifdef RECYCLER_PAGE_HEAP
            PHASE(PageHeap)
#endif
            PHASE(LargeMemoryAllocation)
            PHASE(PageAllocatorAlloc)
        PHASE(Recycler)
            PHASE(ThreadCollect)
            PHASE(ExplicitFree)
            PHASE(ExpirableCollect)
            PHASE(GarbageCollect)
            PHASE(ConcurrentCollect)
                PHASE(BackgroundResetMarks)
                PHASE(BackgroundFindRoots)
                PHASE(BackgroundRescan)
                PHASE(BackgroundRepeatMark)
                PHASE(BackgroundFinishMark)
            PHASE(ConcurrentPartialCollect)
            PHASE(ParallelMark)
            PHASE(PartialCollect)
                PHASE(ResetMarks)
                PHASE(ResetWriteWatch)
                PHASE(FindRoot)
                    PHASE(FindRootArena)
                    PHASE(FindImplicitRoot)
                    PHASE(FindRootExt)
                PHASE(ScanStack)
                PHASE(ConcurrentMark)
                PHASE(ConcurrentWait)
                PHASE(Rescan)
                PHASE(Mark)
                PHASE(Sweep)
                    PHASE(SweepWeak)
                    PHASE(SweepSmall)
                    PHASE(SweepLarge)
                    PHASE(SweepPartialReuse)
                PHASE(ConcurrentSweep)
                PHASE(Finalize)
                PHASE(Dispose)
                PHASE(FinishPartial)
        PHASE(Host)
        PHASE(BailOut)
        PHASE(BailIn)
        PHASE(GeneratorGlobOpt)
        PHASE(RegexQc)
        PHASE(RegexOptBT)
        PHASE(InlineCache)
        PHASE(PolymorphicInlineCache)
        PHASE(MissingPropertyCache)
        PHASE(PropertyCache) // Trace caching of property lookups using PropertyString and JavascriptSymbol
        PHASE(CloneCacheInCollision)
        PHASE(ConstructorCache)
        PHASE(InlineCandidate)
        PHASE(ScriptFunctionWithInlineCache)
        PHASE(IsConcatSpreadableCache)
        PHASE(Arena)
        PHASE(ApplyUsage)
        PHASE(ObjectHeaderInlining)
            PHASE(ObjectHeaderInliningForConstructors)
            PHASE(ObjectHeaderInliningForObjectLiterals)
            PHASE(ObjectHeaderInliningForEmptyObjects)
        PHASE(OptUnknownElementName)
        PHASE(TypePropertyCache)
#if DBG_DUMP
        PHASE(InlineSlots)
#endif
        PHASE(DynamicProfile)
#ifdef DYNAMIC_PROFILE_STORAGE
        PHASE(DynamicProfileStorage)
#endif
        PHASE(JITLoopBody)
        PHASE(JITLoopBodyInTryCatch)
        PHASE(JITLoopBodyInTryFinally)
        PHASE(ReJIT)
        PHASE(ExecutionMode)
        PHASE(SimpleJitDynamicProfile)
        PHASE(SimpleJit)
        PHASE(FullJit)
        PHASE(FailNativeCodeInstall)
        PHASE(PixelArray)
        PHASE(Etw)
        PHASE(Profiler)
        PHASE(CustomHeap)
        PHASE(XDataAllocator)
        PHASE(PageAllocator)
        PHASE(StringConcat)
#if DBG_DUMP
        PHASE(PRNG)
#endif
        PHASE(PreReservedHeapAlloc)
        PHASE(CFG)
        PHASE(ExceptionStackTrace)
        PHASE(ExtendedExceptionInfoStackTrace)
        PHASE(TypeHandlerTransition)
        PHASE(Debugger)
            PHASE(ENC)
        PHASE(ConsoleScope)
        PHASE(ScriptProfiler)
        PHASE(JSON)
        PHASE(Intl)
        PHASE(RegexResultNotUsed)
        PHASE(Error)
        PHASE(PropertyRecord)
        PHASE(TypePathDynamicSize)
        PHASE(ObjectCopy)
        PHASE(ShareTypesWithAttributes)
        PHASE(ShareAccessorTypes)
        PHASE(ShareFuncTypes)
        PHASE(ShareCrossSiteFuncTypes)
        PHASE(ConditionalCompilation)
        PHASE(InterpreterProfile)
        PHASE(InterpreterAutoProfile)
        PHASE(ByteCodeConcatExprOpt)
        PHASE(TraceInlineCacheInvalidation)
        PHASE(TracePropertyGuards)
#ifdef ENABLE_JS_ETW
        PHASE(StackFramesEvent)
#endif
        PHASE(PerfHint)
        PHASE(TypeShareForChangePrototype)
        PHASE(DeferSourceLoad)
        PHASE(DataCache)
        PHASE(ObjectMutationBreakpoint)
        PHASE(NativeCodeData)
        PHASE(XData)
#undef PHASE
#undef PHASE_DEFAULT_ON
#undef PHASE_DEFAULT_OFF
#endif

#ifndef DEFAULT_CONFIG_BgJitDelay
#if _M_ARM
#define DEFAULT_CONFIG_BgJitDelay           (70)
#else
#define DEFAULT_CONFIG_BgJitDelay           (30)
#endif
#endif // DEFAULT_CONFIG_BgJitDelay
#define DEFAULT_CONFIG_AsmJs                (true)
#define DEFAULT_CONFIG_AsmJsEdge            (false)
#define DEFAULT_CONFIG_AsmJsStopOnError     (false)

#define DEFAULT_CONFIG_Wasm               (true)
#define DEFAULT_CONFIG_WasmI64            (false)
#if ENABLE_FAST_ARRAYBUFFER
    #define DEFAULT_CONFIG_WasmFastArray    (true)
#else
    #define DEFAULT_CONFIG_WasmFastArray    (false)
#endif
#define DEFAULT_CONFIG_WasmSharedArrayVirtualBuffer (true)
#define DEFAULT_CONFIG_WasmCheckVersion     (true)
#define DEFAULT_CONFIG_WasmAssignModuleID   (false)
#define DEFAULT_CONFIG_WasmIgnoreLimits     (false)
#define DEFAULT_CONFIG_WasmFold             (true)
#define DEFAULT_CONFIG_WasmMathExFilter     (false)
#define DEFAULT_CONFIG_WasmIgnoreResponse   (false)
#define DEFAULT_CONFIG_WasmMaxTableSize     (10000000)
#define DEFAULT_CONFIG_WasmThreads          (false)
#define DEFAULT_CONFIG_WasmMultiValue       (false)
#define DEFAULT_CONFIG_WasmSignExtends      (true)
#define DEFAULT_CONFIG_WasmNontrapping      (true)
#define DEFAULT_CONFIG_WasmExperimental     (false)
#define DEFAULT_CONFIG_BgParse              (false)
#define DEFAULT_CONFIG_BgJitDelayFgBuffer   (0)
#define DEFAULT_CONFIG_BgJitPendingFuncCap  (31)
#define DEFAULT_CONFIG_CurrentSourceInfo    (true)
#define DEFAULT_CONFIG_CreateFunctionProxy  (true)
#define DEFAULT_CONFIG_HybridFgJit          (false)
#define DEFAULT_CONFIG_HybridFgJitBgQueueLengthThreshold (32)
#define DEFAULT_CONFIG_Prejit               (false)
#define DEFAULT_CONFIG_ParserStateCache     (true)
#define DEFAULT_CONFIG_CompressParserStateCache (false)
#define DEFAULT_CONFIG_DeferTopLevelTillFirstCall (true)
#define DEFAULT_CONFIG_DirectCallTelemetryStats (false)
#define DEFAULT_CONFIG_errorStackTrace      (true)
#define DEFAULT_CONFIG_FastLineColumnCalculation (true)
#define DEFAULT_CONFIG_PrintLineColumnInfo (false)
#define DEFAULT_CONFIG_ForceDecommitOnCollect (false)
#define DEFAULT_CONFIG_ForceDeferParse      (false)
#define DEFAULT_CONFIG_NoDeferParse         (false)
#define DEFAULT_CONFIG_ForceDynamicProfile  (false)
#define DEFAULT_CONFIG_ForceExpireOnNonCacheCollect (false)
#define DEFAULT_CONFIG_ForceFastPath        (false)
#define DEFAULT_CONFIG_ForceJITLoopBody     (false)
#define DEFAULT_CONFIG_ForceStaticInterpreterThunk (false)
#define DEFAULT_CONFIG_ForceCleanPropertyOnCollect (false)
#define DEFAULT_CONFIG_ForceCleanCacheOnCollect (false)
#define DEFAULT_CONFIG_ForceGCAfterJSONParse (false)
#define DEFAULT_CONFIG_ForceSerialized      (false)
#define DEFAULT_CONFIG_ForceES5Array        (false)
#define DEFAULT_CONFIG_ForceAsmJsLinkFail   (false)
#define DEFAULT_CONFIG_StrongArraySort      (false)
#define DEFAULT_CONFIG_DumpCommentsFromReferencedFiles (false)
#define DEFAULT_CONFIG_ExtendedErrorStackForTestHost (false)
#define DEFAULT_CONFIG_ForceSplitScope      (false)
#define DEFAULT_CONFIG_DelayFullJITSmallFunc (0)
#define DEFAULT_CONFIG_EnableFatalErrorOnOOM (true)
#define DEFAULT_CONFIG_RedeferralCap         (3)

//Following determines inline thresholds
#define DEFAULT_CONFIG_InlineThreshold      (35)            //Default start
#define DEFAULT_CONFIG_AggressiveInlineThreshold  (80)      //Limit for aggressive inlining.
#define DEFAULT_CONFIG_InlineThresholdAdjustCountInLargeFunction  (20)
#define DEFAULT_CONFIG_InlineThresholdAdjustCountInMediumSizedFunction  (6)
#define DEFAULT_CONFIG_InlineThresholdAdjustCountInSmallFunction  (10)
#define DEFAULT_CONFIG_ConstructorInlineThreshold (21)      //Monomorphic constructor threshold
#define DEFAULT_CONFIG_AsmJsInlineAdjust (35)                // wasm functions are cheaper to inline, so worth being more aggressive
#define DEFAULT_CONFIG_ConstructorCallsRequiredToFinalizeCachedType (2)
#define DEFAULT_CONFIG_OutsideLoopInlineThreshold (16)      //Threshold to inline outside loops
#define DEFAULT_CONFIG_LeafInlineThreshold  (60)            //Inlinee threshold for function which is leaf (irrespective of it has loops or not)
#define DEFAULT_CONFIG_LoopInlineThreshold  (25)            //Inlinee threshold for function with loops
#define DEFAULT_CONFIG_PolymorphicInlineThreshold  (35)     //Polymorphic inline threshold
#define DEFAULT_CONFIG_InlineCountMax       (1200)          //Max sum of bytecodes of inlinees inlined into a function (excluding built-ins)
#define DEFAULT_CONFIG_InlineCountMaxInLoopBodies (500)     // Max sum of bytecodes of inlinees that can be inlined into a jitted loop body (excluding built-ins)
#define DEFAULT_CONFIG_AggressiveInlineCountMax       (8000)          //Max sum of bytecodes of inlinees inlined into a function (excluding built-ins) when inlined aggressively
#define DEFAULT_CONFIG_MaxFuncInlineDepth   (2)             //Maximum number of times a function can be inlined within a top function
#define DEFAULT_CONFIG_MaxNumberOfInlineesWithLoop   (40) //Inlinee with a loop is controlled by LoopInlineThreshold, though we don't want to inline lot of inlinees with loop, this ensures a limit.
#define DEFAULT_CONFIG_ConstantArgumentInlineThreshold   (157)      // Bytecode threshold for functions with constant arguments which are used for branching
#define DEFAULT_CONFIG_RecursiveInlineThreshold     (2000)      // Bytecode threshold recursive call at a call site
#define DEFAULT_CONFIG_RecursiveInlineDepthMax      (8)      // Maximum inline depth for recursive calls
#define DEFAULT_CONFIG_RecursiveInlineDepthMin      (2)      // Minimum inline depth for recursive call
#define DEFAULT_CONFIG_InlineInLoopBodyScaleDownFactor    (4)
#define DEFAULT_CONFIG_PropertyCacheMissPenalty (10)
#define DEFAULT_CONFIG_PropertyCacheMissThreshold (-100)
#define DEFAULT_CONFIG_PropertyCacheMissReset (-5000)

#define DEFAULT_CONFIG_CloneInlinedPolymorphicCaches (true)
#define DEFAULT_CONFIG_HighPrecisionDate    (false)
#define DEFAULT_CONFIG_ForceOldDateAPI      (false)
#define DEFAULT_CONFIG_Loop                 (1)
#define DEFAULT_CONFIG_ForceDiagnosticsMode (false)
#define DEFAULT_CONFIG_UseFullName          (true)
#define DEFAULT_CONFIG_EnableContinueAfterExceptionWrappersForHelpers  (true)
#define DEFAULT_CONFIG_EnableContinueAfterExceptionWrappersForBuiltIns  (true)
#define DEFAULT_CONFIG_EnableFunctionSourceReportForHeapEnum (true)
#define DEFAULT_CONFIG_LoopInterpretCount   (150)
#define DEFAULT_CONFIG_LoopProfileIterations (25)
#define DEFAULT_CONFIG_JitLoopBodyHotLoopThreshold (20000)
#define DEFAULT_CONFIG_LoopBodySizeThresholdToDisableOpts (255)

#define DEFAULT_CONFIG_MaxJitThreadCount        (2)
#define DEFAULT_CONFIG_ForceMaxJitThreadCount   (false)

#ifdef ENABLE_SPECTRE_RUNTIME_MITIGATIONS
#define DEFAULT_CONFIG_MitigateSpectre (true)

#define DEFAULT_CONFIG_AddMaskingBlocks (true)

#define DEFAULT_CONFIG_PoisonVarArrayLoad (true)
#define DEFAULT_CONFIG_PoisonIntArrayLoad (true)
#define DEFAULT_CONFIG_PoisonFloatArrayLoad (true)
#define DEFAULT_CONFIG_PoisonTypedArrayLoad (true)
#define DEFAULT_CONFIG_PoisonStringLoad (true)
#define DEFAULT_CONFIG_PoisonObjectsForLoads (true)

#define DEFAULT_CONFIG_PoisonVarArrayStore (true)
#define DEFAULT_CONFIG_PoisonIntArrayStore (true)
#define DEFAULT_CONFIG_PoisonFloatArrayStore (true)
#define DEFAULT_CONFIG_PoisonTypedArrayStore (true)
#define DEFAULT_CONFIG_PoisonStringStore (true)
#define DEFAULT_CONFIG_PoisonObjectsForStores (true)
#else
#define DEFAULT_CONFIG_MitigateSpectre (false)

#define DEFAULT_CONFIG_AddMaskingBlocks (false)

#define DEFAULT_CONFIG_PoisonVarArrayLoad (false)
#define DEFAULT_CONFIG_PoisonIntArrayLoad (false)
#define DEFAULT_CONFIG_PoisonFloatArrayLoad (false)
#define DEFAULT_CONFIG_PoisonTypedArrayLoad (false)
#define DEFAULT_CONFIG_PoisonStringLoad (false)
#define DEFAULT_CONFIG_PoisonObjectsForLoads (false)

#define DEFAULT_CONFIG_PoisonVarArrayStore (false)
#define DEFAULT_CONFIG_PoisonIntArrayStore (false)
#define DEFAULT_CONFIG_PoisonFloatArrayStore (false)
#define DEFAULT_CONFIG_PoisonTypedArrayStore (false)
#define DEFAULT_CONFIG_PoisonStringStore (false)
#define DEFAULT_CONFIG_PoisonObjectsForStores (false)
#endif

#ifdef RECYCLER_PAGE_HEAP
#define DEFAULT_CONFIG_PageHeap             ((Js::Number) PageHeapMode::PageHeapModeOff)
#define DEFAULT_CONFIG_PageHeapAllocStack   (false)
#define DEFAULT_CONFIG_PageHeapFreeStack    (false)
#define DEFAULT_CONFIG_PageHeapBlockType    ((Js::Number) PageHeapBlockTypeFilter::PageHeapBlockTypeFilterAll)
#endif

#define DEFAULT_CONFIG_LowMemoryCap         (0xB900000) // 185 MB - based on memory cap for process on low-capacity device
#define DEFAULT_CONFIG_NewPagesCapDuringBGSweeping    (15000 * 4)
#define DEFAULT_CONFIG_MaxSingleAllocSizeInMB  (2048)
#define DEFAULT_CONFIG_AllocationPolicyLimit    (-1)

#define DEFAULT_CONFIG_MaxCodeFill          (500)
#define DEFAULT_CONFIG_MaxLoopsPerFunction  (10)
#define DEFAULT_CONFIG_NopFrequency         (8)
#define DEFAULT_CONFIG_SpeculationCap       (1)         // Needs to be 1 and not 0 since the compiler complains about a condition being always false
#define DEFAULT_CONFIG_ProfileBasedSpeculationCap (1600)
#define DEFAULT_CONFIG_Verbose              (false)
#define DEFAULT_CONFIG_ForceStrictMode      (false)
#define DEFAULT_CONFIG_EnableEvalMapCleanup (true)
#define DEFAULT_CONFIG_ExpirableCollectionGCCount (5)  // Number of GCs during which entry point profiling occurs
#define DEFAULT_CONFIG_ExpirableCollectionTriggerThreshold (50)  // Threshold at which Entry Point Collection is triggered
#define DEFAULT_CONFIG_RegexTracing         (false)
#define DEFAULT_CONFIG_RegexProfile         (false)
#define DEFAULT_CONFIG_RegexDebug           (false)
#define DEFAULT_CONFIG_RegexDebugAST        (true)
#define DEFAULT_CONFIG_RegexDebugAnnotatedAST (true)
#define DEFAULT_CONFIG_RegexBytecodeDebug   (false)
#define DEFAULT_CONFIG_RegexOptimize        (true)
#define DEFAULT_CONFIG_DynamicRegexMruListSize (16)
#define DEFAULT_CONFIG_GoptCleanupThreshold  (25)
#define DEFAULT_CONFIG_AsmGoptCleanupThreshold  (500)
#define DEFAULT_CONFIG_OptimizeForManyInstances (false)
#define DEFAULT_CONFIG_EnableArrayTypeMutation (false)

#define DEFAULT_CONFIG_DeferParseThreshold             (4 * 1024) // Unit is number of characters
#define DEFAULT_CONFIG_ProfileBasedDeferParseThreshold (100)      // Unit is number of characters

#define DEFAULT_CONFIG_ProfileBasedSpeculativeJit (true)
#define DEFAULT_CONFIG_WininetProfileCache        (true)
#define DEFAULT_CONFIG_MinProfileCacheSize        (5)   // Minimum number of functions before profile is saved.
#define DEFAULT_CONFIG_ProfileDifferencePercent   (15)  // If 15% of the functions have different profile we will trigger a save.

#define DEFAULT_CONFIG_Intl                    (true)
#define DEFAULT_CONFIG_IntlBuiltIns            (true)
#define DEFAULT_CONFIG_IntlPlatform            (false) // Makes the EngineExtension.Intl object visible to user code as Intl.platform, meant for testing

#ifdef ENABLE_JS_BUILTINS
    #define DEFAULT_CONFIG_JsBuiltIn             (true)
#else
    #define DEFAULT_CONFIG_JsBuiltIn             (false)
#endif
#define DEFAULT_CONFIG_JitRepro                (false)
#define DEFAULT_CONFIG_LdChakraLib             (false)
#define DEFAULT_CONFIG_TestChakraLib           (false)
#define DEFAULT_CONFIG_EntryPointInfoRpcData   (false)

// ES6 DEFAULT BEHAVIOR
#define DEFAULT_CONFIG_ES6                     (true)  // master flag to gate all P0-spec-test compliant ES6 features

//CollectGarbage is legacy IE specific global function disabled in Microsoft Edge.
#define DEFAULT_CONFIG_CollectGarbage          (false)

// ES6 sub-feature gate - to enable-disable ES6 sub-feature when ES6 flag is enabled
#define DEFAULT_CONFIG_ES6DateParseFix         (true)
#define DEFAULT_CONFIG_ES6FunctionNameFull     (true)
#define DEFAULT_CONFIG_ES6Generators           (true)
#define DEFAULT_CONFIG_ES6IsConcatSpreadable   (true)
#define DEFAULT_CONFIG_ES6Math                 (true)
#ifdef COMPILE_DISABLE_ES6Module
    // If ES6Module needs to be disabled by compile flag, DEFAULT_CONFIG_ES6Module should be false
    #define DEFAULT_CONFIG_ES6Module               (false)
#else
    #define DEFAULT_CONFIG_ES6Module               (true)
#endif
#define DEFAULT_CONFIG_ES6Object               (true)
#define DEFAULT_CONFIG_ES6Number               (true)
#define DEFAULT_CONFIG_ES6ObjectLiterals       (true)
#define DEFAULT_CONFIG_ES6Proxy                (true)
#define DEFAULT_CONFIG_ES6Rest                 (true)
#define DEFAULT_CONFIG_ES6Spread               (true)
#define DEFAULT_CONFIG_ES6String               (true)
#define DEFAULT_CONFIG_ES6StringPrototypeFixes (true)
#define DEFAULT_CONFIG_ES2018ObjectRestSpread  (true)

#ifndef DEFAULT_CONFIG_ES6PrototypeChain
#ifdef COMPILE_DISABLE_ES6PrototypeChain
    // If ES6PrototypeChain needs to be disabled by compile flag, DEFAULT_CONFIG_ES6PrototypeChain should be false
    #define DEFAULT_CONFIG_ES6PrototypeChain       (false)
#else
    #define DEFAULT_CONFIG_ES6PrototypeChain       (true)
#endif
#endif

#define DEFAULT_CONFIG_ES6ToPrimitive          (true)
#define DEFAULT_CONFIG_ES6ToLength             (true)
#define DEFAULT_CONFIG_ES6ToStringTag          (true)
#define DEFAULT_CONFIG_ES6Unicode              (true)
#define DEFAULT_CONFIG_ES6UnicodeVerbose       (true)
#define DEFAULT_CONFIG_ES6Unscopables          (true)
#define DEFAULT_CONFIG_ES6RegExSticky          (true)
#define DEFAULT_CONFIG_ES2018RegExDotAll       (true)
#define DEFAULT_CONFIG_ESBigInt                (false)
#define DEFAULT_CONFIG_ESNumericSeparator      (true)
#define DEFAULT_CONFIG_ESHashbang              (true)
#define DEFAULT_CONFIG_ESSymbolDescription     (true)
#define DEFAULT_CONFIG_ESArrayFindFromLast     (true)
#define DEFAULT_CONFIG_ESPromiseAny            (true)
#define DEFAULT_CONFIG_ESNullishCoalescingOperator (true)
#define DEFAULT_CONFIG_ESOptionalChaining      (true)
#define DEFAULT_CONFIG_ESGlobalThis            (true)

// Jitting generator functions is not functional on ARM
// Also still contains significant bugs on x86/x64 hence disabled
#ifdef _M_ARM32_OR_ARM64
    #define DEFAULT_CONFIG_JitES6Generators            (false)
#else
    #define DEFAULT_CONFIG_JitES6Generators            (false)
#endif

#ifdef COMPILE_DISABLE_ES6RegExPrototypeProperties
    // If ES6RegExPrototypeProperties needs to be disabled by compile flag, DEFAULT_CONFIG_ES6RegExPrototypeProperties should be false
    #define DEFAULT_CONFIG_ES6RegExPrototypeProperties (false)
#else
    #define DEFAULT_CONFIG_ES6RegExPrototypeProperties (false)
#endif
#ifdef COMPILE_DISABLE_ES6RegExSymbols
    // If ES6RegExSymbols needs to be disabled by compile flag, DEFAULT_CONFIG_ES6RegExSymbols should be false
    #define DEFAULT_CONFIG_ES6RegExSymbols         (false)
#else
    #define DEFAULT_CONFIG_ES6RegExSymbols         (false)
#endif
#define DEFAULT_CONFIG_ES7AsyncAwait           (true)
#define DEFAULT_CONFIG_ES7ExponentionOperator  (true)
#define DEFAULT_CONFIG_ES7TrailingComma        (true)
#define DEFAULT_CONFIG_ES7ValuesEntries        (true)
#define DEFAULT_CONFIG_ESObjectGetOwnPropertyDescriptors (true)
#define DEFAULT_CONFIG_ESDynamicImport         (true)
#define DEFAULT_CONFIG_ESImportMeta            (true)
#define DEFAULT_CONFIG_ESExportNsAs            (true)
#define DEFAULT_CONFIG_ES2018AsyncIteration    (true)
#define DEFAULT_CONFIG_ESTopLevelAwait         (true)

#define DEFAULT_CONFIG_ESSharedArrayBuffer     (false)

#define DEFAULT_CONFIG_ES6Verbose              (false)
#define DEFAULT_CONFIG_ES6All                  (false)
// ES6 DEFAULT BEHAVIOR

#define DEFAULT_CONFIG_AsyncDebugging           (true)
#define DEFAULT_CONFIG_TraceAsyncDebugCalls     (false)
#define DEFAULT_CONFIG_ForcePostLowerGlobOptInstrString (false)
#define DEFAULT_CONFIG_EnumerateSpecialPropertiesInDebugger (true)

#define DEFAULT_CONFIG_MaxJITFunctionBytecodeByteLength (4800000)
#define DEFAULT_CONFIG_MaxJITFunctionBytecodeCount (120000)

#define DEFAULT_CONFIG_JitQueueThreshold      (6)

#define DEFAULT_CONFIG_FullJitRequeueThreshold (25)     // Minimum number of times a function needs to be executed before it is re-added to the jit queue

#define DEFAULT_CONFIG_MinTemplatizedJitRunCount      (100)     // Minimum number of times a function needs to be interpreted before it is jitted
#define DEFAULT_CONFIG_MinAsmJsInterpreterRunCount      (10)     // Minimum number of times a function needs to be Asm interpreted before it is jitted
#define DEFAULT_CONFIG_MinTemplatizedJitLoopRunCount      (500)     // Minimum number of times a function needs to be interpreted before it is jitted
#define DEFAULT_CONFIG_MaxTemplatizedJitRunCount      (-1)     // Maximum number of times a function can be TJ before it is jitted
#define DEFAULT_CONFIG_MaxAsmJsInterpreterRunCount      (-1)     // Maximum number of times a function can be Asm interpreted before it is jitted

// Note: The following defaults only apply when the NewSimpleJit is on. The defaults for when it's off are computed in
// ConfigFlagsTable::TranslateFlagConfiguration.
#define DEFAULT_CONFIG_AutoProfilingInterpreter0Limit (12)
#define DEFAULT_CONFIG_ProfilingInterpreter0Limit (4)
#define DEFAULT_CONFIG_AutoProfilingInterpreter1Limit (0)
#define DEFAULT_CONFIG_SimpleJitLimit (132)
#define DEFAULT_CONFIG_ProfilingInterpreter1Limit (12)

// These are used to compute the above defaults for when NewSimpleJit is off
#define DEFAULT_CONFIG_AutoProfilingInterpreterLimit_OldSimpleJit (80)
#define DEFAULT_CONFIG_SimpleJitLimit_OldSimpleJit (25)

#define DEFAULT_CONFIG_MinProfileIterations (16)
#define DEFAULT_CONFIG_MinProfileIterations_OldSimpleJit (25)
#define DEFAULT_CONFIG_MinSimpleJitIterations (16)
#define DEFAULT_CONFIG_NewSimpleJit (false)

#define DEFAULT_CONFIG_MaxLinearIntCaseCount     (3)       // Maximum number of cases (in switch statement) for which instructions can be generated linearly.
#define DEFAULT_CONFIG_MaxSingleCharStrJumpTableRatio  (2)       // Maximum single char string jump table size as multiples of the actual case arm
#define DEFAULT_CONFIG_MaxSingleCharStrJumpTableSize  (128)       // Maximum single char string jump table size
#define DEFAULT_CONFIG_MinSwitchJumpTableSize   (9)     // Minimum number of case target entries in the jump table(this may also include values that are missing in the consecutive set of integer case arms)
#define DEFAULT_CONFIG_SwitchOptHolesThreshold  (50)     // Maximum percentage of holes (missing case values in a switch statement) with which a jump table can be created
#define DEFAULT_CONFIG_MaxLinearStringCaseCount (4)     // Maximum number of String cases (in switch statement) for which instructions can be generated linearly.

#define DEFAULT_CONFIG_MinDeferredFuncTokenCount (20)   // Minimum size in tokens of a defer-parsed function

#if DBG
#define DEFAULT_CONFIG_SkipFuncCountForBailOnNoProfile (0) //Initial Number of functions in a func body to be skipped from forcibly inserting BailOnNoProfile.
#endif
#define DEFAULT_CONFIG_BailOnNoProfileLimit    200      // The limit of bailout on no profile info before triggering a rejit
#define DEFAULT_CONFIG_BailOnNoProfileRejitLimit (50)   // The limit of bailout on no profile info before disable all the no profile bailouts
#define DEFAULT_CONFIG_CallsToBailoutsRatioForRejit 10   // Ratio of function calls to bailouts above which a rejit is considered
#define DEFAULT_CONFIG_LoopIterationsToBailoutsRatioForRejit 50 // Ratio of loop iteration count to bailouts above which a rejit of the loop body is considered
#define DEFAULT_CONFIG_MinBailOutsBeforeRejit 2         // Minimum number of bailouts for a single bailout record after which a rejit is considered
#define DEFAULT_CONFIG_MinBailOutsBeforeRejitForLoops 2         // Minimum number of bailouts for a single bailout record after which a rejit is considered
#define DEFAULT_CONFIG_RejitMaxBailOutCount 500         // Maximum number of bailouts for a single bailout record after which rejit is forced.

#if DBG
#define DEFAULT_CONFIG_ValidateIntRanges (false)
#endif

#define DEFAULT_CONFIG_Sse                  (-1)

#define DEFAULT_CONFIG_DeletedPropertyReuseThreshold (32)
#define DEFAULT_CONFIG_BigDictionaryTypeHandlerThreshold (0xffff)
#define DEFAULT_CONFIG_ForceStringKeyedSimpleDictionaryTypeHandler (false)
#define DEFAULT_CONFIG_TypeSnapshotEnumeration (true)
#define DEFAULT_CONFIG_ConcurrentRuntime (false)
#define DEFAULT_CONFIG_PrimeRecycler     (false)
#if defined(_WIN32)
#define DEFAULT_CONFIG_PrivateHeap       (true)
#else // defined(_WIN32)
// Don't use PrivateHeap on xplat where we statically link and override new/delete
#define DEFAULT_CONFIG_PrivateHeap       (false)
#endif // defined(_WIN32)
#define DEFAULT_CONFIG_DisableRentalThreading (false)
#define DEFAULT_CONFIG_DisableDebugObject (false)
#define DEFAULT_CONFIG_DumpHeap (false)
#define DEFAULT_CONFIG_PerfHintLevel (1)
#define DEFAULT_CONFIG_OOPJITMissingOpts (false)
#define DEFAULT_CONFIG_OOPCFGRegistration (true)
#define DEFAULT_CONFIG_CrashOnOOPJITFailure (false)
#define DEFAULT_CONFIG_ForceJITCFGCheck (false)
#define DEFAULT_CONFIG_UseJITTrampoline (true)

#define DEFAULT_CONFIG_IsolatePrototypes    (true)
#define DEFAULT_CONFIG_ChangeTypeOnProto    (true)
#define DEFAULT_CONFIG_FixPropsOnPathTypes    (true)
#define DEFAULT_CONFIG_BailoutTraceFilter (-1)
#define DEFAULT_CONFIG_TempMin    (0)
#define DEFAULT_CONFIG_TempMax    (INT_MAX)

#define DEFAULT_CONFIG_LibraryStackFrame            (true)
#define DEFAULT_CONFIG_LibraryStackFrameDebugger    (false)

#define DEFAULT_CONFIG_FuncObjectInlineCacheThreshold   (2) // Maximum number of inline caches a function body may have to allow for inline caches to be allocated on the function object.
#define DEFAULT_CONFIG_ShareInlineCaches (false)
#define DEFAULT_CONFIG_InlineCacheInvalidationListCompactionThreshold (4)
#define DEFAULT_CONFIG_ConstructorCacheInvalidationThreshold (500)

#define DEFAULT_CONFIG_InMemoryTrace                (false)
#define DEFAULT_CONFIG_InMemoryTraceBufferSize      (1024)
#define DEFAULT_CONFIG_RichTraceFormat              (false)
#define DEFAULT_CONFIG_TraceWithStack               (false)

#define DEFAULT_CONFIG_InjectPartiallyInitializedInterpreterFrameError (0)
#define DEFAULT_CONFIG_InjectPartiallyInitializedInterpreterFrameErrorType (0)

#define DEFAULT_CONFIG_DeferLoadingAvailableSource  (false)

#define DEFAULT_CONFIG_RecyclerForceMarkInterior (false)

#define DEFAULT_CONFIG_MemProtectHeap (false)

#define DEFAULT_CONFIG_InduceCodeGenFailure (30) // When -InduceCodeGenFailure is passed in, 30% of JIT allocations will fail

#define DEFAULT_CONFIG_SkipSplitWhenResultIgnored (false)

#define DEFAULT_CONFIG_MinMemOpCount (16U)

#if ENABLE_COPYONACCESS_ARRAY
#define DEFAULT_CONFIG_MaxCopyOnAccessArrayLength (32U)
#define DEFAULT_CONFIG_MinCopyOnAccessArrayLength (5U)
#define DEFAULT_CONFIG_CopyOnAccessArraySegmentCacheSize (16U)
#endif

#if defined(_M_IX86) || defined(_M_X64)
#define DEFAULT_CONFIG_LoopAlignNopLimit (6)
#endif

#if defined(_M_IX86) || defined(_M_X64)
#define DEFAULT_CONFIG_ZeroMemoryWithNonTemporalStore (true)
#endif

#define DEFAULT_CONFIG_StrictWriteBarrierCheck  (false)
#define DEFAULT_CONFIG_KeepRecyclerTrackData  (false)
#define DEFAULT_CONFIG_EnableBGFreeZero (true)

#if !GLOBAL_ENABLE_WRITE_BARRIER
#define DEFAULT_CONFIG_ForceSoftwareWriteBarrier  (false)
#else
#define DEFAULT_CONFIG_ForceSoftwareWriteBarrier  (true)
#endif
#define DEFAULT_CONFIG_WriteBarrierTest (false)
#define DEFAULT_CONFIG_VerifyBarrierBit  (false)

#define TraceLevel_Error        (1)
#define TraceLevel_Warning      (2)
#define TraceLevel_Info         (3)

#define TEMP_ENABLE_FLAG_FOR_APPX_BETA_ONLY 1

#define INMEMORY_CACHE_MAX_URL                    (5)             // This is the max number of URLs that the in-memory profile cache can hold.
#define INMEMORY_CACHE_MAX_PROFILE_MANAGER        (50)            // This is the max number of dynamic scripts that the in-memory profile cache can have

#ifdef SUPPORT_INTRUSIVE_TESTTRACES
#define INTRUSIVE_TESTTRACE_PolymorphicInlineCache (1)
#endif

//
//FLAG(type, name, description, defaultValue)
//
//  types:
//      String
//      Phases
//      Number
//      Boolean
//
// If the default value is not required it should be left empty
// For Phases, there is no default value. it should always be empty
// Default values for stings must be prefixed with 'L'. See AsmDumpMode
// Scroll till the extreme right to see the default values

#if defined(FLAG) || defined(FLAG_EXPERIMENTAL) || defined(FLAG_REGOVR_EXP)

#ifndef FLAG
#define FLAG(...)
#endif

#ifndef FLAG_EXPERIMENTAL
#define FLAG_EXPERIMENTAL FLAG
#endif

#ifndef FLAG_REGOVR_EXP
#define FLAG_REGOVR_EXP FLAG_EXPERIMENTAL
#endif

// NON-RELEASE FLAGS
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS

// Regular FLAG
#define FLAGNR(Type, Name, String, Default)                 FLAG(Type, Name, String, Default, NoParent, FALSE)

// Regular flag with acronym
#ifndef FLAGNRA
    #define FLAGNRA(Type, Name, Acronym, String, Default) \
        FLAGNR(Type, Name, String, Default) \
        FLAGNR(Type, Acronym, String, Default)
#endif

// Child FLAG with PARENT FLAG
#define FLAGPNR(Type, ParentName, Name, String, Default)    FLAG(Type, Name, String, Default, ParentName, FALSE)

// Regular FLAG with callback function
#define FLAGNRC(Type, Name, String, Default)                FLAG(Type, Name, String, Default, NoParent, TRUE)

#else
#define FLAGNR(Type, Name, String, Default)

#ifdef FLAGNRA
    #undef FLAGNRA
#endif
#define FLAGNRA(Type, Name, Acronym, String, Default)

#define FLAGPNR(Type, ParentName, Name, String, Default)
#define FLAGNRC(Type, Name, String, Default)
#endif

// RELEASE FLAGS
#define FLAGPR(Type, ParentName, Name, String, Default)     FLAG(Type, Name, String, Default, ParentName, FALSE)
#define FLAGR(Type, Name, String, Default)                  FLAG(Type, Name, String, Default, NoParent, FALSE)

// Release flags with parent and acronym
#ifndef FLAGPRA
#define FLAGPRA(Type, ParentName, Name, Acronym, String, Default) \
        FLAG_REGOVR_EXP(Type, Name, String, Default, ParentName, FALSE) \
        FLAGNR(Type, Acronym, String, Default)
#endif

// RELEASE Flags enabled by Edge experimental flag
#define FLAGPR_EXPERIMENTAL_WASM(Type, Name, String)    FLAG_EXPERIMENTAL(Type, Name, String, true, WasmExperimental, FALSE)
// RELEASE FLAGS WITH REGISTRY OVERRIDE AND EDGE
#define FLAGPR_REGOVR_EXP(Type, ParentName, Name, String, Default)   FLAG_REGOVR_EXP(Type, Name, String, Default, ParentName, FALSE)

// Release flag with non-release acronym
#ifndef FLAGRA
    #define FLAGRA(Type, Name, Acronym, String, Default) \
        FLAGR(Type, Name, String, Default) \
        FLAGNR(Type, Acronym, String, Default)
#endif

// Please keep this list alphabetically sorted

#if DBG
FLAGNR(Boolean, ArrayValidate         , "Validate each array for valid elements (default: false)", false)
FLAGNR(Boolean, MemOpMissingValueValidate, "Validate Missing Value Tracking on memset/memcopy", false)
FLAGNR(Boolean, OOPJITFixupValidate, "Validate that all entries in fixup list are allocated as NativeCodeData and that all NativeCodeData gets fixed up", false)
#endif
#ifdef ARENA_MEMORY_VERIFY
FLAGNR(Boolean, ArenaNoFreeList       , "Do not free list in arena", false)
FLAGNR(Boolean, ArenaNoPageReuse      , "Do not reuse page in arena", false)
FLAGNR(Boolean, ArenaUseHeapAlloc     , "Arena use heap to allocate memory instead of page allocator", false)
#endif
FLAGNR(Boolean, ValidateInlineStack, "Does a stack walk on helper calls to validate inline stack is correctly restored", false)
FLAGNR(Boolean, AsmDiff               , "Dump the IR without memory locations and varying parameters.", false)
FLAGNR(String,  AsmDumpMode           , "Dump the final assembly to a file without memory locations and varying parameters\n\t\t\t\t\tThe 'filename' is the file where the assembly will be dumped. Dump to console if no file is specified", nullptr)
FLAGNR(Boolean, AsmJs                 , "Enable Asmjs", DEFAULT_CONFIG_AsmJs)
FLAGNR(Boolean, AsmJsStopOnError      , "Stop execution on any AsmJs validation errors", DEFAULT_CONFIG_AsmJsStopOnError)
FLAGNR(Boolean, AsmJsEdge             , "Enable asm.js features which may have backward incompatible changes or not validate on old demos", DEFAULT_CONFIG_AsmJsEdge)
FLAGNR(Boolean, Wasm                  , "Enable WebAssembly", DEFAULT_CONFIG_Wasm)
FLAGNR(Boolean, WasmI64               , "Enable Int64 testing for WebAssembly. ArgIns can be [number,string,{low:number,high:number}]. Return values will be {low:number,high:number}", DEFAULT_CONFIG_WasmI64)
FLAGNR(Boolean, WasmFastArray         , "Enable fast array implementation for WebAssembly", DEFAULT_CONFIG_WasmFastArray)
FLAGNR(Boolean, WasmSharedArrayVirtualBuffer, "Use Virtual allocation for WebAssemblySharedArrayBuffer (Windows only)", DEFAULT_CONFIG_WasmSharedArrayVirtualBuffer)
FLAGNR(Boolean, WasmMathExFilter      , "Enable Math exception filter for WebAssembly", DEFAULT_CONFIG_WasmMathExFilter)
FLAGNR(Boolean, WasmCheckVersion      , "Check the binary version for WebAssembly", DEFAULT_CONFIG_WasmCheckVersion)
FLAGNR(Boolean, WasmAssignModuleID    , "Assign an individual ID for WebAssembly module", DEFAULT_CONFIG_WasmAssignModuleID)
FLAGNR(Boolean, WasmIgnoreLimits      , "Ignore the WebAssembly binary limits ", DEFAULT_CONFIG_WasmIgnoreLimits)
FLAGNR(Boolean, WasmFold              , "Enable i32/i64 const folding", DEFAULT_CONFIG_WasmFold)
FLAGNR(Boolean, WasmIgnoreResponse    , "Ignore the type of the Response object", DEFAULT_CONFIG_WasmIgnoreResponse)
FLAGNR(Number,  WasmMaxTableSize      , "Maximum size allowed to the WebAssembly.Table", DEFAULT_CONFIG_WasmMaxTableSize)
FLAGNR(Boolean, WasmThreads           , "Enable WebAssembly threads feature", DEFAULT_CONFIG_WasmThreads)
FLAGNR(Boolean, WasmMultiValue        , "Use new WebAssembly multi-value", DEFAULT_CONFIG_WasmMultiValue)
FLAGNR(Boolean, WasmSignExtends       , "Use new WebAssembly sign extension operators", DEFAULT_CONFIG_WasmSignExtends)
FLAGNR(Boolean, WasmNontrapping, "Enable non-trapping float-to-int conversions in WebAssembly", DEFAULT_CONFIG_WasmNontrapping)

// WebAssembly Experimental Features
// Master WasmExperimental flag to activate WebAssembly experimental features
FLAGR(Boolean, WasmExperimental, "Enable WebAssembly experimental features", DEFAULT_CONFIG_WasmExperimental)

// The default value of the experimental features will be off because the parent is off
// Turning on the parent causes the child flag to take on their default value (aka on)
// In Edge, we manually turn on the individual child flags
// Not having the DEFAULT_CONFIG_XXXX macro ensures we use CONFIG_FLAG_RELEASE instead of CONFIG_FLAG
FLAGPR_EXPERIMENTAL_WASM(Boolean, WasmSimd        , "Enable SIMD in WebAssembly")

FLAGNR(Boolean, AssertBreak           , "Debug break on assert", false)
FLAGNR(Boolean, AssertPopUp           , "Pop up asserts (default: false)", false)
FLAGNR(Boolean, AssertIgnore          , "Ignores asserts if set", false)
FLAGNR(Boolean, AsyncDebugging, "Enable async debugging feature (default: false)", DEFAULT_CONFIG_AsyncDebugging)
FLAGNR(Number,  BailOnNoProfileLimit,   "The limit of bailout on no profile info before triggering a rejit", DEFAULT_CONFIG_BailOnNoProfileLimit)
FLAGNR(Number,  BailOnNoProfileRejitLimit, "The limit of bailout on no profile info before we disable the bailouts", DEFAULT_CONFIG_BailOnNoProfileRejitLimit)
FLAGNR(Boolean, BaselineMode          , "Dump only stable content that can be used for baseline comparison", false)
FLAGNR(String,  DumpOnCrash           , "generate heap dump on asserts or unhandled exception if set", nullptr)
FLAGNR(String,  FullMemoryDump        , "Will perform a full memory dump when -DumpOnCrash is supplied.", nullptr)
#ifdef BAILOUT_INJECTION
FLAGR (NumberPairSet, BailOut         , "Source location to insert BailOut", )
FLAGNR(Boolean, BailOutAtEveryLine    , "Inserts BailOut at every line of source (default: false)", false)
FLAGNR(Boolean, BailOutAtEveryByteCode, "Inserts BailOut at every Byte code (default: false)", false)
FLAGNR(Boolean, BailOutAtEveryImplicitCall, "Force generating implicit call bailout even when we don't need it", false)
FLAGR (NumberSet, BailOutByteCode     , "Byte code location to insert BailOut. Use with -prejit only", )
#endif
FLAGNR(Boolean, Benchmark             , "Disable security code which introduce variability in benchmarks", false)
FLAGR (Boolean, BgJit                 , "Background JIT. Disable to force heuristic-based foreground JITting. (default: true)", true)
FLAGR (Boolean, BgParse               , "Background Parse. Disable to force all parsing to occur on UI thread. (default: true)", DEFAULT_CONFIG_BgParse)
FLAGNR(Number,  BgJitDelay            , "Delay to wait for speculative jitting before starting script execution", DEFAULT_CONFIG_BgJitDelay)
FLAGNR(Number,  BgJitDelayFgBuffer    , "When speculatively jitting in the foreground thread, do so for (BgJitDelay - BgJitDelayBuffer) milliseconds", DEFAULT_CONFIG_BgJitDelayFgBuffer)
FLAGNR(Number,  BgJitPendingFuncCap   , "Disable delay if pending function count larger then cap", DEFAULT_CONFIG_BgJitPendingFuncCap)

FLAGNR(Boolean, CreateFunctionProxy   , "Create function proxies instead of full function bodies", DEFAULT_CONFIG_CreateFunctionProxy)
FLAGNR(Boolean, HybridFgJit           , "When background JIT is enabled, enable jitting in the foreground based on heuristics. This flag is only effective when OptimizeForManyInstances is disabled (UI threads).", DEFAULT_CONFIG_HybridFgJit)
FLAGNR(Number,  HybridFgJitBgQueueLengthThreshold, "The background job queue length must exceed this threshold to consider jitting in the foreground", DEFAULT_CONFIG_HybridFgJitBgQueueLengthThreshold)
FLAGNR(Boolean, BytecodeHist          , "Provide a histogram of the bytecodes run by the script. (NoNative required).", false)
FLAGNR(Boolean, CurrentSourceInfo     , "Enable IASD get current script source info", DEFAULT_CONFIG_CurrentSourceInfo)
FLAGNR(Boolean, CFGLog                , "Log CFG checks", false)
FLAGNR(Boolean, CheckAlignment        , "Insert checks in the native code to verify 8-byte alignment of stack", false)
FLAGNR(Boolean, CheckEmitBufferPermissions, "Check JIT code buffers at commit and decommit time to ensure no PAGE_EXECUTE_READWRITE pages.", false)
#ifdef CHECK_MEMORY_LEAK
FLAGR (Boolean, CheckMemoryLeak       , "Check for heap memory leak", false)
FLAGR (String,  DumpOnLeak            , "Create a dump on failed memory leak check", nullptr)
#endif
FLAGNR(Boolean, CheckOpHelpers        , "Verify opHelper labels in the JIT are set properly", false)
FLAGNR(Boolean, CloneInlinedPolymorphicCaches, "Clones polymorphic inline caches in inlined functions", DEFAULT_CONFIG_CloneInlinedPolymorphicCaches)
FLAGNR(Boolean, ConcurrentRuntime     , "Enable Concurrent GC and background JIT when creating runtime", DEFAULT_CONFIG_ConcurrentRuntime)
#if CONFIG_CONSOLE_AVAILABLE
FLAGNR(Boolean, Console               , "Create console window in GUI app", false)
FLAGNR(Boolean, ConsoleExitPause      , "Pause on exit when a console window is created in GUI app", false)
#endif
FLAGNR(Number,  ConstructorInlineThreshold      , "Maximum size in bytecodes of a constructor inline candidate with monomorphic field access", DEFAULT_CONFIG_ConstructorInlineThreshold)
FLAGNR(Number,  ConstructorCallsRequiredToFinalizeCachedType, "Number of calls to a constructor required before the type cached in the constructor cache is finalized", DEFAULT_CONFIG_ConstructorCallsRequiredToFinalizeCachedType)
FLAGNR(Number,  PropertyCacheMissPenalty, "Number of string or symbol cache hits per miss needed to be worth using cache", DEFAULT_CONFIG_PropertyCacheMissPenalty)
FLAGNR(Number,  PropertyCacheMissThreshold, "Point at which we disable string or symbol property cache", DEFAULT_CONFIG_PropertyCacheMissThreshold)
FLAGNR(Number,  PropertyCacheMissReset, "Point at which we try to start using string or symbol cache after giving up", DEFAULT_CONFIG_PropertyCacheMissReset)
#ifdef SECURITY_TESTING
FLAGNR(Boolean, CrashOnException      , "Removes the top-level exception handler, allowing jc.exe to crash on an unhandled exception.  No effect on IE. (default: false)", false)
#endif
FLAGNR(Boolean, Debug                 , "Disable phases (layout, security code, etc) which makes JIT output harder to debug", false)
FLAGNR(NumberSet,  DebugBreak         , "Index of the function where you want to break", )
FLAGNR(NumberTrioSet,  StatementDebugBreak, "Index of the statement where you want to break", )
FLAGNR(Phases,  DebugBreakOnPhaseBegin, "Break into debugger at the beginning of given phase for listed function", )

FLAGNR(Boolean, DebugWindow           , "Send console output to debugger window", false)
FLAGNR(Boolean, ParserStateCache      , "Enable creation of parser state cache", DEFAULT_CONFIG_ParserStateCache)
FLAGNR(Boolean, CompressParserStateCache, "Enable compression of the parser state cache", DEFAULT_CONFIG_CompressParserStateCache)
FLAGNR(Boolean, DeferTopLevelTillFirstCall      , "Enable tracking of deferred top level functions in a script file, until the first function of the script context is parsed.", DEFAULT_CONFIG_DeferTopLevelTillFirstCall)
FLAGNR(Number,  DeferParse            , "Minimum size of defer-parsed script (non-zero only: use /nodeferparse do disable", 0)
FLAGNR(Boolean, DirectCallTelemetryStats, "Enables logging stats for direct call telemetry", DEFAULT_CONFIG_DirectCallTelemetryStats)
FLAGNR(Boolean, DisableArrayBTree     , "Disable creation of BTree for Arrays", false)
FLAGNR(Boolean, DisableRentalThreading, "Disable rental threading when creating runtime", DEFAULT_CONFIG_DisableRentalThreading)
FLAGNR(Boolean, DisableVTuneSourceLineInfo, "Disable VTune Source line info for Dynamic JITted code", false)
FLAGNR(Boolean, DisplayMemStats, "Display memory usage statistics", false)
FLAGNR(Phases,  Dump                  , "What All to dump", )
#ifdef DUMP_FRAGMENTATION_STATS
FLAGR (Boolean, DumpFragmentationStats, "Dump bucket state after every GC", false)
#endif
FLAGNR(Boolean, DumpIRAddresses,   "Print addresses in IR dumps", false)
FLAGNR(Boolean, DumpLineNoInColor, "Print the source code in high intensity color for better readability", false)
#ifdef RECYCLER_DUMP_OBJECT_GRAPH
FLAGR (Boolean, DumpObjectGraphOnExit , "Dump object graph on recycler destructor", false)
FLAGR (Boolean, DumpObjectGraphOnCollect, "Dump object graph on recycler destructor", false)
#endif
FLAGNR(Boolean, DumpEvalStringOnRemoval, "Dumps an eval string when its being removed from the eval map", false)
FLAGNR(Boolean, DumpObjectGraphOnEnum, "Dump object graph on recycler heap enumeration", false)
#ifdef DYNAMIC_PROFILE_STORAGE
FLAGNRA(String, DynamicProfileCache   , Dpc, "File to cache dynamic profile information", nullptr)
FLAGNR(String,  DynamicProfileCacheDir, "Directory to cache dynamic profile information", nullptr)
FLAGNRA(String, DynamicProfileInput   , Dpi, "Read only file containing dynamic profile information", nullptr)
#endif
#ifdef EDIT_AND_CONTINUE
FLAGNR(Boolean, EditTest              , "Enable edit and continue test tools", false)
#endif
FLAGNR(Boolean, WininetProfileCache, "Use the WININET cache to save the profile information", DEFAULT_CONFIG_WininetProfileCache)
FLAGNR(Boolean, NoDynamicProfileInMemoryCache, "Enable in-memory cache for dynamic sources", false)
FLAGNR(Boolean, ProfileBasedSpeculativeJit, "Enable dynamic profile based speculative JIT", DEFAULT_CONFIG_ProfileBasedSpeculativeJit)
FLAGNR(Number,  ProfileBasedSpeculationCap, "In the presence of dynamic profile speculative JIT is capped to this many bytecode instructions", DEFAULT_CONFIG_ProfileBasedSpeculationCap)
#ifdef DYNAMIC_PROFILE_MUTATOR
FLAGNR(String,  DynamicProfileMutatorDll , "Path of the mutator DLL", _u("DynamicProfileMutatorImpl.dll"))
FLAGNR(String,  DynamicProfileMutator , "Type of local, temp, return, param, loop implicit flag and implicit flag. \n\t\t\t\t\ti.e local=LikelyArray_NoMissingValues_NonInts_NonFloats;temp=Int8Array;param=LikelyNumber;return=LikelyString;loopimplicitflag=ImplicitCall_ToPrimitive;implicitflag=ImplicitCall_None\n\t\t\t\t\tor pass DynamicProfileMutator:random\n\t\t\t\t\tSee DynamicProfileInfo.h for enum values", nullptr)
#endif
FLAGNR(Boolean, ExecuteByteCodeBufferReturnsInvalidByteCode, "Serialized byte code execution always returns SCRIPT_E_INVALID_BYTECODE", false)
FLAGR(Number, ExpirableCollectionGCCount, "Number of GCs during which Expirable object profiling occurs", DEFAULT_CONFIG_ExpirableCollectionGCCount)
FLAGR (Number,  ExpirableCollectionTriggerThreshold, "Threshold at which Expirable Object Collection is triggered (In Percentage)", DEFAULT_CONFIG_ExpirableCollectionTriggerThreshold)
FLAGR(Boolean, SkipSplitOnNoResult, "If the result of Regex split isn't used, skip executing the regex. (Perf optimization)", DEFAULT_CONFIG_SkipSplitWhenResultIgnored)
#ifdef TEST_ETW_EVENTS
FLAGNR(String,  TestEtwDll            , "Path of the TestEtwEventSink DLL", nullptr)
#endif
#ifdef ENABLE_TEST_HOOKS
FLAGNR(Boolean, Force32BitByteCode, "Force CC to generate 32bit bytecode intended only for regenerating bytecode headers.", false)
#endif

FLAGNR(Boolean, CollectGarbage        , "Enable CollectGarbage API", DEFAULT_CONFIG_CollectGarbage)

FLAGR (Boolean, Intl                  , "Intl object support", DEFAULT_CONFIG_Intl)
FLAGNR(Boolean, IntlBuiltIns          , "Intl built-in function support", DEFAULT_CONFIG_IntlBuiltIns)
FLAGNR(Boolean, IntlPlatform          , "Make the internal Intl native helper object visible to user code", DEFAULT_CONFIG_IntlPlatform)

FLAGNR(Boolean, JsBuiltIn             , "JS Built-in function support", DEFAULT_CONFIG_JsBuiltIn)
FLAGNR(Boolean, JitRepro              , "Add Function.invokeJit to execute codegen on an encoded rpc buffer", DEFAULT_CONFIG_JitRepro)
FLAGNR(Boolean, EntryPointInfoRpcData , "Keep encoded rpc buffer for jitted function on EntryPointInfo until cleanup", DEFAULT_CONFIG_EntryPointInfoRpcData)

FLAGNR(Boolean, LdChakraLib           , "Access to the Chakra internal library with the __chakraLibrary keyword", DEFAULT_CONFIG_LdChakraLib)
FLAGNR(Boolean, TestChakraLib         , "Access to the Chakra internal library with the __chakraLibrary keyword without global access restriction", DEFAULT_CONFIG_TestChakraLib)

// ES6 (BLUE+1) features/flags

// Master ES6 flag to enable STABLE ES6 features/flags
FLAGR(Boolean, ES6                         , "Enable ES6 stable features",                        DEFAULT_CONFIG_ES6)

// Master ES6 flag to enable ALL sub ES6 features/flags
FLAGNRC(Boolean, ES6All                    , "Enable all ES6 features, both stable and unstable", DEFAULT_CONFIG_ES6All)

// Master ES6 flag to enable Threshold ES6 features/flags
FLAGNRC(Boolean, ES6Experimental           , "Enable all experimental features",                  DEFAULT_CONFIG_ES6All)

// Per ES6 feature/flag

FLAGPR           (Boolean, ES6, ES7AsyncAwait          , "Enable ES7 'async' and 'await' keywords"                  , DEFAULT_CONFIG_ES7AsyncAwait)
FLAGPR           (Boolean, ES6, ES6DateParseFix        , "Enable ES6 Date.parse fixes"                              , DEFAULT_CONFIG_ES6DateParseFix)
FLAGPR           (Boolean, ES6, ES6FunctionNameFull    , "Enable ES6 Full function.name"                            , DEFAULT_CONFIG_ES6FunctionNameFull)
FLAGPR           (Boolean, ES6, ES6Generators          , "Enable ES6 generators"                                    , DEFAULT_CONFIG_ES6Generators)
FLAGPR           (Boolean, ES6, ES7ExponentiationOperator, "Enable ES7 exponentiation operator (**)"                , DEFAULT_CONFIG_ES7ExponentionOperator)

FLAGPR           (Boolean, ES6, ES7ValuesEntries       , "Enable ES7 Object.values and Object.entries"              , DEFAULT_CONFIG_ES7ValuesEntries)
FLAGPR           (Boolean, ES6, ES7TrailingComma       , "Enable ES7 trailing comma in function"                    , DEFAULT_CONFIG_ES7TrailingComma)
FLAGPR           (Boolean, ES6, ES6IsConcatSpreadable  , "Enable ES6 isConcatSpreadable Symbol"                     , DEFAULT_CONFIG_ES6IsConcatSpreadable)
FLAGPR           (Boolean, ES6, ES6Math                , "Enable ES6 Math extensions"                               , DEFAULT_CONFIG_ES6Math)

#ifndef COMPILE_DISABLE_ESDynamicImport
#define COMPILE_DISABLE_ESDynamicImport 0
#endif
FLAGPR_REGOVR_EXP(Boolean, ES6, ESDynamicImport        , "Enable dynamic import"                                    , DEFAULT_CONFIG_ESDynamicImport)

FLAGPR           (Boolean, ES6, ES6Module              , "Enable ES6 Modules"                                       , DEFAULT_CONFIG_ES6Module)
FLAGPR           (Boolean, ES6, ES6Object              , "Enable ES6 Object extensions"                             , DEFAULT_CONFIG_ES6Object)
FLAGPR           (Boolean, ES6, ES6Number              , "Enable ES6 Number extensions"                             , DEFAULT_CONFIG_ES6Number)
FLAGPR           (Boolean, ES6, ES6ObjectLiterals      , "Enable ES6 Object literal extensions"                     , DEFAULT_CONFIG_ES6ObjectLiterals)
FLAGPR           (Boolean, ES6, ES6Proxy               , "Enable ES6 Proxy feature"                                 , DEFAULT_CONFIG_ES6Proxy)
FLAGPR           (Boolean, ES6, ES6Rest                , "Enable ES6 Rest parameters"                               , DEFAULT_CONFIG_ES6Rest)
FLAGPR           (Boolean, ES6, ES6Spread              , "Enable ES6 Spread support"                                , DEFAULT_CONFIG_ES6Spread)
FLAGPR           (Boolean, ES6, ES6String              , "Enable ES6 String extensions"                             , DEFAULT_CONFIG_ES6String)
FLAGPR           (Boolean, ES6, ES6StringPrototypeFixes, "Enable ES6 String.prototype fixes"                        , DEFAULT_CONFIG_ES6StringPrototypeFixes)
FLAGPR           (Boolean, ES6, ES2018ObjectRestSpread , "Enable ES2018 Object Rest/Spread"                         , DEFAULT_CONFIG_ES2018ObjectRestSpread)

FLAGPR           (Boolean, ES6, ES6PrototypeChain      , "Enable ES6 prototypes (Example: Date prototype is object)", DEFAULT_CONFIG_ES6PrototypeChain)
FLAGPR           (Boolean, ES6, ES6ToPrimitive         , "Enable ES6 ToPrimitive symbol"                            , DEFAULT_CONFIG_ES6ToPrimitive)
FLAGPR           (Boolean, ES6, ES6ToLength            , "Enable ES6 ToLength fixes"                                , DEFAULT_CONFIG_ES6ToLength)
FLAGPR           (Boolean, ES6, ES6ToStringTag         , "Enable ES6 ToStringTag symbol"                            , DEFAULT_CONFIG_ES6ToStringTag)
FLAGPR           (Boolean, ES6, ES6Unicode             , "Enable ES6 Unicode 6.0 extensions"                        , DEFAULT_CONFIG_ES6Unicode)
FLAGPR           (Boolean, ES6, ES6UnicodeVerbose      , "Enable ES6 Unicode 6.0 verbose failure output"            , DEFAULT_CONFIG_ES6UnicodeVerbose)
FLAGPR           (Boolean, ES6, ES6Unscopables         , "Enable ES6 With Statement Unscopables"                    , DEFAULT_CONFIG_ES6Unscopables)
FLAGPR           (Boolean, ES6, ES6RegExSticky         , "Enable ES6 RegEx sticky flag"                             , DEFAULT_CONFIG_ES6RegExSticky)
FLAGPR           (Boolean, ES6, ES2018RegExDotAll      , "Enable ES2018 RegEx dotAll flag"                          , DEFAULT_CONFIG_ES2018RegExDotAll)
FLAGPR           (Boolean, ES6, ESExportNsAs           , "Enable ES experimental export * as name"                  , DEFAULT_CONFIG_ESExportNsAs)
FLAGPR           (Boolean, ES6, ES2018AsyncIteration   , "Enable ES2018 Async Iteration"                            , DEFAULT_CONFIG_ES2018AsyncIteration)
FLAGPR           (Boolean, ES6, ESTopLevelAwait        , "Enable Top Level Await in modules"                        , DEFAULT_CONFIG_ESTopLevelAwait)

#ifndef COMPILE_DISABLE_ES6RegExPrototypeProperties
    #define COMPILE_DISABLE_ES6RegExPrototypeProperties 0
#endif
FLAGPR_REGOVR_EXP(Boolean, ES6, ES6RegExPrototypeProperties, "Enable ES6 properties on the RegEx prototype"         , DEFAULT_CONFIG_ES6RegExPrototypeProperties)

#ifndef COMPILE_DISABLE_ES6RegExSymbols
    #define COMPILE_DISABLE_ES6RegExSymbols 0
#endif

// When we enable ES6RegExSymbols check all String and Regex built-ins which are inlined in JIT and make sure the helper
// sets implicit call flag before calling into script
// Also, the corresponding helpers in JnHelperMethodList.h should be marked as being reentrant
FLAGPR_REGOVR_EXP(Boolean, ES6, ES6RegExSymbols        , "Enable ES6 RegExp symbols"                                , DEFAULT_CONFIG_ES6RegExSymbols)

FLAGPR           (Boolean, ES6, ES6Verbose             , "Enable ES6 verbose trace"                                 , DEFAULT_CONFIG_ES6Verbose)
FLAGPR           (Boolean, ES6, ESObjectGetOwnPropertyDescriptors, "Enable Object.getOwnPropertyDescriptors"        , DEFAULT_CONFIG_ESObjectGetOwnPropertyDescriptors)

#ifndef COMPILE_DISABLE_ESSharedArrayBuffer
    #define COMPILE_DISABLE_ESSharedArrayBuffer 0
#endif

FLAGPR_REGOVR_EXP(Boolean, ES6, ESSharedArrayBuffer    , "Enable SharedArrayBuffer"                                 , DEFAULT_CONFIG_ESSharedArrayBuffer)

// Newer language feature flags

// ES BigInt flag
FLAGR(Boolean, ESBigInt, "Enable ESBigInt flag", DEFAULT_CONFIG_ESBigInt)

// ES Numeric Separator support for numeric constants
FLAGR(Boolean, ESNumericSeparator, "Enable Numeric Separator flag", DEFAULT_CONFIG_ESNumericSeparator)

// ES Nullish coalescing operator support (??)
FLAGR(Boolean, ESNullishCoalescingOperator, "Enable nullish coalescing operator", DEFAULT_CONFIG_ESNullishCoalescingOperator)

// ES Optional chaining operator support (?.)
FLAGR(Boolean, ESOptionalChaining, "Enable optional chaining operator", DEFAULT_CONFIG_ESOptionalChaining)

// ES Hashbang support for interpreter directive syntax
FLAGR(Boolean, ESHashbang, "Enable Hashbang syntax", DEFAULT_CONFIG_ESHashbang)

// ES Symbol.prototype.description flag
FLAGR(Boolean, ESSymbolDescription, "Enable Symbol.prototype.description", DEFAULT_CONFIG_ESSymbolDescription)

FLAGR(Boolean, ESArrayFindFromLast, "Enable findLast, findLastIndex for Array.prototype and TypedArray.prorotype", DEFAULT_CONFIG_ESArrayFindFromLast)

// ES Promise.any and AggregateError flag
FLAGR(Boolean, ESPromiseAny, "Enable Promise.any and AggregateError", DEFAULT_CONFIG_ESPromiseAny)

// ES import.meta keyword meta-property
FLAGR(Boolean, ESImportMeta, "Enable import.meta keyword", DEFAULT_CONFIG_ESImportMeta)

//ES globalThis flag
FLAGR(Boolean, ESGlobalThis, "Enable globalThis", DEFAULT_CONFIG_ESGlobalThis)

// This flag to be removed once JITing generator functions is stable
FLAGNR(Boolean, JitES6Generators        , "Enable JITing of ES6 generators", DEFAULT_CONFIG_JitES6Generators)

FLAGNR(Boolean, FastLineColumnCalculation, "Enable fast calculation of line/column numbers from the source.", DEFAULT_CONFIG_FastLineColumnCalculation)
FLAGR (String,  Filename              , "Jscript source file", nullptr)
FLAGNR(Boolean, FreeRejittedCode      , "Free rejitted code", true)
FLAGNR(Boolean, ForceGuardPages       , "Force the addition of guard pages", false)
FLAGNR(Boolean, PrintGuardPageBounds  , "Prints the bounds of a guard page", false)
FLAGNR(Boolean, ForceLegacyEngine     , "Force a jscript9 dll load", false)
FLAGNR(Phases,  Force                 , "Force certain phase to run ignoring heuristics", )
FLAGNR(Phases,  Stress                , "Stress certain phases by making them kick in even if they normally would not.", )
FLAGNR(Boolean, ForceArrayBTree       , "Force enable creation of BTree for Arrays", false)
FLAGNR(Boolean, StrongArraySort       , "Add secondary comparisons to the default array sort comparator to disambiguate sorts of equal-toString'd objects.", DEFAULT_CONFIG_StrongArraySort)
FLAGNR(Boolean, ForceCleanPropertyOnCollect, "Force cleaning of property on collection", DEFAULT_CONFIG_ForceCleanPropertyOnCollect)
FLAGNR(Boolean, ForceCleanCacheOnCollect, "Force cleaning of dynamic caches on collection", DEFAULT_CONFIG_ForceCleanCacheOnCollect)
FLAGNR(Boolean, ForceGCAfterJSONParse, "Force GC to happen after JSON parsing", DEFAULT_CONFIG_ForceGCAfterJSONParse)
FLAGNR(Boolean, ForceDecommitOnCollect, "Force decommit collect", DEFAULT_CONFIG_ForceDecommitOnCollect)
FLAGNR(Boolean, ForceDeferParse       , "Defer parsing of all function bodies", DEFAULT_CONFIG_ForceDeferParse)
FLAGNR(Boolean, ForceDiagnosticsMode  , "Enable diagnostics mode and debug interpreter loop", false)
FLAGNR(Boolean, ForceGetWriteWatchOOM , "Force GetWriteWatch to go into OOM codepath in HeapBlockMap rescan", false)
FLAGNR(Boolean, ForcePostLowerGlobOptInstrString, "Force tracking of globopt instr string post lower", DEFAULT_CONFIG_ForcePostLowerGlobOptInstrString)
FLAGNR(Boolean, ForceSplitScope       , "All functions will have unmerged body and param scopes", DEFAULT_CONFIG_ForceSplitScope)
FLAGNR(Boolean, EnumerateSpecialPropertiesInDebugger, "Enable enumeration of special debug properties", DEFAULT_CONFIG_EnumerateSpecialPropertiesInDebugger)
FLAGNR(Boolean, EnableContinueAfterExceptionWrappersForHelpers, "Enable wrapper over helper methods in debugger, Fast F12 only", DEFAULT_CONFIG_EnableContinueAfterExceptionWrappersForHelpers)
FLAGNR(Boolean, EnableContinueAfterExceptionWrappersForBuiltIns, "Enable wrapper over library calls in debugger, Fast F12 only", DEFAULT_CONFIG_EnableContinueAfterExceptionWrappersForBuiltIns)
FLAGNR(Boolean, EnableFunctionSourceReportForHeapEnum, "During HeapEnum, whether to report function source info (url/row/col)", DEFAULT_CONFIG_EnableFunctionSourceReportForHeapEnum)
FLAGNR(Number,  ForceFragmentAddressSpace , "Fragment the address space", 128 * 1024 * 1024)
FLAGNR(Number,  ForceOOMOnEBCommit, "Force CommitBuffer to return OOM", 0)
FLAGR (Boolean, ForceDynamicProfile   , "Force to always generate profiling byte code", DEFAULT_CONFIG_ForceDynamicProfile)
FLAGNR(Boolean, ForceES5Array         , "Force using ES5Array", DEFAULT_CONFIG_ForceES5Array)
FLAGNR(Boolean, ForceAsmJsLinkFail    , "Force asm.js link time validation to fail", DEFAULT_CONFIG_ForceAsmJsLinkFail)
FLAGNR(Boolean, ForceExpireOnNonCacheCollect, "Allow expiration collect outside of cache collection cleanups", DEFAULT_CONFIG_ForceExpireOnNonCacheCollect)
FLAGNR(Boolean, ForceFastPath         , "Force fast-paths in native codegen", DEFAULT_CONFIG_ForceFastPath)
FLAGNR(Boolean, ForceFloatPref        , "Force float preferencing (JIT only)", false)
FLAGNR(Boolean, ForceJITLoopBody      , "Force jit loop body only", DEFAULT_CONFIG_ForceJITLoopBody)
FLAGNR(Boolean, ForceStaticInterpreterThunk, "Force using static interpreter thunk", DEFAULT_CONFIG_ForceStaticInterpreterThunk)
FLAGNR(Boolean, DumpCommentsFromReferencedFiles, "Allow printing comments of comment-table of the referenced file as well (use with -trace:CommentTable)", DEFAULT_CONFIG_DumpCommentsFromReferencedFiles)
FLAGNR(Number,  DelayFullJITSmallFunc , "Scale Full JIT threshold for small functions which are going to be inlined soon. To provide fraction scale, the final scale is scale following this option divided by 10", DEFAULT_CONFIG_DelayFullJITSmallFunc)
FLAGNR(Boolean, EnableFatalErrorOnOOM, "Enabling failfast fatal error on OOM", DEFAULT_CONFIG_EnableFatalErrorOnOOM)

#if defined(_M_ARM32_OR_ARM64)
FLAGNR(Boolean, ForceLocalsPtr        , "Force use of alternative locals pointer (JIT only)", false)
#endif
FLAGNR(Boolean, DeferLoadingAvailableSource, "Treat available source code as a dummy defer-mappable object to go through that code path.", DEFAULT_CONFIG_DeferLoadingAvailableSource)
FLAGR (Boolean, ForceNative           , "Force JIT everything that is called before running it, ignoring limits", false)
FLAGNR(Boolean, ForceSerialized       , "Always serialize and deserialize byte codes before execution", DEFAULT_CONFIG_ForceSerialized)
FLAGNR(Number,  ForceSerializedBytecodeMajorVersion, "Force the byte code serializer to write this major version number", 0)
FLAGNR(Number,  ForceSerializedBytecodeVersionSchema, "Force the byte code serializer to write this kind of version. Decimal 10 is engineering, 20 is release mode, and 0 means use the default setting.", 0)
FLAGNR(Boolean, ForceStrictMode, "Force strict mode checks on all functions", false)
FLAGNR(Boolean, ForceUndoDefer        , "Defer parsing of all function bodies, but undo deferral", false)
FLAGNR(Boolean, ForceBlockingConcurrentCollect, "Force doing in-thread GC on concurrent thread- this will skip doing concurrent collect", false)
FLAGNR(Boolean, FreTestDiagMode, "Enabled collection of diagnostic information on fretest builds", false)
#ifdef BYTECODE_TESTING
FLAGNR(Number,  ByteCodeBranchLimit,    "Short branch limit before we use the branch island", 128)
FLAGNR(Boolean, MediumByteCodeLayout  , "Always use medium layout for bytecodes", false)
FLAGNR(Boolean, LargeByteCodeLayout   , "Always use large layout for bytecodes", false)
#endif
#ifdef FAULT_INJECTION
FLAGNR(Number,  FaultInjection        , "FaultInjectMode - 0 (count only), 1 (count equal), 2 (count at or above), 3 (stackhashing)",-1)
FLAGNR(Number,  FaultInjectionCount   , "Injects an out of memory at the specified allocation", -1)
FLAGNR(String,  FaultInjectionType    , "FaultType (flag values) -  1 (Throw), 2 (NoThrow), 4 (MarkThrow), 8 (MarkNoThrow), FFFFFFFF (All)", nullptr)
FLAGNR(String,  FaultInjectionFilter  , "A string to restrict the fault injection, the string can be like ArenaAllocator name", nullptr)
FLAGNR(Number,  FaultInjectionAllocSize, "Do fault injection only this size", -1)
FLAGNR(String,  FaultInjectionStackFile   , "Stacks to match, default: stack.txt in current directory", _u("stack.txt"))
FLAGNR(Number,  FaultInjectionStackLineCount   , "Count of lines in the stack file used for matching", -1)
FLAGNR(String,  FaultInjectionStackHash, "Match stacks hash on Chakra frames to inject the fault, hex string", _u("0"))
FLAGNR(Number,  FaultInjectionScriptContextToTerminateCount, "Script context# COUNT % (Number of script contexts) to terminate", 1)
#endif
FLAGNR(Number, InduceCodeGenFailure, "Probability of a codegen job failing.", DEFAULT_CONFIG_InduceCodeGenFailure)
FLAGNR(Number, InduceCodeGenFailureSeed, "Seed used while calculating codegen failure probability", 0)
FLAGNR(Number, InjectPartiallyInitializedInterpreterFrameError, "The number of interpreter stack frame (with 1 being bottom-most) to inject error before the frame is initialized.", DEFAULT_CONFIG_InjectPartiallyInitializedInterpreterFrameError)
FLAGNR(Number, InjectPartiallyInitializedInterpreterFrameErrorType, "Type of error to inject: 0 - debug break, 1 - exception.", DEFAULT_CONFIG_InjectPartiallyInitializedInterpreterFrameErrorType)
FLAGNR(Boolean, GenerateByteCodeBufferReturnsCantGenerate, "Serialized byte code generation always returns SCRIPT_E_CANT_GENERATE", false)
FLAGNR(Number, GoptCleanupThreshold, "Number of instructions seen before we cleanup the value table", DEFAULT_CONFIG_GoptCleanupThreshold)
FLAGNR(Number, AsmGoptCleanupThreshold, "Number of instructions seen before we cleanup the value table", DEFAULT_CONFIG_AsmGoptCleanupThreshold)
FLAGNR(Boolean, HighPrecisionDate, "Enable sub-millisecond resolution in Javascript Date for benchmark timing", DEFAULT_CONFIG_HighPrecisionDate)
FLAGNR(Number,  InlineCountMax        , "Maximum count in bytecodes to inline in a given function", DEFAULT_CONFIG_InlineCountMax)
FLAGNRA(Number, InlineCountMaxInLoopBodies, icminlb, "Maximum count in bytecodes to inline in a given function", DEFAULT_CONFIG_InlineCountMaxInLoopBodies)
FLAGNRA(Number, InlineInLoopBodyScaleDownFactor, iilbsdf, "Maximum depth of a recursive inline call", DEFAULT_CONFIG_InlineInLoopBodyScaleDownFactor)
FLAGNR(Number,  InlineThreshold       , "Maximum size in bytecodes of an inline candidate", DEFAULT_CONFIG_InlineThreshold)
FLAGNR(Number,  AggressiveInlineCountMax, "Maximum count in bytecodes to inline in a given function", DEFAULT_CONFIG_AggressiveInlineCountMax)
FLAGNR(Number,  AggressiveInlineThreshold, "Maximum size in bytecodes of an inline candidate for aggressive inlining", DEFAULT_CONFIG_AggressiveInlineThreshold)
FLAGNR(Number,  InlineThresholdAdjustCountInLargeFunction       , "Adjustment in the maximum size in bytecodes of an inline candidate in a large function", DEFAULT_CONFIG_InlineThresholdAdjustCountInLargeFunction)
FLAGNR(Number,  InlineThresholdAdjustCountInMediumSizedFunction , "Adjustment in the maximum size in bytecodes of an inline candidate in a medium sized function", DEFAULT_CONFIG_InlineThresholdAdjustCountInMediumSizedFunction)
FLAGNR(Number,  InlineThresholdAdjustCountInSmallFunction       , "Adjustment in the maximum size in bytecodes of an inline candidate in a small function", DEFAULT_CONFIG_InlineThresholdAdjustCountInSmallFunction)
FLAGNR(Number,  AsmJsInlineAdjust       , "Adjustment in the maximum size in bytecodes of an inline candidate for wasm function", DEFAULT_CONFIG_AsmJsInlineAdjust)
FLAGNR(String,  Interpret             , "List of functions to interpret", nullptr)
FLAGNR(Phases,  Instrument            , "Instrument the generated code from the given phase", )
FLAGNR(Number,  JitQueueThreshold     , "Max number of work items/script context in the jit queue", DEFAULT_CONFIG_JitQueueThreshold)
#ifdef LEAK_REPORT
FLAGNR(String,  LeakReport            , "File name for the leak report", nullptr)
#endif
FLAGNR(Number,  LoopInlineThreshold   , "Maximum size in bytecodes of an inline candidate with loops or not enough profile data", DEFAULT_CONFIG_LoopInlineThreshold)
FLAGNR(Number,  LeafInlineThreshold   , "Maximum size in bytecodes of an inline candidate with loops or not enough profile data", DEFAULT_CONFIG_LeafInlineThreshold)
FLAGNR(Number,  ConstantArgumentInlineThreshold, "Maximum size in bytecodes of an inline candidate with constant argument and the argument being used for a branch", DEFAULT_CONFIG_ConstantArgumentInlineThreshold)
FLAGNR(Number,  RecursiveInlineThreshold, "Maximum size in bytecodes of an inline candidate to inline recursively", DEFAULT_CONFIG_RecursiveInlineThreshold)
FLAGNR(Number,  RecursiveInlineDepthMax, "Maximum depth of a recursive inline call", DEFAULT_CONFIG_RecursiveInlineDepthMax)
FLAGNR(Number,  RecursiveInlineDepthMin, "Maximum depth of a recursive inline call", DEFAULT_CONFIG_RecursiveInlineDepthMin)
FLAGNR(Number,  RedeferralCap,           "Number of compilations beyond which we stop redeferring a function", DEFAULT_CONFIG_RedeferralCap)
FLAGNR(Number,  Loop                  , "Number of times to execute the script (useful for profiling short benchmarks and finding leaks)", DEFAULT_CONFIG_Loop)
FLAGRA(Number,  LoopInterpretCount    , lic, "Number of times loop has to be interpreted before JIT Loop body", DEFAULT_CONFIG_LoopInterpretCount)
FLAGNR(Number,  LoopProfileIterations , "Number of iterations of a loop that must be profiled before jitting the loop body", DEFAULT_CONFIG_LoopProfileIterations)
FLAGNR(Number,  OutsideLoopInlineThreshold     , "Maximum size in bytecodes of an inline candidate outside a loop in inliner", DEFAULT_CONFIG_OutsideLoopInlineThreshold)
FLAGNR(Number,  MaxFuncInlineDepth    , "Number of times to allow inlining a function recursively, plus one (min: 1, max: 255)", DEFAULT_CONFIG_MaxFuncInlineDepth)
FLAGNR(Number,  MaxNumberOfInlineesWithLoop, "Number of times to allow inlinees with a loop in a top function", DEFAULT_CONFIG_MaxNumberOfInlineesWithLoop)
#ifdef MEMSPECT_TRACKING
FLAGNR(Phases,  Memspect,              "Enables memspect tracking to perform memory investigations.", )
#endif
FLAGNR(Number,  PolymorphicInlineThreshold     , "Maximum size in bytecodes of a polymorphic inline candidate", DEFAULT_CONFIG_PolymorphicInlineThreshold)
FLAGNR(Boolean, PrimeRecycler         , "Prime the recycler first", DEFAULT_CONFIG_PrimeRecycler)
FLAGNR(Boolean, PrivateHeap           , "Use HeapAlloc with a private heap", DEFAULT_CONFIG_PrivateHeap)
FLAGNR(Boolean, TraceEngineRefcount   , "Output traces for ScriptEngine AddRef/Release to debug lifetime management", false)
#if defined(CHECK_MEMORY_LEAK) || defined(LEAK_REPORT)
FLAGNR(Boolean, LeakStackTrace ,        "Include stack trace on leaked pinned object and heap objects", false)
FLAGNR(Boolean, ForceMemoryLeak ,       "Fake leak some memory to test leak report and check memory leak", false)
#endif
FLAGNR(Boolean, DumpAfterFinalGC, "Collect a process dump after calling finalgc", false)
FLAGNR(Boolean, ForceOldDateAPI       , "Force Chakra to use old dates API regardless of availability of a new one", DEFAULT_CONFIG_ForceOldDateAPI)

FLAGNR(Number,  JitLoopBodyHotLoopThreshold    , "Number of times loop has to be iterated in jitloopbody before it is determined as hot", DEFAULT_CONFIG_JitLoopBodyHotLoopThreshold)
FLAGNR(Number,  LoopBodySizeThresholdToDisableOpts, "Minimum bytecode size of a loop body, above which we might consider switching off optimizations in jit loop body to avoid rejits", DEFAULT_CONFIG_LoopBodySizeThresholdToDisableOpts)

FLAGNR(Number,  MaxJitThreadCount     , "Number of maximum allowed parallel jit threads (actual number is factor of number of processors and other heuristics)", DEFAULT_CONFIG_MaxJitThreadCount)
FLAGNR(Boolean, ForceMaxJitThreadCount, "Force the number of parallel jit threads as specified by MaxJitThreadCount flag (creation guaranteed)", DEFAULT_CONFIG_ForceMaxJitThreadCount)

FLAGR(Boolean, MitigateSpectre, "Use mitigations for Spectre", DEFAULT_CONFIG_MitigateSpectre)

FLAGPR(Boolean, MitigateSpectre, AddMaskingBlocks, "Optimize Spectre mitigations by masking on loop out edges", DEFAULT_CONFIG_AddMaskingBlocks)

FLAGPR(Boolean, MitigateSpectre, PoisonVarArrayLoad, "Poison loads from Var arrays", DEFAULT_CONFIG_PoisonVarArrayLoad)
FLAGPR(Boolean, MitigateSpectre, PoisonIntArrayLoad, "Poison loads from Int arrays", DEFAULT_CONFIG_PoisonIntArrayLoad)
FLAGPR(Boolean, MitigateSpectre, PoisonFloatArrayLoad, "Poison loads from Float arrays", DEFAULT_CONFIG_PoisonFloatArrayLoad)
FLAGPR(Boolean, MitigateSpectre, PoisonTypedArrayLoad, "Poison loads from TypedArrays", DEFAULT_CONFIG_PoisonTypedArrayLoad)
FLAGPR(Boolean, MitigateSpectre, PoisonStringLoad, "Poison indexed loads from strings", DEFAULT_CONFIG_PoisonStringLoad)
FLAGPR(Boolean, MitigateSpectre, PoisonObjectsForLoads, "Poison objects after type checks for loads", DEFAULT_CONFIG_PoisonObjectsForLoads)

FLAGPR(Boolean, MitigateSpectre, PoisonVarArrayStore, "Poison stores to Var arrays", DEFAULT_CONFIG_PoisonVarArrayStore)
FLAGPR(Boolean, MitigateSpectre, PoisonIntArrayStore, "Poison stores to Int arrays", DEFAULT_CONFIG_PoisonIntArrayStore)
FLAGPR(Boolean, MitigateSpectre, PoisonFloatArrayStore, "Poison stores to Float arrays", DEFAULT_CONFIG_PoisonFloatArrayStore)
FLAGPR(Boolean, MitigateSpectre, PoisonTypedArrayStore, "Poison stores to TypedArrays", DEFAULT_CONFIG_PoisonTypedArrayStore)
FLAGPR(Boolean, MitigateSpectre, PoisonStringStore, "Poison indexed stores to strings", DEFAULT_CONFIG_PoisonStringStore)
FLAGPR(Boolean, MitigateSpectre, PoisonObjectsForStores, "Poison objects after type checks for stores", DEFAULT_CONFIG_PoisonObjectsForStores)

FLAGNR(Number,  MinInterpretCount     , "Minimum number of times a function must be interpreted", 0)
FLAGNR(Number,  MinSimpleJitRunCount  , "Minimum number of times a function must be run in simple jit", 0)
FLAGNRA(Number, MaxInterpretCount     , Mic, "Maximum number of times a function can be interpreted", 0)
FLAGNRA(Number, MaxSimpleJitRunCount  , Msjrc, "Maximum number of times a function will be run in SimpleJitted code", 0)
FLAGNRA(Number, MinMemOpCount         , Mmoc, "Minimum count of a loop to activate MemOp", DEFAULT_CONFIG_MinMemOpCount)

#if ENABLE_COPYONACCESS_ARRAY
FLAGNR(Number,  MaxCopyOnAccessArrayLength, "Maximum length of copy-on-access array", DEFAULT_CONFIG_MaxCopyOnAccessArrayLength)
FLAGNR(Number,  MinCopyOnAccessArrayLength, "Minimum length of copy-on-access array", DEFAULT_CONFIG_MinCopyOnAccessArrayLength)
FLAGNR(Number,  CopyOnAccessArraySegmentCacheSize, "Size of copy-on-access array segment cache (1-32)", DEFAULT_CONFIG_CopyOnAccessArraySegmentCacheSize)
#endif

FLAGNR(Number, MinTemplatizedJitRunCount, "Minimum number of times a function must be Templatized Jitted", DEFAULT_CONFIG_MinTemplatizedJitRunCount)
FLAGNR(Number, MinAsmJsInterpreterRunCount, "Minimum number of times a function must be Asm Interpreted", DEFAULT_CONFIG_MinAsmJsInterpreterRunCount)

FLAGNR(Number, MinTemplatizedJitLoopRunCount, "Minimum LoopCount run of the Templatized Jit function to run FullJited", DEFAULT_CONFIG_MinTemplatizedJitLoopRunCount)
FLAGNRA(Number, MaxTemplatizedJitRunCount, Mtjrc, "Maximum number of times a function must be templatized jit", DEFAULT_CONFIG_MaxTemplatizedJitRunCount)
FLAGNRA(Number, MaxAsmJsInterpreterRunCount, Maic, "Maximum number of times a function must be interpreted in asmjs", DEFAULT_CONFIG_MaxAsmJsInterpreterRunCount)

FLAGR (Number,  AutoProfilingInterpreter0Limit, "Limit after which to transition to the next execution mode", DEFAULT_CONFIG_AutoProfilingInterpreter0Limit)
FLAGR (Number,  ProfilingInterpreter0Limit, "Limit after which to transition to the next execution mode", DEFAULT_CONFIG_ProfilingInterpreter0Limit)
FLAGR (Number,  AutoProfilingInterpreter1Limit, "Limit after which to transition to the next execution mode", DEFAULT_CONFIG_AutoProfilingInterpreter1Limit)
FLAGR (Number,  SimpleJitLimit, "Limit after which to transition to the next execution mode", DEFAULT_CONFIG_SimpleJitLimit)
FLAGR (Number,  ProfilingInterpreter1Limit, "Limit after which to transition to the next execution mode", DEFAULT_CONFIG_ProfilingInterpreter1Limit)

FLAGNRA(String, ExecutionModeLimits,        Eml,  "Execution mode limits in th form: AutoProfilingInterpreter0.ProfilingInterpreter0.AutoProfilingInterpreter1.SimpleJit.ProfilingInterpreter1 - Example: -ExecutionModeLimits:12.4.0.132.12", _u(""))
FLAGRA(Boolean, EnforceExecutionModeLimits, Eeml, "Enforces the execution mode limits such that they are never exceeded.", false)

FLAGNRA(Number, SimpleJitAfter        , Sja, "Number of calls to a function after which to simple-JIT the function", 0)
FLAGNRA(Number, FullJitAfter          , Fja, "Number of calls to a function after which to full-JIT the function. The function will be profiled for every iteration.", 0)

FLAGNR(Boolean, NewSimpleJit          , "Uses the new simple JIT", DEFAULT_CONFIG_NewSimpleJit)

FLAGNR(Number,  MaxLinearIntCaseCount , "Maximum number of cases(in switch statement) for which instructions can be generated linearly",DEFAULT_CONFIG_MaxLinearIntCaseCount)
FLAGNR(Number,  MaxSingleCharStrJumpTableSize, "Maximum single char string jump table size", DEFAULT_CONFIG_MaxSingleCharStrJumpTableSize)
FLAGNR(Number,  MaxSingleCharStrJumpTableRatio, "Maximum single char string jump table size as multiples of the actual case arm", DEFAULT_CONFIG_MaxSingleCharStrJumpTableRatio)
FLAGNR(Number,  MinSwitchJumpTableSize , "Minimum size of the jump table, that is created for consecutive integer case arms in a Switch Statement",DEFAULT_CONFIG_MinSwitchJumpTableSize)
FLAGNR(Number,  MaxLinearStringCaseCount,  "Maximum number of string cases(in switch statement) for which instructions can be generated linearly",DEFAULT_CONFIG_MaxLinearStringCaseCount)
FLAGR(Number,   MinDeferredFuncTokenCount, "Minimum length in tokens of defer-parsed function", DEFAULT_CONFIG_MinDeferredFuncTokenCount)
#if DBG
FLAGNR(Number,  SkipFuncCountForBailOnNoProfile,  "Initial Number of functions in a func body to be skipped from forcibly inserting BailOnNoProfile.", DEFAULT_CONFIG_SkipFuncCountForBailOnNoProfile)
#endif
FLAGNR(Number,  MaxJITFunctionBytecodeByteLength, "The biggest function we'll JIT (bytecode bytelength)", DEFAULT_CONFIG_MaxJITFunctionBytecodeByteLength)
FLAGNR(Number,  MaxJITFunctionBytecodeCount, "The biggest function we'll JIT (bytecode count)", DEFAULT_CONFIG_MaxJITFunctionBytecodeCount)
FLAGNR(Number,  MaxLoopsPerFunction   , "Maximum number of loops in any function in the script", DEFAULT_CONFIG_MaxLoopsPerFunction)
FLAGNR(Number,  FuncObjectInlineCacheThreshold  , "Maximum number of inline caches a function body may have to allow for inline caches to be allocated on the function object", DEFAULT_CONFIG_FuncObjectInlineCacheThreshold)
FLAGNR(Boolean, NoDeferParse          , "Disable deferred parsing", false)
FLAGNR(Boolean, NoLogo                , "No logo, which we don't display anyways", false)
FLAGNR(Boolean, OOPJITMissingOpts     , "Use optimizations that are missing from OOP JIT", DEFAULT_CONFIG_OOPJITMissingOpts)
FLAGNR(Boolean, CrashOnOOPJITFailure  , "Crash runtime process if JIT process crashes", DEFAULT_CONFIG_CrashOnOOPJITFailure)
FLAGNR(Boolean, OOPCFGRegistration    , "Do CFG registration OOP (under OOP JIT)", DEFAULT_CONFIG_OOPCFGRegistration)
FLAGNR(Boolean, ForceJITCFGCheck      , "Have JIT code always do CFG check even if range check succeeded", DEFAULT_CONFIG_ForceJITCFGCheck)
FLAGNR(Boolean, UseJITTrampoline      , "Use trampoline for JIT entry points and emit range checks for it", DEFAULT_CONFIG_UseJITTrampoline)
FLAGR (Boolean, NoNative              , "Disable native codegen", false)
FLAGNR(Number,  NopFrequency          , "Frequency of NOPs inserted by NOP insertion phase.  A NOP is guaranteed to be inserted within a range of (1<<n) instrs (default=8)", DEFAULT_CONFIG_NopFrequency)
FLAGNR(Boolean, NoStrictMode          , "Disable strict mode checks on all functions", false)
FLAGNR(Boolean, NormalizeStats        , "When dumping stats, do some normalization (used with -instrument:linearscan)", false)
FLAGNR(Phases,  Off                   , "Turn off specific phases or feature.(Might not work for all phases)", )
FLAGNR(Phases,  OffProfiledByteCode   , "Turn off specific byte code for phases or feature.(Might not work for all phases)", )
FLAGNR(Phases,  On                    , "Turn on specific phases or feature.(Might not work for all phases)", )
FLAGNR(String,  OutputFile            , "Log the output to a specified file. Default: output.log in the working directory.", _u("output.log"))
FLAGNR(String,  OutputFileOpenMode    , "File open mode for OutputFile. Default: wt, specify 'at' for append", _u("wt"))
#ifdef ENABLE_TRACE
FLAGNR(Boolean, InMemoryTrace         , "Enable in-memory trace (investigate crash using trace in dump file). Use !jd.dumptrace to print it.", DEFAULT_CONFIG_InMemoryTrace)
FLAGNR(Number,  InMemoryTraceBufferSize, "The size of circular buffer for in-memory trace (the units used is: number of trace calls). ", DEFAULT_CONFIG_InMemoryTraceBufferSize)
#if CONFIG_RICH_TRACE_FORMAT
FLAGNR(Boolean, RichTraceFormat, "Whether to use extra data in Output/Trace header.", DEFAULT_CONFIG_RichTraceFormat)
#endif
#ifdef STACK_BACK_TRACE
FLAGNR(Boolean, TraceWithStack, "Whether the trace need to include stack trace (for each trace entry).", DEFAULT_CONFIG_TraceWithStack)
#endif // STACK_BACK_TRACE
#endif // ENABLE_TRACE
FLAGNR(Boolean, PrintRunTimeDataCollectionTrace, "Print traces needed for runtime data collection", false)
#ifdef ENABLE_PREJIT
FLAGR (Boolean, Prejit                , "Prejit everything, including things that are not called, ignoring limits (default: false)", DEFAULT_CONFIG_Prejit)
#endif
FLAGNR(Boolean, PrintSrcInDump        , "Print the lineno and the source code in the intermediate dumps", true)
#if PROFILE_DICTIONARY
FLAGNR(Number,  ProfileDictionary     , "Profile dictionary usage. Only dictionaries with max depth of <number> or above are displayed (0=no filter).", -1)
#endif
#ifdef PROFILE_EXEC
FLAGNR(Phases,  Profile               , "Profile the given phase", )
FLAGNR(Number,  ProfileThreshold      , "A phase is displayed in the profiler report only if its contribution is more than this threshold", 0)
#endif
#ifdef PROFILE_OBJECT_LITERALS
FLAGNR(Boolean, ProfileObjectLiteral  , "Profile Object literal usage", false)
#endif
#ifdef PROFILE_MEM
FLAGNR(String,  ProfileMemory         , "Profile memory usage", )
#endif
#ifdef PROFILE_STRINGS
FLAGNR(Boolean, ProfileStrings        , "Profile string statistics", false)
#endif
#ifdef PROFILE_TYPES
FLAGNR(Boolean, ProfileTypes          , "Profile type statistics", false)
#endif
#ifdef PROFILE_EVALMAP
FLAGNR(Boolean, ProfileEvalMap        , "Profile eval map statistics", false)
#endif

#ifdef PROFILE_BAILOUT_RECORD_MEMORY
FLAGNR(Boolean, ProfileBailOutRecordMemory, "Profile bailout record memory statistics", false)
#endif

#if DBG
FLAGNR(Boolean, ValidateIntRanges, "Validate at runtime int ranges/bounds determined by the globopt", DEFAULT_CONFIG_ValidateIntRanges)
#endif
FLAGNR(Number,  RejitMaxBailOutCount, "Maximum number of bailouts for a bailout record after which rejit is forced", DEFAULT_CONFIG_RejitMaxBailOutCount)
FLAGNR(Number,  CallsToBailoutsRatioForRejit, "Ratio of function calls to bailouts above which a rejit is considered", DEFAULT_CONFIG_CallsToBailoutsRatioForRejit)
FLAGNR(Number,  LoopIterationsToBailoutsRatioForRejit, "Ratio of loop iteration count to bailouts above which a rejit of the loop body is considered", DEFAULT_CONFIG_LoopIterationsToBailoutsRatioForRejit)
FLAGNR(Number,  MinBailOutsBeforeRejit, "Minimum number of bailouts for a single bailout record after which a rejit is considered", DEFAULT_CONFIG_MinBailOutsBeforeRejit)
FLAGNR(Number,  MinBailOutsBeforeRejitForLoops, "Minimum number of bailouts for a single bailout record after which a rejit is considered", DEFAULT_CONFIG_MinBailOutsBeforeRejitForLoops)
FLAGNR(Boolean, LibraryStackFrame           , "Display library stack frame", DEFAULT_CONFIG_LibraryStackFrame)
FLAGNR(Boolean, LibraryStackFrameDebugger   , "Assume debugger support for library stack frame", DEFAULT_CONFIG_LibraryStackFrameDebugger)
#ifdef RECYCLER_STRESS
FLAGNR(Boolean, RecyclerStress        , "Stress the recycler by collect on every allocation call", false)
#if ENABLE_CONCURRENT_GC
FLAGNR(Boolean, RecyclerBackgroundStress        , "Stress the recycler by collect in the background thread on every allocation call", false)
FLAGNR(Boolean, RecyclerConcurrentStress        , "Stress the concurrent recycler by concurrent collect on every allocation call", false)
FLAGNR(Boolean, RecyclerConcurrentRepeatStress  , "Stress the concurrent recycler by concurrent collect on every allocation call and repeat mark and rescan in the background thread", false)
#endif
#if ENABLE_PARTIAL_GC
FLAGNR(Boolean, RecyclerPartialStress , "Stress the partial recycler by partial collect on every allocation call", false)
#endif
FLAGNR(Boolean, RecyclerTrackStress, "Stress tracked object handling by simulating tracked objects for regular allocations", false)
FLAGNR(Boolean, RecyclerInduceFalsePositives, "Stress recycler by forcing false positive object marks", false)
#endif // RECYCLER_STRESS
FLAGNR(Boolean, RecyclerForceMarkInterior, "Force all the mark as interior", DEFAULT_CONFIG_RecyclerForceMarkInterior)
#if ENABLE_CONCURRENT_GC
FLAGNR(Number,  RecyclerPriorityBoostTimeout, "Adjust priority boost timeout", 5000)
FLAGNR(Number,  RecyclerThreadCollectTimeout, "Adjust thread collect timeout", 1000)
FLAGRA(Boolean, EnableConcurrentSweepAlloc, ecsa, "Turns off the feature to allow allocations during concurrent sweep.", true)
#endif
#ifdef RECYCLER_PAGE_HEAP
FLAGNR(Number,      PageHeap,             "Use full page for heap allocations", DEFAULT_CONFIG_PageHeap)
FLAGNR(Boolean,     PageHeapAllocStack,   "Capture alloc stack under page heap mode", DEFAULT_CONFIG_PageHeapAllocStack)
FLAGNR(Boolean,     PageHeapFreeStack,    "Capture free stack under page heap mode", DEFAULT_CONFIG_PageHeapFreeStack)
FLAGNR(NumberRange, PageHeapBucketNumber, "Bucket numbers to be used for page heap allocations", )
FLAGNR(Number,      PageHeapBlockType,    "Type of blocks to use page heap for", DEFAULT_CONFIG_PageHeapBlockType)
FLAGNR(Boolean,     PageHeapDecommitGuardPage, "Decommit page heap guard page", true)
#endif
#ifdef RECYCLER_NO_PAGE_REUSE
FLAGNR(Boolean, RecyclerNoPageReuse,     "Do not reuse page in recycler", false)
#endif
#ifdef RECYCLER_MEMORY_VERIFY
FLAGNR(Phases,  RecyclerVerify         , "Verify recycler memory", )
FLAGNR(Number,  RecyclerVerifyPadSize  , "Padding size to verify recycler memory", 12)
#endif
FLAGNR(Boolean, RecyclerTest           , "Run recycler tests instead of executing script", false)
FLAGNR(Boolean, RecyclerProtectPagesOnRescan, "Temporarily switch all pages to read only during rescan", false)
#ifdef RECYCLER_VERIFY_MARK
FLAGNR(Boolean, RecyclerVerifyMark    , "verify concurrent gc", false)
#endif
FLAGR (Number,  LowMemoryCap          , "Memory cap indicating a low-memory process", DEFAULT_CONFIG_LowMemoryCap)
FLAGNR(Number,  NewPagesCapDuringBGSweeping, "New pages count allowed to be allocated during background sweeping", DEFAULT_CONFIG_NewPagesCapDuringBGSweeping)
FLAGR (Number,  AllocPolicyLimit      , "Memory allocation policy limit in MB (default: -1, which means no allocation policy limit).", DEFAULT_CONFIG_AllocationPolicyLimit)
#ifdef RUNTIME_DATA_COLLECTION
FLAGNR(String,  RuntimeDataOutputFile, "Filename to write the dynamic profile info", nullptr)
#endif
FLAGR (Number,  SpeculationCap        , "How much bytecode we'll speculatively JIT", DEFAULT_CONFIG_SpeculationCap)
#if DBG_DUMP || defined(BGJIT_STATS) || defined(RECYCLER_STATS)
FLAGNR(Phases,  Stats                 , "Stats the given phase", )
#endif
#if EXCEPTION_RECOVERY
FLAGNR(Boolean, SwallowExceptions     , "Force a try/catch around every statement", false)
#endif
FLAGNR(Boolean, PrintSystemException  , "Always print a message when there's OOM or OOS", false)
FLAGNR(Number,  SwitchOptHolesThreshold,  "Maximum percentage of holes (missing case values in a switch statement) with which a jump table can be created",DEFAULT_CONFIG_SwitchOptHolesThreshold)
FLAGR (Number,  TempMin                  , "Temp number switch which code can temporarily use for debugging", DEFAULT_CONFIG_TempMin)
FLAGR (Number,  TempMax                  , "Temp number switch which code can temporarily use for debugging", DEFAULT_CONFIG_TempMax)
FLAGNR(Phases,  Trace                 , "Trace the given phase", )

#if defined(_M_IX86) || defined(_M_X64)
FLAGR(Number,   LoopAlignNopLimit       , "Max number of nops for loop alignment", DEFAULT_CONFIG_LoopAlignNopLimit)
#endif

#ifdef PROFILE_MEM
FLAGNR(Phases,  TraceMemory           , "Trace memory usage", )
#endif
#if DBG_DUMP || defined(RECYCLER_TRACE)
//TraceMetaDataParsing flag with optional levels:
//    Level 1 = interfaces only
//    Level 2 = interfaces and methods
//    Level 3 = interfaces, methods and parameters
//    Level 4 = interfaces and properties
//    Level 5 (default) = ALL
FLAGNR(Number,  TraceMetaDataParsing  , "Trace metadata parsing for generating JS projections. [Levels 1-5, with 5 corresponding to most detailed]", 5)
FLAGNR(Boolean, TraceWin8Allocations  , "Trace the win8 memory allocations", false)
FLAGNR(Boolean, TraceWin8DeallocationsImmediate  , "Trace the win8 memory deallocations immediately", false)
FLAGNR(Boolean, PrintWin8StatsDetailed  , "Print the detailed memory trace report", false)
FLAGNR(Boolean, TraceProtectPages     , "Trace calls to protecting pages of custom heap allocated pages", false)
#endif
FLAGNR(Boolean, TraceAsyncDebugCalls  , "Trace calls to async debugging API (default: false)", DEFAULT_CONFIG_TraceAsyncDebugCalls)
#ifdef TRACK_DISPATCH
FLAGNR(Boolean, TrackDispatch         , "Save stack traces of where JavascriptDispatch/HostVariant are created", false)
#endif
FLAGNR(Boolean, Verbose               , "Dump details", DEFAULT_CONFIG_Verbose)
FLAGNR(Boolean, UseFullName           , "Enable fully qualified name", DEFAULT_CONFIG_UseFullName)
FLAGNR(Boolean, Utf8                  , "Use UTF8 for file output", false)
FLAGR (Number,  Version               , "Version in which to run the jscript engine. [one of 1,2,3,4,5,6]. Default is latest for jc/jshost, 1 for IE", 6 )
FLAGR(Boolean, WERExceptionSupport    , "WER feature for extended exception support. Enabled when WinRT is enabled", false)
FLAGNR(Boolean, ExtendedErrorStackForTestHost, "Enable passing extended error stack string to test host.", DEFAULT_CONFIG_ExtendedErrorStackForTestHost)
FLAGNR(Boolean, errorStackTrace       , "error.StackTrace feature. Remove when feature complete", DEFAULT_CONFIG_errorStackTrace)
FLAGNR(Boolean, DoHeapEnumOnEngineShutdown, "Perform a heap enumeration whenever shut a script engine down", false)
#ifdef HEAP_ENUMERATION_VALIDATION
FLAGNR(Boolean, ValidateHeapEnum      , "Validate that heap enumeration is reporting all Js::RecyclableObjects in the heap", false)
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
//
// Regex flags
//
FLAGR (Boolean, RegexTracing          , "Trace all Regex invocations to the output.", DEFAULT_CONFIG_RegexTracing)
FLAGR (Boolean, RegexProfile          , "Collect usage statistics on all Regex invocations.", DEFAULT_CONFIG_RegexProfile)
FLAGR (Boolean, RegexDebug            , "Trace compilation of UnifiedRegex expressions.", DEFAULT_CONFIG_RegexDebug)
FLAGR (Boolean, RegexDebugAST         , "Display Regex AST (requires -RegexDebug to view). [default on]", DEFAULT_CONFIG_RegexDebugAST)
FLAGR (Boolean, RegexDebugAnnotatedAST, "Display Regex Annotated AST (requires -RegexDebug and -RegexDebugAST to view). [default on]", DEFAULT_CONFIG_RegexDebugAnnotatedAST)
FLAGR (Boolean, RegexBytecodeDebug    , "Display layout of UnifiedRegex bytecode (requires -RegexDebug to view).", DEFAULT_CONFIG_RegexBytecodeDebug)
FLAGR (Boolean, RegexOptimize         , "Optimize regular expressions in the unified Regex system (default: true)", DEFAULT_CONFIG_RegexOptimize)
FLAGR (Number,  DynamicRegexMruListSize, "Size of the MRU list for dynamic regexes", DEFAULT_CONFIG_DynamicRegexMruListSize)
#endif

FLAGR (Boolean, OptimizeForManyInstances, "Optimize script engine for many instances (low memory footprint per engine, assume low spare CPU cycles) (default: false)", DEFAULT_CONFIG_OptimizeForManyInstances)
FLAGNR(Boolean, EnableArrayTypeMutation, "Enable force array type mutation on re-entrant region", DEFAULT_CONFIG_EnableArrayTypeMutation)
FLAGNR(Number, ArrayMutationTestSeed, "Seed used for the array mutation", 0)
FLAGNR(Phases,  TestTrace             , "Test trace for the given phase", )
FLAGNR(Boolean, EnableEvalMapCleanup, "Enable cleaning up the eval map", true)
#ifdef PROFILE_MEM
FLAGNR(Boolean, TraceObjectAllocation, "Enable cleaning up the eval map", false)
#endif
FLAGNR(Number, Sse, "Virtually disables SSE-based optimizations above the specified SSE level in the Chakra JIT (does not affect CRT SSE usage)", DEFAULT_CONFIG_Sse)
FLAGNR(Number,  DeletedPropertyReuseThreshold, "Start reusing deleted property indexes after this many properties are deleted. Zero to disable reuse.", DEFAULT_CONFIG_DeletedPropertyReuseThreshold)
FLAGNR(Boolean, ForceStringKeyedSimpleDictionaryTypeHandler, "Force switch to string keyed version of SimpleDictionaryTypeHandler on first new property added to a SimpleDictionaryTypeHandler", DEFAULT_CONFIG_ForceStringKeyedSimpleDictionaryTypeHandler)
FLAGNR(Number,  BigDictionaryTypeHandlerThreshold, "Min Slot Capacity required to convert DictionaryTypeHandler to BigDictionaryTypeHandler.(Advisable to give more than 15 - to avoid false positive cases)", DEFAULT_CONFIG_BigDictionaryTypeHandlerThreshold)
FLAGNR(Boolean, TypeSnapshotEnumeration, "Create a true snapshot of the type of an object before enumeration and enumerate only those properties.", DEFAULT_CONFIG_TypeSnapshotEnumeration)
FLAGNR(Boolean, IsolatePrototypes, "Should prototypes get unique types not shared with other objects (default: true)?", DEFAULT_CONFIG_IsolatePrototypes)
FLAGNR(Boolean, ChangeTypeOnProto, "When becoming a prototype should the object switch to a new type (default: true)?", DEFAULT_CONFIG_ChangeTypeOnProto)
FLAGNR(Boolean, ShareInlineCaches, "Determines whether inline caches are shared between all loads (or all stores) of the same property ID", DEFAULT_CONFIG_ShareInlineCaches)
FLAGNR(Boolean, DisableDebugObject, "Disable test only Debug object properties", DEFAULT_CONFIG_DisableDebugObject)
FLAGNR(Boolean, DumpHeap, "enable Debug.dumpHeap even when DisableDebugObject is set", DEFAULT_CONFIG_DumpHeap)
FLAGNR(String, autoProxy, "enable creating proxy for each object creation", _u("__msTestHandler"))
FLAGNR(Number,  PerfHintLevel, "Specifies the perf-hint level (1,2) 1 == critical, 2 == only noisy", DEFAULT_CONFIG_PerfHintLevel)
#ifdef INTERNAL_MEM_PROTECT_HEAP_ALLOC
FLAGNR(Boolean, MemProtectHeap, "Use the mem protect heap as the default heap", DEFAULT_CONFIG_MemProtectHeap)
#endif
#ifdef RECYCLER_STRESS
FLAGNR(Boolean, MemProtectHeapStress, "Stress the recycler by collect on every allocation call", false)
#if ENABLE_CONCURRENT_GC
FLAGNR(Boolean, MemProtectHeapBackgroundStress, "Stress the recycler by collect in the background thread on every allocation call", false)
FLAGNR(Boolean, MemProtectHeapConcurrentStress, "Stress the concurrent recycler by concurrent collect on every allocation call", false)
FLAGNR(Boolean, MemProtectHeapConcurrentRepeatStress, "Stress the concurrent recycler by concurrent collect on every allocation call and repeat mark and rescan in the background thread", false)
#endif
#if ENABLE_PARTIAL_GC
FLAGNR(Boolean, MemProtectHeapPartialStress, "Stress the partial recycler by partial collect on every allocation call", false)
#endif
#endif
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
FLAGNR(Boolean, FixPropsOnPathTypes, "Mark properties as fixed on path types (default: false).", DEFAULT_CONFIG_FixPropsOnPathTypes)
#endif
FLAGNR(NumberSet, BailoutTraceFilter, "Filter the bailout trace messages to specific bailout kinds.", )
FLAGNR(NumberSet, RejitTraceFilter, "Filter the rejit trace messages to specific bailout kinds.", )

// recycler heuristic flags
FLAGNR(Number,  MaxBackgroundFinishMarkCount, "Maximum number of background finish mark", 1)
FLAGNR(Number,  BackgroundFinishMarkWaitTime, "Millisecond to wait for background finish mark", 15)
FLAGNR(Number,  MinBackgroundRepeatMarkRescanBytes, "Minimum number of bytes rescan to trigger background finish mark",  -1)

#if defined(_M_IX86) || defined(_M_X64)
FLAGNR(Boolean, ZeroMemoryWithNonTemporalStore, "Zero free memory with non-temporal stores to avoid evicting other content from processor cache", DEFAULT_CONFIG_ZeroMemoryWithNonTemporalStore)
#endif

// recycler memory restrict test flags
FLAGNR(Number,  MaxMarkStackPageCount , "Restrict recycler mark stack size (in pages)", -1)
FLAGNR(Number,  MaxTrackedObjectListCount,  "Restrict recycler tracked object count during GC", -1)

// make the recycler page integration path easier to hit
FLAGNR(Number, NumberAllocPlusSize, "Additional bytes to allocate with JavascriptNumber from number allocator (0~496)", 0)

#if DBG
FLAGNR(Boolean, InitializeInterpreterSlotsWithInvalidStackVar, "Enable the initialization of the interpreter local slots with invalid stack vars", false)
#endif

#if DBG
FLAGNR(Number, PRNGSeed0, "Override seed0 for Math.Random()", 0)
FLAGNR(Number, PRNGSeed1, "Override seed1 for Math.Random()", 0)
#endif

FLAGNR(Boolean, ClearInlineCachesOnCollect, "Clear all inline caches on every garbage collection", false)
FLAGNR(Number, InlineCacheInvalidationListCompactionThreshold, "Compact inline cache invalidation lists if their utilization falls below this threshold", DEFAULT_CONFIG_InlineCacheInvalidationListCompactionThreshold)
FLAGNR(Number, ConstructorCacheInvalidationThreshold, "Clear uniquePropertyGuard entries from recyclableData if number of invalidations of constructor caches happened are more than the threshold.", DEFAULT_CONFIG_ConstructorCacheInvalidationThreshold)

#ifdef IR_VIEWER
FLAGNR(Boolean, IRViewer, "Enable IRViewer functionality (improved UI for various stages of IR generation)", false)
#endif /* IR_VIEWER */

FLAGNR(Number,  GCMemoryThreshold, "Threshold for allocation-based GC initiation (in MB)", 0)

#ifdef _CONTROL_FLOW_GUARD
FLAGNR(Boolean, PreReservedHeapAlloc, "Enable Pre-reserved Heap Page Allocator", true)
FLAGNR(Boolean, CFGInJit, "Enable CFG check in JIT", true)
FLAGNR(Boolean, CFG, "Force enable CFG on jshost. version in the jshost's manifest file disables CFG", true)
#endif

#if DBG
    FLAGNR(Number, SimulatePolyCacheWithOneTypeForInlineCacheIndex, "Use with SimulatePolyCacheWithOneTypeForFunction to simulate creating a polymorphic inline cache containing only one type due to a collision, for testing ObjTypeSpec", -1)
#endif

FLAGR(Number, JITServerIdleTimeout, "Idle timeout in milliseconds to do the cleanup in JIT server", 500)
FLAGR(Number, JITServerMaxInactivePageAllocatorCount, "Max inactive page allocators to keep before schedule a cleanup", 10)

FLAGNR(Boolean, StrictWriteBarrierCheck, "Check write barrier setting on none write barrier pages", DEFAULT_CONFIG_StrictWriteBarrierCheck)
FLAGNR(Boolean, WriteBarrierTest, "Always return true while checking barrier to test recycler regardless of annotation", DEFAULT_CONFIG_WriteBarrierTest)
FLAGNR(Boolean, ForceSoftwareWriteBarrier, "Use to turn off write watch to test software write barrier on windows", DEFAULT_CONFIG_ForceSoftwareWriteBarrier)
FLAGNR(Boolean, VerifyBarrierBit, "Verify software write barrier bit is set while marking", DEFAULT_CONFIG_VerifyBarrierBit)
FLAGNR(Boolean, EnableBGFreeZero, "Use to turn off background freeing and zeroing to simulate linux", DEFAULT_CONFIG_EnableBGFreeZero)
FLAGNR(Boolean, KeepRecyclerTrackData, "Keep recycler track data after sweep until reuse", DEFAULT_CONFIG_KeepRecyclerTrackData)

FLAGNR(Number, MaxSingleAllocSizeInMB, "Max size(in MB) in single allocation", DEFAULT_CONFIG_MaxSingleAllocSizeInMB)

#ifdef ENABLE_BASIC_TELEMETRY
FLAGR(String, TelemetryRunType, "Value sent with telemetry events indicating origin class of data.  E.g., if data was triggered from Edge Crawler.", _u(""))
FLAGR(String, TelemetryDiscriminator1, "Value sent with telemetry events. Used to identify filter telemetry data to specific runs.", _u(""))
FLAGR(String, TelemetryDiscriminator2, "Value sent with telemetry events. Used to identify filter telemetry data to specific runs.", _u(""))
#endif

#undef FLAG_REGOVR_EXP
#undef FLAG_EXPERIMENTAL
#undef FLAG
#undef FLAGP

#undef FLAGRA

#undef FLAGNR
#undef FLAGNRA
#undef FLAGPNR
#undef FLAGPRA

#endif
