//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
let passed = true;

function check(expected, funName, args) {
    checkInternal(expected, (a, b) => a == b, funName, args);
}

function checkNaN(funName, args) {
    checkInternal(Number.NaN, (a, b) => Number.isNaN(a), funName, args);
}

function checkInternal(expected, eq, funName, args) {
    let fun = eval(funName);
    var result;
    try {
        result = fun(...args);
    } catch (e) {
        print(e);
        result = e.message;
    }

    if (!eq(result, expected)) {
        passed = false;
        print(`${funName}(${[...args]}) produced ${result}, expected ${expected}`);
    }
}

let ffi = {};
var ii;
var mod;
var exports;

mod = new WebAssembly.Module(readbuffer('binaryArithmeticTests.wasm'));
exports = new WebAssembly.Instance(mod, ffi).exports;

tests = [
    //func_i32x4_add_3  
    {
        func: "func_i32x4_add_3",
        args: [128, 128, 128, 128, 2, 2, 2, 2],
        expected: 130
    },
    {
        func: "func_i32x4_add_3",
        args: [-1, -1, -1, -1, 2, 2, 2, 2],
        expected: 1
    },
    //func_i32x4_sub_3  
    {
        func: "func_i32x4_sub_3",
        args: [128, 128, 128, 128, 28, 28, 28, 28],
        expected: 100
    },
    {
        func: "func_i32x4_sub_3",
        args: [-2147483648, -2147483648, -2147483648, -2147483648, 1, 1, 1, 1],
        expected: 2147483647
    },
    //func_i32x4_mul_3  
    {
        func: "func_i32x4_mul_3",
        args: [55, 55, 55, 55, 2, 2, 2, 2],
        expected: 110
    },
    {
        func: "func_i32x4_mul_3",
        args: [2147483647, 2147483647, 2147483647, 2147483647, 2, 2, 2, 2],
        expected: -2
    },
    //func_i32x4_shl_3  
    {
        func: "func_i32x4_shl_3",
        args: [1, 1, 1, 1, 31, 31, 31, 31],
        expected: -2147483648
    },
    {
        func: "func_i32x4_shl_3",
        args: [1, 1, 1, 1, 32, 32, 32, 32],
        expected: 1
    },
    //func_i32x4_shr_3_u  
    {
        func: "func_i32x4_shr_3_u",
        args: [-2147483648, -2147483648, -2147483648, -2147483648, 31, 31, 31, 31],
        expected: 1
    },
    {
        func: "func_i32x4_shr_3_u",
        args: [-2147483648, -2147483648, -2147483648, -2147483648, 32, 32, 32, 32],
        expected: -2147483648
    },
    //func_i32x4_shr_3_s  
    {
        func: "func_i32x4_shr_3_s",
        args: [-2147483648, -2147483648, -2147483648, -2147483648, 1, 1, 1, 1],
        expected: -1073741824
    },
    {
        func: "func_i32x4_shr_3_s",
        args: [-2147483648, -2147483648, -2147483648, -2147483648, 31, 31, 31, 31],
        expected: -1
    },
    //func_i16x8_add_3_u  
    {
        func: "func_i16x8_add_3_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 130
    },
    {
        func: "func_i16x8_add_3_u",
        args: [65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 1
    },
    // (export  "func_i16x8_addsaturate_3_s_u") 
    {
        func: "func_i16x8_addsaturate_3_s_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 130
    },
    {
        func: "func_i16x8_addsaturate_3_s_u",
        args: [32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 32767
    },
    //func_i16x8_sub_3_u  
    {
        func: "func_i16x8_sub_3_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 28, 28, 28, 28, 28, 28, 28, 28],
        expected: 100
    },
    {
        func: "func_i16x8_sub_3_u",
        args: [-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 1, 1, 1, 1, 1, 1, 1, 1],
        expected: 32767
    },
    //func_i16x8_subsaturate_3_s_u  
    {
        func: "func_i16x8_subsaturate_3_s_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 28, 28, 28, 28, 28, 28, 28, 28],
        expected: 100
    },
    {
        func: "func_i16x8_subsaturate_3_s_u",
        args: [-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 1, 1, 1, 1, 1, 1, 1, 1],
        expected: 32768
    },
    //func_i16x8_mul_3_u
    {
        func: "func_i16x8_mul_3_u",
        args: [55, 55, 55, 55, 55, 55, 55, 55, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 110
    },
    {
        func: "func_i16x8_mul_3_u",
        args: [32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 65534
    },
    //func_i16x8_shl_3  
    {
        func: "func_i16x8_shl_3_u",
        args: [1, 1, 1, 1, 1, 1, 1, 1, 15, 15, 15, 15, 15, 15, 15, 15],
        expected: 32768
    },
    {
        func: "func_i16x8_shl_3_u",
        args: [1, 1, 1, 1, 1, 1, 1, 1, 16, 16, 16, 16, 16, 16, 16, 16],
        expected: 1
    },
    //func_i16x8_shr_3_u  
    {
        func: "func_i16x8_shr_3_u_u",
        args: [-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 15, 15, 15, 15, 15, 15, 15, 15],
        expected: 1
    },
    {
        func: "func_i16x8_shr_3_u_u",
        args: [-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 16, 16, 16, 16, 16, 16, 16, 16],
        expected: 32768
    },
    //func_i16x8_shr_3_s  
    {
        func: "func_i16x8_shr_3_s_u",
        args: [-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 1, 1, 1, 1, 1, 1, 1, 1],
        expected: 49152
    },
    {
        func: "func_i16x8_shr_3_s_u",
        args: [-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 15, 15, 15, 15, 15, 15, 15, 15],
        expected: 65535
    },
    //func_i8x16_add_3_u  
    {
        func: "func_i8x16_add_3_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 130
    },
    {
        func: "func_i8x16_add_3_u",
        args: [255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 1
    },
    //func_i8x16_addsaturate_3_s_u
    {
        func: "func_i8x16_addsaturate_3_s_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 130
    },
    {
        func: "func_i8x16_addsaturate_3_s_u",
        args: [255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 1
    },
    //func_i8x16_sub_3_u  
    {
        func: "func_i8x16_sub_3_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28],
        expected: 100
    },
    {
        func: "func_i8x16_sub_3_u",
        args: [-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
        expected: 127
    },
    //func_i8x16_subsaturate_3_s_u  
    {
        func: "func_i8x16_subsaturate_3_s_u",
        args: [118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28],
        expected: 90
    },
    {
        func: "func_i8x16_subsaturate_3_s_u",
        args: [-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
        expected: 128
    },
    //func_i8x16_mul_3_u
    {
        func: "func_i8x16_mul_3_u",
        args: [55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 110
    },
    {
        func: "func_i8x16_mul_3_u",
        args: [255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2],
        expected: 254
    },
    //func_i8x16_shl_3  
    {
        func: "func_i8x16_shl_3_u",
        args: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7],
        expected: 128
    },
    {
        func: "func_i8x16_shl_3_u",
        args: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8],
        expected: 1
    },
    //func_i8x16_shr_3_u  
    {
        func: "func_i8x16_shr_3_u_u",
        args: [128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7],
        expected: 1
    },
    {
        func: "func_i8x16_shr_3_u_u",
        args: [-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8],
        expected: 128
    },
    //func_i8x16_shr_3_s  
    {
        func: "func_i8x16_shr_3_s_u",
        args: [-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
        expected: 192
    },
    {
        func: "func_i8x16_shr_3_s_u",
        args: [-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7],
        expected: 255
    },
    //func_f32x4_add_3  
    {
        func: "func_f32x4_add_3",
        args: [128, 128, 128, 128, 2, 2, 2, 2],
        expected: 130
    },
    {
        func: "func_f32x4_add_3",
        args: [1073741824, 1073741824, 1073741824, 1073741824, 2, 2, 2, 2],
        expected: 1073741824
    },
    //func_f32x4_sub_3  
    {
        func: "func_f32x4_sub_3",
        args: [128, 128, 128, 128, 28, 28, 28, 28],
        expected: 100
    },
    {
        func: "func_f32x4_sub_3",
        args: [-1073741824, -1073741824, -1073741824, -1073741824, 1, 1, 1, 1],
        expected: -1073741824
    },
    //func_f32x4_mul_3  
    {
        func: "func_f32x4_mul_3",
        args: [55, 55, 55, 55, 2, 2, 2, 2],
        expected: 110
    },
    {
        func: "func_f32x4_mul_3",
        args: [1073741824, 1073741824, 1073741824, 1073741824, 16, 16, 16, 16],
        expected: 17179869184
    },
    //func_f32x4_div_3  
    {
        func: "func_f32x4_div_3",
        args: [110, 110, 110, 110, 2, 2, 2, 2],
        expected: 55
    },
    {
        func: "func_f32x4_div_3",
        args: [1073741824, 1073741824, 1073741824, 1073741824, 16, 16, 16, 16],
        expected: 67108864
    },
    //func_f32x4_min_3  
    {
        func: "func_f32x4_min_3",
        args: [-10, -10, -10, -10, 5, 5, 5, 5],
        expected: -10
    },
    //func_f32x4_max_3 
    {
        func: "func_f32x4_max_3",
        args: [-10, -10, -10, -10, 5, 5, 5, 5],
        expected: 5
    },
];

for (let t of tests) {
    check(t.expected, "exports." + t.func, t.args);
}

//NaN Tests
nanTests = [
    //func_f32x4_min_3  
    {
        func: "func_f32x4_min_3",
        args: [Number.NaN, Number.NaN, Number.NaN, Number.NaN, 2, 2, 2, 2]
    },
    {
        func: "func_f32x4_min_3",
        args: [2, 2, 2, 2, Number.NaN, Number.NaN, Number.NaN, Number.NaN]
    },
    //func_f32x4_max_3  
    {
        func: "func_f32x4_max_3",
        args: [Number.NaN, Number.NaN, Number.NaN, Number.NaN, 2, 2, 2, 2]
    },
    {
        func: "func_f32x4_max_3",
        args: [2, 2, 2, 2, Number.NaN, Number.NaN, Number.NaN, Number.NaN]
    },

];

for (let t of nanTests) {
    checkNaN("exports." + t.func, t.args);
}


if (passed) {
    print("Passed");
}
