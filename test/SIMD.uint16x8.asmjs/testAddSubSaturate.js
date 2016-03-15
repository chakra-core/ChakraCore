//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";
    var ui8 = stdlib.SIMD.Uint16x8;
    var ui8check = ui8.check;
    var ui8addSaturate = ui8.addSaturate;
    var ui8subSaturate = ui8.subSaturate;
 
    var globImportui8 = ui8check(imports.g1);     

    var ui8g1 = ui8(1065353216, 1073741824, 1077936128, 1082130432, 1040103192, 1234123832, 0807627311, 0659275670);      
    var ui8g2 = ui8(353216, 492529, 1128, 1085, 3692, 3937, 9755, 2638);     

    var loopCOUNT = 3;

    function testAddSaturateLocal()
    {
        var a = ui8(50000, 65535, 0, 65535, 1, 1, 1, 1);
        var b = ui8(50000, 32766, 65535, 1, 1, 1, 1, 1);
        var result = ui8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui8addSaturate(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui8check(result);
    }
    
    function testSubSaturateLocal()
    {
        var a = ui8(100, 3401, 665, 3234, 948, 2834, 7748, 25);
        var b = ui8(32767, 2144, 5697, 65, 60000, 984, 3434, 9876);
        var result = ui8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui8subSaturate(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui8check(result);
    }

    function testAddSaturateGlobal()
    {
        var result = ui8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui8addSaturate(ui8g1, ui8g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui8check(result);
    }
    
    function testSubSaturateGlobal()
    {
        var result = ui8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui8subSaturate(ui8g1, ui8g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui8check(result);
    }
    
    function testAddSaturateGlobalImport()
    {
        var a = ui8(50330, 3401, 665, 3234, 948, 2834, 7748, 25);
        var result = ui8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = ui8addSaturate(globImportui8, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui8check(result);
    }
    
    function testSubSaturateGlobalImport()
    {
        var a = ui8(50330, 3401, 665, 3234, 948, 2834, 7748, 25);
        var result = ui8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ((loopIndex | 0) < (loopCOUNT | 0)) {
            result = ui8subSaturate(globImportui8, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return ui8check(result);
    }
    
    return { testAddSaturateLocal: testAddSaturateLocal, testSubSaturateLocal: testSubSaturateLocal, testAddSaturateGlobal: testAddSaturateGlobal, testSubSaturateGlobal: testSubSaturateGlobal, testAddSaturateGlobalImport: testAddSaturateGlobalImport, testSubSaturateGlobalImport: testSubSaturateGlobalImport };
}

var m = asmModule(this, {g1:SIMD.Uint16x8(20000, 1073741824, 1028, 102, 3883, 38, 92929, 1442)});

equalSimd([65535, 65535, 65535, 65535, 2, 2, 2, 2], m.testAddSaturateLocal(), SIMD.Uint16x8, "Func1");
equalSimd([0, 1257, 0, 3169, 0, 1850, 4314, 0], m.testSubSaturateLocal(), SIMD.Uint16x8, "Func2");
equalSimd([25536, 33777, 1128, 1085, 50564, 19353, 36938, 51684], m.testAddSaturateGlobal(), SIMD.Uint16x8, "Func3");
equalSimd([0, 0, 0, 0, 43180, 11479, 17428, 46408], m.testSubSaturateGlobal(), SIMD.Uint16x8, "Func4");
equalSimd([65535, 3401, 1693, 3336, 4831, 2872, 35141, 1467], m.testAddSaturateGlobalImport(), SIMD.Uint16x8, "Func5");
equalSimd([0, 0, 363, 0, 2935, 0, 19645, 1417], m.testSubSaturateGlobalImport(), SIMD.Uint16x8, "Func6");
print("PASS");
