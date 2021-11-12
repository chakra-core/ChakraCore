#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Need to make sure that the reference to origin/master is available.
# We know that HEAD is checked out so that the tests on that source can be run.

# configure the sh environment to run scripts from the bin dir in case that's missing
ls &> /dev/null # checking for ls script on the path
if [ $? -ne 0 ]; then
    PATH=/bin:/usr/bin:$PATH
fi

ERRFILE=check_eol.sh.err
rm -f $ERRFILE

git diff --name-only `git merge-base origin/master HEAD` HEAD | grep -v -E "(test/.*\\.js|\\.cmd|\\.baseline|\\.wasm|\\.wast|\\.vcxproj|\\.vcproj|\\.sln|\\.man|\\.png)" | xargs -I % ./tools/StyleChecks/check_file_eol.sh %

if [ -e $ERRFILE ]; then # if error file exists then there were errors
    >&2 echo "--------------" # leading >&2 means echo to stderr
    >&2 echo "--- ERRORS ---"
    cat $ERRFILE 1>&2 # send output to stderr so it can be redirected as error if desired
    exit 1 # tell the caller there was an error (so the CI task will fail)
else
    echo "--- NO PROBLEMS DETECTED ---"
fi
