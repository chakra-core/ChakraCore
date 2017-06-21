;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (import "dummy" "memory" (memory 1))

    (func (export "m128_store4") (param i32 i32 i32 i32 i32) (local m128)
        (set_local 5 (m128.load offset=0 align=4 (i32.const 16)))
        (m128.store offset=0 align=4 (get_local 0) (get_local 5))
    )

    (func (export "m128_store4_offset") (param i32 i32 i32 i32 i32) (local m128)
        (set_local 5 (m128.load offset=0 align=4 (i32.const 16)))
        (m128.store offset=16 align=4 (get_local 0) (get_local 5))
    )
)
