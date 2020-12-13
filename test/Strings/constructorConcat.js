//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
function f(arg) {
    var i = 0;
    while (i < 5) {
        i++;
        arg += "this_should_not_stay";
    }
}
f("Hello");
f(Int8Array);

if (!Int8Array.toString().includes("this_should_not_stay")) {
    WScript.Echo("Passed");
}
else {
    WScript.Echo("FAILED");
}
