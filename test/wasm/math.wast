;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (func $ctz (param $a i32) (result i32)
      (return (i32.ctz (get_local $a)))
    )
 (export "ctz" $ctz)
)