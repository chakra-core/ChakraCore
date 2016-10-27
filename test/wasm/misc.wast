;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func (export "f32copysign") (param f32) (param f32) (result f32)
    (return (f32.copysign (get_local 0) (get_local 1))))

  (func (export "f64copysign") (param f64) (param f64) (result f64)
    (return (f64.copysign (get_local 0) (get_local 1))))

  (func (export "eqz") (param i32) (result i32)
    (return (i32.eqz (get_local 0))))

  (func (export "trunc") (param f32) (result f32)
    (return (f32.trunc (get_local 0))))

  (func (export "f64trunc") (param f64) (result f64)
    (return (f64.trunc (get_local 0))))

  (func (export "nearest") (param f32) (result f32)
    (return (f32.nearest (get_local 0))))

  (func (export "f64nearest") (param f64) (result f64)
    (return (f64.nearest (get_local 0))))

  (func (export "ifeqz") (param i32) (result i32)
    (if (i32.eqz (get_local 0)) (return (i32.const 1)))
    (return (i32.const 0)))
)
