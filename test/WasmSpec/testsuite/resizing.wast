(module
    (memory 0)

    (export "load_at_zero" $load_at_zero)
    (func $load_at_zero (result i32) (i32.load (i32.const 0)))

    (export "store_at_zero" $store_at_zero)
    (func $store_at_zero (result i32) (i32.store (i32.const 0) (i32.const 2)))

    (export "load_at_page_size" $load_at_page_size)
    (func $load_at_page_size (result i32) (i32.load (i32.const 0x10000)))

    (export "store_at_page_size" $store_at_page_size)
    (func $store_at_page_size (result i32) (i32.store (i32.const 0x10000) (i32.const 3)))

    (export "grow" $grow)
    (func $grow (param $sz i32) (result i32) (grow_memory (get_local $sz)))

    (export "size" $size)
    (func $size (result i32) (current_memory))
)

(assert_return (invoke "size") (i32.const 0))
(assert_trap (invoke "store_at_zero") "out of bounds memory access")
(assert_trap (invoke "load_at_zero") "out of bounds memory access")
(assert_trap (invoke "store_at_page_size") "out of bounds memory access")
(assert_trap (invoke "load_at_page_size") "out of bounds memory access")
(assert_return (invoke "grow" (i32.const 1)) (i32.const 0))
(assert_return (invoke "size") (i32.const 1))
(assert_return (invoke "load_at_zero") (i32.const 0))
(assert_return (invoke "store_at_zero") (i32.const 2))
(assert_return (invoke "load_at_zero") (i32.const 2))
(assert_trap (invoke "store_at_page_size") "out of bounds memory access")
(assert_trap (invoke "load_at_page_size") "out of bounds memory access")
(assert_return (invoke "grow" (i32.const 4)) (i32.const 1))
(assert_return (invoke "size") (i32.const 5))
(assert_return (invoke "load_at_zero") (i32.const 2))
(assert_return (invoke "store_at_zero") (i32.const 2))
(assert_return (invoke "load_at_page_size") (i32.const 0))
(assert_return (invoke "store_at_page_size") (i32.const 3))
(assert_return (invoke "load_at_page_size") (i32.const 3))
