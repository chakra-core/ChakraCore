//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Unit test framework. The one API that can be used by unit tests.
// Usage:
//   How to import UTF -- add the following to the beginning of your test:
//     /// <reference path="../UnitTestFramework/UnitTestFramework.js" />
//     if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
//         this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
//     }
//
//   How to define and run tests:
//     var tests = [ { name: "test name 1", body: function () {} }, ...];
//     testRunner.run(tests);
//
//   How to output "pass" only if run passes so that we can skip baseline:
//     testRunner.run(tests, {verbose: false});
//     Or do this only when "summary" is given on command line:
//          jshost   -args summary -endargs
//          testRunner.run(tests, { verbose: WScript.Arguments[0] != "summary" });
//
//   How to launch module code
//     testRunner.LoadModule(source, context, shouldFail)
//     example: testRunner.LoadModule(source, 'samethread', false);
//
//   How to use assert:
//     assert.areEqual(expected, actual, "those two should be equal (i.e. deep equality of objects using ===)");
//     assert.areNotEqual(expected, actual, "those two should NOT be equal");
//     assert.areAlmostEqual(expected, actual, "those two should be almost equal, numerically (allows difference by epsilon)");
//     assert.isTrue(actual, "actual should be true");
//     assert.isFalse(actual, "actual should be false");
//     assert.isUndefined(actual, "actual should be undefined");
//     assert.isNotUndefined(actual, "actual should not be undefined");
//     assert.throws(function, SyntaxError, "function should throw (in this case, specifically a SyntaxError with fooMessage", "fooMessage");
//     assert.doesNotThrow(function, "this function should not throw anything");
//     assert.fail("error");
//
//   Some useful helpers:
//     helpers.writeln("works in both", "console", "and", "browser);
//     helpers.printObject(WScript);
//     var isInBrowser = helpers.isInBrowser();

var helpers = function helpers() {
    //private
    var undefinedAsString = "undefined";
    var isWScriptAvailable = this.WScript;
    if (isWScriptAvailable && !this.WScript.LoadModuleFile) {
        WScript.LoadModuleFile = function (fileName) {
            WScript.LoadScriptFile(fileName, "module");
        }
    }

    return {
        isInBrowser: function isInBrowser() {
            return typeof (document) !== undefinedAsString;
        },

        // TODO: instead of this hack consider exposing this/ScriptConfiguration through WScript.
        get isCompatVersion9() {
            return (typeof (ArrayBuffer) == undefinedAsString);
        },
        get isVersion10OrLater() {
            return !this.isCompatVersion9;
        },

        // If propName is provided, thedummy object would have 1 property with name = propName, value = 0.
        getDummyObject: function getDummyObject(propName) {
            var dummy = this.isInBrowser() ? document : {};
            //var dummy = {};

            if (propName != undefined) {
                dummy[propName] = 0;
            }
            return dummy;
        },

        writeln: function writeln() {
            var line = "", i;
            for (i = 0; i < arguments.length; i += 1) {
                line = line.concat(arguments[i])
            }
            if (!this.isInBrowser() || isWScriptAvailable) {
                WScript.Echo(line);
            } else {
                document.writeln(line);
                document.writeln("<br/>");
            }
        },

        printObject: function printObject(o) {
            var name;
            for (name in o) {
                this.writeln(name, o.hasOwnProperty(name) ? "" : " (inherited)", ": ", o[name]);
            }
        },

        withPropertyDeleted: function withPropertyDeleted(object, propertyName, callback) {
            var descriptor = Object.getOwnPropertyDescriptor(object, propertyName);
            try {
                delete object[propertyName];
                callback();
            } finally {
                Object.defineProperty(object, propertyName, descriptor);
            }
        },

        getTypeOf: function getTypeOf(object)
        {
            return Object.prototype.toString.call(object);
        },
    }
}(); // helpers module.

var testRunner = function testRunner() {
    var executedTestCount = 0;
    var passedTestCount = 0;
    var objectType = "object";
    var _verbose = true; // If false, try to output "pass" only for passed run so we can skip baseline.
    var _asyncMode = false;  // If true, run tests in asynchronous mode
    var _hasAsyncTestPromise = false;

    var asyncTest = {
        "promise" : [],
        "resolve" : []
    };

    var testRunner = {
        isAsyncTest: function isAsyncTest() {
            return _asyncMode;
        },

        getAsyncTestIndex: function getAsyncTestIndex() {
            return asyncTest.testIndex;
        },

        asyncTestErr: function asyncTestErr(testIndex, str) {
            asyncTest.error[testIndex] += str + "\n";
        },

        asyncTestHasNoErr: function asyncTestHasNoErr(testIndex) {
            return asyncTest.error[testIndex] == "";
        },

        asyncTestCompleted: function asyncTestCompleted(testIndex) {
            for (var i in asyncTest.resolve[testIndex]) {
                if (asyncTest.resolve[testIndex][i] != 0) {
                    return false;
                }
            }
            return true;
        },

        runTests: function runTests(testsToRun, options) {
            ///<summary>Runs provided tests.<br/>
            /// The 'testsToRun' is an object that has enumerable properties,<br/>
            /// each property is an object that has 'name' and 'body' properties.
            /// Also collects and tallies error output of asynchronously executed code.
            ///</summary>

            asyncTest.count = new Array(testsToRun.length).fill(0);
            asyncTest.error = new Array(testsToRun.length).fill("");

            if (options && options.hasOwnProperty("verbose")) {
                _verbose = options.verbose;
            }

            for (var i in testsToRun) {
                var isRunnable = typeof testsToRun[i] === objectType;
                if (isRunnable) {
                    this.runTest(i, testsToRun[i].name, testsToRun[i].body);
                }
            }

            if (!_hasAsyncTestPromise) {
                if (_verbose || executedTestCount != passedTestCount) {
                    helpers.writeln(`Summary of tests: total executed: ${executedTestCount}; passed: ${passedTestCount}; failed: ${executedTestCount - passedTestCount}`);
                } else {
                    helpers.writeln("pass");
                }
            } else {
                var promiseResolved = false;
                var asyncTestSummary = function asyncTestSummary(executedTestCount) {
                    var passedTestCount = 0;
                    for (var i in testsToRun) {
                        var completed = testRunner.asyncTestCompleted(i);
                        var passed = completed && testRunner.asyncTestHasNoErr(i);
                        if (!passed) {
                            helpers.writeln(`*** Async result of test #${Number(i)+1} (${i}): ${testsToRun[i].name}`);
                            helpers.writeln(asyncTest.error[i]);
                            if (!completed) {
                                helpers.writeln("NOT COMPLETED");
                            }
                            helpers.writeln(passed ? "PASSED" : "FAILED");
                        } else {
                            passedTestCount++;
                        }
                    }

                    if (_verbose || executedTestCount != passedTestCount) {
                        helpers.writeln(`Summary of tests: total executed: ${executedTestCount}; passed: ${passedTestCount}; failed: ${executedTestCount - passedTestCount}`);
                    } else {
                        helpers.writeln("pass");
                    }

                };

                Promise.all(asyncTest.promise.map(Promise.all, Promise)).then( function() {
                    asyncTestSummary(executedTestCount);
                    promiseResolved = true;
                    WScript.ClearTimeout(scheduledPromiseCheck);
                });

                var promiseCheck = function promiseCheck(time, keepWaiting) {
                    if (keepWaiting && !promiseResolved) {
                        WScript.SetTimeout(()=>promiseCheck(time, true), time);
                    } else if (!promiseResolved) {
                        asyncTestSummary(executedTestCount);
                    }
                };
                var scheduledPromiseCheck = WScript.SetTimeout(promiseCheck, 5000);
            }
        },

        run: function run(tests, options) {
            this.runTests(tests, options);
        },

        runTest: function runTest(testIndex, testName, testBody) {
            ///<summary>Runs test body catching all exceptions.<br/>
            /// Result: prints PASSED/FAILED to the output.
            ///</summary>
            function logTestNameIf(b) {
                if (b) {
                    helpers.writeln(`*** Running test #${executedTestCount + 1} (${testIndex}): ${testName}`);
                }
            }

            logTestNameIf(_verbose);

            asyncTest.testIndex = testIndex;
            asyncTest.promise[testIndex] = [];
            asyncTest.resolve[testIndex] = [];

            var isSuccess = true;
            try {
                testBody();
            } catch (ex) {
                var message = ex.stack || ex.message || ex;
                logTestNameIf(!_verbose);
                helpers.writeln("Test threw exception: ", message);
                isSuccess = false;
            }
            if (isSuccess) {
                if (_verbose) {
                    helpers.writeln("PASSED");
                }
                ++passedTestCount;
            } else {
                helpers.writeln("FAILED");
                testRunner.asyncTestErr(testIndex, "RUN FAILED");
            }
            ++executedTestCount;
        },

        asyncTestBegin: function asyncTestBegin(testIndex, testCount) {
            _asyncMode = true;
            asyncTest.testIndex = testIndex;
        },

        asyncTestEnd: function asyncTestEnd(testIndex, testCount) {
            _asyncMode = false;
            asyncTest.resolve[testIndex][testCount]();
            asyncTest.resolve[testIndex][testCount] = 0;
        },

        prepareAsyncCode: function prepareAsyncCode(source, shouldFail, explicitAsyncTestExit) {
            var testIndex = asyncTest.testIndex;
            if (typeof shouldFail == "undefined" || shouldFail == false) {
                _hasAsyncTestPromise = true;
                var testCount = asyncTest.count[testIndex]++;
                var promise = new Promise((resolve, reject) => {
                    asyncTest.resolve[testIndex].push(resolve);
                });
                asyncTest.promise[testIndex].push(promise);
                return explicitAsyncTestExit ?
                    `
                    var _asyncEnter = ()=>{ testRunner.asyncTestBegin(${testIndex}, ${testCount}); };
                    var _asyncExit = ()=>{ testRunner.asyncTestEnd(${testIndex}, ${testCount}); };
                    _asyncEnter();
                    ${source};
                    `  :
                    `
                    testRunner.asyncTestBegin(${testIndex}, ${testCount});
                    ${source};
                    testRunner.asyncTestEnd(${testIndex}, ${testCount});
                    `;
            } else {
                return source;
            }
        },

        LoadModule : function LoadModule(source, context, shouldFail, explicitAsyncTestExit) {
            return WScript.LoadModule(testRunner.prepareAsyncCode(source, shouldFail, explicitAsyncTestExit), context);
        },

        LoadScript : function LoadScript(source, context, shouldFail, explicitAsyncTestExit) {
            return WScript.LoadScript(testRunner.prepareAsyncCode(source, shouldFail, explicitAsyncTestExit));
        }
    };
    return testRunner;
}(); // testRunner.

var assert = function assert() {
    // private
    var isObject = function isObject(x) {
        return x instanceof Object && typeof x !== "function";
    };

    var isNaN = function isNaN(x) {
        return x !== x;
    };

    var throwMessage = function throwMessage(ex) {
        if (!testRunner.isAsyncTest()) {
            throw ex;
        } else {
            var message = ex.stack || ex.message || ex;
            testRunner.asyncTestErr(testRunner.getAsyncTestIndex(), "Test threw exception: " + message);
        }
    }

    // returns true on success, and error message on failure
    var compare = function compare(expected, actual) {
        if (expected === actual) {
            return true;
        } else if (isObject(expected)) {
            if (!isObject(actual)) return "actual is not an object";
            var expectedFieldCount = 0, actualFieldCount = 0;
            for (var i in expected) {
                var compareResult = compare(expected[i], actual[i]);
                if (compareResult !== true) return compareResult;
                ++expectedFieldCount;
            }
            for (var i in actual) {
                ++actualFieldCount;
            }
            if (expectedFieldCount !== actualFieldCount) {
                return "actual has different number of fields than expected";
            }
            return true;
        } else {
            if (isObject(actual)) return "actual is an object";
            if (isNaN(expected) && isNaN(actual)) return true;
            return "expected: " + expected + " actual: " + actual;
        }
    };

    var epsilon = (function () {
        var next, result;
        for (next = 1; 1 + next !== 1; next = next / 2) {
            result = next;
        }
        return result;
    }());

    var compareAlmostEqualRelative = function compareAlmostEqualRelative(expected, actual) {
        if (typeof expected !== "number" || typeof actual !== "number") {
            return compare(expected, actual);
        } else {
            var diff = Math.abs(expected - actual);
            var absExpected = Math.abs(expected);
            var absActual = Math.abs(actual);
            var largest = (absExpected > absActual) ? absExpected : absActual;

            var maxRelDiff = epsilon * 5;

            if (diff <= largest * maxRelDiff) {
                return true;
            } else {
                return "expected almost: " + expected + " actual: " + actual + " difference: " + diff;
            }
        }
    }

    var validate = function validate(result, assertType, message) {
        if (result !== true) {
            var exMessage = addMessage("assert." + assertType + " failed: " + result);
            exMessage = addMessage(exMessage, message);
            throwMessage(exMessage);
        }
    }

    var addMessage = function addMessage(baseMessage, message) {
        if (message !== undefined) {
            baseMessage += ": " + message;
        }
        return baseMessage;
    }

    return {
        areEqual: function areEqual(expected, actual, message) {
            /// <summary>
            /// IMPORTANT: NaN compares equal.<br/>
            /// IMPORTANT: for objects, assert.areEqual checks the fields.<br/>
            /// So, for 'var obj1={}, obj2={}' areEqual would be success, although in Javascript obj1 != obj2.<br/><br/>
            /// Performs deep comparison of arguments.<br/>
            /// This works for objects and simple types.<br/><br/>
            ///
            /// TODO: account for other types?<br/>
            /// TODO: account for missing vs undefined fields.
            /// </summary>
            validate(compare(expected, actual), "areEqual", message);
        },

        areNotEqual: function areNotEqual(expected, actual, message) {
            validate(compare(expected, actual) !== true, "areNotEqual", message);
        },

        areAlmostEqual: function areAlmostEqual(expected, actual, message) {
            /// <summary>
            /// Compares numbers with an almost equal relative epsilon comparison.
            /// Useful for verifying math functions.
            /// </summary>
            validate(compareAlmostEqualRelative(expected, actual), "areAlmostEqual", message);
        },

        isTrue: function isTrue(actual, message) {
            validate(compare(true, actual), "isTrue", message);
        },

        isFalse: function isFalse(actual, message) {
            validate(compare(false, actual), "isFalse", message);
        },

        isUndefined: function isUndefined(actual, message) {
            validate(compare(undefined, actual), "isUndefined", message);
        },

        isNotUndefined: function isUndefined(actual, message) {
            validate(compare(undefined, actual) !== true, "isNotUndefined", message);
        },

        throws: function throws(testFunction, expectedException, message, expectedErrorMessage) {
            /// <summary>
            /// Makes sure that the function specified by the 'testFunction' parameter
            /// throws the exception specified by the 'expectedException' parameter.<br/><br/>
            ///
            /// expectedException:  A specific exception type, e.g. TypeError.
            ///                     if undefined, this will pass if testFunction throws any exception.<br/>
            /// message: Test-provided explanation of why this particular exception should be thrown.<br/>
            /// expectedErrorMessage: If specified, verifies the thrown Error has expected message.<br/>
            ///                       You only need to specify this if you are looking for a specific error message.<br/>
            ///                       Does not require the prefix of e.g. "TypeError:" that would be printed by the host.<br/><br/>
            ///
            /// Example:<br/>
            /// assert.throws(function() { eval("{"); }, SyntaxError, "expected SyntaxError")<br/>
            /// assert.throws(function() { eval("{"); }) // -- use this when you don't care which exception is thrown.<br/>
            /// assert.throws(function() { eval("{"); }, SyntaxError, "expected SyntaxError with message about expected semicolon.", "Expected ';'")
            /// </summary>
            var noException = {};         // Some unique object which will not be equal to anything else.
            var exception = noException;  // Set default value.
            try {
                testFunction();
            } catch (ex) {
                exception = ex;
            }

            if (expectedException === undefined && exception !== noException) {
                return;  // Any exception thrown is OK.
            }

            var validationPart = exception;
            if (exception !== noException && exception instanceof Object) {
                validationPart = exception.constructor;
            }
            var validationErrorMessage = exception instanceof Error ? exception.message : undefined;
            if (validationPart !== expectedException || (expectedErrorMessage && validationErrorMessage !== expectedErrorMessage)) {
                var expectedString = expectedException !== undefined ?
                  expectedException.toString().replace(/\n/g, "").replace(/.*function (.*)\(.*/g, "$1") :
                  "<any exception>";
                if (expectedErrorMessage) {
                    expectedString += " " + expectedErrorMessage;
                }
                var actual = exception !== noException ? exception : "<no exception>";
                throwMessage(addMessage("assert.throws failed: expected: " + expectedString + ", actual: " + actual, message));
            }
        },

        doesNotThrow: function doesNotThrow(testFunction, message) {
            var noException = {};
            var exception = noException;
            try {
                testFunction();
            } catch (ex) {
                exception = ex;
            }

            if (exception === noException) {
                return;
            }

            throwMessage(addMessage("assert.doesNotThrow failed: expected: <no exception>, actual: " + exception, message));
        },

        fail: function fail(message) {
            ///<summary>Can be used to fail the test.</summary>
            throwMessage(message);
        }
    }
}(); // assert.
