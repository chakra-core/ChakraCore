//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let asmModule = (function AsmModule() {
  'use asm';
  function conditionnalReturn() {
    if (0) {
      return 0;
    }
  }
  return conditionnalReturn;
})();
print(asmModule());
