//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//Switches: -bgjit- -lic:1
var a = Array(10).fill();
function foo() {
  for (var k = 0; k < a.length; ++k) {
    if (k == 0) {
      k += 1;
    }
    a[k];
  }
}
foo();
print("passed");
