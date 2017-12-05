//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var passed = true;

function testCollatorOptions(option, value, str1, str2, expected) {
    try {
        var options = {};
        options[option] = value;
        var collator = new Intl.Collator("en-US", options);
        var actual = collator.compare(str1, str2);
        if (actual !== expected) {
            passed = false;
            WScript.Echo("ERROR: Comparing '" + str1 + "' and '" + str2 + "' with option '" + option + ": " + value + "' resulted in '" + actual + "'; expected '" + expected + "'!");
        }
    } catch (ex) {
        passed = false;
        WScript.Echo("Error testCollatorOptions: " + ex.message);
    }
}

testCollatorOptions("sensitivity", "base", "A", "a", 0);
testCollatorOptions("sensitivity", "base", "A", "B", -1);
testCollatorOptions("sensitivity", "base", "a", "\u00E2", 0);
testCollatorOptions("sensitivity", "accent", "A", "a", 0);
testCollatorOptions("sensitivity", "accent", "A", "B", -1);
testCollatorOptions("sensitivity", "accent", "a", "\u00E2", -1);
testCollatorOptions("sensitivity", "case", "A", "a", 1);
testCollatorOptions("sensitivity", "case", "A", "B", -1);
testCollatorOptions("sensitivity", "case", "a", "\u00E2", 0);
testCollatorOptions("sensitivity", "variant", "A", "a", 1);
testCollatorOptions("sensitivity", "variant", "A", "B", -1);
testCollatorOptions("sensitivity", "variant", "a", "\u00E2", -1);

testCollatorOptions("ignorePunctuation", true, ".a", "a", 0);
testCollatorOptions("ignorePunctuation", false, ".a", "a", -1);

testCollatorOptions("numeric", true, "10", "9", 1);
testCollatorOptions("numeric", false, "10", "9", -1);

if (new Intl.Collator("es", { collation: "trad" }).resolvedOptions().collation !== "default") {
    WScript.Echo("Error: Collation option passed through option affects the value.");
    passed = false;
}

if (passed === true) {
    WScript.Echo("Pass");
}
