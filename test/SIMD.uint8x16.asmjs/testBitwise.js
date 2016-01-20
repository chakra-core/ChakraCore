//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";
    var ui16 = stdlib.SIMD.Uint8x16;
    var ui16check = ui16.check;

    var ui16and = ui16.and;
    var ui16or = ui16.or;
    var ui16xor = ui16.xor;
    var ui16not = ui16.not;
    var ui16neg = ui16.neg;

    var globImportui16 = ui16check(imports.g1);   

    var ui16g1 = ui16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    var ui16g2 = ui16(256, 255, 128, 127, 0, 0, 1000, 1000, 5, 15, 3, 399, 299, 21, 45, 22);

    var loopCOUNT = 3;

    function testBitwiseOR()
    {
        var a = ui16(255, 128, 256, 127, 28, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var b = ui16(127, 128, 0, 0, 0, 127, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui16or(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }
    
    function testBitwiseAND()
    {
        var a = ui16(255, 255, 255, 255, -128, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var b = ui16(255, 128, 0, 43, 0, 127, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui16and(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }

    function testBitwiseXOR()
    {
        var a = ui16(255, 255, 255, 255, -128, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var b = ui16(255, 128, 0, 43, 0, 127, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui16xor(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }
    
    function testBitwiseNOT() {
        var a = ui16(255, 255, 255, 255, -128, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui16not(a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }
    
        
    function testBitwiseNEG() {
        var a = ui16(255, 255, 255, 255, -128, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui16neg(a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }
    
    return {testBitwiseOR:testBitwiseOR, testBitwiseAND:testBitwiseAND, testBitwiseXOR:testBitwiseXOR, testBitwiseNOT:testBitwiseNOT, testBitwiseNEG:testBitwiseNEG};
}

var m = asmModule(this, { g1: SIMD.Uint8x16(100, -1073741824, -1028, -102, NaN, -38, -92929, Infinity, 52, 127, -127, -129, 129, 0, 88, 100234) });

equalSimd([255, 128, 0, 127, 28, 127, 15, 9, 9, 15, 15, 13, 13, 15, 15, 17], m.testBitwiseOR(), SIMD.Uint8x16, "testBitwiseOR");
equalSimd([255, 128, 0, 43, 0, 0, 2, 8, 8, 2, 2, 4, 4, 2, 2, 0], m.testBitwiseAND(), SIMD.Uint8x16, "testBitwiseAND");
equalSimd([0, 127, 255, 212, 128, 127, 13, 1, 1, 13, 13, 9, 9, 13, 13, 17], m.testBitwiseXOR(), SIMD.Uint8x16, "testBitwiseXOR");
equalSimd([0, 0, 0, 0, 127, 255, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239], m.testBitwiseNOT(), SIMD.Uint8x16, "testBitwiseNOT");
equalSimd([1, 1, 1, 1, 128, 0, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240], m.testBitwiseNEG(), SIMD.Uint8x16, "testBitwiseNEG");

print("PASS");
