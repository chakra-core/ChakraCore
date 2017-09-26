//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testRangeError(tag) {
    assert.throws(function () { Intl.getCanonicalLocales(tag) }, RangeError,
        `Tag '${tag}' should throw RangeError`,
        `Locale '${tag}' is not well-formed`);
}

function assertEachIsOneOf(actualList, expectedList, msg) {
    if (actualList.length === 0) {
        assert.fail("actual result list was empty");
    }
    for (a of actualList) {
        assert.isTrue(expectedList.includes(a), msg);
    }
}

let IntlNamespaces = [
    Intl.Collator,
    Intl.DateTimeFormat,
    Intl.NumberFormat
]

// Tests of ECMA 402 Basic Functionality
// Minor motivation is to exercise internal operations implemented with WinGlob and ICU.
var tests = [
    // #sec-canonicalizelocalelist (JS `Intl.getCanonicalLocales`)
    {
        name: "#sec-isstructurallyvalidlanguagetag (C++ `IsWellFormedLanguageTag`): malformed inputs throw",
        body: function () {
            // * #sec-isstructurallyvalidlanguagetag: C++ `IsWellFormedLanguageTag`: not well-formed; should throw
            let malformed = ['中文', 'en-', '-us', 'en-us-latn'];
            malformed.forEach(testRangeError);
        }
    },
    {
        name: "#sec-canonicalizelanguagetag (C++ `NormalizeLanguageTag`)",
        body: function () {
            // * #sec-isstructurallyvalidlanguagetag: C++ `IsWellFormedLanguageTag`: input is well-formed
            //   * (implies C++ `IsWellFormedLanguageTag` returned true and thus didn't throw)
            assert.areEqual(Intl.getCanonicalLocales(['en-us']), ['en-US']);
            assert.areEqual(Intl.getCanonicalLocales(['de-de']), ['de-DE']);
            assertEachIsOneOf(Intl.getCanonicalLocales(['ja-JP']), ['ja', 'ja-JP'], "Depending on WinGlob or ICU, one of these results");
            assertEachIsOneOf(Intl.getCanonicalLocales(['zh-cn']), ['zh-CN', 'zh-Hans-CN'], "Depending on WinGlob or ICU, one of these results");

            assertEachIsOneOf(
                Intl.getCanonicalLocales(
                    [
                        'en-us', 'de-de', 'ja-JP', 'zh-cn'
                    ]
                ),
                [
                    'en-US',
                    'de-DE',
                    'ja', 'ja-JP',
                    'zh-CN', 'zh-Hans-CN',
                ]
            );
        }
    },
    {
        name: "supportedLocalesOf",
        body: function () {
            IntlNamespaces.forEach(function(ns) {
                assertEachIsOneOf(ns.supportedLocalesOf(['en-us']), ['en', 'en-US']);
                assertEachIsOneOf(ns.supportedLocalesOf(['de-de']), ['de', 'de-DE']);
                assertEachIsOneOf(ns.supportedLocalesOf(['ja-JP']), ['ja', 'ja-JP']);
                assertEachIsOneOf(ns.supportedLocalesOf(['zh-cn']), ['zh', 'zh-CN', 'zh-Hans-CN']);

                assertEachIsOneOf(
                    ns.supportedLocalesOf(
                        [
                            'en-us', 'de-de', 'ja-JP', 'zh-cn'
                        ]
                    ),
                    [
                        'en', 'en-US',
                        'de', 'de-DE',
                        'ja', 'ja-JP',
                        'zh', 'zh-CN', 'zh-Hans-CN',
                    ]
                );
            });
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
