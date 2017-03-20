//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// 1b: U+0041 (A) Uppercase Latin A
// 2b: U+00A1 (¡) inverted exclamation mark
// 2b: U+0101 (ā) LATIN SMALL LETTER A WITH MACRON
// 3b: U+2014 (—) em dash
// 4b: U+10401 Deseret Long E -- surrogate pair \uD801\uDC01

const A         = "\u0041";    // U+0041 (ASCII); UTF-16 0x0041       ; UTF-8 0x41
const iexcl     = "\u00A1";    // U+00A1        ; UTF-16 0x00A1       ; UTF-8 0xC2 0xA0
const amacron   = "\u0101";    // U+0101        ; UTF-16 0x0101       ; UTF-8 0xC4 0x81
const emdash    = "\u2014";    // U+2014        ; UTF-16 0x2014       ; UTF-8 0xE2 0x80 0x94
const desLongE  = "\u{10401}"; // U+10401       ; UTF-16 0xD801 0xDC01; UTF-8 0xF0 0x90 0x90 0x81

console.log(`${A} ${iexcl} ${amacron} ${emdash} ${desLongE}`);
console.log("русский 中文");
