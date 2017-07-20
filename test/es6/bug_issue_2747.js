//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

//
// Check Map
//
var oldSet = Map.prototype.set;
var m;
function constructMap () {
    m = new Map([["a", 1], ["b", 2]]);
}

Object.defineProperty(Map.prototype, "set", {
    get: Map.prototype.get, // can be any Map.prototype method that depends on `this` being valid
    configurable: true
});
assert.throws(function () {
    return Map.prototype.set;
}, TypeError, "Getting Map.prototype.set with the altered getter should throw a TypeError");
assert.throws(constructMap, TypeError, "Constructing a Map (uses Map.prototype.set internally) should throw a TypeError");

Object.defineProperty(Map.prototype, "set", {
    get: function () { return oldSet; }
});
assert.doesNotThrow(function () {
    return Map.prototype.set;
}, "Getting Map.prototype.set with the default set function should not throw");
assert.doesNotThrow(constructMap, "Constructing a Map with the default set function should not throw");
assert.doesNotThrow(function () {
    m.set("a", 2);
}, "Inserting a new key/value pair with the default set function should not through");
assert.isTrue(m.get("a") === 2, "Inserting a new key/value pair should actually insert it");

//
// Check Set
//
var oldAdd = Set.prototype.add;
var s;
function constructSet () {
    s = new Set([1, 2, 3, 2, 4, 1]);
}

Object.defineProperty(Set.prototype, "add", {
    get: Set.prototype.has, // can be any Set.prototype method that depends on `this` being valid
    configurable: true
});
assert.throws(function () {
    return Set.prototype.add;
}, TypeError, "Getting Set.prototype.add with the altered getter should throw a TypeError");
assert.throws(constructSet, TypeError, "Constructing a Set (uses Set.prototype.add internally) should throw a TypeError");

Object.defineProperty(Set.prototype, "add", {
    get: function () { return oldAdd; }
});
assert.doesNotThrow(function () {
    return Set.prototype.add;
}, "Getting Set.prototype.add with the default add function should not throw");
assert.doesNotThrow(constructSet, "Constructing a Set with the default add function should not throw");
assert.doesNotThrow(function () {
    s.add(6);
}, "Inserting a new item with the default set function should not throw");
assert.isTrue(s.has(6) === true, "Inserting a new item should actually insert it");

WScript.Echo("pass");
