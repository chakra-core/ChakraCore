# Detect LLVM and set various variable to link against the different component of LLVM
#
# NOTE: This is a modified version of the module originally found in the OpenGTL project
# at www.opengtl.org
#
# LLVM_BIN_DIR : directory with LLVM binaries
# LLVM_LIB_DIR : directory with LLVM library
# LLVM_INCLUDE_DIR : directory with LLVM include
#
# LLVM_COMPILE_FLAGS : compile flags needed to build a program using LLVM headers
# LLVM_LDFLAGS : ldflags needed to link
# LLVM_LIBS_CORE : ldflags needed to link against a LLVM core library
# LLVM_LIBS_JIT : ldflags needed to link against a LLVM JIT
# LLVM_LIBS_JIT_OBJECTS : objects you need to add to your source when using LLVM JIT

if(NOT LLVM_INCLUDE_DIR)
  find_program(LLVM_CONFIG_EXECUTABLE
    NAMES llvm-config
    PATHS
    /opt/local/bin
    /usr/lib/llvm-3.8/bin     # Ubuntu
    /usr/local/opt/llvm/bin   # OSX brew install path
  )

  if (NOT LLVM_CONFIG_EXECUTABLE STREQUAL LLVM_CONFIG_EXECUTABLE-NOTFOUND)
    MACRO(FIND_LLVM_LIBS LLVM_CONFIG_EXECUTABLE _libname_ LIB_VAR OBJECT_VAR)
      exec_program( ${LLVM_CONFIG_EXECUTABLE} ARGS --libs ${_libname_}  OUTPUT_VARIABLE ${LIB_VAR} )
      STRING(REGEX MATCHALL "[^ ]*[.]o[ $]"  ${OBJECT_VAR} ${${LIB_VAR}})
      SEPARATE_ARGUMENTS(${OBJECT_VAR})
      STRING(REGEX REPLACE "[^ ]*[.]o[ $]" ""  ${LIB_VAR} ${${LIB_VAR}})
    ENDMACRO(FIND_LLVM_LIBS)

    exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --bindir OUTPUT_VARIABLE LLVM_BIN_DIR )
    exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libdir OUTPUT_VARIABLE LLVM_LIB_DIR )
    exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR )
    exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --cxxflags  OUTPUT_VARIABLE LLVM_COMPILE_FLAGS )
    exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --ldflags   OUTPUT_VARIABLE LLVM_LDFLAGS )
    exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libs      OUTPUT_VARIABLE LLVM_LIBS_CORE )
    exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --system-libs OUTPUT_VARIABLE LLVM_LIBS_SYS )
  endif()
endif()

if(LLVM_INCLUDE_DIR)
  message(STATUS "Found LLVM: ${LLVM_INCLUDE_DIR}")
else()
  if(LLVM_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find LLVM")
  endif(LLVM_FIND_REQUIRED)
endif()
