//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f1() {
    var o1 = {},o2 = {};
    var proto1 = {a:'a',b:'b'},proto2 = {a:'a'};
    o1.__proto__ = proto1;
    o2.__proto__ = proto2;

    function a(o) { return o.a; }
    function b(o) { return o.b; }

    a(o1);
    a(o2);
    b(o1);
    b(o2);
    proto2.__proto__ = {b:'b'};
    if (b(o2) !== 'b') {
        WScript.Echo('fail');
    }
}

f1()
f1();

function f2() {
    var o1 = {b:'b'},o2 = {b:'b'};
    var proto1 = {a:'a',b:'b'},proto2 = {a:'a'};
    o1.__proto__ = proto1;
    o2.__proto__ = proto2;

    function a(o) { return o.a; }
    function b(o) { return o.b; }

    a(o1);
    a(o2);

    delete o1.b;
    delete o2.b;

    b(o1);
    b(o2);
    proto2.__proto__ = {b:'b'};
    if (b(o2) !== 'b') {
        WScript.Echo('fail');
    }
}

f2();
f2();

WScript.Echo('pass');
