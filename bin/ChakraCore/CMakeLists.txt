add_library (ChakraCore SHARED
  ChakraCoreCommon.cpp
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


if(BuildJIT)
    set(chakra_backend_objects $<TARGET_OBJECTS:Chakra.Backend>)
endif()

add_library (ChakraCoreStatic STATIC
  ChakraCoreCommon.cpp
  ConfigParserExternals.cpp
  TestHooks.cpp
# Do not take this in. We need to control the
# linker order because of global constructors
# and cross dependencies among them
  $<TARGET_OBJECTS:Chakra.Pal>
  $<TARGET_OBJECTS:Chakra.Common.Core>
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
  $<TARGET_OBJECTS:Chakra.Jsrt>
  )
unset(chakra_backend_objects CACHE)

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
  ${CHAKRACORE_SOURCE_DIR}/lib/Runtime/ByteCode
  ${CHAKRACORE_SOURCE_DIR}/lib/Parser
  ${CHAKRACORE_SOURCE_DIR}/lib/Jsrt
  ${CHAKRACORE_SOURCE_DIR}/lib/JITIDL
  ${CHAKRACORE_SOURCE_DIR}/lib/Backend
  )


if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  target_link_libraries(ChakraCoreStatic
    pthread
    dl
    "-framework CoreFoundation"
    "-framework Security"
  )
endif(CMAKE_SYSTEM_NAME STREQUAL Darwin)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  if(CC_TARGETS_AMD64)
    target_link_libraries(ChakraCoreStatic
      unwind-x86_64
    )
  endif()

  target_link_libraries(ChakraCoreStatic
    gcc_s
    pthread
    rt
    dl
    unwind
    unwind-generic
  )
endif(CMAKE_SYSTEM_NAME STREQUAL Linux)


#
# Link step for the ChakraCore shared library
#
# External libraries we link with are the following:
#  pthread: For threading
#  stdc++/gcc_s: C++ runtime code
#  dl: For shared library loading related functions
#
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(LINKER_START_GROUP
    -Wl,--start-group
    -Wl,--whole-archive
    )

  set(LINKER_END_GROUP
    -Wl,--no-whole-archive
    -Wl,--end-group
    )
endif()

# common link deps
set(Chakralib_target "${Chakralib_target}"
  -Wl,-undefined,error
  ${LINKER_START_GROUP}
  Chakra.Jsrt.Singular
  ${LINKER_END_GROUP}
  pthread
  stdc++
  dl
  ${ICULIB}
  )


if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(Chakralib_target "${Chakralib_target}"
    "-Wl,-force_load,\"${CHAKRA_JSRT_BINARY_DIR}/libChakra.Jsrt.Singular.a\""
    )
endif()

if(CC_TARGETS_X86)
  set(Chakralib_target "${Chakralib_target} -m32")
endif()

#target_link_libraries (ChakraCoreStatic  ${Chakralib_target})
add_dependencies(ChakraCoreStatic  Chakra.Pal)


if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(Chakralib_target "${Chakralib_target}"
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libChakraCoreLib.version
    )
endif()
target_link_libraries (ChakraCore PRIVATE ${Chakralib_target})

unset(lib_target CACHE )
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
