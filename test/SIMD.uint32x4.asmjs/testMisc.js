//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports,buffer) {
    "use asm";
    
    
    var i4 = stdlib.SIMD.Int32x4;
    var i4check = i4.check;
    var i4splat = i4.splat;
    
    var i4fromFloat32x4 = i4.fromFloat32x4;
    var i4fromFloat32x4Bits = i4.fromFloat32x4Bits;
    //var i4abs = i4.abs;
    var i4neg = i4.neg;
    var i4add = i4.add;
    var i4sub = i4.sub;
    var i4mul = i4.mul;
    //var i4swizzle = i4.swizzle;
    //var i4shuffle = i4.shuffle;
    var i4lessThan = i4.lessThan;
    var i4equal = i4.equal;
    var i4greaterThan = i4.greaterThan;
    var i4select = i4.select;
    var i4and = i4.and;
    var i4or = i4.or;
    var i4xor = i4.xor;
    var i4not = i4.not;
    var i4shiftLeftByScalar = i4.shiftLeftByScalar;
    var i4shiftRightByScalar = i4.shiftRightByScalar;
    var i4min = i4.min;
    var i4max = i4.max;
    
    var u4 = stdlib.SIMD.Uint32x4;
    var u4check = u4.check;
    var u4extractLane       = u4.extractLane        ;  
    var u4swizzle           = u4.swizzle            ;
    var u4shuffle           = u4.shuffle            ;

    var u4splat             = u4.splat              ;
    var u4replaceLane       = u4.replaceLane        ;
    var u4and               = u4.and                ;
    var u4or                = u4.or                 ;
    var u4xor               = u4.xor                ;
    var u4not               = u4.not                ;
    var u4add               = u4.add                ;
    var u4sub               = u4.sub                ;
    var u4mul               = u4.mul                ;
    var u4min               = u4.min                ;
    var u4max               = u4.max                ;
    var u4shiftLeftByScalar = u4.shiftLeftByScalar  ;
    var u4shiftRightByScalar= u4.shiftRightByScalar ;
    
    var u4load              = u4.load               ;
    var u4load1              = u4.load1               ;
    var u4load2              = u4.load2               ;
    var u4load3              = u4.load3               ;
    var u4store             = u4.store              ;
    var u4store1             = u4.store1              ;
    var u4store2             = u4.store2              ;
    var u4store3             = u4.store3              ;
    var u4fromFloat32x4Bits = u4.fromFloat32x4Bits  ;
    var u4fromInt32x4Bits   = u4.fromInt32x4Bits    ;
    var u4fromInt16x8Bits  = u4.fromInt16x8Bits   ;
    var u4fromUint16x8Bits  = u4.fromUint16x8Bits   ;
    var u4fromUint8x16Bits  = u4.fromUint8x16Bits   ;

    var i8 = stdlib.SIMD.Int16x8;
    var i8check = i8.check;    
    
    var u8 = stdlib.SIMD.Uint16x8;
    var u8check = u8.check;
    
    var u16 = stdlib.SIMD.Uint8x16;
    var u16check = u16.check;
    


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
    //var f4swizzle = f4.swizzle;
    //var f4shuffle = f4.shuffle;
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

    

    var u4g1 = u4(1065353216, -1073741824, -1077936128, 1082130432);
    var u4g2 = u4(0, -824, -3128, 10);
    var gval = 1234;
    var gval2 = 1234.0;


    var Int8Heap = new stdlib.Int8Array (buffer);    
    var Uint8Heap = new stdlib.Uint8Array (buffer);    

    var Int16Heap = new stdlib.Int16Array(buffer);
    var Uint16Heap = new stdlib.Uint16Array(buffer);
    var Int32Heap = new stdlib.Int32Array(buffer);
    var Uint32Heap = new stdlib.Uint32Array(buffer);
    var Float32Heap = new stdlib.Float32Array(buffer);
    
    var loopCOUNT = 3;

    function func1()
    {
        var x = u4(-1, -2, -3, -4);;
        var y = 0;
        var s = 0;
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        x = u4g2;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            y = u4extractLane(x, 0);
            s = ( s + y ) | 0;
            y = u4extractLane(x, 1);
            s = ( s + y ) | 0;
            y = u4extractLane(x, 2);
            s = ( s + y ) | 0;
            y = u4extractLane(x, 3);
            s = ( s + y ) | 0;
            loopIndex = (loopIndex + 1) | 0;
            
        }
        
        return s | 0;
    }
    
    function func2(a)
    {
        a = u4check(a);
        var x = u4(0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {

            x = u4swizzle(x, 3, 2, 1, 0);
            x = u4shuffle(a, x, 0, 1, 7, 6);
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u4check(x);
    }
    
    function func3(a)
    {
        a = u4check(a);
        var x = u4(0, 0, 0, 0);
        var y = u4(0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u4splat(loopIndex);
            x = u4replaceLane(x, 0, 200);
            
            x = u4replaceLane(x, 3, loopIndex);
            y = x;
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u4check(y);
    }
    
    
    function func4(a, b, c, d, e)
    {
        a = u4check(a);
        b = u4check(b);
        c = u4check(c);
        d = u4check(d);
        e = u4check(e);
        var x = u4(0, 0, 0, 0);
        var y = u4(0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u4and(a, b);
            x = u4or(x, d);
            x = u4xor(x, e);
            x = u4not(x);
            
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u4check(x);
    }
    
    function func5(a, b, c, d, e)
    {
        a = u4check(a);
        b = u4check(b);
        c = u4check(c);
        d = u4check(d);
        e = u4check(e);
        var x = u4(0, 0, 0, 0);
        var y = u4(0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u4add(a, b);
            x = u4sub(x, d);
            x = u4mul(x, e);
            
            
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u4check(x);
    }
    
    function func6(a, b, c, d, e)
    {
        a = u4check(a);
        b = u4check(b);
        c = u4check(c);
        d = u4check(d);
        e = u4check(e);
        var x = u4(0, 0, 0, 0);
        var y = u4(0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u4min(x, b);
            y = u4max(y, d);
            x = u4shiftLeftByScalar(x, loopIndex>>>0);
            y = u4shiftRightByScalar(y, loopIndex>>>0);
            
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u4check(x);
    }
    
    
    
    function func7()
    {
        var loopIndex = 0;
        var loopCOUNT = 3;
        var index = 100;
        var size = 10;
        var x = u4(1, 2, -3, -4);
        var y = u4(0, 0, 0, 0);
        var t = u4(0, 0, 0, 0);
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            u4store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = u4load(Float32Heap, index >> 2);
            y = u4add(y, t);
            index = (index + 16 ) | 0;
        }
        
        x = u4add(x, y);
        
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            u4store3(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = u4load3(Float32Heap, index >> 2);
            y = u4add(y, t);
            index = (index + 16 ) | 0;
        }
        
        x = u4add(x, y);
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            u4store2(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = u4load2(Float32Heap, index >> 2);
            y = u4add(y, t);
            index = (index + 16 ) | 0;
        }
        
        x = u4add(x, y);
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            u4store1(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = u4load1(Float32Heap, index >> 2);
            y = u4add(y, t);
            index = (index + 16 ) | 0;
        }
        
        x = u4add(x, y);
        return u4check(y);
    }

    function bug1() //simd tmp reg reuse.
    {
        var a = i4(1,2,3,4);
        var x = u4(-1, -2, -3, -4);
        u4add(x,x);
        return i4check(a);
    }

    function func8()
    {
        var loopIndex = 0;
        var loopCOUNT = 3;
        var index = 100;
        var size = 10;
        var x = u4(1, -1, -3, -4);
        var y = u4(0, 0, 0, 0);
        var t = u4(0, 0, 0, 0);
        var v_i4 = i4(-29,34,10,4);
        var v_f4 = f4(1.0, -200.0, 123.443, 3000.1);
        var v_i8 = i8(-29,0xffff,10,4, -104, -9999, 1, 22);
        var v_u8 = u8(-29,0xffff,10,4, -104, -9999, 1, 22);
        var v_u16 = u16(-29,0xffff,10,4, -104, -9999, 1, 22, -29,0xffff,10,4, -104, -9999, 1, 22);
        
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            x = u4add(x, u4fromFloat32x4Bits(v_f4));
            x = u4add(x, u4fromInt32x4Bits(v_i4));
            x = u4add(x, u4fromInt16x8Bits(v_i8));
            x = u4add(x, u4fromUint16x8Bits(v_u8));
            x = u4add(x, u4fromUint8x16Bits(v_u16));
        }

        return u4check(x);
    }

    return {func1:func1, func2: func2, func3:func3, func4:func4, 
            func5: func5, func6:func6, func7:func7, func8:func8,
            bug1:bug1};
}

var buffer = new ArrayBuffer(0x10000);
var m = asmModule(this, {g1:SIMD.Float32x4(90934.2,123.9,419.39,449.0), g2:SIMD.Int32x4(-1065353216, -1073741824,-1077936128, -1082130432)}, buffer);



var v1 = SIMD.Uint32x4(1, 2, 3, 4 );
var v2 = SIMD.Uint32x4(134, 211, 0xffff, 0xf0f0);
var v3 = SIMD.Uint32x4(0xcccc, -999996, 0xffff, 0xf0f0);
var ret1 = m.func1();

var ret2 = m.func2(v1);
//printSimdBaseline(ret2, "SIMD.Uint32x4", "ret2", "func2");

var ret3 = m.func3(v1);
//printSimdBaseline(ret3, "SIMD.Uint32x4", "ret3", "func3");

var ret4 = m.func4(v1, v2, v3, v1, v2);
//printSimdBaseline(ret4, "SIMD.Uint32x4", "ret4", "func4");

var ret5 = m.func5(v1, v2, v3, v1, v2);
//printSimdBaseline(ret5, "SIMD.Uint32x4", "ret5", "func5");

var ret6 = m.func6(v1, v2, v3, v1, v2);
//printSimdBaseline(ret6, "SIMD.Uint32x4", "ret6", "func6");

var ret7 = m.func7(v1, v2, v3, v1, v2);
//printSimdBaseline(ret7, "SIMD.Uint32x4", "ret7", "func7");

var ret8 = m.func8(v1, v2, v3, v1, v2);
//printSimdBaseline(ret8, "SIMD.Uint32x4", "ret8", "func8");

var ret9 = m.bug1();
//printSimdBaseline(ret9, "SIMD.Uint32x4", "ret9", "bug1");


equal(-11826, ret1);
equalSimd([1, 2, 1, 2], ret2, SIMD.Uint32x4, "func2")
equalSimd([200, 2, 2, 2], ret3, SIMD.Uint32x4, "func3")
equalSimd([4294967160, 4294967086, 4294901763, 4294905611], ret4, SIMD.Uint32x4, "func4")
equalSimd([17956, 44521, 4294836225, 3804422400], ret5, SIMD.Uint32x4, "func5")
equalSimd([0, 0, 0, 0], ret6, SIMD.Uint32x4, "func6")
equalSimd([40, 60, 4294967236, 4294967256], ret7, SIMD.Uint32x4, "func7")
equalSimd([2741894009, 2100523531, 3103445833, 2451472428], ret8, SIMD.Uint32x4, "func8")
equalSimd([1,2,3,4], ret9, SIMD.Int32x4, "bug1")

print("PASS");


