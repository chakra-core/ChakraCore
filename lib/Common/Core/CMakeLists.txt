add_library (Chakra.Common.Core STATIC
    Api.cpp
    BinaryFeatureControl.cpp
    CmdParser.cpp
    CodexAssert.cpp
    CommonCorePch.cpp
    ConfigFlagsTable.cpp
    ConfigParser.cpp
    DbgHelpSymbolManager.cpp
    DelayLoadLibrary.cpp
    EtwTraceCore.cpp
    FaultInjection.cpp
    Output.cpp
    PerfCounter.cpp
    PerfCounterImpl.cpp
    PerfCounterSet.cpp
    ProfileInstrument.cpp
    ProfileMemory.cpp
    StackBackTrace.cpp
    SysInfo.cpp
)

target_include_directories (
    Chakra.Common.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CC_TARGET_OS_ANDROID)
    add_custom_command(TARGET Chakra.Common.Core POST_BUILD
        COMMAND ${ANDROID_TOOLCHAIN_DIR}/bin/ranlib ${CMAKE_CURRENT_BINARY_DIR}/libChakra.Common.Core.a
    )
endif()
