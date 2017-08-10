//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function asmModule() {
  "use asm";

  let a = [1];
  for (let i = 0; i < 2; i++) { // JIT
    a[0] = 1;
    if (i > 0) {
      a[0] = {}; // the array type changed, bailout!!
    }
  }

  function f(v) {
    v = v | 0;
    return v | 0;
  }
  return f;
}

asmModule(1);
print("PASSED");
