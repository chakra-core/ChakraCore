//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var ui16 = stdlib.SIMD.Uint8x16;
    var ui16check = ui16.check;
    var ui16mul = ui16.mul;

    var globImportui16 = ui16check(imports.g1);

    var ui16g1 = ui16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    var ui16g2 = ui16(10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10);

    var loopCOUNT = 3;

    function testMulLocal()
    {
        var a = ui16(5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80);
        var b = ui16(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16mul(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }
    function testMulGlobal()
    {    
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16mul(ui16g1, ui16g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }

    function testMulGlobalImport()
    {
        var a = ui16(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2);
        var result = ui16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;

        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui16mul(globImportui16, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui16check(result);
    }
    
    return {testMulLocal:testMulLocal, testMulGlobal:testMulGlobal, testMulGlobalImport:testMulGlobalImport};
}

var m = asmModule(this, { g1: SIMD.Uint8x16(5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80) });

equalSimd([10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160], m.testMulLocal(), SIMD.Uint8x16, "Test Neg");
equalSimd([10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160], m.testMulGlobal(), SIMD.Uint8x16, "Test Neg");
equalSimd([10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160], m.testMulGlobalImport(), SIMD.Uint8x16, "Test Neg");
print("PASS");