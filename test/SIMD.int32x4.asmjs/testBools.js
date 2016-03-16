//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");

function asmModule(stdlib, imports) {
    "use asm";


    var b4  = stdlib.SIMD.Bool32x4;
    var b8  = stdlib.SIMD.Bool16x8;
    var b16 = stdlib.SIMD.Bool8x16;
    
    var b4check  = b4.check;
    var b8check  = b8.check;
    var b16check = b16.check;
    
    var b4and  = b4.and;
    var b8and  = b8.and;
    var b16and = b16.and;
    
    var b4or = b4.or;
    var b8or = b8.or;
    var b16or= b16.or;
    
    var b4xor = b4.xor;
    var b8xor = b8.xor;
    var b16xor= b16.xor;
    
    var b4not = b4.not;
    var b8not  = b8.not;
    var b16not = b16.not;
    
    var b4allTrue = b4.allTrue;
    var b8allTrue  = b8.allTrue;
    var b16allTrue = b16.allTrue;
    
    var b4anyTrue = b4.anyTrue;
    var b8anyTrue  = b8.anyTrue;
    var b16anyTrue = b16.anyTrue;
    
    var g1 = b4(1,1,0,1);
    var g2 = b8(1,1,0,1,1,1,0,1);
    var g3 = b16(1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1);
    function and1(a)
    {
        a = a | 0;
        var b = b4(0,5,0,0);
        
        b = b4and(b,g1);
       
        return b4check(b);
        
    }
    function and2(a)
    {   a = a | 0;   var b = b8(0,5,0,0,0,5,0,0);
        b = b8and(b,g2); 
        return b8check(b);     
    } 
    function and3(a)
    {   a = a | 0;   var b = b16(0,5,0,0,0,5,0,0,0,5,0,0,0,5,0,0);
        b = b16and(b,g3); 
        return b16check(b);     }
    
    function or1(a)
    {   a = a | 0;   var b = b4(0,5,0,0);
        b = b4or(b,g1); return b4check(b);     } 
            
    function or2(a)
    {   a = a | 0;   var b = b8(0,5,0,0,0,5,0,0);
        b = b8or(b,g2); return b8check(b);     } 
    function or3(a)
    {   a = a | 0;   var b = b16(0,5,0,0,0,5,0,0,0,5,0,0,0,5,0,0);
        b = b16or(b,g3); 
        return b16check(b);     }
        
    function xor1(a)
    {   a = a | 0;   var b = b4(0,5,0,0);
        b = b4xor(b,g1); return b4check(b);     } 
            
    function xor2(a)
    {   a = a | 0;   var b = b8(0,5,0,0,0,5,0,0);
        b = b8xor(b,g2); return b8check(b);     } 
    function xor3(a)
    {   a = a | 0;   var b = b16(0,5,0,0,0,5,0,0,0,5,0,0,0,5,0,0);
        b = b16xor(b,g3); 
        return b16check(b);     }
        
    function not1(a)
    {   a = a | 0;   var b = b4(0,5,0,0);
        b = b4not(b); return b4check(b);     } 
            
    function not2(a)
    {   a = a | 0;   var b = b8(0,5,0,0,0,5,0,0);
        b = b8not(b); return b8check(b);     } 
    function not3(a)
    {   a = a | 0;   var b = b16(0,5,0,0,0,5,0,0,0,5,0,0,0,5,0,0);
        b = b16not(b); 
        return b16check(b);     }
        
    function allTrue1(a)
    {   a = a | 0;   
        var b1 = b4(0,5,0,0);
        var b2 = b4(-2, 2, 1, 3);
        var x = 0;
        
        x = ((b4allTrue(b1) | 0) + (b4allTrue(b2) | 0)) | 0;
        return x | 0;
    } 
            
    function allTrue2(a)
    {   a = a | 0;   
        var b1 = b8(0,5,0,0,1,5,0,1);
        var b2 = b8(1,5,3,1,0,5,-100,0);
        
        b1 = b8or(b1, b2);
        
        return b8allTrue(   b1) | 0;     
        
    } 
    
    function allTrue3(a)
    {   a = a | 0;   var b = b16(0,5,0,0,0,5,0,0,0,5,0,0,0,5,0,0);
        return b16allTrue(b)| 0;    }    
        
    function anyTrue1(a)
    {   a = a | 0;   var b = b4(0,5,0,0);
        return b4anyTrue(b) | 0;     } 
            
    function anyTrue2(a)
    {   a = a | 0;   var b = b8(0,5,0,0,0,5,0,0);
        return b8anyTrue(b) | 0;     } 
    function anyTrue3(a)
    {   a = a | 0;   var b = b16(0,5,0,0,0,5,0,0,0,5,0,0,0,5,0,0);
        return b16anyTrue(b) | 0;     }                           
       
        
    return {
        and1: and1,
        and2: and2,
        and3: and3,
        or1:  or1,
        or2:  or2,
        or3:  or3,
        xor1: xor1,
        xor2: xor2,
        xor3: xor3,
        not1: not1,
        not2: not2,
        not3: not3,
        allTrue1: allTrue1,
        allTrue2: allTrue2,
        allTrue3: allTrue3,
        anyTrue1: anyTrue1,
        anyTrue2: anyTrue2,
        anyTrue3: anyTrue3
    };
}

var m = asmModule(this, {g1:SIMD.Int8x16(0, 1, 2, 3, 4, 5, -6, -7, -8, 9, 10, 11, 12, 13, 14, 15)});
var b4 = SIMD.Bool32x4;
var b8 = SIMD.Bool16x8;
var b16 = SIMD.Bool8x16;

equalSimd([false, true, false, false], m.and1(1), SIMD.Bool32x4, "and1")
equalSimd([false, true, false, false], m.and1(1), SIMD.Bool32x4, "and1")

equalSimd([false, true, false, false, false, true, false, false], m.and2(1), SIMD.Bool16x8, "and2")
equalSimd([false, true, false, false, false, true, false, false], m.and2(1), SIMD.Bool16x8, "and2")

equalSimd([false, true, false, false, false, true, false, false, false, true, false, false, false, true, false, false], m.and3(1), SIMD.Bool8x16, "and3")
equalSimd([false, true, false, false, false, true, false, false, false, true, false, false, false, true, false, false], m.and3(1), SIMD.Bool8x16, "and3")

equalSimd([true, true, false, true], m.or1(1), SIMD.Bool32x4, "or1")
equalSimd([true, true, false, true], m.or1(1), SIMD.Bool32x4, "or1")

equalSimd([true, true, false, true, true, true, false, true, true, true, false, true, true, true, false, true], m.or2(1), SIMD.Bool8x16, "or3")
equalSimd([true, true, false, true, true, true, false, true, true, true, false, true, true, true, false, true], m.or2(1), SIMD.Bool8x16, "or3")

equalSimd([true, true, false, true, true, true, false, true], m.or3(1), SIMD.Bool16x8, "or2")
equalSimd([true, true, false, true, true, true, false, true], m.or3(1), SIMD.Bool16x8, "or2")

equalSimd([true, false, false, true], m.xor1(1), SIMD.Bool32x4, "xor1")
equalSimd([true, false, false, true], m.xor1(1), SIMD.Bool32x4, "xor1")

equalSimd([true, false, false, true, true, false, false, true], m.xor2(1), SIMD.Bool16x8, "xor2")
equalSimd([true, false, false, true, true, false, false, true], m.xor2(1), SIMD.Bool16x8, "xor2")

equalSimd([true, false, false, true, true, false, false, true, true, false, false, true, true, false, false, true], m.xor3(1), SIMD.Bool8x16, "xor3")
equalSimd([true, false, false, true, true, false, false, true, true, false, false, true, true, false, false, true], m.xor3(1), SIMD.Bool8x16, "xor3")

equalSimd([true, false, true, true], m.not1(1), SIMD.Bool32x4, "not1")
equalSimd([true, false, true, true], m.not1(1), SIMD.Bool32x4, "not1")

equalSimd([true, false, true, true, true, false, true, true], m.not2(1), SIMD.Bool16x8, "not2")
equalSimd([true, false, true, true, true, false, true, true], m.not2(1), SIMD.Bool16x8, "not2")

equalSimd([true, false, true, true, true, false, true, true, true, false, true, true, true, false, true, true], m.not3(1), SIMD.Bool8x16, "not3")
equalSimd([true, false, true, true, true, false, true, true, true, false, true, true, true, false, true, true], m.not3(1), SIMD.Bool8x16, "not3")

equal(m.allTrue1(1), 1);
equal(m.allTrue1(1), 1);

equal(m.allTrue2(1), 1);
equal(m.allTrue2(1), 1);

equal(m.allTrue3(1), 0);
equal(m.allTrue3(1), 0);

equal(m.anyTrue1(1), 1);
equal(m.anyTrue1(1), 1);

equal(m.anyTrue2(1), 1);
equal(m.anyTrue2(1), 1);

equal(m.anyTrue3(1), 1);
equal(m.anyTrue3(1), 1);

print("PASS");
