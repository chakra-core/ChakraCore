if(NOT STATIC_LIBRARY)
  # CH has a direct dependency to this project
  add_library (Chakra.Common.Codex.Singular STATIC
    Utf8Codex.cpp)
  target_include_directories (
    Chakra.Common.Codex.Singular PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
add_library (Chakra.Common.Codex OBJECT
    Utf8Codex.cpp)

target_include_directories (
    Chakra.Common.Codex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
