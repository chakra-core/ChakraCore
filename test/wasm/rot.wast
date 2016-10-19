;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func (param i32) (param i32) (result i32)
    (return (i32.rotl (get_local 0) (get_local 1)))
  )
  (func (param i32) (param i32) (result i32)
    (return (i32.rotr (get_local 0) (get_local 1)))
  )
  (export "rotl" 0)
  (export "rotr" 1)
)
