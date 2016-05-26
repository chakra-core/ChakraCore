//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo1(a, b) {
    if (b != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval('b') != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    var b = 1;
    if (b != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo1(undefined, 10);

function foo2(a, b = 10) {
    if (b != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval('b') != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    var b = 1;
    if (b != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo2();

function foo3(a = 10, b = function () { return a; }) {
    if (b() != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval('b()') != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    var a = 1;
    if (b() != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo3();
