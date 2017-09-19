;; i64 operations

(module
  (func (export "extend8_s") (param $x i64) (result i64) (i64.extend8_s (get_local $x)))
  (func (export "extend16_s") (param $x i64) (result i64) (i64.extend16_s (get_local $x)))
  (func (export "extend32_s") (param $x i64) (result i64) (i64.extend32_s (get_local $x)))
)

(assert_return (invoke "extend8_s" (i64.const 0)) (i64.const 0))
(assert_return (invoke "extend8_s" (i64.const 0x7f)) (i64.const 127))
(assert_return (invoke "extend8_s" (i64.const 0x80)) (i64.const -128))
(assert_return (invoke "extend8_s" (i64.const 0xff)) (i64.const -1))
(assert_return (invoke "extend8_s" (i64.const 0x01234567_89abcd_00)) (i64.const 0))
(assert_return (invoke "extend8_s" (i64.const 0xfedcba98_765432_80)) (i64.const -0x80))
(assert_return (invoke "extend8_s" (i64.const -1)) (i64.const -1))

(assert_return (invoke "extend16_s" (i64.const 0)) (i64.const 0))
(assert_return (invoke "extend16_s" (i64.const 0x7fff)) (i64.const 32767))
(assert_return (invoke "extend16_s" (i64.const 0x8000)) (i64.const -32768))
(assert_return (invoke "extend16_s" (i64.const 0xffff)) (i64.const -1))
(assert_return (invoke "extend16_s" (i64.const 0x12345678_9abc_0000)) (i64.const 0))
(assert_return (invoke "extend16_s" (i64.const 0xfedcba98_7654_8000)) (i64.const -0x8000))
(assert_return (invoke "extend16_s" (i64.const -1)) (i64.const -1))

(assert_return (invoke "extend32_s" (i64.const 0)) (i64.const 0))
(assert_return (invoke "extend32_s" (i64.const 0x7fff)) (i64.const 32767))
(assert_return (invoke "extend32_s" (i64.const 0x8000)) (i64.const 32768))
(assert_return (invoke "extend32_s" (i64.const 0xffff)) (i64.const 65535))
(assert_return (invoke "extend32_s" (i64.const 0x7fffffff)) (i64.const 0x7fffffff))
(assert_return (invoke "extend32_s" (i64.const 0x80000000)) (i64.const -0x80000000))
(assert_return (invoke "extend32_s" (i64.const 0xffffffff)) (i64.const -1))
(assert_return (invoke "extend32_s" (i64.const 0x01234567_00000000)) (i64.const 0))
(assert_return (invoke "extend32_s" (i64.const 0xfedcba98_80000000)) (i64.const -0x80000000))
(assert_return (invoke "extend32_s" (i64.const -1)) (i64.const -1))
