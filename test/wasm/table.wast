;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module $Nt
  (type (func))
  (type (func (result i32)))

  (global $a i32 (i32.const 6))
  
  (table 10 anyfunc)
    ;;(elem (i32.const 6)  $g1 $g2 $g3 $g4)
    (elem (get_global $a)  $g1 $g2 $g3 $g4)
  
  (func $g1 (result i32) (i32.const 15))
  (func $g2 (result i32) (i32.const 25))
  (func $g3 (result i32) (i32.const 35))
  (func $g4 (result i32) (i32.const 45))


  (func (export "call") (param i32) (result i32)
    (call_indirect 1 (get_local 0))
  )
)
