cmake_minimum_required(VERSION 3.2)
project (CHAKRACORE)


function(clr_unknown_arch)
    if (WIN32)
        message(FATAL_ERROR "Only AMD64, ARM and I386 are supported")
    else()
        message(FATAL_ERROR "Only AMD64 and ARM are supported")
    endif()
endfunction()

if(ICU_INCLUDE_PATH)
  set(ICU_CC_PATH "${ICU_INCLUDE_PATH}/../lib/")
  find_library(ICUUC icuuc PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
  find_library(ICU18 icui18n PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
  find_library(ICUDATA icudata PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
  if(ICUUC)
    message("found libraries on ${ICU_CC_PATH}")
    set(ICULIB
      ${ICUUC}
      ${ICU18}
      ${ICUDATA}
      )
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    if(NOT ICULIB)
      set(ICULIB "icuuc")
    endif()

    set(CLR_CMAKE_PLATFORM_UNIX 1)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
        set(CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64 1)
    else()
        clr_unknown_arch()
    endif()
    set(CLR_CMAKE_PLATFORM_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_definitions(
        -DPLATFORM_UNIX
    )

    if(NOT ICULIB)
      set(ICULIB "icucore")
      add_definitions(
        -DU_DISABLE_RENAMING=1 #in case we link against to an older binary of icu
        )
      message("using ICU from system default: ${ICULIB}")
    endif()

    set(CLR_CMAKE_PLATFORM_UNIX 1)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
        set(CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64 1)
    else()
        clr_unknown_arch()
    endif()
    set(CLR_CMAKE_PLATFORM_DARWIN 1)
else()
    clr_unknown_arch()
endif()

if(CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64)
    add_definitions(-D_M_X64_OR_ARM64)
    set(CLR_CMAKE_PLATFORM_ARCH_AMD64 1)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL AppleClang
    OR CMAKE_CXX_COMPILER_ID STREQUAL Clang
    OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    # Color diagnostics for g++ and clang++
    add_definitions("-fdiagnostics-color=always")
endif()

if(STATIC_LIBRARY)
    add_definitions(-DCHAKRA_STATIC_LIBRARY=1)
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
        add_definitions(-D_M_X64 -D_M_AMD64 -D_AMD64_)
    endif(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)

    add_definitions(
        -DUNICODE
        -D_SAFECRT_USE_CPP_OVERLOADS=1
        -D__STDC_WANT_LIB_EXT1__=1
        -DDISABLE_JIT=1  # xplat-todo: enable the JIT for Linux
        )

    set(CMAKE_CXX_STANDARD 11)

    # CC WARNING FLAGS
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
        -Wno-implicit-function-declaration"
    )

    # todo: fix general visibility of the interface
    # do not set to `fvisibility=hidden` as it is going to
    # prevent the required interface is being exported
    # clang by default sets fvisibility=default

if(NOT CC_XCODE_PROJECT)
    # todo: enable for XCode too
    # XCode is a bit more strict
    # keep this until we fix all the warnings there.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -Werror"
        )
endif()

    # CXX WARNING FLAGS
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -Wno-missing-braces\
        -Wno-reorder\
        -Wno-microsoft\
        -Wno-unused-value\
        -Wno-int-to-void-pointer-cast\
        -Wno-invalid-offsetof\
        -Wno-undefined-inline\
        -Wno-inconsistent-missing-override\
        -Wno-c++14-extensions\
        -Wno-macro-redefined\
        -Wno-pragmas\
        -Wno-invalid-token-paste\
        -Wno-format\
        -Wno-invalid-noreturn\
        -Wno-null-arithmetic\
        -Wno-tautological-constant-out-of-range-compare\
        -Wno-tautological-undefined-compare\
        -Wno-address-of-temporary\
        -Wno-null-conversion\
        -Wno-return-type\
        -Wno-switch\
        -Wno-int-to-pointer-cast"
    )
        # notes..
        # -Wno-address-of-temporary  # vtinfo.h, VirtualTableInfo<T>::RegisterVirtualTable
        # -Wno-null-conversion # Check shmemory.cpp and cs.cpp here...
        # -Wno-return-type # switch unreachable code
        # -Wno-switch # switch values not handled

    # CXX COMPILER FLAGS

    # xplat-todo for release build
    # -fno-inline.... -> -mno-omit.... are needed for more accurate stack inf.
    # Release Builds: Not sure if this has to be as strict as the debug/test?
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -fdelayed-template-parsing\
        -fno-omit-frame-pointer\
        -fno-optimize-sibling-calls\
        -mno-omit-leaf-frame-pointer" # this is for compat reasons. i.e. It is a noop with gcc
    )

    # CXX / CC COMPILER FLAGS
    add_compile_options(
        -fms-extensions
        -msse4.1
    )
endif(CLR_CMAKE_PLATFORM_UNIX)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -DDBG=1
        -DDEBUG=1
        -D_DEBUG=1 # for PAL
        -DDBG_DUMP=1
    )
elseif(CMAKE_BUILD_TYPE STREQUAL Test)
    add_definitions(
        -DENABLE_DEBUG_CONFIG_OPTIONS=1
    )
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -O3"
    )
endif(NOT CMAKE_BUILD_TYPE STREQUAL Debug)

if(IS_64BIT_BUILD)
    add_definitions(
        -DBIT64=1
        -DSTACK_ALIGN=16
    )
endif(IS_64BIT_BUILD)

if(CLR_CMAKE_PLATFORM_UNIX)
   add_definitions(-DFEATURE_PAL)
endif(CLR_CMAKE_PLATFORM_UNIX)

if(WITHOUT_FEATURES)
   add_definitions(${WITHOUT_FEATURES})
endif(WITHOUT_FEATURES)

enable_language(ASM)

include_directories(
    .
    lib/Common
    lib/Common/PlaceHolder
    pal
    pal/inc
    pal/inc/rt
    ${ICU_INCLUDE_PATH}
    )

if(ICU_INCLUDE_PATH)
    if(NOT HAVE_LIBICU_UCHAR_H)
        set(HAVE_LIBICU_UCHAR_H "1")
    endif()
endif()

add_subdirectory (pal)

# build the rest with NO_PAL_MINMAX and PAL_STDCPP_COMPAT
add_definitions(
    -DNO_PAL_MINMAX
    -DPAL_STDCPP_COMPAT
    )
add_subdirectory (lib)
add_subdirectory (bin)
