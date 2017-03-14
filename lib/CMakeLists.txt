add_compile_options(-fPIC)

if(BuildJIT)
    set(chakra_backend_objects $<TARGET_OBJECTS:Chakra.Backend>)
endif()

if(CC_TARGETS_AMD64)
    set(wasm_objects $<TARGET_OBJECTS:Chakra.WasmReader>)
    set(wasm_includes ${CHAKRACORE_SOURCE_DIR}/lib/WasmReader)
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

if(BuildJIT)
    add_subdirectory (Backend)
endif()

add_subdirectory (Common)
add_subdirectory (Parser)
add_subdirectory (Runtime)
add_subdirectory (Jsrt)
if (CC_TARGETS_AMD64)
    add_subdirectory (WasmReader)
endif()

if(NOT CC_XCODE_PROJECT)
    add_dependencies(ChakraCoreStatic ChakraCoreHeaders)
endif(NOT CC_XCODE_PROJECT)
