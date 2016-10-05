;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (func $c (result f32)
      (return (f32.const 1001.2)))

    (func $b (export "a") (param i32) (result f32)
      (return
        (if_else f32 (i32.const 0) (f32.const 1)
          (f32.const 2))))

    (func $a (param i32) (result f32)
      (block f32 (call $b (i32.const 1)) (call $c) (drop)))

    (func $e (param i32) (result i32)
      (i32.const 0))
)
