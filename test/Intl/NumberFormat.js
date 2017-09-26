//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function testNumberFormatOptions(opts, number, expected, altExpected) {
    try{
        var options = { minimumFractionDigits: 0, maximumFractionDigits: 0, minimumIntegerDigits: 1, style: "decimal", useGrouping: false };
        for (option in opts) {
            options[option] = opts[option];
        }
        var numberFormatter = new Intl.NumberFormat("en-US", options);
        var actual = numberFormatter.format(number);
        if (actual !== expected && actual != altExpected) {
            WScript.Echo("ERROR: Formatting '" + number + "' with options '" + JSON.stringify(options) + "' resulted in '" + actual + "'; expected '" + expected + "'!");
        }
    }
    catch (ex) {
        WScript.Echo(ex.message);
    }
}

function testNumberFormatOptionsRegex(opts, number, expectedRegex) {
    try{
        var options = { minimumFractionDigits: 0, maximumFractionDigits: 0, minimumIntegerDigits: 1, style: "decimal", useGrouping: false };
        for (option in opts) {
            options[option] = opts[option];
        }
        var numberFormatter = new Intl.NumberFormat("en-US", options);
        var actual = numberFormatter.format(number);
        if (!expectedRegex.test(actual)) {
            WScript.Echo("ERROR: Formatting '" + number + "' with options '" + JSON.stringify(options) + "' resulted in '" + actual + "'; expectedRegex '" + expectedRegex + "'!");
        }
    }
    catch (ex) {
        WScript.Echo(ex.message);
    }
}

function arrayEqual(arr1, arr2) {
    if (arr1.length !== arr2.length) return false;
    for (var i = 0; i < arr1.length; i++) {
        if (arr1[i] !== arr2[i]) return false;
    }
    return true;
}

function testSupportedLocales(locales, expectedResult) {
    try {
        var actual = Intl.NumberFormat.supportedLocalesOf(locales, { localeMatcher: "best fit" });
        if (!arrayEqual(actual, expectedResult)) {
            throw new Error("Calling SupportedLocalesOf on '[" + locales.join(",") + "]' doesn't match expected result '[" + expectedResult.join(",") + "]' when using best fit. Actual:[" + actual.join(",") + "]");
        }
        actual = Intl.NumberFormat.supportedLocalesOf(locales, { localeMatcher: "best fit" });
        if (!arrayEqual(actual, expectedResult)) {
            throw new Error("Calling SupportedLocalesOf on '[" + locales.join(",") + "]' doesn't match expected result '[" + expectedResult.join(",") + "]' when using lookup. Actual: [" + actual.join(",") + "]");
        }
    }
    catch (ex) {
        passed = false;
        WScript.Echo("Error testSupportedLocales: " + ex.message);
    }
}


testNumberFormatOptions({}, 5, "5");
testNumberFormatOptions({ minimumFractionDigits: 2, maximumFractionDigits: 2 }, 5, "5.00");
testNumberFormatOptions({ minimumFractionDigits: 1, maximumFractionDigits: 2 }, 5, "5.0");
testNumberFormatOptions({ minimumFractionDigits: 1, maximumFractionDigits: 2 }, 5.444, "5.44");
testNumberFormatOptions({ minimumIntegerDigits: 2 }, 5, "05");
testNumberFormatOptions({ minimumSignificantDigits: 2 }, 5, "5.0");
testNumberFormatOptions({ minimumSignificantDigits: 2 }, 125, "125");
testNumberFormatOptions({ maximumSignificantDigits: 5 }, 125.123, "125.12");
testNumberFormatOptions({ maximumSignificantDigits: 5 }, 125.125, "125.13");
testNumberFormatOptions({ maximumSignificantDigits: 5 }, -125.125, "-125.13");
testNumberFormatOptions({ useGrouping: true }, 12512, "12,512");
testNumberFormatOptions({ useGrouping: false }, 12512, "12512");
testNumberFormatOptionsRegex({ style: "percent" }, 1.5, new RegExp("150[\x20\u00a0]?%"));
testNumberFormatOptions({ currency: "USD", style: "currency", maximumFractionDigits: 2, minimumFractionDigits: 2 }, 1.5, "$1.50");

testNumberFormatOptions({ minimumIntegerDigits: NaN }, undefined, undefined);
testNumberFormatOptions({ minimumIntegerDigits: "Error" }, undefined, undefined);
testNumberFormatOptions({ minimumIntegerDigits: { foo: "bar" } }, undefined, undefined);
testNumberFormatOptions({ minimumIntegerDigits: null }, undefined, undefined);
testNumberFormatOptions({ minimumFractionDigits: { foo: "bar" } }, undefined, undefined);
testNumberFormatOptions({ minimumFractionDigits: null }, "5", "5");
testNumberFormatOptions({ minimumFractionDigits: NaN }, undefined, undefined);
testNumberFormatOptions({ minimumFractionDigits: "Error" }, undefined, undefined);
testNumberFormatOptions({ maximumFractionDigits: { foo: "bar" } }, undefined, undefined);
testNumberFormatOptions({ maximumFractionDigits: null }, "5", "5");
testNumberFormatOptions({ maximumFractionDigits: NaN }, undefined, undefined);
testNumberFormatOptions({ maximumFractionDigits: "Error" }, undefined, undefined);
testNumberFormatOptions({ minimumSignificantDigits: { foo: "bar" } }, undefined, undefined);
testNumberFormatOptions({ minimumSignificantDigits: null }, undefined, undefined);
testNumberFormatOptions({ minimumSignificantDigits: NaN }, undefined, undefined);
testNumberFormatOptions({ minimumSignificantDigits: "Error" }, undefined, undefined);
testNumberFormatOptions({ maximumSignificantDigits: { foo: "bar" } }, undefined, undefined);
testNumberFormatOptions({ maximumSignificantDigits: null }, undefined, undefined);
testNumberFormatOptions({ maximumSignificantDigits: NaN }, undefined, undefined);
testNumberFormatOptions({ maximumSignificantDigits: "Error" }, undefined, undefined);

testSupportedLocales(undefined, []);
testSupportedLocales(["en-US"], ["en-US"]);
testSupportedLocales([], []);
testSupportedLocales(["xxx"], []);
