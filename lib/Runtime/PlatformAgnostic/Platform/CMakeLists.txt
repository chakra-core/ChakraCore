project(CHAKRA_PLATFORM_AGNOSTIC)

check_function_exists(gmtime_r HAVE_GMTIME_R)

add_library (Chakra.Runtime.PlatformAgnostic
  Linux/UnicodeText.ICU.cpp
  Linux/DateTime.cpp
  Linux/HiResTimer.cpp
  Common/UnicodeText.Common.cpp
  )
  
target_include_directories (
    Chakra.Runtime.PlatformAgnostic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
