cmake_minimum_required(VERSION 3.2)
project (CHAKRACORE)

add_definitions ("-fms-extensions")

# Disable some warnings
add_definitions("-Wno-tautological-constant-out-of-range-compare")
add_definitions("-D__STDC_WANT_LIB_EXT1__=1")
add_definitions("-std=c++11")
add_definitions("-stdlib=libc++")

include_directories("pal")
include_directories("pal/inc")
include_directories("pal/inc/rt")
add_subdirectory (lib)
