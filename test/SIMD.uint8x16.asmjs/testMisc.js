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
    
    var u8 = stdlib.SIMD.Uint16x8;
    var u8check = u8.check;
    
    
    var u16 = stdlib.SIMD.Uint8x16;
    var u16check = u16.check;
    var u16extractLane       = u16.extractLane        ;  
    var u16swizzle           = u16.swizzle            ;
    var u16shuffle           = u16.shuffle            ;

    var u16splat             = u16.splat              ;
    var u16replaceLane       = u16.replaceLane        ;
    var u16and               = u16.and                ;
    var u16or                = u16.or                 ;
    var u16xor               = u16.xor                ;
    var u16not               = u16.not                ;
    var u16add               = u16.add                ;
    var u16sub               = u16.sub                ;
    var u16mul               = u16.mul                ;
    var u16min               = u16.min                ;
    var u16max               = u16.max                ;
    var u16shiftLeftByScalar = u16.shiftLeftByScalar  ;
    var u16shiftRightByScalar= u16.shiftRightByScalar ;
    var u16addSaturate       = u16.addSaturate        ;
    var  u16subSaturate      = u16.subSaturate       ;
    var u16load              = u16.load               ;
    var u16store             = u16.store              ;
    var u16fromFloat32x4Bits = u16.fromFloat32x4Bits  ;
    var u16fromInt32x4Bits   = u16.fromInt32x4Bits    ;
    var u16fromInt16x8Bits  = u16.fromInt16x8Bits   ;
    var u16fromUint32x4Bits  = u16.fromUint32x4Bits   ;
    var u16fromUint16x8Bits  = u16.fromUint16x8Bits   ;

    
   
    


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

    

    var u16g1 = u16(1065353216, -1073741824, -1077936128, 1082130432, 1065353216, -1073741824, -1077936128, 1082130432, 0, -124, 6128, 10432, 216, -824, -3128, 10);
    var u16g2 = u16(0, -124, 6128, 10432, 216, -824, -3128, 10, -1077936128, 1082130432, 1065353216, -1073741824, -1077936128, 1082130432, 0, -124);
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
    
    var i16 = stdlib.SIMD.Int8x16;
    var i16check = i16.check;
    var i16fu16 = i16.fromUint8x16Bits;
    var u16fi16 = u16.fromInt8x16Bits;

    function func1()
    {
        var x = u16(-1, -2, -3, -4, -5, -6, -7, -8, -1, -2, -3, -4, -5, -6, -7, -8);;
        var y = 0;
        var s = 0;
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        x = u16g2;
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            y = u16extractLane(x, 0);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 1);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 2);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 3);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 4);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 5);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 6);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 7);
            s = ( s + y ) | 0;
            
            y = u16extractLane(x, 8);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 9);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 10);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 11);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 12);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 13);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 14);
            s = ( s + y ) | 0;
            y = u16extractLane(x, 15);
            s = ( s + y ) | 0;
            
            loopIndex = (loopIndex + 1) | 0;
            
        }
        
        return s | 0;
    }
    
    function func2(a)
    {
        a = i16check(a);
        var x = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {

            x = u16swizzle(x, 1, 2, 3, 4, 0, 7, 7, 7, 11, 13, 12, 14, 10, 15, 15, 15);
            x = u16shuffle(u16fi16(a), x, 0, 1, 10, 11, 2, 3, 12, 13, 20, 21, 31, 3, 22, 0, 18, 31);
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return i16check(i16fu16(x));
    }
    
    function func3(a)
    {
        a = i16check(a);
        var x = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var y = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u16splat(loopIndex);
            x = u16replaceLane(x, 0, 200);
            
            x = u16replaceLane(x, 7, loopIndex);
            y = x;
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return i16check(i16fu16(y));
    }
    
    
    function func4(a, b, c, d, e)
    {
        a = i16check(a);
        b = i16check(b);
        c = i16check(c);
        d = i16check(d);
        e = i16check(e);
        var x = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var y = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u16and(u16fi16(a), u16fi16(b));
            x = u16or(x, u16fi16(d));
            x = u16xor(x, u16fi16(e));
            x = u16not(x);
            
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return i16check(i16fu16(x));
    }
    
    function func5(a, b, c, d, e)
    {
        a = i16check(a);
        b = i16check(b);
        c = i16check(c);
        d = i16check(d);
        e = i16check(e);
        var x = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var y = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u16add(u16fi16(a), u16fi16(b));
            x = u16sub(x, u16fi16(d));
            x = u16mul(x, u16fi16(e));
            
            loopIndex = (loopIndex + 1) | 0;
        }
        
        return i16check(i16fu16(x));
    }
    
    function func6(a, b, c, d, e)
    {
        a = i16check(a);
        b = i16check(b);
        c = i16check(c);
        d = i16check(d);
        e = i16check(e);
        var x = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var y = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var loopIndex = 0;
        var loopCOUNT = 3;
        
        
        while ( (loopIndex|0) < (loopCOUNT|0)) {
            x = u16min(x, u16fi16(b));
            y = u16max(y, u16fi16(d));
            x = u16shiftLeftByScalar(x, loopIndex>>>0);
            y = u16shiftRightByScalar(y, loopIndex>>>0);
            
            loopIndex = (loopIndex + 1) | 0;
        }
        x = u16addSaturate(x, x);
        x = u16subSaturate(x, y);
        return i16check(i16fu16(x));
    }
    
    
    
    function func7()
    {
        var loopIndex = 0;
        var loopCOUNT = 3;
        var index = 100;
        var size = 10;
        var x = u16(1, 2, 3,0xff, -1, -2, -3, -4, 16, 24, 32, 41, -18, -27, -63, 0xff);
        var y = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var t = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            u16store(Uint32Heap, index >> 2, x);
            index = (index + 16 ) | 0;
        }

        index = 100;
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            t = u16load(Float32Heap, index >> 2);
            y = u16add(y, t);
            index = (index + 16 ) | 0;
        }
        return i16check(i16fu16(y));
    }
    
    function func8()
    {
        var loopIndex = 0;
        var loopCOUNT = 3;
        var index = 100;
        var size = 10;
        var x = u16(1, 2, 3,0xff, -1, -2, -3, -4, 16, 24, 32, 41, -18, -27, -63, 0xff);
        var y = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var t = u16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        var v_i4 = i4(-29,34,10,4);
        var v_f4 = f4(1.0, -200.0, 123.443, 3000.1);
        var v_u4 = u4(-29,0xffff,10,4);
        var v_i8 = i8(-29,0xffff,10,4, -104, -9999, 1, 22);
        var v_u8 = u8(-29,0xffff,10,4, -104, -9999, 1, 22);
        
        
        for (loopIndex = 0; (loopIndex | 0) < (size | 0) ; loopIndex = (loopIndex + 1) | 0)
        {
            x = u16add(x, u16fromFloat32x4Bits(v_f4));
            x = u16add(x, u16fromInt32x4Bits(v_i4));
            x = u16add(x, u16fromInt16x8Bits(v_i8));
            x = u16add(x, u16fromUint32x4Bits(v_u4));
            x = u16add(x, u16fromUint16x8Bits(v_u8));
        }

        return i16check(i16fu16(x));
    }

    function bug1() //simd tmp reg reuse.
    {
        var a = i4(1,2,3,4);
        var x = u16(-1, -2, -3, -4,-1, -2, -3, -4,-1, -2, -3, -4,-1, -2, -3, -4);
        u16add(x,x);
        return i4check(a);
    }

    return {func1:func1, func2: func2, func3:func3, func4:func4, func5: func5, 
            func6:func6, func7:func7, func8:func8, bug1:bug1};
}

var buffer = new ArrayBuffer(0x10000);
var m = asmModule(this, {g1:SIMD.Float32x4(90934.2,123.9,419.39,449.0), g2:SIMD.Int32x4(-1065353216, -1073741824,-1077936128, -1082130432)}, buffer);


var i16fu16 = SIMD.Int8x16.fromUint8x16Bits;

var v1 = SIMD.Uint8x16(1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8);
var v2 = SIMD.Uint8x16(134, 211, -333, 422, -165, 999996, 0xffff, 0xf0f0, 134, 211, -333, 422, -165, 999996, 0xffff, 0xf0f0);
var v3 = SIMD.Uint8x16(0xcccc, -211, 189, 422, -165, -999996, 0xffff, 0xf0f0, 0xcccc, -211, 189, 422, -165, -999996, 0xffff, 0xf0f0);
var ret1 = m.func1();
//print (ret1);
var ret2 = SIMD.Uint8x16.fromInt8x16Bits(m.func2(i16fu16(v1)));
//printSimdBaseline(ret2, "SIMD.Uint8x16", "ret2", "func2");


var ret3 = SIMD.Uint8x16.fromInt8x16Bits(m.func3(i16fu16(v1)));
//printSimdBaseline(ret3, "SIMD.Uint8x16", "ret3", "func3");


var ret4 = SIMD.Uint8x16.fromInt8x16Bits(m.func4(i16fu16(v1), i16fu16(v2), i16fu16(v3), i16fu16(v1), i16fu16(v2)));
//printSimdBaseline(ret4, "SIMD.Uint8x16", "ret4", "func4");


var ret5 = SIMD.Uint8x16.fromInt8x16Bits(m.func5(i16fu16(v1), i16fu16(v2), i16fu16(v3), i16fu16(v1), i16fu16(v2)));
//printSimdBaseline(ret5, "SIMD.Uint8x16", "ret5", "func5");



var ret6 = SIMD.Uint8x16.fromInt8x16Bits(m.func6(i16fu16(v1), i16fu16(v2), i16fu16(v3), i16fu16(v1), i16fu16(v2)));
//printSimdBaseline(ret6, "SIMD.Uint8x16", "ret6", "func6");


var ret7 = SIMD.Uint8x16.fromInt8x16Bits(m.func7(i16fu16(v1), i16fu16(v2), i16fu16(v3), i16fu16(v1), i16fu16(v2)));
//printSimdBaseline(ret7, "SIMD.Uint8x16", "ret7", "func7");


var ret8 = SIMD.Uint8x16.fromInt8x16Bits(m.func8(i16fu16(v1), i16fu16(v2), i16fu16(v3), i16fu16(v1), i16fu16(v2)));
//printSimdBaseline(ret8, "SIMD.Uint8x16", "ret8", "func8");

var ret9 = m.bug1();
//printSimdBaseline(ret9, "SIMD.Int8x16", "ret9", "bug1");

equal(3966, ret1);
equalSimd([1, 2, 3, 4, 3, 4, 5, 6, 1, 6, 0, 4, 6, 1, 4, 0], ret2, SIMD.Uint8x16, "func2")
equalSimd([200, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2], ret3, SIMD.Uint8x16, "func3")
equalSimd([120, 46, 79, 93, 161, 197, 7, 7, 120, 46, 79, 93, 161, 197, 7, 7], ret4, SIMD.Uint8x16, "func4")
equalSimd([36, 233, 41, 164, 89, 16, 1, 0, 36, 233, 41, 164, 89, 16, 1, 0], ret5, SIMD.Uint8x16, "func5")
equalSimd([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], ret6, SIMD.Uint8x16, "func6")
equalSimd([10, 20, 30, 246, 246, 236, 226, 216, 160, 240, 64, 154, 76, 242, 138, 246], ret7, SIMD.Uint8x16, "func7")
equalSimd([121, 218, 219, 77, 17, 244, 29, 154, 226, 216, 144, 157, 86, 239, 199, 177], ret8, SIMD.Uint8x16, "func8")
equalSimd([1,2,3,4], ret9, SIMD.Int32x4, "bug1")
print("PASS");


