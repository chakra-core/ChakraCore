//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "test0",
        body: function () {
            function bar()
            {
                o = {x:2};
            }
            o = {x:1}
            function test0()
            {
                var b;
                for(var i=0;i<2;i++)
                {
                    b = o.x <<= bar();
                }
                assert.areEqual(2, b);
            }
            test0();
            test0();
            o = {x:1};
            test0();
        }
    },
    {
        name: "test1",
        body: function () {
            var obj2 = {};
            var i32 = new Int32Array();
            var func0 = function () {
                return obj2;
            };
            Object.prototype.prop5 = 1;
            var a;
            for (var __loopvar0 = 4; __loopvar0 > 0; __loopvar0--) 
            {
                function func7(arg1) {
                    this.prop2 = arg1;
                }
                obj2 = new func7(obj2.prop5--);
            }
    
            assert.areEqual(1, obj2.prop2);
        }
    },
    {
        name: "test2",
        body: function (){
            function makeArrayLength(x) {
                if (!isNaN(x)) {
                    return Math.floor(x) & 65535;
                }
            }
            var obj0 = {};
            var c = 1;
            obj0.length = makeArrayLength(4294967295);
            
            for (; obj0.length--; c++) 
            {
                obj0 = {
                    method1: function () {
                        return function v1() {
                            ({ nd0: { method1: obj0 } } );
                        };
                    }
                };
            }
            assert.areEqual(2, c);
        }
    }
];
testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });

