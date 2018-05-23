//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function makeArrayLength() {
  return 1;
}
var obj0 = {};
var arrObj0 = {};
var func3 = function () {
  arrObj0[1] = 1;
  return obj0.length;
};

obj0.method0 = func3;
var h = 1;
obj0.length = makeArrayLength();
var __loopvar1 = 1;
do {
  h = obj0.method0(h);
} while (obj0.method0() && __loopvar1++ < 10);
print("passed");