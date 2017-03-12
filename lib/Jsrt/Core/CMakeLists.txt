add_library (Chakra.Jsrt.Core OBJECT
    JsrtContextCore.cpp
    JsrtCore.cpp
    )

target_include_directories (
    Chakra.Jsrt.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../../Backend
    ../../JITIDL
    ../../Runtime
    ../../Runtime/ByteCode
    ../../Parser
    ../
    )
