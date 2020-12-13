//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(a,b){ return a + b; }
function bar(a,b){ return a - b; }

var obj = {};
obj.foo = foo;

function test(obj)
{
    var count = 0;

    for (var i = 0; i < 1000; i++)
    {
        count += obj.foo(10, i);
    }

    return count;
}

obj.foo = bar;
WScript.Echo(test(obj));
obj.foo = 10;
try
{
    WScript.Echo(test(obj));
}
catch(e)
{
    WScript.Echo(e);
}

var obj2 = {};
try
{
    WScript.Echo(test(obj2));
}
catch(e)
{
    WScript.Echo(e);
}

// Will hoist CheckFuncInfo instr
// and then GC its BailOutInfo.
function test0() {
    var obj0 = {};
    var obj1 = {};
    var func2 = function () {
    };
    var func4 = function () {
        for (var _strvar2 in ary) {
            function v0() {
                throw '';
            }
            function v2() {
                var v3 = 0;
                for (var _strvar0 in f32) {
                    if (v3++) {
                        func2.apply(obj1, arguments);
                        v0();
                    }
                }
            }
            try {
                v2();
            } catch (ex) {
            }
        }
    };
    obj0.method0 = func4;
    var ary = Array();
    var f32 = new Float32Array(256);
    ary[0] = 1;
    ary[1] = 1;
    function v14() {
        var v15 = obj0.method0();
    }
    v14();
}
test0();
