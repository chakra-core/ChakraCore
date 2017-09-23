#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

PRINT_USAGE() {
echo "Build RecyclerChecker clang plugin"
echo ""
echo "build.sh [options]"
echo ""
echo "options:"
echo "  -h, --help          Show help"
echo "  --clang-inc=PATH    clang development include path"
echo "  --cxx=PATH          Path to c++ compiler"
echo "  --llvm-config=PATH  llvm-config executable"
echo ""
}

pushd `dirname $0` > /dev/null
SCRIPT_DIR=`pwd -P`
popd > /dev/null
CLANG_INC=
CXX_COMPILER=
LLVM_CONFIG=

while [[ $# -gt 0 ]]; do
    case "$1" in
    --clang-inc=*)
        CLANG_INC=$1
        CLANG_INC="${CLANG_INC:12}"
        ;;

    --cxx=*)
        CXX_COMPILER=$1
        CXX_COMPILER=${CXX_COMPILER:6}
        ;;

    -h | --help)
        PRINT_USAGE && exit
        ;;

    --llvm-config=*)
        LLVM_CONFIG=$1
        LLVM_CONFIG="${LLVM_CONFIG:14}"
        ;;

    *)
        echo "Unknown option $1"
        PRINT_USAGE
        exit -1
        ;;
    esac
    shift
done

if [[ $CLANG_INC ]]; then
    echo "CLANG_INCLUDE_DIRS: $CLANG_INC"
    CLANG_INC="-DCLANG_INCLUDE_DIRS:STRING=$CLANG_INC"
fi

if [[ $CXX_COMPILER ]]; then
    echo "CXX_COMPILER: $CXX_COMPILER"
    CXX_COMPILER="-DCMAKE_CXX_COMPILER=$CXX_COMPILER"
fi

if [[ $LLVM_CONFIG ]]; then
    echo "LLVM_CONFIG: $LLVM_CONFIG"
    LLVM_CONFIG="-DLLVM_CONFIG_EXECUTABLE:STRING=$LLVM_CONFIG"
fi

BUILD_DIR="$SCRIPT_DIR/Build"
mkdir -p $BUILD_DIR
pushd $BUILD_DIR > /dev/null

cmake \
    $CLANG_INC \
    $CXX_COMPILER \
    $LLVM_CONFIG \
    .. \
&& make
_RET=$?

popd > /dev/null
exit $_RET
