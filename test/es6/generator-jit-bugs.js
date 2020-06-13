//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test 1 - const that is unused/replaced with undefined
function* foo() {
    const temp2 = null;
    while (true) {
        yield temp2;
    }
}

const gen = foo();

gen.next();
gen.next();
gen.next();

// Test 2 - bail on no profile results in incorrect resume point
function* bar() {
    for (let i = 0; i < 3; ++i) {
        yield i;
    }
}

const gen2 = bar();
gen2.next();
if (gen2.next().value != 1) { throw new Error("Generator failed to increment loop control 1");}
if (gen2.next().value != 2) { throw new Error("Generator failed to increment loop control 2");}

print("pass");
