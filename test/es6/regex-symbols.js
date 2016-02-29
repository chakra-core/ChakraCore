//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function arrayEquals(array1, array2) {
    if (array1.length !== array2.length) {
        return false;
    }

    var equals = true;
    for (var i = 0; equals && i < array1.length; ++i) {
        equals = equals && array1[i] === array2[i];
    }
    return equals;
}

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

function verifyBuiltInSymbolMethod(stringPropertyName, additionalArguments, symbolName, symbol, createRegExp) {
    var toStringValue = "string value";
    var string = {
        toString: function () {
            return toStringValue;
        }
    };
    var symbolResult = 123;
    var callCount = 0;
    var passedANewRegEx = false;
    var coercedString = false;
    var passedAdditionalArguments = true;
    var re = createRegExp();

    var methodDescriptor = Object.getOwnPropertyDescriptor(RegExp.prototype, symbol);
    var result;
    try {
        var method = function (stringArg, ...rest) {
            callCount += 1;
            passedANewRegEx = this !== re && this instanceof RegExp;
            coercedString = stringArg === string;
            passedAdditionalArguments = arrayEquals(rest, additionalArguments);
            return symbolResult;
        }
        Object.defineProperty(RegExp.prototype, symbol, {value: method});

        result = String.prototype[stringPropertyName].apply(string, [re].concat(additionalArguments));
    }
    finally {
        Object.defineProperty(RegExp.prototype, symbol, methodDescriptor);
    }

    assert.areEqual(symbolResult, result, "result");
    assert.areEqual(1, callCount, "'" + symbolName + "' call count");
    assert.isTrue(passedANewRegEx, "A new RegExp is created");
    assert.isTrue(coercedString, "'string' argument is coerced to String");
    assert.isTrue(passedAdditionalArguments, "additional arguments are passed");
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

    assert.areEqual(expectedLength, func.length);

    var descriptor = Object.getOwnPropertyDescriptor(func, "length");
    assert.isTrue(descriptor.configurable, "descriptor.configurable");
    assert.isFalse(descriptor.writable, "descriptor.writable");
    assert.isFalse(descriptor.enumerable, "descriptor.enumerable");
}

function verifySymbolMethodRequiresThisToBeObject(symbol) {
    var nonObject = "string";
    assert.throws(RegExp.prototype[symbol].bind(nonObject, ""), TypeError);
}

function withObservableRegExpSymbolSplit(callback) {
    var originalExec = RegExp.prototype.exec;
    helpers.withPropertyDeleted(RegExp.prototype, "exec", function () {
        var exec = function () {
            return originalExec.apply(this, arguments);
        }
        Object.defineProperty(RegExp.prototype, "exec", {value: exec, configurable: true});

        callback();
    });
}

function verifySymbolSplitResult(assertResult, re, ...args) {
    withObservableRegExpSymbolSplit(function () {
        var result = re[Symbol.split](...args);

        assert.isTrue(result instanceof Array, "result type");
        assertResult(result);
    });
}

function getFullSymbolMethodName(symbolName) {
    return "RegExp.prototype[" + symbolName + "]";
}

function createTestsForMethodProperties(functionName, functionLength, symbolName, symbol) {
    var fullSymbolMethodName = getFullSymbolMethodName(symbolName);
    return [
        {
            name: fullSymbolMethodName + " exists",
            body: verifySymbolMethodExistence.bind(undefined, symbol)
        },
        {
            name: fullSymbolMethodName + " has the correct name",
            body: verifySymbolMethodName.bind(undefined, functionName, symbol)
        },
        {
            name: fullSymbolMethodName + " has the correct length",
            body: verifySymbolMethodLength.bind(undefined, functionLength, symbol)
        },
    ];
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
                    },

                    flags: "", // Needed by RegExp.prototype[@@split]
                };
                assert.doesNotThrow(RegExp.prototype[symbol].bind(object, ""));
            }
        },
    ];
}

function createTestsForRegExpTypeWhenInvalidRegExpExec(symbolName, symbol) {
    var fullSymbolMethodName = getFullSymbolMethodName(symbolName);
    return [
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

function testThisSameRegExp(thisObj, re) {
    return thisObj === re;
}

function testThisNewRegExp(thisObj, re) {
    return thisObj !== re && thisObj instanceof RegExp;
}

function createTestsForSymbolToExecDelegation(testThis, symbolName, symbol) {
    var fullSymbolMethodName = getFullSymbolMethodName(symbolName);
    return [
        {
            name: fullSymbolMethodName + " should delegate to 'exec'",
            body: function () {
                helpers.withPropertyDeleted(RegExp.prototype, "exec", function () {
                    var re = /./;
                    var string = "string argument";
                    var called = true;
                    var passedCorrectThisObject = false;
                    var passedCorrectString = false;
                    var exec = function (execString) {
                        called = true;
                        passedCorrectThisObject = testThis(this, re);
                        passedCorrectString = execString === string;
                        return null;
                    };
                    Object.defineProperty(RegExp.prototype, "exec", {value: exec, configurable: true});

                    re[symbol](string);

                    assert.isTrue(called, "'exec' is called");
                    assert.isTrue(passedCorrectThisObject, "'this' is correct");
                    assert.isTrue(passedCorrectString, "'string' argument is correct");
                });
            }
        },
        {
            name: fullSymbolMethodName + " should throw when return value of 'exec' is not an Object or 'null'",
            body: function () {
                helpers.withPropertyDeleted(RegExp.prototype, "exec", function () {
                    var re = /./;
                    var exec = function (execString) {
                        return undefined;
                    };
                    Object.defineProperty(RegExp.prototype, "exec", {value: exec, configurable: true});
                    assert.throws(RegExp.prototype[symbol].bind(re), TypeError);
                });
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
                helpers.withPropertyDeleted(RegExp.prototype, "exec", function () {
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
                    Object.defineProperty(RegExp.prototype, "exec", {value: exec, configurable: true});

                    re[symbol](string);

                    assert.isTrue(coercedString);
                });
            }
        },
        {
            name: fullSymbolMethodName + " should use the String 'undefined' when the 'string' argument is missing",
            body: function () {
                helpers.withPropertyDeleted(RegExp.prototype, "exec", function () {
                    var re = /./;
                    var coercedString = false;
                    var exec = function (execString) {
                        coercedString = execString === "undefined";
                        return null;
                    };
                    Object.defineProperty(RegExp.prototype, "exec", {value: exec, configurable: true});

                    re[symbol]();

                    assert.isTrue(coercedString);
                });
            }
        },
    ];
}

function createTestsForStringToRegExpDelegation(stringPropertyName, additionalArguments, symbolName, symbol) {
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
                var symbolResult = 123;
                var callCount = 0;
                var passedCorrectThisObject = false;
                var passedCorrectString = false;
                var passedCorrectAdditionalArguments = true;
                var re = {
                    [symbol]: function (stringArg, ...rest) {
                        callCount += 1;
                        passedCorrectThisObject = this === re;
                        passedCorrectString = stringArg === string;
                        passedCorrectAdditionalArguments = arrayEquals(rest, additionalArguments);

                        return symbolResult;
                    }
                };

                var result = string[stringPropertyName](re, ...additionalArguments);

                assert.areEqual(symbolResult, result, "result");
                assert.areEqual(1, callCount, "'" + symbolName + "' call count");
                assert.isTrue(passedCorrectThisObject, "'this' is correct");
                assert.isTrue(passedCorrectString, "'string' argument is correct");
                assert.isTrue(passedCorrectAdditionalArguments, "additional arguments are correct");
            }
        },
    ];
}

function createTestsForBuiltInSymbolMethod(stringPropertyName, additionalArguments, symbolName, symbol) {
    var fullStringPropertyName = "String.prototype." + stringPropertyName;
    return [
        {
            name: fullStringPropertyName + " should run the built-in '" + symbolName + "' when the '" + symbolName + "' property of the 'regexp' argument is 'undefined'",
            body: function () {
                function createRegExp() {
                    var re = /./;
                    re[symbol] = undefined;
                    return re;
                }
                verifyBuiltInSymbolMethod(stringPropertyName, additionalArguments, symbolName, symbol, createRegExp);
            }
        },
        {
            name: fullStringPropertyName + " should run the built-in '" + symbolName + "' when the 'regexp' argument is 'undefined'",
            body: function () {
                function createRegExp() {
                    return undefined;
                }
                verifyBuiltInSymbolMethod(stringPropertyName, additionalArguments, symbolName, symbol, createRegExp);
            }
        },
        {
            name: fullStringPropertyName + " should run the built-in '" + symbolName + "' when the 'regexp' argument is 'null'",
            body: function () {
                function createRegExp() {
                    return null;
                }
                verifyBuiltInSymbolMethod(stringPropertyName, additionalArguments, symbolName, symbol, createRegExp);
            }
        },
    ];
}

function createGenericTestsForSymbol(stringPropertyName, functionName, functionLength, additionalArguments, symbolName, symbol) {
    return [].concat(createTestsForMethodProperties(functionName, functionLength, symbolName, symbol))
             .concat(createTestsForThisObjectType(symbolName, symbol))
             .concat(createTestsForStringCoercion(symbolName, symbol))
             .concat(createTestsForStringToRegExpDelegation(stringPropertyName, additionalArguments, symbolName, symbol));
}

var tests = [
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
    },
    {
        name: "RegExp.prototype[@@split] should run the built-in 'split' when none of the observable properties are overridden",
        body: function () {
            var pattern = "-";
            var input = "-a-b--c-";

            function verify(assertMessagePrefix, re) {
                var result = re[Symbol.split](input);

                assert.areEqual(6, result.length, assertMessagePrefix + ": result.length");
                assert.areEqual("", result[0], assertMessagePrefix + ": result[0]");
                assert.areEqual("a", result[1], assertMessagePrefix + ": result[1]");
                assert.areEqual("b", result[2], assertMessagePrefix + ": result[2]");
                assert.areEqual("", result[3], assertMessagePrefix + ": result[3]");
                assert.areEqual("c", result[4], assertMessagePrefix + ": result[4]");
                assert.areEqual("", result[5], assertMessagePrefix + ": result[5]");
            }

            verify("non-sticky", new RegExp(pattern));
            verify("sticky", new RegExp(pattern, "y"));
        }
    },
    {
        name: "RegExp.prototype[@@split] should 'Get' 'flags' when it is overridden",
        body: function () {
            var re = /-/;

            var calledFlags = false;
            var getFlags = function () {
                calledFlags = true;
                return "";
            };
            Object.defineProperty(re, "flags", {get: getFlags});

            re[Symbol.split]("");

            assert.isTrue(calledFlags, "'flags' getter is called");
        }
    },
    {
        name: "RegExp.prototype[@@split] should construct a new RegExp using Symbol.species",
        body: function () {
            var re = /./i;

            var ctorCalled = false;
            var ctorThis = undefined;
            var ctorArguments = undefined;
            var ctorResult = /different regexp/i
            re.constructor = function () {}
            re.constructor[Symbol.species] = function () {
                ctorCalled = true;
                ctorThis = this;
                ctorArguments = arguments;

                return ctorResult;
            }

            re[Symbol.split]("123");

            assert.isTrue(ctorCalled, "constructor is called");
            assert.areEqual(re, ctorArguments[0], "constructor is passed the original RegExp object");
            assert.areEqual("iy", ctorArguments[1], "constructor is passed the correct flags (including 'y')");
        }
    },
    {
        name: "RegExp.prototype[@@split] should return an empty Array when the input size is 0 and the RegExp matches the empty string",
        body: function () {
            function assertResult(result) {
                assert.areEqual(0, result.length, "result.length");
            }
            var re = /(?:)/;
            var input = "";
            verifySymbolSplitResult(assertResult, re, input);
        }
    },
    {
        name: "RegExp.prototype[@@split] shouldn'\t return an empty Array when the input size is 0 and the RegExp doesn't match the empty string",
        body: function () {
            function assertResult(result) {
                assert.areEqual(1, result.length, "result.length");
                assert.areEqual("", result[0], "result[0]");
            }
            var re = /./;
            var input = "";
            verifySymbolSplitResult(assertResult, re, input);
        }
    },
    {
        name: "RegExp.prototype[@@split] should advance the string index when the input size is > 0 and the RegExp matches the empty string",
        body: function () {
            function assertResult(result) {
                assert.areEqual(2, result.length, "result.length");
                assert.areEqual("a", result[0], "result[0]");
                assert.areEqual("b", result[1], "result[1]");
            }
            var re = /(?:)/;
            var input = "ab";
            verifySymbolSplitResult(assertResult, re, input);
        }
    },
    {
        name: "RegExp.prototype[@@split] should ignore the matched parts of the input when the input size is > 0 and the RegExp doesn't match the empty string",
        body: function () {
            function assertResult(result) {
                assert.areEqual(4, result.length, "result.length");
                assert.areEqual("", result[0], "result[0]");
                assert.areEqual("a", result[1], "result[1]");
                assert.areEqual("b", result[2], "result[2]");
                assert.areEqual("", result[3], "result[3]");
            }
            var re = /-/;
            var input = "-a-b-";
            verifySymbolSplitResult(assertResult, re, input);
        }
    },
    {
        name: "RegExp.prototype[@@split] should include the capturing groups in the result",
        body: function () {
            function assertResult(result) {
                assert.areEqual(5, result.length, "result.length");
                assert.areEqual("-", result[0], "result[0]");
                assert.areEqual("a", result[1], "result[1]");
                assert.areEqual("b", result[2], "result[2]");
                assert.areEqual("c", result[3], "result[3]");
                assert.areEqual("-", result[4], "result[4]");
            }
            var re = /(a)(b)(c)/;
            var input = "-abc-";
            verifySymbolSplitResult(assertResult, re, input);
        }
    },
    {
        name: "RegExp.prototype[@@split] should stop at limit when there are no capturing groups",
        body: function () {
            function assertResult(result) {
                assert.areEqual(2, result.length, "result.length");
                assert.areEqual("a", result[0], "result[0]");
                assert.areEqual("b", result[1], "result[1]");
            }
            var re = /-/;
            var input = "a-b-c";
            var limit = 2;
            verifySymbolSplitResult(assertResult, re, input, limit);
        }
    },
    {
        name: "RegExp.prototype[@@split] should stop at limit when there are capturing groups",
        body: function () {
            function assertResult(result) {
                assert.areEqual(3, result.length, "result.length");
                assert.areEqual("-", result[0], "result[0]");
                assert.areEqual("a", result[1], "result[1]");
                assert.areEqual("b", result[2], "result[2]");
            }
            var re = /(a)(b)(c)/;
            var input = "-abc-";
            var limit = 3;
            verifySymbolSplitResult(assertResult, re, input, limit);
        }
    },
];
tests = tests.concat(
    // match
    createGenericTestsForSymbol("match", "[Symbol.match]", 1, [], "@@match", Symbol.match),
    createTestsForRegExpTypeWhenInvalidRegExpExec("@@match", Symbol.match),
    createTestsForBuiltInSymbolMethod("match", [], "@@match", Symbol.match),
    createTestsForSymbolToExecDelegation(testThisSameRegExp, "@@match", Symbol.match),

    // search
    createGenericTestsForSymbol("search", "[Symbol.search]", 1, [], "@@search", Symbol.search),
    createTestsForRegExpTypeWhenInvalidRegExpExec("@@search", Symbol.search),
    createTestsForBuiltInSymbolMethod("search", [], "@@search", Symbol.search),
    createTestsForSymbolToExecDelegation(testThisSameRegExp, "@@search", Symbol.search),

    // split
    createGenericTestsForSymbol("split", "[Symbol.split]", 2, [/* limit */ 123], "@@split", Symbol.split),
    createTestsForSymbolToExecDelegation(testThisNewRegExp, "@@split", Symbol.split),
);

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
