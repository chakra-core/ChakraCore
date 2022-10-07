//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check

function AssertNaN(value) {
    if (!isNaN(value))
        throw new Error("Expected NaN as value");
}

let valueOfCounter = 0;
const obj = {
    valueOf: function () {
        valueOfCounter++;
        return 1;
    }
};

AssertNaN(Math.max(NaN, obj));
AssertNaN(Math.min(NaN, obj));

const expectedCount = 2;
if (valueOfCounter != expectedCount)
    throw new Error(`Expected "valueOf" to be called ${expectedCount}x; got ${valueOfCounter}`);

console.log("pass");
