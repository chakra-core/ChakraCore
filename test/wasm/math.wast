;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (func (export "ctz") (param $a i32) (result i32)
      (i32.ctz (get_local $a))
    )
    (func (export "ctzI64") (param $a i64) (result i64)
      (i64.ctz (get_local $a))
    )
)
