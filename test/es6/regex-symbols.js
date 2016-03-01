//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function verifyRegExpObjectWhenExecIsNotCallable(symbol, createRegExp) {
    var re = createRegExp();
    assert.throws(RegExp.prototype[symbol].bind(re), TypeError);
}

function verifyBuiltInSearchWhenExecIsNotCallable(setUp, cleanUp) {
    cleanUp = cleanUp || function () {};

    var result;

    var re = /search/;
    try {
        setUp(re);
        result = re[Symbol.search]("prefix search suffix");
    }
    finally {
        cleanUp();
    }

    assert.isTrue(result === 7, "result");
}

function verifyStringMethodRequiresObjectCoercibleThis(propertyName, thisObj) {
    assert.throws(String.prototype[propertyName].bind(thisObj), TypeError);
}

function verifyBuiltInSymbolMethod(stringPropertyName, symbolName, symbol, createRegExp) {
    var toStringValue = "string value";
    var string = {
        toString: function () {
            return toStringValue;
        }
    };
    var index = 123;
    var callCount = 0;
    var passedANewRegEx = false;
    var coercedString = false;
    var re = createRegExp();

    var methodDescriptor = Object.getOwnPropertyDescriptor(RegExp.prototype, symbol);
    var result;
    try {
        var method = function (stringArg) {
            callCount += 1;
            passedANewRegEx = this !== re && this instanceof RegExp;
            coercedString = stringArg === string;
            return index;
        }
        Object.defineProperty(RegExp.prototype, symbol, {value: method});

        result = String.prototype[stringPropertyName].call(string, re);
    }
    finally {
        Object.defineProperty(RegExp.prototype, symbol, methodDescriptor);
    }

    assert.areEqual(index, result, "result");
    assert.areEqual(1, callCount, "'" + symbolName + "' call count");
    assert.isTrue(passedANewRegEx, "A new RegExp is created");
    assert.isTrue(coercedString, "'string' argument is coerced to String");
}

function verifySymbolMethodExistence(symbol) {
    assert.isTrue(symbol in RegExp.prototype);

    var descriptor = Object.getOwnPropertyDescriptor(RegExp.prototype, symbol);
    assert.isTrue(descriptor.configurable, "descriptor.configurable");
    assert.isTrue(descriptor.writable, "descriptor.writable");
    assert.isFalse(descriptor.enumerable, "descriptor.enumerable");
}

function verifySymbolMethodName(expectedName, symbol) {
    var func = RegExp.prototype[symbol];

    assert.areEqual(expectedName, func.name, "name");

    var descriptor = Object.getOwnPropertyDescriptor(func, "name");
    assert.isTrue(descriptor.configurable, "descriptor.configurable");
    assert.isFalse(descriptor.writable, "descriptor.writable");
    assert.isFalse(descriptor.enumerable, "descriptor.enumerable");
}

function verifySymbolMethodLength(expectedLength, symbol) {
    var func = RegExp.prototype[symbol];

    assert.areEqual(1, func.length);

    var descriptor = Object.getOwnPropertyDescriptor(func, "length");
    assert.isTrue(descriptor.configurable, "descriptor.configurable");
    assert.isFalse(descriptor.writable, "descriptor.writable");
    assert.isFalse(descriptor.enumerable, "descriptor.enumerable");
}

function verifySymbolMethodRequiresThisToBeObject(symbol) {
    var nonObject = "string";
    assert.throws(RegExp.prototype[symbol].bind(nonObject, ""), TypeError);
}

function getFullSymbolMethodName(symbolName) {
    return "RegExp.prototype[" + symbolName + "]";
}

function createTestsForThisObjectType(symbolName, symbol) {
    var fullSymbolMethodName = getFullSymbolMethodName(symbolName);
    return [
        {
            name: fullSymbolMethodName + " should throw an exception when 'this' isn't an Object",
            body: verifySymbolMethodRequiresThisToBeObject.bind(undefined, symbol)
        },
        {
            name: fullSymbolMethodName + " should be callable when 'this' is an ordinary Object and it has 'exec'",
            body: function () {
                var object = {
                    exec: function () {
                        return null;
                    }
                };
                assert.doesNotThrow(RegExp.prototype[symbol].bind(object, ""));
            }
        },
        {
            name: fullSymbolMethodName + " should expect 'this' to be a RegExp object when 'exec' property does not exist",
            body: function () {
                var createRegExp = function () {
                    return {};
                };
                verifyRegExpObjectWhenExecIsNotCallable(symbol, createRegExp);
            }
        },
        {
            name: fullSymbolMethodName + " should expect 'this' to be a RegExp object when 'exec' is not callable",
            body: function () {
                var createRegExp = function () {
                    return {exec: 0};
                };
                verifyRegExpObjectWhenExecIsNotCallable(symbol, createRegExp);
            }
        },
    ];
}

function createTestsForSymbolToExecDelegation(symbolName, symbol) {
    var fullSymbolMethodName = getFullSymbolMethodName(symbolName);
    return [
        {
            name: fullSymbolMethodName + " should delegate to 'exec'",
            body: function () {
                var re = /./;
                var string = "string argument";
                var callCount = 0;
                var passedCorrectThisObject = false;
                var passedCorrectString = false;
                var exec = function (execString) {
                    callCount += 1;
                    passedCorrectThisObject = this === re;
                    passedCorrectString = execString === string;
                    return null;
                };
                Object.defineProperty(re, "exec", {value: exec});

                re[symbol](string);

                assert.areEqual(1, callCount, "'exec' call count");
                assert.isTrue(passedCorrectThisObject, "'this' is correct");
                assert.isTrue(passedCorrectString, "'string' argument is correct");
            }
        },
        {
            name: fullSymbolMethodName + " should throw when return value of 'exec' is not an Object or 'null'",
            body: function () {
                var re = /./;
                var exec = function (execString) {
                    return undefined;
                };
                Object.defineProperty(re, "exec", {value: exec});
                assert.throws(RegExp.prototype[symbol].bind(re), TypeError);
            }
        },
    ];
}

function createTestsForStringCoercion(symbolName, symbol) {
    var fullSymbolMethodName = getFullSymbolMethodName(symbolName);
    return [
        {
            name: fullSymbolMethodName + " should coerce the 'string' argument to String",
            body: function () {
                var re = /./;
                var toStringValue = "string argument";
                var string = {
                    toString: function () {
                        return toStringValue;
                    }
                };
                var coercedString = false;
                var exec = function (execString) {
                    coercedString = execString === toStringValue;
                    return null;
                };
                Object.defineProperty(re, "exec", {value: exec});

                re[symbol](string);

                assert.isTrue(coercedString);
            }
        },
        {
            name: fullSymbolMethodName + " should use the String 'undefined' when the 'string' argument is missing",
            body: function () {
                var re = /./;
                var coercedString = false;
                var exec = function (execString) {
                    coercedString = execString === "undefined";
                    return null;
                };
                Object.defineProperty(re, "exec", {value: exec});

                re[symbol]();

                assert.isTrue(coercedString);
            }
        },
    ];
}

function createTestsForStringToRegExpDelegation(stringPropertyName, symbolName, symbol) {
    var fullStringPropertyName = "String.prototype." + stringPropertyName;
    return [
        {
            name: fullStringPropertyName + " should throw when 'this' is 'undefined'",
            body: verifyStringMethodRequiresObjectCoercibleThis.bind(undefined, stringPropertyName, undefined),
        },
        {
            name: fullStringPropertyName + " should throw when 'this' is 'null'",
            body: verifyStringMethodRequiresObjectCoercibleThis.bind(undefined, stringPropertyName, null),
        },
        {
            name: fullStringPropertyName + " should delegate to '" + symbolName + "' property of the 'regexp' argument",
            body: function () {
                var string = "this string";
                var index = 123;
                var callCount = 0;
                var passedCorrectThisObject = false;
                var passedCorrectString = false;
                var re = {
                    [symbol]: function (stringArg) {
                        callCount += 1;
                        passedCorrectThisObject = this === re;
                        passedCorrectString = stringArg === string;

                        return index;
                    }
                };

                var result = string[stringPropertyName](re);

                assert.areEqual(index, result, "result");
                assert.areEqual(1, callCount, "'" + symbolName + "' call count");
                assert.isTrue(passedCorrectThisObject, "'this' is correct");
                assert.isTrue(passedCorrectString, "'string' argument is correct");
            }
        },
        {
            name: fullStringPropertyName + " should run the built-in '" + symbolName + "' when the '" + symbolName + "' property of the 'regexp' argument is 'undefined'",
            body: function () {
                function createRegExp() {
                    var re = /./;
                    re[symbol] = undefined;
                    return re;
                }
                verifyBuiltInSymbolMethod(stringPropertyName, symbolName, symbol, createRegExp);
            }
        },
        {
            name: fullStringPropertyName + " should run the built-in '" + symbolName + "' when the 'regexp' argument is 'undefined'",
            body: function () {
                function createRegExp() {
                    return undefined;
                }
                verifyBuiltInSymbolMethod(stringPropertyName, symbolName, symbol, createRegExp);
            }
        },
        {
            name: fullStringPropertyName + " should run the built-in '" + symbolName + "' when the 'regexp' argument is 'null'",
            body: function () {
                function createRegExp() {
                    return null;
                }
                verifyBuiltInSymbolMethod(stringPropertyName, symbolName, symbol, createRegExp);
            }
        },
    ];
}

function createGenericTestsForSymbol(stringPropertyName, symbolName, symbol) {
    return [].concat(createTestsForThisObjectType(symbolName, symbol))
             .concat(createTestsForSymbolToExecDelegation(symbolName, symbol))
             .concat(createTestsForStringCoercion(symbolName, symbol))
             .concat(createTestsForStringToRegExpDelegation(stringPropertyName, symbolName, symbol));
}

var tests = [
    {
        name: "RegExp.prototype[@@search] exists",
        body: verifySymbolMethodExistence.bind(undefined, Symbol.search)
    },
    {
        name: "RegExp.prototype[@@search] has the correct name",
        body: verifySymbolMethodName.bind(undefined, "[Symbol.search]", Symbol.search)
    },
    {
        name: "RegExp.prototype[@@search] has the correct length",
        body: verifySymbolMethodLength.bind(undefined, 1, Symbol.search)
    },
    {
        name: "RegExp.prototype[@@search] should run the built-in 'search' when the 'exec' property does not exist",
        body: function () {
            var execDescriptor = Object.getOwnPropertyDescriptor(RegExp.prototype, "exec");
            var setUp = function () {
                delete RegExp.prototype.exec;
            };

            var cleanUp = function () {
                Object.defineProperty(RegExp.prototype, "exec", execDescriptor);
            };

            verifyBuiltInSearchWhenExecIsNotCallable(setUp, cleanUp);
        }
    },
    {
        name: "RegExp.prototype[@@search] should run the built-in 'search' when the 'exec' property is not callable",
        body: function () {
            var setUp = function (re) {
                Object.defineProperty(re, "exec", {value: 0});
            };
            verifyBuiltInSearchWhenExecIsNotCallable(setUp);
        }
    },
    {
        name: "RegExp.prototype[@@search] should return -1 when 'exec' returns 'null'",
        body: function () {
            var re = /./;
            var exec = function (execString) {
                return null;
            };
            Object.defineProperty(re, "exec", {value: exec});
            var result = re[Symbol.search]();
            assert.areEqual(-1, result);
        }
    },
    {
        name: "RegExp.prototype[@@search] should return the 'index' property from the result of the 'exec' call",
        body: function () {
            var re = /./;
            var index = 123;
            var exec = function (execString) {
                return {index: index};
            };
            Object.defineProperty(re, "exec", {value: exec});

            var result = re[Symbol.search]();

            assert.areEqual(index, result);
        }
    },
    {
        name: "RegExp.prototype[@@search] should set 'lastIndex' to '0' before calling 'exec'",
        body: function () {
            var re = /./;
            re.lastIndex = 100;
            var setLastIndexToZero = false;
            var exec = function (execString) {
                setLastIndexToZero = this.lastIndex === 0;
                return null;
            };
            Object.defineProperty(re, "exec", {value: exec});

            re[Symbol.search]()

            assert.isTrue(setLastIndexToZero);
        }
    },
    {
        name: "RegExp.prototype[@@search] should restore 'lastIndex' to its initial value after calling 'exec'",
        body: function () {
            var re = /./;
            var initialLastIndex = 100;
            re.lastIndex = initialLastIndex;

            re[Symbol.search]()

            assert.areEqual(initialLastIndex, re.lastIndex);
        }
    },
    {
        name: "RegExp.prototype[@@search] should throw when 'lastIndex' is not writable",
        body: function () {
            var re = {
                exec: function () {
                    return null;
                },
                get lastIndex() {
                    return 123;
                }
            };
            assert.throws(RegExp.prototype[Symbol.search].bind(re), TypeError);
        }
    },
    {
        name: "RegExp.prototype[@@match] exists",
        body: verifySymbolMethodExistence.bind(undefined, Symbol.match)
    },
    {
        name: "RegExp.prototype[@@match] has the correct name",
        body: verifySymbolMethodName.bind(undefined, "[Symbol.match]", Symbol.match)
    },
    {
        name: "RegExp.prototype[@@match] has the correct length",
        body: verifySymbolMethodLength.bind(undefined, 1, Symbol.match)
    },
    {
        name: "RegExp.prototype[@@match] should run the built-in 'match' when none of the observable properties are overridden",
        body: function () {
            var pattern = "(a)-";

            var nonGlobalRe = new RegExp(pattern);
            var nonGlobalInput = "-a-a-";
            var result = nonGlobalRe[Symbol.match](nonGlobalInput);
            assert.areEqual(1, result.index, "non-global: result.index");
            assert.areEqual("a-", result[0], "non-global: result[0]");
            assert.areEqual("a", result[1], "non-global: result[1]");
            assert.areEqual(nonGlobalInput, result.input, "non-global: result.input");

            var globalRe = new RegExp(pattern, "gy");
            globalRe.lastIndex = 1;
            result = globalRe[Symbol.match]("a-a-aba-");
            assert.areEqual(2, result.length, "global: result.length");
            assert.areEqual("a-", result[0], "global: result[0]");
            assert.areEqual("a-", result[1], "global: result[1]");
        }
    },
    {
        name: "RegExp.prototype[@@match] should 'Get' 'global' when it is overridden",
        body: function () {
            var re = /a-/;
            re.lastIndex = 1; // Will be reset to 0 by RegExp.prototype[@@match]

            var calledGlobal = false;
            var getGlobal = function () {
                calledGlobal = true;
                return true;
            };
            Object.defineProperty(re, "global", {get: getGlobal});

            var result = re[Symbol.match]("a-a-ba-");

            assert.isTrue(calledGlobal, "'global' getter is called");
            assert.areEqual(3, result.length, "result.length");
            assert.areEqual("a-", result[0], "result[0]");
            assert.areEqual("a-", result[1], "result[1]");
            assert.areEqual("a-", result[2], "result[2]");
        }
    },
    {
        name: "RegExp.prototype[@@match] should coerce a missing 'global' to 'false'",
        body: function () {
            var re = /a-/g;
            re.lastIndex = 1; // RegExpBuiltinExec will ignore this and start from 0

            var result;
            helpers.withPropertyDeleted(RegExp.prototype, "global", function () {
                result = re[Symbol.match]("a-a-ba-");
            });

            assert.areEqual(1, result.length, "result.length");
            assert.areEqual("a-", result[0], "result[0]");
        }
    },
    {
        name: "RegExp.prototype[@@match] should 'Get' 'sticky' when it is overridden",
        body: function () {
            var re = /a-/;
            re.lastIndex = 1; // Will be kept at 1

            var calledSticky = false;
            var getSticky = function () {
                calledSticky = true;
                return true;
            };
            Object.defineProperty(re, "sticky", {get: getSticky});

            var result = re[Symbol.match]("a-a-ba-");

            assert.isTrue(calledSticky, "'sticky' getter is called");
            assert.areEqual(null, result, "result");
        }
    },
    {
        name: "RegExp.prototype[@@match] should coerce a missing 'sticky' to 'false'",
        body: function () {
            var re = /a-/y;
            re.lastIndex = 1; // RegExpBuiltinExec will ignore this and start from 0

            var result;
            helpers.withPropertyDeleted(RegExp.prototype, "sticky", function () {
                result = re[Symbol.match]("a-a-ba-");
            });

            assert.areEqual(1, result.length, "result.length");
            assert.areEqual("a-", result[0], "result[0]");
        }
    },
    {
        name: "RegExp.prototype[@@match] should 'Get' 'unicode' when it is overridden",
        body: function () {
            var re = /(?:)/g;

            var calledUnicode = false;
            var getUnicode = function () {
                calledUnicode = true;
                return true;
            };
            Object.defineProperty(re, "unicode", {get: getUnicode});

            var result = re[Symbol.match]("12");

            assert.isTrue(calledUnicode, "'unicode' getter is called");
        }
    },
    {
        name: "RegExp.prototype[@@match] should return what 'exec' returns when 'global' is 'false'",
        body: function () {
            var re = /./;

            var execResult = {
                dummy: "dummy"
            };
            var exec = function () {
                return execResult;
            };
            re.exec = exec;

            var result = re[Symbol.match]("string");

            assert.areEqual(execResult, result);
        }
    },
    {
        name: "RegExp.prototype[@@match] should aggregate results of 'exec' calls when 'global' is 'true'",
        body: function () {
            var re = /./g;

            var execResults = [
                {
                    0: "result 0",
                },
                {
                    0: "result 1",
                },
                null
            ];
            var execResultIndex = 0;
            var exec = function () {
                var result = execResults[execResultIndex];
                ++execResultIndex;
                return result;
            };
            re.exec = exec;

            var result = re[Symbol.match]("string");

            var expectedResult = execResults
                .filter(function (x) { return x !== null; })
                .map(function (x) { return x[0]; });
            assert.areEqual(expectedResult, result);
        }
    },
    {
        name: "String.prototype.match should still update the RegExp constructor with the ES6 logic",
        body: function () {
            var re = /test(.)/;

            // Force the ES6 logic. Otherwise, we go though the ES5 codepath, which already
            // updates the constructor.
            var getGlobal = function () {
                var getterOnPrototype = Object.getOwnPropertyDescriptor(RegExp.prototype, 'global').get;
                return getterOnPrototype.call(this);
            }
            Object.defineProperty(re, "global", {get: getGlobal});

            "test1".match(re);

            assert.areEqual("test1", RegExp.input, "RegExp.input");
            assert.areEqual("1", RegExp.$1, "RegExp.$1");
        }
    }
];
tests = tests.concat(createGenericTestsForSymbol("match", "@@match", Symbol.match));
tests = tests.concat(createGenericTestsForSymbol("search", "@@search", Symbol.search));

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
