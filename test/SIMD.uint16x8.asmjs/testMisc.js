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
    
    var i8 = stdlib.SIMD.Int16x8;
    var i8check = i8.check;    
    
    var u4 = stdlib.SIMD.Uint32x4;
    var u4check = u4.check;
    
    var u16 = stdlib.SIMD.Uint8x16;
    var u16check = u16.check;
    
    
    var u8 = stdlib.SIMD.Uint16x8;
    var u8check = u8.check;
    var u8extractLane       = u8.extractLane        ;  
    var u8swizzle           = u8.swizzle            ;
    var u8shuffle           = u8.shuffle            ;

    var u8splat             = u8.splat              ;
    var u8replaceLane       = u8.replaceLane        ;
    var u8and               = u8.and                ;
    var u8or                = u8.or                 ;
    var u8xor               = u8.xor                ;
    var u8not               = u8.not                ;
    var u8add               = u8.add                ;
    var u8sub               = u8.sub                ;
    var u8mul               = u8.mul                ;
    var u8min               = u8.min                ;
    var u8max               = u8.max                ;
    var u8shiftLeftByScalar = u8.shiftLeftByScalar  ;
    var u8shiftRightByScalar= u8.shiftRightByScalar ;
    var u8addSaturate       = u8.addSaturate        ;
    var  u8subSaturate      = u8.subSaturate       ;
    var u8load              = u8.load               ;
    var u8store             = u8.store              ;
    var u8fromFloat32x4Bits = u8.fromFloat32x4Bits  ;
    var u8fromInt32x4Bits   = u8.fromInt32x4Bits    ;
    var u8fromInt16x8Bits  = u8.fromInt16x8Bits   ;
    var u8fromUint32x4Bits  = u8.fromUint32x4Bits   ;
    var u8fromUint8x16Bits  = u8.fromUint8x16Bits   ;

    
   
    


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

    

    var u8g1 = u8(1065353216, -1073741824, -1077936128, 1082130432, 1065353216, -1073741824, -1077936128, 1082130432);
    var u8g2 = u8(0, -124, 6128, 10432, 216, -824, -3128, 10);
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
        var x = u8(-1, -2, -3, -4, -5, -6, -7, -8);;
        var y = 0;
        var s = 0;
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        x = u8g2;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            y = u8extractLane(x, 0);
            s = ( s + y ) | 0;
            y = u8extractLane(x, 1);
            s = ( s + y ) | 0;
            y = u8extractLane(x, 2);
            s = ( s + y ) | 0;
            y = u8extractLane(x, 3);
            s = ( s + y ) | 0;
            y = u8extractLane(x, 4);
            s = ( s + y ) | 0;
            y = u8extractLane(x, 5);
            s = ( s + y ) | 0;
            y = u8extractLane(x, 6);
            s = ( s + y ) | 0;
            y = u8extractLane(x, 7);
            s = ( s + y ) | 0;
            
            loopIndex = (loopIndex + 1) | 0;
            
        }
        
        return s | 0;
    }
    
    function func2(a)
    {
        a = u8check(a);
        var x = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {

            x = u8swizzle(x, 1, 2, 3, 4, 0, 7, 7, 7);
            x = u8shuffle(a, x, 0, 1, 10, 11, 2, 3, 12, 13);
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u8check(x);
    }
    
    function func3(a)
    {
        a = u8check(a);
        var x = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var y = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u8splat(loopIndex);
            x = u8replaceLane(x, 0, 200);
            
            x = u8replaceLane(x, 7, loopIndex);
            y = x;
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u8check(y);
    }
    
    
    function func4(a, b, c, d, e)
    {
        a = u8check(a);
        b = u8check(b);
        c = u8check(c);
        d = u8check(d);
        e = u8check(e);
        var x = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var y = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u8and(a, b);
            x = u8or(x, d);
            x = u8xor(x, e);
            x = u8not(x);
            
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u8check(x);
    }
    
    function func5(a, b, c, d, e)
    {
        a = u8check(a);
        b = u8check(b);
        c = u8check(c);
        d = u8check(d);
        e = u8check(e);
        var x = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var y = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u8add(a, b);
            x = u8sub(x, d);
            x = u8mul(x, e);
            
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return u8check(x);
    }
    
    function func6(a, b, c, d, e)
    {
        a = u8check(a);
        b = u8check(b);
        c = u8check(c);
        d = u8check(d);
        e = u8check(e);
        var x = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var y = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u8min(x, b);
            y = u8max(y, d);
            x = u8shiftLeftByScalar(x, loopIndex>>>0);
            y = u8shiftRightByScalar(y, loopIndex>>>0);
            
            loopIndex = (loopIndex + 1) | 0;
        }
        x = u8addSaturate(x, x);
        x = u8subSaturate(x, y);
        return u8check(x);
    }
    
    
    
    function func7()
    {
        var loopIndex = 0;
        var loopCOUNT = 3;
        var index = 100;
        var size = 10;
        var x = u8(1, 2, 3, 4, -1, -2, -3, -4);
        var y = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var t = u8(0, 0, 0, 0, 0, 0, 0, 0);
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            u8store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = u8load(Float32Heap, index >> 2);
            y = u8add(y, t);
            index = (index + 16 ) | 0;
        }
        return u8check(y);
    }
    
    function func8()
    {
        var loopIndex = 0;
        var loopCOUNT = 3;
        var index = 100;
        var size = 10;
        var x = u8(1, 2, 3, 4, -1, -2, -3, -4);
        var y = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var t = u8(0, 0, 0, 0, 0, 0, 0, 0);
        var v_i4 = i4(-29,34,10,4);
        var v_f4 = f4(1.0, -200.0, 123.443, 3000.1);
        var v_u4 = u4(-29,0xffff,10,4);
        var v_i8 = i8(-29,0xffff,10,4, -104, -9999, 1, 22);
        var v_u16 = u16(-29,0xffff,10,4, -104, -9999, 1, 22, -29,0xffff,10,4, -104, -9999, 1, 22);
        
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            x = u8add(x, u8fromFloat32x4Bits(v_f4));
            x = u8add(x, u8fromInt32x4Bits(v_i4));
            x = u8add(x, u8fromInt16x8Bits(v_i8));
            x = u8add(x, u8fromUint32x4Bits(v_u4));
            x = u8add(x, u8fromUint8x16Bits(v_u16));
        }

        return u8check(x);
    }

    return {func1:func1, func2: func2, func3:func3, func4:func4, func5: func5, func6:func6, func7:func7, func8:func8};
}

var buffer = new ArrayBuffer(0x10000);
var m = asmModule(this, {g1:SIMD.Float32x4(90934.2,123.9,419.39,449.0), g2:SIMD.Int32x4(-1065353216, -1073741824,-1077936128, -1082130432)}, buffer);



var v1 = SIMD.Uint16x8(1, 2, 3, 4, 5, 6, 7, 8);
var v2 = SIMD.Uint16x8(134, 211, -333, 422, -165, 999996, 0xffff, 0xf0f0);
var v3 = SIMD.Uint16x8(0xcccc, -211, 189, 422, -165, -999996, 0xffff, 0xf0f0);
var ret1 = m.func1();

var ret2 = m.func2(v1);
//printSimdBaseline(ret2, "SIMD.Uint16x8", "ret2", "func2");


var ret3 = m.func3(v1);
//printSimdBaseline(ret3, "SIMD.Uint16x8", "ret3", "func3");


var ret4 = m.func4(v1, v2, v3, v1, v2);
//printSimdBaseline(ret4, "SIMD.Uint16x8", "ret4", "func4");


var ret5 = m.func5(v1, v2, v3, v1, v2);
//printSimdBaseline(ret5, "SIMD.Uint16x8", "ret5", "func5");



var ret6 = m.func6(v1, v2, v3, v1, v2);
//printSimdBaseline(ret6, "SIMD.Uint16x8", "ret6", "func6");


var ret7 = m.func7(v1, v2, v3, v1, v2);
//printSimdBaseline(ret7, "SIMD.Uint16x8", "ret7", "func7");


var ret8 = m.func8(v1, v2, v3, v1, v2);
//printSimdBaseline(ret8, "SIMD.Uint16x8", "ret8", "func8");

equal(627954, ret1);
equalSimd([1, 2, 3, 3, 3, 4, 1, 0], ret2, SIMD.Uint16x8, "func2")
equalSimd([200, 2, 2, 2, 2, 2, 2, 2], ret3, SIMD.Uint16x8, "func3")
equalSimd([65400, 65326, 335, 65117, 161, 48581, 7, 3847], ret4, SIMD.Uint16x8, "func4")
equalSimd([17956, 44521, 45353, 47012, 27225, 65040, 1, 57600], ret5, SIMD.Uint16x8, "func5")
equalSimd([0, 0, 0, 0, 0, 0, 0, 0], ret6, SIMD.Uint16x8, "func6")
equalSimd([10, 20, 30, 40, 65526, 65516, 65506, 65496], ret7, SIMD.Uint16x8, "func7")
equalSimd([64377, 41800, 29089, 32006, 55231, 16232, 32843, 37168], ret8, SIMD.Uint16x8, "func8")

print("PASS");


