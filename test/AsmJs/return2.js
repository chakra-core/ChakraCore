//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let asmModule = (function AsmModule(stdlib) {
  'use asm';
  function empty() {
  }
  function changeType() {
    return empty()|0;
  }
  return { empty:empty };
})({});
print(asmModule.empty());
