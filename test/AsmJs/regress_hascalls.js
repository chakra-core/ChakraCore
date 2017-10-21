//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function asmModule(global) {
  "use asm";
  var sin = global.sin;
  function foo() {
    return +sin(0.0);
  }
  function bar() {
    return +foo();
  }
  return bar;
}
var bar = asmModule(this);
for(let i = 0; i < 1000; ++i) {
  bar();
}
console.log("pass");