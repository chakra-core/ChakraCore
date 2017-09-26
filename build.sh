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

ERROR_EXIT() {
    if [[ $? != 0 ]]; then
        echo $($1 2>&1)
        exit 1;
    fi
}

ERROR_CLANG() {
    echo "ERROR: clang++ not found."
    echo -e "\nYou could use clang++ from a custom location.\n"
    PRINT_USAGE
    exit 1
}

PRINT_USAGE() {
    echo ""
    echo "[ChakraCore Build Script Help]"
    echo ""
    echo "build.sh [options]"
    echo ""
    echo "options:"
    echo "     --arch[=S]        Set target arch (arm, x86, amd64)"
    echo "     --cc=PATH         Path to Clang   (see example below)"
    echo "     --cxx=PATH        Path to Clang++ (see example below)"
    echo "     --create-deb[=V]  Create .deb package with given V version."
    echo " -d, --debug           Debug build. Default: Release"
    echo "     --embed-icu       Download and embed ICU-57 statically."
    echo "     --extra-defines=DEF=VAR,DEFINE,..."
    echo "                       Compile with additional defines"
    echo " -h, --help            Show help"
    echo "     --icu=PATH        Path to ICU include folder (see example below)"
    echo " -j[=N], --jobs[=N]    Multicore build, allow N jobs at once."
    echo " -n, --ninja           Build with ninja instead of make."
    echo "     --no-icu          Compile without unicode/icu support."
    echo "     --no-jit          Disable JIT"
    echo "     --libs-only       Do not build CH and GCStress"
    echo "     --lto             Enables LLVM Full LTO"
    echo "     --lto-thin        Enables LLVM Thin LTO - xcode 8+ or clang 3.9+"
    echo "     --static          Build as static library. Default: shared library"
    echo "     --sanitize=CHECKS Build with clang -fsanitize checks,"
    echo "                       e.g. undefined,signed-integer-overflow."
    echo " -t, --test-build      Test build. Enables test flags on a release build."
    echo "     --target[=S]      Target OS (i.e. android)"
    echo "     --target-path[=S] Output path for compiled binaries. Default: out/"
    echo "     --trace           Enables experimental built-in trace."
    echo "     --xcode           Generate XCode project."
    echo "     --with-intl       Include the Intl object (requires ICU)."
    echo "     --without=FEATURE,FEATURE,..."
    echo "                       Disable FEATUREs from JSRT experimental features."
    echo "     --valgrind        Enable Valgrind support"
    echo "                       !!! Disables Concurrent GC (lower performance)"
    echo " -v, --verbose         Display verbose output including all options"
    echo "     --wb-check CPPFILE"
    echo "                       Write-barrier check given CPPFILE (git path)"
    echo "     --wb-analyze CPPFILE"
    echo "                       Write-barrier analyze given CPPFILE (git path)"
    echo "     --wb-args=PLUGIN_ARGS"
    echo "                       Write-barrier clang plugin args"
    echo " -y                    Automatically answer Yes to questions asked by \
script (at your own risk)"
    echo ""
    echo "example:"
    echo "  ./build.sh --cxx=/path/to/clang++ --cc=/path/to/clang -j"
    echo "with icu:"
    echo "  ./build.sh --icu=/usr/local/opt/icu4c/include"
    echo ""
}

pushd `dirname $0` > /dev/null
CHAKRACORE_DIR=`pwd -P`
popd > /dev/null
_CXX=""
_CC=""
_VERBOSE=""
BUILD_TYPE="Release"
CMAKE_GEN=
EXTRA_DEFINES=""
MAKE=make
MULTICORE_BUILD=""
NO_JIT=
ICU_PATH="-DICU_SETTINGS_RESET=1"
STATIC_LIBRARY="-DSHARED_LIBRARY_SH=1"
SANITIZE=
WITHOUT_FEATURES=""
CREATE_DEB=0
ARCH="-DCC_USES_SYSTEM_ARCH_SH=1"
OS_LINUX=0
OS_APT_GET=0
OS_UNIX=0
LTO=""
TARGET_OS=""
ENABLE_CC_XPLAT_TRACE=""
WB_CHECK=
WB_ANALYZE=
WB_ARGS=
TARGET_PATH=0
VALGRIND=0
# -DCMAKE_EXPORT_COMPILE_COMMANDS=ON useful for clang-query tool
CMAKE_EXPORT_COMPILE_COMMANDS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
LIBS_ONLY_BUILD=
SHOULD_EMBED_ICU=0
ALWAYS_YES=0

if [ -f "/proc/version" ]; then
    OS_LINUX=1
    PROC_INFO=$(cat /proc/version)
    if [[ $PROC_INFO =~ 'Ubuntu' || $PROC_INFO =~ 'Debian'
       || $PROC_INFO =~ 'Linaro' ]]; then
        OS_APT_GET=1
    fi
elif [[ $(uname -s) =~ "Darwin" ]]; then
    OS_UNIX=1
else
    echo -e "Warning: Installation script couldn't detect host OS..\n" # exit ?
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
    --arch=*)
        ARCH=$1
        ARCH="${ARCH:7}"
        ;;

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
        _VERBOSE="V=1"
        ;;

    -d | --debug)
        BUILD_TYPE="Debug"
        ;;

    --embed-icu)
        SHOULD_EMBED_ICU=1
        ;;

    --extra-defines=*)
        DEFINES=$1
        DEFINES=${DEFINES:16}    # value after --extra-defines=
        for x in ${DEFINES//,/ }  # replace comma with space then split
        do
            if [[ "$EXTRA_DEFINES" == "" ]]; then
                EXTRA_DEFINES="-DEXTRA_DEFINES_SH="
            else
                EXTRA_DEFINES="$EXTRA_DEFINES;"
            fi
            EXTRA_DEFINES="${EXTRA_DEFINES}-D${x}"
        done
        ;;

    -t | --test-build)
        BUILD_TYPE="Test"
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
        # resolve tilde on path
        eval ICU_PATH="${ICU_PATH:6}"
        if [[ ! -d ${ICU_PATH} ]]; then
            if [[ -d "${CHAKRACORE_DIR}/${ICU_PATH}" ]]; then
                ICU_PATH="${CHAKRACORE_DIR}/${ICU_PATH}"
            else
                # if ICU_PATH is given, do not fallback to no-icu
                echo "!!! couldn't find ICU at $ICU_PATH"
                exit 1
            fi
        fi
        ICU_PATH="-DICU_INCLUDE_PATH_SH=${ICU_PATH}"
        ;;

    --libs-only)
        LIBS_ONLY_BUILD="-DLIBS_ONLY_BUILD_SH=1"
        ;;

    --lto)
        LTO="-DENABLE_FULL_LTO_SH=1"
        HAS_LTO=1
        ;;

    --lto-thin)
        LTO="-DENABLE_THIN_LTO_SH=1"
        HAS_LTO=1
        ;;

    -n | --ninja)
        CMAKE_GEN="-G Ninja"
        MAKE=ninja
        ;;

    --no-icu)
        ICU_PATH="-DNO_ICU_PATH_GIVEN_SH=1"
        ;;

    --no-jit)
        NO_JIT="-DNO_JIT_SH=1"
        ;;

    --with-intl)
        INTL_ICU="-DINTL_ICU_SH=1"
        ;;

    --xcode)
        CMAKE_GEN="-G Xcode -DCC_XCODE_PROJECT=1"
        CMAKE_EXPORT_COMPILE_COMMANDS=""
        MAKE=0
        ;;

    --create-deb=*)
        CREATE_DEB=$1
        CREATE_DEB="${CREATE_DEB:13}"
        ;;

    --static)
        STATIC_LIBRARY="-DSTATIC_LIBRARY_SH=1"
        ;;

    --sanitize=*)
        SANITIZE=$1
        SANITIZE=${SANITIZE:11}    # value after --sanitize=
        SANITIZE="-DCLANG_SANITIZE_SH=${SANITIZE}"
        ;;

    --target=*)
        _TARGET_OS=$1
        _TARGET_OS="${_TARGET_OS:9}"
        if [[ $_TARGET_OS =~ "android" ]]; then
            OLD_PATH=$PATH
            export TOOLCHAIN=$PWD/android-toolchain-arm
            TARGET_OS="-DCC_TARGET_OS_ANDROID_SH=1 -DANDROID_TOOLCHAIN_DIR=${TOOLCHAIN}/arm-linux-androideabi"
            export PATH=$TOOLCHAIN/bin:$OLD_PATH
            export AR=arm-linux-androideabi-ar
            export CC=arm-linux-androideabi-clang
            export CXX=arm-linux-androideabi-clang++
            export LINK=arm-linux-androideabi-clang++
            export STRIP=arm-linux-androideabi-strip
            # override CXX and CC
            _CXX="${TOOLCHAIN}/bin/${CXX}"
            _CC="${TOOLCHAIN}/bin/${CC}"
        fi
        ;;

    --trace)
        ENABLE_CC_XPLAT_TRACE="-DENABLE_CC_XPLAT_TRACE_SH=1"
        ;;

    --target-path=*)
        TARGET_PATH=$1
        TARGET_PATH=${TARGET_PATH:14}
        ;;

    --without=*)
        FEATURES=$1
        FEATURES=${FEATURES:10}    # value after --without=
        for x in ${FEATURES//,/ }  # replace comma with space then split
        do
            if [[ "$WITHOUT_FEATURES" == "" ]]; then
                WITHOUT_FEATURES="-DWITHOUT_FEATURES_SH="
            else
                WITHOUT_FEATURES="$WITHOUT_FEATURES;"
            fi
            WITHOUT_FEATURES="${WITHOUT_FEATURES}-DCOMPILE_DISABLE_${x}=1"
        done
        ;;

    --wb-check)
        if [[ "$2" =~ ^[^-] ]]; then
            WB_CHECK="$2"
            shift
        else
            WB_CHECK="*"  # check all files
        fi
        ;;

    --wb-analyze)
        if [[ "$2" =~ ^[^-] ]]; then
            WB_ANALYZE="$2"
            shift
        else
            PRINT_USAGE && exit 1
        fi
        ;;

    --wb-args=*)
        WB_ARGS=$1
        WB_ARGS=${WB_ARGS:10}
        WB_ARGS=${WB_ARGS// /;}  # replace space with ; to generate a cmake list
        ;;

    --valgrind)
        VALGRIND="-DENABLE_VALGRIND_SH=1"
        ;;

    -y | -Y)
        ALWAYS_YES=1
        ;;

    *)
        echo "Unknown option $1"
        PRINT_USAGE
        exit -1
        ;;
    esac

    shift
done

if [[ $SHOULD_EMBED_ICU == 1 ]]; then
    if [ ! -d "${CHAKRACORE_DIR}/deps/icu/source/output" ]; then
        ICU_URL="http://source.icu-project.org/repos/icu/icu/tags/release-57-1"
        echo -e "\n----------------------------------------------------------------"
        echo -e "\nThis script will download ICU-LIB from\n${ICU_URL}\n"
        echo "It is licensed to you by its publisher, not Microsoft."
        echo "Microsoft is not responsible for the software."
        echo "Your installation and use of ICU-LIB is subject to the publisher's terms available here:"
        echo -e "http://www.unicode.org/copyright.html#License\n"
        echo -e "----------------------------------------------------------------\n"
        echo "If you don't agree, press Ctrl+C to terminate"
        WAIT_QUESTION="Hit ENTER to continue (or wait 10 seconds)"
        if [[ $ALWAYS_YES == 1 ]]; then
            echo "$WAIT_QUESTION : Y"
        else
            read -t 10 -p "$WAIT_QUESTION"
        fi

        SAFE_RUN `mkdir -p ${CHAKRACORE_DIR}/deps/`
        cd "${CHAKRACORE_DIR}/deps/";
        ABS_DIR=`pwd`
        if [ ! -d "${ABS_DIR}/icu/" ]; then
            echo "Downloading ICU ${ICU_URL}"
            if [ ! -f "/usr/bin/svn" ]; then
                echo -e "\nYou should install 'svn' client in order to use this feature"
                if [ $OS_APT_GET == 1 ]; then
                    echo "tip: Try 'sudo apt-get install subversion'"
                fi
                exit 1
            fi
            svn export -q $ICU_URL icu
            ERROR_EXIT "rm -rf ${ABS_DIR}/icu/"
        fi

        cd "${ABS_DIR}/icu/source";./configure --with-data-packaging=static\
                --prefix="${ABS_DIR}/icu/source/output/"\
                --enable-static --disable-shared --with-library-bits=64\
                --disable-icuio --disable-layout\
                CXXFLAGS="-fPIC" CFLAGS="-fPIC"

        ERROR_EXIT "rm -rf ${ABS_DIR}/icu/source/output/"
        make STATICCFLAGS="-fPIC" STATICCXXFLAGS="-fPIC" STATICCPPFLAGS="-DPIC" install
        ERROR_EXIT "rm -rf ${ABS_DIR}/icu/source/output/"
        cd "${ABS_DIR}/../"
    fi
    ICU_PATH="-DCC_EMBED_ICU_SH=1"
fi

if [[ ${#_VERBOSE} > 0 ]]; then
    # echo options back to the user
    echo "Printing command line options back to the user:"
    echo "_CXX=${_CXX}"
    echo "_CC=${_CC}"
    echo "BUILD_TYPE=${BUILD_TYPE}"
    echo "MULTICORE_BUILD=${MULTICORE_BUILD}"
    echo "ICU_PATH=${ICU_PATH}"
    echo "CMAKE_GEN=${CMAKE_GEN}"
    echo "MAKE=${MAKE} $_VERBOSE"
    echo ""
fi

# if LTO build is enabled and cc-toolchain/clang was compiled, use it instead
if [[ $HAS_LTO == 1 ]]; then
    if [[ -f "${CHAKRACORE_DIR}/cc-toolchain/build/bin/clang++" ]]; then
        SELF=`pwd`
        _CXX="$CHAKRACORE_DIR/cc-toolchain/build/bin/clang++"
        _CC="$CHAKRACORE_DIR/cc-toolchain/build/bin/clang"
    else
        # Linux LD possibly doesn't support LLVM LTO, check.. and compile clang if not
        if [[ $OS_LINUX == 1 ]]; then
            if [[ ! `ld -v` =~ 'GNU gold' ]]; then
                pushd "$CHAKRACORE_DIR" > /dev/null
                $CHAKRACORE_DIR/tools/compile_clang.sh
                if [[ $? != 0 ]]; then
                  echo -e "tools/compile_clang.sh has failed.\n"
                  echo "Try with 'sudo' ?"
                  popd > /dev/null
                  exit 1
                fi
                _CXX="$CHAKRACORE_DIR/cc-toolchain/build/bin/clang++"
                _CC="$CHAKRACORE_DIR/cc-toolchain/build/bin/clang"
                popd > /dev/null
            fi
        fi
    fi
fi

if [ "${HAS_LTO}${OS_LINUX}" == "11" ]; then
    echo "lto: ranlib disabled"
    export RANLIB=/bin/true
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
            # try env CXX and CC
            if [[ ! -f $CXX  || ! -f $CC  ]]; then
                ERROR_CLANG
            fi

            _CXX=$CXX
            _CC=$CC
            CLANG_PATH=$CXX
            VERSION=$($CXX --version)
            if [[ ! $VERSION =~ "clang" ]]; then
                ERROR_CLANG
            fi
            echo -e "Clang++ not found on PATH.\nTrying CCX -> ${CCX} and CC -> ${CC}"
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

RELATIVE_BUILD_PATH="../.."
if [[ $TARGET_PATH == 0 ]]; then
    TARGET_PATH="$CHAKRACORE_DIR/out"
else
    if [[ $TARGET_PATH =~ "~/" ]]; then
        echo "Do not use '~/' for '--target-path'"
        echo -e "\nAborting Build."
        exit 1
    fi
fi

BUILD_DIRECTORY="${TARGET_PATH}/${BUILD_TYPE:0}"
echo "Build path: ${BUILD_DIRECTORY}"

BUILD_RELATIVE_DIRECTORY=$(python -c "import os.path;print \
    os.path.relpath('${CHAKRACORE_DIR}', '$BUILD_DIRECTORY')")

################# Write-barrier check/analyze run #################
WB_FLAG=
WB_TARGET=
if [[ $WB_CHECK || $WB_ANALYZE ]]; then
    # build software write barrier checker clang plugin
    $CHAKRACORE_DIR/tools/RecyclerChecker/build.sh --cxx=$_CXX || exit 1

    if [[ $WB_CHECK && $WB_ANALYZE ]]; then
        echo "Please run only one of --wb-check or --wb-analyze" && exit 1
    fi
    if [[ $WB_CHECK ]]; then
        WB_FLAG="-DWB_CHECK_SH=1"
        WB_FILE=$WB_CHECK
    fi
    if [[ $WB_ANALYZE ]]; then
        WB_FLAG="-DWB_ANALYZE_SH=1"
        WB_FILE=$WB_ANALYZE
    fi

    if [[ $WB_ARGS ]]; then
        if [[ $WB_ARGS =~ "-fix" ]]; then
            MULTICORE_BUILD="-j 1"  # 1 job only if doing write barrier fix
        fi
        WB_ARGS="-DWB_ARGS_SH=$WB_ARGS"
    fi

    # support --wb-check ONE_CPP_FILE
    if [[ $WB_FILE != "*" ]]; then
        if [[ $MAKE != 'ninja' ]]; then
            echo "--wb-check/wb-analyze ONE_FILE only works with --ninja" && exit 1
        fi

        if [[ -f $CHAKRACORE_DIR/$WB_FILE ]]; then
            touch $CHAKRACORE_DIR/$WB_FILE
        else
            echo "$CHAKRACORE_DIR/$WB_FILE not found. Please use full git path for $WB_FILE." && exit 1
        fi

        WB_FILE_DIR=`dirname $WB_FILE`
        WB_FILE_BASE=`basename $WB_FILE`

        WB_FILE_CMAKELISTS="$CHAKRACORE_DIR/$WB_FILE_DIR/CMakeLists.txt"
        if [[ -f $WB_FILE_CMAKELISTS ]]; then
            SUBDIR=$(grep -i add_library $WB_FILE_CMAKELISTS | sed "s/.*(\([^ ]*\) .*/\1/")
        else
            echo "$WB_FILE_CMAKELISTS not found." && exit 1
        fi
        WB_TARGET="$WB_FILE_DIR/CMakeFiles/$SUBDIR.dir/$WB_FILE_BASE.o"
    fi
fi

# prepare DbgController.js.h
CH_DIR="${CHAKRACORE_DIR}/bin/ch"
"${CH_DIR}/jstoc.py" "${CH_DIR}/DbgController.js" controllerScript
if [[ $? != 0 ]]; then
    exit 1
fi

if [ ! -d "$BUILD_DIRECTORY" ]; then
    SAFE_RUN `mkdir -p $BUILD_DIRECTORY`
fi
pushd $BUILD_DIRECTORY > /dev/null

if [[ $ARCH =~ "x86" ]]; then
    ARCH="-DCC_TARGETS_X86_SH=1"
    echo "Compile Target : x86"
elif [[ $ARCH =~ "arm" ]]; then
    ARCH="-DCC_TARGETS_ARM_SH=1"
    echo "Compile Target : arm"
elif [[ $ARCH =~ "amd64" ]]; then
    ARCH="-DCC_TARGETS_AMD64_SH=1"
    echo "Compile Target : amd64"
else
    echo "Compile Target : System Default"
fi

echo Generating $BUILD_TYPE makefiles
echo $EXTRA_DEFINES
cmake $CMAKE_GEN $CC_PREFIX $ICU_PATH $LTO $STATIC_LIBRARY $ARCH $TARGET_OS \
    $ENABLE_CC_XPLAT_TRACE $EXTRA_DEFINES -DCMAKE_BUILD_TYPE=$BUILD_TYPE $SANITIZE $NO_JIT $INTL_ICU \
    $WITHOUT_FEATURES $WB_FLAG $WB_ARGS $CMAKE_EXPORT_COMPILE_COMMANDS $LIBS_ONLY_BUILD\
    $VALGRIND $BUILD_RELATIVE_DIRECTORY

_RET=$?
if [[ $? == 0 ]]; then
    if [[ $MAKE != 0 ]]; then
        if [[ $MAKE != 'ninja' ]]; then
            # $MFLAGS comes from host `make` process. Sub `make` process needs this (recursional make runs)
            TEST_MFLAGS="${MFLAGS}*!"
            if [[ $TEST_MFLAGS != "*!" ]]; then
                # Get -j flag from the host
                MULTICORE_BUILD=""
            fi
            $MAKE $MFLAGS $MULTICORE_BUILD $_VERBOSE $WB_TARGET 2>&1 | tee build.log
        else
            $MAKE $MULTICORE_BUILD $_VERBOSE $WB_TARGET 2>&1 | tee build.log
        fi
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
        if [[ $STATIC_LIBRARY == "-DSHARED_LIBRARY_SH=1" ]]; then
            cp $DEB_FOLDER/../*.so $DEB_FOLDER/usr/local/bin/
        fi
        echo -e "Package: ChakraCore"\
            "\nVersion: ${CREATE_DEB}"\
            "\nSection: base"\
            "\nPriority: optional"\
            "\nArchitecture: amd64"\
            "\nDepends: libc6 (>= 2.19), uuid-dev (>> 0), libicu-dev (>> 0)"\
            "\nMaintainer: ChakraCore <chakracore@microsoft.com>"\
            "\nDescription: Chakra Core"\
            "\n Open source Core of Chakra Javascript Engine"\
            > $DEB_FOLDER/DEBIAN/control

        dpkg-deb --build $DEB_FOLDER
        _RET=$?
        if [[ $_RET == 0 ]]; then
            echo ".deb package is available under $BUILD_DIRECTORY"
        fi
    fi
fi

popd > /dev/null
exit $_RET
