project(CHAKRA_PLATFORM_AGNOSTIC)

check_function_exists(gmtime_r HAVE_GMTIME_R)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
add_library (Chakra.Runtime.PlatformAgnostic
  Linux/UnicodeText.ICU.cpp
  Linux/DateTime.cpp
  Linux/HiResTimer.cpp
  Linux/NumbersUtility.cpp
  Linux/SystemInfo.cpp
  Common/UnicodeText.Common.cpp
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
add_library (Chakra.Runtime.PlatformAgnostic
  #Linux/UnicodeText.ICU.cpp
  #Linux/DateTime.cpp
  #Linux/HiResTimer.cpp
  Unix/SystemInfo.cpp
  Common/UnicodeText.Common.cpp
  )
endif()

  
target_include_directories (
    Chakra.Runtime.PlatformAgnostic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
