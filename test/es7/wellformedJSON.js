//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Broken surrogate pairs should be escaped during JSON.stringify",
        body: function () {
            assert.areEqual(JSON.stringify("\uD834"), '"\\ud834"',
              'JSON.stringify("\\uD834")');
            assert.areEqual(JSON.stringify("\uDF06"), '"\\udf06"',
              'JSON.stringify("\\uDF06")');

            assert.areEqual(JSON.stringify("\uD834\uDF06"), '"𝌆"',
              'JSON.stringify("\\uD834\\uDF06")');
            assert.areEqual(JSON.stringify("\uD834\uD834\uDF06\uD834"), '"\\ud834𝌆\\ud834"',
              'JSON.stringify("\\uD834\\uD834\\uDF06\\uD834")');
            assert.areEqual(JSON.stringify("\uD834\uD834\uDF06\uDF06"), '"\\ud834𝌆\\udf06"',
              'JSON.stringify("\\uD834\\uD834\\uDF06\\uDF06")');
            assert.areEqual(JSON.stringify("\uDF06\uD834\uDF06\uD834"), '"\\udf06𝌆\\ud834"',
              'JSON.stringify("\\uDF06\\uD834\\uDF06\\uD834")');
            assert.areEqual(JSON.stringify("\uDF06\uD834\uDF06\uDF06"), '"\\udf06𝌆\\udf06"',
              'JSON.stringify("\\uDF06\\uD834\\uDF06\\uDF06")');

            assert.areEqual(JSON.stringify("\uDF06\uD834"), '"\\udf06\\ud834"',
              'JSON.stringify("\\uDF06\\uD834")');
            assert.areEqual(JSON.stringify("\uD834\uDF06\uD834\uD834"), '"𝌆\\ud834\\ud834"',
              'JSON.stringify("\\uD834\\uDF06\\uD834\\uD834")');
            assert.areEqual(JSON.stringify("\uD834\uDF06\uD834\uDF06"), '"𝌆𝌆"',
              'JSON.stringify("\\uD834\\uDF06\\uD834\\uDF06")');
            assert.areEqual(JSON.stringify("\uDF06\uDF06\uD834\uD834"), '"\\udf06\\udf06\\ud834\\ud834"',
              'JSON.stringify("\\uDF06\\uDF06\\uD834\\uD834")');
            assert.areEqual(JSON.stringify("\uDF06\uDF06\uD834\uDF06"), '"\\udf06\\udf06𝌆"',
              'JSON.stringify("\\uDF06\\uDF06\\uD834\\uDF06")');
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
