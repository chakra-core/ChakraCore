add_executable (ch
  ch.cpp
  ChakraRtInterface.cpp
  CodexAssert.cpp
  Debugger.cpp
  Helpers.cpp
  HostConfigFlags.cpp
  WScriptJsrt.cpp
  )

set_target_properties(ch
  PROPERTIES
  POSITION_INDEPENDENT_CODE True
  )

include_directories(..)

target_include_directories (ch
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  ../ChakraCore
  ../../lib/Common
  ../../lib/Jsrt
  )

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")

target_link_libraries (ch
  PRIVATE Chakra.Pal
  PRIVATE Chakra.Common.Codex
  -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ch.version
  )

# Add a post build event to the ch target
# which executes a command to copy ch to
# BuildLinux for convenience
add_custom_command(TARGET ch POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CHAKRACORE_BINARY_DIR}/bin/ch/ch"
  ${CHAKRACORE_BINARY_DIR}/)
