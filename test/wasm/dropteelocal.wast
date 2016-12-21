;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module

  (func $tee (export "tee") (param i32) (result i32)
    (local i32 i32)
    (set_local 1 (get_local 0))
    (set_local 2 (i32.const 1))
    (block
      (set_local 2 (i32.mul (get_local 1) (get_local 2)))
      (drop (get_local 2))
      (if (i32.eqz (tee_local 1 (i32.sub (get_local 1) (i32.const 1))))
          (set_local 2 (i32.const 100))
          (set_local 2 (i32.const 200)))
    )
    (get_local 2)
  )
)
