add_compile_options(-fPIC)

if(BuildJIT)
    set(chakra_backend_objects $<TARGET_OBJECTS:Chakra.Backend>)
endif()

if(CC_TARGETS_AMD64)
    set(wasm_objects $<TARGET_OBJECTS:Chakra.WasmReader>)
    set(wasm_includes ${CHAKRACORE_SOURCE_DIR}/lib/WasmReader)
endif()

if(CAN_BUILD_WABT)
    set(wabt_objects $<TARGET_OBJECTS:libwabt>)
    set(wabt_includes ${CHAKRACORE_SOURCE_DIR}/lib/wabt)
endif()

if (USE_LTTNG)
    set(lttng_objects $<TARGET_OBJECTS:Chakra.LTTng>)
endif()

add_library (ChakraCoreStatic STATIC
  ChakraCoreStatic.cpp
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
  $<TARGET_OBJECTS:Chakra.SCACore>
  ${wasm_objects}
  ${wabt_objects}
  ${lttng_objects}
)

if(USE_LTTNG)
    target_link_libraries(ChakraCoreStatic
      -llttng-ust
      -ldl
    )
endif()

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
  ${CHAKRACORE_SOURCE_DIR}/lib/SCACore
  ${wasm_includes}
  ${wabt_includes}
  )

if(BuildJIT)
    add_subdirectory (Backend)
endif()

add_subdirectory (Common)
add_subdirectory (Parser)
add_subdirectory (SCACore)
add_subdirectory (Runtime)
add_subdirectory (Jsrt)
if (CC_TARGETS_AMD64)
    add_subdirectory (WasmReader)
    set(BUILD_TESTS OFF)
    set(RUN_BISON OFF)
    set(RUN_RE2C OFF)
endif()

if(CAN_BUILD_WABT)
    add_subdirectory (wabt)
endif()

if(NOT CC_XCODE_PROJECT)
    add_custom_command(TARGET ChakraCoreStatic POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Jsrt/ChakraCore.h"
            "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Jsrt/ChakraCommon.h"
            "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Jsrt/ChakraDebug.h"
            "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Common/ChakraCoreVersion.h"
            "${CMAKE_BINARY_DIR}/include"
    )
endif(NOT CC_XCODE_PROJECT)

if (EMBED_ICU)
  # Declare ICU dependencies in bulk.
  # TODO Not the most idiomatic solution, need to understand if all of those
  # libraries needed to depend on it
  add_dependencies(Chakra.Jsrt ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Jsrt.Core ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Parser ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.Base ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.ByteCode ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.Debug ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.Library ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.Math ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.Language ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.PlatformAgnostic ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.Runtime.Types ${EMBEDDED_ICU_TARGET})
  add_dependencies(Chakra.SCACore ${EMBEDDED_ICU_TARGET})
  if (BuildJIT)
    add_dependencies(Chakra.Backend ${EMBEDDED_ICU_TARGET})
    if (CC_TARGETS_AMD64)
      add_dependencies(Chakra.WasmReader ${EMBEDDED_ICU_TARGET})
    endif()
  endif()
endif()
