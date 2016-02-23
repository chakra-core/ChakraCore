# xplat-todo: This is a skeleton make file and not used in build yet.
# Please add this to build and fix issues.

add_library (rl
    rl.cpp
    rlfeint.cpp
    rlmp.cpp
    rlregr.cpp
    rlrun.cpp
    xmlreader.cpp
    )

target_include_directories (
    rl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
