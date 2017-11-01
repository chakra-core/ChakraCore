add_library (Chakra.Runtime.Debug OBJECT
    BreakpointProbe.cpp
    DebugContext.cpp
    DebugDocument.cpp
    DebugManager.cpp
    DebuggingFlags.cpp
    DiagHelperMethodWrapper.cpp
    DiagObjectModel.cpp
    DiagProbe.cpp
    DiagStackFrame.cpp
    MutationBreakpoint.cpp
    ProbeContainer.cpp
    RuntimeDebugPch.cpp
    TTActionEvents.cpp
    TTEventLog.cpp
    TTEvents.cpp
    TTExecutionInfo.cpp
    TTInflateMap.cpp
    TTRuntimeInfoTracker.cpp
    TTSerialize.cpp
    TTSnapObjects.cpp
    TTSnapshot.cpp
    TTSnapshotExtractor.cpp
    TTSnapTypes.cpp
    TTSnapValues.cpp
    TTSupport.cpp
    )

target_include_directories (
    Chakra.Runtime.Debug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
