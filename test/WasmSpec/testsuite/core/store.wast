;; Store operator as the argument of control constructs and instructions

(module
  (memory 1)

  (func (export "as-block-value")
    (block (i32.store (i32.const 0) (i32.const 1)))
  )
  (func (export "as-loop-value")
    (loop (i32.store (i32.const 0) (i32.const 1)))
  )

  (func (export "as-br-value")
    (block (br 0 (i32.store (i32.const 0) (i32.const 1))))
  )
  (func (export "as-br_if-value")
    (block
      (br_if 0 (i32.store (i32.const 0) (i32.const 1)) (i32.const 1))
    )
  )
  (func (export "as-br_if-value-cond")
    (block
      (br_if 0 (i32.const 6) (i32.store (i32.const 0) (i32.const 1)))
    )
  )
  (func (export "as-br_table-value")
    (block
      (br_table 0 (i32.store (i32.const 0) (i32.const 1)) (i32.const 1))
    )
  )

  (func (export "as-return-value")
    (return (i32.store (i32.const 0) (i32.const 1)))
  )

  (func (export "as-if-then")
    (if (i32.const 1) (then (i32.store (i32.const 0) (i32.const 1))))
  )
  (func (export "as-if-else")
    (if (i32.const 0) (then) (else (i32.store (i32.const 0) (i32.const 1))))
  )
)

(assert_return (invoke "as-block-value"))
(assert_return (invoke "as-loop-value"))

(assert_return (invoke "as-br-value"))
(assert_return (invoke "as-br_if-value"))
(assert_return (invoke "as-br_if-value-cond"))
(assert_return (invoke "as-br_table-value"))

(assert_return (invoke "as-return-value"))

(assert_return (invoke "as-if-then"))
(assert_return (invoke "as-if-else"))

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (i32.store32 (get_local 0) (i32.const 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (i32.store64 (get_local 0) (i64.const 0)))"
  )
  "unknown operator"
)

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (i64.store64 (get_local 0) (i64.const 0)))"
  )
  "unknown operator"
)

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f32.store32 (get_local 0) (f32.const 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f32.store64 (get_local 0) (f64.const 0)))"
  )
  "unknown operator"
)

(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f64.store32 (get_local 0) (f32.const 0)))"
  )
  "unknown operator"
)
(assert_malformed
  (module quote
    "(memory 1)"
    "(func (param i32) (f64.store64 (get_local 0) (f64.const 0)))"
  )
  "unknown operator"
)
;; store should have no retval

(assert_invalid
  (module (memory 1) (func (param i32) (result i32) (i32.store (i32.const 0) (i32.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param i64) (result i64) (i64.store (i32.const 0) (i64.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param f32) (result f32) (f32.store (i32.const 0) (f32.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param f64) (result f64) (f64.store (i32.const 0) (f64.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param i32) (result i32) (i32.store8 (i32.const 0) (i32.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param i32) (result i32) (i32.store16 (i32.const 0) (i32.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param i64) (result i64) (i64.store8 (i32.const 0) (i64.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param i64) (result i64) (i64.store16 (i32.const 0) (i64.const 1))))
  "type mismatch"
)
(assert_invalid
  (module (memory 1) (func (param i64) (result i64) (i64.store32 (i32.const 0) (i64.const 1))))
  "type mismatch"
)
