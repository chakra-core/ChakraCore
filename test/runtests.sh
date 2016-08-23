#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------
#
# todo-CI: REMOVE THIS AFTER ENABLING runtests.py directly on CI

test_path=`dirname "$0"`

build_type=$1
binary_path=
release_build=0

if [[ -f "$test_path/../BuildLinux/Debug/ch" ]]; then
    echo "Warning: Debug build was found"
    binary_path="Debug";
    build_type="-d"
elif [[ -f "$test_path/../BuildLinux/Test/ch" ]]; then
    echo "Warning: Test build was found"
    binary_path="Test";
    build_type="-t"
elif [[ -f "$test_path/../BuildLinux/Release/ch" ]]; then
    binary_path="Release";
    echo "Warning: Release build was found"
    release_build=1
else
    echo 'Error: ch not found- exiting'
    exit 1
fi

if [[ $release_build != 1 ]]; then
    "$test_path/runtests.py" $build_type --not-tag exclude_jenkins
    if [[ $? != 0 ]]; then
        exit 1
    fi
else
    # TEST flags are not enabled for release build
    # however we would like to test if the compiled binary
    # works or not
    RES=$($test_path/../BuildLinux/${binary_path}/ch $test_path/test/basics/hello.js)
    if [[ $RES =~ "Error :" ]]; then
        echo "FAILED"
        exit 1
    else
        echo "Release Build Passes hello.js run"
    fi
fi

RES=$(pwd)
CH_ABSOLUTE_PATH="$RES/${test_path}/../BuildLinux/${binary_path}/ch"
RES=$(cd $RES/$test_path/native-tests; ./test_native.sh ${CH_ABSOLUTE_PATH} 2>&1)
if [[ $? != 0 ]]; then
    echo "Error: Native tests failed"
    echo -e "$RES"
    exit 1
fi
