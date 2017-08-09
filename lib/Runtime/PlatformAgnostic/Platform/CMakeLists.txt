project(CHAKRA_PLATFORM_AGNOSTIC)

set(PL_SOURCE_FILES
  Common/UnicodeText.Common.cpp
  Common/HiResTimer.cpp
  Common/DateTime.cpp
  Linux/EventTrace.cpp
  Linux/UnicodeText.ICU.cpp
  Linux/NumbersUtility.cpp
  Linux/Thread.cpp
  Linux/PerfTrace.cpp
  Common/Trace.cpp
  Common/SystemInfo.Common.cpp
  )

if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
set(PL_SOURCE_FILES ${PL_SOURCE_FILES}
  Linux/SystemInfo.cpp
  )
elseif(CC_TARGET_OS_OSX)
set(PL_SOURCE_FILES ${PL_SOURCE_FILES}
  Unix/AssemblyCommon.cpp
  Unix/SystemInfo.cpp
  )
endif()

if(NOT STATIC_LIBRARY)
  # CH has a direct dependency to this project
  add_library (Chakra.Runtime.PlatformAgnostic.Singular STATIC
    ${PL_SOURCE_FILES}
    )
  target_include_directories (
    Chakra.Runtime.PlatformAgnostic.Singular PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

add_library (Chakra.Runtime.PlatformAgnostic OBJECT
  ${PL_SOURCE_FILES}
  )

target_include_directories (
  Chakra.Runtime.PlatformAgnostic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
