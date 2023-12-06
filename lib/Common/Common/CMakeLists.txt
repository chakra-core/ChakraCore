if(CC_TARGETS_ARM)
    set(ARCH_SOURCES arm/arm_GET_CURRENT_FRAME.S)
elseif(CC_TARGETS_ARM64)
    set(ARCH_SOURCES arm64/arm64_GET_CURRENT_FRAME.S)
endif()

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
    ${ARCH_SOURCES}
)

target_include_directories (
    Chakra.Common.Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
