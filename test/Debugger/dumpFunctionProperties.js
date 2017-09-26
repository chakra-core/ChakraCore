//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo() {
  var x = 1; /**bp:dumpFunctionProperties();**/
}
foo();

(function () {
  var x = 1; /**bp:dumpFunctionProperties(0, 1);**/
})();

var arr = [0];
arr.forEach((s) => {
  var x = 1; /**bp:dumpFunctionProperties([0], '0');**/
});

function same(shouldBreak) {
  if (shouldBreak) {
    // 0 is same(true), 1 is same(false), 2 is global function). same is dumped only once as functionHandle for frame 0 and 1 is same.
    var x = 1; /**bp:stack();dumpFunctionProperties([0,1,2]);**/
  } else {
    same(!shouldBreak);
  }
}
same(false);

function one(arg1) {
  two();
}
function two(arg1, arg2) {
  three();
}
function three(arg1, arg2, arg3) {
  var x = 1; /**bp:stack();dumpFunctionProperties([0,1,2,3], 0);**/
}
one();

WScript.Echo("pass");
