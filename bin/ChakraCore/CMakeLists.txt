if(BuildJIT)
    set(chakra_backend_objects $<TARGET_OBJECTS:Chakra.Backend>)
endif()

if(CC_TARGETS_AMD64)
    set(wasm_objects $<TARGET_OBJECTS:Chakra.WasmReader>)
    set(wasm_includes ${CHAKRACORE_SOURCE_DIR}/lib/WasmReader)
endif()

add_library (ChakraCoreStatic STATIC
  $<TARGET_OBJECTS:Chakra.Pal>
  $<TARGET_OBJECTS:Chakra.Common.Core>
  $<TARGET_OBJECTS:Chakra.Jsrt>
  $<TARGET_OBJECTS:Chakra.Jsrt.Core>
  ${chakra_backend_objects}
  $<TARGET_OBJECTS:Chakra.Common.Common>
  $<TARGET_OBJECTS:Chakra.Common.Codex>
  $<TARGET_OBJECTS:Chakra.Common.DataStructures>
  $<TARGET_OBJECTS:Chakra.Common.Exceptions>
  $<TARGET_OBJECTS:Chakra.Common.Memory>
  $<TARGET_OBJECTS:Chakra.Common.Util>
  $<TARGET_OBJECTS:Chakra.Runtime.Base>
  $<TARGET_OBJECTS:Chakra.Runtime.ByteCode>
  $<TARGET_OBJECTS:Chakra.Runtime.Debug>
  $<TARGET_OBJECTS:Chakra.Runtime.Language>
  $<TARGET_OBJECTS:Chakra.Runtime.Library>
  $<TARGET_OBJECTS:Chakra.Runtime.Math>
  $<TARGET_OBJECTS:Chakra.Runtime.Types>
  $<TARGET_OBJECTS:Chakra.Runtime.PlatformAgnostic>
  $<TARGET_OBJECTS:Chakra.Parser>
  ${wasm_objects}
)

if(CC_TARGET_OS_OSX)
  target_link_libraries(ChakraCoreStatic
    "-framework CoreFoundation"
    "-framework Security"
    )
else()
  if (NOT CC_TARGET_OS_ANDROID)
    set(PTHREAD "pthread")
  endif()

  target_link_libraries(ChakraCoreStatic
    ${CC_LTO_ENABLED}
    ${PTHREAD}
    "dl"
    )
endif()

target_include_directories (
  ChakraCoreStatic PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHAKRACORE_SOURCE_DIR}/lib/Backend
  ${CHAKRACORE_SOURCE_DIR}/lib/Common
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime/ByteCode
  ${CHAKRACORE_SOURCE_DIR}/lib/Parser
  ${CHAKRACORE_SOURCE_DIR}/lib/Jsrt
  ${wasm_includes}
  )

if (NOT STATIC_LIBRARY)
  add_library (ChakraCore SHARED
    ChakraCoreShared.cpp
    ConfigParserExternals.cpp
    TestHooks.cpp
  )

  #
  # Link step for the ChakraCore shared library
  #
  # External libraries we link with are the following:
  #  pthread: For threading
  #  stdc++/gcc_s: C++ runtime code
  #  dl: For shared library loading related functions
  #
  if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
    set(LINKER_START_GROUP
      -fPIC
      -Wl,--start-group
      -Wl,--whole-archive
      )

    set(LINKER_END_GROUP
      -Wl,--no-whole-archive
      -Wl,--end-group
      -static-libstdc++
      )
  elseif(CC_TARGET_OS_OSX)
    set(LINKER_START_GROUP -Wl,-force_load,)
  endif()

  # common link deps
  set(lib_target "${lib_target}"
    -Wl,-undefined,error
    ${LINKER_START_GROUP}
    ChakraCoreStatic
    ${LINKER_END_GROUP}
    pthread
    dl
    ${ICULIB}
    )

  if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
    set(lib_target "${lib_target}"
      -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libChakraCoreLib.version
      # reduce link time memory usage
      -Xlinker --no-keep-memory
      )
  elseif(CC_TARGET_OS_OSX)
    if(CC_TARGETS_X86)
      set(lib_target "${lib_target} -arch i386")
    elseif(CC_TARGETS_ARM)
      set(lib_target "${lib_target} -arch arm")
    endif()
  endif()

  target_link_libraries (ChakraCore
    ${lib_target}
    ${CC_LTO_ENABLED}
    )

  if(NOT CC_XCODE_PROJECT)
    # Post build step to copy the built shared library
    # to out/{BUILD_TYPE}/ (or whatever the CMakeBuildDir is)
    add_custom_command(TARGET ChakraCore POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CHAKRACORE_BINARY_DIR}/bin/ChakraCore/libChakraCore.${DYN_LIB_EXT}"
      ${CHAKRACORE_BINARY_DIR}/
      )
  endif()
endif()
