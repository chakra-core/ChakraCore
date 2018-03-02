//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Class which extends null won't assert when we do super property references",
    body: function () {
        class c extends null {
            constructor() {
                return {}
            }
            meth() {
                super['prop'] = 'something';
                super.prop = 'something'
            }
        }
        assert.throws(()=>c.prototype.meth.call({}), TypeError, "This shouldn't crash but does throw a TypeError", "Unable to set property 'prop' of undefined or null reference");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
