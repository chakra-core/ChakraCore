//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let threw = false
try
{
    var obj1 = {};
    var func0 = function () {
    for (var _strvar2 in Object) {
        Object.prototype[_strvar2] = {};
    }
    };
    let cnt = 0;

    Object.defineProperty(obj1, 'prop0', {
    get: function () {
        print("BAD!");
    },
    configurable: true
    });

    Object.prototype.prop0 = func0();
    Object.prototype.prop2 = func0();

    Object.prop2 = Object.defineProperty(Object.prototype, 'prop2', {
    get: function () {
    }});
    (function () {
    'use strict';
    for (var _strvar0 in Object) {
        Object.prototype[_strvar0] = func0();
    }
    }());
}
catch(e)
{
    threw = true;
}

print(threw ? "Pass" : "Fail")