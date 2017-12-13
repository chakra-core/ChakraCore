//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

// remove non-ascii characters from strings, mostly for stripping Bi-Di markers
const nonAsciiRegex = /[^\x00-\x7F]/g;
function ascii (str) {
    return str.replace(nonAsciiRegex, "");
}

function check(expected, actual, message) {
    if (expected instanceof RegExp) {
        assert.matches(expected, actual, message);
    } else {
        assert.areEqual(expected, actual, message);
    }
}

const tests = [
    {
        name: "Basic functionality",
        body: function () {
            const date = new Date(2000, 1, 1, 1, 1, 1);
            function test(options, expected, message) {
                check(expected, ascii(new Intl.DateTimeFormat("en-US", options).format(date)), message);
            }

            test({ year: "numeric" }, "2000", "Formatting year as numeric");
            test({ year: "2-digit" }, "00", "Formatting year as 2-digit.");

            test({ month: "numeric" }, "2", "Formatting month as numeric.");
            test({ month: "2-digit" }, "02", "Formatting month as 2-digit.");
            test({ month: "long" }, "February", "Formatting month as 2-digit.");
            test({ month: "short" }, "Feb", "Formatting month as numeric.");
            test({ month: "narrow" }, /F(eb)?/, "Formatting month as narrow."); // WinGlob narrow is Feb, ICU narrow is F

            test({ day: "2-digit" }, "01", "Formatting day as 2-digit.");
            test({ day: "numeric" }, "1", "Formatting day as numeric.");

            test({ hour: "2-digit" }, /0?1 AM/, "Formatting hour as 2-digit."); // ICU 2-digit hour skeleton (jj) doesn't transform into hh/HH correctly in en-US
            test({ hour: "numeric" }, "1 AM", "Formatting hour as numeric.");

            test({ hour: "numeric", minute: "2-digit" }, "1:01 AM", "Formatting hour as numeric and minute as 2-digit.");
            test({ hour: "numeric", minute: "numeric" }, "1:01 AM", "Formatting hour as numeric and minute as numeric.");

            test({ hour: "numeric", minute: "2-digit", second: "2-digit" }, "1:01:01 AM", "Formatting hour as numeric, minute as 2-digit and second as 2-digit.");
            test({ hour: "numeric", minute: "2-digit", second: "numeric" }, /1:01:0?1 AM/, "Formatting hour as numeric, minute as 2-digit and second as numeric."); // WinGlob doesn't have non-2-digit seconds

            test({ hour: "numeric", hour12: true }, "1 AM", "Formatting hour as numeric with hour12=true.");
            test({ hour: "numeric", hour12: false }, /(1:00|01)/, "Formatting hour as numeric with hour12=false.");
        }
    },
    {
        name: "Options resolution",
        body: function () {
            function test(locale, options, expected, message) {
                expected = Object.assign({}, {
                    locale: /en/,
                    numberingSystem: "latn",
                    calendar: "gregory",
                    timeZone: /.+/
                }, expected);
                const actual = new Intl.DateTimeFormat(locale, options).resolvedOptions();
                for (const key in expected) {
                    if (expected[key] !== null) {
                        check(expected[key], actual[key], message);
                    } else {
                        assert.isFalse(actual.hasOwnProperty(key), `${message} - ${key} should not be present in ${JSON.stringify(actual, null, 2)}`);
                    }
                }
            }

            test("en-US", undefined,           { year: "numeric", month: "numeric", day: "numeric" }, "Default options do not match");
            test("en-US", { year: "numeric" }, { year: "numeric", month: null, day: null }, "Requesting year should not fill in other date or time options");
            test("en-US", { hour: "numeric" }, { hour: "numeric", minute: null, month: null }, "Requesting hour should not fill in other date or time options");
            test("en-US", { hour12: false },   { hour12: null }, "Requesting hour12 without hour shouldn't do anything")
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
