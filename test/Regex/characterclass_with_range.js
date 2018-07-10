//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function matchRegExp(str, regexp, expectedResult)
{
    matchResult = str.match(regexp);//regexp.test(str);
    errorMsg = "Expected result of match between string: '" + str + "' and regular expression: " + regexp + " to be " + 
                    expectedResult + " but was " + matchResult;

    actualResult = matchResult == null ? null : matchResult[0];
    assert.areEqual(expectedResult, actualResult, errorMsg); 
}

var tests = [
    {
        name : "RegExp tests with no flags",
        body : function () 
        {
            let re = /[\s-a-z]/;
            matchRegExp("b", re, null);
            matchRegExp("a", re, "a");
            matchRegExp(" ", re, " ");
            matchRegExp("z", re, "z");
            matchRegExp("\t", re, "\t");
            matchRegExp("q", re, null);
            matchRegExp("\\", re, null);
            matchRegExp("\u2028", re, "\u2028");
            matchRegExp("\u2009", re, "\u2009");
        }
    },
    {
        name : "RegExp tests with IgnoreCase flag set",
        body : function () 
        {
            let reIgnoreCase = /^[\s-a-z]$/i;
            matchRegExp("O", reIgnoreCase, null);
            matchRegExp("A", reIgnoreCase, "A");
            matchRegExp(" ", reIgnoreCase, " ");
            matchRegExp("z", reIgnoreCase, "z");
            matchRegExp("\t", reIgnoreCase, "\t");
            matchRegExp("\u2028", reIgnoreCase, "\u2028");
            matchRegExp("\u2009", reIgnoreCase, "\u2009");
        }
    },
    {
        name : "RegExp tests with Unicode flag set",
        body : function () 
        {
            let reUnicode = /^[a-d]$/u;
            matchRegExp("a", reUnicode, "a");
            matchRegExp("c", reUnicode, "c");
            matchRegExp("d", reUnicode, "d");
            matchRegExp("C", reUnicode, null);
            matchRegExp("g", reUnicode, null);
            matchRegExp("\u2028", reUnicode, null);
            matchRegExp("\u2009", reUnicode, null);
            assert.throws(() => eval("/^[\\s-z]$/u.exec(\"-\")"), SyntaxError, "Expected an error due to character sets not being allowed in ranges when unicode flag is set.", "Character classes not allowed in a RegExp class range.");
            assert.throws(() => eval("/^[z-\\s]$/u.exec(\"-\")"), SyntaxError, "Expected an error due to character sets not being allowed in ranges when unicode flag is set.", "Character classes not allowed in a RegExp class range.");
        
        }
    },
    {
        name : "Non-character class tests",
        body : function () 
        {
            let reNoCharClass = /^[a-c-z]$/;
            matchRegExp("b", reNoCharClass, "b");
            matchRegExp("-", reNoCharClass, "-");
            matchRegExp("z", reNoCharClass, "z");
            matchRegExp("y", reNoCharClass, null);
        }
    },
    {
        name : "Regression tests from bugFixRegression",
        body : function () 
        {
            matchRegExp(" -abc", /[\s-a-c]*/, " -a");
            matchRegExp(" -abc", /[\s\-a-c]*/, " -abc");
            matchRegExp(" -ab", /[a-\s-b]*/, " -ab");
            matchRegExp(" -ab", /[a\-\s\-b]*/, " -ab");
            assert.throws(() => eval("/^[\\s--c-!]$/.exec(\"-./0Abc!\")"), SyntaxError, "Expected an error due to 'c-!' being an invalid range.", "Invalid range in character set");
        }
    },
    {
        name : "Special character tests",
        body : function () 
        {
                let re = /^[\s][a\sb][\s--c-f]$/;
                matchRegExp('  \\', re, null);
                matchRegExp(' \\ ', re, null);
                matchRegExp('\\  ', re, null);
                re = /[-][\d\-]/;
                matchRegExp('--', re, '--');
                matchRegExp('-9', re, '-9');
                matchRegExp('  ', re, null);
                matchRegExp('-\\', re, null);
        }
    },
    {
        name : "Negation character set tests",
        body : function () 
        {
                let reNegationCharSet = /[\D-\s]+/;
                matchRegExp('555686', reNegationCharSet, null);
                matchRegExp('555-686', reNegationCharSet, '-');
                matchRegExp('alphabet-123', reNegationCharSet, 'alphabet-');
        }
    },
    {
        name : "Non-range tests",
        body : function () 
        {
                let reNonRange = /[-\w]/
                matchRegExp('-', reNonRange, '-');
                matchRegExp('g', reNonRange, 'g');
                matchRegExp('5', reNonRange, '5');
                matchRegExp(' ', reNonRange, null);
                matchRegExp('\t', reNonRange, null);
                matchRegExp('\u2028', reNonRange, null);
                matchRegExp('\\', reNonRange, null);
                
                reNonRange = /[\w-]/
                matchRegExp('-', reNonRange, '-');
                matchRegExp('g', reNonRange, 'g');
                matchRegExp('5', reNonRange, '5');
                matchRegExp(' ', reNonRange, null);
                matchRegExp('\t', reNonRange, null);
                matchRegExp('\u2028', reNonRange, null);
                matchRegExp('\\', reNonRange, null); 
        }
    }
];

testRunner.runTests(tests, {
    verbose : WScript.Arguments[0] != "summary"
});
