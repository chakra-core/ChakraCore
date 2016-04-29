add_library (Chakra.Common.Codex STATIC
    Utf8Codex.cpp)
  
target_include_directories (
    Chakra.Common.Codex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
