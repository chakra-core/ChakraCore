//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function matchRegExp(str, regexpLiteral, expectedResult)
{
    matchResultLiteral = str.match(regexpLiteral);
    errorMsgBase = "Expected result of match between string: '" + str + "' and regular expression: " + regexpLiteral + " to be " + 
                    expectedResult + " but was "

    actualResultLiteral = matchResultLiteral == null ? null : matchResultLiteral[0];
    assert.areEqual(expectedResult, actualResultLiteral, errorMsgBase + actualResultLiteral); 
    
    regexpConstructor = new RegExp(regexpLiteral);
    matchResultConstructor = str.match(regexpConstructor);

    actualResultConstructor = matchResultConstructor == null ? null : matchResultConstructor[0];
    assert.areEqual(expectedResult, actualResultConstructor, errorMsgBase + actualResultConstructor); 
}

var tests = [
    {
        name : "Control characters followed by a word character ([A-Za-z0-9_])",
        body : function () 
        {
            re = /[\c6]+/; //'6' = ascii x36, parsed as [\x16]+
            matchRegExp("6", re, null);
            matchRegExp("\\", re, null);
            matchRegExp("\\c6", re, null);
            matchRegExp("c", re, null);
            matchRegExp("\x16", re, "\x16");
            
            re = /\c6/; //'6' = ascii x36, parsed as "\\c6"
            matchRegExp("\\c6", re, "\\c6");
            matchRegExp("\\", re, null);
            matchRegExp("6", re, null);
            matchRegExp("c", re, null);
            matchRegExp("\x16", re, null);
            
            re = /\c6[\c6]+/; //'6' = ascii x36, parsed as "\\c6"[\x16]+
            matchRegExp("\\c6\x16", re, "\\c6\x16");
            matchRegExp("\\", re, null);
            matchRegExp("c", re, null);
            matchRegExp("\x16", re, null);
            
            re = /[\ca]+/; //'a' = ascii x61, parsed as [\x01]+
            matchRegExp("a", re, null);
            matchRegExp("\\", re, null);
            matchRegExp("c", re, null);
            matchRegExp("00xyzabc123\x01qrst", re, "\x01");
	    
            re = /[\c_]+/; //'_' = ascii 0x5F, parsed as [\x1F]+
            matchRegExp("\x1F\x1F\x05", re, "\x1F\x1F");
            matchRegExp("\\\\\\", re, null);
            matchRegExp("////", re, null);
            matchRegExp("ccc_", re, null);
            
            re = /[\cG]*/; //'G' = ascii x47, parsed as [\x07]*
            matchRegExp("\x07\x06\x05", re, "\x07");
            matchRegExp("\\\\", re, "");
            matchRegExp("////", re, "");
            matchRegExp("cccG", re, "");
            
            re = /[\cG\c6\cf]+/; //'G' = ascii x47, '6' = ascii x36, 'f' = ascii x66, parsed as [\x07\x16\x06]+
            matchRegExp("\x00\x03\x07\x06\x16\x07\x08", re, "\x07\x06\x16\x07");
            matchRegExp("\\\\", re, null);
            matchRegExp("////", re, null);
            matchRegExp("cfG6", re, null);
            
            re = /\cG\cf/; //'G' = ascii x47, 'f' = ascii x66, parsed as "\x07\x06"
            matchRegExp("\x00\x03\x07\x06\x16\x07\x08", re, "\x07\x06");
            matchRegExp("\\", re, null);
            matchRegExp("/", re, null);
            matchRegExp("\\cG\\c6\\cf", re, null);
            
            re = /[\cz\cZ]+/; //'z' = ascii x7A, 'Z' = ascii x5A, have the same lowest 5 bits, parsed as [\x1A]+
            matchRegExp("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f" + 
                        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f", re, "\x1a");
            matchRegExp("\\\\", re, null);
            matchRegExp("////", re, null);
            matchRegExp("ccczZ", re, null);
        }
    },
    {
        name : "Control characters followed by a non-word character ([^A-Za-z0-9_])",
        body : function () 
        {
            re = /[\c*]+/; //'*' = ascii 42, parsed as [\\c*]+ 
            matchRegExp("\x0a\x09\x08", re, null);
            matchRegExp("a*c*b*d*", re, "*c*");
            matchRegExp("\\\\", re, "\\\\");
            matchRegExp("////", re, null);
            matchRegExp("ccc", re, "ccc");
            
            re = /[\c}]*/; //'}' = ascii 125, parsed as [\\c}]*
            matchRegExp("\x1d\x7d\x3d", re, "");
            matchRegExp("}c}}cd*c*b*d*", re, "}c}}c");
            matchRegExp("\\\\", re, "\\\\");
            matchRegExp("////", re, "");
            matchRegExp("ccc", re, "ccc");
            
            re = /[\c;]+/; //';' = ascii 59, parsed as [\\c;]+
            matchRegExp("\x1b\x1c", re, null);
            matchRegExp("d;c;d;*", re, ";c;");
            matchRegExp("\\\\", re, "\\\\");
            matchRegExp("////", re, null);
            matchRegExp("ccc", re, "ccc");
            
            re = /\c%/; //'%' = ascii x25, parsed as \\c%
            matchRegExp("\\", re, null);
            matchRegExp("\\", re, null);
            matchRegExp("\\c%", re, "\\c%");
            matchRegExp("\x05", re, null);
        }
    },
    {
        name : "Control Character tests with unicode flag present",
        body : function () 
        {
            re = /[\cAg]+/u; //'A' = ascii x41, parsed as [g\x01]+
            matchRegExp("abcdefghi", re, "g");
            matchRegExp("\\\\", re, null);
            matchRegExp("////", re, null);
            matchRegExp("\x01\x01gg\x02\x04ggg", re, "\x01\x01gg");            
            
            re = /[\czA]+/u;  //'z' = ascii x7A, parsed as [\x1AA]+
            matchRegExp("abcdefghi", re, null);
            matchRegExp("\\\\", re, null);
            matchRegExp("////", re, null);
            matchRegExp("YZA\x1aABC", re, "A\x1aA");    
            
            assert.throws(() => eval("\"\".match(/[\\c]/u)"), SyntaxError, "(Character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by no character here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/[\\c-d]/u)"), SyntaxError, "(Character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by a dash, '-', here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/[ab\\c_$]/u)"), SyntaxError, "(Character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by an underscore, '_', here.",
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/[ab\\c\\d]/u)"), SyntaxError, "(Character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by a backslash, '\\', here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/[ab\\c3]/u)"), SyntaxError, "(Character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by a number, '3', here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
                        
            re = /\cAg/u;  //'A' = ascii x41, parsed as "\x01g"
            matchRegExp("abcdefghi", re, null);
            matchRegExp("\\\\", re, null);
            matchRegExp("////", re, null);
            matchRegExp("\x01\x01gg\x02\x04ggg", re, "\x01g");            
            
            re = /\czA/u;  //'z' = ascii x7A, parsed as "\x1aA"
            matchRegExp("abcdefghi", re, null);
            matchRegExp("\\\\", re, null);
            matchRegExp("////", re, null);
            matchRegExp("YZA\x1aABC", re, "\x1aA");   
            
            assert.throws(() => eval("\"\".match(/\\c/u)"), SyntaxError, "(Non-character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by no character here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/\\c-d/u)"), SyntaxError, "(Non-character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by a dash, '-', here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/ab\\c_$/u)"), SyntaxError, "(Non-character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by an underscore, '_', here.",
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/ab\\c\\d/u)"), SyntaxError, "(Non-character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by a backslash, '\\', here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
            assert.throws(() => eval("\"\".match(/ab\\c3/u)"), SyntaxError, "(Non-character class) Expected an error because escaped c must be followed by a letter when unicode flag is present, but is followed by a number, '3', here.", 
                        "Invalid regular expression: invalid escape in unicode pattern");
        }
    },
    {
        name : "Control character edge cases",
        body : function () 
        {
            re = /[\c-g]+/; //'-' = ascii x2D, parsed as [\\c-g]+ 
            matchRegExp("abcdefghi", re, "cdefg");
            matchRegExp("\\\\", re, "\\\\");
            matchRegExp("////", re, null);
            matchRegExp("\x0d", re, null);
            matchRegExp("aobd\\f\\d", re, "d\\f\\d");            
            
            re = /[\c-]+/; //'-' = ascii x2D, parsed as [\\c-]+
            matchRegExp("abcdefghi", re, "c");
            matchRegExp("\x0dc--c", re, "c--c");
            matchRegExp("\\\\", re, "\\\\");
            matchRegExp("////", re, null);
            matchRegExp("aobd\\f\\d", re, "\\");  
            
            assert.throws(() => eval("\"\".match(/[\\c-a]/)"), SyntaxError, "Expected an error due to 'c-a' being an invalid range.", "Invalid range in character set");
        }
    }
];

testRunner.runTests(tests, {
    verbose : WScript.Arguments[0] != "summary"
});
