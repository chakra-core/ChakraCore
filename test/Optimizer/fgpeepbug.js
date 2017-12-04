//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo() {
    var a = {b: {}}
    a = null !== a.b && 0 < a.b.a
    if (!a) return a
}
foo();
foo();
foo();

print("Pass")
