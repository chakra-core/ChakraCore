//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(i0) {
  "use asm";
  function bar() {
    i0 = 0;
  }
  return bar;
}
print("pass");
