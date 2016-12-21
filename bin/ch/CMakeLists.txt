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

if(NOT ENABLE_CC_XPLAT_TRACE)
  add_compile_options(-fPIC)

  set_target_properties(ch
    PROPERTIES
    POSITION_INDEPENDENT_CODE True
    )

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE")

  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie") # osx clang sets this by default
  endif()
else()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-PIE")
endif()

include_directories(..)

target_include_directories (ch
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  ../ChakraCore
  ../../lib/Common
  ../../lib/Jsrt
  ../../lib/Runtime
  ../../lib/Parser
  )

if(STATIC_LIBRARY)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(LINKER_START_GROUP -Wl,--start-group)
    set(LINKER_END_GROUP -Wl,--end-group -static-libstdc++)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(LINKER_START_GROUP -Wl,-force_load,)
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
    dl
    ${ICULIB}
    )

  if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(lib_target "${lib_target}"
      "-framework CoreFoundation"
      "-framework Security"
      )
  endif() # Linux ?
else() # // shared library below
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE")

  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie") # osx clang sets this by default
  endif()

  set(lib_target
    PRIVATE Chakra.Pal
    PRIVATE Chakra.Common.Codex.Singular
    PRIVATE Chakra.Runtime.PlatformAgnostic.Singular
    )
endif()

if(CC_TARGETS_X86)
  set(lib_target "${lib_target} -arch i386")
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
