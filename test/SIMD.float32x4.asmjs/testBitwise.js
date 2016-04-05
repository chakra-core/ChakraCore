//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");

function asmModule(stdlib, imports) {
    "use asm";
    var i4 = stdlib.SIMD.Int32x4;
    var i4check = i4.check;
    var i4splat = i4.splat;
    
    var i4fromFloat32x4 = i4.fromFloat32x4;
    var i4fromFloat32x4Bits = i4.fromFloat32x4Bits;

    var i4neg = i4.neg;
    var i4add = i4.add;
    var i4sub = i4.sub;
    var i4mul = i4.mul;
    var i4lessThan = i4.lessThan;
    var i4equal = i4.equal;
    var i4greaterThan = i4.greaterThan;
    var i4select = i4.select;
    var i4and = i4.and;
    var i4or = i4.or;
    var i4xor = i4.xor;
    var i4not = i4.not;

    var f4 = stdlib.SIMD.Float32x4;  
    var f4check = f4.check;
    var f4splat = f4.splat;
    
    var f4fromInt32x4 = f4.fromInt32x4;
    var f4fromInt32x4Bits = f4.fromInt32x4Bits;
    var f4abs = f4.abs;
    var f4neg = f4.neg;
    var f4add = f4.add;
    var f4sub = f4.sub;
    var f4mul = f4.mul;
    var f4div = f4.div;
    var f4min = f4.min;
    var f4max = f4.max;
    var f4sqrt = f4.sqrt;

    var f4lessThan = f4.lessThan;
    var f4lessThanOrEqual = f4.lessThanOrEqual;
    var f4equal = f4.equal;
    var f4notEqual = f4.notEqual;
    var f4greaterThan = f4.greaterThan;
    var f4greaterThanOrEqual = f4.greaterThanOrEqual;

    var f4select = f4.select;
    var f4and = f4.and;
    var f4or = f4.or;
    var f4xor = f4.xor;
    var f4not = f4.not;

        
    var fround = stdlib.Math.fround;

    var globImportF4 = f4check(imports.g1);       // global var import
    var globImportI4 = i4check(imports.g2);       // global var import
    

    var f4g1 = f4(-5033.2,-3401.0,665.34,32234.1);          // global var initialized
    var f4g2 = f4(1194580.33,-11201.5,63236.93,334.8);          // global var initialized

    var i4g1 = i4(1065353216, -1073741824, -1077936128, 1082130432);          // global var initialized
    var i4g2 = i4(353216, -492529, -1128, 1085);          // global var initialized

    var gval = 1234;
    var gval2 = 1234.0;
    
    var loopCOUNT = 3;

    function func1()
    {
        var b = f4(5033.2,-3401.0,665.34,-32234.1);
        var c = f4(-34183.8985, 212344.12, -569437.0, 65534.99);
        var d = f4(0.0, 0.0, 0.0, 0.0);
        var loopIndex = 0;

        loopIndex = 0;
        while ( (loopIndex|0) < (loopCOUNT|0)) {

            d = f4and(b, b);
            b = f4or (c, c);
            d = f4xor(b, d);
            d = f4not(d);

            loopIndex = (loopIndex + 1) | 0;
        }

        return f4check(d);
    }
    
    function func2()
    {
        var b = f4(5033.2,-3401.0,665.34,-32234.1);
        var c = f4(-34183.8985, 212344.12, -569437.0, 65534.99);
        var d = f4(0.0, 0.0, 0.0, 0.0);
        var loopIndex = 0;

        for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0)
        {

            d = f4and(f4g1, f4g2);
            d = f4or (d, b);
            d = f4xor(d, globImportF4);
            f4g1 = f4not(d);
            globImportF4 = f4and(f4g1, b);

        }

        return f4check(d);
    }

    function func3()
    {
        var b = f4(5033.2,-3401.0,665.34,-32234.1);
        var c = f4(-34183.8985, 212344.12, -569437.0, 65534.99);
        var d = f4(0.0, 0.0, 0.0, 0.0);
        var loopIndex = 0;

        loopIndex = loopCOUNT | 0;
        do {

            f4g1 = f4and(f4g1, f4g2);
            f4g1 = f4or(f4g1, b);
            f4g1 = f4xor(f4g1, c);
            f4g1 = f4not(f4g1);

            loopIndex = (loopIndex - 1) | 0;
        }
        while ( (loopIndex | 0) > 0);

        return f4check(f4g1);
    }
    
    
    return {func1:func1, func2:func2, func3:func3};
}

var m = asmModule(this, {g1:SIMD.Float32x4(90934.2,123.9,419.39,449.0), g2:SIMD.Int32x4(-1065353216, -1073741824,-1077936128, -1082130432)});

var c;
c = m.func1();
equalSimd([NaN, NaN, NaN, NaN], c, SIMD.Float32x4, "func1");

c = m.func2();
equalSimd([0.0, -0.0, 0.0, -0.0], c, SIMD.Float32x4, "func2");

c = m.func3();
equalSimd([293073058436145367398439507197952.0     , 9356964346772358541993744269312.0       , 8346179655603837667078148456448.0       , 85059502322783819315039358698301423616.0], c, SIMD.Float32x4, "func3");
WScript.Echo("PASS");