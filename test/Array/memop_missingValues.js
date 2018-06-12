//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0()
{
  var GiantPrintArray = [];
  var IntArr0 = new Array(1, 1);
  var FloatArr0 = [];
  FloatArr0[1] = 1;
  FloatArr0[0] = 1;
  FloatArr0[12] = 1;
  var v5;
  v5 = IntArr0.length;
  for (var i = 0; i < v5; i++) {
    FloatArr0[i] = IntArr0[i];
  }
  GiantPrintArray.push(FloatArr0);
  for (var v2 = 0; 0 < GiantPrintArray; 0) {
  }
}
test0();
test0();
test0();
print("passed");