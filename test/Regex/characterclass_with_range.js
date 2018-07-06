//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testRegExp(str, regexp, expectedResult)
{
    actualResult = regexp.test(str);
    errorMsg = "Expected result of test for match between string: '" + str + "' and regular expression: " + regexp + " to be " + 
                    expectedResult + " but was " + actualResult;
    assert.areEqual(expectedResult, actualResult, errorMsg); 
}

var tests = [
    {
        name : "RegExp tests with no flags",
        body : function () 
        {
            let re = /[\s-a-z]/;
            testRegExp("b", re, false);
            testRegExp("a", re, true);
            testRegExp(" ", re, true);
            testRegExp("z", re, true);
            testRegExp("\t", re, true);
            testRegExp("q", re, false);
            testRegExp("\\", re, false);
            testRegExp("\u2028", re, true);
            testRegExp("\u2009", re, true);
        }
    },
    {
        name : "RegExp tests with IgnoreCase flag set",
        body : function () 
        {
            let reIgnoreCase = /^[\s-a-z]$/i;
            testRegExp("O", reIgnoreCase, false);
            testRegExp("A", reIgnoreCase, true);
            testRegExp(" ", reIgnoreCase, true);
            testRegExp("z", reIgnoreCase, true);
            testRegExp("\t", reIgnoreCase, true);
            testRegExp("\u2028", reIgnoreCase, true);
            testRegExp("\u2009", reIgnoreCase, true);
        }
    },
    {
        name : "RegExp tests with Unicode flag set",
        body : function () 
        {
            let reUnicode = /^[a-d]$/u;
            testRegExp("a", reUnicode, true);
            testRegExp("c", reUnicode, true);
            testRegExp("d", reUnicode, true);
            testRegExp("C", reUnicode, false);
            testRegExp("g", reUnicode, false);
            testRegExp("\u2028", reUnicode, false);
            testRegExp("\u2009", reUnicode, false);
            assert.throws(() => eval("/^[\\s-z]$/u.exec(\"-\")"), SyntaxError, "Expected an error due to character sets not being allowed in ranges when unicode flag is set.", "Character classes not allowed in class ranges");
        }
    },
    {
        name : "Non-character class tests",
        body : function () 
        {
            let reNoCharClass = /^[a-c-z]$/;
            testRegExp("b", reNoCharClass, true);
            testRegExp("-", reNoCharClass, true);
            testRegExp("z", reNoCharClass, true);
            testRegExp("y", reNoCharClass, false);
        }
    },
    {
        name : "Regression tests from bugFixRegression",
        body : function () 
        {
            assert.areEqual(" -a", /[\s-a-c]*/.exec(" -abc")[0]);
            assert.areEqual(" -abc", /[\s\-a-c]*/.exec(" -abc")[0]);
            assert.areEqual(" -ab", /[a-\s-b]*/.exec(" -ab")[0]);
            assert.areEqual(" -ab", /[a\-\s\-b]*/.exec(" -ab")[0]);
        }
    },
    {
        name : "Special character tests",
        body : function () 
        {
                let re = /^[\s][a\sb][\s--c-f]$/;
                testRegExp('  \\', re, false);
                testRegExp(' \\ ', re, false);
                testRegExp('\\  ', re, false);
                re = /[-][\d\-]/;
                testRegExp('--', re, true);
                testRegExp('-9', re, true);
                testRegExp('  ', re, false);
        }
    },
    {
        name : "Negation character set tests",
        body : function () 
        {
                let reNegationCharSet = /[\D-\s]+/;
                testRegExp('555686', reNegationCharSet, false);
                testRegExp('555-686', reNegationCharSet, true);
                testRegExp('alphabet-123', reNegationCharSet, true);
        }
    },
    {
        name : "Non-range tests",
        body : function () 
        {
                let reNonRange = /[-\w]/
                testRegExp('-', reNonRange, true);
                testRegExp('g', reNonRange, true);
                testRegExp('5', reNonRange, true);
                testRegExp(' ', reNonRange, false);
                testRegExp('\t', reNonRange, false);
                testRegExp('\u2028', reNonRange, false);
                
                reNonRange = /[\w-]/
                testRegExp('-', reNonRange, true);
                testRegExp('g', reNonRange, true);
                testRegExp('5', reNonRange, true);
                testRegExp(' ', reNonRange, false);
                testRegExp('\t', reNonRange, false);
                testRegExp('\u2028', reNonRange, false);
        }
    }
];

testRunner.runTests(tests, {
    verbose : WScript.Arguments[0] != "summary"
});
