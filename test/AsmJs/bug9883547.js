//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function AsmModule() {
  "use asm";
  function f() {
    var a = 0, b = 0.0;
    while((b) == 0.0) {
      b = +(a>>>0); // First use of a in the loop as a uint32
      a = 5|0; // def of a in the loop as an int32
    }
    return +b;
  }
  return f;
}

var f = AsmModule();
console.log("PASSED");
