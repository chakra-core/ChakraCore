;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (func $convert_b32x4_to_i32
        (param b32x4) (result i32) (local i32 i32)
        (set_local 1 (i32.const 0))
        (set_local 2 (i32.const 1))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b32x4.extractLane (get_local 0) (i32.const 3)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b32x4.extractLane (get_local 0) (i32.const 2)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b32x4.extractLane (get_local 0) (i32.const 1)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b32x4.extractLane (get_local 0) (i32.const 0)))))
        (get_local 1)
    )

    (func $convert_b16x8_to_i32
        (param b16x8) (result i32) (local i32 i32)
        (set_local 1 (i32.const 0))
        (set_local 2 (i32.const 1))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 7)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 6)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 5)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 4)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 3)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 2)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 1)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b16x8.extractLane (get_local 0) (i32.const 0)))))
        (get_local 1)
    )

    (func $convert_b8x16_to_i32
        (param b8x16) (result i32) (local i32 i32)
        (set_local 1 (i32.const 0))
        (set_local 2 (i32.const 1))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 15)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 14)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 13)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 12)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 11)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 10)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 9)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 8)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 7)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 6)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 5)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 4)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 3)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 2)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 1)))))
        (set_local 2 (i32.mul (get_local 2)(i32.const 2)))
        ;;
        (set_local 1 (i32.add (get_local 1) (i32.mul  (get_local 2) (b8x16.extractLane (get_local 0) (i32.const 0)))))
        (get_local 1)
    )


    (func (export "func_i32x4_compare_u")
                                (param i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32) (local i32x4 i32x4)
                                (set_local 9  (i32x4.build (get_local 0) (get_local 1) (get_local 2) (get_local 3)))
                                (set_local 10  (i32x4.build (get_local 4) (get_local 5) (get_local 6) (get_local 7)))
                                (block $switch
                                (block $7
                                    (block $default
                                        (block $ge
                                        (block $gt
                                            (block $le
                                            (block $lt
                                                (block $ne
                                                (block $eq
                                                    (br_table $eq $ne $lt $le $gt $ge $default
                                                    (get_local 8)
                                                    )
                                                (unreachable) ;;
                                                )
                                                (return (call $convert_b32x4_to_i32 (i32x4.eq (get_local  9) (get_local  10)))) ;; eq
                                                )
                                                (return (call $convert_b32x4_to_i32 (i32x4.ne (get_local  9) (get_local  10)))) ;; ne
                                            )
                                            (return (call $convert_b32x4_to_i32 (i32x4.lt_u (get_local  9) (get_local  10)))) ;; lt
                                            )
                                            (return (call $convert_b32x4_to_i32 (i32x4.le_u (get_local  9) (get_local  10)))) ;; le
                                        )
                                        (return (call $convert_b32x4_to_i32 (i32x4.gt_u (get_local  9) (get_local  10)))) ;; gt
                                        )
                                    (return (call $convert_b32x4_to_i32 (i32x4.ge_u (get_local  9) (get_local  10)))) ;; ge
                                    )
                                    (unreachable)
                                ) ;; 7
                                    (unreachable)
                                )
                                (unreachable)
                            )

    (func (export "func_i32x4_compare_s")
                                (param i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32) (local i32x4 i32x4)
                                (set_local 9  (i32x4.build (get_local 0) (get_local 1) (get_local 2) (get_local 3)))
                                (set_local 10  (i32x4.build (get_local 4) (get_local 5) (get_local 6) (get_local 7)))
                                (block $switch
                                (block $7
                                    (block $default
                                        (block $ge
                                        (block $gt
                                            (block $le
                                            (block $lt
                                                (block $ne
                                                (block $eq
                                                    (br_table $eq $ne $lt $le $gt $ge $default
                                                    (get_local 8)
                                                    )
                                                (unreachable) ;;
                                                )
                                                (return (call $convert_b32x4_to_i32 (i32x4.eq (get_local  9) (get_local  10)))) ;; eq
                                                )
                                                (return (call $convert_b32x4_to_i32 (i32x4.ne (get_local  9) (get_local  10)))) ;; ne
                                            )
                                            (return (call $convert_b32x4_to_i32 (i32x4.lt_s (get_local  9) (get_local  10)))) ;; lt
                                            )
                                            (return (call $convert_b32x4_to_i32 (i32x4.le_s (get_local  9) (get_local  10)))) ;; le
                                        )
                                        (return (call $convert_b32x4_to_i32 (i32x4.gt_s (get_local  9) (get_local  10)))) ;; gt
                                        )
                                    (return (call $convert_b32x4_to_i32 (i32x4.ge_s (get_local  9) (get_local  10)))) ;; ge
                                    )
                                    (unreachable)
                                ) ;; 7
                                    (unreachable)
                                )
                                (unreachable)
                            )

    (func (export "func_i16x8_compare_u")
                                (param i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32) (local i16x8 i16x8)
                                (set_local 17  (i16x8.build (get_local 0) (get_local 1) (get_local 2) (get_local 3) (get_local 4) (get_local 5) (get_local 6) (get_local 7)))
                                (set_local 18  (i16x8.build (get_local 8) (get_local 9) (get_local 10) (get_local 11) (get_local 12) (get_local 13) (get_local 14) (get_local 15)))
                                (block $switch
                                (block $7
                                    (block $default
                                        (block $ge
                                        (block $gt
                                            (block $le
                                            (block $lt
                                                (block $ne
                                                (block $eq
                                                    (br_table $eq $ne $lt $le $gt $ge $default
                                                    (get_local 16)
                                                    )
                                                (unreachable) ;;
                                                )
                                                (return (call $convert_b16x8_to_i32 (i16x8.eq (get_local  17) (get_local  18)))) ;; eq
                                                )
                                                (return (call $convert_b16x8_to_i32 (i16x8.ne (get_local  17) (get_local  18)))) ;; ne
                                            )
                                            (return (call $convert_b16x8_to_i32 (i16x8.lt_u (get_local  17) (get_local  18)))) ;; lt
                                            )
                                            (return (call $convert_b16x8_to_i32 (i16x8.le_u (get_local  17) (get_local  18)))) ;; le
                                        )
                                        (return (call $convert_b16x8_to_i32 (i16x8.gt_u (get_local  17) (get_local  18)))) ;; gt
                                        )
                                    (return (call $convert_b16x8_to_i32 (i16x8.ge_u (get_local  17) (get_local  18)))) ;; ge
                                    )
                                    (unreachable)
                                ) ;; 7
                                    (unreachable)
                                )
                                (unreachable)
                            )

    (func (export "func_i16x8_compare_s")
                                (param i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32) (local i16x8 i16x8)
                                (set_local 17  (i16x8.build (get_local 0) (get_local 1) (get_local 2) (get_local 3) (get_local 4) (get_local 5) (get_local 6) (get_local 7)))
                                (set_local 18  (i16x8.build (get_local 8) (get_local 9) (get_local 10) (get_local 11) (get_local 12) (get_local 13) (get_local 14) (get_local 15)))
                                (block $switch
                                (block $7
                                    (block $default
                                        (block $ge
                                        (block $gt
                                            (block $le
                                            (block $lt
                                                (block $ne
                                                (block $eq
                                                    (br_table $eq $ne $lt $le $gt $ge $default
                                                    (get_local 16)
                                                    )
                                                (unreachable) ;;
                                                )
                                                (return (call $convert_b16x8_to_i32 (i16x8.eq (get_local  17) (get_local  18)))) ;; eq
                                                )
                                                (return (call $convert_b16x8_to_i32 (i16x8.ne (get_local  17) (get_local  18)))) ;; ne
                                            )
                                            (return (call $convert_b16x8_to_i32 (i16x8.lt_s (get_local  17) (get_local  18)))) ;; lt
                                            )
                                            (return (call $convert_b16x8_to_i32 (i16x8.le_s (get_local  17) (get_local  18)))) ;; le
                                        )
                                        (return (call $convert_b16x8_to_i32 (i16x8.gt_s (get_local  17) (get_local  18)))) ;; gt
                                        )
                                    (return (call $convert_b16x8_to_i32 (i16x8.ge_s (get_local  17) (get_local  18)))) ;; ge
                                    )
                                    (unreachable)
                                ) ;; 7
                                    (unreachable)
                                )
                                (unreachable)
                            )

    (func (export "func_i8x16_compare_u")
                                (param i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32) (local i8x16 i8x16)
                                (set_local 33  (i8x16.build (get_local 0) (get_local 1) (get_local 2) (get_local 3) (get_local 4) (get_local 5) (get_local 6) (get_local 7) (get_local 8) (get_local 9) (get_local 10) (get_local 11) (get_local 12) (get_local 13) (get_local 14) (get_local 15)))
                                (set_local 34  (i8x16.build (get_local 16) (get_local 17) (get_local 18) (get_local 19) (get_local 20) (get_local 21) (get_local 22) (get_local 23) (get_local 24) (get_local 25) (get_local 26) (get_local 27) (get_local 28) (get_local 29) (get_local 30) (get_local 31)))
                                (block $switch
                                (block $7
                                    (block $default
                                        (block $ge
                                        (block $gt
                                            (block $le
                                            (block $lt
                                                (block $ne
                                                (block $eq
                                                    (br_table $eq $ne $lt $le $gt $ge $default
                                                    (get_local 32)
                                                    )
                                                (unreachable) ;;
                                                )
                                                (return (call $convert_b8x16_to_i32 (i8x16.eq (get_local  33) (get_local  34)))) ;; eq
                                                )
                                                (return (call $convert_b8x16_to_i32 (i8x16.ne (get_local  33) (get_local  34)))) ;; ne
                                            )
                                            (return (call $convert_b8x16_to_i32 (i8x16.lt_u (get_local  33) (get_local  34)))) ;; lt
                                            )
                                            (return (call $convert_b8x16_to_i32 (i8x16.le_u (get_local  33) (get_local  34)))) ;; le
                                        )
                                        (return (call $convert_b8x16_to_i32 (i8x16.gt_u (get_local  33) (get_local  34)))) ;; gt
                                        )
                                    (return (call $convert_b8x16_to_i32 (i8x16.ge_u (get_local  33) (get_local  34)))) ;; ge
                                    )
                                    (unreachable)
                                ) ;; 7
                                    (unreachable)
                                )
                                (unreachable)
                            )

    (func (export "func_i8x16_compare_s")
                                (param i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32) (local i8x16 i8x16)
                                (set_local 33  (i8x16.build (get_local 0) (get_local 1) (get_local 2) (get_local 3) (get_local 4) (get_local 5) (get_local 6) (get_local 7) (get_local 8) (get_local 9) (get_local 10) (get_local 11) (get_local 12) (get_local 13) (get_local 14) (get_local 15)))
                                (set_local 34  (i8x16.build (get_local 16) (get_local 17) (get_local 18) (get_local 19) (get_local 20) (get_local 21) (get_local 22) (get_local 23) (get_local 24) (get_local 25) (get_local 26) (get_local 27) (get_local 28) (get_local 29) (get_local 30) (get_local 31)))
                                (block $switch
                                (block $7
                                    (block $default
                                        (block $ge
                                        (block $gt
                                            (block $le
                                            (block $lt
                                                (block $ne
                                                (block $eq
                                                    (br_table $eq $ne $lt $le $gt $ge $default
                                                    (get_local 32)
                                                    )
                                                (unreachable) ;;
                                                )
                                                (return (call $convert_b8x16_to_i32 (i8x16.eq (get_local  33) (get_local  34)))) ;; eq
                                                )
                                                (return (call $convert_b8x16_to_i32 (i8x16.ne (get_local  33) (get_local  34)))) ;; ne
                                            )
                                            (return (call $convert_b8x16_to_i32 (i8x16.lt_s (get_local  33) (get_local  34)))) ;; lt
                                            )
                                            (return (call $convert_b8x16_to_i32 (i8x16.le_s (get_local  33) (get_local  34)))) ;; le
                                        )
                                        (return (call $convert_b8x16_to_i32 (i8x16.gt_s (get_local  33) (get_local  34)))) ;; gt
                                        )
                                    (return (call $convert_b8x16_to_i32 (i8x16.ge_s (get_local  33) (get_local  34)))) ;; ge
                                    )
                                    (unreachable)
                                ) ;; 7
                                    (unreachable)
                                )
                                (unreachable)
                            )

    (func (export "func_f32x4_compare")
                                (param f32 f32 f32 f32 f32 f32 f32 f32 i32) (result i32) (local f32x4 f32x4)
                                (set_local 9  (f32x4.build (get_local 0) (get_local 1) (get_local 2) (get_local 3)))
                                (set_local 10  (f32x4.build (get_local 4) (get_local 5) (get_local 6) (get_local 7)))
                                (block $switch
                                (block $7
                                    (block $default
                                        (block $ge
                                        (block $gt
                                            (block $le
                                            (block $lt
                                                (block $ne
                                                (block $eq
                                                    (br_table $eq $ne $lt $le $gt $ge $default
                                                    (get_local 8)
                                                    )
                                                (unreachable) ;;
                                                )
                                                (return (call $convert_b32x4_to_i32 (f32x4.eq (get_local  9) (get_local  10)))) ;; eq
                                                )
                                                (return (call $convert_b32x4_to_i32 (f32x4.ne (get_local  9) (get_local  10)))) ;; ne
                                            )
                                            (return (call $convert_b32x4_to_i32 (f32x4.lt (get_local  9) (get_local  10)))) ;; lt
                                            )
                                            (return (call $convert_b32x4_to_i32 (f32x4.le (get_local  9) (get_local  10)))) ;; le
                                        )
                                        (return (call $convert_b32x4_to_i32 (f32x4.gt (get_local  9) (get_local  10)))) ;; gt
                                        )
                                    (return (call $convert_b32x4_to_i32 (f32x4.ge (get_local  9) (get_local  10)))) ;; ge
                                    )
                                    (unreachable)
                                ) ;; 7
                                    (unreachable)
                                )
                                (unreachable)
                            )

)
