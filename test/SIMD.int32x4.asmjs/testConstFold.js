//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function asmModule(stdlib, imports) {
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
    //var i4shiftLeftByScalar = i4.shiftLeftByScalar;
    //var i4shiftRightByScalar = i4.shiftRightByScalar;
    //var i4shiftRightArithmeticByScalar = i4.shiftRightArithmeticByScalar;

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


    

    var fround = stdlib.Math.fround;
	var loopCOUNT = 10;

	function func2() {
		
		
		var a = i4(1,1,1,1);
		var b = i4(2,2,2,2);
		var c = i4(3,3,3,3);
		var d = i4(1,1,1,1);
		var loopIndex = 0;
		
		for (loopIndex = 0; (loopIndex | 0) < (loopCOUNT | 0) ; loopIndex = (loopIndex + 1) | 0) {

			d = i4add(b,c);
			d = i4mul(d,c);
			d = i4sub(d,b);

        }
		

        return i4check(d);
    }

    


    function func1()
    {
        var i = 0;
		var a = i4(1,1,1,1);
		var b = i4(2,2,2,2);
		var c = i4(3,3,3,3);
		
		a = i4add(b,c);
		a = i4mul(a,c);
		a = i4sub(a,b);

        return i4check(a);
    }
    
    
    
    return {func1:func1, func2:func2};
}

var m = asmModule(this, {});

print(m.func1());
print(m.func2());

print("PASS");
