//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

if (Intl.platform === undefined) {
    WScript.Echo("This test must be run with -IntlPlatform to enable the Intl.platform object");
    WScript.Quit(1);
} else if (WScript.Platform.INTL_LIBRARY === "winglob") {
    // Skip for winglob -- there is an equivalent test in ChakraFull for it
    WScript.Echo("pass");
    WScript.Quit(0);
}

var platform = Intl.platform;
Intl.platform.useCaches = false;

/**
 * Caches all methods on Intl.platform and restores them after the test executes
 */
function testSingleConstructor(Ctor, test) {
    return function () {
        const methods = Object.getOwnPropertyNames(Intl.platform);
        const cache = {};

        for (const method of methods) {
            cache[method] = Intl.platform[method];
        }

        test(Ctor);

        for (const method of methods) {
            Intl.platform[method] = cache[method];
        }
    }
}

/**
 * Creates a test for each constructor
 */
function testEachConstructor(name, test) {
    const ret = [];
    for (const Ctor of [Intl.Collator, Intl.NumberFormat, Intl.DateTimeFormat]) {
        ret.push({
            name: `${name} (using Intl.${Ctor.name})`,
            body: testSingleConstructor(Ctor, test)
        });
    }
    return ret;
}

const tests = [
    ...testEachConstructor("Changing the default locale", function (Ctor) {
        platform.getDefaultLocale = () => "de-DE";
        assert.areEqual("de-DE", new Ctor().resolvedOptions().locale, "Default locale is respected with undefined language tag");
    }),
    ...testEachConstructor("Limiting available locales", function (Ctor) {
        const isXLocaleAvailableMap = {
            Collator: "Collator",
            NumberFormat: "NF",
            DateTimeFormat: "DTF",
        };

        const mapped = isXLocaleAvailableMap[Ctor.name];
        assert.isNotUndefined(mapped, `Invalid test setup: no mapped name available for ${Ctor.name}`);

        platform[`is${mapped}LocaleAvailable`] = (langtag) => ["de", "de-DE", "en", "zh", "en-UK"].includes(langtag);
        platform.getDefaultLocale = () => "en-UK";
        assert.areEqual("en", new Ctor("en-US").resolvedOptions().locale, "en-US should fall back to en");
        assert.areEqual("zh", new Ctor("zh-Hans").resolvedOptions().locale, "zh-Hans should fall back to zh");
        assert.areEqual("de-DE", new Ctor("de-DE-gregory").resolvedOptions().locale, "de-DE-gregory should fall back to de-DE");
        assert.areEqual("en-UK", new Ctor("sp").resolvedOptions().locale, "An unknown language tag should fall back to the default");
    })
];

testRunner.runTests(tests, { verbose: false });
