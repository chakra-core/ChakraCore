//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = [];
function f() {
    try {
        f();
    } catch (e) {
        a.foo = 100;
    }
}
f();

print("pass");