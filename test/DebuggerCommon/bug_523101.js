//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    function bar2(x) {
        x = 1; /**bp:stack();setFrame(1);locals();**/
    }
    switch ('abc') {
        case 1:
            function a() { }
            function b() {
                a = 1;
            }
            b();
            break;
        case 'abc':
            y = bar2({ a: 1 });
            break;
        default:
            break;
    }
}
test0();
test0();

WScript.Echo('pass');