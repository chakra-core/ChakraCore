//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f(a) {
    return (function () {
        return { a };
    });
}

var o = f()();
if (o.a !== undefined) {
    WScript.Echo('fail 1: o.a === ' + o.a);
}
o = f('a')();
if (o.a !== 'a') {
    WScript.Echo('fail 2: o.a === ' + o.a);
}
WScript.Echo('pass');
