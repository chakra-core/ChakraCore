cmake_minimum_required(VERSION 3.10)
project (CHAKRACORE)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")

# Disable expected CMake workflow
option(CHAKRACORE_BUILD_SH "Use build.sh")

set(CCACHE_PROGRAM_NAME_DEFAULT "ccache")

if(NOT CHAKRACORE_BUILD_SH)
    option(DISABLE_JIT "Disable JIT compilation" OFF)
    option(INTL_ICU "Enable Intl" ON)
    option(EMBED_ICU "Build ICU within ChakraCore build" OFF)
    option(USE_CCACHE "Build using ccache" OFF)
    set(CCACHE_PROGRAM_NAME ${CCACHE_PROGRAM_NAME_DEFAULT} CACHE
        STRING "ccache executable")
    set(ICU_INCLUDE_PATH "" CACHE STRING "libicu iclude path")
    if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
      set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    endif (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
else(NOT CHAKRACORE_BUILD_SH)

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

    if (CLANG_SANITIZE_SH)
        set(CLANG_SANITIZE ${CLANG_SANITIZE_SH})
        unset(CLANG_SANITIZE_SH CACHE)
    endif()

endif(NOT CHAKRACORE_BUILD_SH)

if(CC_USES_SYSTEM_ARCH_SH OR NOT CHAKRACORE_BUILD_SH)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(CC_TARGETS_AMD64_SH 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
        set(CC_TARGETS_ARM_SH 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(CC_TARGETS_ARM64_SH 1)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(CC_TARGETS_ARM64_SH 1)
    endif()
    unset(CC_USES_SYSTEM_ARCH_SH CACHE)
endif()

if(CC_TARGETS_AMD64_SH)
    set(CC_TARGETS_AMD64 1)
elseif(CC_TARGETS_ARM_SH)
    set(CC_TARGETS_ARM 1)
    add_definitions(-D_ARM_=1)
    set(CMAKE_SYSTEM_PROCESSOR "armv7l")
elseif(CC_TARGETS_ARM64_SH)
    add_definitions(-D_ARM64_=1)
    add_definitions(-D__arm64__=1)
    set(CC_TARGETS_ARM64 1)
    set(CMAKE_SYSTEM_PROCESSOR "arm64")
elseif(CC_TARGETS_X86_SH)
    set(CC_TARGETS_X86 1)
    set(CMAKE_SYSTEM_PROCESSOR "i386")
else()
    message(FATAL_ERROR "Unsupported target processor: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

unset(CC_TARGETS_ARM_SH CACHE)
unset(CC_TARGETS_X86_SH CACHE)
unset(CC_TARGETS_AMD64_SH CACHE)

if (CHAKRACORE_BUILD_SH)
    if(CCACHE_PROGRAM_NAME_SH)
        set(USE_CCACHE ON)
        set(CCACHE_PROGRAM_NAME ${CCACHE_PROGRAM_NAME_SH})
        unset(CCACHE_PROGRAM_NAME_SH CACHE)
    else()
        set(USE_CCACHE OFF)
        set(CCACHE_PROGRAM_NAME ${CCACHE_PROGRAM_NAME_DEFAULT})
    endif()
endif()

if(USE_CCACHE)
  find_program(CCACHE_PROGRAM ${CCACHE_PROGRAM_NAME} REQUIRED)
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

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
    unset(ICU_INCLUDE_PATH_SH CACHE)
    unset(NO_ICU_SH CACHE)
    unset(LOCAL_ICU_SH CACHE)
    unset(SYSTEM_ICU_SH CACHE)
    unset(EMBED_ICU_SH CACHE)
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

if(ICU_INCLUDE_PATH_SH)
    set(ICU_INCLUDE_PATH ${ICU_INCLUDE_PATH_SH})
    unset(ICU_INCLUDE_PATH_SH CACHE)
endif()

if(NO_ICU_SH)
    set(NO_ICU 1)
    unset(NO_ICU_SH CACHE)
endif()

if(SYSTEM_ICU_SH)
    set(SYSTEM_ICU 1)
    unset(SYSTEM_ICU_SH CACHE)
endif()

if(CHAKRACORE_BUILD_SH)
    if(INTL_ICU_SH)
        unset(INTL_ICU_SH CACHE)
        set(INTL_ICU 1)
    else()
        unset(INTL_ICU_SH CACHE)
        set(INTL_ICU 0)
    endif()
endif(CHAKRACORE_BUILD_SH)

if(EMBED_ICU_SH)
    set(EMBED_ICU 1)
    unset(EMBED_ICU_SH CACHE)
endif()

if(EMBED_ICU AND ICU_INCLUDE_PATH)
    message(FATAL_ERROR "Embedded ICU and ICU include path cannot be set at the same time")
endif()

if(EMBED_ICU)
    # Keep consistent with what ICU download script used to print
    message("Note: ICU installation and use is subject to it's publisher's licensing terms")

    set(ICU_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/thirdparty/icu)
    set(ICU_DOWNLOAD_DIR ${ICU_PREFIX}/download)
    set(ICU_SOURCE_DIR ${ICU_PREFIX}/stage)
    set(EMBEDDED_ICU_TARGET icu4c)
    set(ICU_INCLUDE_PATH ${ICU_PREFIX}/include)
    set(ICU_LIBRARY_PATH ${ICU_PREFIX}/lib)
    add_definitions(-DHAS_REAL_ICU=1)
    add_definitions(-DHAS_ICU)
    add_definitions(-DINTL_ICU=1)
    set(ICU_LIBRARIES
        ${ICU_LIBRARY_PATH}/libicuuc.a
        ${ICU_LIBRARY_PATH}/libicui18n.a
        ${ICU_LIBRARY_PATH}/libicudata.a
    )

    include(ExternalProject)
    ExternalProject_Add(${EMBEDDED_ICU_TARGET}
        PREFIX ${ICU_PREFIX}
        DOWNLOAD_DIR ${ICU_DOWNLOAD_DIR}
        SOURCE_DIR ${ICU_SOURCE_DIR}
        URL https://github.com/unicode-org/icu/releases/download/release-63-2/icu4c-63_2-src.tgz
        URL_HASH SHA512=5fa9092efd8d6da6dfc8d498e4026167fda43423eaafc754d1789cf8fd4f6e76377878ebcaa32e14f314836136b764873511a93bfbcc5419b758841cc6df8f32
        CONFIGURE_COMMAND ${ICU_SOURCE_DIR}/source/configure --prefix=${ICU_PREFIX} --with-data-packaging=static --enable-static --disable-shared --with-library-bits=64 --disable-icuio --disable-layout --disable-tests --disable-samples
        BUILD_COMMAND make STATICCFLAGS="-fPIC" STATICCXXFLAGS="-fPIC" STATICCPPFLAGS="-DPIC"
        INSTALL_COMMAND make install
        BYPRODUCTS ${ICU_LIBRARIES}
    )
elseif(ICU_INCLUDE_PATH)
    add_definitions(-DHAS_REAL_ICU=1)
    add_definitions(-DHAS_ICU)
    set(ICU_LIBRARY_PATH "${ICU_INCLUDE_PATH}/../lib/")
    find_library(ICUUC icuuc PATHS ${ICU_LIBRARY_PATH} NO_DEFAULT_PATH)

    if(INTL_ICU)
        add_definitions(-DINTL_ICU=1)
        find_library(ICUIN icui18n PATHS ${ICU_LIBRARY_PATH} NO_DEFAULT_PATH)
        # In a default install, ICU header files are all in ICU_ROOT/include
        # However, for Node, the include/ folder is never generated, so we have to look
        # in NODE/deps/icu/source/{common|i18n} for headers
        set(ICU_INCLUDE_PATH
            "${ICU_INCLUDE_PATH}"
            "${ICU_INCLUDE_PATH}/../i18n/"
        )
    endif()

    if(ICUUC)
        message("-- found ICU libs: ${ICU_LIBRARY_PATH}")
        find_library(ICUDATA icudata PATHS ${ICU_LIBRARY_PATH} NO_DEFAULT_PATH)
        if (NOT ICUDATA)
            set(ICUDATA "")
        endif()
        set(ICU_LIBRARIES
            ${ICUUC}
            ${ICUIN}
            ${ICUDATA}
        )
    endif()
endif()

if(CC_TARGET_OS_LINUX OR CC_TARGET_OS_ANDROID)
    if(SYSTEM_ICU OR (NOT ICU_INCLUDE_PATH AND NOT NO_ICU))
        set(ICU_LIBRARIES "icuuc")
        if(INTL_ICU)
            add_definitions(-DINTL_ICU=1)
            set(ICU_LIBRARIES
                ${ICU_LIBRARIES}
                icui18n
            )
        endif()
        add_definitions(-DHAS_REAL_ICU=1)
        add_definitions(-DHAS_ICU)
    endif()
elseif(CC_TARGET_OS_OSX)
    # in case ICU path was given but it doesn't exist, build script will fail.
    # so, fallback only if ICU path wasn't given
    if(NOT ICU_INCLUDE_PATH)
        set(NO_ICU 1)
        message("-- Couldn't find ICU. Falling back to --no-icu build")
    endif()
endif()

set(CLR_CMAKE_PLATFORM_XPLAT 1)
if(CC_TARGETS_AMD64)
    add_definitions(-DTARGET_64)
    add_compile_options(-msse4.2)

    if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
        set(CAN_BUILD_WABT 1)
    endif()
elseif(CC_TARGETS_X86)
    add_definitions(-D__i686__)
    add_definitions(-DTARGET_32)
    add_compile_options(-arch i386)
    add_compile_options(-msse3)

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} \
        -m32"
    )
elseif(CC_TARGETS_ARM)
    add_definitions(-D__arm__)
    add_definitions(-DTARGET_32)
    add_definitions(-D_M_ARM32_OR_ARM64)
    if(CC_TARGET_OS_OSX)
        add_compile_options(-arch arm)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # reduce link time memory usage
        set(LINKER_REDUCED_MEMORY "-Xlinker --no-keep-memory")
    endif()
elseif(CC_TARGETS_ARM64)
    add_definitions(-D__aarch64__)
    add_definitions(-DTARGET_64)
    add_definitions(-D_M_ARM32_OR_ARM64)
    if(CC_TARGET_OS_OSX)
        add_compile_options(-arch arm64)
    endif()
    if (CC_TARGET_OS_LINUX)
      # arm64 .S mostly use ; comments, not accepted by GNU assembler.
      # In lieu of converting them all, just strip these comments during build.
      set(CMAKE_ASM_COMPILE_OBJECT
          "sed -e 's/\;.*//g' <SOURCE> > <OBJECT_DIR>/$$(basename <SOURCE>)"
          "<CMAKE_ASM_COMPILER> <DEFINES> <INCLUDES> -I $$(dirname <SOURCE>) <FLAGS> -o <OBJECT> -c <OBJECT_DIR>/$$(basename <SOURCE>)"
      )
    endif()
else()
    message(FATAL_ERROR "Only AMD64, ARM, ARM64 and I386 are supported")
endif()

if(CAN_BUILD_WABT)
    add_definitions(-DCAN_BUILD_WABT)
endif()

if(CC_TARGET_OS_LINUX OR CC_TARGET_OS_ANDROID)
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
    message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}")
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
    elseif(CC_TARGETS_ARM64)
        set(IS_64BIT_BUILD 1)
        add_definitions(-D_M_ARM64 -D_ARM64_)
    endif()

    add_definitions(
        -DUNICODE
        -D_SAFECRT_USE_CPP_OVERLOADS=1
        -D__STDC_WANT_LIB_EXT1__=1
        )

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED On)

    # todo: fix general visibility of the interface
    # do not set to `fvisibility=hidden` as it is going to
    # prevent the required interface is being exported
    # clang by default sets fvisibility=default

    # Allow ch to export `OnChakraCoreLoadedEntry`
    set(CMAKE_ENABLE_EXPORTS ON)

    # CXX WARNING FLAGS
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -Wno-ignored-attributes\
        -Wno-parentheses-equality\
        -Wno-reorder\
        -Wno-microsoft\
        -Wno-unused-value\
        -Wno-int-to-void-pointer-cast\
        -Wno-invalid-offsetof\
        -Wno-undefined-inline\
        -Wno-inconsistent-missing-override\
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
        -Wno-int-to-pointer-cast\
        -Wno-tautological-constant-compare\
        -Wno-enum-compare-switch\
        -Wno-unknown-warning-option"
    )
        # notes..
        # -Wno-address-of-temporary  # vtinfo.h, VirtualTableInfo<T>::RegisterVirtualTable
        # -Wno-null-conversion # Check shmemory.cpp and cs.cpp here...
        # -Wno-return-type # switch unreachable code
        # -Wno-switch # switch values not handled
        # -W-enum-compare-switch # throws warning on enum1 == enum2 where both
        # enums represent same value but do not share the type. ie. we use both AsmJsType
        # and AsmJsRetType interchangably and filter things out by `default:` case.
        # -W-tautological-constant-compare throws warning for checks known at compile time.
        # Some of those checks are optimized out during compile / lto time, and others
        # are platform / compiler dependent.
        # -Wno-unknown-warning-option ... well, some of the new switches are not
        # recognized by older compilers and they fail. So, put this one and not fail

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
        -fwrapv # Treat signed integer overflow as two's complement
    )

    # Only disable RTTI in release builds so that TrackAlloc works for debug and test builds
    # Also disable RTTI when building a shared library
    # TODO: why does the shared library break with rtti disabled?
    if(CMAKE_BUILD_TYPE STREQUAL Release)
        if(STATIC_LIBRARY)
            add_compile_options(-fno-rtti)
        endif()
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
elseif(CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    add_definitions(
        -DENABLE_DEBUG_CONFIG_OPTIONS=1
    )
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

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
elseif(DISABLE_JIT)
    set(BuildJIT 0)
    add_definitions(-DDISABLE_JIT=1)
else()
    set(BuildJIT 1)
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

if(CC_TARGETS_ARM64)
    if(CC_TARGET_OS_LINUX)
        message(WARNING "ARM64 linux build has not yet been tested, this build is unsupported.")
    endif()
    if(BuildJIT)
        message(WARNING "ARM64 Jit not yet functional on platforms other than windows.")
        message(WARNING "For use rather than development please build with Jit disabled --no-jit with ./build.sh or -DDISABLE_JIT=1 if using CMake directly")
    endif()
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
        set(HAVE_LIBICU_UCHAR_H 1)
    endif()
endif()

# detect features
include_directories(SYSTEM /usr/local/include)
include(pal/src/configure.cmake)

# Clang sanitizer support, this should be after `detect feature` to not to
# affect feature detection
if (CLANG_SANITIZE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${CLANG_SANITIZE_SH}")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -fsanitize=${CLANG_SANITIZE_SH}")
endif()

add_subdirectory (pal)

# build the rest with NO_PAL_MINMAX and PAL_STDCPP_COMPAT
add_definitions(
    -DNO_PAL_MINMAX
    -DPAL_STDCPP_COMPAT
    )

if (ENABLE_JS_LTTNG_SH)
    unset(ENABLE_JS_LTTNG_SH CACHE)
    include_directories (
      ${CMAKE_CURRENT_SOURCE_DIR}/out/lttng
    )
    add_subdirectory ($ENV{TARGET_PATH}/lttng ${CMAKE_CURRENT_BINARY_DIR}/lttng)

    add_definitions(
      -DENABLE_JS_ETW
      -DENABLE_JS_LTTNG
    )
    set(USE_LTTNG "1")
endif()

add_subdirectory (lib)

add_subdirectory (bin)

add_subdirectory(test)
