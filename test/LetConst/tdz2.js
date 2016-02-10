//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test TDZ behavior when writing to let declared in switch and closure-captured.
function test0() {
    switch (x) {
        case 1:
            let inner;
            inner = 2;
            function f() { if (inner !== 2) WScript.Echo('fail'); }
            f();
            break;
        case 2:
        case 0:
            try {
                inner = 1;
            }
            catch (e) {
                break;
            }
            WScript.Echo('fail');
    }
}
var x = 0;
test0();
x = 1;
test0();

function test1() {
    switch (x) {
        case 1:
            let inner;
            inner = 2;
            function f() { if (eval('inner') !== 2) WScript.Echo('fail'); }
            f();
            break;
        case 2:
        case 0:
            try {
                inner = 1;
            }
            catch (e) {
                break;
            }
            WScript.Echo('fail');
    }
}
var x = 0;
test1();
x = 1;
test1();

WScript.Echo('pass');
