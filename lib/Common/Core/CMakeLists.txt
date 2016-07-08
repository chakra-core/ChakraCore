add_library (Chakra.Common.Core STATIC
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

if(NOT CC_XCODE_PROJECT)
  if (STATIC_LIBRARY)
    add_custom_command(TARGET Chakra.Common.Core POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CHAKRACORE_BINARY_DIR}/lib/Common/Core/libChakra.Common.Core.a"
      "${CHAKRACORE_BINARY_DIR}/../bin/"
      )
  endif()
endif()
