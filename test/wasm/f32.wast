;;-------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors.
;; Licensed under the MIT license. See LICENSE.txt file i
;;-------------------------------------------------------

(module
  (func (param f32) (param f32) (result f32)
    (return (f32.min (get_local 0) (get_local 1)))
  )
  (func (param f32) (param f32) (result f32)
    (return (f32.max (get_local 0) (get_local 1)))
    )
  (export "min" 0)
  (export "max" 1)
)
