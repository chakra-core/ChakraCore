//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validating locals at lower frames, it is exercising the fetching locals from the native (JITed) frames.

function F2() {
    var a = new Array;                       
    for (var i = 0; i < 100; i++) {
        a.push(i * 10.2);
    }

    var sub = -10;                            /**bp:stack();locals();setFrame(1);locals();setFrame(2);locals();setFrame(3);locals();setFrame(4);locals();setFrame(5);locals();**/
    for (var i = 0; i < 100; i++) {
        a[i] -= sub;
    }
    return a.length;
}

function F3(r) {
    return 2 * 3.14 * r;
}

function F5() {
    var x = 20;     
    var y = {y1:1}; 
    var t1 = Math.abs(22.2);
    function F5_1(a) {
        eval('var z = x * y.y1;');
        var f6 = function () {
            var b1 = 10;
            var obj1 = {a1:2};
            obj4 = F3(obj1.a1);
            return b1 + F2();
        }
        f6();
    }
    F5_1(t1);
    return y;
}

function F6(a,b) {
    (function () {
        var j = "Anonymous function2";
        F5();
        return j;                                    /**bp:stack();locals();setFrame(1);locals();setFrame(2);locals();setFrame(3);locals();**/
    })();
}

function F7(a1)
{
    var j = 10;
    var k = 20 * 1.34;
    var m = 20 + a1;
    function f2(b21, b22)
    {
        var j2 = arguments.length;
        var k2 = 10;
        function f3(c31,c32)
        {
            var a = 10;
            a++;                            /**bp:stack();locals();setFrame(1);locals();setFrame(2);locals();setFrame(3);locals();**/
            a++;                            
            return c31+c32+a;               
        }
        
        function f32()
        {
            j;
        }
        f3();
    }
    
    f2(k);
}

function Run(arg1, arg2, arg3)
{
    F6(arg1, arg2);
    F7(arg3);
}

Run(12, "assdf", 112);
Run([3,5,8], {a:"aa"}, 21);
Run(12, "assdf", 112);
Run([3,5,8], {a:"aa"}, 21);

WScript.Echo("Pass");
