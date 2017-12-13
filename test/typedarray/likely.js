//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var buf = new ArrayBuffer(0x100000);
var arrays = [new Int32Array(buf)];
function foo() {
  var arr = arrays[0];
  for (let i = 0; i < 1000; ++i) {
    Number(arr[0]) || !!arr[0];
  }
}
foo();
print("pass");
