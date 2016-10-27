;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func (export "min") (param f32) (param f32) (result f32)
    (return (f32.min (get_local 0) (get_local 1)))
  )

  (func (export "max") (param f32) (param f32) (result f32)
    (return (f32.max (get_local 0) (get_local 1)))
    )
)
