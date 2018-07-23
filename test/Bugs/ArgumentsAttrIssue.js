//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f1(...args) {
    eval("var arguments = 1;");
}
f1();
f1();
f1(); // Shouldn't throw.
print("PASSED");