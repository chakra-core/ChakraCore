set(ch_source_files
  ch.cpp
  ChakraRtInterface.cpp
  Debugger.cpp
  Helpers.cpp
  HostConfigFlags.cpp
  WScriptJsrt.cpp
  )

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(ch_source_files ${ch_source_files}
    $<TARGET_OBJECTS:Chakra.Pal>
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

set_target_properties(ch PROPERTIES LINK_FLAGS "-fPIE") # osx clang sets this by default
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set_target_properties(ch PROPERTIES LINK_FLAGS "-pie") # osx clang sets this by default
endif()

if(STATIC_LIBRARY)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(LINKER_START_GROUP -Wl,--start-group)
    set(LINKER_END_GROUP -Wl,--end-group)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(LINKER_START_GROUP -Wl,-force_load,)
  endif()
  # common link deps
  set(lib_target "${lib_target}"
    ChakraCoreStatic
    pthread
    stdc++
    dl
  )
  if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(lib_target "${lib_target}"
      "-framework CoreFoundation"
      "-framework Security"
    )
    target_sources(ch PRIVATE
      $<TARGET_OBJECTS:Chakra.Pal>
    )
  endif()
else() # // shared library below
  target_sources(ch PRIVATE
    CodexAssert.cpp
    )

  set(lib_target "${lib_target}"
    PRIVATE Chakra.Common.Codex.Singular
    PRIVATE Chakra.Runtime.PlatformAgnostic.Singular
    pthread
    dl
    ${ICULIB}
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(lib_target "${lib_target}"
    "-framework CoreFoundation"
    "-framework Security"
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
  if(CC_TARGETS_AMD64)
      set(lib_target "${lib_target}"
        unwind-x86_64
      )
  endif()

  set(lib_target "${lib_target}"
    gcc_s
    rt
    unwind
    unwind-generic
  )
endif()

if(NOT CC_XCODE_PROJECT)
  if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(lib_target "${lib_target}"
      "-stdlib=libstdc++" # expected 'deprecated' warning. needed for std::string
      )
  endif()
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
