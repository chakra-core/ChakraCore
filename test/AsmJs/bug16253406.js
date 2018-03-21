//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function AsmModule(stdlib) {
  'use asm';
  var fr = stdlib.fround;
  function f3(x) {
    x = fr();
  }
}
console.log("pass");
