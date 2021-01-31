;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (import "dummy" "memory" (memory 1))

    (func (export "v128_load4") (param $x i32) (result i32)
        (i32x4.extract_lane 0 (v128.load offset=0 (get_local $x)))
    )

    (func (export "v128_load4_offset") (param $x i32) (result i32)
        (i32x4.extract_lane 0 (v128.load offset=16 (get_local $x)))
    )

    (func (export "v128_load_test") (param $x i32) (local v128)
        (set_local 1 (v128.load offset=0 (get_local $x)))
        (i32.store offset=0 (get_local $x) (i32.popcnt (i32x4.extract_lane 0 (get_local 1))))
        (set_local 1 (v128.load offset=0 (get_local $x)))
        (i32.store offset=4 (get_local $x) (i32.popcnt (i32x4.extract_lane 1 (get_local 1))))
        (set_local 1 (v128.load offset=0 (get_local $x)))
        (i32.store offset=8 (get_local $x) (i32.popcnt (i32x4.extract_lane 2 (get_local 1))))
        (set_local 1 (v128.load offset=0 (get_local $x)))
        (i32.store offset=12 (get_local $x) (i32.popcnt (i32x4.extract_lane 3 (get_local 1))))
        ;;
        (set_local 1 (v128.load offset=20 (get_local $x)))
        (i32.store offset=20 (get_local $x) (i32.popcnt (i32x4.extract_lane 0 (get_local 1))))
        (set_local 1 (v128.load offset=20 (get_local $x)))
        (i32.store offset=24 (get_local $x) (i32.popcnt (i32x4.extract_lane 1 (get_local 1))))
        (set_local 1 (v128.load offset=20 (get_local $x)))
        (i32.store offset=28 (get_local $x) (i32.popcnt (i32x4.extract_lane 2 (get_local 1))))
        (set_local 1 (v128.load offset=20 (get_local $x)))
        (i32.store offset=32 (get_local $x) (i32.popcnt (i32x4.extract_lane 3 (get_local 1))))
        ;;
        (set_local 1 (v128.load offset=44 (get_local $x)))
        (i32.store offset=44 (get_local $x) (i32.popcnt (i32x4.extract_lane 0 (get_local 1))))
        (set_local 1 (v128.load offset=44 (get_local $x)))
        (i32.store offset=48 (get_local $x) (i32.popcnt (i32x4.extract_lane 1 (get_local 1))))
        (set_local 1 (v128.load offset=44 (get_local $x)))
        (i32.store offset=52 (get_local $x) (i32.popcnt (i32x4.extract_lane 2 (get_local 1))))
        (set_local 1 (v128.load offset=44 (get_local $x)))
        (i32.store offset=56 (get_local $x) (i32.popcnt (i32x4.extract_lane 3 (get_local 1))))
        ;;
        (set_local 1 (v128.load offset=68 (get_local $x)))
        (i32.store offset=68 (get_local $x) (i32.popcnt (i32x4.extract_lane 0 (get_local 1))))
        (set_local 1 (v128.load offset=68 (get_local $x)))
        (i32.store offset=72 (get_local $x) (i32.popcnt (i32x4.extract_lane 1 (get_local 1))))
        (set_local 1 (v128.load offset=68 (get_local $x)))
        (i32.store offset=76 (get_local $x) (i32.popcnt (i32x4.extract_lane 2 (get_local 1))))
        (set_local 1 (v128.load offset=68 (get_local $x)))
        (i32.store offset=80 (get_local $x) (i32.popcnt (i32x4.extract_lane 3 (get_local 1))))
    )
)
