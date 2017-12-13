//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Basic tests",
        body: function () {
            const date = new Date(2000, 1, 1, 1, 1, 1);
            function equal(options, expected, message) {
                const result = new Intl.DateTimeFormat("en-US", options)
                    .format(date)
                    .replace(/[^\x00-\x7F]/g, ""); // replace bi-di markers and other unicode characters from output
                assert.areEqual(expected, result, message);
            }

            function matches(options, expected, message) {
                const result = new Intl.DateTimeFormat("en-US", options)
                    .format(date)
                    .replace(/[^\x00-\x7F]/g, ""); // replace bi-di markers and other unicode characters from output
                assert.matches(expected, result, message);
            }

            equal({ year: "numeric" }, "2000", "Formatting year as numeric");
            equal({ year: "2-digit" }, "00", "Formatting year as 2-digit.");

            equal({ month: "numeric" }, "2", "Formatting month as numeric.");
            equal({ month: "2-digit" }, "02", "Formatting month as 2-digit.");
            equal({ month: "long" }, "February", "Formatting month as 2-digit.");
            equal({ month: "short" }, "Feb", "Formatting month as numeric.");
            matches({ month: "narrow" }, /F(eb)?/, "Formatting month as narrow."); // WinGlob narrow is Feb, ICU narrow is F

            equal({ day: "2-digit" }, "01", "Formatting day as 2-digit.");
            equal({ day: "numeric" }, "1", "Formatting day as numeric.");

            matches({ hour: "2-digit" }, /0?1 AM/, "Formatting hour as 2-digit."); // ICU 2-digit hour skeleton (jj) doesn't transform into hh/HH correctly in en-US
            equal({ hour: "numeric" }, "1 AM", "Formatting hour as numeric.");

            equal({ hour: "numeric", minute: "2-digit" }, "1:01 AM", "Formatting hour as numeric and minute as 2-digit.");
            equal({ hour: "numeric", minute: "numeric" }, "1:01 AM", "Formatting hour as numeric and minute as numeric.");

            equal({ hour: "numeric", minute: "2-digit", second: "2-digit" }, "1:01:01 AM", "Formatting hour as numeric, minute as 2-digit and second as 2-digit.");
            matches({ hour: "numeric", minute: "2-digit", second: "numeric" }, /1:01:0?1 AM/, "Formatting hour as numeric, minute as 2-digit and second as numeric."); // WinGlob doesn't have non-2-digit seconds

            equal({ hour: "numeric", hour12: true }, "1 AM", "Formatting hour as numeric with hour12=true.");
            matches({ hour: "numeric", hour12: false }, /(1:00|01)/, "Formatting hour as numeric with hour12=false.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
