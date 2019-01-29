//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f(){
    f64 = new Float64Array([NaN]);
    u8 = new Uint8Array(f64.buffer);
    print(u8)
    
    f64 = new Float64Array([-NaN]);
    u8 = new Uint8Array(f64.buffer);
    print(u8)

    f64 = new Float32Array([NaN]);
    u8 = new Uint8Array(f64.buffer);
    print(u8)
    
    f64 = new Float32Array([-NaN]);
    u8 = new Uint8Array(f64.buffer);
    print(u8)

    // GitHub bug #398
    function numberToRawBits(v) {
        var isLittleEndian = new Uint8Array(new Uint16Array([1]).buffer)[0] === 1;
        var reduce = Array.prototype[isLittleEndian ? 'reduceRight' : 'reduce'];
        var uint8 = new Uint8Array(new Float64Array([v]).buffer);
        return reduce.call(uint8, (a, v) => a + (v < 16 ? "0" : "") + v.toString(16), "");
    }
    
    console.log(numberToRawBits(NaN));
    console.log(numberToRawBits(-NaN));
  
}
f()
f()

