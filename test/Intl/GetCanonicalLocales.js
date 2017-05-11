//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Intl.getCanonicalLocales Functionality",
        body: function () {
            const DE_U_INCORRECT = 'de-de-u-kn-true-co-phonebk';
            const DE_U_CORRECT = 'de-DE-u-co-phonebk-kn-true';

            // basics: ensure array, canonicalize each, remove duplicates
            assert.areEqual(Intl.getCanonicalLocales('en'), ['en'], "Input is a singleton string (not an array) -> output is array");
            assert.areEqual(Intl.getCanonicalLocales(['en']), ['en'], "Input matches output, no lookup is performed");
            assert.areEqual(Intl.getCanonicalLocales(['en-us']), ['en-US'], "Canonicalize country casing (en-US) (all-lowercase)");
            assert.areEqual(Intl.getCanonicalLocales(['en-Us']), ['en-US'], "Canonicalize country casing (en-US) (mixed-case)");
            assert.areEqual(Intl.getCanonicalLocales(['EN-us']), ['en-US'], "Canonicalize country casing (en-US) (completely incorrect casing)");
            assert.areEqual(Intl.getCanonicalLocales(['de-de']), ['de-DE'], "Canonicalize country casing (de-DE)");

            // canonicalization of script code subkey
            assert.areEqual(Intl.getCanonicalLocales(['zh-hans-cn']), ['zh-Hans-CN'], "Chinese (zh) Han Simplified (Hans) as used in China (CN)");
            assert.areEqual(Intl.getCanonicalLocales(['zh-hant-hk']), ['zh-Hant-HK'], "Chinese (zh) Han Traditional (Hant) as used in Hong Kong (HK)");

            // canonicalization of -u- extension keys
            assert.areEqual(Intl.getCanonicalLocales(DE_U_INCORRECT), [DE_U_CORRECT], "Casing and reordering keys (input string)");
            assert.areEqual(Intl.getCanonicalLocales([DE_U_INCORRECT]), [DE_U_CORRECT], "Casing and reordering keys (input singleton)");
            assert.areEqual(Intl.getCanonicalLocales(['en-us', DE_U_INCORRECT]), ['en-US', DE_U_CORRECT], "Casing and reordering keys (input multiple)");

            // no duplicates
            assert.areEqual(Intl.getCanonicalLocales(['en-us', 'en-us']), ['en-US'], "No duplicates, same input casing (casing was incorrect)");
            assert.areEqual(Intl.getCanonicalLocales(['en-US', 'en-US']), ['en-US'], "No duplicates, same input casing (casing was correct)");
            assert.areEqual(Intl.getCanonicalLocales(['en-us', 'en-US']), ['en-US'], "No duplicates, different input casing");

            // locale includes all options, don't de-dupe locales with and without options, but do de-dupe same options after canonicalization
            assert.areEqual(Intl.getCanonicalLocales(['de-de', DE_U_CORRECT, DE_U_INCORRECT]), ['de-DE', DE_U_CORRECT],
                "de-dupe canonicalized locales, but not locales with and without options");
        }
    },
    {
        name: "Intl.getCanonicalLocales handling of unsupported tags and subtags (general canonicalization)",
        // Intl.getCanonicalLocales does not care whether a locale tag is supported.
        // It simply canonicalizes all properly formatted (i.e. "valid") tags.
        // Therefore, anything that fits into the general language tag grammar should be canonicalized.
        // * ECMA 402 #sec-isstructurallyvalidlanguagetag
        // * (Note: The above basically just refers to RFC 5646 section 2.1)
        body: function () {
            assert.areEqual(Intl.getCanonicalLocales('en-zz'), ['en-ZZ'], "en-ZZ: English as used in [unsupported locale ZZ]");
            assert.areEqual(Intl.getCanonicalLocales('ZZ-us'), ['zz-US'], "zz-US: [unsupported language zz] as used in US");
            assert.areEqual(Intl.getCanonicalLocales('xx-abcd-zz'), ['xx-Abcd-ZZ'],
                "xx-Abcd-ZZ: [unsupported language xx] using [unsupported script Abcd] as used in [unsupported locale ZZ]");
            // TODO extension keys
        }
    },
    {
        name: "Structurally invalid tags",
        // * ECMA 402 #sec-isstructurallyvalidlanguagetag
        // * (Note: The above basically just refers to RFC 5646 section 2.1)
        body: function () {
            function test(tag) {
                assert.throws(function () { Intl.getCanonicalLocales(tag) }, RangeError,
                    `Tag '${tag}' should throw RangeError`,
                    `Locale '${tag}' is not well-formed`);
            }

            const invalidChars = ['en-A1', 'en-a@'];
            const boundaryHyphen = ['-en', '-en-us', 'en-', 'en-us-'];
            const incompleteSubtags = ['de-de-u'];
            const regionTooLong = ['xx-zzz', 'xx-abcd-zzz'];

            invalidChars.forEach(test);
            boundaryHyphen.forEach(test);
            incompleteSubtags.forEach(test);
            regionTooLong.forEach(test);

            // TODO

            // TODO move or remove comment
            // Locales must be formatted like aa-AA[-u-xx-xxvalue[-yy-value]
        }
    },
    {
        name: "Intl.getCanonicalLocales wrong input",
        body: function () {
            // According to the definition of Intl.getCanonicalLocales, if locales is undefined, return []
            assert.areEqual(Intl.getCanonicalLocales(), [], "Implicit undefined");
            assert.areEqual(Intl.getCanonicalLocales(undefined), [], "Explicit undefined");

            // There is no special case for null in the definition, so throw TypeError
            assert.throws(function () { Intl.getCanonicalLocales(null) }, TypeError, "Cannot convert null to object.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
