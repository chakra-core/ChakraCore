;; Test `block` opcode

(module
  (func "empty"
    (block)
    (block $l)
  )

  (func "singular" (result i32)
    (block (nop))
    (block (i32.const 7))
  )

  (func $nop)
  (func "multi" (result i32)
    (block (call $nop) (call $nop) (call $nop) (call $nop))
    (block (call $nop) (call $nop) (call $nop) (i32.const 8))
  )

  (func "nested" (result i32)
    (block (block (call $nop) (block) (nop)) (block (call $nop) (i32.const 9)))
  )

  (func "deep" (result i32)
    (block (block (block (block (block (block (block (block (block (block
      (block (block (block (block (block (block (block (block (block (block
        (block (block (block (block (block (block (block (block (block (block
          (block (block (block (block (block (block (block (block (block (block
            (block (block (block (block (block (call $nop) (i32.const 150))))))
          ))))))))))
        ))))))))))
      ))))))))))
    ))))))))))
  )

  (func "as-unary-operand" (result i32)
    (i32.ctz (block (call $nop) (i32.const 13)))
  )
  (func "as-binary-operand" (result i32)
    (i32.mul (block (call $nop) (i32.const 3)) (block (call $nop) (i32.const 4)))
  )
  (func "as-test-operand" (result i32)
    (i32.eqz (block (call $nop) (i32.const 13)))
  )
  (func "as-compare-operand" (result i32)
    (f32.gt (block (call $nop) (f32.const 3)) (block (call $nop) (f32.const 3)))
  )

  (func "break-bare" (result i32)
    (block (br 0) (unreachable))
    (block (br_if 0 (i32.const 1)) (unreachable))
    (block (br_table 0 (i32.const 0)) (unreachable))
    (block (br_table 0 0 0 (i32.const 1)) (unreachable))
    (i32.const 19)
  )
  (func "break-value" (result i32)
    (block (br 0 (i32.const 18)) (i32.const 19))
  )
  (func "break-repeated" (result i32)
    (block
      (br 0 (i32.const 18))
      (br 0 (i32.const 19))
      (br_if 0 (i32.const 20) (i32.const 0))
      (br_if 0 (i32.const 20) (i32.const 1))
      (br 0 (i32.const 21))
      (br_table 0 (i32.const 22) (i32.const 4))
      (br_table 0 0 0 (i32.const 23) (i32.const 1))
      (i32.const 21)
    )
  )
  (func "break-inner" (result i32)
    (local i32)
    (set_local 0 (i32.const 0))
    (set_local 0 (i32.add (get_local 0) (block (block (br 1 (i32.const 0x1))))))
    (set_local 0 (i32.add (get_local 0) (block (block (br 0)) (i32.const 0x2))))
    (set_local 0 (i32.add (get_local 0) (block (i32.ctz (br 0 (i32.const 0x4))))))
    (set_local 0 (i32.add (get_local 0) (block (i32.ctz (block (br 1 (i32.const 0x8)))))))
    (get_local 0)
  )

  (func "drop-mid" (result i32)
    (block (call $fx) (i32.const 7) (call $nop) (i32.const 8))
  )
  (func "drop-last"
    (block (call $nop) (call $fx) (nop) (i32.const 8))
  )
  (func "drop-break-void"
    (block (br 0 (nop)))
    (block (br 0 (call $nop)))
    (block (br_if 0 (nop) (i32.const 0)))
    (block (br_if 0 (nop) (i32.const 1)))
    (block (br_if 0 (call $nop) (i32.const 0)))
    (block (br_if 0 (call $nop) (i32.const 1)))
    (block (br_table 0 (nop) (i32.const 3)))
    (block (br_table 0 0 0 (nop) (i32.const 2)))
  )
  (func "drop-break-value"
    (block (br 0 (i32.const 12)))
    (block (br_if 0 (i32.const 11) (i32.const 0)))
    (block (br_if 0 (i32.const 10) (i32.const 1)))
    (block (br_table 0 (i32.const 9) (i32.const 5)))
    (block (br_table 0 0 0 (i32.const 8) (i32.const 1)))
  )
  (func "drop-break-value-heterogeneous"
    (block (br 0 (i32.const 8)) (br 0 (f64.const 8)) (br 0 (f32.const 8)))
    (block (br 0 (i32.const 8)) (br 0) (br 0 (f64.const 8)))
    (block (br 0 (i32.const 8)) (br 0 (call $nop)) (br 0 (f64.const 8)))
    (block (br 0 (i32.const 8)) (br 0) (br 0 (f32.const 8)) (i64.const 3))
    (block (br 0) (br 0 (i32.const 8)) (br 0 (f64.const 8)) (br 0 (nop)))
    (block (br 0) (br 0 (i32.const 8)) (br 0 (f32.const 8)) (i64.const 3))
    (block (block (br 0) (br 1 (i32.const 8))) (br 0 (f32.const 8)) (i64.const 3))
  )

  (func "effects" $fx (result i32)
    (local i32)
    (block
      (set_local 0 (i32.const 1))
      (set_local 0 (i32.mul (get_local 0) (i32.const 3)))
      (set_local 0 (i32.sub (get_local 0) (i32.const 5)))
      (set_local 0 (i32.mul (get_local 0) (i32.const 7)))
      (br 0)
      (set_local 0 (i32.mul (get_local 0) (i32.const 100)))
    )
    (i32.eq (get_local 0) (i32.const -14))
  )
)

(assert_return (invoke "empty"))
(assert_return (invoke "singular") (i32.const 7))
(assert_return (invoke "multi") (i32.const 8))
(assert_return (invoke "nested") (i32.const 9))
(assert_return (invoke "deep") (i32.const 150))

(assert_return (invoke "as-unary-operand") (i32.const 0))
(assert_return (invoke "as-binary-operand") (i32.const 12))
(assert_return (invoke "as-test-operand") (i32.const 0))
(assert_return (invoke "as-compare-operand") (i32.const 0))

(assert_return (invoke "break-bare") (i32.const 19))
(assert_return (invoke "break-value") (i32.const 18))
(assert_return (invoke "break-repeated") (i32.const 18))
(assert_return (invoke "break-inner") (i32.const 0xf))

(assert_return (invoke "drop-mid") (i32.const 8))
(assert_return (invoke "drop-last"))
(assert_return (invoke "drop-break-void"))
(assert_return (invoke "drop-break-value"))
(assert_return (invoke "drop-break-value-heterogeneous"))

(assert_return (invoke "effects") (i32.const 1))

(assert_invalid
  (module (func $type-empty-i32 (result i32) (block)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-empty-i64 (result i64) (block)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-empty-f32 (result f32) (block)))
  "type mismatch"
)
(assert_invalid
  (module (func $type-empty-f64 (result f64) (block)))
  "type mismatch"
)

(assert_invalid
  (module (func $type-value-void-vs-num (result i32)
    (block (nop))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-value-num-vs-num (result i32)
    (block (f32.const 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-value-void-vs-num-after-break (result i32)
    (block (br 0 (i32.const 1)) (nop))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-value-num-vs-num-after-break (result i32)
    (block (br 0 (i32.const 1)) (f32.const 0))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-break-last-void-vs-num (result i32)
    (block (br 0))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-break-void-vs-num (result i32)
    (block (br 0) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-break-num-vs-num (result i32)
    (block (br 0 (i64.const 1)) (i32.const 1))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-break-first-num-vs-num (result i32)
    (block (br 0 (i64.const 1)) (br 0 (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-break-second-num-vs-num (result i32)
    (block (br 0 (i32.const 1)) (br 0 (f64.const 1)))
  ))
  "type mismatch"
)

(assert_invalid
  (module (func $type-break-nested-void-vs-num (result i32)
    (block (block (br 1)) (br 0 (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-break-nested-num-vs-num (result i32)
    (block (block (br 1 (i64.const 1))) (br 0 (i32.const 1)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-break-operand-void-vs-num (result i32)
    (i32.ctz (block (br 0)))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func $type-break-operand-num-vs-num (result i32)
    (i64.ctz (block (br 0 (i64.const 9))))
  ))
  "type mismatch"
)
(assert_invalid
  (module (func (result i32) (block (br 0))))
  "type mismatch"
)
(assert_invalid
  (module (func (result i32) (i32.ctz (block (br 0)))))
  "type mismatch"
)

