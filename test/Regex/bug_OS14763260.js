//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Verify last match invalidated as expected",
    body: function () {
        const r1 = /(abc)/;
        const r2 = /(def)/;
        const s1 = "abc";
        const s2 = "def";
         
        r1.test(s1);
        r2.test(s2);
        r1.test(s1);

        assert.areEqual("abc", RegExp.$1, "Stale last match should be invalidated by second r1.test(s1)");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
