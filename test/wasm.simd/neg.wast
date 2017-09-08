;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (import "dummy" "memory" (memory 1))

    (func (export "i32x4_neg") (param i32 i32) (local m128)
        (set_local 2 (m128.load offset=0 align=4 (get_local 0)))
        (set_local 2 (i32x4.neg (get_local 2)))
        (m128.store offset=0 align=4 (get_local 0) (get_local 2))
    )

    (func (export "i16x8_neg") (param i32 i32) (local m128)
        (set_local 2 (m128.load offset=0 align=4 (get_local 0)))
        (set_local 2 (i16x8.neg (get_local 2)))
        (m128.store offset=0 align=4 (get_local 0) (get_local 2))
    )

    (func (export "i8x16_neg") (param i32 i32) (local m128)
        (set_local 2 (m128.load offset=0 align=4 (get_local 0)))
        (set_local 2 (i8x16.neg (get_local 2)))
        (m128.store offset=0 align=4 (get_local 0) (get_local 2))
    )

    (func (export "f32x4_neg") (param i32 f32) (local m128)
        (set_local 2 (m128.load offset=0 align=4 (get_local 0)))
        (set_local 2 (f32x4.neg (get_local 2)))
        (m128.store offset=0 align=4 (get_local 0) (get_local 2))
    )

    (func (export "f64x2_neg") (param i32 f32) (local m128)
        (set_local 2 (m128.load offset=0 align=4 (get_local 0)))
        (set_local 2 (f64x2.neg (get_local 2)))
        (m128.store offset=0 align=4 (get_local 0) (get_local 2))
    )
)
