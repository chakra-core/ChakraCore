//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var {exports: {binop}} = Wasm.instantiateModule(readbuffer('table.wasm'), {
  math: {
    add(a, b) {
      print("custom add (+5)")
      return a + b + 5;
    },
    sub(a, b) {
      print("custom sub (-5)")
      return a - b - 5;
    }
  }
});

for(let i = 0; i < 4; ++i) {
  print(binop(1, 2, i));
}
