add_library (ChakraCore SHARED
  ChakraCoreShared.cpp
  ConfigParserExternals.cpp
  TestHooks.cpp
)

target_include_directories (
  ChakraCore PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHAKRACORE_SOURCE_DIR}/lib/Backend
  ${CHAKRACORE_SOURCE_DIR}/lib/Common
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime/ByteCode
  ${CHAKRACORE_SOURCE_DIR}/lib/Parser
  ${CHAKRACORE_SOURCE_DIR}/lib/Jsrt
  ${wasm_includes}
  "-fvisibility=hidden"
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
    ${LINKER_REDUCED_MEMORY}
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
