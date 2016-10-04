//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function makeProperty(obj, prop) {
    Object.defineProperty(obj, prop, {
      configurable: false,
      writable: true,
      value:'basic'
    });
}
var tests = [
  {
    name: "Delete non-configurable property on Array.prototype.copyWithin",
    body: function () {
        var obj = {length: 4};
        makeProperty(obj, '3');
        assert.throws(() => Array.prototype.copyWithin.call(obj, 3, 0), TypeError, "copyWithin is trying to delete the non-configurable property","Cannot delete non-configurable property '3'");
    }
  },
  {
    name: "Delete non-configurable property on Array.prototype.pop",
    body: function () {
        var obj = {length: 4};
        makeProperty(obj, '3');
        assert.throws(() => Array.prototype.pop.call(obj), TypeError, "pop is trying to delete the non-configurable property", "Cannot delete non-configurable property '3'");
    }
  },
  {
    name: "Delete non-configurable property on Array.prototype.shift",
    body: function () {
        var obj = {length: 4};
        makeProperty(obj, '3');
        assert.throws(() => Array.prototype.shift.call(obj), TypeError, "shift is trying to delete the non-configurable property", "Cannot delete non-configurable property '3'");
    }
  },
  {
    name: "Delete non-configurable property on Array.prototype.reverse",
    body: function () {
        var obj = {length: 4};
        makeProperty(obj, '3');
        assert.throws(() => Array.prototype.reverse.call(obj), TypeError, "reverse is trying to delete the non-configurable property", "Cannot delete non-configurable property '3'");
    }
  },
  
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
