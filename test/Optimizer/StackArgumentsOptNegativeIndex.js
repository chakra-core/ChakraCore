//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let fail = 0;

// Test 1 inline negative index
function func1() {
    return func2(5);
}

function func2() {
    return arguments[-1];
}

const obj = { "o" : func2 };

for (i = 0; i < 500; i++) {
    if(func1() != undefined) {
        ++ fail;
    }
}

// Test 2 edited negative index
function func3() {
    return func4(5);
}

function func4() {
    arguments[-1] = 5;
    return arguments[-1];
}

for (i = 0; i < 500; i++) {
    if(func3() != 5) {
        ++ fail;
    }
}

// Test 3 edit the negative index after jitting
function func5(count) {
    if (count > 10) {
        arguments[-2] = 5;
    }
    return arguments[-2];
}

for (let i = 0; i < 20; ++i) {
    let val = func5(i);
    if (i > 10 && val !== 5 || i <= 10 && val !== undefined) {
        ++fail;
    }
}

function func6(count) {
    return func5(count);
}

for (let i = 0; i < 20; ++i) {
    let val = func6(i);
    if (i > 10 && val !== 5 || i <= 10 && val !== undefined) {
        ++fail;
    }
}

// Test 4 dynamic negative index
function func7(count) {
    let n = 0;
    if (count > 10) {
        n = -1;
    }
    if (count > 20) {
        n = 1;
    }
    return arguments[n];
}

function func8(count) {
    return func7(count);
}

for (let i = 0; i < 40; ++i) {
    let val = func7(i);
    if (i < 11 && val !== i || i > 10 && val !== undefined) {
        ++fail;
    }
}

for (let i = 0; i < 40; ++i) {
    let val = func8(i);
    if (i < 11 && val !== i || i > 10 && val !== undefined) {
        ++fail;
    }
}


print (fail > 0 ? "fail" : "pass");
