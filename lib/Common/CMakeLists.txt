project(CHAKRA_COMMON)

include_directories(${CHAKRA_COMMON_SOURCE_DIR})

add_subdirectory (Codex)
add_subdirectory (Common)
add_subdirectory (Core)
add_subdirectory (DataStructures)
add_subdirectory (Exceptions)
add_subdirectory (Memory)
add_subdirectory (Util)


if(NOT CC_XCODE_PROJECT)
    add_custom_target(ChakraCoreHeaders ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/ChakraCoreVersion.h"
            "${CMAKE_BINARY_DIR}/include"
    )
endif()