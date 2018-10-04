//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function AsmModule(stdlib, foreign) {
  'use asm';
  var Bar = foreign.Bar;
  function foo() {
    return new Bar();
  }
  return foo;
}
AsmModule(this, { Bar: function () { console.log(`new.target: ${new.target}`); } })();
