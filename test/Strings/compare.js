//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js", "self");

var str1 = "abcd1234"
var str2 = "1234567a"
var str3 = "abcd12345"
var str1a = "abcd1234"
var str1null = "\0\0ab\0\0cd1234\0"

assert.isTrue(str1.localeCompare(str1a) == 0);
assert.isTrue(str1.localeCompare(str1null) == 0);
assert.isTrue(str1.localeCompare(str2) > 0);
assert.isTrue(str1.localeCompare(str3) < 0);
assert.isTrue(str1.localeCompare() < 0);

console.log("pass")
