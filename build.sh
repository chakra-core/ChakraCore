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
    echo "[ChakraCore Build Script Help]"
    echo ""
    echo "  build.sh [options]"
    echo ""
    echo "  options:"
    echo "    --cxx             : Path to Clang++ (see example below)"
    echo "    --cc              : Path to Clang   (see example below)"
    echo "    --debug, -d       : Debug build (by default Release build)"
    echo "    --help, -h        : Show help"
    echo "    --icu             : Path to ICU include folder (see example below)"
    echo "    --jobs, -j        : Multicore build (i.e. -j=3 for 3 cores)"
    echo "    --ninja, -n       : Build with ninja instead of make"
    echo "    --test-build, -t  : Test build (by default Release build)"
    echo "    --verbose, -v     : Display verbose output including all options"
    echo ""
    echo "  example:"
    echo "    ./build.sh --cxx=/path/to/clang++ --cc=/path/to/clang -j=2"
    echo "  with icu:"
    echo "    ./build.sh --icu=/usr/local/Cellar/icu4c/version/include/"
    echo ""
}

_CXX=""
_CC=""
VERBOSE=""
BUILD_TYPE="Release"
CMAKE_GEN=
MAKE=make
MULTICORE_BUILD=""
ICU_PATH=""

while [[ $# -gt 0 ]]; do
    if [[ "$1" =~ "--cxx" ]]; then
        _CXX=$1
        _CXX=${_CXX:6}
    fi

    if [[ "$1" =~ "--cc" ]]; then
        _CC=$1
        _CC=${_CC:5}
    fi

    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        PRINT_USAGE
        exit
    fi

    if [[ "$1" == "--verbose" || "$1" == "-v" ]]; then
        _VERBOSE="verbose"
    fi

    if [[ "$1" == "--debug" || "$1" == "-d" ]]; then
        BUILD_TYPE="Debug"
    fi

    if [[ "$1" == "--test-build" || "$1" == "-t" ]]; then
        BUILD_TYPE="Test"
    fi

    if [[ "$1" =~ "-j" ]]; then
        MULTICORE_BUILD=$1
        MULTICORE_BUILD="-j ${MULTICORE_BUILD:3}"
    fi

    if [[ "$1" =~ "--jobs" ]]; then
        MULTICORE_BUILD=$1
        MULTICORE_BUILD="-j ${MULTICORE_BUILD:7}"
    fi

    if [[ "$1" =~ "--icu" ]]; then
        ICU_PATH=$1
        ICU_PATH="-DICU_INCLUDE_PATH=${ICU_PATH:6}"
    fi

    if [[ "$1" == "--ninja" || "$1" == "-n" ]]; then
        CMAKE_GEN="-G Ninja"
        MAKE=ninja
    fi

    shift
done

if [[ ${#_VERBOSE} > 0 ]]; then
    # echo options back to the user
    echo "Printing command line options back to the user:"
    echo "_CXX=${_CXX}"
    echo "_CC=${_CC}"
    echo "BUILD_TYPE=${BUILD_TYPE}"
    echo "MULTICORE_BUILD=${MULTICORE_BUILD}"
    echo "ICU_PATH=${ICU_PATH}"
    echo "CMAKE_GEN=${CMAKE_GEN}"
    echo "MAKE=${MAKE}"
    echo ""
fi

if [[ ${#_CXX} > 0 || ${#_CC} > 0 ]]; then
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

echo Generating $BUILD_TYPE makefiles
cmake $CMAKE_GEN $CC_PREFIX $ICU_PATH -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

$MAKE $MULTICORE_BUILD 2>&1 | tee build.log
popd > /dev/null
