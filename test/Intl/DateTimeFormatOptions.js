//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

// NOTE: \u200e is the U+200E LEFT-TO-RIGHT MARK
var tests = [
    {
        name: "Test Valid Options",
        body: function () {
            assert.areEqual(new Intl.DateTimeFormat("en-US", { year: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e2000", "Formatting year as numeric.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { year: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e00", "Formatting year as 2-digit.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e2", "Formatting month as numeric.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e02", "Formatting month as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "long" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200eFebruary", "Formatting month as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "short" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200eFeb", "Formatting month as numeric.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { month: "narrow" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200eFeb", "Formatting month as narrow.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { day: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01", "Formatting day as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { day: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1", "Formatting day as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e \u200eAM", "Formatting hour as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1\u200e \u200eAM", "Formatting hour as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit and minute as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit and minute as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "2-digit", second: "2-digit" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit, minute as 2-digit and second as 2-digit.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "2-digit", minute: "2-digit", second: "numeric" }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e01\u200e:\u200e01\u200e:\u200e01\u200e \u200eAM", "Formatting hour as 2-digit, minute as 2-digit and second as numeric.");

            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "numeric", hour12: true }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1\u200e \u200eAM", "Formatting hour as numeric with hour12=true.");
            assert.areEqual(new Intl.DateTimeFormat("en-US", { hour: "numeric", hour12: false }).format(new Date(2000, 1, 1, 1, 1, 1)), "\u200e1\u200e:\u200e00", "Formatting hour as numeric with hour12=false.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
