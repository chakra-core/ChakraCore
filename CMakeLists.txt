cmake_minimum_required(VERSION 3.2)
project (CHAKRACORE)

# Keep CMake from caching static/shared library
# option. Otherwise, CMake fails to update cached
# references
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

if(CC_TARGETS_AMD64_SH)
  unset(CC_TARGETS_X86 CACHE)
  unset(CC_TARGETS_X86_SH CACHE)
  unset(CC_TARGETS_AMD64_SH CACHE)
  set(CC_TARGETS_AMD64 1)
endif()

if(CC_TARGETS_X86_SH)
  unset(CC_TARGETS_X86_SH CACHE)
  unset(CC_TARGETS_AMD64_SH CACHE)
  unset(CC_TARGETS_AMD64 CACHE)
  set(CC_TARGETS_X86 1)
  set(CMAKE_SYSTEM_PROCESSOR "i386")
endif()

if(ICU_SETTINGS_RESET)
  unset(ICU_SETTINGS_RESET CACHE)
  unset(ICU_INCLUDE_PATH CACHE)
  unset(ICU_INCLUDE_PATH_SH CACHE)
  unset(NO_ICU_PATH_GIVEN_SH CACHE)
  unset(NO_ICU_PATH_GIVEN CACHE)
  unset(CC_EMBED_ICU_SH CACHE)
endif()

if(CC_EMBED_ICU_SH)
  unset(CC_EMBED_ICU_SH CACHE)
  set(CC_EMBED_ICU 1)
  set(ICU_INCLUDE_PATH "deps/icu/source/output/include/")
  add_definitions(-DU_STATIC_IMPLEMENTATION)
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
  if (WIN32)
      message(FATAL_ERROR "Only AMD64, ARM and I386 are supported")
  else()
      message(FATAL_ERROR "Only AMD64 and I386 are supported")
  endif()
endfunction()

if(ICU_INCLUDE_PATH)
  add_definitions(-DHAS_REAL_ICU=1)
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
elseif(CC_EMBED_ICU)
  set(ICU_CC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../deps/icu/source/output/lib/")
  find_library(ICUUC icuuc PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
  find_library(ICU18 icui18n PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
  find_library(ICUDATA icudata PATHS ${ICU_CC_PATH} NO_DEFAULT_PATH)
  set(ICULIB
    ${ICUUC}
    ${ICU18}
    ${ICUDATA}
    )
 endif()

set(CLR_CMAKE_PLATFORM_XPLAT 1)
if(CC_TARGETS_AMD64)
    add_definitions(-D_M_X64_OR_ARM64)
    add_compile_options(-msse4.1)
elseif(CC_TARGETS_X86)
    add_definitions(-D__i686__)
    add_definitions(-D_M_IX86_OR_ARM32)
    add_compile_options(-arch i386)
    add_compile_options(-msse3)

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} \
        -m32"
    )
else()
    clr_unknown_arch()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -Werror"
        )
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_definitions(
        -DPLATFORM_UNIX
    )

    if(NOT ICULIB)
      if(NOT NO_ICU_PATH_GIVEN)
        add_definitions(-DHAS_REAL_ICU=1)
        if(NOT CC_EMBED_ICU)
          set(ICULIB "icucore")
          add_definitions(
            -DU_DISABLE_RENAMING=1 #in case we link against to an older binary of icu
            )
        endif()
        message("using ICU from system default: ${ICULIB}")
      endif()
    endif()

    if(NOT CC_XCODE_PROJECT)
        set(CLR_CMAKE_PLATFORM_DARWIN 1)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
            -mmacosx-version-min=10.7")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -mmacosx-version-min=10.7")
        set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} \
            -mmacosx-version-min=10.7")
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
        -Wno-int-to-pointer-cast"
    )
        # notes..
        # -Wno-address-of-temporary  # vtinfo.h, VirtualTableInfo<T>::RegisterVirtualTable
        # -Wno-null-conversion # Check shmemory.cpp and cs.cpp here...
        # -Wno-return-type # switch unreachable code
        # -Wno-switch # switch values not handled

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
        -fasm-blocks
        -fms-extensions
    )
endif(CLR_CMAKE_PLATFORM_XPLAT)

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

if(CLR_CMAKE_PLATFORM_XPLAT)
   add_definitions(-DFEATURE_PAL)
endif(CLR_CMAKE_PLATFORM_XPLAT)

if(NO_JIT
   OR CLR_CMAKE_PLATFORM_DARWIN)  # TODO: JIT for OSX
    add_definitions(-DDISABLE_JIT=1)
else()
    set(BuildJIT 1)
endif()

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
