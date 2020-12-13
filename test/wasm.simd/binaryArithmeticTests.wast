;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (func
        (export  "func_i32x4_add_3")
        (param  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i32x4.replace_lane 0
        get_local 1
        i32x4.replace_lane 1
        get_local 2
        i32x4.replace_lane 2
        get_local 3
        i32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        i32x4.replace_lane 0
        get_local 5
        i32x4.replace_lane 1
        get_local 6
        i32x4.replace_lane 2
        get_local 7
        i32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (i32x4.add
                (get_local  8)
                (get_local  9)
            )
        )
        (i32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_i32x4_sub_3")
        (param  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i32x4.replace_lane 0
        get_local 1
        i32x4.replace_lane 1
        get_local 2
        i32x4.replace_lane 2
        get_local 3
        i32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        i32x4.replace_lane 0
        get_local 5
        i32x4.replace_lane 1
        get_local 6
        i32x4.replace_lane 2
        get_local 7
        i32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (i32x4.sub
                (get_local  8)
                (get_local  9)
            )
        )
        (i32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_i32x4_mul_3")
        (param  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i32x4.replace_lane 0
        get_local 1
        i32x4.replace_lane 1
        get_local 2
        i32x4.replace_lane 2
        get_local 3
        i32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        i32x4.replace_lane 0
        get_local 5
        i32x4.replace_lane 1
        get_local 6
        i32x4.replace_lane 2
        get_local 7
        i32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (i32x4.mul
                (get_local  8)
                (get_local  9)
            )
        )
        (i32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_i32x4_shl_3")
        (param  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i32x4.replace_lane 0
        get_local 1
        i32x4.replace_lane 1
        get_local 2
        i32x4.replace_lane 2
        get_local 3
        i32x4.replace_lane 3
        set_local 5

        (set_local
            5
            (i32x4.shl
                (get_local  5)
                (get_local  4)
            )
        )
        (i32x4.extract_lane 3 (get_local  5))
    )
    (func
        (export  "func_i32x4_shr_3_s")
        (param  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i32x4.replace_lane 0
        get_local 1
        i32x4.replace_lane 1
        get_local 2
        i32x4.replace_lane 2
        get_local 3
        i32x4.replace_lane 3
        set_local 5

        (set_local
            5
            (i32x4.shr_s
                (get_local  5)
                (get_local  4)
            )
        )
        (i32x4.extract_lane 3 (get_local  5))
    )
    (func
        (export  "func_i32x4_shr_3_u")
        (param  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i32x4.replace_lane 0
        get_local 1
        i32x4.replace_lane 1
        get_local 2
        i32x4.replace_lane 2
        get_local 3
        i32x4.replace_lane 3
        set_local 5

        (set_local
            5
            (i32x4.shr_u
                (get_local  5)
                (get_local  4)
            )
        )
        (i32x4.extract_lane 3 (get_local  5))
    )
    (func
        (export  "func_i16x8_add_3_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 16

        v128.const i32x4 0 0 0 0
        get_local 8
        i16x8.replace_lane 0
        get_local 9
        i16x8.replace_lane 1
        get_local 10
        i16x8.replace_lane 2
        get_local 11
        i16x8.replace_lane 3
        get_local 12
        i16x8.replace_lane 4
        get_local 13
        i16x8.replace_lane 5
        get_local 14
        i16x8.replace_lane 6
        get_local 15
        i16x8.replace_lane 7
        set_local 17

        (set_local
            18
            (i16x8.add
                (get_local  16)
                (get_local  17)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  18))
    )
    (func
        (export  "func_i16x8_addsaturate_3_s_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 16

        v128.const i32x4 0 0 0 0
        get_local 8
        i16x8.replace_lane 0
        get_local 9
        i16x8.replace_lane 1
        get_local 10
        i16x8.replace_lane 2
        get_local 11
        i16x8.replace_lane 3
        get_local 12
        i16x8.replace_lane 4
        get_local 13
        i16x8.replace_lane 5
        get_local 14
        i16x8.replace_lane 6
        get_local 15
        i16x8.replace_lane 7
        set_local 17

        (set_local
            18
            (i16x8.add_sat_s
                (get_local  16)
                (get_local  17)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  18))
    )
    (func
        (export  "func_i16x8_addsaturate_3_u_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 16

        v128.const i32x4 0 0 0 0
        get_local 8
        i16x8.replace_lane 0
        get_local 9
        i16x8.replace_lane 1
        get_local 10
        i16x8.replace_lane 2
        get_local 11
        i16x8.replace_lane 3
        get_local 12
        i16x8.replace_lane 4
        get_local 13
        i16x8.replace_lane 5
        get_local 14
        i16x8.replace_lane 6
        get_local 15
        i16x8.replace_lane 7
        set_local 17

        (set_local
            18
            (i16x8.add_sat_u
                (get_local  16)
                (get_local  17)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  18))
    )
    (func
        (export  "func_i16x8_sub_3_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 16

        v128.const i32x4 0 0 0 0
        get_local 8
        i16x8.replace_lane 0
        get_local 9
        i16x8.replace_lane 1
        get_local 10
        i16x8.replace_lane 2
        get_local 11
        i16x8.replace_lane 3
        get_local 12
        i16x8.replace_lane 4
        get_local 13
        i16x8.replace_lane 5
        get_local 14
        i16x8.replace_lane 6
        get_local 15
        i16x8.replace_lane 7
        set_local 17

        (set_local
            18
            (i16x8.sub
                (get_local  16)
                (get_local  17)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  18))
    )
    (func
        (export  "func_i16x8_subsaturate_3_s_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 16

        v128.const i32x4 0 0 0 0
        get_local 8
        i16x8.replace_lane 0
        get_local 9
        i16x8.replace_lane 1
        get_local 10
        i16x8.replace_lane 2
        get_local 11
        i16x8.replace_lane 3
        get_local 12
        i16x8.replace_lane 4
        get_local 13
        i16x8.replace_lane 5
        get_local 14
        i16x8.replace_lane 6
        get_local 15
        i16x8.replace_lane 7
        set_local 17

        (set_local
            18
            (i16x8.sub_sat_s
                (get_local  16)
                (get_local  17)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  18))
    )
    (func
        (export  "func_i16x8_subsaturate_3_u_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 16

        v128.const i32x4 0 0 0 0
        get_local 8
        i16x8.replace_lane 0
        get_local 9
        i16x8.replace_lane 1
        get_local 10
        i16x8.replace_lane 2
        get_local 11
        i16x8.replace_lane 3
        get_local 12
        i16x8.replace_lane 4
        get_local 13
        i16x8.replace_lane 5
        get_local 14
        i16x8.replace_lane 6
        get_local 15
        i16x8.replace_lane 7
        set_local 17

        (set_local
            18
            (i16x8.sub_sat_u
                (get_local  16)
                (get_local  17)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  18))
    )
    (func
        (export  "func_i16x8_mul_3_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 16

        v128.const i32x4 0 0 0 0
        get_local 8
        i16x8.replace_lane 0
        get_local 9
        i16x8.replace_lane 1
        get_local 10
        i16x8.replace_lane 2
        get_local 11
        i16x8.replace_lane 3
        get_local 12
        i16x8.replace_lane 4
        get_local 13
        i16x8.replace_lane 5
        get_local 14
        i16x8.replace_lane 6
        get_local 15
        i16x8.replace_lane 7
        set_local 17

        (set_local
            18
            (i16x8.mul
                (get_local  16)
                (get_local  17)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  18))
    )
    (func
        (export  "func_i16x8_shl_3_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 9

        (set_local
            9
            (i16x8.shl
                (get_local  9)
                (get_local  8)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  9))
    )
    (func
        (export  "func_i16x8_shr_3_s_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 9

        (set_local
            9
            (i16x8.shr_s
                (get_local  9)
                (get_local  8)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  9))
    )
    (func
        (export  "func_i16x8_shr_3_u_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i16x8.replace_lane 0
        get_local 1
        i16x8.replace_lane 1
        get_local 2
        i16x8.replace_lane 2
        get_local 3
        i16x8.replace_lane 3
        get_local 4
        i16x8.replace_lane 4
        get_local 5
        i16x8.replace_lane 5
        get_local 6
        i16x8.replace_lane 6
        get_local 7
        i16x8.replace_lane 7
        set_local 9

        (set_local
            9
            (i16x8.shr_u
                (get_local  9)
                (get_local  8)
            )
        )
        (i16x8.extract_lane_u 3 (get_local  9))
    )
    (func
        (export  "func_i8x16_add_3_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 32

        v128.const i32x4 0 0 0 0
        get_local 16
        i8x16.replace_lane 0
        get_local 17
        i8x16.replace_lane 1
        get_local 18
        i8x16.replace_lane 2
        get_local 19
        i8x16.replace_lane 3
        get_local 20
        i8x16.replace_lane 4
        get_local 21
        i8x16.replace_lane 5
        get_local 22
        i8x16.replace_lane 6
        get_local 23
        i8x16.replace_lane 7
        get_local 24
        i8x16.replace_lane 8
        get_local 25
        i8x16.replace_lane 9
        get_local 26
        i8x16.replace_lane 10
        get_local 27
        i8x16.replace_lane 11
        get_local 28
        i8x16.replace_lane 12
        get_local 29
        i8x16.replace_lane 13
        get_local 30
        i8x16.replace_lane 14
        get_local 31
        i8x16.replace_lane 15
        set_local 33

        (set_local
            34
            (i8x16.add
                (get_local  32)
                (get_local  33)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  34))
    )
    (func
        (export  "func_i8x16_addsaturate_3_s_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 32

        v128.const i32x4 0 0 0 0
        get_local 16
        i8x16.replace_lane 0
        get_local 17
        i8x16.replace_lane 1
        get_local 18
        i8x16.replace_lane 2
        get_local 19
        i8x16.replace_lane 3
        get_local 20
        i8x16.replace_lane 4
        get_local 21
        i8x16.replace_lane 5
        get_local 22
        i8x16.replace_lane 6
        get_local 23
        i8x16.replace_lane 7
        get_local 24
        i8x16.replace_lane 8
        get_local 25
        i8x16.replace_lane 9
        get_local 26
        i8x16.replace_lane 10
        get_local 27
        i8x16.replace_lane 11
        get_local 28
        i8x16.replace_lane 12
        get_local 29
        i8x16.replace_lane 13
        get_local 30
        i8x16.replace_lane 14
        get_local 31
        i8x16.replace_lane 15
        set_local 33

        (set_local
            34
            (i8x16.add_sat_s
                (get_local  32)
                (get_local  33)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  34))
    )
    (func
        (export  "func_i8x16_addsaturate_3_u_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 32

        v128.const i32x4 0 0 0 0
        get_local 16
        i8x16.replace_lane 0
        get_local 17
        i8x16.replace_lane 1
        get_local 18
        i8x16.replace_lane 2
        get_local 19
        i8x16.replace_lane 3
        get_local 20
        i8x16.replace_lane 4
        get_local 21
        i8x16.replace_lane 5
        get_local 22
        i8x16.replace_lane 6
        get_local 23
        i8x16.replace_lane 7
        get_local 24
        i8x16.replace_lane 8
        get_local 25
        i8x16.replace_lane 9
        get_local 26
        i8x16.replace_lane 10
        get_local 27
        i8x16.replace_lane 11
        get_local 28
        i8x16.replace_lane 12
        get_local 29
        i8x16.replace_lane 13
        get_local 30
        i8x16.replace_lane 14
        get_local 31
        i8x16.replace_lane 15
        set_local 33

        (set_local
            34
            (i8x16.add_sat_u
                (get_local  32)
                (get_local  33)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  34))
    )
    (func
        (export  "func_i8x16_sub_3_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 32

        v128.const i32x4 0 0 0 0
        get_local 16
        i8x16.replace_lane 0
        get_local 17
        i8x16.replace_lane 1
        get_local 18
        i8x16.replace_lane 2
        get_local 19
        i8x16.replace_lane 3
        get_local 20
        i8x16.replace_lane 4
        get_local 21
        i8x16.replace_lane 5
        get_local 22
        i8x16.replace_lane 6
        get_local 23
        i8x16.replace_lane 7
        get_local 24
        i8x16.replace_lane 8
        get_local 25
        i8x16.replace_lane 9
        get_local 26
        i8x16.replace_lane 10
        get_local 27
        i8x16.replace_lane 11
        get_local 28
        i8x16.replace_lane 12
        get_local 29
        i8x16.replace_lane 13
        get_local 30
        i8x16.replace_lane 14
        get_local 31
        i8x16.replace_lane 15
        set_local 33

        (set_local
            34
            (i8x16.sub
                (get_local  32)
                (get_local  33)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  34))
    )
    (func
        (export  "func_i8x16_subsaturate_3_s_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 32

        v128.const i32x4 0 0 0 0
        get_local 16
        i8x16.replace_lane 0
        get_local 17
        i8x16.replace_lane 1
        get_local 18
        i8x16.replace_lane 2
        get_local 19
        i8x16.replace_lane 3
        get_local 20
        i8x16.replace_lane 4
        get_local 21
        i8x16.replace_lane 5
        get_local 22
        i8x16.replace_lane 6
        get_local 23
        i8x16.replace_lane 7
        get_local 24
        i8x16.replace_lane 8
        get_local 25
        i8x16.replace_lane 9
        get_local 26
        i8x16.replace_lane 10
        get_local 27
        i8x16.replace_lane 11
        get_local 28
        i8x16.replace_lane 12
        get_local 29
        i8x16.replace_lane 13
        get_local 30
        i8x16.replace_lane 14
        get_local 31
        i8x16.replace_lane 15
        set_local 33

        (set_local
            34
            (i8x16.sub_sat_s
                (get_local  32)
                (get_local  33)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  34))
    )
    (func
        (export  "func_i8x16_subsaturate_3_u_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 32

        v128.const i32x4 0 0 0 0
        get_local 16
        i8x16.replace_lane 0
        get_local 17
        i8x16.replace_lane 1
        get_local 18
        i8x16.replace_lane 2
        get_local 19
        i8x16.replace_lane 3
        get_local 20
        i8x16.replace_lane 4
        get_local 21
        i8x16.replace_lane 5
        get_local 22
        i8x16.replace_lane 6
        get_local 23
        i8x16.replace_lane 7
        get_local 24
        i8x16.replace_lane 8
        get_local 25
        i8x16.replace_lane 9
        get_local 26
        i8x16.replace_lane 10
        get_local 27
        i8x16.replace_lane 11
        get_local 28
        i8x16.replace_lane 12
        get_local 29
        i8x16.replace_lane 13
        get_local 30
        i8x16.replace_lane 14
        get_local 31
        i8x16.replace_lane 15
        set_local 33

        (set_local
            34
            (i8x16.sub_sat_u
                (get_local  32)
                (get_local  33)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  34))
    )
    (func
        (export  "func_i8x16_shl_3_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 17

        (set_local
            17
            (i8x16.shl
                (get_local  17)
                (get_local  16)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  17))
    )
    (func
        (export  "func_i8x16_shr_3_s_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 17

        (set_local
            17
            (i8x16.shr_s
                (get_local  17)
                (get_local  16)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  17))
    )
    (func
        (export  "func_i8x16_shr_3_u_u")
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)
        (result  i32)
        (local  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        i8x16.replace_lane 0
        get_local 1
        i8x16.replace_lane 1
        get_local 2
        i8x16.replace_lane 2
        get_local 3
        i8x16.replace_lane 3
        get_local 4
        i8x16.replace_lane 4
        get_local 5
        i8x16.replace_lane 5
        get_local 6
        i8x16.replace_lane 6
        get_local 7
        i8x16.replace_lane 7
        get_local 8
        i8x16.replace_lane 8
        get_local 9
        i8x16.replace_lane 9
        get_local 10
        i8x16.replace_lane 10
        get_local 11
        i8x16.replace_lane 11
        get_local 12
        i8x16.replace_lane 12
        get_local 13
        i8x16.replace_lane 13
        get_local 14
        i8x16.replace_lane 14
        get_local 15
        i8x16.replace_lane 15
        set_local 17

        (set_local
            17
            (i8x16.shr_u
                (get_local  17)
                (get_local  16)
            )
        )
        (i8x16.extract_lane_u 3 (get_local  17))
    )
    (func
        (export  "func_f32x4_add_3")
        (param  f32  f32  f32  f32  f32  f32  f32  f32)
        (result  f32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        f32x4.replace_lane 0
        get_local 1
        f32x4.replace_lane 1
        get_local 2
        f32x4.replace_lane 2
        get_local 3
        f32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        f32x4.replace_lane 0
        get_local 5
        f32x4.replace_lane 1
        get_local 6
        f32x4.replace_lane 2
        get_local 7
        f32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (f32x4.add
                (get_local  8)
                (get_local  9)
            )
        )
        (f32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_f32x4_sub_3")
        (param  f32  f32  f32  f32  f32  f32  f32  f32)
        (result  f32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        f32x4.replace_lane 0
        get_local 1
        f32x4.replace_lane 1
        get_local 2
        f32x4.replace_lane 2
        get_local 3
        f32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        f32x4.replace_lane 0
        get_local 5
        f32x4.replace_lane 1
        get_local 6
        f32x4.replace_lane 2
        get_local 7
        f32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (f32x4.sub
                (get_local  8)
                (get_local  9)
            )
        )
        (f32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_f32x4_mul_3")
        (param  f32  f32  f32  f32  f32  f32  f32  f32)
        (result  f32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        f32x4.replace_lane 0
        get_local 1
        f32x4.replace_lane 1
        get_local 2
        f32x4.replace_lane 2
        get_local 3
        f32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        f32x4.replace_lane 0
        get_local 5
        f32x4.replace_lane 1
        get_local 6
        f32x4.replace_lane 2
        get_local 7
        f32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (f32x4.mul
                (get_local  8)
                (get_local  9)
            )
        )
        (f32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_f32x4_div_3")
        (param  f32  f32  f32  f32  f32  f32  f32  f32)
        (result  f32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        f32x4.replace_lane 0
        get_local 1
        f32x4.replace_lane 1
        get_local 2
        f32x4.replace_lane 2
        get_local 3
        f32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        f32x4.replace_lane 0
        get_local 5
        f32x4.replace_lane 1
        get_local 6
        f32x4.replace_lane 2
        get_local 7
        f32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (f32x4.div
                (get_local  8)
                (get_local  9)
            )
        )
        (f32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_f32x4_min_3")
        (param  f32  f32  f32  f32  f32  f32  f32  f32)
        (result  f32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        f32x4.replace_lane 0
        get_local 1
        f32x4.replace_lane 1
        get_local 2
        f32x4.replace_lane 2
        get_local 3
        f32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        f32x4.replace_lane 0
        get_local 5
        f32x4.replace_lane 1
        get_local 6
        f32x4.replace_lane 2
        get_local 7
        f32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (f32x4.min
                (get_local  8)
                (get_local  9)
            )
        )
        (f32x4.extract_lane 3 (get_local  10))
    )
    (func
        (export  "func_f32x4_max_3")
        (param  f32  f32  f32  f32  f32  f32  f32  f32)
        (result  f32)
        (local  v128  v128  v128)

        v128.const i32x4 0 0 0 0
        get_local 0
        f32x4.replace_lane 0
        get_local 1
        f32x4.replace_lane 1
        get_local 2
        f32x4.replace_lane 2
        get_local 3
        f32x4.replace_lane 3
        set_local 8

        v128.const i32x4 0 0 0 0
        get_local 4
        f32x4.replace_lane 0
        get_local 5
        f32x4.replace_lane 1
        get_local 6
        f32x4.replace_lane 2
        get_local 7
        f32x4.replace_lane 3
        set_local 9

        (set_local
            10
            (f32x4.max
                (get_local  8)
                (get_local  9)
            )
        )
        (f32x4.extract_lane 3 (get_local  10))
    )
)
