//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

// NOTE: \u200e is the U+200E LEFT-TO-RIGHT MARK
var tests = [
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
