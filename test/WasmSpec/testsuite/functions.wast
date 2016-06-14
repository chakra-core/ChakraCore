(module
  (func $empty)
  (export "empty" $empty)

  (func $result-nop (nop))
  (export "result-nop" $result-nop)

  (func $result-drop (i32.const 1))
  (export "result-drop" $result-drop)

  (func $result-block-nop (block (i32.const 1) (nop)))
  (export "result-block-nop" $result-block-nop)

  (func $result-block-drop (block (nop) (i32.const 1)))
  (export "result-block-drop" $result-block-drop)

  (func $return (return))
  (export "return" $return)

  (func $return-nop (return (nop)))
  (export "return-nop" $return-nop)

  (func $return-drop (return (i32.const 1)))
  (export "return-drop" $return-drop)

  (func $return-block-nop (return (block (i32.const 1) (nop))))
  (export "return-block-nop" $return-block-nop)

  (func $return-block-drop (return (block (nop) (i32.const 1))))
  (export "return-block-drop" $return-block-drop)
)

(assert_return (invoke "empty"))
(assert_return (invoke "result-nop"))
(assert_return (invoke "result-drop"))
(assert_return (invoke "result-block-nop"))
(assert_return (invoke "result-block-drop"))

(assert_return (invoke "return"))
(assert_return (invoke "return-nop"))
(assert_return (invoke "return-drop"))
(assert_return (invoke "return-block-nop"))
(assert_return (invoke "return-block-drop"))

