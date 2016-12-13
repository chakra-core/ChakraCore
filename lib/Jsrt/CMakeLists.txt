project(CHAKRA_JSRT)

if(BuildJIT)
    set(chakra_backend_objects $<TARGET_OBJECTS:Chakra.Backend>)
endif()

add_library (Chakra.Jsrt OBJECT
    Jsrt.cpp
    JsrtDebugUtils.cpp
    JsrtDebugManager.cpp
    JsrtDebuggerObject.cpp
    JsrtDiag.cpp
    JsrtContext.cpp
    JsrtExternalArrayBuffer.cpp
    JsrtExternalObject.cpp
    JsrtDebugEventObject.cpp
    JsrtHelper.cpp
    JsrtPch.cpp
    JsrtRuntime.cpp
    JsrtSourceHolder.cpp
    JsrtThreadService.cpp
    )

add_library (Chakra.Jsrt.Singular STATIC
    $<TARGET_OBJECTS:Chakra.Pal>
    $<TARGET_OBJECTS:Chakra.Common.Core>
    $<TARGET_OBJECTS:Chakra.Jsrt.Core>
    ${chakra_backend_objects}
#   Do not take this in. We need to control the
#   linker order because of global constructors
#   and cross dependencies among them
#   $<TARGET_OBJECTS:Chakra.Common.Core>
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

add_subdirectory(Core)

target_include_directories (
    Chakra.Jsrt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../Backend
    ../JITIDL
    ../Runtime
    ../Runtime/Base
    ../Runtime/ByteCode
    ../Runtime/Debug
    ../Parser
    )

target_include_directories (
    Chakra.Jsrt.Singular PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../Backend
    ../JITIDL
    ../Runtime
    ../Runtime/Base
    ../Runtime/ByteCode
    ../Runtime/Debug
    ../Parser
    )

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  target_link_libraries(Chakra.Jsrt.Singular
    pthread
    dl
    "-framework CoreFoundation"
    "-framework Security"
  )
endif(CMAKE_SYSTEM_NAME STREQUAL Darwin)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  if(CC_TARGETS_AMD64)
    target_link_libraries(Chakra.Jsrt.Singular
      unwind-x86_64
    )
  endif()

  target_link_libraries(Chakra.Jsrt.Singular
    gcc_s
    pthread
    rt
    dl
    unwind
    unwind-generic
  )
endif(CMAKE_SYSTEM_NAME STREQUAL Linux)

