set (SOURCES
  Api.cpp
  cfglogger.cpp
  CommonCommonPch.cpp
  # DateUtilities.cpp
  # DaylightTimeHelper.cpp
  Event.cpp
  Int32Math.cpp
  Int64Math.cpp
  # Jobs.cpp
  # MathUtil.cpp
  # NumberUtilities.cpp
  # NumberUtilities_strtod.cpp
  # RejitReason.cpp
  # SmartFPUControl.cpp
  Tick.cpp
  unicode.cpp
  vtinfo.cpp
)

add_library (Chakra.Common.common
  ${SOURCES}
)

target_include_directories (Chakra.Common.common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
