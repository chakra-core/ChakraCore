//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports) {
    "use asm";

    var i8 = stdlib.SIMD.Int16x8;
    var i8check = i8.check;
    var i8min = i8.min;
    var i8max = i8.max;

    var globImporti8 = i8check(imports.g1);    

    var i8g1 = i8(106, 0, -10, 1082, 3192, 32, 19, 5);         
    var i8g2 = i8(353, -4929, -1128, 0, 3692, 3937, 9755, 2638);          

    var loopCOUNT = 3;

    function testMinLocal()
    {
        var a = i8(5000, -3401, 665, -3224, -948, 2834, 7748, -25);
        var b = i8(4999, 3401, 0, 1000, -1000, 9299, 0, 0);
        var result = i8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = i8min(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i8check(result);
    }
    
    function testMaxLocal()
    {
        var a = i8(5000, -3401, 665, -3224, -948, 2834, 7748, -25);
        var b = i8(4999, 3401, 0, 1000, -1000, 9299, 0, 0);
        var result = i8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = i8max(a, b);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i8check(result);
    }

    function testMinGlobal()
    {
        var result = i8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = i8min(i8g1, i8g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i8check(result);
    }

    function testMaxGlobal()
    {
        var result = i8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = i8max(i8g1, i8g2);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i8check(result);
    }

    function testMinGlobalImport()
    {
        var a = i8(5033, -3401, 665, -3224, -948, 2834, 7748, -25);
        var result = i8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = i8min(globImporti8, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i8check(result);
    }

    function testMaxGlobalImport()
    {
        var a = i8(5033, -3401, 665, -3224, -948, 2834, 7748, -25);
        var result = i8(0, 0, 0, 0, 0, 0, 0, 0);

        var loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            result = i8max(globImporti8, a);
            loopIndex = (loopIndex + 1) | 0;
        }

        return i8check(result);
    }
    
    return { testMinLocal: testMinLocal, testMaxLocal: testMaxLocal, testMinGlobal: testMinGlobal, testMaxGlobal: testMaxGlobal, testMinGlobalImport: testMinGlobalImport, testMaxGlobalImport: testMaxGlobalImport };
}

var m = asmModule(this, {g1:SIMD.Int16x8(-50, 1000, 3092, -3393, 0, -39283838, 0, -838)});

equalSimd([4999, -3401, 0, -3224, -1000, 2834, 0, -25], m.testMinLocal(), SIMD.Int16x8, "testMinLocal");
equalSimd([5000, 3401, 665, 1000, -948, 9299, 7748, 0], m.testMaxLocal(), SIMD.Int16x8, "testMaxLocal");
equalSimd([106, -4929, -1128, 0, 3192, 32, 19, 5], m.testMinGlobal(), SIMD.Int16x8, "testMinGlobal");
equalSimd([353, 0, -10, 1082, 3692, 3937, 9755, 2638], m.testMaxGlobal(), SIMD.Int16x8, "testMaxGlobal");
equalSimd([-50, -3401, 665, -3393, -948, -27774, 0, -838], m.testMinGlobalImport(), SIMD.Int16x8, "testMinGlobalImport");
equalSimd([5033, 1000, 3092, -3224, 0, 2834, 7748, -25], m.testMaxGlobalImport(), SIMD.Int16x8, "testMaxGlobalImport");
print("PASS");
