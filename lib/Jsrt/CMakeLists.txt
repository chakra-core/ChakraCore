add_library (Chakra.Jsrt STATIC
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

add_subdirectory(Core)
  
target_include_directories (
    Chakra.Jsrt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../Runtime
    ../Runtime/Base
    ../Runtime/Debug
    ../Parser
    )
