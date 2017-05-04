//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
  this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var a = new ArrayBuffer(0x10);

function foo() {
  detach(a);
  return 6;
};

function detach(ab) {
  var c = ArrayBuffer.transfer(a, 0x10);
};

var obj = {
  valueOf: foo
};

function test() {
  ArrayBuffer.transfer(a, obj);
}

assert.throws(test, TypeError, "", "ArrayBuffer.transfer: The ArrayBuffer is detached.");

print("pass");
