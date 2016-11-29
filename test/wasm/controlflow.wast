;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func $c (result f32)
    (return (f32.const 1001.2)))

  (func $b (param i32) (result f32)
    (return
      (if f32 (get_local 0)
        (then (f32.const 1))
        (else (f32.const 2))
      )
    )
  )

  (func $a (export "a") (param i32) (result f32)
    (block f32 (call $b (get_local 0)) (call $c) (drop)))

  (func $e (param i32) (result i32)
    (i32.const 0))

  (func (export "yield_top") (param i32) (result i32)
    (block
      (return (i32.add
        (br_if 0 (i32.const 4) (get_local 0))
        (i32.const 3)
      ))
    )
    (i32.const 3)
  )

  (func (export "br_if") (param i32) (result i32)
    (block i32
      (block
        (br_if 1 (i32.const 4) (get_local 0))
        (drop)
      )
      (i32.const 3)
    )
  )

  (func (export "br") (result i32)
    (block
      (i32.const 4)
      (br 0)
      (drop)
    )
    (i32.const 3)
  )

  (func (export "block") (result i32)
    (i32.const 4)
    (block
      (i32.const 5)
      (br 0)
      (drop)
    )
  )
)

(assert_return (invoke "a" (i32.const 0)) (f32.const 2))
(assert_return (invoke "a" (i32.const 1)) (f32.const 1))
(assert_return (invoke "yield_top" (i32.const 0)) (i32.const 7))
(assert_return (invoke "yield_top" (i32.const 1)) (i32.const 3))
(assert_return (invoke "br_if" (i32.const 0)) (i32.const 3))
(assert_return (invoke "br_if" (i32.const 1)) (i32.const 4))
(assert_return (invoke "br") (i32.const 3))
(assert_return (invoke "block") (i32.const 4))
