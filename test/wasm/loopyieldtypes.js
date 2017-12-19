//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
const mod = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(`(module
  (func (export "foo") (param $d i32) (result i32)
    (block $b1 (result i32)
      (block $b2 (result i64)
        (block $b3 (result f32)
          (block $b4 (result f64)
            (loop
              (i32.const 1) (br_if $b1 (i32.eq (get_local $d) (i32.const 0))) (drop)
              (i64.const 2) (br_if $b2 (i32.eq (get_local $d) (i32.const 1))) (drop)
              (f32.const 3) (br_if $b3 (i32.eq (get_local $d) (i32.const 2))) (drop)
              (f64.const 4) (br_if $b4 (i32.eq (get_local $d) (i32.const 3))) (drop)
            )
            (f64.const 5)
          )
          (f32.demote/f64)
        )
        (i64.trunc_u/f32)
      )
      (i32.wrap/i64)
    )
  )
)`));

const {exports: {foo}} = new WebAssembly.Instance(mod);

const expected = [1, 2, 3, 4, 5];
for (let i = 0; i < expected.length; ++i) {
  const res = foo(i);
  if (res !== expected[i]) {
    console.log(`Failed foo(${i}). Expected ${expected[i].toString(16)}, got 0x${res.toString(16)}`);
  }
}
console.log("pass");
