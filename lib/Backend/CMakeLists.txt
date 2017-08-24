add_library (Chakra.Backend OBJECT
    AgenPeeps.cpp
    AsmJsJITInfo.cpp
    Backend.cpp
    BackendApi.cpp
    BackendOpCodeAttrAsmJs.cpp
    BackwardPass.cpp
    BailOut.cpp
    CaseNode.cpp
    CodeGenAllocators.cpp
    CodeGenNumberAllocator.cpp
    CodeGenWorkItem.cpp
    DbCheckPostLower.cpp
    Debug.cpp
    EhFrame.cpp
    EmitBuffer.cpp
    Encoder.cpp
    EquivalentTypeSet.cpp
    FixedFieldInfo.cpp
    FlowGraph.cpp
    Func.cpp
    FunctionCodeGenJitTimeData.cpp
    FunctionJITRuntimeInfo.cpp
    FunctionJITTimeInfo.cpp
    GlobOpt.cpp
    GlobOptBailOut.cpp
    GlobOptBlockData.cpp
    GlobOptExpr.cpp
    GlobOptFields.cpp
    GlobOptIntBounds.cpp
    GlobOptSimd128.cpp
    IR.cpp
    IRBuilder.cpp
    IRBuilderAsmJs.cpp
    IRType.cpp
    IRViewer.cpp
    InductionVariable.cpp
    Inline.cpp
    InlineeFrameInfo.cpp
    InliningDecider.cpp
    InliningHeuristics.cpp
    IntBounds.cpp
    InterpreterThunkEmitter.cpp
    JavascriptNativeOperators.cpp
    JITThunkEmitter.cpp
    JITOutput.cpp
    JITTimeConstructorCache.cpp
    JITTimeFunctionBody.cpp
    JITTimePolymorphicInlineCache.cpp
    JITTimePolymorphicInlineCacheInfo.cpp
    JITTimeProfileInfo.cpp
    JITTimeWorkItem.cpp
    JITType.cpp
    JITTypeHandler.cpp
    JnHelperMethod.cpp
    LinearScan.cpp
    Lower.cpp
    LowerMDShared.cpp
    LowerMDSharedSimd128.cpp
    NativeCodeData.cpp
    NativeCodeGenerator.cpp
    ObjTypeSpecFldInfo.cpp
    Opnd.cpp
    PDataManager.cpp
    PageAllocatorPool.cpp
    Peeps.cpp
    PreLowerPeeps.cpp
    PrologEncoder.cpp
    QueuedFullJitWorkItem.cpp
    Region.cpp
    SccLiveness.cpp
    Security.cpp
    ServerScriptContext.cpp
    ServerThreadContext.cpp
    SimpleJitProfilingHelpers.cpp
    SimpleLayout.cpp
    SwitchIRBuilder.cpp
    Sym.cpp
    SymTable.cpp
    TempTracker.cpp
    ValueInfo.cpp
    ValueRelativeOffset.cpp
    amd64/EncoderMD.cpp
    amd64/LinearScanMD.cpp
    amd64/LowererMDArch.cpp
    amd64/PeepsMD.cpp
    amd64/PrologEncoderMD.cpp
    amd64/LinearScanMdA.S
    amd64/Thunks.S
#    arm64/EncoderMD.cpp
#    arm64/LowerMD.cpp
#    arm/EncoderMD.cpp
#    arm/LegalizeMD.cpp
#    arm/LinearScanMD.cpp
#    arm/LowerMD.cpp
#    arm/PeepsMD.cpp
#    arm/UnwindInfoManager.cpp
#    i386/EncoderMD.cpp
#    i386/LinearScanMD.cpp
#    i386/LowererMDArch.cpp
#    i386/PeepsMD.cpp
    )

target_include_directories (
    Chakra.Backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    amd64
    ../Common
    ../JITIDL
    ../Runtime
    ../Runtime/ByteCode
    ../Runtime/Math
    ../Parser
    )
