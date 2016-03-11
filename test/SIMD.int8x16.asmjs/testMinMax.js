//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var i16 = stdlib.SIMD.Int8x16;
    var i16check = i16.check;
    var i16min = i16.min;
    var i16max = i16.max;

    var globImporti16 = i16check(imports.g1);    

    var i16g1 = i16(0, 0, 0, 0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    var i16g2 = i16(128, 128, -128, -128, 0, 0, 1000, -1000, 5, 15, -3, -399, 299, 21, 0, 0);

    var loopCOUNT = 3;

    function testMinLocal()
    {
        var a = i16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var b = i16(-16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1);
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = i16min(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(result);
    }
    
    function testMaxLocal()
    {
        var a = i16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var b = i16(-16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1);
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16max(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(result);
    }

    function testMinGlobal()
    {
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16min(i16g1, i16g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(result);
    }

    function testMaxGlobal()
    {
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16max(i16g1, i16g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(result);
    }

    function testMinGlobalImport()
    {
        var a = i16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16min(globImporti16, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(result);
    }

    function testMaxGlobalImport()
    {
        var a = i16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16max(globImporti16, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(result);
    }
    
    return { testMinLocal: testMinLocal, testMaxLocal: testMaxLocal, testMinGlobal: testMinGlobal, testMaxGlobal: testMaxGlobal, testMinGlobalImport: testMinGlobalImport, testMaxGlobalImport: testMaxGlobalImport };
}

var m = asmModule(this, { g1: SIMD.Int8x16(100, -1073741824, -1028, -102, NaN, -38, -92929, Infinity, 52, 127, -127, -129, 129, 0, 88, 100234) });

equalSimd([-16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1], m.testMinLocal(), SIMD.Int8x16, "testMinLocal");
equalSimd([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], m.testMaxLocal(), SIMD.Int8x16, "testMaxLocal");
equalSimd([-128, -128, -128, -128, 0, 0, -24, 8, 5, 10, -3, 12, 13, 14, 0, 0], m.testMinGlobal(), SIMD.Int8x16, "testMinGlobal");
equalSimd([0, 0, 0, 0, 5, 6, 7, 24, 9, 15, 11, 113, 43, 21, 15, 16], m.testMaxGlobal(), SIMD.Int8x16, "testMaxGlobal");
equalSimd([1, 0, -4, -102, 0, -38, -1, 0, 9, 10, -127, 12, -127, 0, 15, -118], m.testMinGlobalImport(), SIMD.Int8x16, "testMinGlobalImport");
equalSimd([100, 2, 3, 4, 5, 6, 7, 8, 52, 127, 11, 127, 13, 14, 88, 16], m.testMaxGlobalImport(), SIMD.Int8x16, "testMaxGlobalImport");
print("PASS");
