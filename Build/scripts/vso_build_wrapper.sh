#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

BUILD_COMMANDLINE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
    --debug)
        BUILD_COMMANDLINE="-d"
        ;;
    --test)
        BUILD_COMMANDLINE="-t"
        ;;
    --release)
        BUILD_COMMANDLINE=""
        ;;
    --x64)
        BUILD_ARCH="--x64"
        ;;
    --x86)
        BUILD_ARCH="--x86"
        ;;
    --arm)
        BUILD_ARCH="--arm"
        ;;
    esac

    shift
done

BUILD_COMMANDLINE="${BUILD_COMMANDLINE} -n"

echo "Build command line: $BUILD_COMMANDLINE"
echo "Build arch: $BUILD_ARCH"
echo "Current directory: $PWD"

pushd ../../
./build.sh $BUILD_COMMANDLINE
popd
