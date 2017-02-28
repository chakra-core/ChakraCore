;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module

  (func (export "while") (result i32) (local i64 i64 i64 i64 i64)
    (set_local 0 (i64.const 5))
    (set_local 1 (i64.const 1))
    (set_local 2 (i64.const 3))
    (set_local 3 (i64.const 2))
    (set_local 4 (i64.const 7))
    (block
      (loop
        (br_if 1 (i64.eqz (get_local 0)))
        (set_local 3 (i64.mul (get_local 2) (get_local 4)))
        (set_local 1 (i64.mul (get_local 0) (get_local 1)))
        (set_local 1 (i64.mul (get_local 0) (get_local 3)))
        (set_local 0 (i64.sub (get_local 0) (i64.const 1)))
        (br 0)
      )
    )
    (i32.wrap/i64 (get_local 1))
  )

)

