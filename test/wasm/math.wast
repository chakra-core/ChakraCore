;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (func (export "ctz") (param $a i32) (result i32)
      (i32.ctz (get_local $a))
    )
    (func (export "i32_div_s") (param $x i32) (param $y i32) (result i32)
        (i32.div_s (get_local $x) (get_local $y))
    )
    (func (export "i32_div_s_by_two") (param $x i32) (result i32)
        (i32.div_s (get_local $x) (i32.const 2))
    )
    (func (export "i32_div_s_over") (result i32)
        (i32.div_s (i32.const 0x80000000  ) (i32.const 0xffffffff))
    )
    (func (export "i32_div_s_zero") (param $x i32) (result i32)
        (i32.div_s (get_local $x) (i32.const 0))
    )
    (func (export "i32_div_u") (param $x i32) (param $y i32) (result i32)
        (i32.div_u (get_local $x) (get_local $y))
    )
    (func (export "i32_rem_s") (param $x i32) (param $y i32) (result i32)
        (i32.rem_s (get_local $x) (get_local $y))
    )
    (func (export "i32_rem_u") (param $x i32) (param $y i32) (result i32)
        (i32.rem_u (get_local $x) (get_local $y))
    )
    (func (export "i64_div_s_over") (result i64)
        (i64.div_s (i64.const 0x8000000000000000  ) (i64.const 0xffffffffffffffff)) ;; overflow
    )

    (func(export "test1") (result i32) (i64.eq (i64.div_s (i64.const 5) (i64.const 2))  (i64.const 2)))
    (func(export "test2") (result i32) (i64.eq (i64.div_s (i64.const 5) (i64.const 0))  (i64.const 0))) ;; div by zero
    (func(export "test3") (result i32) (i64.eq (i64.div_u (i64.const 5) (i64.const 0))  (i64.const 0))) ;; div by zero
    (func(export "test4") (result i32) (i64.eq (i64.rem_u (i64.const 5) (i64.const 0))  (i64.const 0))) ;; div by zero
    (func(export "test5") (result i32) (i64.eq (i64.rem_s (i64.const 5) (i64.const 0))  (i64.const 0))) ;; div by zero
    (func(export "test6") (result i32) (i64.eq (i64.div_s (i64.const 5) (i64.const -2))  (i64.const -2)))
    (func(export "test7") (result i32) (i64.eq (i64.div_u (i64.const 5) (i64.const 2))  (i64.const 2)))
    (func(export "test8") (result i32) (i64.eq (i64.rem_u (i64.const 5) (i64.const 1))  (i64.const 0)))
    (func(export "test9") (result i32) (i64.eq (i64.rem_s (i64.const 5) (i64.const -1))  (i64.const 0)))
    (func(export "test10") (result i32) (i64.eq (i64.rem_s (i64.const 5) (i64.const 2))  (i64.const 1)))
)
