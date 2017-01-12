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

//
// Detect regressions in the CaseInsensitive table
//

// 01BA != 01BB under /i.
assertDoesNotMatch(/\u{01ba}/iu, 0x01bb, "\u{01bb}");
assertDoesNotMatch(/\u{01bb}/iu, 0x01ba, "\u{01ba}");

// 01F0 doesn't match anything
assertDoesNotMatch(/\u{01f0}/iu, 0x01f1, "\u{01f1}");
assertDoesNotMatch(/\u{01f1}/iu, 0x01f0, "\u{01f0}");

// 01F4-5 match (G with ACUTE)
assertMatches(/\u{01f4}/iu, 0x01f5, "\u{01f5}");
assertMatches(/\u{01f5}/iu, 0x01f4, "\u{01f4}");

//
// Latin ligature triples DZ WITH CARON, LJ, NJ (01C4-01CC); DZ (01F1-3)
//

assertMatches(/\u{01c4}/iu, 0x01c4, '\u{01c4}');
assertMatches(/\u{01c4}/iu, 0x01c5, '\u{01c5}');
assertMatches(/\u{01c4}/iu, 0x01c6, '\u{01c6}');
assertMatches(/\u{01c5}/iu, 0x01c4, '\u{01c4}');
assertMatches(/\u{01c5}/iu, 0x01c5, '\u{01c5}');
assertMatches(/\u{01c5}/iu, 0x01c6, '\u{01c6}');
assertMatches(/\u{01c6}/iu, 0x01c4, '\u{01c4}');
assertMatches(/\u{01c6}/iu, 0x01c5, '\u{01c5}');
assertMatches(/\u{01c6}/iu, 0x01c6, '\u{01c6}');

assertMatches(/\u{01c7}/iu, 0x01c7, '\u{01c7}');
assertMatches(/\u{01c7}/iu, 0x01c8, '\u{01c8}');
assertMatches(/\u{01c7}/iu, 0x01c9, '\u{01c9}');
assertMatches(/\u{01c9}/iu, 0x01c7, '\u{01c7}');
assertMatches(/\u{01c9}/iu, 0x01c8, '\u{01c8}');
assertMatches(/\u{01c9}/iu, 0x01c9, '\u{01c9}');
assertMatches(/\u{01c8}/iu, 0x01c7, '\u{01c7}');
assertMatches(/\u{01c8}/iu, 0x01c8, '\u{01c8}');
assertMatches(/\u{01c8}/iu, 0x01c9, '\u{01c9}');

assertMatches(/\u{01ca}/iu, 0x01ca, '\u{01ca}');
assertMatches(/\u{01ca}/iu, 0x01cb, '\u{01cb}');
assertMatches(/\u{01ca}/iu, 0x01cc, '\u{01cc}');
assertMatches(/\u{01cb}/iu, 0x01ca, '\u{01ca}');
assertMatches(/\u{01cb}/iu, 0x01cb, '\u{01cb}');
assertMatches(/\u{01cb}/iu, 0x01cc, '\u{01cc}');
assertMatches(/\u{01cc}/iu, 0x01ca, '\u{01ca}');
assertMatches(/\u{01cc}/iu, 0x01cb, '\u{01cb}');
assertMatches(/\u{01cc}/iu, 0x01cc, '\u{01cc}');

assertMatches(/\u{01f1}/iu, 0x01f1, '\u{01f1}');
assertMatches(/\u{01f1}/iu, 0x01f2, '\u{01f2}');
assertMatches(/\u{01f1}/iu, 0x01f3, '\u{01f3}');
assertMatches(/\u{01f2}/iu, 0x01f2, '\u{01f2}');
assertMatches(/\u{01f2}/iu, 0x01f1, '\u{01f1}');
assertMatches(/\u{01f2}/iu, 0x01f3, '\u{01f3}');
assertMatches(/\u{01f3}/iu, 0x01f1, '\u{01f1}');
assertMatches(/\u{01f3}/iu, 0x01f2, '\u{01f2}');
assertMatches(/\u{01f3}/iu, 0x01f3, '\u{01f3}');

// 037F and 03F3 - GREEK LETTER YOT
assertMatches(/\u{037f}/iu, 0x037f, '\u{037f}');
assertMatches(/\u{037f}/iu, 0x03f3, '\u{03f3}');
assertMatches(/\u{03f3}/iu, 0x037f, '\u{037f}');
assertMatches(/\u{03f3}/iu, 0x03f3, '\u{03f3}');

// New Cyrillic case-mapped pairs
assertMatches(/\u{0528}/iu, 0x0528, '\u{0528}');
assertMatches(/\u{0528}/iu, 0x0529, '\u{0529}');
assertMatches(/\u{0529}/iu, 0x0528, '\u{0528}');
assertMatches(/\u{0529}/iu, 0x0529, '\u{0529}');
assertMatches(/\u{052a}/iu, 0x052a, '\u{052a}');
assertMatches(/\u{052a}/iu, 0x052b, '\u{052b}');
assertMatches(/\u{052b}/iu, 0x052a, '\u{052a}');
assertMatches(/\u{052b}/iu, 0x052b, '\u{052b}');
assertMatches(/\u{052c}/iu, 0x052c, '\u{052c}');
assertMatches(/\u{052c}/iu, 0x052d, '\u{052d}');
assertMatches(/\u{052d}/iu, 0x052c, '\u{052c}');
assertMatches(/\u{052d}/iu, 0x052d, '\u{052d}');
assertMatches(/\u{052e}/iu, 0x052e, '\u{052e}');
assertMatches(/\u{052e}/iu, 0x052f, '\u{052f}');
assertMatches(/\u{052f}/iu, 0x052e, '\u{052e}');
assertMatches(/\u{052f}/iu, 0x052f, '\u{052f}');

assertMatches(/\u{a698}/iu, 0xa698, '\u{a698}');
assertMatches(/\u{a698}/iu, 0xa699, '\u{a699}');
assertMatches(/\u{a699}/iu, 0xa698, '\u{a698}');
assertMatches(/\u{a699}/iu, 0xa699, '\u{a699}');
assertMatches(/\u{a69a}/iu, 0xa69a, '\u{a69a}');
assertMatches(/\u{a69a}/iu, 0xa69b, '\u{a69b}');
assertMatches(/\u{a69b}/iu, 0xa69a, '\u{a69a}');
assertMatches(/\u{a69b}/iu, 0xa69b, '\u{a69b}');

console.log("PASS");
