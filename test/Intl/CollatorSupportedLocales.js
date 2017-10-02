//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var passed = true;

function arrayEqual(arr1, arr2) {
    if (arr1.length !== arr2.length) return false;
    for (var i = 0; i < arr1.length; i++) {
        if (arr1[i] !== arr2[i]) return false;
    }
    return true;
}

function testSupportedLocales(locales, expectedResult) {
    try {
        var actual = Intl.Collator.supportedLocalesOf(locales, { localeMatcher: "best fit" });
        if (!arrayEqual(actual, expectedResult)) {
            throw new Error("Calling SupportedLocalesOf on '[" + locales.join(",") + "]' doesn't match expected result '[" + expectedResult.join(",") + "]' when using best fit. Actual:[" + actual.join(",") + "]");
        }
        actual = Intl.Collator.supportedLocalesOf(locales, { localeMatcher: "best fit" });
        if (!arrayEqual(actual, expectedResult)) {
            throw new Error("Calling SupportedLocalesOf on '[" + locales.join(",") + "]' doesn't match expected result '[" + expectedResult.join(",") + "]' when using lookup. Actual: [" + actual.join(",") + "]");
        }
    }
    catch (ex) {
        passed = false;
        WScript.Echo("Error testSupportedLocales: " + ex.message);
    }
}

testSupportedLocales(undefined, []);
testSupportedLocales(["en-US"], ["en-US"]);
testSupportedLocales([], []);

if (passed === true) {
    WScript.Echo("Pass");
}
