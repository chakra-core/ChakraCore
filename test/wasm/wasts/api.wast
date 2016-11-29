;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (type $t1 (func (result i32)))
  (type $t2 (func (result f32)))
  (import "test" "fn" (func $fn (result i32)))
  (import "test" "fn2" (func $fn2 (result f32)))
  (import "test" "memory" (memory $mem 1 3))
  (import "test" "g1" (global $g1 i32))
  (import "test" "g2" (global $g2 f32))
  (import "table" "" (table $table 10 anyfunc))
  (elem (i32.const 1) $fn $fn2)
  (data (i32.const 1) "0123456789")

  (export "fn" (func $fn))
  (export "g1" (global $g1))
  (func (export "f1") (result i32) (i32.const 5))
  (export "g2" (global $g2))
  (export "table" (table $table))
  (export "fn2" (func $fn2))
  (func (export "call_i32") (param i32) (result i32) (call_indirect $t1 (get_local 0)))
  (export "mem" (memory $mem))
  (func (export "call_f32") (param i32) (result f32) (call_indirect $t2 (get_local 0)))
  (export "g3" (global $g1))
  (func (export "load") (param i32) (result i32) (i32.load (get_local 0)))
)
