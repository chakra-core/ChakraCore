#!/bin/bash
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
    echo "build.sh [options]"
    echo ""
    echo "options:"
    echo "      --cxx=PATH       Path to Clang++ (see example below)"
    echo "      --cc=PATH        Path to Clang   (see example below)"
    echo "  -d, --debug-build    Debug build (by default Release build)"
    echo "  -h, --help           Show help"
    echo "      --icu=PATH       Path to ICU include folder (see example below)"
    echo "  -j [N], --jobs[=N]   Multicore build, allow N jobs at once"
    echo "  -n, --ninja          Build with ninja instead of make"
    echo "      --xcode          Generate XCode project"
    echo "  -t, --test-build     Test build (by default Release build)"
    echo "      --static         Build as static library (by default shared library)"
    echo "  -v, --verbose        Display verbose output including all options"
    echo "      --create-deb=V   Create .deb package with given V version"
    echo "      --without=FEATURE,FEATURE,..."
    echo "                       Disable FEATUREs from JSRT experimental"
    echo "                       features."
    echo ""
    echo "example:"
    echo "  ./build.sh --cxx=/path/to/clang++ --cc=/path/to/clang -j"
    echo "with icu:"
    echo "  ./build.sh --icu=/usr/local/Cellar/icu4c/version/include/"
    echo ""
}

CHAKRACORE_DIR=`dirname $0`
_CXX=""
_CC=""
VERBOSE=""
BUILD_TYPE="release"
BUILD_ARCH="x64"
CMAKE_GEN=
MAKE=make
MULTICORE_BUILD=""
ICU_PATH=""
STATIC_LIBRARY=""
WITHOUT_FEATURES=""
CREATE_DEB=0

while [[ $# -gt 0 ]]; do
    case "$1" in
    --cxx=*)
        _CXX=$1
        _CXX=${_CXX:6}
        ;;

    --cc=*)
        _CC=$1
        _CC=${_CC:5}
        ;;

    -h | --help)
        PRINT_USAGE
        exit
        ;;

    -v | --verbose)
        _VERBOSE="verbose"
        ;;

    -d | --debug-build)
        BUILD_TYPE="debug"
        ;;

    -t | --test-build)
        BUILD_TYPE="test"
        ;;

    -j | --jobs)
        if [[ "$1" == "-j" && "$2" =~ ^[^-] ]]; then
            MULTICORE_BUILD="-j $2"
            shift
        else
            MULTICORE_BUILD="-j $(nproc)"
        fi
        ;;

    -j=* | --jobs=*)            # -j=N syntax used in CI
        MULTICORE_BUILD=$1
        if [[ "$1" =~ ^-j= ]]; then
            MULTICORE_BUILD="-j ${MULTICORE_BUILD:3}"
        else
            MULTICORE_BUILD="-j ${MULTICORE_BUILD:7}"
        fi
        ;;

    --icu=*)
        ICU_PATH=$1
        ICU_PATH="-DICU_INCLUDE_PATH=${ICU_PATH:6}"
        ;;

    -n | --ninja)
        CMAKE_GEN="-G Ninja"
        MAKE=ninja
        ;;

    --xcode)
        CMAKE_GEN="-G Xcode -DCC_XCODE_PROJECT=1"
        MAKE=0
        ;;

    --create-deb=*)
        CREATE_DEB=$1
        CREATE_DEB="${CREATE_DEB:13}"
        ;;

    --static)
        STATIC_LIBRARY="-DSTATIC_LIBRARY=1"
        ;;

    --without=*)
        FEATURES=$1
        FEATURES=${FEATURES:10}    # value after --without=
        for x in ${FEATURES//,/ }  # replace comma with space then split
        do
            if [[ "$WITHOUT_FEATURES" == "" ]]; then
                WITHOUT_FEATURES="-DWITHOUT_FEATURES="
            else
                WITHOUT_FEATURES="$WITHOUT_FEATURES;"
            fi
            WITHOUT_FEATURES="${WITHOUT_FEATURES}-DCOMPILE_DISABLE_${x}=1"
        done
        ;;

    *)
        echo "Unknown option $1"
        PRINT_USAGE
        exit -1
        ;;
    esac

    shift
done

if [[ ${#_VERBOSE} > 0 ]]; then
    # echo options back to the user
    echo "Printing command line options back to the user:"
    echo "_CXX=${_CXX}"
    echo "_CC=${_CC}"
    echo "BUILD_TYPE=${BUILD_TYPE}"
    echo "BUILD_ARCH=${BUILD_ARCH}"
    echo "MULTICORE_BUILD=${MULTICORE_BUILD}"
    echo "ICU_PATH=${ICU_PATH}"
    echo "CMAKE_GEN=${CMAKE_GEN}"
    echo "MAKE=${MAKE}"
    echo ""
fi

CLANG_PATH=
if [[ ${#_CXX} > 0 || ${#_CC} > 0 ]]; then
    if [[ ${#_CXX} == 0 || ${#_CC} == 0 ]]; then
        echo "ERROR: '-cxx' and '-cc' options must be used together."
        exit 1
    fi
    echo "Custom CXX ${_CXX}"
    echo "Custom CC  ${_CC}"

    if [[ ! -f $_CXX || ! -f $_CC ]]; then
        echo "ERROR: Custom compiler not found on given path"
        exit 1
    fi
    CLANG_PATH=$_CXX
else
    RET_VAL=$(SAFE_RUN 'c++ --version')
    if [[ ! $RET_VAL =~ "clang" ]]; then
        echo "Searching for Clang..."
        if [[ -f /usr/bin/clang++ ]]; then
            echo "Clang++ found at /usr/bin/clang++"
            _CXX=/usr/bin/clang++
            _CC=/usr/bin/clang
            CLANG_PATH=$_CXX
        else
            echo "ERROR: clang++ not found at /usr/bin/clang++"
            echo ""
            echo "You could use clang++ from a custom location."
            PRINT_USAGE
            exit 1
        fi
    else
        CLANG_PATH=c++
    fi
fi

# check clang version (min required 3.7)
VERSION=$($CLANG_PATH --version | grep "version [0-9]*\.[0-9]*" --o -i | grep "[0-9]\.[0-9]*" --o)
VERSION=${VERSION/./}

if [[ $VERSION -lt 37 ]]; then
    echo "ERROR: Minimum required Clang version is 3.7"
    exit 1
fi

CC_PREFIX=""
if [[ ${#_CXX} > 0 ]]; then
    CC_PREFIX="-DCMAKE_CXX_COMPILER=$_CXX -DCMAKE_C_COMPILER=$_CC"
fi

root_build_directory="$CHAKRACORE_DIR/Build/clang_build/${BUILD_ARCH}_${BUILD_TYPE}"
if [ ! -d "$root_build_directory" ]; then
    SAFE_RUN `mkdir -p $root_build_directory`
fi

bin_directory="$root_build_directory/bin"
if [ ! -d "$bin_directory" ]; then
    SAFE_RUN `mkdir -p $bin_directory`
fi

build_directory="$root_build_directory/obj"
if [ ! -d "$build_directory" ]; then
    SAFE_RUN `mkdir -p $build_directory`
fi

pushd $build_directory > /dev/null

echo Generating $BUILD_TYPE makefiles
cmake $CMAKE_GEN $CC_PREFIX $ICU_PATH $STATIC_LIBRARY \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE^} $WITHOUT_FEATURES ../../../..

_RET=$?
if [[ $? == 0 ]]; then
    if [[ $MAKE != 0 ]]; then
        $MAKE $MULTICORE_BUILD 2>&1 | tee ../build.log
        _RET=${PIPESTATUS[0]}
    else
        echo "Visit given folder above for xcode project file ----^"
    fi
fi

if [[ $_RET != 0 ]]; then
    echo "See error details above. Exit code was $_RET"
else
    if [[ $CREATE_DEB != 0 ]]; then
        DEB_FOLDER=`realpath .`
        DEB_FOLDER="${DEB_FOLDER}/chakracore_${CREATE_DEB}"

        mkdir -p $DEB_FOLDER/usr/local/bin
        mkdir -p $DEB_FOLDER/DEBIAN
        cp $DEB_FOLDER/../ch $DEB_FOLDER/usr/local/bin/
        if [[ $STATIC_LIBRARY == "" ]]; then
            cp $DEB_FOLDER/../*.so $DEB_FOLDER/usr/local/bin/
        fi
        echo -e "Package: ChakraCore"\
            "\nVersion: ${CREATE_DEB}"\
            "\nSection: base"\
            "\nPriority: optional"\
            "\nArchitecture: amd64"\
            "\nDepends: libc6 (>= 2.19), uuid-dev (>> 0), libunwind-dev (>> 0), libicu-dev (>> 0)"\
            "\nMaintainer: ChakraCore <chakracore@microsoft.com>"\
            "\nDescription: Chakra Core"\
            "\n Open source Core of Chakra Javascript Engine"\
            > $DEB_FOLDER/DEBIAN/control

        dpkg-deb --build $DEB_FOLDER
        _RET=$?
        if [[ $_RET == 0 ]]; then
            echo ".deb package is available under $build_directory"
        fi
    fi
fi

popd > /dev/null
exit $_RET
