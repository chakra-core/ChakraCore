if(BuildJIT)
    add_definitions(-D_ENABLE_DYNAMIC_THUNKS=1)
endif()

set(CRL_SOURCE_FILES ${CRL_SOURCE_FILES}
    AsmJs.cpp
    AsmJsByteCodeGenerator.cpp
    AsmJsCodeGenerator.cpp
    AsmJsEncoder.cpp
    AsmJsLink.cpp
    AsmJsModule.cpp
    AsmJsTypes.cpp
    AsmJsUtils.cpp
    WAsmjsUtils.cpp
    WebAssemblySource.cpp
    CacheOperators.cpp
    ConstructorCache.cpp
    CodeGenRecyclableData.cpp
    DynamicProfileInfo.cpp
    DynamicProfileMutator.cpp
    DynamicProfileStorage.cpp
    ExecutionMode.cpp
    FunctionCodeGenRuntimeData.cpp
    InlineCache.cpp
    InterpreterStackFrame.cpp
    JavascriptConversion.cpp
    JavascriptExceptionObject.cpp
    JavascriptExceptionOperators.cpp
    JavascriptMathOperators.cpp
    JavascriptOperators.cpp
    JavascriptStackWalker.cpp
    ModuleNamespace.cpp
    ModuleNamespaceEnumerator.cpp
    ProfilingHelpers.cpp
    RuntimeLanguagePch.cpp
    SimdBool16x8Operation.cpp
    SimdBool16x8OperationX86X64.cpp
    SimdBool32x4Operation.cpp
    SimdBool32x4OperationX86X64.cpp
    SimdBool8x16Operation.cpp
    SimdBool8x16OperationX86X64.cpp
    SimdFloat32x4Operation.cpp
    SimdFloat32x4OperationX86X64.cpp
    SimdFloat64x2Operation.cpp
    SimdFloat64x2OperationX86X64.cpp
    SimdInt16x8Operation.cpp
    SimdInt16x8OperationX86X64.cpp
    SimdInt32x4Operation.cpp
    SimdInt32x4OperationX86X64.cpp
    SimdInt8x16Operation.cpp
    SimdInt8x16OperationX86X64.cpp
    SimdUint16x8Operation.cpp
    SimdUint16x8OperationX86X64.cpp
    SimdUint32x4Operation.cpp
    SimdUint32x4OperationX86X64.cpp
    SimdUint8x16Operation.cpp
    SimdUint8x16OperationX86X64.cpp
    SimdUtils.cpp
    SourceDynamicProfileManager.cpp
    SourceTextModuleRecord.cpp
    StackTraceArguments.cpp
    TaggedInt.cpp
    ValueType.cpp
    )

if(CC_TARGETS_AMD64)
    set (CRL_SOURCE_FILES ${CRL_SOURCE_FILES}
        amd64/AsmJsJitTemplate.cpp
        amd64/StackFrame.SystemV.cpp
        amd64/JavascriptOperatorsA.S
        amd64/amd64_Thunks.S
    )
elseif(CC_TARGETS_X86)
    set (CRL_SOURCE_FILES ${CRL_SOURCE_FILES}
        i386/AsmJsJitTemplate.cpp
        i386/StackFrame.cpp
    )
elseif(CC_TARGETS_ARM)
    set (CRL_SOURCE_FILES ${CRL_SOURCE_FILES}
        arm/StackFrame.cpp
        arm/arm_Thunks.S
    )
endif()

add_library (Chakra.Runtime.Language OBJECT ${CRL_SOURCE_FILES})

include_directories (
    ../Math
    )

target_include_directories (
    Chakra.Runtime.Language PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
