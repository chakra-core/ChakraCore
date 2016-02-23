# xplat-todo: This is a skeleton make file and not used in build yet.
# Please add this to build and fix issues.

add_library (Chakra.Runtime.Base
    CallInfo.cpp
    CharStringCache.cpp
    Constants.cpp
    CrossSite.cpp
    Debug.cpp
    DelayLoadLibrary.cpp
    Entropy.cpp
    EtwTrace.cpp
    Exception.cpp
    ExpirableObject.cpp
    FunctionBody.cpp
    FunctionInfo.cpp
    HiResTimer.cpp
    LeaveScriptObject.cpp
    PerfHint.cpp
    PropertyRecord.cpp
    RuntimeBasePch.cpp
    ScriptContext.cpp
    ScriptContextOptimizationOverrideInfo.cpp
    ScriptContextProfiler.cpp
    ScriptMemoryDumper.cpp
    SourceHolder.cpp
    StackProber.cpp
    TempArenaAllocatorObject.cpp
    TestEtwEventSink.cpp
    ThreadBoundThreadContextManager.cpp
    ThreadContext.cpp
    ThreadContextTlsEntry.cpp
    ThreadServiceWrapperBase.cpp
    Utf8SourceInfo.cpp
    WindowsFoundationAdapter.cpp
    WindowsGlobalizationAdapter.cpp
    )

target_include_directories (
    Chakra.Runtime.Base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
