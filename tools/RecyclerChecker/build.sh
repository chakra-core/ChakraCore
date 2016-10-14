#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

pushd `dirname $0`
mkdir Build
pushd Build

cmake \
    -DCLANG_INCLUDE_DIRS:STRING=/usr/include/clang/3.8/ \
    -DLLVM_CONFIG_EXECUTABLE:STRING=/usr/bin/llvm-config-3.8 \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_BUILD_TYPE=Debug \
    ..
make

popd
popd
