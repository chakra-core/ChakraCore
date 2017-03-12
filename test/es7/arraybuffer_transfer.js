//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function assertEquals(expected, actual) {
    if (expected != actual) {
        throw `Expected ${expected}, received ${actual}`;
    }
}

function arrayBufferAllocAndTransfer(initSize) {

    const UNDER_1GB = 0x3FFF0000;
    let buffers = [];
    const n = 10;

    for (let i = 0; i < n; i++) {
        try {
            const b = new Uint8Array(initSize);
            ArrayBuffer.transfer(b.buffer, UNDER_1GB);
            buffers.push(b);
        } catch (e) {
            return e;
        }
    }

    return new Error('OOM Expected');
}

assertEquals(1, WScript.Arguments.length);
const INITIAL_SIZE = parseInt(WScript.Arguments[0]);

let {name, message } = arrayBufferAllocAndTransfer(INITIAL_SIZE);
assertEquals("Out of memory", message); //message check comes first to render test failures more intuitive
assertEquals("Error", name);
print ("PASSED");
