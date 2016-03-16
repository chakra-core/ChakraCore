//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";
    var ui4 = stdlib.SIMD.Uint32x4;
    var ui4check = ui4.check;
    var ui4add = ui4.add;
    var ui4sub = ui4.sub;
 
    var globImportui4 = ui4check(imports.g1);     

    var ui4g1 = ui4(1065353216, 1073741824,1077936128, 1082130432);          // global var initialized
    var ui4g2 = ui4(6531634, 74182444, 779364128, 821730432);

    var loopCOUNT = 3;

    function testAddLocal()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var b = ui4(99372621, 18848392, 888288822, 1000010012);
        var result = ui4(0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4add(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }
    
    function testSubLocal()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var b = ui4(99372621, 18848392, 888288822, 1000010012);
        var result = ui4(0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4sub(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }

    function testAddGlobal()
    {
        var result = ui4(0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4add(ui4g1, ui4g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }
    
    function testSubGlobal()
    {
        var result = ui4(0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4sub(ui4g1, ui4g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }
    
    function testAddGlobalImport()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var result = ui4(0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4add(globImportui4, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }
    
    function testSubGlobalImport()
    {
        var a = ui4(8488484, 4848848, 29975939, 9493872);
        var result = ui4(0, 0, 0, 0);
        var loopIndex = 0;

        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui4sub(globImportui4, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui4check(result);
    }
    
    return { testAddLocal: testAddLocal, testSubLocal: testSubLocal, testAddGlobal: testAddGlobal, testSubGlobal: testSubGlobal, testAddGlobalImport: testAddGlobalImport, testSubGlobalImport: testSubGlobalImport };
}

var m = asmModule(this, {g1:SIMD.Uint32x4(100, 1073741824, 1028, 102)});

equalSimd([107861105, 23697240, 918264761, 1009503884], m.testAddLocal(), SIMD.Uint32x4, "Func1");
equalSimd([4204083159, 4280967752, 3436654413, 3304451156], m.testSubLocal(), SIMD.Uint32x4, "Func2");
equalSimd([1071884850, 1147924268, 1857300256, 1903860864], m.testAddGlobal(), SIMD.Uint32x4, "Func3");
equalSimd([1058821582, 999559380, 298572000, 260400000], m.testSubGlobal(), SIMD.Uint32x4, "Func4");
equalSimd([8488584, 1078590672, 29976967, 9493974], m.testAddGlobalImport(), SIMD.Uint32x4, "Func5");
equalSimd([4286478912, 1068892976, 4264992385, 4285473526], m.testSubGlobalImport(), SIMD.Uint32x4, "Func6");
print("PASS");
