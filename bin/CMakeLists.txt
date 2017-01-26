if(NOT CC_TARGET_OS_ANDROID)
    add_subdirectory (GCStress)
endif()

add_subdirectory (ch)
add_subdirectory (ChakraCore)
