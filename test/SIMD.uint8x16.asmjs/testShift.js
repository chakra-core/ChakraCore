//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var ui16 = stdlib.SIMD.Uint8x16;
    var ui16check = ui16.check;
    var ui16shiftLeftByScalar = ui16.shiftLeftByScalar;
    var ui16shiftRightByScalar = ui16.shiftRightByScalar;

    var globImportui16 = ui16check(imports.g1);    

    var ui16g1 = ui16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    var ui16g2 = ui16(256, 255, 128, 127, 0, 0, 1000, 1000, 5, 15, 3, 399, 299, 21, 45, 22);

    var loopCOUNT = 4;

    function testShiftLeftScalarLocal()
    {
        var a = ui16(40, 40, 80, 80, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            a = ui16shiftLeftByScalar(a, 1);
        }

        return ui16check(a);
    }
    
    function testShiftRightScalarLocal()
    {
        var a = ui16(40, 40, 80, 80, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            a = ui16shiftRightByScalar(a, 1);
        }

        return ui16check(a);
    }

    function testShiftLeftScalarGlobal()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            ui16g1 = ui16shiftLeftByScalar(ui16g1, 1);
        }
        return ui16check(ui16g1);
    }

    function testShiftRightScalarGlobal()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            ui16g1 = ui16shiftRightByScalar(ui16g1, 1);
        }
        return ui16check(ui16g1);
    }

    function testShiftLeftScalarGlobalImport()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            globImportui16 = ui16shiftLeftByScalar(globImportui16, 1);
        }
        return ui16check(globImportui16);
    }

    function testShiftRightScalarGlobalImport()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            globImportui16 = ui16shiftRightByScalar(globImportui16, 1);
        }
        return ui16check(globImportui16);
    }
    
    return {
        testShiftLeftScalarLocal: testShiftLeftScalarLocal, testShiftRightScalarLocal: testShiftRightScalarLocal, testShiftLeftScalarGlobal: testShiftLeftScalarGlobal,
        testShiftRightScalarGlobal: testShiftRightScalarGlobal, testShiftLeftScalarGlobalImport: testShiftLeftScalarGlobalImport, testShiftRightScalarGlobalImport: testShiftRightScalarGlobalImport
    };
}

var m = asmModule(this, { g1: SIMD.Uint8x16(100, 10824, 1028, 82, 3883, 8, 2929, 1442, 52, 127, 128, 129, 129, 0, 88, 100234) });

equalSimd([128, 128, 0, 0, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 0], m.testShiftLeftScalarLocal(), SIMD.Uint8x16, "func1");
equalSimd([2, 2, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], m.testShiftRightScalarLocal(), SIMD.Uint8x16, "func2");
equalSimd([16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 0], m.testShiftLeftScalarGlobal(), SIMD.Uint8x16, "func3");
equalSimd([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0], m.testShiftRightScalarGlobal(), SIMD.Uint8x16, "func4");
equalSimd([64, 128, 64, 32, 176, 128, 16, 32, 64, 240, 0, 16, 16, 0, 128, 160], m.testShiftLeftScalarGlobalImport(), SIMD.Uint8x16, "func5");
equalSimd([4, 8, 4, 2, 11, 8, 1, 2, 4, 15, 0, 1, 1, 0, 8, 10], m.testShiftRightScalarGlobalImport(), SIMD.Uint8x16, "func6");
print("PASS");
