;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func (export "params") (result i32)
    (i32.const 1)
    (i32.const 2)
    (block (param i32 i32) (result i32)
      (i32.add)
    )
  )
)
(assert_return (invoke "params") (i32.const 3))

(module
  (func (export "params-id") (result i32)
    (i32.const 1)
    (i32.const 2)
    (block (param i32 i32) (result i32 i32))
    (i32.add)
  )
)
(assert_return (invoke "params-id") (i32.const 3))

(module
  (func (export "results") (result i32)
    (block (result i32 i32)
      (i32.const 1)
      (i32.const 2)
    )
    (i32.add)
  )
)
(assert_return (invoke "results") (i32.const 3))

(module
  (func (export "multi-type") (result i32)
    (i32.const 1)
    (f64.const 2)
    (block (param i32 f64) (result i32 i32)
      (i32.trunc_s/f64)
    )
    (i32.add)
  )
)
(assert_return (invoke "multi-type") (i32.const 3))