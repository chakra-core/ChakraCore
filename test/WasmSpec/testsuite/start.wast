(assert_invalid
  (module (func (i32.const 1)) (start 1))
  "unknown function 1"
)
(assert_invalid
  (module
    (func $main (result i32) (return (i32.const 0)))
    (start $main)
  )
  "start function must not return anything"
)
(assert_invalid
  (module
    (func $main (param $a i32))
    (start $main)
  )
  "start function must be nullary"
)
(module
  (memory 1 (segment 0 "A"))
  (func $inc
    (i32.store8
      (i32.const 0)
      (i32.add
        (i32.load8_u (i32.const 0))
        (i32.const 1)
      )
    )
  )
  (func $get (result i32)
    (return (i32.load8_u (i32.const 0)))
  )
  (func $main
    (call $inc)
    (call $inc)
    (call $inc)
  )
  (start $main)
  (export "inc" $inc)
  (export "get" $get)
)
(assert_return (invoke "get") (i32.const 68))
(invoke "inc")
(assert_return (invoke "get") (i32.const 69))
(invoke "inc")
(assert_return (invoke "get") (i32.const 70))

(module
  (memory 1 (segment 0 "A"))
  (func $inc
    (i32.store8
      (i32.const 0)
      (i32.add
        (i32.load8_u (i32.const 0))
        (i32.const 1)
      )
    )
  )
  (func $get (result i32)
    (return (i32.load8_u (i32.const 0)))
  )
  (func $main
    (call $inc)
    (call $inc)
    (call $inc)
  )
  (start 2)
  (export "inc" $inc)
  (export "get" $get)
)
(assert_return (invoke "get") (i32.const 68))
(invoke "inc")
(assert_return (invoke "get") (i32.const 69))
(invoke "inc")
(assert_return (invoke "get") (i32.const 70))

(module
 (import $print_i32 "spectest" "print" (param i32))
 (func $main
   (call_import $print_i32 (i32.const 1)))
 (start 0)
)

(module
 (import $print_i32 "spectest" "print" (param i32))
 (func $main
   (call_import $print_i32 (i32.const 2)))
 (start $main)
)
