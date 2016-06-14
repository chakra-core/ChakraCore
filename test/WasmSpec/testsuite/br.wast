;; Test `br` operator

(module
  (func "type-i32" (block (i32.ctz (br 0))))
  (func "type-i64" (block (i64.ctz (br 0))))
  (func "type-f32" (block (f32.neg (br 0))))
  (func "type-f64" (block (f64.neg (br 0))))

  (func "type-i32-value" (result i32) (block (i32.ctz (br 0 (i32.const 1)))))
  (func "type-i64-value" (result i64) (block (i64.ctz (br 0 (i64.const 2)))))
  (func "type-f32-value" (result f32) (block (f32.neg (br 0 (f32.const 3)))))
  (func "type-f64-value" (result f64) (block (f64.neg (br 0 (f64.const 4)))))

  (func "as-block-first"
    (block (br 0) (i32.const 2))
  )
  (func "as-block-mid"
    (block (i32.const 1) (br 0) (i32.const 2))
  )
  (func "as-block-last"
    (block (nop) (i32.const 1) (br 0))
  )
  (func "as-block-value" (result i32)
    (block (nop) (i32.const 1) (br 0 (i32.const 2)))
  )

  (func "as-loop-first" (result i32)
    (loop (br 1 (i32.const 3)) (i32.const 2))
  )
  (func "as-loop-mid" (result i32)
    (loop (i32.const 1) (br 1 (i32.const 4)) (i32.const 2))
  )
  (func "as-loop-last" (result i32)
    (loop (nop) (i32.const 1) (br 1 (i32.const 5)))
  )

  (func "as-br-value" (result i32)
    (block (br 0 (br 0 (i32.const 9))))
  )

  (func "as-br_if-cond"
    (block (br_if 0 (br 0)))
  )
  (func "as-br_if-value" (result i32)
    (block (br_if 0 (br 0 (i32.const 8)) (i32.const 1)) (i32.const 7))
  )
  (func "as-br_if-value-cond" (result i32)
    (block (br_if 0 (i32.const 6) (br 0 (i32.const 9))) (i32.const 7))
  )

  (func "as-br_table-index"
    (block (br_table 0 0 0 (br 0)))
  )
  (func "as-br_table-value" (result i32)
    (block (br_table 0 0 0 (br 0 (i32.const 10)) (i32.const 1)) (i32.const 7))
  )
  (func "as-br_table-value-index" (result i32)
    (block (br_table 0 0 (i32.const 6) (br 0 (i32.const 11))) (i32.const 7))
  )

  (func "as-return-value" (result i64)
    (block (return (br 0 (i64.const 7))))
  )

  (func "as-if-cond" (result i32)
    (block (if (br 0 (i32.const 2)) (i32.const 0) (i32.const 1)))
  )
  (func "as-if-then" (param i32 i32) (result i32)
    (block (if (get_local 0) (br 1 (i32.const 3)) (get_local 1)))
  )
  (func "as-if-else" (param i32 i32) (result i32)
    (block (if (get_local 0) (get_local 1) (br 1 (i32.const 4))))
  )

  (func "as-select-first" (param i32 i32) (result i32)
    (block (select (br 0 (i32.const 5)) (get_local 0) (get_local 1)))
  )
  (func "as-select-second" (param i32 i32) (result i32)
    (block (select (get_local 0) (br 0 (i32.const 6)) (get_local 1)))
  )
  (func "as-select-cond" (result i32)
    (block (select (i32.const 0) (i32.const 1) (br 0 (i32.const 7))))
  )

  (func $f (param i32 i32 i32) (result i32) (i32.const -1))
  (func "as-call-first" (result i32)
    (block (call $f (br 0 (i32.const 12)) (i32.const 2) (i32.const 3)))
  )
  (func "as-call-mid" (result i32)
    (block (call $f (i32.const 1) (br 0 (i32.const 13)) (i32.const 3)))
  )
  (func "as-call-last" (result i32)
    (block (call $f (i32.const 1) (i32.const 2) (br 0 (i32.const 14))))
  )

  (import "spectest" "print" (param i32 i32 i32))
  (func "as-call_import-first"
    (block (call_import 0 (br 0) (i32.const 2) (i32.const 3)))
  )
  (func "as-call_import-mid"
    (block (call_import 0 (i32.const 1) (br 0) (i32.const 3)))
  )
  (func "as-call_import-last"
    (block (call_import 0 (i32.const 1) (i32.const 2) (br 0)))
  )

  (type $sig (func (param i32 i32 i32) (result i32)))
  (table $f)
  (func "as-call_indirect-func" (result i32)
    (block (call_indirect $sig (br 0 (i32.const 20)) (i32.const 1) (i32.const 2) (i32.const 3)))
  )
  (func "as-call_indirect-first" (result i32)
    (block (call_indirect $sig (i32.const 0) (br 0 (i32.const 21)) (i32.const 2) (i32.const 3)))
  )
  (func "as-call_indirect-mid" (result i32)
    (block (call_indirect $sig (i32.const 0) (i32.const 1) (br 0 (i32.const 22)) (i32.const 3)))
  )
  (func "as-call_indirect-last" (result i32)
    (block (call_indirect $sig (i32.const 0) (i32.const 1) (i32.const 2) (br 0 (i32.const 23))))
  )

  (func "as-set_local-value" (result i32) (local f32)
    (block (set_local 0 (br 0 (i32.const 17))) (i32.const -1))
  )

  (memory 1)
  (func "as-load-address" (result f32)
    (block (f32.load (br 0 (f32.const 1.7))))
  )
  (func "as-loadN-address" (result i64)
    (block (i64.load8_s (br 0 (i64.const 30))))
  )

  (func "as-store-address" (result i32)
    (block (f64.store (br 0 (i32.const 30)) (f64.const 7)) (i32.const -1))
  )
  (func "as-store-value" (result i32)
    (block (i64.store (i32.const 2) (br 0 (i32.const 31))) (i32.const -1))
  )

  (func "as-storeN-address" (result i32)
    (block (i32.store8 (br 0 (i32.const 32)) (i32.const 7)) (i32.const -1))
  )
  (func "as-storeN-value" (result i32)
    (block (i64.store16 (i32.const 2) (br 0 (i32.const 33))) (i32.const -1))
  )

  (func "as-unary-operand" (result f32)
    (block (f32.neg (br 0 (f32.const 3.4))))
  )

  (func "as-binary-left" (result i32)
    (block (i32.add (br 0 (i32.const 3)) (i32.const 10)))
  )
  (func "as-binary-right" (result i64)
    (block (i64.sub (i64.const 10) (br 0 (i64.const 45))))
  )

  (func "as-test-operand" (result i32)
    (block (i32.eqz (br 0 (i32.const 44))))
  )

  (func "as-compare-left" (result i32)
    (block (f64.le (br 0 (i32.const 43)) (f64.const 10)))
  )
  (func "as-compare-right" (result i32)
    (block (f32.ne (f32.const 10) (br 0 (i32.const 42))))
  )

  (func "as-convert-operand" (result i32)
    (block (i32.wrap/i64 (br 0 (i32.const 41))))
  )

  (func "as-grow_memory-size" (result i32)
    (block (grow_memory (br 0 (i32.const 40))))
  )

  (func "nested-block-value" (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (i32.add (i32.const 4) (br 0 (i32.const 8)))
      )
    )
  )

  (func "nested-br-value" (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (block
          (i32.const 4)
          (br 0 (br 1 (i32.const 8)))
        )
        (i32.const 16)
      )
    )
  )

  (func "nested-br_if-value" (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (block
          (i32.const 4)
          (br_if 0 (br 1 (i32.const 8)) (i32.const 1))
        )
        (i32.const 16)
      )
    )
  )

  (func "nested-br_if-value-cond" (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (br_if 0 (i32.const 4) (br 0 (i32.const 8)))
        (i32.const 16)
      )
    )
  )

  (func "nested-br_table-value" (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (block
          (i32.const 4)
          (br_table 0 (br 1 (i32.const 8)) (i32.const 1))
        )
        (i32.const 16)
      )
    )
  )

  (func "nested-br_table-value-index" (result i32)
    (i32.add
      (i32.const 1)
      (block
        (i32.const 2)
        (br_table 0 (i32.const 4) (br 0 (i32.const 8)))
        (i32.const 16)
      )
    )
  )
)

(assert_return (invoke "type-i32"))
(assert_return (invoke "type-i64"))
(assert_return (invoke "type-f32"))
(assert_return (invoke "type-f64"))

(assert_return (invoke "type-i32-value") (i32.const 1))
(assert_return (invoke "type-i64-value") (i64.const 2))
(assert_return (invoke "type-f32-value") (f32.const 3))
(assert_return (invoke "type-f64-value") (f64.const 4))

(assert_return (invoke "as-block-first"))
(assert_return (invoke "as-block-mid"))
(assert_return (invoke "as-block-last"))
(assert_return (invoke "as-block-value") (i32.const 2))

(assert_return (invoke "as-loop-first") (i32.const 3))
(assert_return (invoke "as-loop-mid") (i32.const 4))
(assert_return (invoke "as-loop-last") (i32.const 5))

(assert_return (invoke "as-br-value") (i32.const 9))

(assert_return (invoke "as-br_if-cond"))
(assert_return (invoke "as-br_if-value") (i32.const 8))
(assert_return (invoke "as-br_if-value-cond") (i32.const 9))

(assert_return (invoke "as-br_table-index"))
(assert_return (invoke "as-br_table-value") (i32.const 10))
(assert_return (invoke "as-br_table-value-index") (i32.const 11))

(assert_return (invoke "as-return-value") (i64.const 7))

(assert_return (invoke "as-if-cond") (i32.const 2))
(assert_return (invoke "as-if-then" (i32.const 1) (i32.const 6)) (i32.const 3))
(assert_return (invoke "as-if-then" (i32.const 0) (i32.const 6)) (i32.const 6))
(assert_return (invoke "as-if-else" (i32.const 0) (i32.const 6)) (i32.const 4))
(assert_return (invoke "as-if-else" (i32.const 1) (i32.const 6)) (i32.const 6))

(assert_return (invoke "as-select-first" (i32.const 0) (i32.const 6)) (i32.const 5))
(assert_return (invoke "as-select-first" (i32.const 1) (i32.const 6)) (i32.const 5))
(assert_return (invoke "as-select-second" (i32.const 0) (i32.const 6)) (i32.const 6))
(assert_return (invoke "as-select-second" (i32.const 1) (i32.const 6)) (i32.const 6))
(assert_return (invoke "as-select-cond") (i32.const 7))

(assert_return (invoke "as-call-first") (i32.const 12))
(assert_return (invoke "as-call-mid") (i32.const 13))
(assert_return (invoke "as-call-last") (i32.const 14))

(assert_return (invoke "as-call_import-first"))
(assert_return (invoke "as-call_import-mid"))
(assert_return (invoke "as-call_import-last"))

(assert_return (invoke "as-call_indirect-func") (i32.const 20))
(assert_return (invoke "as-call_indirect-first") (i32.const 21))
(assert_return (invoke "as-call_indirect-mid") (i32.const 22))
(assert_return (invoke "as-call_indirect-last") (i32.const 23))

(assert_return (invoke "as-set_local-value") (i32.const 17))

(assert_return (invoke "as-load-address") (f32.const 1.7))
(assert_return (invoke "as-loadN-address") (i64.const 30))

(assert_return (invoke "as-store-address") (i32.const 30))
(assert_return (invoke "as-store-value") (i32.const 31))
(assert_return (invoke "as-storeN-address") (i32.const 32))
(assert_return (invoke "as-storeN-value") (i32.const 33))

(assert_return (invoke "as-unary-operand") (f32.const 3.4))

(assert_return (invoke "as-binary-left") (i32.const 3))
(assert_return (invoke "as-binary-right") (i64.const 45))

(assert_return (invoke "as-test-operand") (i32.const 44))

(assert_return (invoke "as-compare-left") (i32.const 43))
(assert_return (invoke "as-compare-right") (i32.const 42))

(assert_return (invoke "as-convert-operand") (i32.const 41))

(assert_return (invoke "as-grow_memory-size") (i32.const 40))

(assert_return (invoke "nested-block-value") (i32.const 9))
(assert_return (invoke "nested-br-value") (i32.const 9))
(assert_return (invoke "nested-br_if-value") (i32.const 9))
(assert_return (invoke "nested-br_if-value-cond") (i32.const 9))
(assert_return (invoke "nested-br_table-value") (i32.const 9))
(assert_return (invoke "nested-br_table-value-index") (i32.const 9))

(assert_invalid
  (module (func $type-arg-void-vs-num (result i32)
    (block (br 0) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-arg-num-vs-num (result i32)
    (block (br 0 (i64.const 1)) (i32.const 1))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $unbound-label (br 1)))
  "unknown label"
)
(assert_invalid
  (module (func $unbound-nested-label (block (block (br 5)))))
  "unknown label"
)
(assert_invalid
  (module (func $large-label (br 0x100000001)))
  "unknown label"
)
