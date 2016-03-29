;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func (param f32) (param f32) (result f32)
    (return (f32.copysign (get_local 0) (get_local 1))))
  (func (param f64) (param f64) (result f64)
    (return (f64.copysign (get_local 0) (get_local 1))))
  (func (param i32) (result i32)
    (return (i32.eqz (get_local 0))))
  (func (param f32) (result f32)
    (return (f32.trunc (get_local 0))))
  (func (param f32) (result f32)
    (return (f32.nearest (get_local 0))))
  (export "f32copysign" 0)
  (export "f64copysign" 1)
  (export "eqz" 2)
  (export "trunc" 3)
  (export "nearest" 4)
)
