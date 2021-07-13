//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function obj() { return this; }

var z = obj();
var o = new Object();
var a = [11,12,13];

a[o] = 100;
a.x  = 200;
o.x  = 300;
a.some = undefined;


const cases = [
    [o, "x", true],
    [o, "y", false],
    [o, "", false],
    [o, , false],

    [a, 0, true],
    [a, 1, true],
    [a, 2, true],
    [a, 3, false],
    [a, "0", true],
    [a, "1", true],
    [a, "2", true],
    [a, "3", false],
    [a, "x", true],
    [a, "some", true],
    [a, "y", false],
    [a, "", false],
    [a, "length", true],
    [a, , false],
    [a, o, true],
    [a, "o", false],
    [a, "[object Object]", true],

    [z, , true],
    [z, undefined, true]
];

function assert (result, message) {
    if (!result) {
        throw new Error (message);
    }
}


for (let i = 0; i < cases.length; ++i) {
    let test = cases[i];
    assert (test[0].hasOwnProperty(test[1]) === test[2], `${test[0]}.hasOwnProperty(${test[1]}) should equal ${test[2]}`);
    assert (Object.hasOwn(test[0], test[1]) === test[2], `Object.hasOwn(${test[0]}, ${test[1]}) should equal ${test[2]}`);
}

assert(Object.hasOwn.length === 2, "Object.hasOwn.length should equal 2");
assert(Object.prototype.hasOwnProperty.length === 1, "Object.prototype.hasOwnProperty.length should equal 1");
assert(!Object.prototype.hasOwn, "Object.prototype.hasOwn should not exist");

print("pass");
