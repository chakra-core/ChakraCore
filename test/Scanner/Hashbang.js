//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

let invalidScripts = [
    [" #!\n", "Hashbang must be the first token (even before whitespace)"],
    ["\n#!\n", "Hashbang must be the first token (even before whitespace)"],
    ["##!\n", "Hashbang token is '#!'"],
    [";#!\n", "Hashbang must be the first token"],
    ["'use strict'#!\n", "Hashbang must come before 'use strict'"],
    ["#!\n#!\n", "Only one hashbang may exist because it has to be the first token"],
    ["function foo() {\n#!\n}", "Hashbang must be the first token in the script (not local function)"],
    ["function foo() {#!\n}", "Hashbang must be the first token in the script (not local function)"],
    ["#\\041\n", "Hashbang can't be made of encoded characters"],
    ["#\\u0021\n", "Hashbang can't be made of encoded characters"],
    ["#\\u{21}\n", "Hashbang can't be made of encoded characters"],
    ["#\\x21\n", "Hashbang can't be made of encoded characters"],
    ["\\043!\n", "Hashbang can't be made of encoded characters"],
    ["\\u0023!\n", "Hashbang can't be made of encoded characters"],
    ["\\u{23}!\n", "Hashbang can't be made of encoded characters"],
    ["\\x23!\n", "Hashbang can't be made of encoded characters"],
    ["\\u0023\\u0021\n", "Hashbang can't be made of encoded characters"],
    ["Function('#!\n','')", "Hashbang is not valid in function evaluator contexts"],
    ["new Function('#!\n','')", "Hashbang is not valid in function evaluator contexts"],
    ["{\n#!\n}\n", "Hashbang not valid in block"],
    ["#!/*\nthrow 123;\n*/\nthrow 456;", "Hashbang comments out a single line"],
    ["\\\\ single line comment\n#! hashbang\n", "Single line comment may not preceed hashbang"],
    ["/**/#! hashbang\n", "Multi-line comment may not preceed hashbang"],
    ["/**/\n#! hashbang\n", "Multi-line comment may not preceed hashbang"],
];

var tests = [
    {
        name: "Valid hashbang in ordinary script",
        body: function () {
            assert.areEqual(2, WScript.LoadScript("#! throw 'error';\nthis.prop=2;").prop);
            assert.areEqual(3, WScript.LoadScript("#! throw 'error'\u{000D}this.prop=3;").prop);
            assert.areEqual(4, WScript.LoadScript("#! throw 'error'\u{2028}this.prop=4;").prop);
            assert.areEqual(5, WScript.LoadScript("#! throw 'error'\u{2029}this.prop=5;").prop);
        }
    },
    {
        name: "Valid hashbang in module script",
        body: function () {
            WScript.RegisterModuleSource('module_hashbang_valid.js', "#! export default 123;\n export default 456;");
            
            testRunner.LoadModule(`
                import {default as prop} from 'module_hashbang_valid.js';
                assert.areEqual(456, prop);
            `, 'samethread', false, false);
        }
    },
    {
        name: "Valid hashbang in eval",
        body: function () {
            assert.areEqual(undefined, eval('#!'));
            assert.areEqual(undefined, eval('#!\n'));
            assert.areEqual(1, eval('#!\n1'));
            assert.areEqual(undefined, eval('#!2\n'));
        }
    },
    {
        name: "Valid hashbang in indirect eval",
        body: function () {
            let _eval = eval;
            assert.areEqual(undefined, _eval('#!'));
            assert.areEqual(undefined, _eval('#!\n'));
            assert.areEqual(1, _eval('#!\n1'));
            assert.areEqual(undefined, _eval('#!2\n'));
        }
    },
    {
        name: "Invalid hashbang in ordinary script",
        body: function () {
            for (a of invalidScripts) {
                assert.throws(()=>WScript.LoadScript(a[0]), SyntaxError, a[1]);
            }
        }
    },
    {
        name: "Invalid hashbang in module script",
        body: function () {
            for (a of invalidScripts) {
                assert.throws(()=>WScript.LoadModule(a[0]), SyntaxError, a[1]);
            }
        }
    },
    {
        name: "Invalid hashbang in eval",
        body: function () {
            for (a of invalidScripts) {
                assert.throws(()=>eval(a[0]), SyntaxError, a[1]);
            }
        }
    },
    {
        name: "Invalid hashbang in indirect eval",
        body: function () {
            let _eval = eval;
            for (a of invalidScripts) {
                assert.throws(()=>_eval(a[0]), SyntaxError, a[1]);
            }
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
