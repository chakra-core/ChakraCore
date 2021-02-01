add_library (Chakra.Common.Codex OBJECT
    Utf8Codex.cpp)

target_include_directories (
    Chakra.Common.Codex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT STATIC_LIBRARY)
  # CH has a direct dependency to this project
  add_library (Chakra.Common.Codex.Singular STATIC
    $<TARGET_OBJECTS:Chakra.Common.Codex>)
endif()

