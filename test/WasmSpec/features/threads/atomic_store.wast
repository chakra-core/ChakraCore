(module
  (memory 1 1 shared)

  (func (export "i32.atomic.store8") (result i32)
      i32.const 0 i32.const 0xfb i32.atomic.store8
      i32.const 1 i32.const 0xfc i32.atomic.store8
      i32.const 2 i32.const 0xfd i32.atomic.store8
      i32.const 3 i32.const 0xfe i32.atomic.store8
      i32.const 0 i32.load)

  (func (export "i32.atomic.store16") (result i32)
      i32.const 0 i32.const 0xcac9 i32.atomic.store16
      i32.const 2 i32.const 0xcccb i32.atomic.store16
      i32.const 0 i32.load)

  (func (export "i32.atomic.store") (result i32)
      i32.const 0 i32.const -123456 i32.atomic.store
      i32.const 0 i32.load)

  (func (export "i64.atomic.store8") (result i32)
      i32.const 0 i64.const 0xeeeeeeeeeeeeeefb i64.atomic.store8
      i32.const 1 i64.const 0xeeeeeeeeeeeeeefc i64.atomic.store8
      i32.const 2 i64.const 0xeeeeeeeeeeeeeefd i64.atomic.store8
      i32.const 3 i64.const 0xeeeeeeeeeeeeeefe i64.atomic.store8
      i32.const 0 i32.load)

  (func (export "i64.atomic.store16") (result i32)
      i32.const 0 i64.const 0xeeeeeeeeeeeecac9 i64.atomic.store16
      i32.const 2 i64.const 0xeeeeeeeeeeeecccb i64.atomic.store16
      i32.const 0 i32.load)

  (func (export "i64.atomic.store32") (result i32)
      i32.const 0 i64.const -123456 i64.atomic.store32
      i32.const 0 i32.load)

  (func (export "i64.atomic.store") (result i64)
      i32.const 0 i64.const 0xbaddc0de600dd00d i64.atomic.store
      i32.const 0 i64.load)

  ;; Test bad alignment

  (func (export "bad.align-i32.atomic.store16")
      i32.const 1 i32.const 0 i32.atomic.store16)
  (func (export "bad.align-i32.atomic.store")
      i32.const 2 i32.const 0 i32.atomic.store)

  (func (export "bad.align-i64.atomic.store16")
      i32.const 1 i64.const 0 i64.atomic.store16)
  (func (export "bad.align-i64.atomic.store32")
      i32.const 2 i64.const 0 i64.atomic.store32)
  (func (export "bad.align-i64.atomic.store")
      i32.const 4 i64.const 0 i64.atomic.store)

)

(assert_return (invoke "i32.atomic.store8") (i32.const 4278058235))
(assert_return (invoke "i32.atomic.store16") (i32.const 3435907785))
(assert_return (invoke "i32.atomic.store") (i32.const 4294843840))
(assert_return (invoke "i64.atomic.store8") (i32.const 4278058235))
(assert_return (invoke "i64.atomic.store16") (i32.const 3435907785))
(assert_return (invoke "i64.atomic.store32") (i32.const 4294843840))
(assert_return (invoke "i64.atomic.store") (i64.const 13465130522234441741))
(assert_trap (invoke "bad.align-i32.atomic.store16") "atomic memory access is unaligned")
(assert_trap (invoke "bad.align-i32.atomic.store") "atomic memory access is unaligned")
(assert_trap (invoke "bad.align-i64.atomic.store16") "atomic memory access is unaligned")
(assert_trap (invoke "bad.align-i64.atomic.store32") "atomic memory access is unaligned")
(assert_trap (invoke "bad.align-i64.atomic.store") "atomic memory access is unaligned")
