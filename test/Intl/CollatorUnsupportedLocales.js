//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testSupportedLocales(locales, expected, localeMatcher) {
    return function body() {
        const actual = Intl.Collator.supportedLocalesOf(locales, { localeMatcher });
        assert.areEqual(
            actual,
            expected,
            `supportedLocalesOf(${JSON.stringify(locales)}, { localeMatcher: ${localeMatcher}}) returned ${JSON.stringify(actual)}, which does not equal expected value ${JSON.stringify(expected)}.`
        );
    };
}

const tests = [
    {
        name: "Unsupported locale with best fit matcher",
        body: testSupportedLocales(["xxx"], [], "best fit")
    },
    {
        name: "Unsupported locale with lookup matcher",
        body: testSupportedLocales(["xxx"], [], "lookup")
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
