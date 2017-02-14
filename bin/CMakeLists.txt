if(NOT CC_TARGET_OS_ANDROID)
    add_subdirectory (GCStress)
endif()

add_subdirectory (ch)

if (NOT STATIC_LIBRARY)
    add_subdirectory (ChakraCore)
endif()
