//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var ui16 = stdlib.SIMD.Uint8x16;
    var ui16check = ui16.check;
    var ui16min = ui16.min;
    var ui16max = ui16.max;

    var globImportui16 = ui16check(imports.g1);    

    var ui16g1 = ui16(0, 255, 7, 120, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    var ui16g2 = ui16(256, 255, 128, 127, 0, 0, 1000, 1000, 5, 15, 3, 399, 299, 21, 45, 22);

    var loopCOUNT = 3;
    
    var i16 = stdlib.SIMD.Int8x16;
    var i16check = i16.check;
    var i16fu16 = i16.fromUint8x16Bits;

    function testMinLocal()
    {
        var a = ui16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var b = ui16(127, 128, 0, 0, 0, 127, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui16min(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(i16fu16(result));
    }
    
    function testMaxLocal()
    {
        var a = ui16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var b = ui16(127, 128, 0, 0, 0, 127, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16max(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(i16fu16(result));
    }

    function testMinGlobal()
    {
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16min(ui16g1, ui16g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(i16fu16(result));
    }

    function testMaxGlobal()
    {
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16max(ui16g1, ui16g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(i16fu16(result));
    }

    function testMinGlobalImport()
    {
        var a = ui16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16min(globImportui16, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(i16fu16(result));
    }

    function testMaxGlobalImport()
    {
        var a = ui16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16max(globImportui16, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i16check(i16fu16(result));
    }
    
    return { testMinLocal: testMinLocal, testMaxLocal: testMaxLocal, testMinGlobal: testMinGlobal, testMaxGlobal: testMaxGlobal, testMinGlobalImport: testMinGlobalImport, testMaxGlobalImport: testMaxGlobalImport };
}

var m = asmModule(this, { g1: SIMD.Uint8x16(100, 1073741824, 1028, 102, 124, 55, -929, 100, 52, 127, 127, -129, 129, 0, 88, 100234) });

var ret1 = SIMD.Uint8x16.fromInt8x16Bits(m.testMinLocal());
var ret2 = SIMD.Uint8x16.fromInt8x16Bits(m.testMaxLocal());
var ret3 = SIMD.Uint8x16.fromInt8x16Bits(m.testMinGlobal());
var ret4 = SIMD.Uint8x16.fromInt8x16Bits(m.testMaxGlobal());
var ret5 = SIMD.Uint8x16.fromInt8x16Bits(m.testMinGlobalImport());
var ret6 = SIMD.Uint8x16.fromInt8x16Bits(m.testMaxGlobalImport());

equalSimd([1, 2, 0, 0, 0, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1], ret1, SIMD.Uint8x16, "testMinLocal");
equalSimd([127, 128, 3, 4, 5, 127, 10, 9, 9, 10, 11, 12, 13, 14, 15, 16], ret2, SIMD.Uint8x16, "testMaxLocal");
equalSimd([0, 255, 7, 120, 0, 0, 7, 8, 5, 10, 3, 12, 13, 14, 15, 16], ret3, SIMD.Uint8x16, "testMinGlobal");
equalSimd([0, 255, 128, 127, 5, 6, 232, 232, 9, 15, 11, 143, 43, 21, 45, 22], ret4, SIMD.Uint8x16, "testMaxGlobal");
equalSimd([1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 15, 16], ret5, SIMD.Uint8x16, "testMinGlobalImport");
equalSimd([100, 2, 4, 102, 124, 55, 95, 100, 52, 127, 127, 127, 129, 14, 88, 138], ret6, SIMD.Uint8x16, "testMaxGlobalImport");
print("PASS");
