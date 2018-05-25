//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    for(abcd of [])
    {
        try {
        } catch(e) {
        }
        try {
        } catch(e) {
        }
    }
}

test0();
test0();
test0();

console.log("PASSED");
