;; Test `tee_local` operator

(module
  ;; Typing

  (func (export "type-local-i32") (result i32) (local i32) (tee_local 0 (i32.const 0)))
  (func (export "type-local-i64") (result i64) (local i64) (tee_local 0 (i64.const 0)))
  (func (export "type-local-f32") (result f32) (local f32) (tee_local 0 (f32.const 0)))
  (func (export "type-local-f64") (result f64) (local f64) (tee_local 0 (f64.const 0)))

  (func (export "type-param-i32") (param i32) (result i32) (tee_local 0 (i32.const 10)))
  (func (export "type-param-i64") (param i64) (result i64) (tee_local 0 (i64.const 11)))
  (func (export "type-param-f32") (param f32) (result f32) (tee_local 0 (f32.const 11.1)))
  (func (export "type-param-f64") (param f64) (result f64) (tee_local 0 (f64.const 12.2)))

  (func (export "type-mixed") (param i64 f32 f64 i32 i32) (local f32 i64 i64 f64)
    (drop (i64.eqz (tee_local 0 (i64.const 0))))
    (drop (f32.neg (tee_local 1 (f32.const 0))))
    (drop (f64.neg (tee_local 2 (f64.const 0))))
    (drop (i32.eqz (tee_local 3 (i32.const 0))))
    (drop (i32.eqz (tee_local 4 (i32.const 0))))
    (drop (f32.neg (tee_local 5 (f32.const 0))))
    (drop (i64.eqz (tee_local 6 (i64.const 0))))
    (drop (i64.eqz (tee_local 7 (i64.const 0))))
    (drop (f64.neg (tee_local 8 (f64.const 0))))
  )

  ;; Writing

  (func (export "write") (param i64 f32 f64 i32 i32) (result i64) (local f32 i64 i64 f64)
    (drop (tee_local 1 (f32.const -0.3)))
    (drop (tee_local 3 (i32.const 40)))
    (drop (tee_local 4 (i32.const -7)))
    (drop (tee_local 5 (f32.const 5.5)))
    (drop (tee_local 6 (i64.const 6)))
    (drop (tee_local 8 (f64.const 8)))
    (i64.trunc_s/f64
      (f64.add
        (f64.convert_u/i64 (get_local 0))
        (f64.add
          (f64.promote/f32 (get_local 1))
          (f64.add
            (get_local 2)
            (f64.add
              (f64.convert_u/i32 (get_local 3))
              (f64.add
                (f64.convert_s/i32 (get_local 4))
                (f64.add
                  (f64.promote/f32 (get_local 5))
                  (f64.add
                    (f64.convert_u/i64 (get_local 6))
                    (f64.add
                      (f64.convert_u/i64 (get_local 7))
                      (get_local 8)
                    )
                  )
                )
              )
            )
          )
        )
      )
    )
  )

  ;; Result

  (func (export "result") (param i64 f32 f64 i32 i32) (result f64)
    (local f32 i64 i64 f64)
    (f64.add
      (f64.convert_u/i64 (tee_local 0 (i64.const 1)))
      (f64.add
        (f64.promote/f32 (tee_local 1 (f32.const 2)))
        (f64.add
          (tee_local 2 (f64.const 3.3))
          (f64.add
            (f64.convert_u/i32 (tee_local 3 (i32.const 4)))
            (f64.add
              (f64.convert_s/i32 (tee_local 4 (i32.const 5)))
              (f64.add
                (f64.promote/f32 (tee_local 5 (f32.const 5.5)))
                (f64.add
                  (f64.convert_u/i64 (tee_local 6 (i64.const 6)))
                  (f64.add
                    (f64.convert_u/i64 (tee_local 7 (i64.const 0)))
                    (tee_local 8 (f64.const 8))
                  )
                )
              )
            )
          )
        )
      )
    )
  )

  (func $dummy)

  (func (export "as-block-first") (param i32) (result i32)
    (block (result i32) (tee_local 0 (i32.const 1)) (call $dummy))
  )
  (func (export "as-block-mid") (param i32) (result i32)
    (block (result i32) (call $dummy) (tee_local 0 (i32.const 1)) (call $dummy))
  )
  (func (export "as-block-last") (param i32) (result i32)
    (block (result i32) (call $dummy) (call $dummy) (tee_local 0 (i32.const 1)))
  )

  (func (export "as-loop-first") (param i32) (result i32)
    (loop (result i32) (tee_local 0 (i32.const 3)) (call $dummy))
  )
  (func (export "as-loop-mid") (param i32) (result i32)
    (loop (result i32) (call $dummy) (tee_local 0 (i32.const 4)) (call $dummy))
  )
  (func (export "as-loop-last") (param i32) (result i32)
    (loop (result i32) (call $dummy) (call $dummy) (tee_local 0 (i32.const 5)))
  )

  (func (export "as-br-value") (param i32) (result i32)
    (block (result i32) (br 0 (tee_local 0 (i32.const 9))))
  )

  (func (export "as-br_if-cond") (param i32)
    (block (br_if 0 (tee_local 0 (i32.const 1))))
  )
  (func (export "as-br_if-value") (param i32) (result i32)
    (block (result i32)
      (drop (br_if 0 (tee_local 0 (i32.const 8)) (i32.const 1))) (i32.const 7)
    )
  )
  (func (export "as-br_if-value-cond") (param i32) (result i32)
    (block (result i32)
      (drop (br_if 0 (i32.const 6) (tee_local 0 (i32.const 9)))) (i32.const 7)
    )
  )

  (func (export "as-br_table-index") (param i32)
    (block (br_table 0 0 0 (tee_local 0 (i32.const 0))))
  )
  (func (export "as-br_table-value") (param i32) (result i32)
    (block (result i32)
      (br_table 0 0 0 (tee_local 0 (i32.const 10)) (i32.const 1)) (i32.const 7)
    )
  )
  (func (export "as-br_table-value-index") (param i32) (result i32)
    (block (result i32)
      (br_table 0 0 (i32.const 6) (tee_local 0 (i32.const 11))) (i32.const 7)
    )
  )

  (func (export "as-return-value") (param i32) (result i32)
    (return (tee_local 0 (i32.const 7)))
  )

  (func (export "as-if-cond") (param i32) (result i32)
    (if (result i32) (tee_local 0 (i32.const 2))
      (then (i32.const 0)) (else (i32.const 1))
    )
  )
  (func (export "as-if-then") (param i32) (result i32)
    (if (result i32) (get_local 0)
      (then (tee_local 0 (i32.const 3))) (else (get_local 0))
    )
  )
  (func (export "as-if-else") (param i32) (result i32)
    (if (result i32) (get_local 0)
      (then (get_local 0)) (else (tee_local 0 (i32.const 4)))
    )
  )

  (func (export "as-select-first") (param i32 i32) (result i32)
    (select (tee_local 0 (i32.const 5)) (get_local 0) (get_local 1))
  )
  (func (export "as-select-second") (param i32 i32) (result i32)
    (select (get_local 0) (tee_local 0 (i32.const 6)) (get_local 1))
  )
  (func (export "as-select-cond") (param i32) (result i32)
    (select (i32.const 0) (i32.const 1) (tee_local 0 (i32.const 7)))
  )

  (func $f (param i32 i32 i32) (result i32) (i32.const -1))
  (func (export "as-call-first") (param i32) (result i32)
    (call $f (tee_local 0 (i32.const 12)) (i32.const 2) (i32.const 3))
  )
  (func (export "as-call-mid") (param i32) (result i32)
    (call $f (i32.const 1) (tee_local 0 (i32.const 13)) (i32.const 3))
  )
  (func (export "as-call-last") (param i32) (result i32)
    (call $f (i32.const 1) (i32.const 2) (tee_local 0 (i32.const 14)))
  )

  (type $sig (func (param i32 i32 i32) (result i32)))
  (table anyfunc (elem $f))
  (func (export "as-call_indirect-first") (param i32) (result i32)
    (call_indirect (type $sig)
      (tee_local 0 (i32.const 1)) (i32.const 2) (i32.const 3) (i32.const 0)
    )
  )
  (func (export "as-call_indirect-mid") (param i32) (result i32)
    (call_indirect (type $sig)
      (i32.const 1) (tee_local 0 (i32.const 2)) (i32.const 3) (i32.const 0)
    )
  )
  (func (export "as-call_indirect-last") (param i32) (result i32)
    (call_indirect (type $sig)
      (i32.const 1) (i32.const 2) (tee_local 0 (i32.const 3)) (i32.const 0)
    )
  )
  (func (export "as-call_indirect-index") (param i32) (result i32)
    (call_indirect (type $sig)
      (i32.const 1) (i32.const 2) (i32.const 3) (tee_local 0 (i32.const 0))
    )
  )

  (func (export "as-set_local-value") (local i32)
    (set_local 0 (tee_local 0 (i32.const 1)))
  )
  (func (export "as-tee_local-value") (param i32) (result i32)
    (tee_local 0 (tee_local 0 (i32.const 1)))
  )
  (global $g (mut i32) (i32.const 0))
  (func (export "as-set_global-value") (local i32)
    (set_global $g (tee_local 0 (i32.const 1)))
  )

  (memory 1)
  (func (export "as-load-address") (param i32) (result i32)
    (i32.load (tee_local 0 (i32.const 1)))
  )
  (func (export "as-loadN-address") (param i32) (result i32)
    (i32.load8_s (tee_local 0 (i32.const 3)))
  )

  (func (export "as-store-address") (param i32)
    (i32.store (tee_local 0 (i32.const 30)) (i32.const 7))
  )
  (func (export "as-store-value") (param i32)
    (i32.store (i32.const 2) (tee_local 0 (i32.const 1)))
  )

  (func (export "as-storeN-address") (param i32)
    (i32.store8 (tee_local 0 (i32.const 1)) (i32.const 7))
  )
  (func (export "as-storeN-value") (param i32)
    (i32.store16 (i32.const 2) (tee_local 0 (i32.const 1)))
  )

  (func (export "as-unary-operand") (param f32) (result f32)
    (f32.neg (tee_local 0 (f32.const nan:0x0f1e2)))
  )

  (func (export "as-binary-left") (param i32) (result i32)
    (i32.add (tee_local 0 (i32.const 3)) (i32.const 10))
  )
  (func (export "as-binary-right") (param i32) (result i32)
    (i32.sub (i32.const 10) (tee_local 0 (i32.const 4)))
  )

  (func (export "as-test-operand") (param i32) (result i32)
    (i32.eqz (tee_local 0 (i32.const 0)))
  )

  (func (export "as-compare-left") (param i32) (result i32)
    (i32.le_s (tee_local 0 (i32.const 43)) (i32.const 10))
  )
  (func (export "as-compare-right") (param i32) (result i32)
    (i32.ne (i32.const 10) (tee_local 0 (i32.const 42)))
  )

  (func (export "as-convert-operand") (param i64) (result i32)
    (i32.wrap/i64 (tee_local 0 (i64.const 41)))
  )

  (func (export "as-memory.grow-size") (param i32) (result i32)
    (memory.grow (tee_local 0 (i32.const 40)))
  )

)

(assert_return (invoke "type-local-i32") (i32.const 0))
(assert_return (invoke "type-local-i64") (i64.const 0))
(assert_return (invoke "type-local-f32") (f32.const 0))
(assert_return (invoke "type-local-f64") (f64.const 0))

(assert_return (invoke "type-param-i32" (i32.const 2)) (i32.const 10))
(assert_return (invoke "type-param-i64" (i64.const 3)) (i64.const 11))
(assert_return (invoke "type-param-f32" (f32.const 4.4)) (f32.const 11.1))
(assert_return (invoke "type-param-f64" (f64.const 5.5)) (f64.const 12.2))

(assert_return (invoke "as-block-first" (i32.const 0)) (i32.const 1))
(assert_return (invoke "as-block-mid" (i32.const 0)) (i32.const 1))
(assert_return (invoke "as-block-last" (i32.const 0)) (i32.const 1))

(assert_return (invoke "as-loop-first" (i32.const 0)) (i32.const 3))
(assert_return (invoke "as-loop-mid" (i32.const 0)) (i32.const 4))
(assert_return (invoke "as-loop-last" (i32.const 0)) (i32.const 5))

(assert_return (invoke "as-br-value" (i32.const 0)) (i32.const 9))

(assert_return (invoke "as-br_if-cond" (i32.const 0)))
(assert_return (invoke "as-br_if-value" (i32.const 0)) (i32.const 8))
(assert_return (invoke "as-br_if-value-cond" (i32.const 0)) (i32.const 6))

(assert_return (invoke "as-br_table-index" (i32.const 0)))
(assert_return (invoke "as-br_table-value" (i32.const 0)) (i32.const 10))
(assert_return (invoke "as-br_table-value-index" (i32.const 0)) (i32.const 6))

(assert_return (invoke "as-return-value" (i32.const 0)) (i32.const 7))

(assert_return (invoke "as-if-cond" (i32.const 0)) (i32.const 0))
(assert_return (invoke "as-if-then" (i32.const 1)) (i32.const 3))
(assert_return (invoke "as-if-else" (i32.const 0)) (i32.const 4))

(assert_return (invoke "as-select-first" (i32.const 0) (i32.const 1)) (i32.const 5))
(assert_return (invoke "as-select-second" (i32.const 0) (i32.const 0)) (i32.const 6))
(assert_return (invoke "as-select-cond" (i32.const 0)) (i32.const 0))

(assert_return (invoke "as-call-first" (i32.const 0)) (i32.const -1))
(assert_return (invoke "as-call-mid" (i32.const 0)) (i32.const -1))
(assert_return (invoke "as-call-last" (i32.const 0)) (i32.const -1))

(assert_return (invoke "as-call_indirect-first" (i32.const 0)) (i32.const -1))
(assert_return (invoke "as-call_indirect-mid" (i32.const 0)) (i32.const -1))
(assert_return (invoke "as-call_indirect-last" (i32.const 0)) (i32.const -1))
(assert_return (invoke "as-call_indirect-index" (i32.const 0)) (i32.const -1))

(assert_return (invoke "as-set_local-value"))
(assert_return (invoke "as-tee_local-value" (i32.const 0)) (i32.const 1))
(assert_return (invoke "as-set_global-value"))

(assert_return (invoke "as-load-address" (i32.const 0)) (i32.const 0))
(assert_return (invoke "as-loadN-address" (i32.const 0)) (i32.const 0))
(assert_return (invoke "as-store-address" (i32.const 0)))
(assert_return (invoke "as-store-value" (i32.const 0)))
(assert_return (invoke "as-storeN-address" (i32.const 0)))
(assert_return (invoke "as-storeN-value" (i32.const 0)))

(assert_return (invoke "as-unary-operand" (f32.const 0)) (f32.const -nan:0x0f1e2))
(assert_return (invoke "as-binary-left" (i32.const 0)) (i32.const 13))
(assert_return (invoke "as-binary-right" (i32.const 0)) (i32.const 6))
(assert_return (invoke "as-test-operand" (i32.const 0)) (i32.const 1))
(assert_return (invoke "as-compare-left" (i32.const 0)) (i32.const 0))
(assert_return (invoke "as-compare-right" (i32.const 0)) (i32.const 1))
(assert_return (invoke "as-convert-operand" (i64.const 0)) (i32.const 41))
(assert_return (invoke "as-memory.grow-size" (i32.const 0)) (i32.const 1))

(assert_return
  (invoke "type-mixed"
    (i64.const 1) (f32.const 2.2) (f64.const 3.3) (i32.const 4) (i32.const 5)
  )
)

(assert_return
  (invoke "write"
    (i64.const 1) (f32.const 2) (f64.const 3.3) (i32.const 4) (i32.const 5)
  )
  (i64.const 56)
)

(assert_return
  (invoke "result"
    (i64.const -1) (f32.const -2) (f64.const -3.3) (i32.const -4) (i32.const -5)
  )
  (f64.const 34.8)
)


;; Invalid typing of access to locals

(assert_invalid
  (module (func $type-local-num-vs-num (result i64) (local i32) (tee_local 0 (i32.const 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-local-num-vs-num (local f32) (i32.eqz (tee_local 0 (f32.const 0)))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-local-num-vs-num (local f64 i64) (f64.neg (tee_local 1 (i64.const 0)))))
  "type mismatch"
)

(assert_invalid
  (module (func $type-local-arg-void-vs-num (local i32) (tee_local 0 (nop))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-local-arg-num-vs-num (local i32) (tee_local 0 (f32.const 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-local-arg-num-vs-num (local f32) (tee_local 0 (f64.const 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-local-arg-num-vs-num (local f64 i64) (tee_local 1 (f64.const 0))))
  "type mismatch"
)


;; Invalid typing of access to parameters

(assert_invalid
  (module (func $type-param-num-vs-num (param i32) (result i64) (get_local 0)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-param-num-vs-num (param f32) (i32.eqz (get_local 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-param-num-vs-num (param f64 i64) (f64.neg (get_local 1))))
  "type mismatch"
)

(assert_invalid
  (module (func $type-param-arg-void-vs-num (param i32) (tee_local 0 (nop))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-param-arg-num-vs-num (param i32) (tee_local 0 (f32.const 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-param-arg-num-vs-num (param f32) (tee_local 0 (f64.const 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-param-arg-num-vs-num (param f64 i64) (tee_local 1 (f64.const 0))))
  "type mismatch"
)


;; Invalid local index

(assert_invalid
  (module (func $unbound-local (local i32 i64) (get_local 3)))
  "unknown local"
)
(assert_invalid
  (module (func $large-local (local i32 i64) (get_local 14324343)))
  "unknown local"
)

(assert_invalid
  (module (func $unbound-param (param i32 i64) (get_local 2)))
  "unknown local"
)
(assert_invalid
  (module (func $large-param (local i32 i64) (get_local 714324343)))
  "unknown local"
)

(assert_invalid
  (module (func $unbound-mixed (param i32) (local i32 i64) (get_local 3)))
  "unknown local"
)
(assert_invalid
  (module (func $large-mixed (param i64) (local i32 i64) (get_local 214324343)))
  "unknown local"
)

(assert_invalid
  (module (func $type-mixed-arg-num-vs-num (param f32) (local i32) (tee_local 1 (f32.const 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-mixed-arg-num-vs-num (param i64 i32) (local f32) (tee_local 1 (f32.const 0))))
  "type mismatch"
)
(assert_invalid
  (module (func $type-mixed-arg-num-vs-num (param i64) (local f64 i64) (tee_local 1 (i64.const 0))))
  "type mismatch"
)

