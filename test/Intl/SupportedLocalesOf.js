//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const ctors = [Intl.Collator, Intl.NumberFormat, Intl.DateTimeFormat];

function assertEachIsOneOf(expectedList, actualList, msg) {
    if (!actualList || actualList.length === 0) {
        assert.fail(`actualList was empty: ${msg}`);
    }
    for (a of actualList) {
        assert.isTrue(expectedList.includes(a), msg);
    }
}

const tests = [
    {
        name: "supportedLocalesOf throws correct errors",
        body: function () {
            const rangeErrorMessage = "Option value 'incorrect' for 'localeMatcher' is outside of valid range. Expected: ['best fit', 'lookup']";
            const fakeLocales = { get length() { throw new Error("User-provided locale object throws"); } };

            function test(ctor) {
                if (WScript.Platform.INTL_LIBRARY === "icu") {
                    assert.throws(() => new ctor.supportedLocalesOf(), TypeError, "", "Function is not a constructor");
                    assert.throws(() => Reflect.construct(function() {}, [], ctor.supportedLocalesOf), TypeError, "", "'newTarget' is not a constructor");
                } else {
                    assert.throws(() => new ctor.supportedLocalesOf(), TypeError, "", `Function 'Intl.${ctor.name}.supportedLocalesOf' is not a constructor`);
                }
                assert.throws(() => ctor.supportedLocalesOf(["en-US"], { localeMatcher: "incorrect" }), RangeError, "", rangeErrorMessage);
                assert.throws(() => ctor.supportedLocalesOf(null), TypeError, "", "Object expected");
                assert.throws(() => ctor.supportedLocalesOf(fakeLocales), Error, "", "User-provided locale object throws");
            }

            ctors.forEach(test);
        }
    },
    {
        name: "supportedLocalesOf basic tests",
        body: function () {
            function test(ctor) {
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf(["en"]));
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf(["en"], { localeMatcher: "lookup" }));
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf(["en"], { localeMatcher: "best fit" }));

                assertEachIsOneOf(['de', 'de-DE'], ctor.supportedLocalesOf(['de-de']));
                assertEachIsOneOf(['ja', 'ja-JP'], ctor.supportedLocalesOf(['ja-JP']));
                assertEachIsOneOf(['zh', 'zh-CN', 'zh-Hans-CN'], ctor.supportedLocalesOf(['zh-cn']));

                assertEachIsOneOf(
                    [
                        'en', 'en-US',
                        'de', 'de-DE',
                        'ja', 'ja-JP',
                        'zh', 'zh-CN', 'zh-Hans-CN',
                    ],
                    ctor.supportedLocalesOf(['en-us', 'de-de', 'ja-JP', 'zh-cn'])
                );
            }

            ctors.forEach(test);
        }
    },
    {
        name: "Modifying `this` should not break supportedLocalesOf",
        body: function () {
            function test(ctor) {
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf.call({}, ["en"]));
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf.call({}, ["en"], { localeMatcher: "lookup" }));
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf.call({}, ["en"], { localeMatcher: "best fit" }));
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf.bind({})(["en"]));
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf.bind({})(["en"], { localeMatcher: "lookup" }));
                assertEachIsOneOf(["en", "en-US"], ctor.supportedLocalesOf.bind({})(["en"], { localeMatcher: "best fit" }));
            }

            ctors.forEach(test);
        }
    },
    {
        name: "supportedLocalesOf an empty array or undefined should produce an empty array",
        body: function () {
            function test(ctor) {
                assert.areEqual(0, ctor.supportedLocalesOf(undefined).length);
                assert.areEqual(0, ctor.supportedLocalesOf(undefined, { localeMatcher: "lookup" }).length);
                assert.areEqual(0, ctor.supportedLocalesOf(undefined, { localeMatcher: "best fit" }).length);
            }
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
