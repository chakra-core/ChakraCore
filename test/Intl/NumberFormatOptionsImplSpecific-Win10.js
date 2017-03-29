//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function extendOptions(base, ext) {
    const ret = {};
    for (const x in base) {
        if (base.hasOwnProperty(x)) {
            ret[x] = base[x];
        }
    }
    for (const x in ext) {
        if (ext.hasOwnProperty(x)) {
            ret[x] = ext[x];
        }
    }
    return ret;
}

const NON_BREAKING_SPACE = "\u00a0";

var tests = [
    {
        name: "Test Valid Options - Formatting Currency with Significant Digits",
        body: function () {
            const usdBaseOptions = { minimumSignificantDigits: 3, maximumSignificantDigits: 3, style: "currency", currency: "USD" };

            assert.areEqual(new Intl.NumberFormat("en-US", extendOptions(usdBaseOptions, {})).format(123.1), "$123", "currency: USD (default display)");
            assert.areEqual(new Intl.NumberFormat("en-US", extendOptions(usdBaseOptions, { currencyDisplay: "code" })).format(123.1), `USD${NON_BREAKING_SPACE}123`, "currency code: USD");

            assert.areEqual(new Intl.NumberFormat("en-US", extendOptions(usdBaseOptions, { currencyDisplay: "symbol" })).format(123.1), "$123", "currency symbol: USD ($)");
            assert.areEqual(new Intl.NumberFormat("en-US", extendOptions(usdBaseOptions, { currencyDisplay: "name" })).format(123.1), `USD${NON_BREAKING_SPACE}123`, "currency name: USD");
        }
    },
    {
        name: "Test Valid Options - Formatting Percentage with Significant Digits",
        body: function () {
            const baseSigFigs = { minimumSignificantDigits: 3, maximumSignificantDigits: 3 };

            assert.areEqual(new Intl.NumberFormat("en-US", extendOptions(baseSigFigs, { style: "decimal" })).format(123.1), "123", "style: decimal");
            assert.areEqual(new Intl.NumberFormat("en-US", extendOptions(baseSigFigs, { style: "percent" })).format(123.1), `12,300${NON_BREAKING_SPACE}%`, "style: percent");
            assert.areEqual(new Intl.NumberFormat("en-US", extendOptions(baseSigFigs, { style: "percent", useGrouping: false })).format(123.1), `12300${NON_BREAKING_SPACE}%`, "style: percent, no grouping");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
