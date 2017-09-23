;; i32 operations

(module
  (func (export "extend8_s") (param $x i32) (result i32) (i32.extend8_s (get_local $x)))
  (func (export "extend16_s") (param $x i32) (result i32) (i32.extend16_s (get_local $x)))
)

(assert_return (invoke "extend8_s" (i32.const 0)) (i32.const 0))
(assert_return (invoke "extend8_s" (i32.const 0x7f)) (i32.const 127))
(assert_return (invoke "extend8_s" (i32.const 0x80)) (i32.const -128))
(assert_return (invoke "extend8_s" (i32.const 0xff)) (i32.const -1))
(assert_return (invoke "extend8_s" (i32.const 0x012345_00)) (i32.const 0))
(assert_return (invoke "extend8_s" (i32.const 0xfedcba_80)) (i32.const -0x80))
(assert_return (invoke "extend8_s" (i32.const -1)) (i32.const -1))

(assert_return (invoke "extend16_s" (i32.const 0)) (i32.const 0))
(assert_return (invoke "extend16_s" (i32.const 0x7fff)) (i32.const 32767))
(assert_return (invoke "extend16_s" (i32.const 0x8000)) (i32.const -32768))
(assert_return (invoke "extend16_s" (i32.const 0xffff)) (i32.const -1))
(assert_return (invoke "extend16_s" (i32.const 0x0123_0000)) (i32.const 0))
(assert_return (invoke "extend16_s" (i32.const 0xfedc_8000)) (i32.const -0x8000))
(assert_return (invoke "extend16_s" (i32.const -1)) (i32.const -1))
