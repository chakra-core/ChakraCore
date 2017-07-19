//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var oldSet = Map.prototype.set;
var m;
function constructorFunc () {
    m = new Map([["a", 1], ["b", 2]]);
}

Object.defineProperty(Map.prototype, "set", {
    get: Map.prototype.get, // can be any Map.prototype method that depends on `this` being a valid map
    configurable: true
});
assert.throws(function () { return Map.prototype.set });
assert.throws(constructorFunc);

Object.defineProperty(Map.prototype, "set", {
    get: function () { return oldSet; }
});
assert.doesNotThrow(function () { return Map.prototype.set });
assert.doesNotThrow(constructorFunc);
assert.doesNotThrow(function () { m.set("a", 2); });
assert.isTrue(m.get("a") === 2);

WScript.Echo("pass");