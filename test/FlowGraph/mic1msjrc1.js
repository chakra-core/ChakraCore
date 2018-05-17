//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    while (undefined) {
        if (undefined) {
            continue;
        }
        try {
            continue;
        } catch (ex) {
        } finally {
            break;
        }
    }
}
test0();
test0();
test0();


function test1() {
    try {
        LABEL2:
        while (h != d) {
            try {
                continue;
            } catch (ex) {
                continue;
            } finally {
                return -1839801917;
            }
        }
    } catch (ex) {
    }
}
test1();
test1();
test1();


var _oo1obj = undefined;
function test2() {
    var _oo1obj = function () {
        var _oo1obj = {
            prop1 : []
        };
        for (; ([])[1]; ) {
        }
        _oo1obj.f1 = undefined;
    }();
}
test2();
test2();
test2();


function test3() {
    var IntArr1 = Array(1);
    for (var _ of IntArr1) {
        if (this || 1) {
            return 1;
        }
    }
}
test3();
test3();
test3();


var ary = Array();
var func4 = function () {
    for (var _ of ary) {
        if (undefined || func4) {
            break;
        }
    }
};
func4();
func4();
func4();

console.log('PASSED');
