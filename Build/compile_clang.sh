#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

LLVM_VERSION="3.9.1"

CC_URL="git://sourceware.org/git/binutils-gdb.git\nhttp://llvm.org/releases/${LLVM_VERSION}/\n"
echo -e "\n----------------------------------------------------------------"
echo -e "\nThis script will download LLVM/CLANG and LLVM Gold Bintools from\n${CC_URL}\n"
echo "These software are licensed to you by its publisher(s), not Microsoft."
echo "Microsoft is not responsible for the software."
echo "Your installation and use of the software is subject to the publisher’s terms available here:"
echo -e "http://llvm.org/docs/DeveloperPolicy.html#license\nhttp://llvm.org/docs/GoldPlugin.html#licensing\n"
echo -e "----------------------------------------------------------------\n"
echo "If you don't agree, press Ctrl+C to terminate"
read -t 10 -p "Hit ENTER to continue (or wait 10 seconds)"

echo -e "\nThis will take some time... [and free memory 2GB+]\n"

ROOT=${PWD}/cc-toolchain/
GOLD_PLUGIN=""

if [ ! -d ./cc-toolchain/src/llvm/projects/compiler-rt ]; then
    rm -rf cc-toolchain
    mkdir cc-toolchain
    cd cc-toolchain
    mkdir src
    mkdir bin
    cd src

    apt-get -v >/dev/null 2>&1
    if [ $? == 0 ]; then # debian
        sudo apt-get install -y apt-file texinfo texi2html csh gawk automake libtool libtool-bin bison flex libncurses5-dev
        if [ $? != 0 ]; then
          exit 1
        fi
    else
        yum -v >/dev/null 2>&1
        if [ $? == 0 ]; then # redhat
            yum install -y texinfo texi2html csh gawk automake libtool libtool-bin bison flex ncurses-devel
        else
            echo "This script requires (texinfo texi2html csh gawk automake libtool libtool-bin bison flex ncurses-devel)"
            echo "Automated installation of these requirements is supported with apt-get and yum only."
            echo ""
            echo "If you don't have these packages are installed, press Ctrl+C to terminate"
            read -t 10 -p "Hit ENTER to continue (or wait 10 seconds)"
        fi
    fi

    mkdir lto_utils
    cd lto_utils
    echo "Downloading LLVM Gold Plugin"
    git clone --depth 1 git://sourceware.org/git/binutils-gdb.git binutils >/dev/null 2>&1
    mkdir binutils_compile; cd binutils_compile
    ../binutils/configure --enable-gold --enable-plugins --disable-werror --prefix="${ROOT}/build"
    make -j2
    make install
    if [ $? != 0 ]; then
        exit 1
    fi

    echo -e "\n\n\n\n"
    cd "${ROOT}/src/"

    echo "Downloading LLVM ${LLVM_VERSION}"
    wget –quiet "http://llvm.org/releases/${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz" >/dev/null 2>&1
    tar -xf "llvm-${LLVM_VERSION}.src.tar.xz"
    if [ $? == 0 ]; then
        rm "llvm-${LLVM_VERSION}.src.tar.xz"
        mv "llvm-${LLVM_VERSION}.src" llvm
    else
        exit 1
    fi

    cd llvm/tools/
    echo "Downloading Clang ${LLVM_VERSION}"
    wget –quiet "http://llvm.org/releases/${LLVM_VERSION}/cfe-${LLVM_VERSION}.src.tar.xz" >/dev/null 2>&1
    tar -xf "cfe-${LLVM_VERSION}.src.tar.xz"
    if [ $? == 0 ]; then
        mv "cfe-${LLVM_VERSION}.src" clang
        rm "cfe-${LLVM_VERSION}.src.tar.xz"
    else
        exit 1
    fi

    mkdir -p ../projects/
    cd ../projects/
    echo "Downloading Compiler-RT ${LLVM_VERSION}"
    wget –quiet "http://llvm.org/releases/${LLVM_VERSION}/compiler-rt-${LLVM_VERSION}.src.tar.xz" >/dev/null 2>&1
    tar -xf "compiler-rt-${LLVM_VERSION}.src.tar.xz"
    if [ $? == 0 ]; then
        mv "compiler-rt-${LLVM_VERSION}.src" compiler-rt
        rm "compiler-rt-${LLVM_VERSION}.src.tar.xz"
    else
        exit 1
    fi
fi

GOLD_PLUGIN=-DLLVM_BINUTILS_INCDIR="${ROOT}/src/lto_utils/binutils/include"

mkdir -p "${ROOT}/build"
cd "${ROOT}/src/llvm"
mkdir -p build_
cd build_

cmake ../ -DCMAKE_INSTALL_PREFIX="${ROOT}/build" -DCMAKE_BUILD_TYPE=Release ${GOLD_PLUGIN}

if [ $? != 0 ]; then
    cd ..
    rm -rf build_
    mkdir build_
    exit 1
fi

make -j4 install

if [ $? == 0 ]; then
    echo -e "Done!\n./build.sh args are given below;\n\n"
    # Create a local bfd-plugins and copy LLVMgold.so into there.
    mkdir -p "${ROOT}/build/lib/bfd-plugins/"
    cp "${ROOT}/build/lib/LLVMgold.so" "${ROOT}/build/lib/bfd-plugins/"
    echo "--cxx=${ROOT}/build/bin/clang++ --cc=${ROOT}/build/bin/clang"
fi
