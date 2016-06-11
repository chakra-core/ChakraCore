add_library (Chakra.Jsrt.Core STATIC
    JsrtContextCore.cpp
    )

target_include_directories (
    Chakra.Jsrt.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../../Runtime
    ../../Parser
    ../
    )
