//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


var TEST = function(a, b) {
  if (a != b) {
    throw new Error(a + " != " + b);
  }
}

var fnc = function(n) { this.number = n };
fnc.prototype.toJSON = function() {
  return this.number.toString();
}

// test - function prototype new instance
TEST("\"1\"", JSON.stringify(new fnc(1)))

// test - pre-post alter Date toJSON definition
var dateString = JSON.stringify(new Date(0));
TEST("1970", dateString.substr(dateString.indexOf("1970"), 4))

Date.prototype.toJSON = 1;
TEST("{}", JSON.stringify(new Date(0)))

delete Date.prototype.toJSON
TEST("{}", JSON.stringify(new Date(0)))

// test - use from Object prototype
Object.prototype.toJSON = function() { return 2; }
delete fnc.prototype.toJSON;
TEST(2, JSON.stringify(new fnc(1)))

// test - symbol
Object.prototype[Symbol("toJSON")] = function() { return 3; }
TEST(2, JSON.stringify(new fnc(1)))

console.log("PASS")
