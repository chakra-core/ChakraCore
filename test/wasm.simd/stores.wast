;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (import "dummy" "memory" (memory 1))

    (func (export "v128_store4") (param i32 i32 i32 i32 i32) (local v128)
        (set_local 5 (v128.load offset=0 (i32.const 16)))
        (v128.store offset=0 (get_local 0) (get_local 5))
    )

    (func (export "v128_store4_offset") (param i32 i32 i32 i32 i32) (local v128)
        (set_local 5 (v128.load offset=0 (i32.const 16)))
        (v128.store offset=16 (get_local 0) (get_local 5))
    )
    (func
        (export "v128_store_i32x4")
        (param $index i32)
        (param $value i32)
        (v128.store align=4 (get_local $index) (i32x4.splat (get_local $value)))
    )
)
