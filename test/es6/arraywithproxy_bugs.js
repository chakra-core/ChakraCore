//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Issue#3530 - Incorrect behavior of Array.prototype.filter()",
    body: function () {
        var words = ["abc", "aaaaaaa", "abcd", "aaaaabcd", "abcde", "aaaaabcdef"];

        var p = new Proxy([],  {
            get: function(oTarget, sKey) {
                if (sKey.toString()=="constructor") {
                    return Reflect.get(oTarget, sKey);
                }
            },
            has: function (oTarget, sKey) {
                return Reflect.has(oTarget, sKey);
            },
        });

        Object.setPrototypeOf(words, p);
        words.length = 10;
        assert.areEqual(["aaaaaaa", "aaaaabcd", "aaaaabcdef"], Array.prototype.filter.call(words, function(word){ return word.length > 6; }));
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
