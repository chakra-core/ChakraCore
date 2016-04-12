add_executable (ch
  ch.cpp
  ChakraRtInterface.cpp
  CodexAssert.cpp
  Helpers.cpp
  HostConfigFlags.cpp
  WScriptJsrt.cpp
    )

include_directories(..)

target_include_directories (ch
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  ../ChakraCore
  ../../lib/Common
  ../../lib/Jsrt
  )

target_link_libraries (ch
  PRIVATE Chakra.Pal
  PRIVATE Chakra.Common.Codex
  )

