;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (import "dummy" "memory" (memory 1))

            (func (export "i32x4_trunc_s") (local $v1 m128)
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (m128.store offset=0 (i32.const 0) (i32x4.trunc_s (get_local $v1)))
            )

            (func (export "i32x4_trunc_u") (local $v1 m128)
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (m128.store offset=0 (i32.const 0) (i32x4.trunc_u (get_local $v1)))
            )

            (func (export "f32x4_convert_s") (local $v1 m128)
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (m128.store offset=0 (i32.const 0) (f32x4.convert_s (get_local $v1)))
            )

            (func (export "f32x4_convert_u") (local $v1 m128)
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (m128.store offset=0 (i32.const 0) (f32x4.convert_u (get_local $v1)))
            )
)
