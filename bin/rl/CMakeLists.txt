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
