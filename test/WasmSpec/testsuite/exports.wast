(module (func (i32.const 1)) (export "a" 0))
(module (func (i32.const 1)) (export "a" 0) (export "b" 0))
(module (func (i32.const 1)) (func (i32.const 2)) (export "a" 0) (export "b" 1))
(assert_invalid
  (module (func (i32.const 1)) (export "a" 1))
  "unknown function 1")
(assert_invalid
  (module (func (i32.const 1)) (func (i32.const 2)) (export "a" 0) (export "a" 1))
  "duplicate export name")
(assert_invalid
  (module (func (i32.const 1)) (export "a" 0) (export "a" 0))
  "duplicate export name")

(module
  (func $f (param $n i32) (result i32)
    (return (i32.add (get_local $n) (i32.const 1)))
  )

  (export "e" $f)
)

(assert_return (invoke "e" (i32.const 42)) (i32.const 43))

(module (memory 0 0) (export "a" memory))
(module (memory 0 0) (export "a" memory) (export "b" memory))
(assert_invalid (module (export "a" memory)) "no memory to export")
