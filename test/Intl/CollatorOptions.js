//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

// NOTE: \u00C0 is U+00C0 LATIN CAPITAL LETTER A WITH GRAVE
// NOTE: \u00E4 is U+00E4 LATIN SMALL LETTER A WITH DIAERESIS
var tests = [
    {
        name: "Test Valid Options Resolution",
        body: function () {
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "variant" }).resolvedOptions().sensitivity, "variant", "Ensure that variant sensitivity is interpreted correctly.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "case" }).resolvedOptions().sensitivity, "case", "Ensure that case sensitivity is interpreted correctly.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "accent" }).resolvedOptions().sensitivity, "accent", "Ensure that accent sensitivity is interpreted correctly.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "base" }).resolvedOptions().sensitivity, "base", "Ensure that base sensitivity is interpreted correctly.");

            assert.areEqual(new Intl.Collator("de-DE", { collation: "phonebk" }).resolvedOptions().collation, "default", "Ensure that collation option is ignored.");
            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk").resolvedOptions().collation, "phonebk", "Ensure that collation unicode extension is interpreted correctly (with absent options).");
            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", undefined).resolvedOptions().collation, "phonebk", "Ensure that collation unicode extension is interpreted correctly (with undefined options).");
            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", {}).resolvedOptions().collation, "phonebk", "Ensure that collation unicode extension is interpreted correctly (with empty options).");

            assert.areEqual(new Intl.Collator("de-DE").resolvedOptions().numeric, false, "Ensure numeric set to false implicitly.");
            assert.areEqual(new Intl.Collator("de-DE", { numeric: false }).resolvedOptions().numeric, false, "Ensure numeric set to false explicitly.");
            assert.areEqual(new Intl.Collator("de-DE", { numeric: true }).resolvedOptions().numeric, true, "Ensure numeric set to true.");

            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", { numeric: false }).resolvedOptions().collation, "phonebk", "Mixed -u-co-phonebk and numeric option false (collation).");
            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", { numeric: false }).resolvedOptions().numeric, false, "Mixed -u-co-phonebk and numeric option false (numeric).");
            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", { numeric: true }).resolvedOptions().collation, "phonebk", "Mixed -u-co-phonebk and numeric option true (collation).");
            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", { numeric: true }).resolvedOptions().numeric, true, "Mixed -u-co-phonebk and numeric option true (numeric).");

            assert.areEqual(new Intl.Collator("de-DE-u-kn-true").resolvedOptions().numeric, true, "Ensure that -u-kn-true is interpreted correctly.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-false").resolvedOptions().numeric, false, "Ensure that -u-kn-false is interpreted correctly.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-true-co-phonebk").resolvedOptions().numeric, true, "Ensure that -u-kn-true-co-phonebk is interpreted correctly (numeric).");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-true-co-phonebk").resolvedOptions().collation, "phonebk", "Ensure that -u-kn-true-co-phonebk is interpreted correctly (collation).");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-false-co-phonebk").resolvedOptions().numeric, false, "Ensure that -u-kn-false-co-phonebk is interpreted correctly (numeric).");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-false-co-phonebk").resolvedOptions().collation, "phonebk", "Ensure that -u-kn-false-co-phonebk is interpreted correctly (collation).");
        }
    },
    {
        name: "Test Valid Options Behavior",
        body: function () {
            assert.areEqual(new Intl.Collator("en-US").compare("a", "A"), -1, "Comparing with default options.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "variant" }).compare("a", "A"), -1, "Comparing with variant sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "variant" }).compare("\u00C0", "A"), 1, "Comparing with variant sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "variant" }).compare("a", "b"), -1, "Comparing with variant sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "case" }).compare("a", "A"), -1, "Comparing with case sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "case" }).compare("a", "b"), -1, "Comparing with case sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "case" }).compare("\u00C0", "A"), 0, "Comparing with case sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "accent" }).compare("\u00C0", "A"), 1, "Comparing with accent sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "accent" }).compare("a", "A"), 0, "Comparing with accent sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "accent" }).compare("a", "b"), -1, "Comparing with accent sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "base" }).compare("a", "A"), 0, "Comparing with base sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "base" }).compare("\u00C0", "A"), 0, "Comparing with base sensitivity.");
            assert.areEqual(new Intl.Collator("en-US", { sensitivity: "base" }).compare("a", "b"), -1, "Comparing with base sensitivity.");

            assert.areEqual(new Intl.Collator("de-DE", { collation: "phonebk" }).compare("\u00e4b", "ada"), -1, "Comparing with default collation, option ignored.");
            assert.areEqual(new Intl.Collator("de-DE", { collation: "phonebk" }).compare("äb", "ada"), -1, "Comparing with default collation, option ignored (using explicit ä character).");

            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", {}).compare("\u00e4b", "ada"), 1, "Comparing with collation unicode extension phonebk.");
            assert.areEqual(new Intl.Collator("de-DE-u-co-phonebk", {}).compare("äb", "ada"), 1, "Comparing with collation unicode extension phonebk (using explicit ä character).");
            assert.areEqual(new Intl.Collator("de-DE", {}).compare("\u00e4b", "ada"), -1, "Comparing without collation option of phonebk.");
            assert.areEqual(new Intl.Collator("de-DE", {}).compare("äb", "ada"), -1, "Comparing without collation option of phonebk (using explicit ä character).");

            assert.areEqual(new Intl.Collator("de-DE", { numeric: true }).compare("21", "100"), -1, "Comparing with numeric option set to true.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-true", {}).compare("21", "100"), -1, "Comparing with numeric unicode extension set to true.");
            assert.areEqual(new Intl.Collator("de-DE", { numeric: false }).compare("21", "100"), 1, "Comparing with numeric option set to false.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-false", {}).compare("21", "100"), 1, "Comparing with numeric unicode extension set to false.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-true-co-phonebk", {}).compare("21", "100"), -1, "Comparing with collation set to phonebk and numeric set to true.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-true-co-phonebk", {}).compare("\u00e4b", "ada"), 1, "Comparing with collation set to phonebk and numeric set to true.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-false-co-phonebk", {}).compare("21", "100"), 1, "Comparing with collation set to phonebk and numeric set to false.");
            assert.areEqual(new Intl.Collator("de-DE-u-kn-false-co-phonebk", {}).compare("\u00e4b", "ada"), 1, "Comparing with collation set to phonebk and numeric set to false.");
            assert.areEqual(new Intl.Collator("en-US", { ignorePunctuation: true }).compare("aa", "a!a"), 0, "Comparing with ignore punctuation set to true.");
            assert.areEqual(new Intl.Collator("en-US", { ignorePunctuation: false }).compare("aa", "a!a"), 1, "Comparing with ignore punctuation set to true.");
        }
    },
    {
        name: "Test Invalid Options",
        body: function () {
            function verifyCollatorException(locale, options, expectingInvalidOption, validValuesStr) {
                try {
                    //Since minute and second aren't supported alone; doing this to prevent that exception.
                    new Intl.Collator(locale, options);
                    assert.fail("Exception was expected. Option: " + expectingInvalidOption + "; options passed in: " + JSON.stringify(options));
                }
                catch (e) {
                    if (!(e instanceof RangeError || e instanceof TypeError)) {
                        assert.fail("Incorrect exception was thrown.");
                    }
                    assert.isTrue(e.message.indexOf(validValuesStr) !== -1,
                        "Exception didn't have the correct valid values when testing option:" + expectingInvalidOption +
                        ".\nMessage: " + e.message +
                        "\nSearched For:" + validValuesStr);
                }
            }

            verifyCollatorException("en-US-u-kf-invalid", {}, "caseFirst", "['upper', 'lower', 'false']");
            verifyCollatorException("en-US", { caseFirst: "invalid" }, "caseFirst", "['upper', 'lower', 'false']");

            assert.areEqual(new Intl.Collator("en-US", { numeric: "blah" }).resolvedOptions().numeric, true, "Testing invalid numeric option.");
            assert.areEqual(new Intl.Collator("en-US-u-kn-blah", {}).resolvedOptions().numeric, false, "Testing invalid numeric option.");
            assert.areEqual(new Intl.Collator("en-US", { ignorePunctuation: "blah" }).resolvedOptions().ignorePunctuation, true, "Testing invalid ignorePunctuation option.");
            assert.areEqual(new Intl.Collator("en-US", { collation: "blah" }).resolvedOptions().collation, "default", "Testing invalid collation option.");
            assert.areEqual(new Intl.Collator("en-US-u-co-blah", {}).resolvedOptions().collation, "default", "Testing invalid colation option.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
