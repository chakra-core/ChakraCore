//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var arr0 = Array(1, 2);
  arr0[-7] = 3;
  return (-7 in arr0);
}


var res = test0();
res &= test0();
res &= test0();

function test1() {
  var arr0 = Array();
  arr0[11] = -2;
  return (11 in arr0);
}

res &= test1();
res &= test1();
res &= test1();

if (res) {
  print("Passed");
}
