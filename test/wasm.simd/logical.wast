;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (import "dummy" "memory" (memory 1))

            (func (export "m128_and") (local $v1 m128) (local $v2 m128)       
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (set_local $v2 (m128.load offset=0 align=4 (i32.const 16)))
                (m128.store offset=0 (i32.const 0) (m128.and (get_local $v1) (get_local $v2))) 
            )

            (func (export "m128_or") (local $v1 m128) (local $v2 m128)       
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (set_local $v2 (m128.load offset=0 align=4 (i32.const 16)))
                (m128.store offset=0 (i32.const 0) (m128.or (get_local $v1) (get_local $v2))) 
            )

            (func (export "m128_xor") (local $v1 m128) (local $v2 m128)       
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (set_local $v2 (m128.load offset=0 align=4 (i32.const 16)))
                (m128.store offset=0 (i32.const 0) (m128.xor (get_local $v1) (get_local $v2))) 
            )

            (func (export "m128_not") (local $v1 m128) 
                (set_local $v1 (m128.load offset=0 align=4 (i32.const 0)))
                (m128.store offset=0 (i32.const 0) (m128.not (get_local $v1))) 
            )

            (func (export "i32x4_anytrue") (result i32)
                (i32x4.any_true (m128.load offset=0 align=4 (i32.const 0)))
            )
            
            (func (export "i32x4_alltrue") (result i32)
                (i32x4.all_true (m128.load offset=0 align=4 (i32.const 0)))
            )
            
            (func (export "i16x8_anytrue") (result i32)
                (i16x8.any_true (m128.load offset=0 align=4 (i32.const 0)))
            )
            
            (func (export "i16x8_alltrue") (result i32)
                (i16x8.all_true (m128.load offset=0 align=4 (i32.const 0)))
            )
            
            (func (export "i8x16_anytrue") (result i32)
                (i8x16.any_true (m128.load offset=0 align=4 (i32.const 0)))
            )
            
            (func (export "i8x16_alltrue") (result i32)
                (i8x16.all_true (m128.load offset=0 align=4 (i32.const 0)))
            )
            
)
