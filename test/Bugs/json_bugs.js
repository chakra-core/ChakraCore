//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "JSON.stringify on proxy object with different length",
    body: function () {
        var i = 0;
        var ret = JSON.stringify(new Proxy([], {
            get(t, pk, r){
                if (pk === "length") {
                    return ++i;
                }
                return Reflect.get(t, pk, r);
            }
        }));
        assert.areEqual("[null]", ret, "JSON.stringify will work on the array with the length 1");
        assert.areEqual(1, i, 'proxy.get with property "length" will be called only once');
    }
  },
  {
    name: "JSON.stringify on proxy object with length > 2**31",
    body: function () {
        assert.throws(() =>
        JSON.stringify(new Proxy([], {
            get(t, pk, r){
                if (pk === "length") {
                    return 2**31 + 1;
                }
                return Reflect.get(t, pk, r);
            }
        })), RangeError, "JSON.stringify will throw range error when needs to allocate string more that 2**31", "String length is out of bound");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
