//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Step out from JITted function - ES5
*/

function foo() {
    var a = [];
    a.push(1);

    a.forEach(function () {
        var x; /**loc(bp1):stack();resume('step_out')**/
    });
}
function Run() {
    foo();
    foo();
    foo(); /**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}
//No JIT only attach
WScript.Attach(Run);





