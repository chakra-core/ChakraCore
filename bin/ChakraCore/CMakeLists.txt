add_library (ChakraCore
    ChakraCore.def
    ChakraCoreDLLFunc.cpp
    ConfigParserExternals.cpp
    TestHooks.cpp
    )

target_include_directories (
    ChakraCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
