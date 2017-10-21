//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
let passed = true;

function assertEquals(expected, actual) {
    if (expected != actual) {
        passed = false;
        throw `Expected ${expected}, received ${actual}`;
    }
}

const INITIAL_SIZE = 1;
const memObj = new WebAssembly.Memory({initial:INITIAL_SIZE});
const module = new WebAssembly.Module(readbuffer('math.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;
const arrays = {
    "i32x4" : new Int32Array (memObj.buffer),
    "i16x8" : new Int16Array (memObj.buffer),
    "i8x16" : new Int8Array (memObj.buffer),
    "f32x4" : new Float32Array (memObj.buffer),
    "f64x2" : new Float64Array (memObj.buffer),
    "i64x2" : new Int32Array (memObj.buffer)
};

function moveArgsIntoArray(args, offset, arr) {
    for (let i = 0; i < args.length; i++) {
        arr[offset + i] = args[i];
    }
}

let testCompOps = function (funcname, args1, args2, op, resultArr) {

    const len = args1.length;
    const arr = arrays[funcname.split('_')[1]];

    moveArgsIntoArray(args1, 0, arr);
    moveArgsIntoArray(args2, len, arr);
    instance[funcname](op);
    for (let i = 0; i < len; i++) {
        assertEquals(resultArr[i], Number.isNaN(arr[i]) || !!arr[i]);
    }
}

let testMathOps = function (funcname, args1, args2, resultArr) {

    const len = args1.length;
    const type = funcname.split('_')[1];
    const arr = arrays[type];

    moveArgsIntoArray(args1, 0, arr);
    if (Array.isArray(args2)) { //binary ops
        moveArgsIntoArray(args2, len, arr);
        instance[funcname]();
    }
    else if (Number.isInteger(args2)) {
        instance[funcname](args2); //shift amount for shl/shr
    }
    else {
        instance[funcname](); //unary ops
    }

    for (let i = 0; i < resultArr.length; i++) {
        if ((type === "f32x4" || type === "f64x2") && Number.isNaN(resultArr[i])) {
            assertEquals(true, Number.isNaN(arr[i]));
        } else {
            assertEquals(resultArr[i], arr[i]);
        }

    }
}

let reverse64x2Type = (type) => type === "f64x2" ? "i64x2" : "f64x2";
let testTruncConvOps = function (funcname, args1, resultArr) {

    const len = args1.length;
    const type = funcname.split('_')[1];
    const resultType = reverse64x2Type(type);
    const arr = arrays[resultType];
    const resultArrView = arrays[type];

    moveArgsIntoArray(args1, 0, arr);
    instance[funcname]();

    for (let i = 0; i < resultArr.length; i++) {
        if (type === "f64x2" && Number.isNaN(resultArr[i])) {
            assertEquals(true, Number.isNaN(resultArrView[i]));
        } else {
            assertEquals(resultArr[i], resultArrView[i]);
        }

    }
}

//i32x4
testMathOps("func_i32x4_add",
    [2147483645 , 0, -1, 65536],
    [3          , 0, 1 , 65536],
    [-2147483648, 0, 0 , 131072]
);

testMathOps("func_i32x4_sub",
    [-2147483648, 0 , 65536, 32768],
    [1          , -1, 65536, 65536],
    [2147483647 , 1 , 0    , -32768]
);

testMathOps("func_i32x4_mul",
    [65536, 65536 , 0         , 1],
    [65536, 65535 , 2147483647, -2147483648],
    [0    , -65536, 0         , -2147483648]
);

testMathOps("func_i32x4_shl", [1,2,1,0], 30, [1073741824, -2147483648, 1073741824, 0]);
testMathOps("func_i32x4_shl", [1,2,8,16], 32, [1,2,8,16]);

testMathOps("func_i32x4_shr_s", [-2147483648,0x80010000,0x10000,1], 16, [-32768,-32767,1,0]);
testMathOps("func_i32x4_shr_u", [-2147483648,0x80010000,0x10000,1], 16, [32768,32769,1,0]);

//i16x8
testMathOps("func_i16x8_add",
    [32767 , -32768, -1, 16, 32767 , -32768, 0, 0],
    [1     , -1    , 1 , 16, 2     , -2    , 0, 32767],
    [-32768, 32767 , 0 , 32, -32767, 32766 , 0, 32767]
);

testMathOps("func_i16x8_addsaturate_s",
    [32767, 32767, -1    , -32768, 32767, -32768, 0    , 0],
    [1    , 32767, -32768, -32768, -2   , 0     , 32767, -1],
    [32767, 32767, -32768, -32768, 32765, -32768, 32767, -1]
);

testMathOps("func_i16x8_subsaturate_s",
    [32767, 32767 , -2    , -32768, 32767, -32768, 0     , 0],
    [-1   , -32767, 32767 ,  32767, 2    , 0     , -32767, -1],
    [32767, 32767 , -32768, -32768, 32765, -32768, 32767 , 1]
);

testMathOps("func_i16x8_addsaturate_u",
    [65535, 65535, 65535, 32768, 32767 , 0, 0     , 32768],
    [1    , 2    , 0    , 32768, 2     , 0, 32768 , 1],
    [-1   , -1   , -1   , -1   , -32767, 0, -32768, -32767]
);

testMathOps("func_i16x8_subsaturate_u",
    [0    , 65535, 65535, 32768, 32767, 0, 65535, 65535],
    [65535, 2    , 0    , 32768, 32768, 0, 32768, 32767],
    [0    , -3   , -1   , 0    ,     0, 0, 32767, -32768]
);

testMathOps("func_i16x8_sub",
    [-1   , 32767 , 1, 16, 32767 , -32768, 0, 0],
    [32768,-1     , 1, 16, -2    , 2     , 0, 32767],
    [32767,-32768 , 0, 0 , -32767, 32766 , 0, -32767]
);

testMathOps("func_i16x8_mul",
    [256, 128  , 1     , -32768, -32768, -32768, -128 , 0],
    [256, 256  , -32767, 0     , -32768, 2     , -128 , 0],
    [0  ,-32768, -32767, 0     , 0     , 0     , 16384, 0]
);

testMathOps("func_i16x8_shl", [1, 2, 3, 0, 4, 5, 6, 7], 15, [-32768, 0, -32768, 0, 0, -32768, 0, -32768]);
testMathOps("func_i16x8_shl", [1, 2, 3, 0, 2048, 128, 256, 512], 4, [16, 32, 48, 0, -32768, 2048, 4096, 8192]);

testMathOps("func_i16x8_shr_s", [0x8000, 0x8100, 256, 1, 0x8000, 0x8101, 257, 0], 8, [-128, -127, 1, 0, -128, -127, 1, 0]);
testMathOps("func_i16x8_shr_u", [0x8000, 0x8100, 256, 1, 0x8000, 0x8101, 257, 0], 8, [128, 129, 1, 0, 128, 129, 1, 0]);

//i8x16
testMathOps("func_i8x16_add",
    [127 , -128, -1, 16, 127 , -128, 0, 0  , 127 , -128, -1, 16, 127 , -128, 0, 0],
    [1   , -1  , 1 , 16, 2   , -2  , 0, 127, 1   , -1  , 1 , 16, 2   , -2  , 0, 127],
    [-128, 127 , 0 , 32, -127, 126 , 0, 127, -128, 127 , 0 , 32, -127, 126 , 0, 127]
);

testMathOps("func_i8x16_addsaturate_s",
    [127, 127, -1  , -128, 127, -128, 0  , 0 , 127, 127, -1  , -128, 127, -128, 0  , 0 ],
    [1  , 127, -128, -128, -2 , 0   , 127, -1, 1  , 127, -128, -128, -2 , 0   , 127, -1],
    [127, 127, -128, -128, 125, -128, 127, -1, 127, 127, -128, -128, 125, -128, 127, -1]
);

testMathOps("func_i8x16_subsaturate_s",
    [127, 127 , -2  , -128, 127, -128, 0   , 0 , 127, 127 , -2  , -128, 127, -128, 0   , 0],
    [-1 , -127, 127 ,  127, 2  , 0   , -127, -1, -1 , -127, 127 ,  127, 2  , 0   , -127, -1],
    [127, 127 , -128, -128, 125, -128, 127 , 1 , 127, 127 , -128, -128, 125, -128, 127 , 1]
);

testMathOps("func_i8x16_addsaturate_u",
    [255, 255, 255, 128, 127 , 0, 0   , 128 , 255, 255, 255, 128, 127 , 0, 0   , 128 ],
    [1  , 2  , 0  , 128, 2   , 0, 128 , 1   , 1  , 2  , 0  , 128, 2   , 0, 128 , 1],
    [-1 , -1 , -1 , -1 , -127, 0, -128, -127, -1 , -1 , -1 , -1 , -127, 0, -128, -127]
);

testMathOps("func_i8x16_subsaturate_u",
    [0  , 255, 255, 128, 127, 0, 255, 255 , 0  , 255, 255, 128, 127, 0, 255, 255],
    [255, 2  , 0  , 128, 128, 0, 128, 127 , 255, 2  , 0  , 128, 128, 0, 128, 127],
    [0  , -3 , -1 , 0  , 0  , 0, 127, -128, 0  , -3 , -1 , 0  , 0  , 0, 127, -128]
);

testMathOps("func_i8x16_sub",
    [-1   , 127   , 1, 16, 127   , -128  , 0, 0   , -1   , 127   , 1, 16, 127   , -128  , 0, 0],
    [128  ,-1     , 1, 16, -2    , 2     , 0, 127 , 128  ,-1     , 1, 16, -2    , 2     , 0, 127],
    [127  ,-128   , 0, 0 , -127  , 126   , 0, -127, 127  ,-128   , 0, 0 , -127  , 126   , 0, -127]
);

testMathOps("func_i8x16_mul",
    [16, 8  , 1   , -128, -128, -128, -8 , 12 , 16, 8  , 1   , -128, -128, -128, -8 , 12],
    [16, 16 , -127, 0   , -128, 2   , -8 , 10 , 16, 16 , -127, 0   , -128, 2   , -8 , 10],
    [0 ,-128, -127, 0   , 0   , 0   , 64 , 120, 0 ,-128, -127, 0   , 0   , 0   , 64 , 120]
);

testMathOps("func_i8x16_shl", [1, 2, 3, 0, 4, 5, 6, 7, 1, 2, 3, 0, 4, 5, 6, 16], 4, [16, 32, 48, 0, 64, 80, 96, 112, 16, 32, 48, 0, 64, 80, 96, 0]);
testMathOps("func_i8x16_shr_s", [-128, -127, 16, 1, -128, -127, 16, -64, -128, -127, 16, 1, -128, -127, 16, 1], 4, [-8, -8, 1, 0, -8, -8, 1, -4, -8, -8, 1, 0, -8, -8, 1, 0]);
testMathOps("func_i8x16_shr_u", [128, 192, 16, 1, 128, 193, 17, 0, 128, 192, 16, 1, 128, 193, 17, 0], 4, [8, 12, 1, 0, 8, 12, 1, 0, 8, 12, 1, 0, 8, 12, 1, 0]);

//f32x4
testMathOps("func_f32x4_add",
    [400000512, 400000256, 1.4E-45              , -1],
    [400000505, 400000100, 1.4E-45              , Number.NaN],
    [800001024, 800000384, 2.802596928649634e-45, Number.NaN]
);

testMathOps("func_f32x4_sub",
    [800001024, 800000256, 2.802596928649634e-45, 1],
    [400000512, 400000128, 1.4E-45              , Number.NaN],
    [400000512, 400000128, 1.401298464324817e-45, Number.NaN]
);

testMathOps("func_f32x4_mul",
    [400000512         , 800000256         , 2.802596928649634e-45, 1],
    [400000512         , 400000128         , 1.4E-45              , Number.NaN],
    [160000416677888000, 320000214880485400, 0                    , Number.NaN]
);

testMathOps("func_f32x4_div",
    [400000512, 800000256, 2.802596928649634e-45, 1],
    [400000512, 400000128, 1.4E-45              , Number.NaN],
    [1        , 2        , 2                    , Number.NaN]
);

testMathOps("func_f32x4_abs",
    [-800001024, 0, 2.802596928649634e-45, -1.401298464324817e-45],
    null,
    [800001024, 0 , 2.802596928649634e-45, 1.401298464324817e-45]
);

testMathOps("func_f32x4_sqrt",
    [1 << 20, 0, 6.25, -1],
    null,
    [1 << 10, 0, 2.5 , Number.NaN]
);

//f64x2
testMathOps("func_f64x2_add",
    [1.34826985114673713038100984656E308, 2.7670146896890036224E19],
    [1.34826985114673693079697889309E308, 1.288491622400006103515625E10],
    [Number.POSITIVE_INFINITY           , 27670146909774954000]
);

testMathOps("func_f64x2_add",
    [1.4E-45              , -1],
    [1.4E-45              , Number.NaN],
    [2.8e-45, Number.NaN]
);

testMathOps("func_f64x2_add",
    [400000512, 400000256],
    [400000505, 400000100],
    [800001017, 800000356]
);

testMathOps("func_f64x2_sub",
    [1.34826985114673713038100984656E308, 2.7670146896890036224E19],
    [1.34826985114673693079697889309E308, 1.288491622400006103515625E10],
    [1.99584030953472e+292              , 27670146884005120000]
);

testMathOps("func_f64x2_sub",
    [2.802596928649634e-45, 1],
    [1.4E-45              , Number.NaN],
    [1.402596928649634e-45, Number.NaN]
);

testMathOps("func_f64x2_sub",
    [800001024, 800000256],
    [400000512, 400000128],
    [400000512, 400000128]
);

testMathOps("func_f64x2_mul",
    [1.34826985114673713038100984656E308, 2.7670146896890036224E19],
    [1.34826985114673693079697889309E308, 1.288491622400006103515625E10],
    [Number.POSITIVE_INFINITY           , 3.565275246722034e+29]
);

testMathOps("func_f64x2_mul",
    [400000512         , 800000256         ],
    [400000512         , 400000128         ],
    [160000409600262140, 320000204800032800]
);

testMathOps("func_f64x2_div",
    [-4.27214248753826131799357622933E-306, 1],
    [3                                    , Number.NaN],
    [-1.424047495846087e-306              , Number.NaN]
);

testMathOps("func_f64x2_div",
    [400000512, 800000256],
    [400000512, 400000128],
    [1        , 2        ]
);

testMathOps("func_f64x2_abs",
    [-800001024, -4.27214248753826131799357622933E-306],
    null,
    [800001024 , 4.27214248753826131799357622933E-306]
);

testMathOps("func_f64x2_abs",
    [-Math.pow(120), -0],
    null,
    [Math.pow(120) , 0]
);

testMathOps("func_f64x2_sqrt",
    [Math.pow(2, 40), 0],
    null,
    [Math.pow(2, 20), 0]
);

testMathOps("func_f64x2_sqrt",
    [6.25, -1],
    null,
    [2.5 , Number.NaN]
);

//bitselect
testMathOps("func_i32x4_bitselect",
    [0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0, 0, 0, 0,
    0xF0F0F0F0, 0xFFFFFFFF, 0xD6D6D6D6, 0xAAAAAAAA],
    null,
    [0xF0F0F0F0|0, 0xFFFFFFFF|0, 0xD6D6D6D6|0, 0xAAAAAAAA|0]
);

testMathOps("func_i32x4_bitselect",
    [0, 0, 0, 0,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    ~0xF0F0F0F0, ~0xFFFFFFFF, ~0xD6D6D6D6, ~0xAAAAAAAA],
    null,
    [0xF0F0F0F0|0, 0xFFFFFFFF|0, 0xD6D6D6D6|0, 0xAAAAAAAA|0]
);

testMathOps("func_i32x4_bitselect",
    [0xBEBEBEBE, 0xD7D7D7D7, 0xF3F3F3F3, 0xFFFFFFFF,
    0x55555555, 0x29292929, 0x0F0F0F0F, 0,
    0xAAAAAAAA, 0xD6D6D6D6,0xF0F0F0F0, 0],
    null,
    [-1,-1,-1,0]
);

//shuffle
testMathOps("func_i8x16_shuffle_test1",
    [0  ,  1 , 2  , 3  , 4  , 5  , 6  , 7  , 8  , 9  , 10 , 11 , 12 , 13 , 14 , 15],
    [16 , 17 , 18 , 19 , 20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 , 30 , 31],
    [16 , 17 , 18 , 19 , 20 , 21 , 22 , 23 , 0  , 1  , 2  , 3  , 4  , 5  , 6  , 7]
);

testMathOps("func_i8x16_shuffle_test0",
    [0  ,  1 , 2  , 3  , 4  , 5  , 6  , 7  , 8  , 9  , 10 , 11 , 12 , 13 , 14 , 15],
    [16 , 17 , 18 , 19 , 20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 , 30 , 31],
    [31 , 30 , 29 , 28 , 1  , 17 , 2  , 19 , 3  , 4  , 5  , 6  , 21 , 20 , 11 , 10]
);

testMathOps("func_i8x16_shuffle_test2",
    [0  ,  1 , 2  , 3  , 4  , 5  , 6  , 7  , 8  , 9  , 10 , 11 , 12 , 13 , 14 , 15],
    [16 , 17 , 18 , 19 , 20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 , 30 , 31],
    [0  , 17 , 1  , 18 , 2  , 19 , 3  , 20 , 4  , 21 , 5  , 22 , 6  , 23  , 7  , 24]
);

testTruncConvOps("func_f64x2_convert_u",
    [0xffffffff|0, 0xffffffff|0, 0, 0],
    [1.8446744073709552e+19, 0]
);

testTruncConvOps("func_f64x2_convert_s",
    [0xffffffff|0, 0x7fffffff|0, 0x0|0, 0x80000000|0],
    [9.2233720368547758e+18, -9.2233720368547758e+18]
);

testTruncConvOps("func_i64x2_trunc_u",
    [1.8446744073709553e+19, -1],
    [0xffffffff|0, 0xffffffff|0, 0, 0]
);

testTruncConvOps("func_i64x2_trunc_u",
    [12345.6, Number.NaN],
    [12345, 0, 0, 0]
);

testTruncConvOps("func_i64x2_trunc_s",
    [9.2233720368547759e+18, -9.2233720368547759e+18],
    [0xffffffff|0, 0x7fffffff|0, 0x0|0, 0x80000000|0]
);

testTruncConvOps("func_i64x2_trunc_s",
    [-12345.6, Number.NaN],
    [-12345, -1, 0, 0]
);

if (passed) {
    print("Passed");
}
