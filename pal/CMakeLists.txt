cmake_minimum_required(VERSION 3.2)

project(COREPAL)

include_directories(${COREPAL_SOURCE_DIR}/inc)
include_directories(${COREPAL_SOURCE_DIR}/src)

add_compile_options(-fstack-protector)

add_subdirectory(src)
