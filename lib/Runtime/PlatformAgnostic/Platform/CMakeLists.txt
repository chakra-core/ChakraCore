project(CHAKRA_PLATFORM_AGNOSTIC)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
set(PL_SOURCE_FILES
  Linux/UnicodeText.ICU.cpp
  Linux/DateTime.cpp
  Linux/HiResTimer.cpp
  Linux/NumbersUtility.cpp
  Linux/SystemInfo.cpp
  Common/UnicodeText.Common.cpp
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
set(PL_SOURCE_FILES
  # xplat-todo: implement them for unix! or carry them into common
  Linux/UnicodeText.ICU.cpp
  Unix/DateTime.cpp
  Linux/HiResTimer.cpp
  Linux/NumbersUtility.cpp
  Unix/SystemInfo.cpp
  Common/UnicodeText.Common.cpp
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
