#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

DEFAULT_COLOR='\033[0m'
ERROR_COLOR='\033[0;31m'
GREEN_COLOR='\033[0;32m'

IF_ERROR_EXIT() {
    ERROR_CODE=$1
    if [[ $ERROR_CODE != 0 ]]; then
        echo -e "${ERROR_COLOR} $2 ${DEFAULT_COLOR}"
        exit $ERROR_CODE
    fi
}

PRINT_USAGE() {
    echo -e "\n${GREEN_COLOR}ChakraCore Package Creation Script${DEFAULT_COLOR}"
    echo -e "${ERROR_COLOR}This script may\n- download third party software\n- \
make changes to your system\nand it won't ask you any questions while doing it.${DEFAULT_COLOR}"
    echo -e "For this reason,\nyou should run this script with\n${ERROR_COLOR}\
I_ACCEPT_TO_RUN_THIS${DEFAULT_COLOR} secret argument, ${ERROR_COLOR}at your own risk${DEFAULT_COLOR}\n"

    echo -e "${GREEN_COLOR}Usage:${DEFAULT_COLOR}  tools/create_package.sh <secret argument> <version>"
    echo -e "${GREEN_COLOR}Sample:${DEFAULT_COLOR} tools/create_package.sh <secret argument> 2.0.0\n"
}

if [[ $# < 2 || $1 != "I_ACCEPT_TO_RUN_THIS" ]]; then
    PRINT_USAGE
    exit 0
fi

CHAKRA_VERSION=$2
CHAKRA_VERSION=${CHAKRA_VERSION//./_}

if [[ $2 == $CHAKRA_VERSION ]]; then
    PRINT_USAGE
    echo -e "${ERROR_COLOR}Unexpected version argument. Try something similar to \
2.0.0${DEFAULT_COLOR}"
    exit 1
fi

if [[ ! -f './build.sh' ]]; then
    echo -e "${ERROR_COLOR} Run this script from the repository root folder.${DEFAULT_COLOR}"
    echo "Try -> tools/create_package.sh"
    exit 1
fi

HOST_OS="osx"
HOST_EXT="dylib"
if [[ ! "$OSTYPE" =~ "darwin" ]]; then # osx
    HOST_OS="linux"
    HOST_EXT="so"
    tools/compile_clang.sh -y
    IF_ERROR_EXIT $? "Clang build failed"
fi

## Build
BUILD="./build.sh --embed-icu -y --lto -j=3"
${BUILD} --target-path=out/shared
IF_ERROR_EXIT $? "ChakraCore shared library build failed."
${BUILD} --static --target-path=out/static
IF_ERROR_EXIT $? "ChakraCore static library build failed."

## Create folders
rm -rf out/ChakraCoreFiles/
mkdir -p out/ChakraCoreFiles/include/ out/ChakraCoreFiles/lib/ out/ChakraCoreFiles/bin/ out/ChakraCoreFiles/sample/
IF_ERROR_EXIT $? "Creating ChakraCoreFiles folder failed"

## Copy Files
# lib (so or dylib)
cp "out/shared/Release/libChakraCore.${HOST_EXT}" out/ChakraCoreFiles/lib/
# bin
cp out/static/Release/ch out/ChakraCoreFiles/bin/
# include
cp out/shared/Release/include/*.h out/ChakraCoreFiles/include/
# license
cat LICENSE.txt > out/ChakraCoreFiles/LICENSE
echo -e "\n***** Third Party Notices [ for PreBuilt Binaries ] *****\n" >> out/ChakraCoreFiles/LICENSE
cat tools/XPlatInstall/BINARY-DIST-ONLY-NOTICES.txt >> out/ChakraCoreFiles/LICENSE
# sample
cp "tools/XPlatInstall/sample/README.md"  out/ChakraCoreFiles/sample/README.md
cp "tools/XPlatInstall/sample/Makefile"   out/ChakraCoreFiles/sample/Makefile
cp "tools/XPlatInstall/sample/sample.cpp.txt" out/ChakraCoreFiles/sample/sample.cpp

## Test
python test/native-tests/test-python/helloWorld.py Release \
    "out/ChakraCoreFiles/lib/libChakraCore.${HOST_EXT}"  > /dev/null
IF_ERROR_EXIT $? "Shared library test failed"

out/ChakraCoreFiles/bin/ch test/Basics/hello.js > /dev/null
IF_ERROR_EXIT $? "CH binary test failed"

## Package
pushd out/ > /dev/null
PACKAGE_FILE="cc_${HOST_OS}_x64_${CHAKRA_VERSION}.tar.gz"
tar -czf "${PACKAGE_FILE}" ChakraCoreFiles/
mkdir -p temp/ChakraCoreFiles/
cp "${PACKAGE_FILE}" temp/chakracore.tar.gz
cd temp
SHASUM_FILE="cc_${HOST_OS}_x64_${CHAKRA_VERSION}_s.tar.gz"
shasum -a 512256 chakracore.tar.gz > ChakraCoreFiles/shasum
tar -czf "$SHASUM_FILE" ChakraCoreFiles
mv $SHASUM_FILE ../
cd ..
rm -rf temp/
popd > /dev/null

## Credits
echo -e "\nPackage & Shasum files are ready under ${GREEN_COLOR}out/${DEFAULT_COLOR}"
