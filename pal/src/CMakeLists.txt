cmake_minimum_required(VERSION 3.2)

include_directories(SYSTEM /usr/local/include)

include(configure.cmake)

project(chakrapal)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

cmake_policy(SET CMP0042 NEW)

# Include directories
include_directories(include)

# Compile options
if(CC_TARGETS_AMD64)
    add_definitions(-D_AMD64_)
elseif(CC_TARGETS_X86)
    add_definitions(-D__i686__)
elseif(CC_TARGETS_ARM)
    add_definitions(-D_ARM_)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
    set(PAL_CMAKE_PLATFORM_ARCH_ARM64 1)
    add_definitions(-D_ARM64_)
else()
    clr_unknown_arch()
endif()

if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
  add_definitions(-D__LINUX__=1)
endif()

add_definitions(-DPLATFORM_UNIX=1)
add_definitions(-DLP64COMPATIBLE=1)
add_definitions(-DFEATURE_PAL=1)
add_definitions(-DPIC=1)
add_definitions(-D_FILE_OFFSET_BITS=64)
if(CC_TARGETS_AMD64)
  add_definitions(-DBIT64=1)
  add_definitions(-D_WIN64=1)
elseif(CC_TARGETS_ARM OR CC_TARGETS_X86)
  add_definitions(-DBIT32=1)
elseif(PAL_CMAKE_PLATFORM_ARCH_ARM64)
  add_definitions(-DBIT64=1)
  add_definitions(-D_WIN64=1)
endif()

# turn off capability to remove unused functions (which was enabled in debug build with sanitizers)
# set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,--no-gc-sections")

add_compile_options(-fno-builtin)
add_compile_options(-fPIC)

if(CC_TARGET_OS_OSX)
  set(PLATFORM_SOURCES
    exception/machexception.cpp
    exception/machmessage.cpp
  )
endif()

if(CC_TARGETS_AMD64 OR CC_TARGETS_X86)
  if(CC_TARGET_OS_OSX)
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES}
      arch/i386/activationhandlerwrapper.S
      arch/i386/dispatchexceptionwrapper.S
    )
  endif()
  set(ARCH_SOURCES
    arch/i386/context2.S
    arch/i386/debugbreak.S
    arch/i386/processor.cpp
    arch/i386/asmhelpers.S
    )
elseif(CC_TARGETS_ARM)
  set(ARCH_SOURCES
    arch/arm/context2.S
    arch/arm/debugbreak.S
    arch/arm/processor.cpp
    arch/arm/asmhelpers.S
    )
endif()

set(SOURCES
  cruntime/file.cpp
  cruntime/filecrt.cpp
  cruntime/finite.cpp
  cruntime/lstr.cpp
  cruntime/malloc.cpp
  cruntime/mbstring.cpp
  cruntime/misc.cpp
  cruntime/misctls.cpp
  cruntime/path.cpp
  cruntime/printf.cpp
  cruntime/printfcpp.cpp
  cruntime/silent_printf.cpp
  cruntime/string.cpp
  cruntime/stringtls.cpp
  cruntime/thread.cpp
  cruntime/wchar.cpp
  cruntime/wchartls.cpp
  cruntime/runtime_proxy.cpp
  safecrt/makepath_s.c
  safecrt/mbusafecrt.c
  safecrt/safecrt_input_s.c
  safecrt/safecrt_output_s.c
  safecrt/safecrt_winput_s.c
  safecrt/safecrt_woutput_s.c
  safecrt/memcpy_s.c
  safecrt/memmove_s.c
  safecrt/sprintf.c
  safecrt/sscanf.c
  safecrt/strcat_s.c
  safecrt/strcpy_s.c
  safecrt/strncat_s.c
  safecrt/strncpy_s.c
  safecrt/swprintf.c
  safecrt/vsprintf.c
  safecrt/vswprint.c
  safecrt/wcscpy_s.c
  safecrt/wcsncpy_s.c
  safecrt/xtoa_s.c
  safecrt/xtow_s.c
  debug/debug.cpp
  exception/seh.cpp
  exception/signal.cpp
  file/directory.cpp
  file/pal_file.cpp
  file/filetime.cpp
  file/pal_path.cpp
  file/shmfilelockmgr.cpp
  handlemgr/handleapi.cpp
  handlemgr/handlemgr.cpp
  init/pal.cpp
  init/sxs.cpp
  loader/module.cpp
  loader/modulename.cpp
  locale/unicode.cpp
  locale/unicode_data.cpp
  locale/utf8.cpp
  map/common.cpp
  map/map.cpp
  map/virtual.cpp
  memory/heap.cpp
  memory/local.cpp
  misc/bstr.cpp
  misc/dbgmsg.cpp
  misc/error.cpp
  misc/environ.cpp
  misc/random.cpp
  misc/strutil.cpp
  misc/time.cpp
  misc/sysinfo.cpp
  misc/utils.cpp
  objmgr/palobjbase.cpp
  objmgr/shmobject.cpp
  objmgr/shmobjectmanager.cpp
  shmemory/shmemory.cpp
  synchobj/event.cpp
  synchobj/mutex.cpp
  synchmgr/synchcontrollers.cpp
  synchmgr/synchmanager.cpp
  synchmgr/wait.cpp
  sync/cs.cpp
  sync/CCSpinLock.cpp
  thread/context.cpp
  thread/process.cpp
  thread/pal_thread.cpp
  thread/threadsusp.cpp
)

add_library(Chakra.Pal
  OBJECT
  ${SOURCES}
  ${PLATFORM_SOURCES}
  ${ARCH_SOURCES}
)

if (NOT STATIC_LIBRARY)
  add_library(Chakra.Pal.Singular
    STATIC
    ${SOURCES}
    ${PLATFORM_SOURCES}
    ${ARCH_SOURCES}
  )

  if(CC_TARGET_OS_OSX)
    target_link_libraries(Chakra.Pal.Singular
      "-framework CoreFoundation"
      "-framework Security"
      )
  endif()

  if (NOT CC_TARGET_OS_ANDROID)
    set(PTHREAD pthread)
  endif()

  target_link_libraries(Chakra.Pal.Singular
    ${PTHREAD}
    dl
    )
endif()
