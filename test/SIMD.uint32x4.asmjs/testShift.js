//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var ui4 = stdlib.SIMD.Uint32x4;
    var ui4check = ui4.check;
    var ui4shiftLeftByScalar = ui4.shiftLeftByScalar;
    var ui4shiftRightByScalar = ui4.shiftRightByScalar;

    var globImportui4 = ui4check(imports.g1);    

    var ui4g1 = ui4(-1165353216, 1073741824, 1077936128, 1082130432);          // global var initialized
    var ui4g2 = ui4(6531634, 74182444, 779364128, 821730432);

    var loopCOUNT = 16;

    function testShiftLeftScalarLocal()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            a = ui4shiftLeftByScalar(a, 1);
        }

        return ui4check(a);
    }
    
    function testShiftRightScalarLocal()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            a = ui4shiftRightByScalar(a, 1);
        }

        return ui4check(a);
    }

    function testShiftLeftScalarGlobal()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            ui4g1 = ui4shiftLeftByScalar(ui4g1, 1);
        }
        return ui4check(ui4g1);
    }

    function testShiftRightScalarGlobal()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            ui4g1 = ui4shiftRightByScalar(ui4g1, 1);
        }
        return ui4check(ui4g1);
    }

    function testShiftLeftScalarGlobalImport()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            globImportui4 = ui4shiftLeftByScalar(globImportui4, 1);
        }
        return ui4check(globImportui4);
    }

    function testShiftRightScalarGlobalImport()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            globImportui4 = ui4shiftRightByScalar(globImportui4, 1);
        }
        return ui4check(globImportui4);
    }

    function testShiftLeftScalarLocal1()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            a = ui4shiftLeftByScalar(a, 32);
            a = ui4shiftLeftByScalar(a, 33);
        }

        return ui4check(a);
    }
    
    function testShiftRightScalarLocal1()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            a = ui4shiftRightByScalar(a, 32);
            a = ui4shiftRightByScalar(a, 33);
        }

        return ui4check(a);
    }

    function testShiftLeftScalarGlobal1()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            ui4g1 = ui4shiftLeftByScalar(ui4g1, 32);
            ui4g1 = ui4shiftLeftByScalar(ui4g1, 33);
        }
        return ui4check(ui4g1);
    }

    function testShiftRightScalarGlobal1()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            ui4g1 = ui4shiftRightByScalar(ui4g1, 32);
            ui4g1 = ui4shiftRightByScalar(ui4g1, 33);
        }
        return ui4check(ui4g1);
    }

    function testShiftLeftScalarGlobalImport1()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            globImportui4 = ui4shiftLeftByScalar(globImportui4, 32);
            globImportui4 = ui4shiftLeftByScalar(globImportui4, 33);
        }
        return ui4check(globImportui4);
    }

    function testShiftRightScalarGlobalImport1()
    {
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {
            globImportui4 = ui4shiftRightByScalar(globImportui4, 32);
            globImportui4 = ui4shiftRightByScalar(globImportui4, 33);
        }
        return ui4check(globImportui4);
    }

    return {
        testShiftLeftScalarLocal: testShiftLeftScalarLocal, testShiftRightScalarLocal: testShiftRightScalarLocal, testShiftLeftScalarGlobal: testShiftLeftScalarGlobal,
        testShiftRightScalarGlobal: testShiftRightScalarGlobal, testShiftLeftScalarGlobalImport: testShiftLeftScalarGlobalImport, testShiftRightScalarGlobalImport: testShiftRightScalarGlobalImport,
        testShiftLeftScalarLocal1: testShiftLeftScalarLocal1, testShiftRightScalarLocal1: testShiftRightScalarLocal1, testShiftLeftScalarGlobal1: testShiftLeftScalarGlobal1,
        testShiftRightScalarGlobal1: testShiftRightScalarGlobal1, testShiftLeftScalarGlobalImport1: testShiftLeftScalarGlobalImport1, testShiftRightScalarGlobalImport1: testShiftRightScalarGlobalImport1
    };
}

var m = asmModule(this, { g1: SIMD.Uint32x4(100, 1073741824, 1028, 102) });

equalSimd([2250506240, 4241489920, 1703084032, 3715104768], m.testShiftLeftScalarLocal(), SIMD.Uint32x4, "testShift1");
equalSimd([129, 73, 457, 144], m.testShiftRightScalarLocal(), SIMD.Uint32x4, "testShift2");
equalSimd([520093696, 0, 0, 0], m.testShiftLeftScalarGlobal(), SIMD.Uint32x4, "testShift3");
equalSimd([7936, 0, 0, 0], m.testShiftRightScalarGlobal(), SIMD.Uint32x4, "testShift4");
equalSimd([6553600, 0, 67371008, 6684672], m.testShiftLeftScalarGlobalImport(), SIMD.Uint32x4, "testShift5");
equalSimd([100, 0, 1028, 102], m.testShiftRightScalarGlobalImport(), SIMD.Uint32x4, "testShift6");

equalSimd([2250506240, 4241489920, 1703084032, 3715104768], m.testShiftLeftScalarLocal1(), SIMD.Uint32x4, "testShift1_1");
equalSimd([129, 73, 457, 144], m.testShiftRightScalarLocal1(), SIMD.Uint32x4, "testShift2_1");
equalSimd([520093696, 0, 0, 0], m.testShiftLeftScalarGlobal1(), SIMD.Uint32x4, "testShift3_1");
equalSimd([7936, 0, 0, 0], m.testShiftRightScalarGlobal1(), SIMD.Uint32x4, "testShift4_1");
equalSimd([6553600, 0, 67371008, 6684672], m.testShiftLeftScalarGlobalImport1(), SIMD.Uint32x4, "testShift5_1");
equalSimd([100, 0, 1028, 102], m.testShiftRightScalarGlobalImport1(), SIMD.Uint32x4, "testShift6_1");

print("PASS");
