project(CHAKRA_PLATFORM_AGNOSTIC)

add_library (Chakra.Runtime.PlatformAgnostic
  Linux/UnicodeText.ICU.cpp
  Common/UnicodeText.Common.cpp
  )
  
target_include_directories (
    Chakra.Runtime.PlatformAgnostic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
