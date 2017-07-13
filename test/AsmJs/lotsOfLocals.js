//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const txt = `
  return function() {
    "use asm";
    function foo() {
      ${Array(50000).fill().map((_, i) => `var l${i} = 0.0;`).join("\n")}
    }
    return foo;
  }
`;
const asmModule = (new Function(txt))();
const asmFn = asmModule();
asmFn();
print("PASSED");
