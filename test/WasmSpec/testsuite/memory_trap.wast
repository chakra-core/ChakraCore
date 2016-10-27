(module
    (memory 1)

    (func $addr_limit (result i32)
      (i32.mul (current_memory) (i32.const 0x10000))
    )

    (func (export "store") (param $i i32) (param $v i32)
      (i32.store (i32.add (call $addr_limit) (get_local $i)) (get_local $v))
    )

    (func (export "load") (param $i i32) (result i32)
      (i32.load (i32.add (call $addr_limit) (get_local $i)))
    )

    (func (export "grow_memory") (param i32) (result i32)
      (grow_memory (get_local 0))
    )
)

(assert_return (invoke "store" (i32.const -4) (i32.const 42)))
(assert_return (invoke "load" (i32.const -4)) (i32.const 42))
(assert_trap (invoke "store" (i32.const -3) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i32.const -3)) "out of bounds memory access")
(assert_trap (invoke "store" (i32.const -2) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i32.const -2)) "out of bounds memory access")
(assert_trap (invoke "store" (i32.const -1) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i32.const -1)) "out of bounds memory access")
(assert_trap (invoke "store" (i32.const 0) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "store" (i32.const 0x80000000) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i32.const 0x80000000)) "out of bounds memory access")
(assert_return (invoke "grow_memory" (i32.const 0x10001)) (i32.const -1))
