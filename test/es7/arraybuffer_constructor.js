//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function assertEquals(expected, actual) {
    if (expected != actual) {
        throw `Expected ${expected}, received ${actual}`;
    }
}

function arrayBufferAlloc() {

    const UNDER_1GB = 0x3FFF0000;
    let buffers = [];
    const n = 10;

    for (let i = 0; i < n; i++) {
        try {
            const b = new ArrayBuffer(UNDER_1GB);
            buffers.push(b);
        } catch (e) {
            return e;
        }
    }

    return new Error('OOM Expected');
}

let {name, message } = arrayBufferAlloc();
assertEquals("Invalid function argument", message); //message check comes first to render test failures more intuitive
assertEquals("RangeError", name);
print ("PASSED");
