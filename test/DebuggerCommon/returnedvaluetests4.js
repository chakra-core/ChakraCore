//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validating return values with built-ins and host objects.
function Run() {
    function test1() {
        var a = Date(); /**bp:locals();resume('step_over');locals();resume('step_over');locals()**/
        WScript.Echo("Pass") + WScript.SetTimeout("1", 10);
    }
    test1();

    Array.prototype.ucase = function () {
        for(var i =0; i < this.length;i++) {
            this[i] = this[i].toUpperCase();
        }
    }
    function test2() {
        var arr = new Array("a","b"); /**bp:locals();resume('step_over');locals();resume('step_over');locals();resume('step_over');locals();resume('step_over');locals();resume('step_over');locals()**/
        arr.push("c");
        var str = arr.join();
        arr.ucase();
        var str1 = "All caps " + arr.join();
    }
    test2();
}
WScript.Attach(Run);