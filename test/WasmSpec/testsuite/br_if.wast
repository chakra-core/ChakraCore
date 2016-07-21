;; Test `br_if` operator

(module
  (func $dummy)

  (func "as-block-first" (param i32) (result i32)
    (block (br_if 0 (get_local 0)) (return (i32.const 2))) (i32.const 3)
  )
  (func "as-block-mid" (param i32) (result i32)
    (block (call $dummy) (br_if 0 (get_local 0)) (return (i32.const 2))) (i32.const 3)
  )
  (func "as-block-last" (param i32)
    (block (call $dummy) (call $dummy) (br_if 0 (get_local 0)))
  )
  (func "as-block-first-value" (param i32) (result i32)
    (block (br_if 0 (i32.const 10) (get_local 0)) (i32.const 11))
  )
  (func "as-block-mid-value" (param i32) (result i32)
    (block (call $dummy) (br_if 0 (i32.const 20) (get_local 0)) (i32.const 21))
  )

  (func "as-loop-first" (param i32) (result i32)
    (loop (br_if 1 (i32.const 3) (get_local 0)) (i32.const 2))
  )
  (func "as-loop-mid" (param i32) (result i32)
    (loop (call $dummy) (br_if 1 (i32.const 4) (get_local 0)) (i32.const 2))
  )
  (func "as-loop-last" (param i32)
    (loop (call $dummy) (br_if 1 (get_local 0)))
  )

  (func "as-br-value" (param i32)
    (block (br 0 (br_if 0 (get_local 0))))
  )
  (func "as-br_if-value" (param i32 i32)
    (block (br_if 0 (br_if 0 (get_local 0)) (get_local 1)))
  )
  (func "as-br_table-value" (param i32 i32)
    (block (br_table 0 0 0 (br_if 0 (get_local 0)) (get_local 1)))
  )

  (func "as-return-value" (param i32)
    (block (return (br_if 0 (get_local 0))))
  )

  (func "as-if-then" (param i32 i32)
    (block (if (get_local 0) (br_if 1 (i32.const 3) (get_local 1)) (call $dummy)))
  )
  (func "as-if-else" (param i32 i32)
    (block (if (get_local 0) (call $dummy) (br_if 1 (i32.const 4) (get_local 1))))
  )

  (func "nested-block-value" (param i32) (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (i32.add
          (i32.const 4)
          (block (br_if 1 (i32.const 8) (get_local 0)) (i32.const 16))
        )
      )
    )
  )

  (func "nested-br-value" (param i32) (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (br 0
          (block (br_if 1 (i32.const 8) (get_local 0)) (i32.const 4))
        )
        (i32.const 16)
      )
    )
  )

  (func "nested-br_if-value" (param i32) (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (br_if 0
          (block (br_if 1 (i32.const 8) (get_local 0)) (i32.const 4))
          (i32.const 1)
        )
        (i32.const 16)
      )
    )
  )

  (func "nested-br_if-value-cond" (param i32) (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (br_if 0
          (i32.const 4)
          (block (br_if 1 (i32.const 8) (get_local 0)) (i32.const 1))
        )
        (i32.const 16)
      )
    )
  )

  (func "nested-br_table-value" (param i32) (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (br_table 0
          (block (br_if 1 (i32.const 8) (get_local 0)) (i32.const 4))
          (i32.const 1)
        )
        (i32.const 16)
      )
    )
  )

  (func "nested-br_table-value-index" (param i32) (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (br_table 0
          (i32.const 4)
          (block (br_if 1 (i32.const 8) (get_local 0)) (i32.const 1))
        )
        (i32.const 16)
      )
    )
  )
)

(assert_return (invoke "as-block-first" (i32.const 0)) (i32.const 2))
(assert_return (invoke "as-block-first" (i32.const 1)) (i32.const 3))
(assert_return (invoke "as-block-mid" (i32.const 0)) (i32.const 2))
(assert_return (invoke "as-block-mid" (i32.const 1)) (i32.const 3))
(assert_return (invoke "as-block-last" (i32.const 0)))
(assert_return (invoke "as-block-last" (i32.const 1)))

(assert_return (invoke "as-block-first-value" (i32.const 0)) (i32.const 11))
(assert_return (invoke "as-block-first-value" (i32.const 1)) (i32.const 10))
(assert_return (invoke "as-block-mid-value" (i32.const 0)) (i32.const 21))
(assert_return (invoke "as-block-mid-value" (i32.const 1)) (i32.const 20))

(assert_return (invoke "as-loop-first" (i32.const 0)) (i32.const 2))
(assert_return (invoke "as-loop-first" (i32.const 1)) (i32.const 3))
(assert_return (invoke "as-loop-mid" (i32.const 0)) (i32.const 2))
(assert_return (invoke "as-loop-mid" (i32.const 1)) (i32.const 4))
(assert_return (invoke "as-loop-last" (i32.const 0)))
(assert_return (invoke "as-loop-last" (i32.const 1)))

(assert_return (invoke "as-br-value" (i32.const 0)))
(assert_return (invoke "as-br-value" (i32.const 1)))

(assert_return (invoke "as-br_if-value" (i32.const 0) (i32.const 0)))
(assert_return (invoke "as-br_if-value" (i32.const 1) (i32.const 0)))
(assert_return (invoke "as-br_if-value" (i32.const 0) (i32.const 1)))
(assert_return (invoke "as-br_if-value" (i32.const 1) (i32.const 1)))

(assert_return (invoke "as-br_table-value" (i32.const 0) (i32.const 0)))
(assert_return (invoke "as-br_table-value" (i32.const 1) (i32.const 0)))
(assert_return (invoke "as-br_table-value" (i32.const 10) (i32.const 0)))
(assert_return (invoke "as-br_table-value" (i32.const 0) (i32.const 1)))
(assert_return (invoke "as-br_table-value" (i32.const 1) (i32.const 1)))
(assert_return (invoke "as-br_table-value" (i32.const 10) (i32.const 1)))

(assert_return (invoke "as-return-value" (i32.const 0)))
(assert_return (invoke "as-return-value" (i32.const 1)))

(assert_return (invoke "as-if-then" (i32.const 0) (i32.const 0)))
(assert_return (invoke "as-if-then" (i32.const 4) (i32.const 0)))
(assert_return (invoke "as-if-then" (i32.const 0) (i32.const 1)))
(assert_return (invoke "as-if-then" (i32.const 4) (i32.const 1)))
(assert_return (invoke "as-if-else" (i32.const 0) (i32.const 0)))
(assert_return (invoke "as-if-else" (i32.const 3) (i32.const 0)))
(assert_return (invoke "as-if-else" (i32.const 0) (i32.const 1)))
(assert_return (invoke "as-if-else" (i32.const 3) (i32.const 1)))

(assert_return (invoke "nested-block-value" (i32.const 0)) (i32.const 21))
(assert_return (invoke "nested-block-value" (i32.const 1)) (i32.const 9))
(assert_return (invoke "nested-br-value" (i32.const 0)) (i32.const 5))
(assert_return (invoke "nested-br-value" (i32.const 1)) (i32.const 9))
(assert_return (invoke "nested-br_if-value" (i32.const 0)) (i32.const 5))
(assert_return (invoke "nested-br_if-value" (i32.const 1)) (i32.const 9))
(assert_return (invoke "nested-br_if-value-cond" (i32.const 0)) (i32.const 5))
(assert_return (invoke "nested-br_if-value-cond" (i32.const 1)) (i32.const 9))
(assert_return (invoke "nested-br_table-value" (i32.const 0)) (i32.const 5))
(assert_return (invoke "nested-br_table-value" (i32.const 1)) (i32.const 9))
(assert_return (invoke "nested-br_table-value-index" (i32.const 0)) (i32.const 5))
(assert_return (invoke "nested-br_table-value-index" (i32.const 1)) (i32.const 9))

(assert_invalid
  (module (func $type-false-i32 (block (i32.ctz (br_if 0 (i32.const 0))))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-false-i64 (block (i64.ctz (br_if 0 (i32.const 0))))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-false-f32 (block (f32.neg (br_if 0 (i32.const 0))))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-false-f64 (block (f64.neg (br_if 0 (i32.const 0))))))
  "type mismatch"
)

(assert_invalid
  (module (func $type-true-i32 (block (i32.ctz (br_if 0 (i32.const 1))))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-true-i64 (block (i64.ctz (br_if 0 (i64.const 1))))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-true-f32 (block (f32.neg (br_if 0 (f32.const 1))))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-true-f64 (block (f64.neg (br_if 0 (i64.const 1))))))
  "type mismatch"
)

(assert_invalid
  (module (func $type-false-arg-void-vs-num (result i32)
    (block (br_if 0 (i32.const 0)) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-true-arg-void-vs-num (result i32)
    (block (br_if 0 (i32.const 1)) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-false-arg-num-vs-num (result i32)
    (block (br_if 0 (i64.const 1) (i32.const 0)) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-true-arg-num-vs-num (result i32)
    (block (br_if 0 (i64.const 1) (i32.const 0)) (i32.const 1))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-cond-void-vs-i32
    (block (br_if 0 (nop)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-cond-num-vs-i32
    (block (br_if 0 (i64.const 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-arg-cond-void-vs-i32 (result i32)
    (block (br_if 0 (i32.const 0) (nop)) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-arg-cond-num-vs-i32 (result i32)
    (block (br_if 0 (i32.const 0) (i64.const 0)) (i32.const 1))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $unbound-label (br_if 1 (i32.const 1))))
  "unknown label"
)
(assert_invalid
  (module (func $unbound-nested-label (block (block (br_if 5 (i32.const 1))))))
  "unknown label"
)
(assert_invalid
  (module (func $large-label (br_if 0x100000001 (i32.const 1))))
  "unknown label"
)

