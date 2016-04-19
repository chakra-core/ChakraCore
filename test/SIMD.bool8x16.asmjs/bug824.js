//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//https://github.com/Microsoft/ChakraCore/issues/824
function asmModule(stdlib, imports) {
    "use asm";

    var b16 = stdlib.SIMD.Bool8x16;
    var b16extractLane = b16.extractLane;

 function testExtractLane() {

        var b16const = b16(1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0);
        return b16extractLane(b16const,6)|0; 
    }

    return {testExtractLane:testExtractLane};
}

var m = asmModule(this, {});

print(m.testExtractLane());
print(m.testExtractLane());
print(m.testExtractLane());
print(m.testExtractLane());
print(m.testExtractLane());


print("PASS");