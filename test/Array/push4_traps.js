//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var arr = [], arr2 = [];

let setCount = 0;
let getCount = 0;

function check(actual, expected)
{
    if (actual !== expected)
    {
        throw new Error("Wrong value actual: " + actual + " expected: " + expected);
    }
}

// Test push traps with int based accessors on prototype
Object.defineProperty(Array.prototype, '0', {
    get: function() {
        ++getCount;
        return 30;
    },
    set: function() {
        ++setCount;
        return 60;
    },
    configurable: true
});

arr.push(1);
check(arr.length, 1);
check(setCount, 1);
check(getCount, 0);
check(JSON.stringify(arr), '[30]')
check(setCount, 1);
check(getCount, 1);
check(arr[0], 30);
check(getCount, 2);
check(arr2[0], 30);
check(arr2.length, 0);

// Test push traps with float based accessors on prototype
Object.defineProperty(Array.prototype, '0', {
    get: function() {
        ++getCount;
        return 30.5;
    },
    set: function() {
        ++setCount;
        return 60.3;
    },
    configurable: true
});

arr.push(1);
arr2.push(0.5);
check(arr.length, 2);
check(arr2.length, 1);
check(setCount, 2);
check(getCount, 3);
check(JSON.stringify(arr), '[30.5,1]');
check(JSON.stringify(arr2), '[30.5]');
check(setCount, 2);
check(getCount, 5);
check(arr[0], 30.5);
check(arr[1], 1);
check(arr2[0], 30.5);
check(getCount, 7);

print("pass");
