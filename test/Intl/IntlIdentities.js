//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

let staticMethods = [
    Intl.getCanonicalLocales,
    Intl.Collator.supportedLocalesOf,
    Intl.DateTimeFormat.supportedLocalesOf,
    Intl.NumberFormat.supportedLocalesOf
];
let longNames = [
    "Intl.getCanonicalLocales",
    "Intl.Collator.supportedLocalesOf",
    "Intl.DateTimeFormat.supportedLocalesOf",
    "Intl.NumberFormat.supportedLocalesOf"
];
let shortNames = [
    "getCanonicalLocales",
    "supportedLocalesOf",
    "supportedLocalesOf",
    "supportedLocalesOf"
];

let expectedToString =
`function() {
    [native code]
}`;

let tests = [
    {
        name: "Short names",
        body: function () {
            for (let i in staticMethods) {
                assert.areEqual(staticMethods[i].name, shortNames[i]);
            }
        }
    },
    {
        name: "Invoking built-in static methods with `new` fails (check name in error message)",
        body: function () {
            for (let i in staticMethods) {
                assert.throws(() => new staticMethods[i](), TypeError, "", `Function '${longNames[i]}' is not a constructor`);
            }
        }
    },
    {
        name: "toString of built-in static methods",
        body: function () {
            for (let i in staticMethods) {
                assert.areEqual('' + staticMethods[i], expectedToString);
                assert.areEqual(staticMethods[i].toString(), expectedToString);
            }
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
