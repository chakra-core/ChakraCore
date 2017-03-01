cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
# set(CMAKE_VERBOSE_MAKEFILE on)

project(clangRecyclerChecker CXX)

Find_Package(LLVM REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_COMPILE_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LLVM_LDFLAGS}")

Find_Package(Clang REQUIRED)
include_directories( ${CLANG_INCLUDE_DIRS} )

add_definitions("-fno-rtti -std=c++11")
add_definitions("-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -Wno-strict-aliasing \
    "
)

add_library(clangRecyclerChecker SHARED
    Helpers.cpp
    RecyclerChecker.cpp
    RecyclerAnalyzer.cpp
)

# OSX requires following libs.
# Ubuntu complains/fails if these libs specified.
#
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    target_link_libraries(clangRecyclerChecker
        ${CLANG_LIBS}
        ${LLVM_LIBS_CORE}
        ${LLVM_LIBS_SYS}
    )
endif()
