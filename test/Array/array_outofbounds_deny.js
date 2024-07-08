//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.Echo("*** Running test #1 (0): Deny out-of-bounds write");
try {
    let arr = new Array(2);
    WScript.Echo("arr.length is " + arr.length);

    arr[0] = 0;
    WScript.Echo("arr[0] is " + arr[0]);

    arr[1] = 1;
    WScript.Echo("arr[1] is " + arr[1]);

    arr[100] = 100;
    WScript.Echo("FAILED");
} catch (e) {
    WScript.Echo(e.name + ": " + e.message);
    WScript.Echo("PASSED");
}


WScript.Echo("*** Running test #2 (1): Deny out-of-bounds read");
try {
    let arr = new Array(2);
    WScript.Echo("arr.length is " + arr.length);

    arr[0] = 0;
    WScript.Echo("arr[0] is " + arr[0]);

    arr[1] = 1;
    WScript.Echo("arr[1] is " + arr[1]);

    WScript.Echo("arr[100] is " + arr[100]);
    WScript.Echo("FAILED");
} catch (e) {
    WScript.Echo(e.name + ": " + e.message);
    WScript.Echo("PASSED");
}
