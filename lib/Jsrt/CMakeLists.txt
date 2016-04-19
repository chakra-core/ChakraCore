add_library (Chakra.Jsrt
    Jsrt.cpp
    JsrtContext.cpp
    JsrtExternalArrayBuffer.cpp
    JsrtExternalObject.cpp
    JsrtPch.cpp
    JsrtRuntime.cpp
    JsrtSourceHolder.cpp
    JsrtThreadService.cpp
    )

target_include_directories (
    Chakra.Jsrt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
