;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
    (type $binopI32 (func (param i32 i32) (result i32)))
    (type $binopI64 (func (param i64 i64) (result i64)))
    (type $binopF32 (func (param f32 f32) (result f32)))
    (type $binopF64 (func (param f64 f64) (result f64)))

    (import "math" "addI32" (func $_addI32 (type $binopI32)))
    (import "math" "addI64" (func $_addI64 (type $binopI64)))
    (import "math" "addF32" (func $_addF32 (type $binopF32)))
    (import "math" "addF64" (func $_addF64 (type $binopF64)))

    (func $addI32 (export "addI32") (type $binopI32)
        (return (i32.add (get_local 0) (get_local 1)))
    )
    (func $addI64 (export "addI64") (type $binopI64)
        (return (i64.add (get_local 0) (get_local 1)))
    )
    (func $addF32 (export "addF32") (type $binopF32)
        (return (f32.add (get_local 0) (get_local 1)))
    )
    (func $addF64 (export "addF64") (type $binopF64)
        (return (f64.add (get_local 0) (get_local 1)))
    )

    (func (export "binopI32") (param i32 i32 i32) (result i32)
        (call_indirect $binopI32 (get_local 0) (get_local 1) (get_local 2))
    )
    (func (export "binopI64") (param i64 i64 i32) (result i64)
        (call_indirect $binopI64 (get_local 0) (get_local 1) (get_local 2))
    )
    (func (export "binopF32") (param f32 f32 i32) (result f32)
        (call_indirect $binopF32 (get_local 0) (get_local 1) (get_local 2))
    )
    (func (export "binopF64") (param f64 f64 i32) (result f64)
        (call_indirect $binopF64 (get_local 0) (get_local 1) (get_local 2))
    )

    (table anyfunc (elem
        $addI32 $_addI32
        $addI64 $_addI64
        $addF32 $_addF32
        $addF64 $_addF64
    ))
)
