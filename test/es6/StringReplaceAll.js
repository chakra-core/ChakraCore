//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests for String.prototype.replaceAll (ES2021)

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "String.prototype.replaceAll should exist",
        body: function () {
            assert.isTrue(String.prototype.hasOwnProperty('replaceAll'), "String.prototype should have replaceAll method");
            assert.areEqual(2, String.prototype.replaceAll.length, "replaceAll should have length 2");
        }
    },
    {
        name: "String.prototype.replaceAll should replace all occurrences with string",
        body: function () {
            assert.areEqual("foo-foo-foo", "bar-bar-bar".replaceAll("bar", "foo"), "Basic string replacement");
            assert.areEqual("Hello World", "Hello World".replaceAll("foo", "bar"), "No matches - returns original");
            assert.areEqual("aXbXc", "a-b-c".replaceAll("-", "X"), "Single char replacement");
            assert.areEqual("foobar", "foobar".replaceAll("", "X"), "Empty search string should not add X at beginning");
        }
    },
    {
        name: "String.prototype.replaceAll should replace all occurrences with function",
        body: function () {
            var result = "hello world".replaceAll("o", function(match, offset, string) {
                assert.areEqual("o", match, "match should be 'o'");
                assert.isTrue(typeof offset === "number", "offset should be a number");
                assert.areEqual("hello world", string, "string should be the original");
                return "0";
            });
            assert.areEqual("hell0 w0rld", result, "Function replacement should work");
        }
    },
    {
        name: "String.prototype.replaceAll should work with RegExp with global flag",
        body: function () {
            assert.areEqual("foo-bar-foo-bar", "abc-abc-abc-abc".replaceAll(/abc/g, "foo"), "RegExp with g flag should work");
            assert.areEqual("XaXbXcX", "a-b-c-".replaceAll(/-/g, "X"), "RegExp with g flag");
        }
    },
    {
        name: "String.prototype.replaceAll should throw for RegExp without global flag",
        body: function () {
            var f = "test".replaceAll.bind("test", /./, "X");
            assert.throws(f, TypeError, "RegExp without g flag should throw TypeError");
        }
    },
    {
        name: "String.prototype.replaceAll should throw when first argument is undefined",
        body: function () {
            var f = "test".replaceAll.bind("test", undefined);
            assert.throws(f, TypeError, "undefined search value should throw TypeError");
        }
    },
    {
        name: "String.prototype.replaceAll should handle special replacement patterns",
        body: function () {
            assert.areEqual("test$test", "testest".replaceAll("es", "$$"), "$$ should produce $");
            assert.areEqual("testmatchedtest", "testes".replaceAll("es", "$&"), "$& should produce matched string");
            assert.areEqual("testpre", "testes".replaceAll("es", "$`"), "$` should produce prefix");
            assert.areEqual("testpost", "testes".replaceAll("es", "$'"), "$' should produce suffix");
        }
    },
    {
        name: "String.prototype.replaceAll should handle empty search string",
        body: function () {
            // Empty string matches between every character including start and end
            var result = "ab".replaceAll("", "-");
            assert.areEqual("-a-b-", result, "Empty search should insert at each position");
        }
    },
    {
        name: "String.prototype.replaceAll should handle overlapping matches correctly",
        body: function () {
            // After replacement, search continues from after the replaced text
            assert.areEqual("XXX", "aaa".replaceAll("aa", "X"), "aa -> X on aaa should give XXX");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] == "verbose" });
