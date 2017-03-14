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

if(NOT CC_XCODE_PROJECT)
    add_custom_command(TARGET ChakraCoreHeaders PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/ChakraCore.h"
            "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/ChakraCommon.h"
            "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/ChakraDebug.h"
            "${CMAKE_BINARY_DIR}/include"
        APPEND
    )
endif(NOT CC_XCODE_PROJECT)
