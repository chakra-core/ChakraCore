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
)

