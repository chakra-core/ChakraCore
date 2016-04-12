# xplat-todo: This is a skeleton make file and not used in build yet.
# Please add this to build and fix issues.

add_library (Chakra.Runtime.Math
    AsmJsMath.cpp
    JavascriptSSE2MathOperators.cpp
    RuntimeMathPch.cpp
    )

target_include_directories (
    Chakra.Runtime.Math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
