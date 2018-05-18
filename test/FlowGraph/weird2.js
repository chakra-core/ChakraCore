//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var shouldBailout = false;
function test0() {
    var loopInvariant = 4;
    var obj1 = {};
    var i16 = new Int16Array();
    function _array2iterate(_array2tmp) {
        for (var _array2i in _array2tmp) {
            if (_array2i.indexOf('') == 0) {
            }
            var __loopvar1 = loopInvariant - 6;
            for (; obj1.prop0 < ~(shouldBailout ? (Object.defineProperty(obj1, '', {
                get: function () {
                },
                configurable: true
            }), obj1.prop0) : obj1.prop0); obj1++) {
                5;
                if (obj1) {
                }
            }
            _array2iterate(_array2tmp[_array2i]);
            obj1.prop0 = {
                prop0: obj1.prop0 >> 'caller',
                prop1: i16[53 & 255],
                prop2: obj1.prop0 >> '',
                prop3: new RegExp('xyz') instanceof (typeof Function == 'function' && Function[Symbol.toStringTag] == 'AsyncFunction' ? Function : Object),
                prop4: obj1[shouldBailout ? obj1[8] = 'x' : undefined, 8]
            };
        }
    }
    _array2iterate([
        [],
        []
    ]);
}
test0();
test0();
test0();

console.log("PASSED");
