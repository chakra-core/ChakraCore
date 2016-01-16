cmake_minimum_required(VERSION 3.2)
project (CHAKRACORE)


function(clr_unknown_arch)
    if (WIN32)
        message(FATAL_ERROR "Only AMD64, ARM and I386 are supported")
    else()
        message(FATAL_ERROR "Only AMD64 and ARM are supported")
    endif()
endfunction()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CLR_CMAKE_PLATFORM_UNIX 1)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
        set(CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64 1)
    else()
        clr_unknown_arch()
    endif()
    set(CLR_CMAKE_PLATFORM_LINUX 1)
else()
    clr_unknown_arch()
endif (CMAKE_SYSTEM_NAME STREQUAL Linux)

if(CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64)
  set(CLR_CMAKE_PLATFORM_ARCH_AMD64 1)
endif()

if(CLR_CMAKE_PLATFORM_UNIX)
    add_definitions(-DPLATFORM_UNIX=1)

    if(CLR_CMAKE_PLATFORM_LINUX)
        add_definitions(-D__LINUX__=1)
        if(CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64)
            message("Detected Linux x86_64")
            add_definitions(-DLINUX64)
        else()
            clr_unknown_arch()
        endif(CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64)
    endif(CLR_CMAKE_PLATFORM_LINUX)

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
        set(IS_64BIT_BUILD 1)
        add_definitions(-D_M_X64)
    endif(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)

    add_definitions("-fms-extensions")

    # Disable some warnings
    add_definitions("-Wno-tautological-constant-out-of-range-compare")
    add_definitions("-D__STDC_WANT_LIB_EXT1__=1")
    add_definitions("-std=c++11")
    add_definitions("-stdlib=libc++")
    add_definitions(
        -DPAL_STDCPP_COMPAT=1
        -D_SAFECRT_USE_CPP_OVERLOADS=1
        )

    # xplat-todo: revisit these
    add_definitions(
        -fdelayed-template-parsing
        -Wno-microsoft
        -Wno-unused-value
        -Wno-int-to-void-pointer-cast
        -Wno-invalid-offsetof
        -Wno-undefined-inline
        -Wno-inconsistent-missing-override
        -Wno-c++14-extensions
        -Wno-macro-redefined
    )
endif(CLR_CMAKE_PLATFORM_UNIX)

if(IS_64BIT_BUILD)
    add_definitions(
        -DBIT64=1
        -DSTACK_ALIGN=16
    )
endif(IS_64BIT_BUILD)


include_directories("pal")
include_directories("pal/inc")
include_directories("pal/inc/rt")
include_directories(lib/common)
add_subdirectory (lib)
