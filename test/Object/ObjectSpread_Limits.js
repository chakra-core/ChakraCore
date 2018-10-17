//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Object Spread unit tests

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    // Disabled for now until decision on whether this should be included
    // {
    //     name: "Test Maximum Spread size",
    //     body: function() {
    //         let maxSize = 243019865;
    //         let arr = new Array(maxSize);
    //         for (i = 0; i < maxSize; i++) {
    //             arr[i] = i;
    //         }
    //         let obj = {...arr};
    //         assert.areEqual(maxSize, Object.keys(obj).length);
    //         for(var propName in obj) {
    //             propValue = obj[propName];
    //             assert.areEqual(propName, propValue.toString());
    //         }
    //     }
    // },
    {
        name: "Test Spread near array limits",
        body: function() {
            function testRange(start, end) {
                let arr = [] ;
                for (i = start; i < end; i++) {
                    arr[i] = i;
                }
                let obj = {...arr};
                assert.areEqual(end-start, Object.keys(obj).length);
                for(var propName in obj) {
                    propValue = obj[propName];
                    assert.areEqual(propName, propValue.toString());
                }
            };
            testRange(2**31-100, 2**31+100);
            testRange(2**32-100, 2**32+100);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
