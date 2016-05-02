add_library (ChakraCore SHARED
    ChakraCoreDllFunc.cpp
    ConfigParserExternals.cpp
    TestHooks.cpp
)

# By default, don't export any symbols from this library
# We'll manually export the relevant individual functions
set_target_properties(ChakraCore PROPERTIES CXX_VISIBILITY_PRESET hidden)
  
target_include_directories (
  ChakraCore PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHAKRACORE_SOURCE_DIR}/lib/Common
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime
  ${CHAKRACORE_SOURCE_DIR}/lib/Parser
  ${CHAKRACORE_SOURCE_DIR}/lib/Jsrt
  )

#
# Link step for the ChakraCore shared library
#
# This requires that the library have no undefined
# symbols after linking
# Chakra.Jsrt and Chakra.Jsrt.Core have their archives
# imported wholesale since all functions used in these
# libraries are considered to be used by Chakra
#
# External libraries we link with are the following:
#  pthread: For threading
#  stdc++/gcc_s: C++ runtime code
#  dl: For shared library loading related functions
#  icuuc: For the ICU (xplat-todo: Make this optional)
#
target_link_libraries(ChakraCore
  -Wl,--no-undefined
  -Wl,--start-group
  -Wl,--whole-archive
  Chakra.Jsrt
  Chakra.Jsrt.Core
  -Wl,--no-whole-archive
  Chakra.Runtime.Types
  Chakra.Runtime.Math
  Chakra.Runtime.Library
  Chakra.Runtime.Language
  Chakra.Runtime.Debug
  Chakra.Runtime.ByteCode
  Chakra.Runtime.PlatformAgnostic
  Chakra.Runtime.Base
  Chakra.Parser
  Chakra.Common.Util
  Chakra.Common.Memory
  Chakra.Common.Common
  Chakra.Common.Core
  Chakra.Common.DataStructures
  Chakra.Common.Exceptions
  Chakra.Common.Codex
  -Wl,--end-group
  Chakra.Pal
  pthread
  stdc++
  dl
  icuuc
  -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libChakraCoreLib.version
)

#
# Post build step to copy the built shared library
# to BuildLinux (or whatever the CMakeBuildDir is)
add_custom_command(TARGET ChakraCore POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different 
  "${CHAKRACORE_BINARY_DIR}/bin/ChakraCore/libChakraCore.so"
   ${CHAKRACORE_BINARY_DIR}/)
