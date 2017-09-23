//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var counter = 2;

  function test0Inner() {
    return counter--;
  };

  var outterStr = 't';
  while (test0Inner()) {
    var str2 = outterStr.replace('test', ' test ');
    var str3 = 'test1' + 'test2' + outterStr;
  }
}
test0();

print("pass");
