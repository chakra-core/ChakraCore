//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
const mod = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(`(module
  (func (export "foo") (param $d i32) (result i32)
    (local $tmp i32)
    (set_local $tmp (i32.mul (get_local $d) (i32.const 100)))
    (block $b1 (result i32)
      (block $b2 (result i32)
        (block $b3 (result i32)
          (block $b4 (result i32)
            (block $b5 (result i32)
              (loop (result i32)
                (tee_local $tmp (i32.sub (get_local $tmp) (get_local $d)))
                (br_if 0 (i32.gt_u (get_local $d)))

                (block $bloop (result i32)
                  (block $breturn (result i32)
                    (get_local $tmp) ;; Yield value
                    (get_local $tmp) ;; br table index
                    (br_table
                      $bloop $b5 $b4 $b3 $b2 $b1 8 ;; 8 refers to the func depth
                      $breturn ;; default
                    )
                  )
                  (return)
                )
              )
              (i32.add (i32.const 0x10))
            )
            (i32.add (i32.const 0x100))
          )
          (i32.add (i32.const 0x1000))
        )
        (i32.add (i32.const 0x10000))
      )
      (i32.add (i32.const 0x100000))
    )
    (i32.add (i32.const 0x1000000))
  )
)`));

const {exports: {foo}} = new WebAssembly.Instance(mod);

const expected = [
  0x1111110,
  0x1111101,
  0x1111002,
  0x1110003,
  0x1100004,
  0x1000005,
  0x6,
  0x7,
  0x8,
];
for (const i in expected) {
  const res = foo(i);
  if (res !== expected[i]) {
    console.log(`Failed foo(${i}). Expected 0x${expected[i].toString(16)}, got 0x${res.toString(16)}`);
  }
}
console.log("pass");
