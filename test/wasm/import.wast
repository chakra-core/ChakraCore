;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (global $x (import "m" "x") i32)
  (global $z (import "m" "z") f64)
  (global $c (import "m" "c") f64)
  (global $d (import "m" "d") f64)
  (global $e (import "m" "e") i32)
 
  (global $a i32 (i32.const 13))
  (global $b f32 (f32.const 5))
  (global $f f64 (get_global $d))
  (global $y i32 (i32.const 0))

  (func (export "get-x") (result i32) (get_global $x))
  (func (export "get-z") (result f64) (get_global $z))
  (func (export "get-c") (result f64) (get_global $c))
  (func (export "get-d") (result f64) (get_global $d))
  (func (export "get-e") (result i32) (get_global $e))
  
  (func (export "get-a") (result i32) (get_global $a))
  (func (export "get-b") (result f32) (get_global $b))
  (func (export "get-f") (result f64) (get_global $f))
  (func (export "get-y") (result i32) (get_global $y))
)



