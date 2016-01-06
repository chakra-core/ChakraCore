cmake_minimum_required(VERSION 3.2)
project (CHAKRACORE)

add_definitions ("-fms-extensions")

# Disable some warnings
add_definitions("-Wno-tautological-constant-out-of-range-compare")

include_directories("pal")
add_subdirectory (lib)
