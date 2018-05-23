//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");


function testGen(func, values, count)
{
    const gen = func();
    let counter = 0;
    for (const value of gen)
    {
        assert.isTrue(value == values[counter]);
        ++counter;
    }
    assert.areEqual(counter, count);
}


const tests = [
    {
        name : "For - in with Arrow function sloppy",
        body : function () {
            const arr = [0,1,2,5];
            for (var a = () => { return "a"} in {});
            assert.areEqual(a(), "a");
            for (var a = () => { return "a"} in arr);
            assert.isTrue(a == "3");
        }
    },
    {
        name : "For - in with Arrow function strict",
        body : function () {
            "use strict";
            assert.throws(()=>{eval("for (var a = () => { return 'a'} in {});")}, SyntaxError);
        }
    },
    {
        name : "For - in with yield - sloppy",
        body : function () {
            function* gen1()
            {
                for (var a = yield 'a' in {b: 1}) {
                    assert.isTrue(a == "b");
                }
            }
            testGen(gen1, ["a"], 1);
            function* gen2()
            {
                for (var a = yield in {c: 1}) {
                    assert.isTrue(a == "c");
                }
            }
            testGen(gen2, [undefined], 1);
            function* gen3()
            {
                for (var a = yield 'd' in {} in {a: 1}) {
                    assert.isTrue(false, "shouldn't reach here");
                }
            }
            testGen(gen3, ['d'], 1);
        }
    },
    {
        name : "For - in with yield - strict",
        body : function () {
            "use strict";
            assert.throws(()=>{eval(`function* gen1()
            {
                for (var a = yield 'a' in {b: 1}) {
                    assert.isTrue(a == "b");
                }
            }`)}, SyntaxError);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
