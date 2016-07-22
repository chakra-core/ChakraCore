set(ch_source_files
  ch.cpp
  ChakraRtInterface.cpp
  CodexAssert.cpp
  Debugger.cpp
  Helpers.cpp
  HostConfigFlags.cpp
  WScriptJsrt.cpp
  )

if (STATIC_LIBRARY)
  set(ch_source_files "${ch_source_files}"
    ../ChakraCore/TestHooks.cpp
    )
endif()

add_executable (ch ${ch_source_files})

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
  ../../lib/Runtime
  ../../lib/Parser
  )

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE")

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie") # osx clang sets this by default
endif()

if(STATIC_LIBRARY)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(ICULIB "icuuc")
    set(LINKER_START_GROUP -Wl,--start-group)
    set(LINKER_END_GROUP -Wl,--end-group)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(ICULIB "icucore")
  endif()

  if(CC_EMBED_ICU)
    set(ICU_CC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../deps/icu/source/output/lib/")
    find_library(ICUUC icuuc PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
    find_library(ICU18 icui18n PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
    find_library(ICUDATA icudata PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
    set(ICULIB
      ${ICUUC}
      ${ICU18}
      ${ICUDATA}
      )
  endif()

  # common link deps
  set(lib_target "${lib_target}"
    -Wl,-undefined,error
    ${LINKER_START_GROUP}
    Chakra.Pal
    Chakra.Common.Core
    Chakra.Jsrt
    ${LINKER_END_GROUP}
    pthread
    stdc++
    dl
  )

  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(lib_target "${lib_target}"
      ${ICULIB}
      unwind-x86_64
      )
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(lib_target "${lib_target}"
      ${ICULIB}
      "-framework CoreFoundation"
      "-framework Security"
      # set stack size to 64Mb for stack tests
      -Wl,-stack_size,0x04000000
      )
  endif() # Linux ?
else() # // !from shared library
  set(lib_target
    PRIVATE Chakra.Pal
    PRIVATE Chakra.Common.Codex.Singular
    PRIVATE Chakra.Runtime.PlatformAgnostic.Singular
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(lib_target "${lib_target}"
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ch.version
    )
endif()

target_link_libraries (ch ${lib_target})

if(NOT CC_XCODE_PROJECT)
  # Add a post build event to the ch target
  # which executes a command to copy ch to
  # BuildLinux for convenience
  add_custom_command(TARGET ch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CHAKRACORE_BINARY_DIR}/bin/ch/ch"
    ${CHAKRACORE_BINARY_DIR}/)
endif()
