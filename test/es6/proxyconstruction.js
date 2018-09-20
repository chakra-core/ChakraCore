//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testConstructProxy (object, shouldFail)
{
    let proxy1 = new Proxy(object, {construct: () => ({})});
    let proxy2 = new Proxy(object, {});
    
    if (shouldFail)
    {
        assert.throws(() => {new proxy1 ()}, TypeError, "Function is not a constructor");
        assert.throws(() => {new proxy2 ()}, TypeError, "Function is not a constructor");
    }
    else
    {
        assert.doesNotThrow(() => {new proxy1 ()});
        assert.doesNotThrow(() => {new proxy2 ()});
    }
}


var tests = [
    {
        name: "Proxy of anonymous Arrow function should not be constructable",
        body() {
            testConstructProxy(()=>{}, true);
        }
    },
    {
        name: "Proxy of named Arrow function should not be constructable",
        body() {
            let test2 = () => {}; 
            testConstructProxy(test2, true);
        }
    },
    {
        name: "Proxy of anonymous Async function should not be constructable",
        body() {
            testConstructProxy(async function () {}, true);
        }
    },
    {
        name: "Proxy of named Async function should not be constructable",
        body() {
            testConstructProxy(async function test4() {}, true);
        }
    },
    {
        name: "Proxy of normal anonymous function should be constructable",
        body() {
            testConstructProxy(function () {}, false);
        }
    },
    {
        name: "Proxy of normal function should be constructable",
        body() {
            testConstructProxy(function test6 () {}, false);
        }
    },
    {
        name: "Proxy of static class member function should not be constructable",
        body() {
            class testing1
            {
                static test () {}
            }
            testConstructProxy(testing1.test, true);
        }
    },
    {
        name: "Proxy of class member function should not be constructable",
        body() {
            class testing2
            {
                test () {}
            }
            let instance = new testing2();
            testConstructProxy(instance.test, true);
        }
    },
    {
        name: "Proxy of Math object should not be constructable",
        body() {
            testConstructProxy(Math, true);
        }
    },
    {
        name: "Proxy of Array object should be constructable",
        body() {
            testConstructProxy(Array, false);
        }
    },
    {
        name: "Proxy of Array instance methods should not be constructable",
        body() {
            let testing = [];
            testConstructProxy(testing.sort, true);
            testConstructProxy(testing.includes, true);
            testConstructProxy(testing.slice, true);
        }
    },
    {
        name: "Proxy of Object object should be constructable",
        body() {
            testConstructProxy(Object, false);
        }
    },
    {
        name: "Proxy of Object literal should not be constructable",
        body() {
            testConstructProxy({}, true);
        }
    },
    {
        name: "Proxy of Object literal function should be constructable",
        body() {
            let testingMethod = {test:function() {}}
            testConstructProxy(testingMethod.test, false);
        }
    },
    {
        name: "Proxy of Object literal method should not be constructable",
        body() {
            let testingMethod = { test() { } }
            testConstructProxy(testingMethod.test, true);
        }
    },
    {
        name: "Proxy of Date object should be constructable",
        body() {
            testConstructProxy(Date, false);
        }
    },
    {
        name: "Proxy of Number object should be constructable",
        body() {
            testConstructProxy(Number, false);
        }
    },
    {
        name: "Proxy of BuiltIn methods should not be constructable",
        body() {
            testConstructProxy(Math.abs, true);
            testConstructProxy(Math.max, true);
            testConstructProxy(Math.min, true);
            testConstructProxy(Math.floor, true);
            testConstructProxy(Math.ceil, true);
            testConstructProxy(parseInt, true);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
