#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------
#
# todo-CI: REMOVE THIS AFTER ENABLING runtests.py directly on CI

test_path=`dirname "$0"`

build_arch="x64"
build_type="debug"

if [[ $# -gt 0 ]]; then
    build_arch=$1
fi

if [[ $# -gt 1 ]]; then
    build_type=$2
fi

if [[ $build_type == "debug" ]]; then
    test_flags="-d"
elif [[ $build_type == "test" ]]; then
    test_flags="-t"
elif [[ $build_type == "release" ]]; then
    # however we would like to test if the compiled binary
    # works or not
    CH="$test_path/../Build/clang_build/${build_arch}_${build_type}/ch"
    echo "Warning: Release build was found"
    RES=$(${CH} $test_path/test/basics/hello.js)
    if [[ $RES =~ "Error :" ]]; then
        echo "FAILED"
        exit 1
    else
        echo "PASS"
        exit 0
    fi
fi

echo "Invoking runtests.py with parameter $test_flags ($build_arch $build_type)"

"$test_path/runtests.py" $test_flags --not-tag exclude_jenkins
if [[ $? != 0 ]]; then
    exit 1
fi
