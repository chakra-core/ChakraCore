//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function asm() {
  "use asm"
  function f(a, b) {
    a = a|0;
    b = b|0;
    return a|0;
  }
  return f;
}

eval = asm();
eval("some string");
print("PASSED");
