//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const buf = WebAssembly.wabt.convertWast2Wasm(`
(module
  (func (export "min") (param f64 f64) (result f64)
    (f64.min (get_local 0) (get_local 1))
  )

  (func (export "max") (param f64 f64) (result f64)
    (f64.max (get_local 0) (get_local 1))
  )
)`);
const view = new Uint8Array(buf);
view[buf.byteLength - 1] = 6;
var mod = new WebAssembly.Module(buf);
var {min, max} = new WebAssembly.Instance(mod).exports;

function foo(fn) {
  fn();
}

try {foo(min);} catch (e) {}
try {foo(max);} catch (e) {}
try {foo(min);} catch (e) {}
print("Pass");