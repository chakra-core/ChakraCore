

add_library (ChakraCoreStatic STATIC 
  ChakraCoreCommon.cpp
  ConfigParserExternals.cpp
  TestHooks.cpp
  ${jsrt_objects}
  )

add_library (ChakraCore SHARED
  ChakraCoreCommon.cpp
  ChakraCoreDllFunc.cpp
  ConfigParserExternals.cpp
  TestHooks.cpp
  ${jsrt_objects}
  )

target_compile_options(ChakraCoreStatic PRIVATE "-fPIC")
target_compile_definitions(ChakraCoreStatic
  PUBLIC
    -DCHAKRA_STATIC_LIBRARY=1
  )

target_include_directories (
  ChakraCoreStatic PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHAKRACORE_SOURCE_DIR}/lib/Common
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime
  ${CHAKRACORE_SOURCE_DIR}/lib/Parser
  ${CHAKRACORE_SOURCE_DIR}/lib/Jsrt
  ${CHAKRACORE_SOURCE_DIR}/lib/JITIDL
  ${CHAKRACORE_SOURCE_DIR}/lib/Backend
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime/ByteCode/
  )

target_include_directories (
  ChakraCore PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHAKRACORE_SOURCE_DIR}/lib/Common
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime
  ${CHAKRACORE_SOURCE_DIR}/lib/Parser
  ${CHAKRACORE_SOURCE_DIR}/lib/Jsrt
  ${CHAKRACORE_SOURCE_DIR}/lib/JITIDL
  ${CHAKRACORE_SOURCE_DIR}/lib/Backend
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime/ByteCode/
  )

#
# Link step for the ChakraCore shared library
#
# External libraries we link with are the following:
#  pthread: For threading
#  stdc++/gcc_s: C++ runtime code
#  dl: For shared library loading related functions
#

# common link deps
set(lib_target "${lib_target}"
  -Wl,-undefined,error
  pthread
  stdc++
  dl
  ${ICULIB}
  )

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

if(CC_TARGETS_X86)
  set(lib_target "${lib_target} -m32")
endif()
target_link_libraries (ChakraCoreStatic ${lib_target})

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
  add_custom_command(TARGET ChakraCoreStatic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CHAKRACORE_BINARY_DIR}/bin/ChakraCore/libChakraCoreStatic.a"
    ${CHAKRACORE_BINARY_DIR}/
    )
endif()
