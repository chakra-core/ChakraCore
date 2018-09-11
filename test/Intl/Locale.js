//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

testRunner.runTests([
    {
        name: "Basic functionality",
        body() {
            assert.areEqual("Locale", Intl.Locale.name);
            const locale = new Intl.Locale("en");
            assert.areEqual("en", locale.toString());
            assert.areEqual("[object Intl.Locale]", Object.prototype.toString.call(locale));
        },
    },
    {
        name: "Applying options",
        body() {
            function test(expected, langtag, options) {
                const locale = new Intl.Locale(langtag, options);
                assert.areEqual(expected, locale.toString());
            }

            test("es", "es");
            test("de", "en", { language: "de" });
            test("de-Latn-DE-u-ca-chinese", "en", { language: "de", script: "Latn", region: "DE", calendar: "chinese" });
            test("ar-u-co-unihan", "ar-u-co-unihan");
            test("ar-u-co-unihan", "ar", { collation: "unihan" });
        }
    },
    {
        name: "Using an existing Locale object for the langtag argument",
        body() {
            const enUS = new Intl.Locale("en-US");
            assert.areEqual("en", enUS.language);
            assert.areEqual("US", enUS.region);

            const enGB = new Intl.Locale(enUS, { region: "GB" });
            assert.areEqual("en", enGB.language);
            assert.areEqual("GB", enGB.region);

            const deGB = new Intl.Locale(enGB, { language: "de" });
            assert.areEqual("de", deGB.language);
            assert.areEqual("GB", deGB.region);

            const deLatnGB = new Intl.Locale(deGB, { script: "Latn" });
            assert.areEqual("de", deLatnGB.language);
            assert.areEqual("GB", deLatnGB.region);
            assert.areEqual("Latn", deLatnGB.script);

            const dePhonebk = new Intl.Locale("de-u-co-phonebk");
            assert.areEqual("de", dePhonebk.language);
            assert.areEqual("phonebk", dePhonebk.collation);

            const deUnihan = new Intl.Locale(dePhonebk, { collation: "unihan" });
            assert.areEqual("de", deUnihan.language);
            assert.areEqual("unihan", deUnihan.collation);

            const esUnihanH24 = new Intl.Locale(deUnihan, { language: "es", hourCycle: "h24" });
            assert.areEqual("es", esUnihanH24.language);
            assert.areEqual("unihan", esUnihanH24.collation);
            assert.areEqual("h24", esUnihanH24.hourCycle);
        }
    },
    {
        name: "Maximizing and minimizing",
        body() {
            function test(input, expected, minimal, maximal) {
                const locale = new Intl.Locale(input);
                assert.areEqual(expected, locale.toString(), `Incorrect canonicalization of ${input}`);
                assert.areEqual(minimal, locale.minimize().toString(), `Incorrect minimization of ${input}`);
                assert.areEqual(maximal, locale.maximize().toString(), `Incorrect maximization of ${input}`);
            }

            test("en", "en", "en", "en-Latn-US");
            test("DE-de", "de-DE", "de", "de-Latn-DE");

            // cases inspired by the examples in https://www.unicode.org/reports/tr35/#Likely_Subtags
            test("zh", "zh", "zh", "zh-Hans-CN");
            test("zh-HanT", "zh-Hant", "zh-Hant", "zh-Hant-TW");
            test("zh-MO", "zh-MO", "zh-MO", "zh-Hant-MO");
            test("ZH-Hant-TW", "zh-Hant-TW", "zh-TW", "zh-Hant-TW");
            test("und-Hant", "und-Hant", "und-Hant", "zh-Hant-TW");

            // the UTS35 example says the maximized version should be fa-Arab-AF?
            test("und-Arab-AF", "und-Arab-AF", "und-Arab-AF", "ar-Arab-AF");

            // Chakra performs incorrect canonicalization, so the following cases don't pass.
            // test("sh-Arab-AQ", "sr-Arab-AQ", "sr-Arab-AQ", "sr-Arab-AQ");
            // test("ZH-ZZZZ-SG", "zh-SG", "zh-SG", "zh-Hans-SG");
        }
    },
], { verbose: false })
