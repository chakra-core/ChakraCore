add_library (Chakra.Runtime.Base OBJECT
    CallInfo.cpp
    CharStringCache.cpp
    Constants.cpp
    CrossSite.cpp
    Debug.cpp
    # DelayLoadLibrary.cpp
    # Entropy.cpp
    EtwTrace.cpp
    Exception.cpp
    ExpirableObject.cpp
    FunctionBody.cpp
    FunctionInfo.cpp
    LeaveScriptObject.cpp
    LineOffsetCache.cpp
    PerfHint.cpp
    PropertyRecord.cpp
    RuntimeBasePch.cpp
    ScriptContext.cpp
    ScriptContextOptimizationOverrideInfo.cpp
    ScriptContextProfiler.cpp
    ScriptMemoryDumper.cpp
    SourceContextInfo.cpp
    SourceHolder.cpp
    StackProber.cpp
    TempArenaAllocatorObject.cpp
    TestEtwEventSink.cpp
    ThreadBoundThreadContextManager.cpp
    ThreadContextInfo.cpp
    ThreadContext.cpp
    ThreadContextTlsEntry.cpp
    ThreadServiceWrapperBase.cpp
    Utf8SourceInfo.cpp
    VTuneChakraProfile.cpp
    # WindowsFoundationAdapter.cpp
    WindowsGlobalizationAdapter.cpp
    jitprofiling.cpp
    )

target_include_directories (
    Chakra.Runtime.Base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
