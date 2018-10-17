;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
    (import "dummy" "memory" (memory 0 16384))
    (func (export "grow") (param $sz i32) (result i32) (memory.grow (get_local $sz)))
    (func (export "size") (result i32) (memory.size))
)
