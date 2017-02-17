;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (memory (export "mem") 0)
  (func (export "load") (param $i i32)
    (drop (i32.load offset=4294967295 (get_local $i)))
  )
  (func (export "store") (param $i i32)
    (i32.store offset=4294967295 (get_local $i) (i32.const 0))
  )
)
