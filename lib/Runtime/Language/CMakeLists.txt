if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
    set( ARCH_CHAKRA_RUNTIME_LANGUAGE
        amd64/JavascriptOperatorsA.S
        )
endif()

add_library (Chakra.Runtime.Language OBJECT
    AsmJs.cpp
    AsmJsByteCodeGenerator.cpp
    AsmJsCodeGenerator.cpp
    AsmJsEncoder.cpp
    AsmJsLink.cpp
    AsmJsModule.cpp
    AsmJsTypes.cpp
    AsmJsUtils.cpp
    CacheOperators.cpp
    CodeGenRecyclableData.cpp
    DynamicProfileInfo.cpp
    DynamicProfileMutator.cpp
    DynamicProfileStorage.cpp
    ExecutionMode.cpp
    FunctionCodeGenJitTimeData.cpp
    FunctionCodeGenRuntimeData.cpp
    InlineCache.cpp
    InterpreterStackFrame.cpp
    JavascriptConversion.cpp
    JavascriptExceptionObject.cpp
    JavascriptExceptionOperators.cpp
    JavascriptMathOperators.cpp
    JavascriptNativeOperators.cpp
    JavascriptOperators.cpp
    JavascriptStackWalker.cpp
    ModuleNamespace.cpp
    ModuleNamespaceEnumerator.cpp
    ProfilingHelpers.cpp
    #ReadOnlyDynamicProfileInfo.cpp
    RuntimeLanguagePch.cpp
    # SimdBool16x8Operation.cpp
    # SimdBool16x8OperationX86X64.cpp
    # SimdBool32x4Operation.cpp
    # SimdBool32x4OperationX86X64.cpp
    # SimdBool8x16Operation.cpp
    # SimdBool8x16OperationX86X64.cpp
    # SimdFloat32x4Operation.cpp
    # SimdFloat32x4OperationX86X64.cpp
    # SimdFloat64x2Operation.cpp
    # SimdFloat64x2OperationX86X64.cpp
    # SimdInt16x8Operation.cpp
    # SimdInt16x8OperationX86X64.cpp
    # SimdInt32x4Operation.cpp
    # SimdInt32x4OperationX86X64.cpp
    # SimdInt8x16Operation.cpp
    # SimdInt8x16OperationX86X64.cpp
    # SimdUint16x8Operation.cpp
    # SimdUint16x8OperationX86X64.cpp
    # SimdUint32x4Operation.cpp
    # SimdUint32x4OperationX86X64.cpp
    # SimdUint8x16Operation.cpp
    # SimdUint8x16OperationX86X64.cpp
    # SimdUtils.cpp
    SourceDynamicProfileManager.cpp
    SourceTextModuleRecord.cpp
    StackTraceArguments.cpp
    TaggedInt.cpp
    ValueType.cpp
    amd64/AsmJsJitTemplate.cpp
    amd64/StackFrame.SystemV.cpp
    # arm64/StackFrame.cpp
    # arm/StackFrame.cpp
    # i386/AsmJsJitTemplate.cpp
    # i386/StackFrame.cpp
    ${ARCH_CHAKRA_RUNTIME_LANGUAGE}
    )

include_directories (
    ../Math
    )

target_include_directories (
    Chakra.Runtime.Language PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
