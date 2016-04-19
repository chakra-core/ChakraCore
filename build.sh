#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

SAFE_RUN() {
    local SF_RETURN_VALUE=$($1 2>&1)

    if [[ $? != 0 ]]; then
        >&2 echo $SF_RETURN_VALUE
        exit 1
    fi
    echo $SF_RETURN_VALUE
}

PRINT_USAGE() {
    echo ""
    echo "build.sh [options]"
    echo ""
    echo "options:"
    echo "--help, -h : Show help"
    echo "--debug, -d : Debug build"
    echo "CXX=<path to clang++>"
    echo "CC=<path to clang>"
    echo ""
    echo "example: ./build.sh CXX=/path/to/clang++ CC=/path/to/clang"
}

_CXX=""
_CC=""
DEBUG_BUILD=0

while [[ $# -gt 0 ]]; do
    if [[ "$1" =~ "CXX=" ]]; then
        _CXX=$1
        _CXX=${_CXX:4}
    fi

    if [[ "$1" =~ "CC=" ]]; then
        _CC=$1
        _CC=${_CC:3}
    fi

    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        PRINT_USAGE
        exit
    fi

    if [[ "$1" == "--debug" || "$1" == "-d" ]]; then
        DEBUG_BUILD=1
    fi

    shift
done

if [[ ${#_CXX} > 0 && ${#_CC} > 0 ]]; then
    echo "Custom CXX ${_CXX}"
    echo "Custom CC  ${_CC}"

    if [[ ! -f $_CXX || ! -f $_CC ]]; then
        echo "ERROR: Custom compiler not found on given path"
        exit 1
    fi
else
    RET_VAL=$(SAFE_RUN 'c++ --version')
    if [[ ! $RET_VAL =~ "clang" ]]; then
        echo "Searching for Clang..."
        if [[ -f /usr/bin/clang++ ]]; then
            echo "Clang++ found at /usr/bin/clang++"
            _CXX=/usr/bin/clang++
            _CC=/usr/bin/clang
        else
            echo "ERROR: clang++ not found at /usr/bin/clang++"
            echo ""
            echo "You could use clang++ from a custom location."
            PRINT_USAGE
            exit 1
        fi
    fi
fi

CC_PREFIX=""
if [[ ${#_CXX} > 0 ]]; then
    CC_PREFIX="-DCMAKE_CXX_COMPILER=$_CXX -DCMAKE_C_COMPILER=$_CC"
fi

if [ ! -d "BuildLinux" ]; then
    SAFE_RUN 'mkdir BuildLinux'
fi

pushd BuildLinux > /dev/null

if [ $DEBUG_BUILD -eq 1 ]; then
    echo Generating Debug makefiles
    cmake $CC_PREFIX -DCMAKE_BUILD_TYPE=Debug ..
else
    echo Generating Retail makefiles
    echo Building Retail;
    cmake $CC_PREFIX -DCMAKE_BUILD_TYPE=Release ..
fi

make -j $(nproc) |& tee build.log
popd > /dev/null
