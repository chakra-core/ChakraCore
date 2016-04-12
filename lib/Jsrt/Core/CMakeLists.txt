# xplat-todo: This is a skeleton make file and not used in build yet.
# Please add this to build and fix issues.

add_library (Chakra.Jsrt.Core
    JsrtContextCore.cpp
    )

target_include_directories (
    Chakra.Jsrt.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../../Runtime
    ../../Parser
    ../
    )
