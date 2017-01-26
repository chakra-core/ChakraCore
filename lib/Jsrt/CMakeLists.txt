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
