//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
function asmModule(stdlib, imports, buffer) {
    "use asm";

    var log = stdlib.Math.log;
    var toF = stdlib.Math.fround;
    var imul = stdlib.Math.imul;

    var i4 = stdlib.SIMD.Int32x4;
    var i4store = i4.store;
    var i4load = i4.load;
    var i4swizzle = i4.swizzle;
    var i4check = i4.check;
    var i4add = i4.add;
    var i4sub = i4.sub;

    var f4 = stdlib.SIMD.Float32x4;
    var f4equal = f4.equal;
    var f4lessThan = f4.lessThan;
    var f4splat = f4.splat;
    var f4store = f4.store;
    var f4load = f4.load;
    var f4check = f4.check;
    var f4abs = f4.abs;
    var f4add = f4.add;
    var f4sub = f4.sub;

    var Float32Heap = new stdlib.Float32Array(buffer);
    var Int32Heap = new stdlib.Int32Array(buffer);
    var BLOCK_SIZE = 4;

    function matrixSubtraction(aIndex, bIndex, cIndex) {
        aIndex = aIndex | 0;
        bIndex = bIndex | 0;
        cIndex = cIndex | 0;

        var i = 0, dim1 = 0, dim2 = 0, matrixSize = 0;
        var aPiece = i4(0, 0, 0, 0), bPiece = i4(0, 0, 0, 0);

        dim1 = Int32Heap[aIndex << 2 >> 2] | 0;
        dim2 = Int32Heap[aIndex + 1 << 2 >> 2] | 0;
        matrixSize = imul(dim1, dim2);

        //array dimensions don't match
        if (((dim2 | 0) != (Int32Heap[bIndex + 1 << 2 >> 2] | 0)) | ((dim1 | 0) != (Int32Heap[bIndex << 2 >> 2] | 0))) {
            return -1;
        }

        Int32Heap[cIndex << 2 >> 2] = dim1;
        Int32Heap[cIndex + 1 << 2 >> 2] = dim2;

        while ((i|0) < (matrixSize|0)) {
            aPiece = i4load(Int32Heap, aIndex + 2 + i << 2 >> 2);
            bPiece = i4load(Int32Heap, bIndex + 2 + i << 2 >> 2);
            i4store(Int32Heap, cIndex + 2 + i << 2 >> 2, i4sub(aPiece, bPiece));

            i = (i + BLOCK_SIZE)|0;
        }

        return 0;
    }

    function new2DMatrix(startIndex, dim1, dim2) {
        startIndex = startIndex | 0;
        dim1 = dim1 | 0;
        dim2 = dim2 | 0;

        var i = 0, matrixSize = 0;
        matrixSize = imul(dim1, dim2);
        Int32Heap[startIndex << 2 >> 2] = dim1;
        Int32Heap[startIndex + 1 << 2 >> 2] = dim2;
        for (i = 0; (i|0) < ((matrixSize - BLOCK_SIZE)|0); i = (i + BLOCK_SIZE)|0) {
            i4store(Int32Heap, startIndex + 2 + i << 2 >> 2, i4((i + 1), (i + 2), (i + 3), (i + 4)));
        }
        for (; (i|0) < (matrixSize|0); i = (i + 1)|0) {
            Int32Heap[(startIndex + 2 + i) << 2 >> 2] = (i + 1) | 0;
        }
        return (startIndex + 2 + i) | 0;
    }

    return {
        new2DMatrix: new2DMatrix,
        matrixSubtraction: matrixSubtraction
    };
}

////////////////////////////////////////////////////////////////
//Call GEN_BASELINE() to generate baseline data and initialize RESULTS with it.
///////////////////////////////////////////////////////////////
function GEN_BASELINE(buffer, start) {
    var IntHeap32 = new Int32Array(buffer);
    var FloatHeap32 = new Float32Array(buffer);
    var i4;
    var dim1 = IntHeap32[start];
    var dim2 = IntHeap32[start + 1];

    print("[");
    for (var i = 0; i < Math.imul(dim1, dim2) ; i += 4) {
        i4 = SIMD.Int32x4.load(IntHeap32, i + start + 2);
        print(i4.toString()+",");
    }
    print("]");
}
function verify2DMatrix(buffer, start, results) {
    var IntHeap32 = new Int32Array(buffer);
    var FloatHeap32 = new Float32Array(buffer);
    var i4;
    var dim1 = IntHeap32[start];
    var dim2 = IntHeap32[start + 1];
    for (var i = 0, rslt_idx = 0; i < Math.imul(dim1, dim2) ; i += 4) {
        i4 = SIMD.Int32x4.load(IntHeap32, i + start + 2);
        equalSimd(results[rslt_idx++], i4, SIMD.Int32x4, "2d Matrix Addition");
    }
}


var buffer = new ArrayBuffer(16 * 1024 * 1024);
var m = asmModule(this, null, buffer);

print("2D Matrix Subtraction");
m.new2DMatrix(0, 13, 17);
m.new2DMatrix(500, 13, 17);
m.matrixSubtraction(0, 500, 1000);
m.matrixSubtraction(1000, 0, 1500);
// print2DMatrix(buffer, 1000);
// print2DMatrix(buffer, 1500);
// GEN_BASELINE(buffer, 1000);
var RESULTS =[
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
SIMD.Int32x4(0, 0, 0, 0),
];
verify2DMatrix(buffer, 1000, RESULTS);
// GEN_BASELINE(buffer, 1500);
var RESULTS =[
SIMD.Int32x4(-1, -2, -3, -4),
SIMD.Int32x4(-5, -6, -7, -8),
SIMD.Int32x4(-9, -10, -11, -12),
SIMD.Int32x4(-13, -14, -15, -16),
SIMD.Int32x4(-17, -18, -19, -20),
SIMD.Int32x4(-21, -22, -23, -24),
SIMD.Int32x4(-25, -26, -27, -28),
SIMD.Int32x4(-29, -30, -31, -32),
SIMD.Int32x4(-33, -34, -35, -36),
SIMD.Int32x4(-37, -38, -39, -40),
SIMD.Int32x4(-41, -42, -43, -44),
SIMD.Int32x4(-45, -46, -47, -48),
SIMD.Int32x4(-49, -50, -51, -52),
SIMD.Int32x4(-53, -54, -55, -56),
SIMD.Int32x4(-57, -58, -59, -60),
SIMD.Int32x4(-61, -62, -63, -64),
SIMD.Int32x4(-65, -66, -67, -68),
SIMD.Int32x4(-69, -70, -71, -72),
SIMD.Int32x4(-73, -74, -75, -76),
SIMD.Int32x4(-77, -78, -79, -80),
SIMD.Int32x4(-81, -82, -83, -84),
SIMD.Int32x4(-85, -86, -87, -88),
SIMD.Int32x4(-89, -90, -91, -92),
SIMD.Int32x4(-93, -94, -95, -96),
SIMD.Int32x4(-97, -98, -99, -100),
SIMD.Int32x4(-101, -102, -103, -104),
SIMD.Int32x4(-105, -106, -107, -108),
SIMD.Int32x4(-109, -110, -111, -112),
SIMD.Int32x4(-113, -114, -115, -116),
SIMD.Int32x4(-117, -118, -119, -120),
SIMD.Int32x4(-121, -122, -123, -124),
SIMD.Int32x4(-125, -126, -127, -128),
SIMD.Int32x4(-129, -130, -131, -132),
SIMD.Int32x4(-133, -134, -135, -136),
SIMD.Int32x4(-137, -138, -139, -140),
SIMD.Int32x4(-141, -142, -143, -144),
SIMD.Int32x4(-145, -146, -147, -148),
SIMD.Int32x4(-149, -150, -151, -152),
SIMD.Int32x4(-153, -154, -155, -156),
SIMD.Int32x4(-157, -158, -159, -160),
SIMD.Int32x4(-161, -162, -163, -164),
SIMD.Int32x4(-165, -166, -167, -168),
SIMD.Int32x4(-169, -170, -171, -172),
SIMD.Int32x4(-173, -174, -175, -176),
SIMD.Int32x4(-177, -178, -179, -180),
SIMD.Int32x4(-181, -182, -183, -184),
SIMD.Int32x4(-185, -186, -187, -188),
SIMD.Int32x4(-189, -190, -191, -192),
SIMD.Int32x4(-193, -194, -195, -196),
SIMD.Int32x4(-197, -198, -199, -200),
SIMD.Int32x4(-201, -202, -203, -204),
SIMD.Int32x4(-205, -206, -207, -208),
SIMD.Int32x4(-209, -210, -211, -212),
SIMD.Int32x4(-213, -214, -215, -216),
SIMD.Int32x4(-217, -218, -219, -220),
SIMD.Int32x4(-221, 0, 0, 0),
];
verify2DMatrix(buffer, 1500, RESULTS);
print("PASS");
