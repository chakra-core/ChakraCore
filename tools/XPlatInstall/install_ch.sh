#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

DEFAULT_COLOR='\033[0m'
ERROR_COLOR='\033[0;31m'
SUCCESS_COLOR='\033[0;32m'
CURRENT_OS=
CHAKRA_VERSION=0
BINARY_NAME=

PRINT()
{
    TEXT_COLORED="$1$2"
    echo -e "${TEXT_COLORED} ${DEFAULT_COLOR}"
}

GET_ARCH()
{
    UNAME_OUTPUT=$(uname -mrsn)
    if [[ "$UNAME_OUTPUT" =~ 'arm' ]]; then
        echo "arm"
    elif [[ "$UNAME_OUTPUT" =~ '64' ]]; then
        echo "x64"
    else
        echo "x86"
    fi
}

GET_OS()
{
    UNAME_OUTPUT=$(uname -mrsn)
    ARCH=$(GET_ARCH)

    if [[ "${ARCH}" =~ "x64" ]]; then
        if [[ "${UNAME_OUTPUT}" =~ 'Darwin' ]]; then
            CURRENT_OS="macOS"
            BINARY_NAME="cc_osx_${ARCH}"
            return
        fi

        if [[ "${UNAME_OUTPUT}" =~ 'Linux' ]]; then
            CURRENT_OS="Linux"
            BINARY_NAME="cc_linux_${ARCH}"
            return
        fi
    fi

    PRINT $ERROR_COLOR "] This OS / ARCH is not yet supported - ${UNAME_OUTPUT}"
    exit
}

GET_LATEST()
{
    GET_OS

    PRINT $DEFAULT_COLOR "] Fetching the latest ChakraCore version"
    CHAKRA_VERSION=$(curl -k -s -SL "https://aka.ms/chakracore/version")

    if [[ "$CHAKRA_VERSION" == *"Error"* ]]; then
        PRINT $ERROR_COLOR "] Please ensure you are connected to the internet and try again."
        exit 1
    fi

    PRINT $SUCCESS_COLOR "] Found ChakraCore ${CHAKRA_VERSION//_/.} for ${CURRENT_OS}"
    BINARY_NAME="https://aka.ms/chakracore/${BINARY_NAME}_${CHAKRA_VERSION}"
    SHASUM_NAME="${BINARY_NAME}_s"
}

PRINT $DEFAULT_COLOR "ChakraCore Installation Script 0.1b\n"
PRINT $DEFAULT_COLOR "Visit https://aka.ms/WhatIs/ChakraCore for more information"

GET_LATEST

PRINT $DEFAULT_COLOR "----------------------------------------------------------------"
PRINT $SUCCESS_COLOR   "\nThis script will download & execute ChakraCore binary"
PRINT $DEFAULT_COLOR "located at ${BINARY_NAME}\n"
PRINT $DEFAULT_COLOR "----------------------------------------------------------------"
PRINT $DEFAULT_COLOR "If you don't agree, press Ctrl+C to terminate"
read -t 20 -p "Hit ENTER to continue (or wait 20 seconds)"

if [ -d "./ChakraCoreFiles" ]; then
    PRINT $ERROR_COLOR "] Found 'ChakraCoreFiles' folder on the current path."
    PRINT $DEFAULT_COLOR "] You should delete that folder or try this script on another path"
    exit 1
fi

CHECK_DOWNLOAD_FAIL()
{
    if [[ $? != 0 ]]; then
        PRINT $ERROR_COLOR "] Download failed."
        PRINT $DEFAULT_COLOR "] ${___}"
        exit 1
    fi
}

PRINT $DEFAULT_COLOR "\n] Downloading ChakraCore"
PRINT $SUCCESS_COLOR "] ${BINARY_NAME}"
___=$(curl -kSL -o "chakracore.tar.gz" "${BINARY_NAME}" 2>&1)
CHECK_DOWNLOAD_FAIL

PRINT $DEFAULT_COLOR "\n] Downloading ChakraCore shasum"
PRINT $SUCCESS_COLOR "] ${SHASUM_NAME}"
___=$(curl -kSL -o "chakracore_s.tar.gz" "${SHASUM_NAME}" 2>&1)
CHECK_DOWNLOAD_FAIL

PRINT $SUCCESS_COLOR "] Download completed"

CHECK_EXT_FAIL()
{
    if [[ $? != 0 ]]; then
        PRINT $ERROR_COLOR "] Extracting the compressed file failed."
        PRINT $DEFAULT_COLOR "] ${___}"
        rm -rf ChakraCoreFiles/
        rm -rf chakracore.tar.gz
        rm -rf chakracore_s.tar.gz
        exit 1
    fi
}

___=$(tar -xzf chakracore_s.tar.gz 2>&1)
CHECK_EXT_FAIL

SUM1=`shasum -a 512256 chakracore.tar.gz`
SUM2=`cat ChakraCoreFiles/shasum`
SUM1="${SUM1}@"
SUM2="${SUM2}@"

if [[ ! $SUM1 =~ $SUM2 ]]; then
    PRINT $ERROR_COLOR "] Corrupted binary package."
    PRINT $DEFAULT_COLOR "] Check your network connection."
    PRINT $ERROR_COLOR "] If you suspect there is some other problem,\
 https://github.com/Microsoft/ChakraCore#contact-us"
    rm -rf chakracore.tar.gz
    rm -rf chakracore_s.tar.gz
    rm -rf ChakraCoreFiles/
    exit 1
fi

rm -rf chakracore_s.tar.gz

PRINT $SUCCESS_COLOR "] ChakraCore package SHASUM matches"

___=$(tar -xzf chakracore.tar.gz 2>&1)
CHECK_EXT_FAIL

rm -rf chakracore.tar.gz

# test ch
___=$(./ChakraCoreFiles/bin/ch -v 2>&1)

if [[ $? != 0 ]]; then
    if [[ $___ =~ "GLIBC_2.17" ]]; then
        PRINT $ERROR_COLOR   "] This ChakraCore binary is compiled for newer systems."
        PRINT $DEFAULT_COLOR "] You may compile ChakraCore on your current system."
        PRINT $DEFAULT_COLOR "  Once you do that, it should work on your system.\n"
        PRINT $DEFAULT_COLOR "] You may also try running the pre-compiled binary on a newer system"
    else
        PRINT $ERROR_COLOR   "] Something went wrong. 'ch' installation is failed."
        PRINT $DEFAULT_COLOR "] ${___}"
        PRINT $DEFAULT_COLOR "] Please send us this error from https://github.com/Microsoft/ChakraCore"
    fi
    rm -rf ChakraCoreFiles/
    exit 1
fi

PRINT $SUCCESS_COLOR "] Success\n"

if [[ $___ =~ "-beta" ]]; then
    echo -e "Downloaded a ${ERROR_COLOR}release candidate${DEFAULT_COLOR} binary, $____"
    PRINT $DEFAULT_COLOR "] If you observe issue(s), please send us from https://github.com/Microsoft/ChakraCore"
fi

PRINT $DEFAULT_COLOR "] Try './ChakraCoreFiles/bin/ch -?'"
