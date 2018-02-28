;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
;;AUTO-GENERATED do not modify

(module
  (func (export "extend8_s0") (result i32) (i32.extend8_s (i32.const 0)))
  (func (export "extend8_s1") (result i32) (i32.extend8_s (i32.const 0x7f)))
  (func (export "extend8_s2") (result i32) (i32.extend8_s (i32.const 0x80)))
  (func (export "extend8_s3") (result i32) (i32.extend8_s (i32.const 0xff)))
  (func (export "extend8_s4") (result i32) (i32.extend8_s (i32.const 0x012345_00)))
  (func (export "extend8_s5") (result i32) (i32.extend8_s (i32.const 0xfedcba_80)))
  (func (export "extend8_s6") (result i32) (i32.extend8_s (i32.const -1)))
  (func (export "extend16_s7") (result i32) (i32.extend16_s (i32.const 0)))
  (func (export "extend16_s8") (result i32) (i32.extend16_s (i32.const 0x7fff)))
  (func (export "extend16_s9") (result i32) (i32.extend16_s (i32.const 0x8000)))
  (func (export "extend16_s10") (result i32) (i32.extend16_s (i32.const 0xffff)))
  (func (export "extend16_s11") (result i32) (i32.extend16_s (i32.const 0x0123_0000)))
  (func (export "extend16_s12") (result i32) (i32.extend16_s (i32.const 0xfedc_8000)))
  (func (export "extend16_s13") (result i32) (i32.extend16_s (i32.const -1)))
)

(assert_return (invoke "extend8_s0") (i32.const 0))
(assert_return (invoke "extend8_s1") (i32.const 127))
(assert_return (invoke "extend8_s2") (i32.const -128))
(assert_return (invoke "extend8_s3") (i32.const -1))
(assert_return (invoke "extend8_s4") (i32.const 0))
(assert_return (invoke "extend8_s5") (i32.const -0x80))
(assert_return (invoke "extend8_s6") (i32.const -1))
(assert_return (invoke "extend16_s7") (i32.const 0))
(assert_return (invoke "extend16_s8") (i32.const 32767))
(assert_return (invoke "extend16_s9") (i32.const -32768))
(assert_return (invoke "extend16_s10") (i32.const -1))
(assert_return (invoke "extend16_s11") (i32.const 0))
(assert_return (invoke "extend16_s12") (i32.const -0x8000))
(assert_return (invoke "extend16_s13") (i32.const -1))