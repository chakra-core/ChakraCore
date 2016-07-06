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
  )

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE")

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie") # osx clang sets this by default
endif()

if(STATIC_LIBRARY)
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
else() # // !from shared library
  set(lib_target
    PRIVATE Chakra.Pal
    PRIVATE Chakra.Common.Codex
    PRIVATE Chakra.Runtime.PlatformAgnostic
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
