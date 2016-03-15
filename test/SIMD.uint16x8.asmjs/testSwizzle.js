//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var i16 = stdlib.SIMD.Uint16x8;
    var i16check = i16.check;
    var i16swizzle = i16.swizzle;
    var i16add = i16.add;
    var i16mul = i16.mul;

    var globImporti16 = i16check(imports.g1);

    var i16g1 = i16(1, 2, 3, 4, 5, 6, 7, 8);

    var loopCOUNT = 3;

    function testswizzleLocal() {
        var a = i16(1, 2, 3, 4, 5, 6, 7, 8);
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16swizzle(a, 0, 1, 4, 5, 7, 4, 2, 3);
            loopIndex = (loopIndex + 1) | 0;
        }   
        return i16check(result);
    }

    function testswizzleGlobal() {
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16swizzle(i16g1, 0, 1, 4, 5, 7, 4, 2, 3);
            loopIndex = (loopIndex + 1) | 0;
        }
        return i16check(result);
    }

    function testswizzleGlobalImport() {
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16swizzle(globImporti16, 0, 1, 4, 5, 7, 4, 2, 3);
            loopIndex = (loopIndex + 1) | 0;
        }
        return i16check(result);
    }

    function testswizzleFunc() {
        var a = i16(1, 2, 3, 4, 5, 6, 7, 8);
        var result = i16(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = i16swizzle(i16add(a, i16g1), 0, 1, 4, 5, 7, 4, 2, 3);
            loopIndex = (loopIndex + 1) | 0;
        }
        return i16check(result);
    }

    return { testswizzleLocal: testswizzleLocal, testswizzleGlobal: testswizzleGlobal, testswizzleGlobalImport: testswizzleGlobalImport, testswizzleFunc: testswizzleFunc };
}

var m = asmModule(this, { g1: SIMD.Uint16x8(50, 1000, 3092, 3393, 8838, 63838, NaN, 838) });

equalSimd([1, 2, 5, 6, 8, 5, 3, 4], m.testswizzleLocal(), SIMD.Uint16x8, "");
equalSimd([1, 2, 5, 6, 8, 5, 3, 4], m.testswizzleGlobal(), SIMD.Uint16x8, "");
equalSimd([50, 1000, 8838, 63838, 838, 8838, 3092, 3393], m.testswizzleGlobalImport(), SIMD.Uint16x8, "");
equalSimd([2, 4, 10, 12, 16, 10, 6, 8], m.testswizzleFunc(), SIMD.Uint16x8, "");
print("PASS");



