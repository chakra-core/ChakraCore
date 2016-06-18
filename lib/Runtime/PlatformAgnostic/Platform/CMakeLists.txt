project(CHAKRA_PLATFORM_AGNOSTIC)

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
  # xplat-todo: implement them for unix! or carry them into common
  Linux/UnicodeText.ICU.cpp
  Linux/DateTime.cpp
  Linux/HiResTimer.cpp
  Linux/NumbersUtility.cpp
  Unix/SystemInfo.cpp
  Common/UnicodeText.Common.cpp
  )
endif()
  
target_include_directories (
    Chakra.Runtime.PlatformAgnostic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
