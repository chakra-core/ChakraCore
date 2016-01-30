//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function verifyRegExpObjectWhenExecIsNotCallable(createRegExp) {
    var re = createRegExp();
    assert.throws(RegExp.prototype[Symbol.search].bind(re), TypeError);
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

function verifyStringSearchRequiresObjectCoercibleThis(thisObj) {
    assert.throws(String.prototype.search.bind(thisObj), TypeError);
}

function verifyBuiltInSymbolSearch(createRegExp) {
    var toStringValue = "search string";
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

    var searchDescriptor = Object.getOwnPropertyDescriptor(RegExp.prototype, Symbol.search);
    var result;
    try {
        var search = function (searchString) {
            callCount += 1;
            passedANewRegEx = this !== re && this instanceof RegExp;
            coercedString = searchString === string;
            return index;
        }
        Object.defineProperty(RegExp.prototype, Symbol.search, {value: search});

        result = String.prototype.search.call(string, re);
    }
    finally {
        Object.defineProperty(RegExp.prototype, Symbol.search, searchDescriptor);
    }

    assert.areEqual(index, result, "result");
    assert.areEqual(1, callCount, "'@@search' call count");
    assert.isTrue(passedANewRegEx, "A new RegExp is created");
    assert.isTrue(coercedString, "'string' argument is coerced to String");
}

var tests = [
    {
        name: "RegExp.prototype[@@search] exists",
        body: function () {
            assert.isTrue(Symbol.search in RegExp.prototype);

            var descriptor = Object.getOwnPropertyDescriptor(RegExp.prototype, Symbol.search);
            assert.isTrue(descriptor.configurable, "descriptor.configurable");
            assert.isTrue(descriptor.writable, "descriptor.writable");
            assert.isFalse(descriptor.enumerable, "descriptor.enumerable");
        }
    },
    {
        name: "RegExp.prototype[@@search] has the correct name",
        body: function () {
            var searchFunction = RegExp.prototype[Symbol.search];

            assert.areEqual("[Symbol.search]", searchFunction.name, "name");

            var descriptor = Object.getOwnPropertyDescriptor(searchFunction, "name");
            assert.isTrue(descriptor.configurable, "descriptor.configurable");
            assert.isFalse(descriptor.writable, "descriptor.writable");
            assert.isFalse(descriptor.enumerable, "descriptor.enumerable");
        }
    },
    {
        name: "RegExp.prototype[@@search] has the correct length",
        body: function () {
            var searchFunction = RegExp.prototype[Symbol.search];

            assert.areEqual(1, searchFunction.length);

            var descriptor = Object.getOwnPropertyDescriptor(searchFunction, "length");
            assert.isTrue(descriptor.configurable, "descriptor.configurable");
            assert.isFalse(descriptor.writable, "descriptor.writable");
            assert.isFalse(descriptor.enumerable, "descriptor.enumerable");
        }
    },
    {
        name: "RegExp.prototype[@@search] should throw an exception when 'this' isn't an Object",
        body: function () {
            var nonObject = "string";
            assert.throws(RegExp.prototype[Symbol.search].bind(nonObject, ""), TypeError);
        }
    },
    {
        name: "RegExp.prototype[@@search] should be callable when 'this' is an ordinary Object and it has 'exec'",
        body: function () {
            var object = {
                exec: function () {
                    return null;
                }
            };
            assert.doesNotThrow(RegExp.prototype[Symbol.search].bind(object, ""));
        }
    },
    {
        name: "RegExp.prototype[@@search] should expect 'this' to be a RegExp object when 'exec' property does not exist",
        body: function () {
            var createRegExp = function () {
                return {};
            };
            verifyRegExpObjectWhenExecIsNotCallable(createRegExp);
        }
    },
    {
        name: "RegExp.prototype[@@search] should expect 'this' to be a RegExp object when 'exec' is not callable",
        body: function () {
            var createRegExp = function () {
                return {exec: 0};
            };
            verifyRegExpObjectWhenExecIsNotCallable(createRegExp);
        }
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
        name: "RegExp.prototype[@@search] should delegate to 'exec'",
        body: function () {
            var re = /./;
            var string = "search string";
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

            re[Symbol.search](string);

            assert.areEqual(1, callCount, "'exec' call count");
            assert.isTrue(passedCorrectThisObject, "'this' is correct");
            assert.isTrue(passedCorrectString, "'string' argument is correct");
        }
    },
    {
        name: "RegExp.prototype[@@search] should coerce the 'string' argument to String",
        body: function () {
            var re = /./;
            var toStringValue = "search string";
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

            re[Symbol.search](string);

            assert.isTrue(coercedString);
        }
    },
    {
        name: "RegExp.prototype[@@search] should use the String 'undefined' when the 'string' argument is missing",
        body: function () {
            var re = /./;
            var coercedString = false;
            var exec = function (execString) {
                coercedString = execString === "undefined";
                return null;
            };
            Object.defineProperty(re, "exec", {value: exec});

            re[Symbol.search]();

            assert.isTrue(coercedString);
        }
    },
    {
        name: "RegExp.prototype[@@search] should throw when return value of 'exec' is not an Object or 'null'",
        body: function () {
            var re = /./;
            var exec = function (execString) {
                return undefined;
            };
            Object.defineProperty(re, "exec", {value: exec});
            assert.throws(RegExp.prototype[Symbol.search].bind(re), TypeError);
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
        name: "String.prototype.search should throw when 'this' is 'undefined'",
        body: verifyStringSearchRequiresObjectCoercibleThis.bind(undefined),
    },
    {
        name: "String.prototype.search should throw when 'this' is 'null'",
        body: verifyStringSearchRequiresObjectCoercibleThis.bind(null),
    },
    {
        name: "String.prototype.search should delegate to '@@search' property of the 'regexp' argument",
        body: function () {
            var string = "this string";
            var index = 123;
            var callCount = 0;
            var passedCorrectThisObject = false;
            var passedCorrectString = false;
            var re = {
                [Symbol.search]: function (searchString) {
                    callCount += 1;
                    passedCorrectThisObject = this === re;
                    passedCorrectString = searchString === string;

                    return index;
                }
            };

            var result = string.search(re);

            assert.areEqual(index, result, "result");
            assert.areEqual(1, callCount, "'@@search' call count");
            assert.isTrue(passedCorrectThisObject, "'this' is correct");
            assert.isTrue(passedCorrectString, "'string' argument is correct");
        }
    },
    {
        name: "String.prototype.search should run the built-in '@@search' when the '@@search' property of the 'regexp' argument is 'undefined'",
        body: function () {
            function createRegExp() {
                var re = /./;
                re[Symbol.search] = undefined;
                return re;
            }
            verifyBuiltInSymbolSearch(createRegExp);
        }
    },
    {
        name: "String.prototype.search should run the built-in '@@search' when the 'regexp' argument is 'undefined'",
        body: function () {
            function createRegExp() {
                return undefined;
            }
            verifyBuiltInSymbolSearch(createRegExp);
        }
    },
    {
        name: "String.prototype.search should run the built-in '@@search' when the 'regexp' argument is 'null'",
        body: function () {
            function createRegExp() {
                return null;
            }
            verifyBuiltInSymbolSearch(createRegExp);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
