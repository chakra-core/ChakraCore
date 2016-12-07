;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
    (import "test" "foo" (func $foo (param i32)))
    (func (export "a") (param i32)
        (call $foo (get_local 0))
    )
    (func $b (export "b") (param i32)
        (call $foo (get_local 0))
    )

    (func $c (export "c") (param i32)
        (call $d (get_local 0))
    )
    (func $d (param i32)
        (call $b (get_local 0))
    )
)
