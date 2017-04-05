;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
 
(module 
    (func 
        (export  "func_i32x4_add_3")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i32x4  i32x4  i32x4)  
        (set_local 
            8 
            (i32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (i32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (i32x4.add 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (i32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i32x4_sub_3")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i32x4  i32x4  i32x4)  
        (set_local 
            8 
            (i32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (i32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (i32x4.sub 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (i32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i32x4_mul_3")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i32x4  i32x4  i32x4)  
        (set_local 
            8 
            (i32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (i32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (i32x4.mul 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (i32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i32x4_shl_3")  
        (param  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i32x4)  
        (set_local 
            5 
            (i32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            5 
            (i32x4.shl 
                (get_local  5)  
                (get_local  4)  
            ) 
        ) 
        (i32x4.extractLane 
            (get_local  5)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i32x4_shr_3_s")  
        (param  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i32x4)  
        (set_local 
            5 
            (i32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            5 
            (i32x4.shr_s 
                (get_local  5)  
                (get_local  4)  
            ) 
        ) 
        (i32x4.extractLane 
            (get_local  5)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i32x4_shr_3_u")  
        (param  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i32x4)  
        (set_local 
            5 
            (i32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            5 
            (i32x4.shr_u 
                (get_local  5)  
                (get_local  4)  
            ) 
        ) 
        (i32x4.extractLane 
            (get_local  5)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_add_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8  i16x8  i16x8)  
        (set_local 
            16 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            17 
            (i16x8.build 
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            18 
            (i16x8.add 
                (get_local  16)  
                (get_local  17)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  18)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_addsaturate_3_s_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8  i16x8  i16x8)  
        (set_local 
            16 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            17 
            (i16x8.build 
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            18 
            (i16x8.addsaturate_s 
                (get_local  16)  
                (get_local  17)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  18)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_addsaturate_3_u_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8  i16x8  i16x8)  
        (set_local 
            16 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            17 
            (i16x8.build 
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            18 
            (i16x8.addsaturate_u 
                (get_local  16)  
                (get_local  17)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  18)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_sub_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8  i16x8  i16x8)  
        (set_local 
            16 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            17 
            (i16x8.build 
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            18 
            (i16x8.sub 
                (get_local  16)  
                (get_local  17)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  18)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_subsaturate_3_s_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8  i16x8  i16x8)  
        (set_local 
            16 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            17 
            (i16x8.build 
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            18 
            (i16x8.subsaturate_s 
                (get_local  16)  
                (get_local  17)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  18)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_subsaturate_3_u_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8  i16x8  i16x8)  
        (set_local 
            16 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            17 
            (i16x8.build 
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            18 
            (i16x8.subsaturate_u 
                (get_local  16)  
                (get_local  17)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  18)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_mul_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8  i16x8  i16x8)  
        (set_local 
            16 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            17 
            (i16x8.build 
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            18 
            (i16x8.mul 
                (get_local  16)  
                (get_local  17)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  18)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_shl_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8)  
        (set_local 
            9 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            9 
            (i16x8.shl 
                (get_local  9)  
                (get_local  8)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  9)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_shr_3_s_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8)  
        (set_local 
            9 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            9 
            (i16x8.shr_s 
                (get_local  9)  
                (get_local  8)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  9)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i16x8_shr_3_u_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i16x8)  
        (set_local 
            9 
            (i16x8.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            9 
            (i16x8.shr_u 
                (get_local  9)  
                (get_local  8)  
            ) 
        ) 
        (i16x8.extractLane_u 
            (get_local  9)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_add_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16  i8x16  i8x16)  
        (set_local 
            32 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            33 
            (i8x16.build 
                (get_local  16)  
                (get_local  17)  
                (get_local  18)  
                (get_local  19)  
                (get_local  20)  
                (get_local  21)  
                (get_local  22)  
                (get_local  23)  
                (get_local  24)  
                (get_local  25)  
                (get_local  26)  
                (get_local  27)  
                (get_local  28)  
                (get_local  29)  
                (get_local  30)  
                (get_local  31)  
            ) 
        ) 
        (set_local 
            34 
            (i8x16.add 
                (get_local  32)  
                (get_local  33)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  34)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_addsaturate_3_s_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16  i8x16  i8x16)  
        (set_local 
            32 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            33 
            (i8x16.build 
                (get_local  16)  
                (get_local  17)  
                (get_local  18)  
                (get_local  19)  
                (get_local  20)  
                (get_local  21)  
                (get_local  22)  
                (get_local  23)  
                (get_local  24)  
                (get_local  25)  
                (get_local  26)  
                (get_local  27)  
                (get_local  28)  
                (get_local  29)  
                (get_local  30)  
                (get_local  31)  
            ) 
        ) 
        (set_local 
            34 
            (i8x16.addsaturate_s 
                (get_local  32)  
                (get_local  33)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  34)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_addsaturate_3_u_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16  i8x16  i8x16)  
        (set_local 
            32 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            33 
            (i8x16.build 
                (get_local  16)  
                (get_local  17)  
                (get_local  18)  
                (get_local  19)  
                (get_local  20)  
                (get_local  21)  
                (get_local  22)  
                (get_local  23)  
                (get_local  24)  
                (get_local  25)  
                (get_local  26)  
                (get_local  27)  
                (get_local  28)  
                (get_local  29)  
                (get_local  30)  
                (get_local  31)  
            ) 
        ) 
        (set_local 
            34 
            (i8x16.addsaturate_u 
                (get_local  32)  
                (get_local  33)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  34)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_sub_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16  i8x16  i8x16)  
        (set_local 
            32 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            33 
            (i8x16.build 
                (get_local  16)  
                (get_local  17)  
                (get_local  18)  
                (get_local  19)  
                (get_local  20)  
                (get_local  21)  
                (get_local  22)  
                (get_local  23)  
                (get_local  24)  
                (get_local  25)  
                (get_local  26)  
                (get_local  27)  
                (get_local  28)  
                (get_local  29)  
                (get_local  30)  
                (get_local  31)  
            ) 
        ) 
        (set_local 
            34 
            (i8x16.sub 
                (get_local  32)  
                (get_local  33)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  34)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_subsaturate_3_s_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16  i8x16  i8x16)  
        (set_local 
            32 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            33 
            (i8x16.build 
                (get_local  16)  
                (get_local  17)  
                (get_local  18)  
                (get_local  19)  
                (get_local  20)  
                (get_local  21)  
                (get_local  22)  
                (get_local  23)  
                (get_local  24)  
                (get_local  25)  
                (get_local  26)  
                (get_local  27)  
                (get_local  28)  
                (get_local  29)  
                (get_local  30)  
                (get_local  31)  
            ) 
        ) 
        (set_local 
            34 
            (i8x16.subsaturate_s 
                (get_local  32)  
                (get_local  33)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  34)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_subsaturate_3_u_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16  i8x16  i8x16)  
        (set_local 
            32 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            33 
            (i8x16.build 
                (get_local  16)  
                (get_local  17)  
                (get_local  18)  
                (get_local  19)  
                (get_local  20)  
                (get_local  21)  
                (get_local  22)  
                (get_local  23)  
                (get_local  24)  
                (get_local  25)  
                (get_local  26)  
                (get_local  27)  
                (get_local  28)  
                (get_local  29)  
                (get_local  30)  
                (get_local  31)  
            ) 
        ) 
        (set_local 
            34 
            (i8x16.subsaturate_u 
                (get_local  32)  
                (get_local  33)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  34)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_mul_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16  i8x16  i8x16)  
        (set_local 
            32 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            33 
            (i8x16.build 
                (get_local  16)  
                (get_local  17)  
                (get_local  18)  
                (get_local  19)  
                (get_local  20)  
                (get_local  21)  
                (get_local  22)  
                (get_local  23)  
                (get_local  24)  
                (get_local  25)  
                (get_local  26)  
                (get_local  27)  
                (get_local  28)  
                (get_local  29)  
                (get_local  30)  
                (get_local  31)  
            ) 
        ) 
        (set_local 
            34 
            (i8x16.mul 
                (get_local  32)  
                (get_local  33)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  34)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_shl_3_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16)  
        (set_local 
            17 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            17 
            (i8x16.shl 
                (get_local  17)  
                (get_local  16)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  17)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_shr_3_s_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16)  
        (set_local 
            17 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            17 
            (i8x16.shr_s 
                (get_local  17)  
                (get_local  16)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  17)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_i8x16_shr_3_u_u")  
        (param  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32  i32)  
        (result  i32)  
        (local  i8x16)  
        (set_local 
            17 
            (i8x16.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
                (get_local  8)  
                (get_local  9)  
                (get_local  10)  
                (get_local  11)  
                (get_local  12)  
                (get_local  13)  
                (get_local  14)  
                (get_local  15)  
            ) 
        ) 
        (set_local 
            17 
            (i8x16.shr_u 
                (get_local  17)  
                (get_local  16)  
            ) 
        ) 
        (i8x16.extractLane_u 
            (get_local  17)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_f32x4_add_3")  
        (param  f32  f32  f32  f32  f32  f32  f32  f32)  
        (result  f32)  
        (local  f32x4  f32x4  f32x4)  
        (set_local 
            8 
            (f32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (f32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (f32x4.add 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (f32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_f32x4_sub_3")  
        (param  f32  f32  f32  f32  f32  f32  f32  f32)  
        (result  f32)  
        (local  f32x4  f32x4  f32x4)  
        (set_local 
            8 
            (f32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (f32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (f32x4.sub 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (f32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_f32x4_mul_3")  
        (param  f32  f32  f32  f32  f32  f32  f32  f32)  
        (result  f32)  
        (local  f32x4  f32x4  f32x4)  
        (set_local 
            8 
            (f32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (f32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (f32x4.mul 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (f32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_f32x4_div_3")  
        (param  f32  f32  f32  f32  f32  f32  f32  f32)  
        (result  f32)  
        (local  f32x4  f32x4  f32x4)  
        (set_local 
            8 
            (f32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (f32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (f32x4.div 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (f32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_f32x4_min_3")  
        (param  f32  f32  f32  f32  f32  f32  f32  f32)  
        (result  f32)  
        (local  f32x4  f32x4  f32x4)  
        (set_local 
            8 
            (f32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (f32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (f32x4.min 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (f32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
    (func 
        (export  "func_f32x4_max_3")  
        (param  f32  f32  f32  f32  f32  f32  f32  f32)  
        (result  f32)  
        (local  f32x4  f32x4  f32x4)  
        (set_local 
            8 
            (f32x4.build 
                (get_local  0)  
                (get_local  1)  
                (get_local  2)  
                (get_local  3)  
            ) 
        ) 
        (set_local 
            9 
            (f32x4.build 
                (get_local  4)  
                (get_local  5)  
                (get_local  6)  
                (get_local  7)  
            ) 
        ) 
        (set_local 
            10 
            (f32x4.max 
                (get_local  8)  
                (get_local  9)  
            ) 
        ) 
        (f32x4.extractLane 
            (get_local  10)  
            (i32.const  3)  
        ) 
    ) 
) 
