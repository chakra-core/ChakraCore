cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
# set(CMAKE_VERBOSE_MAKEFILE on)

project(clangRecyclerChecker CXX)

Find_Package(LLVM REQUIRED)
link_directories( ${LLVM_LIB_DIR} )

Find_Package(Clang REQUIRED)
include_directories( ${CLANG_INCLUDE_DIRS} )

add_definitions("-fno-rtti -std=c++11")
add_definitions("-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS ")

# target_link_libraries(clangRecyclerChecker ${CLANG_LIBS} ${LLVM_LIBS_CORE} )
add_library(clangRecyclerChecker SHARED  RecyclerChecker.cpp RecyclerAnalyzer.cpp)

# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --unresolved-symbols=report-all")
target_link_libraries(clangRecyclerChecker ${CLANG_LIBS})
