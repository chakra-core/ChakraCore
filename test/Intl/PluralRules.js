//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testData(locale, ordinals, cardinals) {
    const plOrdinal = new Intl.PluralRules(locale, { type: "ordinal" });
    const plCardinal = new Intl.PluralRules(locale, { type: "cardinal" });
    assert.areEqual(locale, plOrdinal.resolvedOptions().locale);
    assert.areEqual(locale, plCardinal.resolvedOptions().locale);

    ordinals.forEach((ordinal, i) => ordinal !== undefined && assert.areEqual(ordinal, plOrdinal.select(i), `Selecting ${i} for locale ${locale} with ordinal type`));
    cardinals.forEach((cardinal, i) => cardinal !== undefined && assert.areEqual(cardinal, plCardinal.select(i), `Selecting ${i} for locale ${locale} with cardinal type`));
}

testRunner.runTests([
    {
        name: "Basic resolved options",
        body() {
            const pr = new Intl.PluralRules("en");
            const opts = pr.resolvedOptions();
            assert.areEqual("string", typeof opts.locale, "Locale should be the default locale");
            assert.areEqual("cardinal", opts.type, "Default type should be cardinal");
            assert.areEqual(1, opts.minimumIntegerDigits, "Default minimumIntegerDigits should be 1");
            assert.areEqual(0, opts.minimumFractionDigits, "Default minimumFractionDigits should be 0");
            assert.areEqual(3, opts.maximumFractionDigits, "Default maximumFractionDigits should be 3");

            const acceptableCategories = ["zero", "one", "two", "few", "many", "other"];
            assert.isTrue(Array.isArray(opts.pluralCategories), "pluralCategories should always be an array");
            opts.pluralCategories.forEach((category) => assert.isTrue(acceptableCategories.includes(category)));

            if (WScript.Platform.ICU_VERSION >= 61) {
                // In ICU 61+, uplrules_getKeywords became stable, so we can actually use it
                // REVIEW(jahorto): In all locales, there should be at least a singular and plural category?
                assert.isTrue(opts.pluralCategories.length >= 2, "pluralCategories should use uplrules_getKeywords");
            } else {
                assert.isTrue(opts.pluralCategories.length === 1, "pluralCategories should not use uplrules_getKeywords");
            }
        }
    },
    {
        name: "pluralCategories",
        body() {
            assert.areEqual("en-US", Intl.PluralRules.supportedLocalesOf("en-US")[0], "PluralRules must support en-US for these tests");
            function test(pr, categories) {
                if (WScript.Platform.ICU_VERSION >= 61) {
                    const resolvedCategories = pr.resolvedOptions().pluralCategories;
                    assert.areEqual(categories.length, resolvedCategories.length);
                    for (const c of resolvedCategories) {
                        assert.isTrue(categories.includes(c));
                    }
                }

                for (let i = 0; i < 15; i++) {
                    assert.isTrue(categories.includes(pr.select(i)), `Incorrect value for select(${i})`);
                }
            }

            test(new Intl.PluralRules("en-US"), ["other", "one"]);
            test(new Intl.PluralRules("en-US", { type: "ordinal" }), ["few", "one", "two", "other"]);
        }
    },
    {
        name: "Welsh data tests",
        body() {
            const ordinals = ["zero", "one", "two", "few", "few", "many", "many", "zero", "zero", "zero", "other"];
            const cardinals = ["zero", "one", "two", "few", "other", "other", "many", "other", "other", "other"];

            testData("cy", ordinals, cardinals);
        }
    },
    {
        name: "Slovenian data tests",
        body() {
            const ordinals = Array(10).fill("other");
            const cardinals = ["other", "one", "two", "few", "few", "other"];
            cardinals[10] = "other";
            cardinals[11] = "other";
            cardinals[12] = "other";
            cardinals[13] = "other";
            cardinals[14] = "other";
            cardinals[15] = "other";
            cardinals[100] = "other";
            cardinals[101] = "one";
            cardinals[102] = "two";
            cardinals[103] = "few";
            cardinals[104] = "few";
            cardinals[105] = "other";

            testData("sl", ordinals, cardinals);
        }
    },
    {
        name: "https://github.com/tc39/ecma402/issues/224",
        body() {
            const pr1 = new Intl.PluralRules();
            const pr2 = new Intl.PluralRules();
            const opts1 = pr1.resolvedOptions();
            const opts2 = pr2.resolvedOptions();

            assert.isFalse(opts1.pluralCategories === opts2.pluralCategories, "Different PluralRules instances should have different category array instances");
            opts1.pluralCategories.forEach((cat, i) => {
                assert.areEqual(cat, opts2.pluralCategories[i], "Different PluralRules instances should have the same category array contents");
            });

            assert.isFalse(opts1.pluralCategories === pr1.resolvedOptions().pluralCategories, "Calling resolvedOptions again on the same PluralRules instance should return a new array instance");
            opts1.pluralCategories[0] = "changed";
            assert.areNotEqual(opts1.pluralCategories[0], pr1.resolvedOptions().pluralCategories[0], "Changing the pluralCategories from one call to resolvedOptions should not impact future calls");
        }
    },
    {
        name: "Number digit options",
        body() {
            function test(options, n, expected) {
                const pr = new Intl.PluralRules("en", options);
                const res = pr.resolvedOptions();
                assert.areEqual(expected, pr.select(n), `Incorrect result using n = ${n} and options = ${JSON.stringify(options)}`);

                // We should only report sigfigs in the resolved options if they were asked for https://github.com/tc39/ecma402/issues/244
                if (options.minimumSignificantDigits !== undefined || options.maximumSignificantDigits !== undefined) {
                    if (options.minimumSignificantDigits !== undefined) {
                        assert.areEqual(options.minimumSignificantDigits, res.minimumSignificantDigits, "Incorrect minimumSignificantDigits");
                    }
                    if (options.maximumSignificantDigits !== undefined) {
                        assert.areEqual(options.maximumSignificantDigits, res.maximumSignificantDigits, "Incorrect maximumSignificantDigits");
                    }
                } else {
                    assert.isFalse(res.hasOwnProperty("minimumSignificantDigits"), "Reported minimumSignificantDigits when it shouldn't have been");
                    assert.isFalse(res.hasOwnProperty("maximumSignificantDigits"), "Reported maximumSignificantDigits when it shouldn't have been");
                }
            }

            test({}, 1.0, "one");
            test({}, 1.1, "other");
            test({}, 1.001, "other");

            test({ minimumFractionDigits: 1 }, 1.0, "one");
            test({ minimumFractionDigits: 1 }, 1.1, "other");
            test({ minimumFractionDigits: 1 }, 1.001, "other");

            test({ maximumFractionDigits: 0 }, 1.0, "one");
            test({ maximumFractionDigits: 0 }, 1.1, "one");
            test({ maximumFractionDigits: 0 }, 1.001, "one");

            test({ maximumFractionDigits: 1 }, 1.0, "one");
            test({ maximumFractionDigits: 1 }, 1.1, "other");
            test({ maximumFractionDigits: 1 }, 1.001, "one");

            test({ minimumSignificantDigits: 2 }, 1.0, "one");
            test({ minimumSignificantDigits: 2 }, 1.1, "other");
            test({ minimumSignificantDigits: 2 }, 1.001, "other");

            test({ maximumSignificantDigits: 2 }, 1.0, "one");
            test({ maximumSignificantDigits: 2 }, 1.1, "other");
            test({ maximumSignificantDigits: 2 }, 1.001, "one");

            test({ maximumSignificantDigits: 1 }, 1.0, "one");
            test({ maximumSignificantDigits: 1 }, 1.1, "one");
            test({ maximumSignificantDigits: 1 }, 1.001, "one");

            // significantDigits should override fractionDigits and integerDigits
            test({ maximumSignificantDigits: 2, maximumFractionDigits: 0 }, 1.1, "other");
        }
    },
], { verbose: false });
