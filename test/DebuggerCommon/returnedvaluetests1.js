//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function Run() {
    function f1() {
        var m = 31;
        m++;
        var coll = new Intl.Collator();
        m += f2();
        return m;
    }

    function f2() {
        return 100;
    }

    function test6() {
        var formatter = new Intl.NumberFormat("en-US");/**bp:locals();resume('step_over');locals();resume('step_into');locals();stack();resume('step_out');locals();stack();**/ 
        f1(new Intl.Collator());
        formatter;
        formatter = new Intl.NumberFormat("en-US"); /**bp:locals();resume('step_into');locals();**/
    }
    test6();

    function test8() {
        function test7() {
            var d = new Date(2013, 1, 1);     
            [d.toLocaleString].forEach(function (f) {
                f; /**bp:resume('step_out');locals();stack()**/
                return f;
            });
            return d;
        }
        test7();        /**bp:locals();resume('step_into');locals();removeExpr()**/
    }
    test8();


    function test9() {
        var k = 10;
        function test10 () {
            var k1 = 10; /**bp:locals()**/
            return k1;
        }
        k+= test10(); /**bp:resume('step_over');**/
    }
    test9();
    WScript.Echo("Pass");
}
WScript.Attach(Run);