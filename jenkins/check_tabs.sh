#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Need to make sure that the reference to origin/master is available.
# We know that HEAD is checked out so that the tests on that source can be run.

# configure the sh environment to run scripts from the bin dir in case that's missing
ls &> /dev/null # checking for ls script on the path
if [ $? -ne 0 ]; then
    PATH=/bin:/usr/bin:$PATH
fi

ERRFILE=check_tabs.sh.err
rm -f $ERRFILE

# git diff --name-only `git merge-base origin/master HEAD` HEAD | xargs grep -P -l "\t" > /dev/nul

git diff --name-only `git merge-base origin/master HEAD` HEAD |
    xargs grep -P -l "\t" |
    grep -v -E '^pal/' |
    grep -v -E '\Makefile$' |
    grep -v -E '\.sln$' |
    grep -v -E '\.js$' |
    grep -v -E '\.baseline$' |
    grep -v -E '\.wasm$' |
    grep -v -E '\.wast$' |
    grep -v -E '^lib/wabt' |
    grep -v -E 'bin/External/.*$' |
    xargs -I % sh -c 'echo --- IN FILE % ---; git blame HEAD -- % | grep -P "(\t|--- IN FILE)"' > check_tabs.sh.err

if [ -s $ERRFILE ]; then # if file exists and is non-empty then there were errors
    >&2 echo "--------------" # leading >&2 means echo to stderr
    >&2 echo "--- ERRORS ---"
    >&2 echo ""

    cat $ERRFILE 1>&2 # send output to stderr so it can be redirected as error if desired

    exit 1
else
    echo "--- NO PROBLEMS DETECTED ---"
fi
