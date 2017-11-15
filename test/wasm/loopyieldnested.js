//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// -lic:10 -bgjit-
const mod = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(`
(module
  (func (export "foo") (param $i i32) (result i32)
    (local $l i32)
    (block $b1 (result i32)
      (loop (result i32)
        (i32.const -1) (br_if 2  (i32.eq (get_local $i) (i32.const 0))) (drop)
        (i32.const 0) (br_if $b1 (i32.eq (get_local $i) (i32.const 1))) (drop)

        (block $b2 (result i32)
          (set_local $l (i32.const 0))
          (loop
            (i32.const -2) (br_if 4  (i32.eq (get_local $i) (i32.const 2))) (drop)
            (i32.const 1) (br_if $b1 (i32.eq (get_local $i) (i32.const 3))) (drop)
            (i32.const 2) (br_if $b2 (i32.eq (get_local $i) (i32.const 4))) (drop)

            ;; loop 100 times to trigger jit loop body on inner loops first
            (i32.add (get_local $l) (i32.const 1))
            (tee_local $l)
            (br_if 0 (i32.lt_u (i32.const 100)))
          )
          (block $b3 (result i32)
            (set_local $l (i32.const 0))
            (loop
              (i32.const -3) (br_if 5  (i32.eq (get_local $i) (i32.const 5))) (drop)
              (i32.const 3) (br_if $b1 (i32.eq (get_local $i) (i32.const 6))) (drop)
              (i32.const 4) (br_if $b2 (i32.eq (get_local $i) (i32.const 7))) (drop)
              (i32.const 5) (br_if $b3 (i32.eq (get_local $i) (i32.const 8))) (drop)

              ;; loop 100 times to trigger jit loop body on inner loops first
              (i32.add (get_local $l) (i32.const 1))
              (tee_local $l)
              (br_if 0 (i32.lt_u (i32.const 100)))
            )
            (i32.const 6)
          )
        )
      )
    )
    (i32.add (i32.const 1))
  )
)`));
const {exports: {foo}} = new WebAssembly.Instance(mod);
const expected = [-1, 1, -2, 2, 3, -3, 4, 5, 6, 7];
// Do this a few times to try to cover all possible jited loop
for (let l = 0; l < 100; ++l) {
  for (let i = 9; i >= 0; --i) {
    const res = foo(i);
    if (res !== expected[i]) {
      console.log(`Failed foo(${i}). Expected ${expected[i]}, got ${res}`);
    }
  }
}
console.log("pass");
