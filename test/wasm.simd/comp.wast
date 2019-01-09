;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (import "dummy" "memory" (memory 1))


  (func (export "func_i32x4_compare_s") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (i32x4.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (i32x4.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (i32x4.lt_s
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (i32x4.le_s
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (i32x4.gt_s
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (i32x4.ge_s
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )


  (func (export "func_i32x4_compare_u") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (i32x4.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (i32x4.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (i32x4.lt_u
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (i32x4.le_u
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (i32x4.gt_u
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (i32x4.ge_u
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )


  (func (export "func_i16x8_compare_s") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (i16x8.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (i16x8.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (i16x8.lt_s
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (i16x8.le_s
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (i16x8.gt_s
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (i16x8.ge_s
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )


  (func (export "func_i16x8_compare_u") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (i16x8.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (i16x8.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (i16x8.lt_u
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (i16x8.le_u
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (i16x8.gt_u
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (i16x8.ge_u
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )


  (func (export "func_i8x16_compare_s") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (i8x16.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (i8x16.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (i8x16.lt_s
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (i8x16.le_s
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (i8x16.gt_s
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (i8x16.ge_s
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )


  (func (export "func_i8x16_compare_u") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (i8x16.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (i8x16.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (i8x16.lt_u
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (i8x16.le_u
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (i8x16.gt_u
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (i8x16.ge_u
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )


  (func (export "func_f32x4_compare") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (f32x4.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (f32x4.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (f32x4.lt
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (f32x4.le
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (f32x4.gt
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (f32x4.ge
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )

  (func (export "func_f64x2_compare") (param $2 i32)
    (block $label$0
      (block $label$1
        (block $label$2
          (block $label$3
            (block $label$4
              (block $label$5
                (br_if $label$5
                  (i32.gt_u
                    (get_local $2)
                    (i32.const 5)
                  )
                )
                (block $label$6
                  (br_table $label$6 $label$4 $label$3 $label$2 $label$1 $label$0 $label$6
                    (get_local $2)
                  )
                )
                (v128.store offset=0 align=4 (i32.const 0)
                  (f64x2.eq
                    (v128.load offset=0 align=4 (i32.const 0))
                    (v128.load offset=16 align=4 (i32.const 0))
                  )
                )
                (return)
              )
                (unreachable) ;; invalid operation
            )
            (v128.store offset=0 align=4 (i32.const 0)
              (f64x2.ne
                (v128.load offset=0 align=4 (i32.const 0))
                (v128.load offset=16 align=4 (i32.const 0))
              )
            )
            (return)
          )
          (v128.store offset=0 align=4 (i32.const 0)
            (f64x2.lt
              (v128.load offset=0 align=4 (i32.const 0))
              (v128.load offset=16 align=4 (i32.const 0))
            )
          )
          (return)
        )
        (v128.store offset=0 align=4 (i32.const 0)
          (f64x2.le
            (v128.load offset=0 align=4 (i32.const 0))
            (v128.load offset=16 align=4 (i32.const 0))
          )
        )
        (return)
      )
      (v128.store offset=0 align=4 (i32.const 0)
        (f64x2.gt
          (v128.load offset=0 align=4 (i32.const 0))
          (v128.load offset=16 align=4 (i32.const 0))
        )
      )
      (return)
    )
    (v128.store offset=0 align=4 (i32.const 0)
      (f64x2.ge
        (v128.load offset=0 align=4 (i32.const 0))
        (v128.load offset=16 align=4 (i32.const 0))
      )
    )
  )
)
