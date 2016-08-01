;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func $copysign32 (param f32) (param f32) (result f32)
    (return (f32.copysign (get_local 0) (get_local 1))))
  ;;(func $copysign64 (param f64) (param f64) (result f64)
  ;;  (return (f64.copysign (get_local 0) (get_local 1))))
  (func $eqz32 (param i32) (result i32)
    (return (i32.eqz (get_local 0))))
  (func $trunc32 (param f32) (result f32)
    (return (f32.trunc (get_local 0))))
  (func $nearest32 (param f32) (result f32)
    (return (f32.nearest (get_local 0))))
  (func $ifeqz (param i32) (result i32)
    (if (i32.eqz (get_local 0)) (return (i32.const 1)))
    (return (i32.const 0)))
  (export "f32copysign" $copysign32)
  ;;(export "f64copysign" $copysign64)
  (export "eqz" $eqz32)
  (export "trunc" $trunc32)
  (export "nearest" $nearest32)
  (export "ifeqz" $ifeqz)
)
