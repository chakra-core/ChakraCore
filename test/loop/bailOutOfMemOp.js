//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f0() {
    var printArr = [];
    Object.prototype.m = {};
    Object.defineProperty(Array.prototype, "5", {writable : true});

    for (var iterator = 0; iterator < 10; iterator++) {
        var arr0 = [];
        arr0[10] = "Should not see this";
        arr0.shift();
        for (var arr0Elem in arr0) {
            if (arr0Elem.indexOf('m')) {
                continue;
            }
            for (var i = 9.1 | 0; i < arr0.length; i++) {
                arr0[i] = "";
            }
            printArr.push(arr0);
        }
    }
    WScript.Echo(printArr);
}
f0();
f0();

function f1() {
    var printArr = [];
    var arr0 = new Array(1, 1);
    var arr1 = [];
    arr0[3] = 1;
    arr0[2] = 1;
    arr1[1] = 1;
    arr1[3] = -1;
    arr1[2] = 1;
    for (var i = 0.1 ? 1 : -1; i < arr0.length; i++) {
        arr0[i] = arr1[i];
    }
    printArr.push(arr0);
    i | 0;
    WScript.Echo(printArr);
}
f1();
f1();