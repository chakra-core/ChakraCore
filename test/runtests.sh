#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------
#
# Eventual test running harness on *nix for ChakraCore
# Today, it's simply there to make sure hello.js doesn't regress
#

test_path=`dirname "$0"`
ch_path="$test_path/../BuildLinux/ch"
hello_path="$test_path/Basics/hello.js"

if [ ! -f $ch_path ]; then
    echo 'ch not found- exiting'
    # TODO change this to exit 1 once clang requirement on build machines is satisfied
    exit 0
fi

output=`$ch_path $hello_path 2>&1 | tail -n 1`

if [ ! $output == "PASS" ]; then
    echo "Hello world failed"
    exit 1
fi

echo "Hello world passed"
exit 0
