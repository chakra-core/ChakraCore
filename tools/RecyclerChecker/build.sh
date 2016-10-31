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
echo "  --llvm-config=PATH  llvm-config executable"
echo ""
}

SCRIPT_DIR=`dirname $0`
CLANG_INC=
LLVM_CONFIG=

while [[ $# -gt 0 ]]; do
    case "$1" in
    --clang-inc=*)
        CLANG_INC=$1
        CLANG_INC="${CLANG_INC:12}"
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
    CLANG_INC="-DCLANG_INCLUDE_DIRS:STRING=$CLANG_INC"
fi

if [[ $LLVM_CONFIG ]]; then
    LLVM_CONFIG="-DLLVM_CONFIG_EXECUTABLE:STRING=$LLVM_CONFIG"
fi

BUILD_DIR="$SCRIPT_DIR/Build"
mkdir -p $BUILD_DIR
pushd $BUILD_DIR > /dev/null

cmake \
    $CLANG_INC \
    $LLVM_CONFIG \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_BUILD_TYPE=Debug \
    ..
make

popd > /dev/null
