(module
  (func $br (block (br 0)))
  (export "br" $br)

  (func $br-nop (block (br 0 (nop))))
  (export "br-nop" $br-nop)

  (func $br-drop (block (br 0 (i32.const 1))))
  (export "br-drop" $br-drop)

  (func $br-block-nop (block (br 0 (block (i32.const 1) (nop)))))
  (export "br-block-nop" $br-block-nop)

  (func $br-block-drop (block (br 0 (block (nop) (i32.const 1)))))
  (export "br-block-drop" $br-block-drop)

  (func $br_if (block (br_if 0 (i32.const 1))))
  (export "br_if" $br_if)

  (func $br_if-nop (block (br_if 0 (nop) (i32.const 1))))
  (export "br_if-nop" $br_if-nop)

  (func $br_if-drop (block (br_if 0 (i32.const 1) (i32.const 1))))
  (export "br_if-drop" $br_if-drop)

  (func $br_if-block-nop (block (br_if 0 (block (i32.const 1) (nop)) (i32.const 1))))
  (export "br_if-block-nop" $br_if-block-nop)

  (func $br_if-block-drop (block (br_if 0 (block (nop) (i32.const 1)) (i32.const 1))))
  (export "br_if-block-drop" $br_if-block-drop)

  (func $br_table (block (br_table 0 (i32.const 0))))
  (export "br_table" $br_table)

  (func $br_table-nop (block (br_table 0 (nop) (i32.const 0))))
  (export "br_table-nop" $br_table-nop)

  (func $br_table-drop (block (br_table 0 (i32.const 1) (i32.const 0))))
  (export "br_table-drop" $br_table-drop)

  (func $br_table-block-nop (block (br_table 0 (block (i32.const 1) (nop)) (i32.const 0))))
  (export "br_table-block-nop" $br_table-block-nop)

  (func $br_table-block-drop (block (br_table 0 (block (nop) (i32.const 1)) (i32.const 0))))
  (export "br_table-block-drop" $br_table-block-drop)
)

(assert_return (invoke "br"))
(assert_return (invoke "br-nop"))
(assert_return (invoke "br-drop"))
(assert_return (invoke "br-block-nop"))
(assert_return (invoke "br-block-drop"))

(assert_return (invoke "br_if"))
(assert_return (invoke "br_if-nop"))
(assert_return (invoke "br_if-drop"))
(assert_return (invoke "br_if-block-nop"))
(assert_return (invoke "br_if-block-drop"))

(assert_return (invoke "br_table"))
(assert_return (invoke "br_table-nop"))
(assert_return (invoke "br_table-drop"))
(assert_return (invoke "br_table-block-nop"))
(assert_return (invoke "br_table-block-drop"))
