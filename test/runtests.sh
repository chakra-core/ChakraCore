#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------
#
# todo-CI: REMOVE THIS AFTER ENABLING runtests.py on CI

test_path=`dirname "$0"`
ch_path="$test_path/../BuildLinux/ch"

if [ ! -f $ch_path ]; then
    echo 'ch not found- exiting'
    exit 1
fi

"$test_path/runtests.py" Basics/hello.js
