set (SOURCES
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

add_library (Chakra.Common.core
  ${SOURCES}
)

target_include_directories (Chakra.Common.core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
