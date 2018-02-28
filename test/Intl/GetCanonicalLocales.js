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

/**
 * Allows different values to be asserted depending on the Intl implementation
 * 
 * @param {String|String[]} expectedWinGlob
 * @param {String|String[]} expectedICU
 * @param {String|String[]} actual
 * @param {String} message
 */
const equal = (function () {
    if (WScript.Platform.INTL_LIBRARY === "icu") {
        return function (_, expectedICU, actual, message) {
            assert.areEqual(expectedICU, actual, message);
        }
    } else {
        assert.isTrue(WScript.Platform.INTL_LIBRARY === "winglob");
        return function (expectedWinGlob, _, actual, message) {
            assert.areEqual(expectedWinGlob, actual, message);
        }
    }
})();

const gcl = Intl.getCanonicalLocales;

var tests = [
    {
        name: "Intl.getCanonicalLocales Functionality (according to ECMA 402 #sec-canonicalizelocalelist)",
        // ensure array (or array-like) or convert to array, canonicalize each entry, remove duplicates
        body: function () {
            // ensure output is an array even if input was not an array
            assert.areEqual(Intl.getCanonicalLocales('en'), ['en'], "Input is a singleton string (not an array) -> output is array");

            // canonicalize case
            assert.areEqual(Intl.getCanonicalLocales(['en']), ['en'], "Input matches output, no lookup is performed");
            assert.areEqual(Intl.getCanonicalLocales(['en-us']), ['en-US'], "Canonicalize country casing (en-US) (all-lowercase)");
            assert.areEqual(Intl.getCanonicalLocales(['en-Us']), ['en-US'], "Canonicalize country casing (en-US) (mixed-case)");
            assert.areEqual(Intl.getCanonicalLocales(['EN-us']), ['en-US'], "Canonicalize country casing (en-US) (completely incorrect casing)");
            assert.areEqual(Intl.getCanonicalLocales(['de-de']), ['de-DE'], "Canonicalize country casing (de-DE)");

            // array-like objects are be fine (according to spec, arrays are converted to Object anyway)
            // ECMA 402 #sec-canonicalizelocalelist
            //      5. Let len be ? ToLength(? Get(O, "length")).
            //      6. Let k be 0.
            //      7. Repeat, while k < len
            // Since ToLength(undefined) === 0, we don't enter the loop (essentially treat the input as a zero-length array).
            //      ToLength(undefined) -> ToInteger(undefined) -> ToNumber(undefined) -> NaN
            //      ToInteger converts NaN to +0.
            assert.areEqual(Intl.getCanonicalLocales({ '0': 'en-us' }), [], "Objects which might look like arrays are fine, but treated as 0 length.");
            assert.areEqual(Intl.getCanonicalLocales({ 'a': 'b' }), [], "Arbitrary Objects are fine, treated as 0-length arrays.");

            // Objects contained in the input array are fine if their toString is a valid language tag.
            assert.areEqual(Intl.getCanonicalLocales(['en-us', { toString() { return 'en-us' } }]), ['en-US'], "Object.toString returning a valid language tag is fine.");

            assert.throws(function () { Intl.getCanonicalLocales([{ toString() { return undefined } }]) }, RangeError,
                "Object.toString returning a non-string or invalid language tag is RangeError.");

            // canonicalization of script code subkey
            assert.areEqual(Intl.getCanonicalLocales(['zh-hans-cn']), ['zh-Hans-CN'], "Chinese (zh) Han Simplified (Hans) as used in China (CN)");
            assert.areEqual(Intl.getCanonicalLocales(['zh-hant-hk']), ['zh-Hant-HK'], "Chinese (zh) Han Traditional (Hant) as used in Hong Kong (HK)");

            // language-extlang form and other non-preferred forms normalize to preferred ISO 639-3
            // This should be handled implicitly by canonicalization routine (no knowledge of language tags required),
            // but we make sure it works for some actual languages in any case.
            // RFC 5646 2.1:
            //      language = 2-3ALPHA ["-" extlang]
            //      extlang  = 2-3ALPHA *2("-" 3ALPHA)
            // https://en.wikipedia.org/wiki/IETF_language_tag#ISO_639-3_and_ISO_639-1
            let mandarinChinese = ['cmn', 'zh-cmn']; // Mandarin Chinese (language-extlang: zh-cmn; prefer ISO 639-3: cmn)
            let minNanChinese = ['nan', 'zh-nan', 'zh-min-nan']; // Min-Nan Chinese (ISO 639-3: nan)
            let hakkaChinese = ['hak', 'zh-hak', 'zh-hakka', 'i-hak']; // Hakka Chinese (ISO 639-3: hak)
            let chineseIn = [].concat(mandarinChinese, minNanChinese, hakkaChinese);
            let chineseOut = [].concat(mandarinChinese[0], minNanChinese[0], hakkaChinese[0]); // after de-dup should be only these three preferred codes
            assert.areEqual(Intl.getCanonicalLocales(chineseIn), chineseOut, "Chinese language-extlang and other forms map to preferred ISO 639-3 codes");

            // canonicalization of -u- extension keys
            // V8 and CC-ICU convert boolean keys (kn) to boolean string values (including giving them default values),
            // which is incorrect. SpiderMonkey and CC-WinGlob correctly avoid this.
            // V8 and CC-ICU also give the default value of "yes" to non-boolean keys (co), which also is incorrect.
            // Everyone (should) correctly re-order extension keys alphabetically
            // Microsoft/ChakraCore#4490 tracks the incorrect defaulting, Microsoft/ChakraCore#2964 tracks the overall investigation
            equal("de-DE-u-co-kn", "de-DE-u-co-yes-kn-true", gcl("de-de-u-kn-co")[0]);
            equal("de-DE-u-co-phonebk-kn", "de-DE-u-co-phonebk-kn-true", gcl("de-de-u-kn-co-phonebk")[0]);
            equal("de-DE-u-co-phonebk-kn-yes", "de-DE-u-co-phonebk-kn-true", gcl("de-DE-u-kn-yes-co-phonebk")[0]);

            // De-dupe after locales are canonicalized
            assert.areEqual(Intl.getCanonicalLocales(['en-us', 'en-us']), ['en-US'], "No duplicates, same input casing (casing was incorrect)");
            assert.areEqual(Intl.getCanonicalLocales(['en-US', 'en-US']), ['en-US'], "No duplicates, same input casing (casing was correct)");
            assert.areEqual(Intl.getCanonicalLocales(['en-us', 'en-US']), ['en-US'], "No duplicates, different input casing");

            assert.areEqual(
                Intl.getCanonicalLocales(["de-de", "de-DE-u-co-phonebk-kn-true", "de-DE-u-kn-true-co-phonebk"]),
                ["de-DE", "de-DE-u-co-phonebk-kn-true"],
                "No duplicates after re-ordering options"
            );
        }
    },
    {
        name: "Handling of unsupported tags and subtags (general canonicalization)",
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

            // TODO (doilij): Investigate what is correct/allowable here (Microsoft/ChakraCore#2964)
            equal("xx-zzz", "zzz", gcl("xx-zzz")[0]);

            // See discussion of defaulting above (V8/CC-ICU and CC-WinGlob/SM distinction remains true here)
            equal("xx-ZZ-u-yy-zz", "xx-ZZ-u-yy-yes-zz-yes", gcl("xx-zz-u-zz-yy")[0]);
        }
    },
    {
        name: "Rejection of duplicate tags",
        body: function () {
            const duplicateSingletons = ['cmn-hans-cn-u-u', 'cmn-hans-cn-t-u-ca-u'];
            const duplicateUnicodeExtensionKeys = ['de-de-u-kn-true-co-phonebk-co-phonebk'];

            if (WScript.Platform.INTL_LIBRARY === "icu") {
                const duplicateTags = ['de-gregory-gregory'];
                duplicateTags.forEach(testRangeError);
            }

            // duplicateTags.forEach(testRangeError);
            duplicateSingletons.forEach(testRangeError);
            duplicateUnicodeExtensionKeys.forEach(testRangeError);
        }
    },
    {
        name: "Structurally invalid tags",
        // * ECMA 402 #sec-canonicalizelocalelist -- step 7.c.iv. If IsStructurallyValidLanguageTag(tag) is false, throw a RangeError exception.
        // * ECMA 402 #sec-isstructurallyvalidlanguagetag
        // * (Note: The above basically just refers to RFC 5646 section 2.1)
        body: function () {
            const empty = [''];
            const invalidSubtags = ['en-A1'];
            const invalidVariants = ['en-us-latn', 'en-us-latnlatnlatn'];
            const invalidChars = ['en-a@'];
            const nonAsciiChars = ['中文', 'de-ßß'];
            const boundaryHyphen = ['-en', '-en-us', 'en-', 'en-us-'];
            const incompleteSubtags = ['de-de-u'];
            const extlangNotAllowedAfterScript = ['xx-abcd-zzz', 'xx-yyy-abcd-zzz', 'xx-yyy-Abcd-zzz-aa'];

            empty.forEach(testRangeError);
            invalidSubtags.forEach(testRangeError);
            invalidVariants.forEach(testRangeError);
            invalidChars.forEach(testRangeError);
            nonAsciiChars.forEach(testRangeError)
            boundaryHyphen.forEach(testRangeError);
            incompleteSubtags.forEach(testRangeError);
            extlangNotAllowedAfterScript.forEach(testRangeError);
        }
    },
    {
        name: "Bad/weird input",
        body: function () {
            // ECMA 402 #sec-canonicalizelocalelist -- step 1.a. if locales is undefined, return []
            assert.areEqual(Intl.getCanonicalLocales(), [], "Implicit undefined");
            assert.areEqual(Intl.getCanonicalLocales(undefined), [], "Explicit undefined");

            // There is no special case for null type inputs in the definition, so throw TypeError
            // ECMA 402 #sec-canonicalizelocalelist -- step 4.a. Let O be ? ToObject(locales).
            // ECMA 262 #sec-toobject
            assert.throws(function () { Intl.getCanonicalLocales(null) }, TypeError, "Cannot convert null to object.");
            // Test Number literals
            assert.areEqual(Intl.getCanonicalLocales(1), [], "Number is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales(3.14), [], "Number is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales(Infinity), [], "Number is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales(-Infinity), [], "Number is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales(NaN), [], "Number is converted to string internally and no locale is found");
            // Test other types of literals
            assert.areEqual(Intl.getCanonicalLocales(true), [], "Boolean is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales(Symbol.toStringTag), [], "Symbol is converted to string internally and no locale is found");
            // RegExp and Object literals
            assert.areEqual(Intl.getCanonicalLocales(/a/), [], "RegExp is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales(/en-us/), [], "RegExp is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales([]), [], "Object is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales({}), [], "Object is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales({ '0': 'en-us' }), [], "Object is converted to string internally and no locale is found");
            assert.areEqual(Intl.getCanonicalLocales(['en-us', { toString: () => 'en-us' }]), ['en-US'], "Element is an Object whose toString produces a valid language tag");
            assert.areEqual(Intl.getCanonicalLocales({ toString: () => 'en-us' }), [], "Argument is an Object which doesn't have any numeric indexes");

            // Arrays containing anything which is not String or Object type should throw.
            // ECMA 402 #sec-canonicalizelocalelist
            // * step 7.c.ii. If Type(kValue) is not String or Object, throw a TypeError exception.
            // * step 7.c.iii. Let tag be ? ToString(kValue).
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', null]) }, TypeError, "null is not String or Object.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', 1]) }, TypeError, "Number is not String or Object.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', 3.14]) }, TypeError, "Number is not String or Object.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', Infinity]) }, TypeError, "Number is not String or Object.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', -Infinity]) }, TypeError, "Number is not String or Object.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', NaN]) }, TypeError, "Number is not String or Object.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', true]) }, TypeError, "Boolean is not String or Object.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', Symbol.toStringTag]) }, TypeError, "Symbol is not String or Object.");
            // RegExp and Object literals
            // * step 7.c.iv. If IsStructurallyValidLanguageTag(tag) is false, throw a RangeError exception.
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', /a/]) }, RangeError, "RegExp is an Object, whose toString is not a well-formed language tag.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', /en-us/]) }, RangeError, "RegExp is an Object, whose toString is not a well-formed language tag.");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', []]) }, RangeError, "Array contained within an array. [].toString()==='' (invalid tag).");
            assert.throws(function () { Intl.getCanonicalLocales(['en-us', {}]) }, RangeError, "Object whose toString is not a well-formed language tag.");
            assert.throws(function () { Intl.getCanonicalLocales([{ '0': 'en-us' }]) }, RangeError, "Array containing object where toString() produces an invalid tag.");
        }
    },
    {
        name: "Array with holes",
        body: function () {
            let a = [];
            a[1] = 'en';
            assert.areEqual(Intl.getCanonicalLocales(a), ['en']);
        }
    },
    {
        name: "Array-like object (without holes)",
        body: function () {
            let locales = {
                length: 2,
                0: 'zh',
                1: 'en'
            };
            assert.areEqual(Intl.getCanonicalLocales(locales), ['zh', 'en']);
        }
    },
    {
        name: "Array-like object (with holes)",
        body: function () {
            let locales = {
                length: 2,
                // 0: 'zh',
                1: 'en'
            };
            assert.areEqual(Intl.getCanonicalLocales(locales), ['en']);
        }
    },
    {
        name: "Array-like class with numeric getters (without holes)",
        body: function () {
            class x {
                get 0() { return 'zh'; }
                get 1() { return 'en'; }
                get length() { return 2; }
            }
            let locales = new x();
            assert.areEqual(Intl.getCanonicalLocales(locales), ['zh', 'en']);
        }
    },
    {
        name: "Array-like class with numeric getters (with holes)",
        body: function () {
            class x {
                // get 0() { return 'zh'; } // culture[0] is a hole
                get 1() { return 'en'; }
                get length() { return 2; }
            }
            let locales = new x();
            assert.areEqual(Intl.getCanonicalLocales(locales), ['en']);
        }
    },
    {
        name: "Array-like class with numeric getters (with base class closing the hole)",
        body: function () {
            class base {
                get 0() { return 'jp'; } // closes the hole in x
            }
            class x extends base {
                // get 0() { return 'zh'; } // culture[0] has a hole
                get 1() { return 'en'; }
                get length() { return 2; } // try 2 with get 0 defined in base; try 2,3 with get 2 defined in base; try 3 with get 0, get 1 defined
            }
            let locales = new x();
            assert.areEqual(Intl.getCanonicalLocales(locales), ['jp', 'en']);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
