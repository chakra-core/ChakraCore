//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const constructors = [Intl.Collator, Intl.NumberFormat, Intl.DateTimeFormat];

testRunner.runTests([
    {
        name: "OSS-Fuzz #6657: stress uloc_forLanguageTag status code and parsed length on duplicate variant subtags",
        body() {
            function test(Ctor, locale) {
                assert.throws(() => new Ctor(locale), RangeError, `new Intl.${Ctor.name}("${locale}") should throw`);
            }

            function testWithVariants(variants) {
                for (const Ctor of constructors) {
                    for (let i = 0; i < variants.length; ++i) {
                        for (let k = 0; k < variants.length; ++k) {
                            for (let m = 0; m < variants.length; ++m) {
                                test(Ctor, `und-${variants[i]}-${variants[k]}-${variants[m]}`);
                                test(Ctor, `en-${variants[i]}-${variants[k]}-${variants[m]}`);
                                test(Ctor, `de-DE-${variants[i]}-${variants[k]}-${variants[m]}`);
                                test(Ctor, `zh-Hans-CN-${variants[i]}-${variants[k]}-${variants[m]}`);
                            }

                            test(Ctor, `und-${variants[i]}-${variants[k]}`);
                            test(Ctor, `en-${variants[i]}-${variants[k]}`);
                            test(Ctor, `de-DE-${variants[i]}-${variants[k]}`);
                            test(Ctor, `zh-Hans-CN-${variants[i]}-${variants[k]}`);
                        }
                    }
                }
            }

            if (WScript.Platform.INTL_LIBRARY === "icu") {
                testWithVariants(["gregory", "GREGORY", "gregORY"]);
                testWithVariants(["invalid", "INVALID", "invaLID"]);
            }
        }
    },
    {
        name: "OSS-Fuzz #7950: Have option getters redefine themselves",
        body() {
            if (WScript.Platform.INTL_LIBRARY === "winglob") {
                return;
            }

            function test(callback, optionName, optionValue, shouldCallSecondGetter) {
                const options = {};
                let calledSecondGetter = false;
                Object.defineProperty(options, optionName, {
                    get() {
                        Object.defineProperty(options, optionName, {
                            get() {
                                calledSecondGetter = true;
                                return undefined;
                            }
                        });

                        return optionValue;
                    },
                    configurable: true
                });

                callback(options);
                assert.areEqual(shouldCallSecondGetter, calledSecondGetter, "Second getter behavior was incorrect");
            }

            test((options) => assert.areEqual(1, new Intl.Collator("en-US", options).compare("A", "a")), "sensitivity", "case", false);
            test((options) => assert.areEqual(-1, new Intl.Collator("en-US", options).compare("A", "B")), "sensitivity", "case", false);
            test((options) => assert.areEqual(0, new Intl.Collator("en-US", options).compare("a", "\u00E2")), "sensitivity", "case", false);
            test((options) => assert.areEqual("1000", new Intl.NumberFormat("en-US", options).format(1000)), "useGrouping", false, false);
            test((options) => assert.areEqual("$1.50", new Intl.NumberFormat("en-US", Object.assign(options, { currency: "USD" })).format(1.5)), "style", "currency", false);

            // This was the original bug - at present, all browsers format the string as "" because the value returned by the second getter dictates format selection
            test((options) => assert.areEqual("", new Intl.DateTimeFormat("en-US", options).format()), "year", "numeric", true);
            test((options) => assert.areEqual("", new Intl.DateTimeFormat("en-US", options).format()), "minute", "numeric", true);
        }
    }
], { verbose: false });
