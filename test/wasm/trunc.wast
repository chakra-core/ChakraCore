;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func (export "i32_trunc_s_f32") (param $x f32) (result i32) (i32.trunc_s/f32 (get_local $x)))
  (func (export "i32_trunc_u_f32") (param $x f32) (result i32) (i32.trunc_u/f32 (get_local $x)))
  (func (export "i32_trunc_s_f64") (param $x f64) (result i32) (i32.trunc_s/f64 (get_local $x)))
  (func (export "i32_trunc_u_f64") (param $x f64) (result i32) (i32.trunc_u/f64 (get_local $x)))
  (func (export "i64_trunc_s_f32") (param $x f32) (result i64) (i64.trunc_s/f32 (get_local $x)))
  (func (export "i64_trunc_u_f32") (param $x f32) (result i64) (i64.trunc_u/f32 (get_local $x)))
  (func (export "i64_trunc_s_f64") (param $x f64) (result i64) (i64.trunc_s/f64 (get_local $x)))
  (func (export "i64_trunc_u_f64") (param $x f64) (result i64) (i64.trunc_u/f64 (get_local $x)))
  ;;
  (func(export "test1") (result i32) (i64.eq (i64.trunc_u/f64 (f64.const 18446744073709549568.0))  (i64.const 0xfffffffffffff800)))
  (func(export "test2") (result i32) (i64.eq (i64.trunc_u/f64 (f64.const 18446744073709550592.0))  (i64.const 0))) ;; overflow
  ;;
  (func(export "test3") (result i32) (i64.eq (i64.trunc_u/f64 (f64.const 0.0))  (i64.const 0)))
  (func(export "test4") (result i32) (i64.eq (i64.trunc_u/f64 (f64.const 0.5))  (i64.const 0)))
  (func(export "test5") (result i32) (i64.eq (i64.trunc_u/f64 (f64.const -1))  (i64.const 0)))
  ;;
  (func(export "test6") (result i32) (i64.eq (i64.trunc_s/f64 (f64.const 9223372036854774784))  (i64.const 9223372036854774784)))
  (func(export "test7") (result i32) (i64.eq (i64.trunc_s/f64 (f64.const 9223372036854775296))  (i64.const 0))) ;; overflow
  ;;
  (func(export "test8") (result i32) (i64.eq (i64.trunc_s/f64 (f64.const -9223372036854775808.0))  (i64.const -9223372036854775808)))
  (func(export "test9") (result i32) (i64.eq (i64.trunc_s/f64 (f64.const -36893488147419103232.0))  (i64.const 0))) ;; overflow
  ;;
  (func(export "test10") (result i32) (i64.eq (i64.trunc_u/f32 (f32.const 4294967040.0))  (i64.const 4294967040)))
  (func(export "test11") (result i32) (i64.eq (i64.trunc_u/f32 (f32.const 18446744073709550592.0))  (i64.const 0))) ;; overflow
  ;;
  (func(export "test12") (result i32) (i64.eq (i64.trunc_u/f32 (f32.const 0.0))  (i64.const 0)))
  (func(export "test13") (result i32) (i64.eq (i64.trunc_u/f32 (f32.const 0.5))  (i64.const 0)))
  (func(export "test14") (result i32) (i64.eq (i64.trunc_u/f32 (f32.const -1))  (i64.const 0))) ;; overflow
  ;;
  (func(export "test15") (result i32) (i64.eq (i64.trunc_s/f32 (f32.const 2147483520.0))  (i64.const 2147483520)))
  (func(export "test16") (result i32) (i64.eq (i64.trunc_s/f32 (f32.const 9223372036854775296.0))  (i64.const 2147483584))) ;; overflow
  ;;
  (func(export "test17") (result i32) (i64.eq (i64.trunc_s/f32 (f32.const 2147483520.0))  (i64.const 2147483520)))
  (func(export "test18") (result i32) (i64.eq (i64.trunc_s/f32 (f32.const 18446744073709551616.0))  (i64.const 0))) ;; overflow
)

