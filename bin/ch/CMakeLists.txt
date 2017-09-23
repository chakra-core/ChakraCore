set(ch_source_files
  ch.cpp
  ChakraRtInterface.cpp
  CodexAssert.cpp
  Debugger.cpp
  Helpers.cpp
  HostConfigFlags.cpp
  WScriptJsrt.cpp
  RuntimeThreadData.cpp
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

if(CC_TARGETS_AMD64)
    set(Wasm_dep ../../lib/WasmReader)
endif()

target_include_directories (ch
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  ../ChakraCore
  ../../lib/Common
  ../../lib/Jsrt
  ../../lib/Runtime
  ../../lib/Parser
  ${Wasm_dep}
  )

if(STATIC_LIBRARY)
  if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
    set(LINKER_START_GROUP -pie -Wl,--start-group)
    set(LINKER_END_GROUP -Wl,--end-group)
    if (CC_TARGET_OS_LINUX) # do not embed libstdc++ statically on Android
      set(LINKER_END_GROUP "${LINKER_END_GROUP} -static-libstdc++")
    else()
      set(LINKER_END_GROUP "${LINKER_END_GROUP} -llog")
    endif()
  elseif(CC_TARGET_OS_OSX)
    set(LINKER_START_GROUP -Wl,-force_load,)
  endif()

  if (NOT CC_TARGET_OS_ANDROID)
    set(LINKER_END_GROUP
        "${LINKER_END_GROUP}"
        pthread
        )
  endif()

  # common link deps
  set(lib_target "${lib_target}"
    -Wl,-undefined,error
    ${LINKER_START_GROUP}
    ChakraCoreStatic
    ${LINKER_END_GROUP}
    dl
    ${ICULIB}
    )

  if(CC_TARGET_OS_OSX)
    set(lib_target "${lib_target}"
      "-framework CoreFoundation"
      "-framework Security"
      )
  elseif (NOT CC_TARGET_OS_ANDROID)
    set(lib_target "${lib_target}"
      "pthread"
      )
  endif()

else() # // shared library below
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE")

  if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie") # osx clang sets this by default
  endif()

  set(lib_target
    PRIVATE Chakra.Pal.Singular
    PRIVATE Chakra.Common.Codex.Singular
    PRIVATE Chakra.Runtime.PlatformAgnostic.Singular
    )
endif()

if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
  set(lib_target "${lib_target}"
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ch.version
    ${LINKER_REDUCED_MEMORY}
    )
elseif(CC_TARGET_OS_OSX)
  if(CC_TARGETS_X86)
    set(lib_target "${lib_target} -arch i386")
  elseif(CC_TARGETS_ARM)
    set(lib_target "${lib_target} -arch arm")
  endif()
endif()


target_link_libraries (ch
  ${lib_target}
  ${CC_LTO_ENABLED}
  )

if(NOT CC_XCODE_PROJECT)
  # Add a post build event to the ch target
  # which executes a command to copy ch to
  # out/{BUILD_TYPE}/ for convenience
  add_custom_command(TARGET ch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CHAKRACORE_BINARY_DIR}/bin/ch/ch"
    ${CHAKRACORE_BINARY_DIR}/)
endif()
