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
    if (a != 10) {
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
    if (a != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo3();

function foo4(a = 10, b = function () { return a; }) {
    if (b() != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (a != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    a = 1;
    if (a != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (b() != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo4();

function foo5(a = 10, b = function () { return a; }) {
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
    if (a != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo5();

function foo6(a = 10, b = function () { return a; }) {
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
    a = 1;
    if (b() != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval('a') != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo6();

function foo7(a, b = function () { a; }) {
    if (eval("a") !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (arguments[0] !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval("arguments[0]") !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    arguments = 100;
    if (eval("a") !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval("arguments") !== 100) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (arguments !== 100) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo7(1);

function foo8(a = 10, b = eval("a")) {
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
    if (a != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    var a = 1;
    if (b != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (a != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo8();

function foo9(a = 10, b = () => eval("a")) {
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
    if (a != 10) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    a = 1;
    if (b() != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (a != 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo9();

function foo10(a, b = () => eval("a")) {
    if (eval("a") !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (arguments[0] !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval("arguments[0]") !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    arguments = 100;
    if (eval("a") !== 1) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (eval("arguments") !== 100) {
        print("FAILED")
    } else {
        print("PASSED");
    }
    if (arguments !== 100) {
        print("FAILED")
    } else {
        print("PASSED");
    }
}
foo10(1);