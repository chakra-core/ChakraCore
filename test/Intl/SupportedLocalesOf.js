//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Test Correct Errors",
        body: function () {
            assert.throws(function () { new Intl.Collator.supportedLocalesOf(); }, TypeError, "", "Function 'Intl.Collator.supportedLocalesOf' is not a constructor");
            assert.throws(function () { new Intl.NumberFormat.supportedLocalesOf(); }, TypeError, "", "Function 'Intl.NumberFormat.supportedLocalesOf' is not a constructor");
            assert.throws(function () { new Intl.DateTimeFormat.supportedLocalesOf(); }, TypeError, "", "Function 'Intl.DateTimeFormat.supportedLocalesOf' is not a constructor");

            const rangeErrorMessage = "Option value 'incorrect' for 'localeMatcher' is outside of valid range. Expected: ['best fit', 'lookup']";
            assert.throws(function () { Intl.Collator.supportedLocalesOf(["en-US"], { localeMatcher: "incorrect" }) }, RangeError, "", rangeErrorMessage);
            assert.throws(function () { Intl.NumberFormat.supportedLocalesOf(["en-US"], { localeMatcher: "incorrect" }) }, RangeError, "", rangeErrorMessage);
            assert.throws(function () { Intl.DateTimeFormat.supportedLocalesOf(["en-US"], { localeMatcher: "incorrect" }) }, RangeError, "", rangeErrorMessage);

            assert.throws(function () { Intl.Collator.supportedLocalesOf(null) }, TypeError, "", "Object expected");
            assert.throws(function () { Intl.NumberFormat.supportedLocalesOf(null) }, TypeError, "", "Object expected");
            assert.throws(function () { Intl.DateTimeFormat.supportedLocalesOf(null) }, TypeError, "", "Object expected");

            assert.throws(function () {
                var locales = { get length() { throw new Error("Intentional throw"); } };
                Intl.Collator.supportedLocalesOf(locales);
                console.log("Intentional throw didn't throw.");
            }, Error, "", "Intentional throw");
        }
    },
    {
        name: "",
        body: function () {
            const ctors = [Intl.Collator, Intl.NumberFormat, Intl.DateTimeFormat];

            function test(ctor) {
                assert.areEqual(ctor.supportedLocalesOf(["en"]).toString(), "en");
                assert.areEqual(ctor.supportedLocalesOf(["en"], { localeMatcher: "lookup" }).toString(), "en-US");
                assert.areEqual(ctor.supportedLocalesOf(["en"], { localeMatcher: "best fit" }).toString(), "en");
                assert.areEqual(ctor.supportedLocalesOf().length, 0);
                assert.areEqual(ctor.supportedLocalesOf(undefined, { localeMatcher: "lookup" }).length, 0);
                assert.areEqual(ctor.supportedLocalesOf(undefined, { localeMatcher: "best fit" }).length, 0);
                assert.areEqual(ctor.supportedLocalesOf.call({}, ["en"]).toString(), "en");
                assert.areEqual(ctor.supportedLocalesOf.call({}, ["en"], { localeMatcher: "lookup" }).toString(), "en-US");
                assert.areEqual(ctor.supportedLocalesOf.call({}, ["en"], { localeMatcher: "best fit" }).toString(), "en");
                assert.areEqual(ctor.supportedLocalesOf.bind({})(["en"]).toString(), "en");
                assert.areEqual(ctor.supportedLocalesOf.bind({})(["en"], { localeMatcher: "lookup" }).toString(), "en-US");
                assert.areEqual(ctor.supportedLocalesOf.bind({})(["en"], { localeMatcher: "best fit" }).toString(), "en");
            }

            ctors.forEach(test);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
