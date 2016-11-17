;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (global $a i32 (i32.const -2))
  (global $x (export "x") i32 (i32.const 10))
  (global $y (mut f32) (f32.const 5))
  (global $z (export "z") f64 (f64.const 777))
  (global $b (mut f64) (f64.const 1001))
  (global $c (export "c") f64 (f64.const 9.5))
  (global $d (export "d") f64 (get_global $c))
  (global $e (export "e") i32 (get_global $x))
  (global $f (export "f") i32 (i32.const 18))
  (global $i64 i64 (i64.const 0xb204004248402040))

  (func (export "get-a") (result i32) (get_global $a))
  (func (export "get-x") (result i32) (get_global $x))
  (func (export "get-y") (result f32) (get_global $y))
  (func (export "get-z") (result f64) (get_global $z))
  (func (export "get-b") (result f64) (get_global $b))
  (func (export "get-c") (result f64) (get_global $c))
  (func (export "get-d") (result f64) (get_global $d))
  (func (export "get-e") (result i32) (get_global $e))
  (func (export "get-f") (result i32) (get_global $f))
  (func (export "get-i64") (result i64) (get_global $i64))

  (func (export "set-y") (param f32)  (set_global 2 (get_local 0)))
  (func (export "set-b") (param f64) (set_global 4 (get_local 0)))
)



