;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module $Nt
  (type (func))
  (type (func (result i32)))

  (global $x (import "m" "x") i32)
  (global $a i32 (i32.const 6))

  (table 15 anyfunc)
    (elem (i32.const 1)    $g1 $g2 $g3 $g4) ;;local offset
    (elem (get_global $x)  $g3 $g4 $g5 $g6) ;;imported global offset
    ;;(elem (get_global $a)  $g5 $g6 $g7 $g8) ;;global offset

  (func $g1 (result i32) (i32.const 15))
  (func $g2 (result i32) (i32.const 25))
  (func $g3 (result i32) (i32.const 35))
  (func $g4 (result i32) (i32.const 45))
  (func $g5 (result i32) (i32.const 55))
  (func $g6 (result i32) (i32.const 65))
  (func $g7 (result i32) (i32.const 75))
  (func $g8 (result i32) (i32.const 85))

  (func (export "call") (param i32) (result i32)
    (call_indirect 1 (get_local 0))
  )
)
