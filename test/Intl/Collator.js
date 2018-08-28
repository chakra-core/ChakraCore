//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

testRunner.runTests([
    {
        name: "Basic compare behavior",
        body() {
            function test(locale, options, left, right, expected) {
                const msg = `Comparing ${left} and ${right} with options ${JSON.stringify(options)} and locale ${locale}`;
                assert.areEqual(expected, new Intl.Collator(locale, options).compare(left, right), `${msg} using Intl.Collator`);
                assert.areEqual(expected, left.localeCompare(right, locale, options), `${msg} using String.prototype.localeCompare`);
            }

            test("en-US", { sensitivity: "base" }, "A", "a", 0);
            test("en-US", { sensitivity: "base" }, "A", "B", -1);
            test("en-US", { sensitivity: "base" }, "a", "\u00E2", 0);
            test("en-US", { sensitivity: "accent" }, "A", "a", 0);
            test("en-US", { sensitivity: "accent" }, "A", "B", -1);
            test("en-US", { sensitivity: "accent" }, "a", "\u00E2", -1);
            test("en-US", { sensitivity: "case" }, "A", "a", 1);
            test("en-US", { sensitivity: "case" }, "A", "B", -1);
            test("en-US", { sensitivity: "case" }, "a", "\u00E2", 0);
            test("en-US", { sensitivity: "variant" }, "A", "a", 1);
            test("en-US", { sensitivity: "variant" }, "A", "B", -1);
            test("en-US", { sensitivity: "variant" }, "a", "\u00E2", -1);

            test("en-US", { ignorePunctuation: true }, ".a", "a", 0);
            test("en-US", { ignorePunctuation: false }, ".a", "a", -1);

            test("en-US", { numeric: true }, "10", "9", 1);
            test("en-US", { numeric: false }, "10", "9", -1);

            assert.areEqual("de-DE", Intl.Collator.supportedLocalesOf("de-DE")[0], "This test requires de-DE");
            test("de-DE-u-co-phonebk", {}, "\u00e4b", "ada", 1);
            test("de-DE-u-co-phonebk", {}, "äb", "ada", 1);
            test("de-DE", {}, "\u00e4b", "ada", -1);
            test("de-DE", {}, "äb", "ada", -1);

            test("de-DE", { numeric: true }, "21", "100", -1);
            test("de-DE-u-kn-true", {}, "21", "100", -1);
            test("de-DE", { numeric: false }, "21", "100", 1);
            test("de-DE-u-kn-false", {}, "21", "100", 1);
            test("de-DE-u-kn-true-co-phonebk", {}, "21", "100", -1);
            test("de-DE-u-kn-true-co-phonebk", {}, "\u00e4b", "ada", 1);
            test("de-DE-u-kn-false-co-phonebk", {}, "21", "100", 1);
            test("de-DE-u-kn-false-co-phonebk", {}, "\u00e4b", "ada", 1);
            test("en-US", { ignorePunctuation: true }, "aa", "a!a", 0);
            test("en-US", { ignorePunctuation: false }, "aa", "a!a", 1);
        }
    },
    {
        name: "Options resolution",
        body() {
            assert.areEqual("es", Intl.Collator.supportedLocalesOf("es")[0], "Collator must support es for this test");
            assert.areEqual("default", new Intl.Collator("es", { collation: "trad" }).resolvedOptions().collation, "Collation must not be taken through options object");
            assert.areEqual("trad", new Intl.Collator("es-u-co-trad").resolvedOptions().collation, "Collation must be taken through unicode extension");

            function test(locale, options, expectedResolved) {
                const coll = new Intl.Collator(locale, options);
                const resolved = coll.resolvedOptions();

                const expected = expectedResolved || options;

                for (const prop of Object.getOwnPropertyNames(expected)) {
                    assert.areEqual(expected[prop], resolved[prop], `Testing option property ${prop} with locale ${locale} and options ${JSON.stringify(options)}`);
                }
            }

            test("en-US", { sensitivity: "variant" });
            test("en-US", { sensitivity: "case" });
            test("en-US", { sensitivity: "accent" });
            test("en-US", { sensitivity: "base" });

            if (WScript.Platform.INTL_LIBRARY === "winglob") {
                // Windows Globalization has different, usually incorrect, option resolution strategies
                return;
            }

            assert.areEqual("de-DE", Intl.Collator.supportedLocalesOf("de-DE")[0], "This test requires de-DE");
            // collation can not be passed via options object
            test("de-DE", { collation: "phonebk" }, { collation: "default" });
            test("de-DE-u-co-phonebk", { collation: "phonebk" });

            // Defaults
            test("de-DE", undefined, { numeric: false, caseFirst: "false", collation: "default", sensitivity: "variant", ignorePunctuation: false });

            // numeric option
            test("de-DE", { numeric: false });
            test("de-DE", { numeric: true });
            test("de-DE-u-kn-true", undefined, { numeric: true });
            test("de-DE-u-kn-false", undefined, { numeric: false });
            // options object takes precedence
            test("de-DE-u-kn-true", { numeric: false });
            test("de-DE-u-kn-false", { numeric: true });

            // caseFirst option
            test("de-DE", { caseFirst: "upper" });
            test("de-DE", { caseFirst: "lower" });
            test("de-DE", { caseFirst: "false" });
            test("de-DE-u-kf-upper", undefined, { caseFirst: "upper" });
            test("de-DE-u-kf-lower", undefined, { caseFirst: "lower" });
            test("de-DE-u-kf-false", undefined, { caseFirst: "false" });
            // options object takes precedence
            test("de-DE-u-kf-upper", { caseFirst: "lower" });
            test("de-DE-u-kf-false", { caseFirst: "upper" });

            // mixing unicode extensions and options object
            test("de-DE-u-co-phonebk", { numeric: false }, { numeric: false, collation: "phonebk" });
            test("de-DE-u-co-phonebk-kn-true", { numeric: false, caseFirst: "upper", ignorePunctuation: true }, { numeric: false, caseFirst: "upper", ignorePunctuation: true, collation: "phonebk" });
        }
    },
    {
        name: "Invalid options should throw",
        body() {
            function test(locale, options) {
                assert.throws(() => new Intl.Collator(locale, options), RangeError, `Creating a collator`);
                assert.throws(() => "a".localeCompare("b", locale, options), RangeError, `localeCompare with locale ${locale} and options ${JSON.stringify(options)}`);
            }

            test("en-US", { usage: "invalid" });
            test("en-US", { localeMatcher: "invalid" });
            test("en-US", { caseFirst: "invalid" });
            test("en-US", { sensitivity: "invalid" });
        }
    },
    {
        name: "Collator should normalize strings",
        body() {
            // this test is adapted from test262's `canonically-equivalent-strings` test

            const collator = new Intl.Collator();
            const pairs = [
                // example from Unicode 5.0, section 3.7, definition D70
                ["o\u0308", "ö"],
                // examples from Unicode 5.0, chapter 3.11
                ["ä\u0323", "a\u0323\u0308"],
                ["a\u0308\u0323", "a\u0323\u0308"],
                ["ạ\u0308", "a\u0323\u0308"],
                ["ä\u0306", "a\u0308\u0306"],
                ["ă\u0308", "a\u0306\u0308"],
                // example from Unicode 5.0, chapter 3.12
                ["\u1111\u1171\u11B6", "퓛"],
                // examples from UTS 10, Unicode Collation Algorithm
                ["Å", "Å"],
                ["Å", "A\u030A"],
                ["x\u031B\u0323", "x\u0323\u031B"],
                ["ự", "ụ\u031B"],
                ["ự", "u\u031B\u0323"],
                ["ự", "ư\u0323"],
                ["ự", "u\u0323\u031B"],
                // examples from UAX 15, Unicode Normalization Forms
                ["Ç", "C\u0327"],
                ["q\u0307\u0323", "q\u0323\u0307"],
                ["가", "\u1100\u1161"],
                ["Å", "A\u030A"],
                ["Ω", "Ω"],
                ["Å", "A\u030A"],
                ["ô", "o\u0302"],
                ["ṩ", "s\u0323\u0307"],
                ["ḋ\u0323", "d\u0323\u0307"],
                ["ḋ\u0323", "ḍ\u0307"],
                ["q\u0307\u0323", "q\u0323\u0307"],
                // examples involving supplementary characters from UCD NormalizationTest.txt
                ["\uD834\uDD5E", "\uD834\uDD57\uD834\uDD65"],
                ["\uD87E\uDC2B", "北"]
            ];

            function test(left, right) {
                assert.areEqual(0, collator.compare(left, right), `${left} and ${right} were not equal using Intl.Collator`);
                assert.areEqual(0, left.localeCompare(right), `${left} and ${right} were not equal using String.prototype.localeCompare`);
            }

            pairs.forEach((pair) => test(pair[0], pair[1]));
        }
    },
    {
        name: "https://github.com/Microsoft/ChakraCore/issues/5097",
        body() {
            const cases = [0, 1, true, false, null, undefined, { toString() { return "hello!" }}, [1, 2, 3, 4], {}, new (class ToStringTag { get [Symbol.toStringTag]() { return "MyClass" } })];

            const coll = new Intl.Collator();
            cases.forEach((test) => {
                assert.areEqual(0, ("" + test).localeCompare(test), `${test} did not compare equal to itself using String.prototype.localeCompare`);
                assert.areEqual(0, coll.compare("" + test, test), `${test} did not compare equal to itself using Collator.prototype.compare`);
            });
        }
    },
    {
        name: "Usage option should be respected",
        body() {
            if (WScript.Platform.INTL_LIBRARY === "winglob") {
                return;
            }

            function test(locale, usage, expectedLocale, expectedUsage, expectedCollation, expectedArray) {
                const input = ["AE", "A", "B", "Ä"];
                const collator = new Intl.Collator(locale, { usage });
                assert.areEqual(expectedLocale, collator.resolvedOptions().locale);
                assert.areEqual(expectedUsage, collator.resolvedOptions().usage);
                assert.areEqual(expectedCollation, collator.resolvedOptions().collation);
                assert.areEqual(expectedArray, input.sort(collator.compare).join(","));
            }

            test("de", "sort", "de", "sort", "default", "A,Ä,AE,B");
            test("de", "search", "de", "search", "default", "A,AE,Ä,B");
            test("de-u-co-phonebk", "sort", "de-u-co-phonebk", "sort", "phonebk", "A,AE,Ä,B");
            test("de-u-co-phonebk", "search", "de", "search", "default", "A,AE,Ä,B");
        }
    }
], { verbose: !WScript.Arguments.includes("summary") });
