cmake_minimum_required(VERSION 3.2)
project (CHAKRACORE)

# Keep CMake from caching static/shared library
# option. Otherwise, CMake fails to update cached
# references

# todo: create a sub cmake file to take care of _SH uncaching...
if(SHARED_LIBRARY_SH)
    unset(SHARED_LIBRARY_SH CACHE)
    unset(STATIC_LIBRARY_SH CACHE)
    unset(STATIC_LIBRARY CACHE)
    set(SHARED_LIBRARY 1)
endif()

if(STATIC_LIBRARY_SH)
    unset(SHARED_LIBRARY_SH CACHE)
    unset(STATIC_LIBRARY_SH CACHE)
    unset(SHARED_LIBRARY CACHE)
    set(STATIC_LIBRARY 1)
endif()

if(LIBS_ONLY_BUILD_SH)
    unset(LIBS_ONLY_BUILD_SH CACHE)
    set(CC_LIBS_ONLY_BUILD 1)
endif()

if(CC_USES_SYSTEM_ARCH_SH)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(CC_TARGETS_AMD64_SH 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
        set(CC_TARGETS_ARM_SH 1)
    endif()
    unset(CC_USES_SYSTEM_ARCH_SH CACHE)
endif()

if(CC_TARGETS_AMD64_SH)
    set(CC_TARGETS_AMD64 1)
elseif(CC_TARGETS_ARM_SH)
    set(CC_TARGETS_ARM 1)
    add_definitions(-D_ARM_=1)
    set(CMAKE_SYSTEM_PROCESSOR "armv7l")
elseif(CC_TARGETS_X86_SH)
    set(CC_TARGETS_X86 1)
    set(CMAKE_SYSTEM_PROCESSOR "i386")
else()
    message(FATAL_ERROR "Couldn't detect target processor, try `--arch` argument with build.sh")
endif()

unset(CC_TARGETS_ARM_SH CACHE)
unset(CC_TARGETS_X86_SH CACHE)
unset(CC_TARGETS_AMD64_SH CACHE)

if(ENABLE_VALGRIND_SH)
    unset(ENABLE_VALGRIND_SH CACHE)
    if(NOT CC_TARGETS_X86)
        # Enable Valgrind is not needed for x86 builds. Already <= 32Gb address space
        set(ENABLE_VALGRIND 1)
        add_definitions(-DENABLE_VALGRIND=1)
    endif()
endif()

if(ICU_SETTINGS_RESET)
    unset(ICU_SETTINGS_RESET CACHE)
    unset(ICU_INCLUDE_PATH CACHE)
    unset(ICU_INCLUDE_PATH_SH CACHE)
    unset(NO_ICU_PATH_GIVEN_SH CACHE)
    unset(NO_ICU_PATH_GIVEN CACHE)
    unset(CC_EMBED_ICU_SH CACHE)
endif()

if(CC_TARGET_OS_ANDROID_SH)
    set(CC_TARGET_OS_ANDROID 1)
    set(CMAKE_SYSTEM_NAME Android)
    set(ANDROID_NDK "android-toolchain-arm/")
    set(ANDROID_ABI armeabi-v7a)
    set(CMAKE_SYSTEM_VERSION 21)
    set(CMAKE_ANDROID_ARCH_ABI armeabi)
    set(ANDROID_TOOLCHAIN_NAME arm-linux-androideabi-clang3.8)
    set(ANDROID_STL "c++_static")
    unset(CC_TARGET_OS_ANDROID_SH CACHE)
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CC_TARGET_OS_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(CC_TARGET_OS_OSX 1)
endif()

if (ENABLE_CC_XPLAT_TRACE_SH)
    unset(ENABLE_CC_XPLAT_TRACE_SH CACHE)
    set(ENABLE_CC_XPLAT_TRACE 1)
    if (CC_TARGET_OS_ANDROID)
        add_definitions(-DTRACE_OUTPUT_TO_LOGCAT=1)
    else()
        add_definitions(-DTRACE_OUTPUT_TARGET_FILE=1)
    endif()
    add_definitions(-DENABLE_CC_XPLAT_TRACE=1)
    add_compile_options(-finstrument-functions)
    add_compile_options(-g)
    add_compile_options(-ggdb)
    if(NOT STATIC_LIBRARY)
        message(FATAL_ERROR "Trace option is available only for --static builds")
    endif()
endif()

if(CC_EMBED_ICU_SH)
    unset(CC_EMBED_ICU_SH CACHE)
    set(CC_EMBED_ICU 1)
    set(ICU_INCLUDE_PATH "deps/icu/source/output/include/")
endif()

if(ICU_INCLUDE_PATH_SH)
    set(ICU_INCLUDE_PATH ${ICU_INCLUDE_PATH_SH})
    unset(NO_ICU_PATH_GIVEN_SH CACHE)
    unset(NO_ICU_PATH_GIVEN CACHE)
    unset(ICU_INCLUDE_PATH_SH CACHE)
endif()

if(NO_ICU_PATH_GIVEN_SH)
    set(NO_ICU_PATH_GIVEN ${NO_ICU_PATH_GIVEN_SH})
    unset(NO_ICU_PATH_GIVEN_SH CACHE)
    unset(ICU_INCLUDE_PATH_SH CACHE)
    unset(ICU_INCLUDE_PATH CACHE)
endif()

function(clr_unknown_arch)
    message(FATAL_ERROR "Only AMD64, ARM and I386 are supported")
endfunction()

if(ICU_INCLUDE_PATH)
    add_definitions(-DHAS_REAL_ICU=1)
    set(ICU_CC_PATH "${ICU_INCLUDE_PATH}/../lib/")
    find_library(ICUUC icuuc PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
    find_library(ICU18 icui18n PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
    if(ICUUC)
      message("-- found ICU libs: ${ICU_CC_PATH}")
      find_library(ICUDATA icudata PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
      if (NOT ICUDATA)
          set(ICUDATA "")
      endif()
      find_library(ICUTOOLS icutools PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
      if (NOT ICUTOOLS)
          set(ICUTOOLS "")
      endif()
      set(ICULIB
        ${ICUUC}
        ${ICU18}
        ${ICUDATA}
        ${ICUTOOLS}
        )
    endif()
endif()

set(CLR_CMAKE_PLATFORM_XPLAT 1)
if(CC_TARGETS_AMD64)
    add_definitions(-D_M_X64_OR_ARM64)
    add_compile_options(-msse4.2)

    set(CAN_BUILD_WABT 1)
elseif(CC_TARGETS_X86)
    add_definitions(-D__i686__)
    add_definitions(-D_M_IX86_OR_ARM32)
    add_compile_options(-arch i386)
    add_compile_options(-msse3)

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} \
        -m32"
    )
elseif(CC_TARGETS_ARM)
    add_definitions(-D__arm__)
    add_definitions(-D_M_IX86_OR_ARM32)
    add_definitions(-D_M_ARM32_OR_ARM64)
    if(CC_TARGET_OS_OSX)
        add_compile_options(-arch arm)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # reduce link time memory usage
        set(LINKER_REDUCED_MEMORY "-Xlinker --no-keep-memory")
    endif()
else()
    clr_unknown_arch()
endif()

if(CAN_BUILD_WABT)
    add_definitions(-DCAN_BUILD_WABT)
endif()

if(CC_TARGET_OS_LINUX OR CC_TARGET_OS_ANDROID)
    if(NOT ICULIB)
      if(NOT NO_ICU_PATH_GIVEN)
        if(NOT CC_EMBED_ICU)
          set(ICULIB "icuuc")
        endif()
        add_definitions(-DHAS_REAL_ICU=1)
      endif()
    endif()

    set(CLR_CMAKE_PLATFORM_LINUX 1)
    # OSX 10.12 Clang deprecates libstdc++ [See GH #1599]
    # So, -Werror is linux only for now
    # + Android ARM ABI shows ld warnings
    # xplat-todo: Do we need this ?
    if (NOT CC_TARGET_OS_ANDROID)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
          -Werror"
          )
    endif()
elseif(CC_TARGET_OS_OSX)
    add_definitions(
        -DPLATFORM_UNIX
    )

    # in case ICU path was given but it doesn't exist, build script will fail.
    # so, fallback only if ICU path wasn't given
    if(NOT ICU_INCLUDE_PATH)
      set(NO_ICU_PATH_GIVEN 1)
      message("-- Couldn't find ICU. Falling back to --no-icu build")
    endif()

    if(NOT CC_XCODE_PROJECT)
      set(OSX_DEPLOYMENT_TARGET "$ENV{MACOSX_DEPLOYMENT_TARGET} CC")
      if (${OSX_DEPLOYMENT_TARGET} STREQUAL " CC")
        set(OSX_DEPLOYMENT_TARGET "10.9")
        add_compile_options(-mmacosx-version-min=10.9)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
          -mmacosx-version-min=10.9 -std=gnu99")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
          -mmacosx-version-min=10.9 -std=gnu++11")
      else()
        set(OSX_DEPLOYMENT_TARGET "$ENV{MACOSX_DEPLOYMENT_TARGET}")
        message(WARNING "-- !! macOS Deployment Target was set to $ENV{MACOSX_DEPLOYMENT_TARGET}. Using it as is.")
      endif()
    endif()
else()
    message(FATAL_ERROR "This OS is not supported")
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

if(CLR_CMAKE_PLATFORM_XPLAT)
    add_definitions(-D_CHAKRACOREBUILD)
    add_definitions(-DFEATURE_PAL)
    add_definitions(-DPLATFORM_UNIX=1)

    if(CLR_CMAKE_PLATFORM_LINUX)
        add_definitions(-D__LINUX__=1)
        if(CC_TARGETS_AMD64)
            add_definitions(-DLINUX64)
        endif(CC_TARGETS_AMD64)
    endif(CLR_CMAKE_PLATFORM_LINUX)

    if(CC_TARGETS_AMD64)
        set(IS_64BIT_BUILD 1)
        add_definitions(-D_M_X64 -D_M_AMD64 -D_AMD64_)
    endif(CC_TARGETS_AMD64)

    add_definitions(
        -DUNICODE
        -D_SAFECRT_USE_CPP_OVERLOADS=1
        -D__STDC_WANT_LIB_EXT1__=1
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

    # CXX WARNING FLAGS
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -Wno-ignored-attributes\
        -Wno-deprecated-declarations\
        -Wno-parentheses-equality\
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
        -Wno-implicit-function-declaration\
        -Wno-int-to-pointer-cast"
    )
        # notes..
        # -Wno-address-of-temporary  # vtinfo.h, VirtualTableInfo<T>::RegisterVirtualTable
        # -Wno-null-conversion # Check shmemory.cpp and cs.cpp here...
        # -Wno-return-type # switch unreachable code
        # -Wno-switch # switch values not handled

    include(Build/CMakeFeatureDetect.cmake)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        ${CXX_DO_NOT_OPTIMIZE_SIBLING_CALLS} \
        -fno-omit-frame-pointer \
        -fdelayed-template-parsing"
        )

    # CXX / CC COMPILER FLAGS
    add_compile_options(
        -fasm-blocks
        -fms-extensions
        -fwrapv                 # Treat signed integer overflow as two's complement
    )

    # Clang -fsanitize.
    if (CLANG_SANITIZE_SH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${CLANG_SANITIZE_SH}")
        set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -fsanitize=${CLANG_SANITIZE_SH}")
        unset(CLANG_SANITIZE_SH CACHE)      # don't cache
    endif()
endif(CLR_CMAKE_PLATFORM_XPLAT)

if (ENABLE_FULL_LTO_SH OR ENABLE_THIN_LTO_SH)
    if (CC_TARGET_OS_LINUX)
        set(CC_LTO_ENABLED -use-gold-plugin)
        set(CC_LTO_ENABLED_C -c)
    endif()

    if (ENABLE_FULL_LTO_SH)
        unset(DENABLE_FULL_LTO_SH CACHE)
        add_compile_options(-flto ${CC_LTO_ENABLED_C})

        if (CC_LTO_ENABLED)
            set(CC_LTO_ENABLED "${CC_LTO_ENABLED} -flto")
        endif()
    elseif (ENABLE_THIN_LTO_SH)
        unset(ENABLE_THIN_LTO_SH CACHE)
        add_compile_options(-flto=thin)
        if (CC_LTO_ENABLED)
            set(CC_LTO_ENABLED "${CC_LTO_ENABLED} -flto=thin")
        endif()
    endif()
endif()

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
    add_compile_options(-O3)
else()
    add_compile_options(-O0)
endif(NOT CMAKE_BUILD_TYPE STREQUAL Debug)

if(IS_64BIT_BUILD)
    add_definitions(
        -DBIT64=1
        -DSTACK_ALIGN=16
    )
endif(IS_64BIT_BUILD)

if(NO_JIT_SH)
    unset(NO_JIT_SH CACHE)      # don't cache
    unset(BuildJIT CACHE)       # also clear it just in case
    add_definitions(-DDISABLE_JIT=1)
else()
    set(BuildJIT 1)
endif()

if(INTL_ICU_SH)
    unset(INTL_ICU_SH CACHE)    # don't cache
    add_definitions(-DINTL_ICU=1)
    set(IntlICU 1)
endif()

if(WITHOUT_FEATURES_SH)
    add_definitions(${WITHOUT_FEATURES_SH})
    unset(WITHOUT_FEATURES_SH CACHE)    # don't cache
endif(WITHOUT_FEATURES_SH)

if(EXTRA_DEFINES_SH)
    add_definitions(${EXTRA_DEFINES_SH})
    unset(EXTRA_DEFINES_SH CACHE)    #don't cache
endif(EXTRA_DEFINES_SH)

enable_language(ASM)

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(DYN_LIB_EXT "dylib")
else()
    set(DYN_LIB_EXT "so")
endif()

################# Write-barrier check/analyze ##################
if (WB_CHECK_SH OR WB_ANALYZE_SH)
    add_definitions(
        -Xclang -load
        -Xclang ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecyclerChecker/Build/libclangRecyclerChecker.${DYN_LIB_EXT}
    )
endif()
if (WB_CHECK_SH)
    unset(WB_CHECK_SH CACHE)    # don't cache
    add_definitions(
        -Xclang -add-plugin
        -Xclang check-recycler
    )
endif()
if (WB_ANALYZE_SH)
    unset(WB_ANALYZE_SH CACHE)  # don't cache
    add_definitions(
        -Xclang -analyze
        -Xclang -analyzer-checker=chakra.RecyclerChecker
    )
endif()
if (WB_ARGS_SH)
    foreach(wb_arg IN LISTS WB_ARGS_SH)
        add_definitions(
            -Xclang -plugin-arg-check-recycler
            -Xclang ${wb_arg}
        )
    endforeach(wb_arg)
    unset(WB_ARGS_SH CACHE)  # don't cache
endif()

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
