//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validate that it is valid to yield nothing when unreachable
if (!WebAssembly.validate(WebAssembly.wabt.convertWast2Wasm(`
(module
  (func (export "foo") (result i32)
    block (result i32)
      loop
        br 0
      end
      unreachable
      drop
    end)
)`))) {
  print("Module should be valid");
}

print("pass");
