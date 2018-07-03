//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

let re = /^[\s-a-z]$/;
let reIgnoreCase = /^[\s-a-z]$/i;
let reUnicode = /^[\s-z]$/u;
let reNoCharClass = /^[a-c-z]$/;

var tests = [
    /*No Flag RegExp Tests begin*/
    {
        name : "Ensure 'a-z' not counted as range",
        body : function () 
        {
            assert.isFalse(re.test("b"));
        }
    },
    {
        name : "Ensure 'a' included in set",
        body : function () 
        {
            assert.isTrue(re.test("a"));
        }
    },
    {
        name : "Ensure ' ' included in set",
        body : function () 
        {
            assert.isTrue(re.test(" "));
        }
    },
    {
        name : "Ensure 'z' included in set",
        body : function () 
        {
            assert.isTrue(re.test("z"));
        }
    },
    {
        name : "Ensure '\t' included in set",
        body : function () 
        {
            assert.isTrue(re.test("\t"));
        }
    },
    {
        name : "Ensure 'a-z' not counted as range",
        body : function () 
        {
            assert.isFalse(re.test("q"));
        }
    },
    {
        name : "Ensure '\' not counted in set",
        body : function () 
        {
            assert.isFalse(re.test("\\"));
        }
    },
    /*No Flag RegExp Tests End*/
    /*IgnoreCase Flag RegExp Tests Begin*/
    {
        name : "Ensure 'O' not included in set",
        body : function () 
        {
            assert.isFalse(reIgnoreCase.test("O"));
        }
    },
    {
        name : "Ensure 'A' included in set",
        body : function () 
        {
            assert.isTrue(reIgnoreCase.test("A"));
        }
    },
    {
        name : "Ensure ' ' included in set",
        body : function () 
        {
            assert.isTrue(reIgnoreCase.test(" "));
        }
    },
    {
        name : "Ensure 'z' included in set",
        body : function () 
        {
            assert.isTrue(reIgnoreCase.test("z"));
        }
    },
    {
        name : "Ensure '\t' included in set",
        body : function () 
        {
            assert.isTrue(reIgnoreCase.test("\t"));
        }
    },
    /*IgnoreCase Flag RegExp Tests End*/
    /*Unicode Flag RegExp Tests Begin*/
    {
        name : "'-' included in set since \s-z treated as union of three types, not range",
        body : function () 
        {
            assert.isTrue(reUnicode.test("-"));
        }
    },
    {
        name : "' ' in set from \s character set",
        body : function () 
        {
            assert.isTrue(reUnicode.test(" "));
        }
    },
    {
        name : "b not included in '\s-z'",
        body : function () 
        {
            assert.isFalse(reUnicode.test("b"));
        }
    },
    /*Unicode Flag RegExp Tests End*/
    /*Non-character class tests Begin*/
    {
        name : "First range is used",
        body : function () 
        {
            assert.isTrue(reNoCharClass.test("b"));
        }
    },
    {
        name : "'-' included in set from 2nd dash",
        body : function () 
        {
            assert.isTrue(reNoCharClass.test("-"));
        }
    },
    {
        name : "z included in set",
        body : function () 
        {
            assert.isTrue(reNoCharClass.test("z"));
        }
    },
    {
        name : "'c-z' not viewed as range",
        body : function () 
        {
            assert.isFalse(reNoCharClass.test("y"));
        }
    }    
    /*Non-character class tests End*/
];

if (typeof modifyTests !== "undefined") {
    tests = modifyTests(tests);
}

testRunner.runTests(tests, {
    verbose : WScript.Arguments[0] != "summary"
});
