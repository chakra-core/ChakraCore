//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var ui4 = stdlib.SIMD.Uint32x4;
    var ui4check = ui4.check;
    var ui4min = ui4.min;
    var ui4max = ui4.max;

    var globImportui4 = ui4check(imports.g1);    

    var ui4g1 = ui4(1065353216, 1073741824, 1077936128, 1082130432);          // global var initialized
    var ui4g2 = ui4(6531634, 74182444, 779364128, 821730432);

    var loopCOUNT = 3;

    function testMinLocal()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var b = ui4(99372621, 18848392, 888288822, 1000010012);
        var result = ui4(0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4min(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }
    
    function testMaxLocal()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var b = ui4(99372621, 18848392, 888288822, 1000010012);
        var result = ui4(0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4max(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }

    function testMinGlobal()
    {
        var result = ui4(0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4min(ui4g1, ui4g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }

    function testMaxGlobal()
    {
        var result = ui4(0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4max(ui4g1, ui4g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }

    function testMinGlobalImport()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var result = ui4(0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4min(globImportui4, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }

    function testMaxGlobalImport()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var result = ui4(0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4max(globImportui4, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }
    
    return { testMinLocal: testMinLocal, testMaxLocal: testMaxLocal, testMinGlobal: testMinGlobal, testMaxGlobal: testMaxGlobal, testMinGlobalImport: testMinGlobalImport, testMaxGlobalImport: testMaxGlobalImport };
}

var m = asmModule(this, { g1: SIMD.Uint32x4(100, 1073741824, 1028, 102) });

equalSimd([8488484, 4848848, 29975939, 9493872], m.testMinLocal(), SIMD.Uint32x4, "testMinLocal");
equalSimd([99372621, 18848392, 888288822, 1000010012], m.testMaxLocal(), SIMD.Uint32x4, "testMaxLocal");
equalSimd([6531634, 74182444, 779364128, 821730432], m.testMinGlobal(), SIMD.Uint32x4, "testMinGlobal");
equalSimd([1065353216, 1073741824, 1077936128, 1082130432], m.testMaxGlobal(), SIMD.Uint32x4, "testMaxGlobal");
equalSimd([100, 4848848, 1028, 102], m.testMinGlobalImport(), SIMD.Uint32x4, "testMinGlobalImport");
equalSimd([8488484, 1073741824, 29975939, 9493872], m.testMaxGlobalImport(), SIMD.Uint32x4, "testMaxGlobalImport");
print("PASS");
