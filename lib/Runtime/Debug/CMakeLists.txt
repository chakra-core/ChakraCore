add_library (Chakra.Runtime.Debug STATIC
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
    SourceContextInfo.cpp
    )

target_include_directories (
    Chakra.Runtime.Debug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
