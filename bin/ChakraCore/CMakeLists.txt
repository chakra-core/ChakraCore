add_library (ChakraCore SHARED
  ChakraCoreDllFunc.cpp
  ConfigParserExternals.cpp
  TestHooks.cpp
)
  
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
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(lib_target
    -Wl,--no-undefined
    -Wl,--start-group
    -Wl,--whole-archive
    Chakra.Jsrt
    Chakra.Jsrt.Core
    -Wl,--no-whole-archive
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(lib_target
    -Wl,-undefined,error
    Chakra.Jsrt
    Chakra.Jsrt.Core
    )
else()
  message("This platform is not yet supported")
endif() # Linux ?

# make sure to include Common.Core before others
# this will help linker on some platforms for correct
# initialization order
set(lib_target "${lib_target}"
  Chakra.Common.Core
  )

# common link deps
set(lib_target "${lib_target}"
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
  Chakra.Common.DataStructures
  Chakra.Common.Exceptions
  Chakra.Common.Codex
  )

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(lib_target "${lib_target}"
    -Wl,--end-group
    Chakra.Pal
    pthread
    stdc++
    dl
    icuuc
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(lib_target "${lib_target}"
    Chakra.Pal
    pthread
    stdc++
    dl
    icucore
    )
endif() # Linux ?

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(lib_target "${lib_target}"
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libChakraCoreLib.version
    )
endif()

target_link_libraries (ChakraCore ${lib_target})

if(NOT CC_XCODE_PROJECT)
  set(CC_LIB_EXT "so")
  # Post build step to copy the built shared library
  # to BuildLinux (or whatever the CMakeBuildDir is)
  if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(CC_LIB_EXT "dylib")
  endif()
  
  add_custom_command(TARGET ChakraCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CHAKRACORE_BINARY_DIR}/bin/ChakraCore/libChakraCore.${CC_LIB_EXT}"
    ${CHAKRACORE_BINARY_DIR}/
    )
endif()
