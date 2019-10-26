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

print("Pass");
