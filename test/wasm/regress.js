//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const toValidate = {
  "stack-polymorphic-yield": `
(module
  (func (result i32)
    (block (result i32)
      (loop (br 0))
      unreachable
      drop
    )
  )
)`,
  "stack-polymorphic-br_table": `
(module
  (func (result i32)
    (block (result i32)
      unreachable
      drop
      (br_table 0 0)
    )
  )
)`,
  "stack-polymorphic-br": `
(module
  (func (result i32)
    unreachable
    drop
    br 0
  )
)`,
  "stack-polymorphic-return": `
(module
  (func (result i32)
    unreachable
    drop
    return
  )
)`,
};

for (const key in toValidate) {
  if (!WebAssembly.validate(WebAssembly.wabt.convertWast2Wasm(toValidate[key]))) {
    print(`Module ${key} should be valid`);
  }
}

print("pass");
