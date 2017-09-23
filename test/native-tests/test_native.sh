#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

CH_DIR=$1
BUILD_TYPE=$2
RES=
CC=0
CXX=0

FIND_CLANG() {
    for i in 7 8 9
    do
        if [[ -f "/usr/bin/clang-3.${i}" ]]; then
            CC="/usr/bin/clang-3.${i}"
            CXX="/usr/bin/clang++-3.${i}"
        fi
    done
    if [[ $CC == 0 ]]; then
        echo "Error: Couldn't find Clang"
        exit 1
    else
    echo "Using CC [${CC}]"
    echo "Using CXX [${CXX}]"
    fi
}

SAFE_RUN() {
    local SF_RETURN_VALUE=$($1 2>&1)

    if [[ $? != 0 ]]; then
        >&2 echo $SF_RETURN_VALUE
        exit 1
    fi
}

TEST () {
    if [[ $RES =~ $1 ]]; then
        echo "${TEST_PATH} : PASS"
    else
        echo "${TEST_PATH} FAILED"
        echo -e "$RES"
        exit 1
    fi
}

RES=$(c++ --version)
if [[ ! $RES =~ "Apple LLVM" ]]; then
    FIND_CLANG
else
    CC="cc"
    CXX="c++"
fi

RUN () {
    TEST_PATH=$1
    echo "Testing $TEST_PATH"
    SAFE_RUN `cd $TEST_PATH; ${CH_DIR} Platform.js > Makefile`
    RES=$(cd $TEST_PATH; cat Makefile)

    if [[ $RES =~ "# IGNORE_THIS_TEST" ]]; then
        echo "Ignoring $TEST_PATH"
    else
        SAFE_RUN `cd $TEST_PATH; make CC=${CC} CXX=${CXX}`
        RES=$(cd $TEST_PATH; ./sample.o)
        TEST "SUCCESS"
        SAFE_RUN `cd $TEST_PATH; rm -rf ./sample.o`
    fi
}

RUN_CMD () {
    TEST_PATH=$1
    CMD=$2
    echo "Testing $TEST_PATH"
    SAFE_RUN `cd $TEST_PATH; $CMD`
}

# static lib tests

# test-c98
RUN "test-c98"

# test-char
RUN "test-char"

# test-char16
RUN "test-char16"

#test-pal
RUN "test-pal"

# test-static-native
RUN "test-static-native"

# shared lib tests
LIB_DIR="$(dirname ${CH_DIR})"
if [[ `uname -a` =~ "Darwin" ]]; then
    export DYLD_LIBRARY_PATH=${LIB_DIR}/:$DYLD_LIBRARY_PATH
else
    export LD_LIBRARY_PATH=${LIB_DIR}/:$LD_LIBRARY_PATH
fi

RUN "test-shared-basic"

# test python
RUN_CMD "test-python" "python helloWorld.py ${BUILD_TYPE}"

SAFE_RUN `rm -rf Makefile`
