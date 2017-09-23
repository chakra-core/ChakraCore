//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const buf = WebAssembly.wabt.convertWast2Wasm(`
(module
    (global $x (mut i32) (i32.const -12))

  (func (export "stslot") (param i32)
    (loop
      (br_if 1 (i32.eqz (get_local 0)))
    (set_global $x (get_local 0))
    (set_global $x (get_local 0))
    (br 0)
    )
  )
)`);
const view = new Uint8Array(buf);
var mod = new WebAssembly.Module(buf);
var {stslot} = new WebAssembly.Instance(mod).exports;
stslot(0);
print("Pass");
