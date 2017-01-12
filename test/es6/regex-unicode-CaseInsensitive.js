//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function assertMatches(re, codepoint, str) {
    let passed = re.test(str);
    if (!passed) {
        console.log("FAILED -- regex: " + re.toString() + " should match codepoint: " + codepoint.toString(16));
    }
}

function assertDoesNotMatch(re, codepoint, str) {
    let passed = re.test(str);
    if (passed) {
        console.log("FAILED -- regex: " + re.toString() + " should not match codepoint: " + codepoint.toString(16));
    }
}

(function missedMappingsWithUnicodeFlag() {

    assertMatches(/\u0345/iu, 0x0345, '\u0345');
    assertMatches(/\u0345/iu, 0x03b9, '\u0399');
    assertMatches(/\u0345/iu, 0x03b9, '\u03b9');
    assertMatches(/\u0345/iu, 0x1fbe, '\u1fbe');
    assertMatches(/\u0399/iu, 0x0345, '\u0345');
    assertMatches(/\u0399/iu, 0x03b9, '\u0399');
    assertMatches(/\u0399/iu, 0x03b9, '\u03b9');
    assertMatches(/\u0399/iu, 0x1fbe, '\u1fbe');
    assertMatches(/\u03b9/iu, 0x0345, '\u0345');
    assertMatches(/\u03b9/iu, 0x03b9, '\u0399');
    assertMatches(/\u03b9/iu, 0x03b9, '\u03b9');
    assertMatches(/\u03b9/iu, 0x1fbe, '\u1fbe');
    assertMatches(/\u1fbe/iu, 0x0345, '\u0345');
    assertMatches(/\u1fbe/iu, 0x03b9, '\u0399');
    assertMatches(/\u1fbe/iu, 0x03b9, '\u03b9');
    assertMatches(/\u1fbe/iu, 0x1fbe, '\u1fbe');

    assertMatches(/\u01f1/iu, 0x01f3, '\u01f3');
    assertMatches(/\u0345/iu, 0x03b9, '\u03b9');
    assertMatches(/\u037f/iu, 0x03f3, '\u03f3');
    assertMatches(/\u0528/iu, 0x0529, '\u0529');
    assertMatches(/\u052a/iu, 0x052b, '\u052b');
    assertMatches(/\u052c/iu, 0x052d, '\u052d');
    assertMatches(/\u052e/iu, 0x052f, '\u052f');
    assertMatches(/\ua698/iu, 0xa699, '\ua699');
    assertMatches(/\ua69a/iu, 0xa69b, '\ua69b');
    assertMatches(/\ua796/iu, 0xa797, '\ua797');
    assertMatches(/\ua798/iu, 0xa799, '\ua799');
    assertMatches(/\ua79a/iu, 0xa79b, '\ua79b');
    assertMatches(/\ua79c/iu, 0xa79d, '\ua79d');
    assertMatches(/\ua79e/iu, 0xa79f, '\ua79f');
    assertMatches(/\ua7ab/iu, 0x025c, '\u025c');
    assertMatches(/\ua7ac/iu, 0x0261, '\u0261');
    assertMatches(/\ua7ad/iu, 0x026c, '\u026c');
    assertMatches(/\ua7b0/iu, 0x029e, '\u029e');
    assertMatches(/\ua7b1/iu, 0x0287, '\u0287');

})();

(function missedMappingsWithoutUnicodeFlag() {

    assertMatches(/\u0345/i, 0x0345, '\u0345');
    assertMatches(/\u0345/i, 0x03b9, '\u0399');
    assertMatches(/\u0345/i, 0x03b9, '\u03b9');
    assertMatches(/\u0345/i, 0x1fbe, '\u1fbe');
    assertMatches(/\u0399/i, 0x0345, '\u0345');
    assertMatches(/\u0399/i, 0x03b9, '\u0399');
    assertMatches(/\u0399/i, 0x03b9, '\u03b9');
    assertMatches(/\u0399/i, 0x1fbe, '\u1fbe');
    assertMatches(/\u03b9/i, 0x0345, '\u0345');
    assertMatches(/\u03b9/i, 0x03b9, '\u0399');
    assertMatches(/\u03b9/i, 0x03b9, '\u03b9');
    assertMatches(/\u03b9/i, 0x1fbe, '\u1fbe');
    assertMatches(/\u1fbe/i, 0x0345, '\u0345');
    assertMatches(/\u1fbe/i, 0x03b9, '\u0399');
    assertMatches(/\u1fbe/i, 0x03b9, '\u03b9');
    assertMatches(/\u1fbe/i, 0x1fbe, '\u1fbe');

    assertMatches(/\u01f1/i, 0x01f3, '\u01f3');
    assertMatches(/\u037f/i, 0x03f3, '\u03f3');
    assertMatches(/\u0528/i, 0x0529, '\u0529');
    assertMatches(/\u052a/i, 0x052b, '\u052b');
    assertMatches(/\u052c/i, 0x052d, '\u052d');
    assertMatches(/\u052e/i, 0x052f, '\u052f');
    assertMatches(/\ua698/i, 0xa699, '\ua699');
    assertMatches(/\ua69a/i, 0xa69b, '\ua69b');
    assertMatches(/\ua796/i, 0xa797, '\ua797');
    assertMatches(/\ua798/i, 0xa799, '\ua799');
    assertMatches(/\ua79a/i, 0xa79b, '\ua79b');
    assertMatches(/\ua79c/i, 0xa79d, '\ua79d');
    assertMatches(/\ua79e/i, 0xa79f, '\ua79f');
    assertMatches(/\ua7ab/i, 0x025c, '\u025c');
    assertMatches(/\ua7ac/i, 0x0261, '\u0261');
    assertMatches(/\ua7ad/i, 0x026c, '\u026c');
    assertMatches(/\ua7b0/i, 0x029e, '\u029e');
    assertMatches(/\ua7b1/i, 0x0287, '\u0287');

})();

//
// Detect regressions in the CaseInsensitive table
//

// 01BA != 01BB under /i.
assertDoesNotMatch(/\u01ba/iu, 0x01bb, "\u01bb");
assertDoesNotMatch(/\u01bb/iu, 0x01ba, "\u01ba");
assertDoesNotMatch(/\u01ba/i, 0x01bb, "\u01bb");
assertDoesNotMatch(/\u01bb/i, 0x01ba, "\u01ba");

// 01F0 doesn't match anything
assertDoesNotMatch(/\u01f0/iu, 0x01f1, "\u01f1");
assertDoesNotMatch(/\u01f1/iu, 0x01f0, "\u01f0");
assertDoesNotMatch(/\u01f0/i, 0x01f1, "\u01f1");
assertDoesNotMatch(/\u01f1/i, 0x01f0, "\u01f0");

// 01F4-5 match (G with ACUTE)
assertMatches(/\u01f4/iu, 0x01f5, "\u01f5");
assertMatches(/\u01f5/iu, 0x01f4, "\u01f4");
assertMatches(/\u01f4/i, 0x01f5, "\u01f5");
assertMatches(/\u01f5/i, 0x01f4, "\u01f4");

//
// Latin ligature triples DZ WITH CARON, LJ, NJ (01C4-01CC); DZ (01F1-3)
//

assertMatches(/\u01c4/iu, 0x01c4, '\u01c4');
assertMatches(/\u01c4/iu, 0x01c5, '\u01c5');
assertMatches(/\u01c4/iu, 0x01c6, '\u01c6');
assertMatches(/\u01c5/iu, 0x01c4, '\u01c4');
assertMatches(/\u01c5/iu, 0x01c5, '\u01c5');
assertMatches(/\u01c5/iu, 0x01c6, '\u01c6');
assertMatches(/\u01c6/iu, 0x01c4, '\u01c4');
assertMatches(/\u01c6/iu, 0x01c5, '\u01c5');
assertMatches(/\u01c6/iu, 0x01c6, '\u01c6');

assertMatches(/\u01c7/iu, 0x01c7, '\u01c7');
assertMatches(/\u01c7/iu, 0x01c8, '\u01c8');
assertMatches(/\u01c7/iu, 0x01c9, '\u01c9');
assertMatches(/\u01c9/iu, 0x01c7, '\u01c7');
assertMatches(/\u01c9/iu, 0x01c8, '\u01c8');
assertMatches(/\u01c9/iu, 0x01c9, '\u01c9');
assertMatches(/\u01c8/iu, 0x01c7, '\u01c7');
assertMatches(/\u01c8/iu, 0x01c8, '\u01c8');
assertMatches(/\u01c8/iu, 0x01c9, '\u01c9');

assertMatches(/\u01ca/iu, 0x01ca, '\u01ca');
assertMatches(/\u01ca/iu, 0x01cb, '\u01cb');
assertMatches(/\u01ca/iu, 0x01cc, '\u01cc');
assertMatches(/\u01cb/iu, 0x01ca, '\u01ca');
assertMatches(/\u01cb/iu, 0x01cb, '\u01cb');
assertMatches(/\u01cb/iu, 0x01cc, '\u01cc');
assertMatches(/\u01cc/iu, 0x01ca, '\u01ca');
assertMatches(/\u01cc/iu, 0x01cb, '\u01cb');
assertMatches(/\u01cc/iu, 0x01cc, '\u01cc');

assertMatches(/\u01f1/iu, 0x01f1, '\u01f1');
assertMatches(/\u01f1/iu, 0x01f2, '\u01f2');
assertMatches(/\u01f1/iu, 0x01f3, '\u01f3');
assertMatches(/\u01f2/iu, 0x01f2, '\u01f2');
assertMatches(/\u01f2/iu, 0x01f1, '\u01f1');
assertMatches(/\u01f2/iu, 0x01f3, '\u01f3');
assertMatches(/\u01f3/iu, 0x01f1, '\u01f1');
assertMatches(/\u01f3/iu, 0x01f2, '\u01f2');
assertMatches(/\u01f3/iu, 0x01f3, '\u01f3');

assertMatches(/\u01c4/i, 0x01c4, '\u01c4');
assertMatches(/\u01c4/i, 0x01c5, '\u01c5');
assertMatches(/\u01c4/i, 0x01c6, '\u01c6');
assertMatches(/\u01c5/i, 0x01c4, '\u01c4');
assertMatches(/\u01c5/i, 0x01c5, '\u01c5');
assertMatches(/\u01c5/i, 0x01c6, '\u01c6');
assertMatches(/\u01c6/i, 0x01c4, '\u01c4');
assertMatches(/\u01c6/i, 0x01c5, '\u01c5');
assertMatches(/\u01c6/i, 0x01c6, '\u01c6');

assertMatches(/\u01c7/i, 0x01c7, '\u01c7');
assertMatches(/\u01c7/i, 0x01c8, '\u01c8');
assertMatches(/\u01c7/i, 0x01c9, '\u01c9');
assertMatches(/\u01c9/i, 0x01c7, '\u01c7');
assertMatches(/\u01c9/i, 0x01c8, '\u01c8');
assertMatches(/\u01c9/i, 0x01c9, '\u01c9');
assertMatches(/\u01c8/i, 0x01c7, '\u01c7');
assertMatches(/\u01c8/i, 0x01c8, '\u01c8');
assertMatches(/\u01c8/i, 0x01c9, '\u01c9');

assertMatches(/\u01ca/i, 0x01ca, '\u01ca');
assertMatches(/\u01ca/i, 0x01cb, '\u01cb');
assertMatches(/\u01ca/i, 0x01cc, '\u01cc');
assertMatches(/\u01cb/i, 0x01ca, '\u01ca');
assertMatches(/\u01cb/i, 0x01cb, '\u01cb');
assertMatches(/\u01cb/i, 0x01cc, '\u01cc');
assertMatches(/\u01cc/i, 0x01ca, '\u01ca');
assertMatches(/\u01cc/i, 0x01cb, '\u01cb');
assertMatches(/\u01cc/i, 0x01cc, '\u01cc');

assertMatches(/\u01f1/i, 0x01f1, '\u01f1');
assertMatches(/\u01f1/i, 0x01f2, '\u01f2');
assertMatches(/\u01f1/i, 0x01f3, '\u01f3');
assertMatches(/\u01f2/i, 0x01f2, '\u01f2');
assertMatches(/\u01f2/i, 0x01f1, '\u01f1');
assertMatches(/\u01f2/i, 0x01f3, '\u01f3');
assertMatches(/\u01f3/i, 0x01f1, '\u01f1');
assertMatches(/\u01f3/i, 0x01f2, '\u01f2');
assertMatches(/\u01f3/i, 0x01f3, '\u01f3');

// 037F and 03F3 - GREEK LETTER YOT
assertMatches(/\u037f/iu, 0x037f, '\u037f');
assertMatches(/\u037f/iu, 0x03f3, '\u03f3');
assertMatches(/\u03f3/iu, 0x037f, '\u037f');
assertMatches(/\u03f3/iu, 0x03f3, '\u03f3');

assertMatches(/\u037f/i, 0x037f, '\u037f');
assertMatches(/\u037f/i, 0x03f3, '\u03f3');
assertMatches(/\u03f3/i, 0x037f, '\u037f');
assertMatches(/\u03f3/i, 0x03f3, '\u03f3');

// New Cyrillic case-mapped pairs
assertMatches(/\u0528/iu, 0x0528, '\u0528');
assertMatches(/\u0528/iu, 0x0529, '\u0529');
assertMatches(/\u0529/iu, 0x0528, '\u0528');
assertMatches(/\u0529/iu, 0x0529, '\u0529');
assertMatches(/\u052a/iu, 0x052a, '\u052a');
assertMatches(/\u052a/iu, 0x052b, '\u052b');
assertMatches(/\u052b/iu, 0x052a, '\u052a');
assertMatches(/\u052b/iu, 0x052b, '\u052b');
assertMatches(/\u052c/iu, 0x052c, '\u052c');
assertMatches(/\u052c/iu, 0x052d, '\u052d');
assertMatches(/\u052d/iu, 0x052c, '\u052c');
assertMatches(/\u052d/iu, 0x052d, '\u052d');
assertMatches(/\u052e/iu, 0x052e, '\u052e');
assertMatches(/\u052e/iu, 0x052f, '\u052f');
assertMatches(/\u052f/iu, 0x052e, '\u052e');
assertMatches(/\u052f/iu, 0x052f, '\u052f');

assertMatches(/\ua698/iu, 0xa698, '\ua698');
assertMatches(/\ua698/iu, 0xa699, '\ua699');
assertMatches(/\ua699/iu, 0xa698, '\ua698');
assertMatches(/\ua699/iu, 0xa699, '\ua699');
assertMatches(/\ua69a/iu, 0xa69a, '\ua69a');
assertMatches(/\ua69a/iu, 0xa69b, '\ua69b');
assertMatches(/\ua69b/iu, 0xa69a, '\ua69a');
assertMatches(/\ua69b/iu, 0xa69b, '\ua69b');

assertMatches(/\u0528/i, 0x0528, '\u0528');
assertMatches(/\u0528/i, 0x0529, '\u0529');
assertMatches(/\u0529/i, 0x0528, '\u0528');
assertMatches(/\u0529/i, 0x0529, '\u0529');
assertMatches(/\u052a/i, 0x052a, '\u052a');
assertMatches(/\u052a/i, 0x052b, '\u052b');
assertMatches(/\u052b/i, 0x052a, '\u052a');
assertMatches(/\u052b/i, 0x052b, '\u052b');
assertMatches(/\u052c/i, 0x052c, '\u052c');
assertMatches(/\u052c/i, 0x052d, '\u052d');
assertMatches(/\u052d/i, 0x052c, '\u052c');
assertMatches(/\u052d/i, 0x052d, '\u052d');
assertMatches(/\u052e/i, 0x052e, '\u052e');
assertMatches(/\u052e/i, 0x052f, '\u052f');
assertMatches(/\u052f/i, 0x052e, '\u052e');
assertMatches(/\u052f/i, 0x052f, '\u052f');

assertMatches(/\ua698/i, 0xa698, '\ua698');
assertMatches(/\ua698/i, 0xa699, '\ua699');
assertMatches(/\ua699/i, 0xa698, '\ua698');
assertMatches(/\ua699/i, 0xa699, '\ua699');
assertMatches(/\ua69a/i, 0xa69a, '\ua69a');
assertMatches(/\ua69a/i, 0xa69b, '\ua69b');
assertMatches(/\ua69b/i, 0xa69a, '\ua69a');
assertMatches(/\ua69b/i, 0xa69b, '\ua69b');

// New Cherokee uppercase-lowercase mappings and case-mapping pairs.
assertMatches(/\u13a0/iu, 0xab70, '\uab70');
assertMatches(/\uab70/iu, 0x13a0, '\u13a0');
assertMatches(/\u13a1/iu, 0xab71, '\uab71');
assertMatches(/\uab71/iu, 0x13a1, '\u13a1');
// ...
assertMatches(/\u13ee/iu, 0xabbe, '\uabbe');
assertMatches(/\uabbe/iu, 0x13ee, '\u13ee');
assertMatches(/\u13ef/iu, 0xabbf, '\uabbf');
assertMatches(/\uabbf/iu, 0x13ef, '\u13ef');

assertMatches(/\u13f0/iu, 0x13f8, '\u13f8');
assertMatches(/\u13f8/iu, 0x13f0, '\u13f0');
assertMatches(/\u13f1/iu, 0x13f9, '\u13f9');
assertMatches(/\u13f9/iu, 0x13f1, '\u13f1');
assertMatches(/\u13f2/iu, 0x13fa, '\u13fa');
assertMatches(/\u13fa/iu, 0x13f2, '\u13f2');
assertMatches(/\u13f3/iu, 0x13fb, '\u13fb');
assertMatches(/\u13fb/iu, 0x13f3, '\u13f3');
assertMatches(/\u13f4/iu, 0x13fc, '\u13fc');
assertMatches(/\u13fc/iu, 0x13f4, '\u13f4');
assertMatches(/\u13f5/iu, 0x13fd, '\u13fd');
assertMatches(/\u13fd/iu, 0x13f5, '\u13f5');

// REVIEW: Not matching without /u is compat with v8, but not what was expected from the UCD data.
// REVIEW: Need to confirm with someone more familiar with spec in this area.
assertDoesNotMatch(/\u13a0/i, 0xab70, '\uab70');
assertDoesNotMatch(/\uab70/i, 0x13a0, '\u13a0');
assertDoesNotMatch(/\u13a1/i, 0xab71, '\uab71');
assertDoesNotMatch(/\uab71/i, 0x13a1, '\u13a1');
// ...
assertDoesNotMatch(/\u13ee/i, 0xabbe, '\uabbe');
assertDoesNotMatch(/\uabbe/i, 0x13ee, '\u13ee');
assertDoesNotMatch(/\u13ef/i, 0xabbf, '\uabbf');
assertDoesNotMatch(/\uabbf/i, 0x13ef, '\u13ef');

assertDoesNotMatch(/\u13f0/i, 0x13f8, '\u13f8');
assertDoesNotMatch(/\u13f8/i, 0x13f0, '\u13f0');
assertDoesNotMatch(/\u13f1/i, 0x13f9, '\u13f9');
assertDoesNotMatch(/\u13f9/i, 0x13f1, '\u13f1');
assertDoesNotMatch(/\u13f2/i, 0x13fa, '\u13fa');
assertDoesNotMatch(/\u13fa/i, 0x13f2, '\u13f2');
assertDoesNotMatch(/\u13f3/i, 0x13fb, '\u13fb');
assertDoesNotMatch(/\u13fb/i, 0x13f3, '\u13f3');
assertDoesNotMatch(/\u13f4/i, 0x13fc, '\u13fc');
assertDoesNotMatch(/\u13fc/i, 0x13f4, '\u13f4');
assertDoesNotMatch(/\u13f5/i, 0x13fd, '\u13fd');
assertDoesNotMatch(/\u13fd/i, 0x13f5, '\u13f5');

// Latin extensions added in Unicode 7.0
assertMatches(/\ua796/iu, 0xa796, '\ua796');
assertMatches(/\ua796/iu, 0xa797, '\ua797');
assertMatches(/\ua797/iu, 0xa796, '\ua796');
assertMatches(/\ua797/iu, 0xa797, '\ua797');
assertMatches(/\ua798/iu, 0xa798, '\ua798');
assertMatches(/\ua798/iu, 0xa799, '\ua799');
assertMatches(/\ua799/iu, 0xa798, '\ua798');
assertMatches(/\ua799/iu, 0xa799, '\ua799');
assertMatches(/\ua79a/iu, 0xa79a, '\ua79a');
assertMatches(/\ua79a/iu, 0xa79b, '\ua79b');
assertMatches(/\ua79b/iu, 0xa79a, '\ua79a');
assertMatches(/\ua79b/iu, 0xa79b, '\ua79b');
assertMatches(/\ua79c/iu, 0xa79c, '\ua79c');
assertMatches(/\ua79c/iu, 0xa79d, '\ua79d');
assertMatches(/\ua79d/iu, 0xa79c, '\ua79c');
assertMatches(/\ua79d/iu, 0xa79d, '\ua79d');
assertMatches(/\ua79e/iu, 0xa79e, '\ua79e');
assertMatches(/\ua79e/iu, 0xa79f, '\ua79f');
assertMatches(/\ua79f/iu, 0xa79e, '\ua79e');
assertMatches(/\ua79f/iu, 0xa79f, '\ua79f');

assertMatches(/\ua796/i, 0xa796, '\ua796');
assertMatches(/\ua796/i, 0xa797, '\ua797');
assertMatches(/\ua797/i, 0xa796, '\ua796');
assertMatches(/\ua797/i, 0xa797, '\ua797');
assertMatches(/\ua798/i, 0xa798, '\ua798');
assertMatches(/\ua798/i, 0xa799, '\ua799');
assertMatches(/\ua799/i, 0xa798, '\ua798');
assertMatches(/\ua799/i, 0xa799, '\ua799');
assertMatches(/\ua79a/i, 0xa79a, '\ua79a');
assertMatches(/\ua79a/i, 0xa79b, '\ua79b');
assertMatches(/\ua79b/i, 0xa79a, '\ua79a');
assertMatches(/\ua79b/i, 0xa79b, '\ua79b');
assertMatches(/\ua79c/i, 0xa79c, '\ua79c');
assertMatches(/\ua79c/i, 0xa79d, '\ua79d');
assertMatches(/\ua79d/i, 0xa79c, '\ua79c');
assertMatches(/\ua79d/i, 0xa79d, '\ua79d');
assertMatches(/\ua79e/i, 0xa79e, '\ua79e');
assertMatches(/\ua79e/i, 0xa79f, '\ua79f');
assertMatches(/\ua79f/i, 0xa79e, '\ua79e');
assertMatches(/\ua79f/i, 0xa79f, '\ua79f');

console.log("PASS");
