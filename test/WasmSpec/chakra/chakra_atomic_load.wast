(module
  ;; todo make this a shared memory
  (memory 1)
  (data (i32.const 16) "\ff\ff\ff\ff\ff\ff\ff\ff")
  (data (i32.const 24) "\12\34\56\78\00\00\ce\41")

  (func (export "load") (param i32) (result i64)
    (i64.atomic.load32_u offset=15 (get_local 0))
  )
)

(assert_return (invoke "load" (i32.const 1)) (i64.const 0xffffffff))
(assert_return (invoke "load" (i32.const 5)) (i64.const 0xffffffff))
(assert_return (invoke "load" (i32.const 9)) (i64.const 0x78563412))
(assert_return (invoke "load" (i32.const 13)) (i64.const 0x41ce0000))
(assert_trap (invoke "load" (i32.const 0)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 2)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 3)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 4)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 6)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 7)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 8)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 10)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 11)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 12)) "atomic memory access is unaligned")
(assert_trap (invoke "load" (i32.const 14)) "atomic memory access is unaligned")

(assert_return (invoke "load" (i32.const 65517)) (i64.const 0))
(assert_trap (invoke "load" (i32.const 65521)) "out of bounds memory access")
