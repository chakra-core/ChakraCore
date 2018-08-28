//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const constructors = [Intl.Collator, Intl.NumberFormat, Intl.DateTimeFormat];

if (WScript.Platform.INTL_LIBRARY === "icu") {
    constructors.push(Intl.PluralRules);
}

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
    },
    {
        name: "Intl.FallbackSymbol behavior",
        body() {
            if (WScript.Platform.INTL_LIBRARY === "winglob") {
                return;
            }

            function testFallbackSymbol(Ctor, shouldHaveFallbackSymbol) {
                const objNew = new Ctor();
                const objCall = Ctor.call(objNew);
                const symbols = Object.getOwnPropertySymbols(objCall);
                assert.isTrue(objCall instanceof Ctor, `The given object should be an instance of ${Ctor.name}`);
                assert.areEqual(0, Object.getOwnPropertyNames(objCall).length, "Incorrect number of OwnPropertyNames");
                if (shouldHaveFallbackSymbol) {
                    assert.areEqual(1, symbols.length, "Incorrect number of OwnPropertySymbols");
                    const fallbackSymbol = symbols[0];
                    assert.areEqual("Symbol(Intl.FallbackSymbol)", fallbackSymbol.toString(), "Unexpected symbol description");
                    assert.areEqual("object", typeof objCall[fallbackSymbol], "objCall[fallbackSymbol] should be an object");
                    assert.isTrue(objCall[fallbackSymbol] instanceof Ctor, `objCall[fallbackSymbol] should be an instance of ${Ctor.name}`);

                    assert.throws(() => Ctor.call(objNew), TypeError, "Should not be able to legacy-construct an already-legacy-constructed Intl object (using original non-legacy new object)", "Cannot modify non-writable property 'Intl.FallbackSymbol'");
                    assert.throws(() => Ctor.call(objCall), TypeError, "Should not be able to legacy-construct an already-legacy-constructed Intl object (using legacy .call() object", "Cannot modify non-writable property 'Intl.FallbackSymbol'");

                    assert.areEqual(objNew, objCall, "Object created with .call should return `this`");
                } else {
                    assert.areEqual(0, symbols.length, "Incorrect number of OwnPropertySymbols");
                }
            }

            // only NumberFormat and DateTimeFormat should have Intl.FallbackSymbol behavior. PluralRules has no legacy construction behavior.
            testFallbackSymbol(Intl.Collator, false);
            testFallbackSymbol(Intl.NumberFormat, true);
            testFallbackSymbol(Intl.DateTimeFormat, true);
            assert.throws(() => Intl.PluralRules.call(new Intl.PluralRules()), TypeError, "Intl.PluralRules requires `new`");
        }
    }
], { verbose: false });
