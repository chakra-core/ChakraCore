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

ERRFILE=jenkins.check_copyright.sh.err
ERRFILETEMP=$ERRFILE.0
rm -f $ERRFILE
rm -f $ERRFILETEMP

echo "Check Copyright > Begin Checking..."
git diff --name-only `git merge-base origin/master HEAD` HEAD |
    grep -v -E '\.git.*' |
    grep -v -E '\.xml$' |
    grep -v -E '\.props$' |
    grep -v -E '\.md$' |
    grep -v -E '\.txt$' |
    grep -v -E '\.baseline$' |
    grep -v -E '\.sln$' |
    grep -v -E '\.vcxproj$' |
    grep -v -E '\.filters$' |
    grep -v -E '\.targets$' |
    grep -v -E '\.def$' |
    grep -v -E '\.inc$' |
    grep -v -E 'test/benchmarks/.*\.js$' |
    grep -v -E 'test/SIMD.bool16x8.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.bool16x8.asmjs/testTrue.js$' |
    grep -v -E 'test/SIMD.bool32x4.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.bool32x4.asmjs/testTrue.js$' |
    grep -v -E 'test/SIMD.bool8x16.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.bool8x16.asmjs/testTrue.js$' |
    grep -v -E 'test/SIMD.float32x4.asmjs/testMisc.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testAddSub.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testAddSubSaturate.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testCalls.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testComparison.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testComparisonSelect.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testConstructorLanes.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testConversion-2.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testConversion.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testLoadStore.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testMinMax.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testMisc.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testMisc2.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testMul.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testNeg.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testShift.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testShuffle.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testSplat.js$' |
    grep -v -E 'test/SIMD.int16x8.asmjs/testSwizzle.js$' |
    grep -v -E 'test/SIMD.int32x4.asmjs/testBool.js$' |
    grep -v -E 'test/SIMD.int32x4.asmjs/testBoolCalls.js$' |
    grep -v -E 'test/SIMD.int32x4.asmjs/testBools.js$' |
    grep -v -E 'test/SIMD.int32x4.asmjs/testMisc.js$' |
    grep -v -E 'test/SIMD.int32x4/testSplat.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testAddSub.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testAddSubSaturate.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testCalls.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testComparison.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testComparisonSelect.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testConstructorLanes.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testConversion-2.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testConversion.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testLoadStore.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testMinMax.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testMisc.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testMul.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testNeg.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testShift.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testShuffle.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testSplat.js$' |
    grep -v -E 'test/SIMD.int8x16.asmjs/testSwizzle.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testAddSub.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testAddSubSaturate.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testCalls.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testComparison.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testComparisonSelect.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testConstructorLanes.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testConversion-2.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testConversion.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testLoadStore.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testMinMax.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testMisc.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testMul.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testShift.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testShuffle.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testSplat.js$' |
    grep -v -E 'test/SIMD.uint16x8.asmjs/testSwizzle.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testAddSub.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testCalls.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testComparison.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testComparisonSelect.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testConstructorLanes.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testConversion-2.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testConversion.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testLoadStore.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testMinMax.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testMisc.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testMul.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testShift.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testShuffle.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testSplat.js$' |
    grep -v -E 'test/SIMD.uint32x4.asmjs/testSwizzle.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testAddSub.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testAddSubSaturate.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testBitwise.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testCalls.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testComparison.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testComparisonSelect.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testConstructorLanes.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testConversion-2.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testConversion.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testLoadStore.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testMinMax.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testMisc.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testMul.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testShift.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testShuffle.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testSplat.js$' |
    grep -v -E 'test/SIMD.uint8x16.asmjs/testSwizzle.js$' |
	xargs -I % sh -c "echo 'Check Copyright > Checking %'; python jenkins.check_copyright.py % > $ERRFILETEMP; if [ \$? -ne 0 ]; then cat $ERRFILETEMP >> $ERRFILE; fi"

if [ -e $ERRFILE ]; then # if error file exists then there were errors
    >&2 echo "--------------" # leading >&2 means echo to stderr
    >&2 echo "--- ERRORS ---"
    cat $ERRFILE 1>&2 # send output to stderr so it can be redirected as error if desired
    >&2 echo "--------------"
    exit 1 # tell the caller there was an error (so Jenkins will fail the CI task)
else
    echo "--- NO PROBLEMS DETECTED ---"
fi
