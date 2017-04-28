if(NOT CC_LIBS_ONLY_BUILD)
    if(NOT (CC_TARGET_OS_ANDROID OR ENABLE_VALGRIND))
        add_subdirectory (GCStress)
    endif()

    add_subdirectory (ch)
endif()

if (NOT STATIC_LIBRARY)
    add_subdirectory (ChakraCore)
endif()
