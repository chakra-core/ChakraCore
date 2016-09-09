;; Test that optimizers don't do redundant-load, store-to-load, or dead-store
;; optimizations when there are interfering stores, even of different types
;; and to non-identical addresses.

(module
  (memory 1 1)

  (export "zero_everything" $zero_everything)
  (func $zero_everything
    (i32.store (i32.const 0) (i32.const 0))
    (i32.store (i32.const 4) (i32.const 0))
    (i32.store (i32.const 8) (i32.const 0))
    (i32.store (i32.const 12) (i32.const 0))
  )

  (export "test_store_to_load" $test_store_to_load)
  (func $test_store_to_load (result i32)
    (i32.store (i32.const 8) (i32.const 0))
    (f32.store (i32.const 5) (f32.const -0.0))
    (i32.load (i32.const 8))
  )

  (export "test_redundant_load" $test_redundant_load)
  (func $test_redundant_load (result i32)
    (i32.load (i32.const 8))
    (f32.store (i32.const 5) (f32.const -0.0))
    (i32.load (i32.const 8))
  )

  (export "test_dead_store" $test_dead_store)
  (func $test_dead_store (result f32)
    (local $t f32)
    (i32.store (i32.const 8) (i32.const 0x23232323))
    (set_local $t (f32.load (i32.const 11)))
    (i32.store (i32.const 8) (i32.const 0))
    (get_local $t)
  )
)

(assert_return (invoke "test_store_to_load") (i32.const 0x00000080))
(invoke "zero_everything")
(assert_return (invoke "test_redundant_load") (i32.const 0x00000080))
(invoke "zero_everything")
(assert_return (invoke "test_dead_store") (f32.const 0x1.18p-144))
