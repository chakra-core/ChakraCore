
add_library (Chakra.Jsrt.object OBJECT
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

macro(CHAKRACORE_ADD_JSRT_OBJECTS _TARGET)
  target_sources(${_TARGET} PRIVATE
    $<TARGET_OBJECTS:Chakra.Pal>
    $<TARGET_OBJECTS:Chakra.Common.Core>
    $<TARGET_OBJECTS:Chakra.Common.Memory>
    $<TARGET_OBJECTS:Chakra.Common.Exceptions>
    $<TARGET_OBJECTS:Chakra.Runtime.PlatformAgnostic>
    $<TARGET_OBJECTS:Chakra.Jsrt.Core>
    )
  if(TARGET Chakra.Backend)
    target_sources(${_TARGET} PRIVATE
      $<TARGET_OBJECTS:Chakra.Backend>
      )
  endif()
  target_sources(${_TARGET} PRIVATE  
#   Do not take this in. We need to control the
#   linker order because of global constructors
#   and cross dependencies among them
    $<TARGET_OBJECTS:Chakra.Common.Core>
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
    $<TARGET_OBJECTS:Chakra.Jsrt.object>
    )
endmacro(CHAKRACORE_ADD_JSRT_OBJECTS)


add_subdirectory(Core)

target_include_directories (
    Chakra.Jsrt.object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../Backend
    ../JITIDL
    ../Runtime
    ../Runtime/Base
    ../Runtime/ByteCode
    ../Runtime/Debug
    ../Parser
    )
