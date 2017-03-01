//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    var obj1 = {};
    var func0 = function () {
        if (true) {
            function func5() {
                return function () {
                    (function () {
                        func5;
                    });
                }();
            }
            with ({}) {
                argMath2 = func5.call(obj1);
            }
        }
    };
    func0();
}
test0();
WScript.Echo('pass');
