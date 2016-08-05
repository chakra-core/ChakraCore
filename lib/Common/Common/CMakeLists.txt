add_library (Chakra.Common.Common OBJECT
    CfgLogger.cpp
    CommonCommonPch.cpp
    DateUtilities.cpp
    Event.cpp
    Int32Math.cpp
    Int64Math.cpp
    Jobs.cpp
    MathUtil.cpp
    NumberUtilities.cpp
    NumberUtilities_strtod.cpp
    RejitReason.cpp
    SmartFpuControl.cpp
    Tick.cpp
    vtinfo.cpp
)

target_include_directories (
    Chakra.Common.Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
