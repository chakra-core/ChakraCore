//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

// NOTE: \u200e is the U+200E LEFT-TO-RIGHT MARK
var tests = [
    {
        name: "Test Valid Options",
        body: function () {
            assert.areEqual(new Intl.DateTimeFormat("en-US", { year: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e2000", "Formatting year as numeric.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { year: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e00", "Formatting year as 2-digit.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e2", "Formatting month as numeric.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e02", "Formatting month as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "long" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200eFebruary", "Formatting month as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "short" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200eFeb", "Formatting month as numeric.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "narrow" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200eFeb", "Formatting month as narrow.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { day: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01", "Formatting day as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { day: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1", "Formatting day as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e \u200eAM", "Formatting hour as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1\u200e \u200eAM", "Formatting hour as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit and minute as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit and minute as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "2-digit", second: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit, minute as 2-digit and second as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "2-digit", second: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit, minute as 2-digit and second as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "numeric", hour12: true }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1\u200e \u200eAM", "Formatting hour as numeric with hour12=true.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "numeric", hour12: false }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1\u200e:\u200e00", "Formatting hour as numeric with hour12=false.");
        }
    },
    {
        name: "Test Invalid Options",
        body: function () {
            function verifyDTFException(locale, option, invalidValue, validValues) {
                if (validValues.indexOf(invalidValue) !== -1) {
                    assert.fail("Test constructed incorrectly.");
                }
                try {
                    // Since minute and second aren't supported alone; doing this to prevent that exception.
                    var options = { hour: "numeric", minute: "numeric" };
                    options[option] = invalidValue;
                    new Intl.DateTimeFormat(locale, options);
                    assert.fail("Exception was expected. Option: " + option + "; invalid value: " + invalidValue);
                }
                catch (e) {
                    if (!(e instanceof RangeError)) {
                        assert.fail("Incorrect exception was thrown.");
                    }
                    assert.isTrue(e.message.indexOf(validValues) !== -1,
                        "Checking exception message for correct values string. Looking for: " + validValues +
                        "\nMessage: " + e.message);
                }
            }

            verifyDTFException("en-US", "year", "long", "['2-digit', 'numeric']");
            verifyDTFException("en-US", "month", "false", "['2-digit', 'numeric', 'narrow', 'short', 'long']");
            verifyDTFException("en-US", "day", "long", "['2-digit', 'numeric']");
            verifyDTFException("en-US", "hour", "long", "['2-digit', 'numeric']");
            verifyDTFException("en-US", "minute", "long", "['2-digit', 'numeric']");
            verifyDTFException("en-US", "second", "long", "['2-digit', 'numeric']");
            verifyDTFException("en-US", "era", "numeric", "['narrow', 'short', 'long']");
            verifyDTFException("en-US", "localeMatcher", "long", "['lookup', 'best fit']");
            verifyDTFException("en-US", "formatMatcher", "long", "['basic', 'best fit']");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "numeric", hour12: "asdasd" }).resolvedOptions().hour12, true, "Hour12 special invalid option treatment.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
