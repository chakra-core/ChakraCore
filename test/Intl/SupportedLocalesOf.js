//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const ctors = [Intl.Collator, Intl.NumberFormat, Intl.DateTimeFormat];
const tests = [
    {
        name: "supportedLocalesOf throws correct errors",
        body: function () {
            const rangeErrorMessage = "Option value 'incorrect' for 'localeMatcher' is outside of valid range. Expected: ['best fit', 'lookup']";
            const fakeLocales = { get length() { throw new Error("User-provided locale object throws"); } };

            function test(ctor) {
                assert.throws(() => new ctor.supportedLocalesOf(), TypeError, "", `Function 'Intl.${ctor.name}.supportedLocalesOf' is not a constructor`);
                assert.throws(() => ctor.supportedLocalesOf(["en-US"], { localeMatcher: "incorrect" }), RangeError, "", rangeErrorMessage);
                assert.throws(() => ctor.supportedLocalesOf(null), TypeError, "", "Object expected");
                assert.throws(() => ctor.supportedLocalesOf(fakeLocales), Error, "", "User-provided locale object throws");
            }

            ctors.forEach(test);
        }
    },
    {
        name: "supportedLocalesOf basic tests",
        body: function () {
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
