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
    }
], { verbose: false });
