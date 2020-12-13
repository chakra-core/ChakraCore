;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (import "dummy" "memory" (memory 1))

        (func (export "m128_const_1") (local $v1 v128)
            (set_local $v1
            (v128.const i32x4 0x00000000 0xff00abcc 0x00000000 0x00000000)
            )
            (v128.store offset=0 align=4 (i32.const 0) (get_local $v1))
        )

        (func (export "m128_const_2") (local $v1 v128)
            (set_local $v1
            (v128.const i32x4 0xa100bc00 0xffffffff 0x0000ff00 0x00000001)
            )
            (v128.store offset=0 align=4 (i32.const 0) (get_local $v1))
        )

        (func (export "m128_const_3") (local $v1 v128)
            (set_local $v1
            (v128.const i32x4 0xffffffff 0xffffffff 0x00000000 0xffffffff)
            )
            (v128.store offset=0 align=4 (i32.const 0) (get_local $v1))
        )

        (func (export "m128_const_4") (local $v1 v128)
            (set_local $v1
            (v128.const i32x4 0x00000000 0x00000000 0x00000000 0x00000000)
            )
            (v128.store offset=0 align=4 (i32.const 0) (get_local $v1))
        )
)
