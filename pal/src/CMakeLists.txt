cmake_minimum_required(VERSION 2.8.12.2)

include_directories(SYSTEM /usr/local/include)

include(configure.cmake)

project(chakrapal)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

cmake_policy(SET CMP0042 NEW)

# Include directories

include_directories(include)

# Compile options

if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL AMD64)
  set(PAL_CMAKE_PLATFORM_ARCH_AMD64 1)
  add_definitions(-D_AMD64_)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7l)
  set(PAL_CMAKE_PLATFORM_ARCH_ARM 1)
  add_definitions(-D_ARM_)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
  set(PAL_CMAKE_PLATFORM_ARCH_ARM64 1)
  add_definitions(-D_ARM64_)
else()
  message(FATAL_ERROR "Only ARM and AMD64 is supported")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  add_definitions(-D__LINUX__=1)
endif(CMAKE_SYSTEM_NAME STREQUAL Linux)

add_definitions(-DPLATFORM_UNIX=1)
add_definitions(-DLP64COMPATIBLE=1)
add_definitions(-DFEATURE_PAL=1)
# add_definitions(-DCORECLR=1)
add_definitions(-DPIC=1)
add_definitions(-D_FILE_OFFSET_BITS=64)
if(PAL_CMAKE_PLATFORM_ARCH_AMD64)
  add_definitions(-DBIT64=1)
  add_definitions(-D_WIN64=1)
elseif(PAL_CMAKE_PLATFORM_ARCH_ARM)
  add_definitions(-DBIT32=1)
  add_definitions(-D_WIN32=1)
elseif(PAL_CMAKE_PLATFORM_ARCH_ARM64)
  add_definitions(-DBIT64=1)
  add_definitions(-D_WIN64=1)
endif()

# turn off capability to remove unused functions (which was enabled in debug build with sanitizers)
# set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,--no-gc-sections")

add_compile_options(-fno-builtin)
add_compile_options(-fPIC)

set(SOURCES
  safecrt/memcpy_s.c
)

add_library(coreclrpal
  STATIC
  ${SOURCES}
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  if(PAL_CMAKE_PLATFORM_ARCH_AMD64)
    target_link_libraries(coreclrpal
      unwind-x86_64
    )
  endif()
    
  target_link_libraries(coreclrpal
    gcc_s
    pthread
    rt
    dl
    unwind
    unwind-generic
    uuid
  )
endif(CMAKE_SYSTEM_NAME STREQUAL Linux)

# Install the static PAL library for VS
install (TARGETS coreclrpal DESTINATION lib)
