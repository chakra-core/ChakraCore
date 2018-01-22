//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES6 Module functionality tests -- verifies functionality of import and export statements

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testScript(source, message, shouldFail = false, explicitAsync = false) {
    message += " (script)";
    let testfunc = () => testRunner.LoadScript(source, undefined, shouldFail, explicitAsync);

    if (shouldFail) {
        let caught = false;

        assert.throws(testfunc, SyntaxErrr, message);
        assert.isTrue(caught, `Expected error not thrown: ${message}`);
    } else {
        assert.doesNotThrow(testfunc, message);
    }
}

function testModuleScript(source, message, shouldFail = false, explicitAsync = false) {
    message += " (module)";
    let testfunc = () => testRunner.LoadModule(source, 'samethread', shouldFail, explicitAsync);

    if (shouldFail) {
        let caught = false;

        // We can't use assert.throws here because the SyntaxError used to construct the thrown error
        // is from a different context so it won't be strictly equal to our SyntaxError.
        try {
            testfunc();
        } catch(e) {
            caught = true;

            // Compare toString output of SyntaxError and other context SyntaxError constructor.
            assert.areEqual(e.constructor.toString(), SyntaxError.toString(), message);
        }

        assert.isTrue(caught, `Expected error not thrown: ${message}`);
    } else {
        assert.doesNotThrow(testfunc, message);
    }
}

function testDynamicImport(importFunc, thenFunc, catchFunc, _asyncEnter, _asyncExit) {
    var promise = importFunc();
    assert.isTrue(promise instanceof Promise);
    promise.then((v)=>{
        _asyncEnter();
        thenFunc(v);
        _asyncExit();
    }).catch((err)=>{
        _asyncEnter();
        catchFunc(err);
        _asyncExit();
    });
}

var tests = [
    {
        name: "Valid cases for import()",
        body: function () {
                let functionBody =
                    `
                    assert.doesNotThrow(function () { eval("import(undefined)"); }, "undefined");
                    assert.doesNotThrow(function () { eval("import(null)"); }, "null");
                    assert.doesNotThrow(function () { eval("import(true)"); }, "boolean - true");
                    assert.doesNotThrow(function () { eval("import(false)"); }, "boolean - false");
                    assert.doesNotThrow(function () { eval("import(1234567890)"); }, "number");
                    assert.doesNotThrow(function () { eval("import('abc789cde')"); }, "string literal");
                    assert.doesNotThrow(function () { eval("import('number' + 100 + 0.4 * 18)"); }, "expression");
                    assert.doesNotThrow(function () { eval("import(import(true))"); }, "nested import");
                    assert.doesNotThrow(function () { eval("import(import(Infinity) + import(undefined))"); }, "nested import expression");
                    `;

                testScript(functionBody, "Test importing a simple exported function");
                testModuleScript(functionBody, "Test importing a simple exported function");
        }
    },
    {
        name: "Syntax errors for import() call",
        body: function () {
                let functionBody =
                    `
                    assert.throws(function () { eval("import()"); }, SyntaxError, "no argument");
                    assert.throws(function () { eval("import(1, 2)"); }, SyntaxError, "more than one arguments");
                    assert.throws(function () { eval("import('abc.js', 'def.js')"); }, SyntaxError, "more than one argument - case 2");
                    assert.throws(function () { eval("import(...['abc.js', 'def.js'])"); }, SyntaxError, "spread argument");
                    `;

                testScript(functionBody, "Test importing a simple exported function");
                testModuleScript(functionBody, "Test importing a simple exported function");
        }
    },
    {
        name: "Module specifier that are not string",
        body: function () {
            var testNonStringSpecifier = function (specifier, expectedType, expectedErrMsg, message) {
                if (typeof message === "undefined" ) {
                    message = specifier;
                }

                let functionBody =
                    `testDynamicImport(
                        ()=>{
                            return import(${specifier});
                        },
                        (v)=>{
                            assert.fail('Expected: promise rejected; actual: promise resolved: ' + '${message}');
                        },
                        (err)=>{
                            assert.isTrue(err instanceof Error, '${message}');
                            assert.areEqual(${expectedType}, err.constructor, '${message}');
                            assert.areEqual("${expectedErrMsg}", err.message, '${message}');
                        }, _asyncEnter, _asyncExit
                    )`;

                testScript(functionBody, "Test importing a simple exported function", false, true);
                testModuleScript(functionBody, "Test importing a simple exported function", false, true);
            };

            testNonStringSpecifier("undefined", "URIError", "undefined");
            testNonStringSpecifier("null", "URIError", "null");
            testNonStringSpecifier("true", "URIError", "true");
            testNonStringSpecifier("false", "URIError", "false");
            testNonStringSpecifier("NaN", "URIError", "NaN");
            testNonStringSpecifier("+0", "URIError", "0");
            testNonStringSpecifier("-0", "URIError", "0");
            testNonStringSpecifier("-12345", "URIError", "-12345");
            testNonStringSpecifier("1/0", "URIError", "Infinity");
            testNonStringSpecifier("1.123456789012345678901", "URIError", "1.1234567890123457");
            testNonStringSpecifier("-1.123456789012345678901", "URIError", "-1.1234567890123457");
            testNonStringSpecifier('Symbol("abc")', "TypeError", "Object doesn't support property or method 'ToString'");
            testNonStringSpecifier("{}", "URIError", "[object Object]");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
