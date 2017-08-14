//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var func2 = function() {};
  var c = 2147483647;
  while (func2.call(c++, c++)) {}
}
test0();
test0();
console.log("pass");
