;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
    (type $binop (func (param i32 i32) (result i32)))

    (import "math" "add" (func $_add (type $binop)))
    (import "math" "sub" (func $_sub (type $binop)))

    (func $add (type $binop)
        (return (i32.add (get_local 0) (get_local 1)))
    )
    (func $sub (type $binop)
        (return (i32.sub (get_local 0) (get_local 1)))
    )

    (func (export "binop") (param i32 i32 i32) (result i32)
        (call_indirect $binop (get_local 0) (get_local 1) (get_local 2))
    )

    (table anyfunc (elem $add $_add $sub $_sub))
)
