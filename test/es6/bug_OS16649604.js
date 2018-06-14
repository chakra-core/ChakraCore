//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Computed get-set property names",
        body: function () {
            const n = 1;
            const m = 2;
            const r = 0.5;
            const s = 'prop';
            function test2() {
                c = class {
                    get [n]() { return 42; }
                    set [m](val) { }
                    get [r]() { return 'a'; }
                    set [s](val) { }
                    get [1 & Math]() { return 42; }
                }

                d = {
                    get [n]() { return 42; },
                    set [m](val) {},
                    get [r]() { return 'a'; },
                    set [s](val) {}
                };
            }
            for (let i = 0; i < 100; ++i) {
                test2();
            }

            assert.areEqual('number', typeof ((new c())[1]), "Integer as class member getter property name");
            assert.areEqual('undefined', typeof ((new c())[2]), "Integer as class member setter property name");
            assert.areEqual('string', typeof ((new c())[0.5]), "Float as class member getter property name");
            assert.areEqual('undefined', typeof ((new c())['prop']), "String as class member setter property name");
            assert.areEqual('number', typeof ((new c())[1 & Math]), "Expression as class member setter property name");

            assert.areEqual('number', typeof (d[1]), "Integer as getter property name");
            assert.areEqual('undefined', typeof (d[2]), "Integer as setter property name");
            assert.areEqual('string', typeof (d[0.5]), "Float as getter property name");
            assert.areEqual('undefined', typeof (d['prop']), "String as setter property name");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
    