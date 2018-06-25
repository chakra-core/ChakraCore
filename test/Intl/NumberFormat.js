//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function format() {
    let locale = "en-US", options, n;
    assert.isTrue(arguments.length > 0);

    if (typeof arguments[0] === "number") {
        [n] = arguments;
    } else if (typeof arguments[0] === "object" && !(arguments[0] instanceof Array)) {
        [options, n] = arguments;
    } else {
        [locale, options, n] = arguments;
    }

    const nf = new Intl.NumberFormat(locale, options);
    const format = nf.format(n);
    const localeString = n.toLocaleString(locale, options);

    assert.isTrue(format === localeString, `[locale = ${JSON.stringify(locale)}, options = ${JSON.stringify(options)}] format does not match toLocaleString`);
    if (WScript.Platform.INTL_LIBRARY === "icu") {
        assert.isTrue(format === nf.formatToParts(n).map((part) => part.value).join(""), `[locale = ${JSON.stringify(locale)}, options = ${JSON.stringify(options)}] format does not match formatToParts`);
    }

    return format;
}

const tests = [
    {
        name: "Decimal style default options",
        body: function () {
            assert.areEqual("5", format(5));
            assert.areEqual("5,000", format(5000));
            assert.areEqual("50.474", format(50.474));
        }
    },
    {
        name: "Min/max fractional digits",
        body: function () {
            // min
            assert.areEqual("5.00", format({ minimumFractionDigits: 2 }, 5));
            assert.areEqual("5.0", format({ minimumFractionDigits: 1 }, 5));

            // min and max
            assert.areEqual("5.00", format({ minimumFractionDigits: 2, maximumFractionDigits: 2 }, 5));
            assert.areEqual("5.0", format({ minimumFractionDigits: 1, maximumFractionDigits: 2 }, 5));

            // max
            assert.areEqual("5.44", format({ maximumFractionDigits: 2 }, 5.444));
            assert.areEqual("5.444", format({ maximumFractionDigits: 4 }, 5.444));
            assert.areEqual("5.45", format({ maximumFractionDigits: 2 }, 5.445));
            assert.areEqual("5.445", format({ maximumFractionDigits: 4 }, 5.445));
            assert.areEqual("5.55", format({ maximumFractionDigits: 2 }, 5.554));
            assert.areEqual("5.554", format({ maximumFractionDigits: 4 }, 5.554));
            assert.areEqual("5", format({ maximumFractionDigits: 0 }, 5.45));
            assert.areEqual("6", format({ maximumFractionDigits: 0 }, 5.5));
        }
    },
    {
        name: "Min integer digits",
        body: function () {
            assert.areEqual("5", format({ minimumIntegerDigits: 1 }, 5));
            assert.areEqual("05", format({ minimumIntegerDigits: 2 }, 5));
            assert.areEqual("0,000,000,005", format({ minimumIntegerDigits: 10 }, 5));
            assert.areEqual("500", format({ minimumIntegerDigits: 1 }, 500));
            assert.areEqual("0,000,000,500", format({ minimumIntegerDigits: 10 }, 500));
        }
    },
    {
        name: "Min/max significant digits",
        body: function () {
            // min
            assert.areEqual("5.0", format({ minimumSignificantDigits: 2 }, 5));
            assert.areEqual("500", format({ minimumSignificantDigits: 2 }, 500));
            assert.areEqual("500.0", format({ minimumSignificantDigits: 4 }, 500));

            // min and max
            assert.areEqual("5.0", format({ minimumSignificantDigits: 2, maximumSignificantDigits: 2 }, 5));
            assert.areEqual("5", format({ minimumSignificantDigits: 1, maximumSignificantDigits: 2 }, 5));

            // max
            assert.areEqual("5.44", format({ maximumSignificantDigits: 3 }, 5.444));
            assert.areEqual("5.444", format({ maximumSignificantDigits: 4 }, 5.4444));
            assert.areEqual("5.45", format({ maximumSignificantDigits: 3 }, 5.445));
            assert.areEqual("5.445", format({ maximumSignificantDigits: 4 }, 5.4445));
            assert.areEqual("5.55", format({ maximumSignificantDigits: 3 }, 5.554));
        }
    },
    {
        name: "Grouping separator",
        body: function () {
            assert.areEqual("50,000", format({ useGrouping: true }, 50000));
            assert.areEqual("50000", format({ useGrouping: false }, 50000));
            assert.areEqual("0000000005", format({ minimumIntegerDigits: 10, useGrouping: false }, 5));
            assert.areEqual("0000005000", format({ minimumIntegerDigits: 10, useGrouping: false }, 5000));
        }
    },
    {
        name: "Default style option combinations",
        body: function () {
            assert.areEqual("123", format({ minimumSignificantDigits: 3, maximumSignificantDigits: 3, minimumIntegerDigits: 5, minimumFractionDigits: 5, maximumFractionDigits: 5 }, 123.1));
            assert.areEqual("00,123.10000", format({ minimumIntegerDigits: 5, minimumFractionDigits: 5, maximumFractionDigits: 5 }, 123.1))
        }
    },
    {
        name: "Currency style",
        body: function () {
            function formatCurrency() {
                let locale = "en-US", currency = "USD", options, n;
                assert.isTrue(arguments.length > 0);

                if (typeof arguments[0] === "number") {
                    [n] = arguments;
                } else if (typeof arguments[0] === "object") {
                    [options, n] = arguments;
                } else if (arguments.length === 3) {
                    [currency, options, n] = arguments;
                } else {
                    [locale, currency, options, n] = arguments;
                }

                options = options || {};
                options.style = "currency",
                options.currency = currency;

                return format(locale, options, n)
            }

            assert.areEqual("$1.00", formatCurrency(1));
            assert.areEqual("$1.50", formatCurrency(1.50));
            assert.areEqual("$1.50", formatCurrency(1.504));
            assert.areEqual("$1.51", formatCurrency(1.505));
            assert.matches(/USD[\x20\u00a0]?1.00/, formatCurrency({ currencyDisplay: "code" }, 1), "Currency display: code");
            assert.matches(/USD[\x20\u00a0]?1.50/, formatCurrency({ currencyDisplay: "code" }, 1.504), "Currency display: code");
            assert.matches(/USD[\x20\u00a0]?1.51/, formatCurrency({ currencyDisplay: "code" }, 1.505), "Currency display: code");
            assert.areEqual("$1.00", formatCurrency({ currencyDisplay: "symbol" }, 1), "Currency display: symbol");
            assert.areEqual("$1.50", formatCurrency({ currencyDisplay: "symbol" }, 1.504), "Currency display: symbol");
            assert.areEqual("$1.51", formatCurrency({ currencyDisplay: "symbol" }, 1.505), "Currency display: symbol");
            // ICU has a proper "name" currency display, while WinGlob falls back to "code"
            assert.matches(/(?:USD[\x20\u00a0]?1.00|1.00 US dollars)/, formatCurrency({ currencyDisplay: "name" }, 1), "Currency display: name");
            assert.matches(/(?:USD[\x20\u00a0]?1.50|1.50 US dollars)/, formatCurrency({ currencyDisplay: "name" }, 1.504), "Currency display: name");
            assert.matches(/(?:USD[\x20\u00a0]?1.51|1.51 US dollars)/, formatCurrency({ currencyDisplay: "name" }, 1.505), "Currency display: name");
        }
    },
    {
        name: "Percent style",
        body: function () {
            assert.matches(/100[\x20\u00a0]?%/, format({ style: "percent" }, 1));
            assert.matches(/1[\x20\u00a0]?%/, format({ style: "percent" }, 0.01));
            assert.matches(/10,000[\x20\u00a0]?%/,format({ style: "percent" }, 100));
        }
    },
    {
        name: "Negative 0",
        body() {
            assert.areEqual(
                0,
                new Intl.NumberFormat(undefined, { minimumFractionDigits: -0 }).resolvedOptions().minimumFractionDigits,
                "Passing -0 for minimumFractionDigits should get normalized to 0 by DefaultNumberOption"
            );
            // This assert may need to change in the future, pending any decision made on https://github.com/tc39/ecma402/issues/219
            assert.areEqual("0", (-0).toLocaleString(), "-0 should be formatted as 0");
        }
    },
    {
        name: "formatToParts",
        body() {
            if (WScript.Platform.INTL_LIBRARY === "winglob") {
                return;
            }

            function assertParts(locale, options, n, expectedParts) {
                const nf = new Intl.NumberFormat(locale, options);
                const resolved = nf.resolvedOptions();
                assert.areEqual(locale, resolved.locale, `This test requires ${locale} support`);
                if (options) {
                    for (const opt of Object.getOwnPropertyNames(options)) {
                        assert.areEqual(options[opt], resolved[opt], `Bad option resolution for option ${opt}`)
                    }
                }

                const actualParts = nf.formatToParts(n);
                assert.isTrue(Array.isArray(actualParts), `formatToParts(${n}) did not return an array`);

                if (WScript.Platform.ICU_VERSION < 61) {
                    // real formatToParts support was only added with ICU 61
                    assert.areEqual(1, actualParts.length, `formatToParts(${n}) stub implementation should return only one part`);
                    const literal = actualParts[0];
                    assert.areEqual("unknown", literal.type, `formatToParts(${n}) stub implementation should return an unknown part`);
                    assert.areEqual(nf.format(n), literal.value, `formatToParts(${n}) stub implementation should return one part whose value matches the fully formatted number`);
                    return;
                }

                assert.areEqual(expectedParts.length, actualParts.length, `formatToParts(${n}) returned wrong number of parts (actual: ${JSON.stringify(actualParts, null, 2)})`);
                expectedParts.forEach((part, i) => {
                    assert.areEqual(expectedParts[i].type, actualParts[i].type, `formatToParts(${n}) returned bad type for part ${i}`);
                    assert.areEqual(expectedParts[i].value, actualParts[i].value, `formatToParts(${n}) returned bad value for part ${i} (code points: ${actualParts[i].value.split("").map(char => char.charCodeAt(0)).toString()})`);
                })
            }

            assertParts("en-US", undefined, 1000, [
                { type: "integer" , value: "1" },
                { type: "group", value: "," },
                { type: "integer", value: "000" }
            ]);
            assertParts("en-US", undefined, -1000, [
                { type: "minusSign", value: "-" },
                { type: "integer" , value: "1" },
                { type: "group", value: "," },
                { type: "integer", value: "000" }
            ]);
            assertParts("en-US", undefined, NaN, [{ type: "nan", value: "NaN" }]);
            assertParts("en-US", undefined, Infinity, [{ type: "infinity", value: "∞" }]);
            assertParts("en-US", undefined, 1000.3423, [
                { type: "integer", value: "1" },
                { type: "group", value: "," },
                { type: "integer", value: "000" },
                { type: "decimal", value: "." },
                { type: "fraction", value: "342" }
            ]);
            assertParts("en-US", { minimumFractionDigits: 5 }, 1000.3423, [
                { type: "integer", value: "1" },
                { type: "group", value: "," },
                { type: "integer", value: "000" },
                { type: "decimal", value: "." },
                { type: "fraction", value: "34230" }
            ]);
            assertParts("en-US", { style: "currency", currency: "CAD", currencyDisplay: "name" }, 1000.3423, [
                { type: "integer", value: "1" },
                { type: "group", value: "," },
                { type: "integer", value: "000" },
                { type: "decimal", value: "." },
                { type: "fraction", value: "34" },
                { type: "literal", value: " " },
                { type: "currency", value: "Canadian dollars" }
            ]);
            assertParts("en-US", { style: "percent", minimumSignificantDigits: 4 }, 0.3423, [
                { type: "integer", value: "34" },
                { type: "decimal", value: "." },
                { type: "fraction", value: "23" },
                { type: "percent", value: "%" }
            ]);

            assertParts("de-DE", { minimumFractionDigits: 5 }, 1000.3423, [
                { type: "integer", value: "1" },
                { type: "group", value: "." },
                { type: "integer", value: "000" },
                { type: "decimal", value: "," },
                { type: "fraction", value: "34230" }
            ]);
            assertParts("de-DE", { style: "currency", currency: "CAD", currencyDisplay: "name" }, 1000.3423, [
                { type: "integer", value: "1" },
                { type: "group", value: "." },
                { type: "integer", value: "000" },
                { type: "decimal", value: "," },
                { type: "fraction", value: "34" },
                { type: "literal", value: " " },
                { type: "currency", value: "Kanadische Dollar" }
            ]);
            assertParts("de-DE", { style: "percent", minimumSignificantDigits: 4 }, 0.3423, [
                { type: "integer", value: "34" },
                { type: "decimal", value: "," },
                { type: "fraction", value: "23" },
                { type: "literal", value: "\u00A0" }, // non-breaking space
                { type: "percent", value: "%" }
            ]);
        }
    }
];

testRunner.runTests(tests, { verbose: false });
