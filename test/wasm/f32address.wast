;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (memory 1 1)
  (data (i32.const 1) "\00\00\e0\7f")

  (func (export "f32.load") (result f32) (f32.load offset=0 align=1 (i32.const 1)))
  (func (export "i32.load") (result i32) (i32.load offset=0 align=2 (i32.const 1)))
  (func (export "f32.store") (f32.store (i32.const 1) (f32.const 0x7fa00000)))
  (func (export "i32.store") (i32.store (i32.const 1) (i32.const 0x7fa00000)))
  (func (export "reset") (i32.store (i32.const 1) (i32.const 0)))


  (func (export "f32.load1") (result f32) (f32.load offset=1 align=1 (i32.const 1)))
  (func (export "i32.load1") (result i32) (i32.load offset=1 align=2 (i32.const 1)))
  (func (export "f32.store1") (f32.store offset=1 (i32.const 1) (f32.const 0x7fa00000)))
  (func (export "i32.store1") (i32.store offset=1 (i32.const 1) (i32.const 0x7fa00000)))
  (func (export "reset1") (i32.store offset=1 (i32.const 1) (i32.const 0)))

  (func (export "f32.load2") (result f32) (f32.load offset=2 align=1 (i32.const 1)))
  (func (export "i32.load2") (result i32) (i32.load offset=2 align=2 (i32.const 1)))
  (func (export "f32.store2") (f32.store offset=2 (i32.const 1) (f32.const 0x7fa00000)))
  (func (export "i32.store2") (i32.store offset=2 (i32.const 1) (i32.const 0x7fa00000)))
  (func (export "reset2") (i32.store offset=2 (i32.const 1) (i32.const 0)))
)


;;(assert_return (invoke "i32.load") (i32.const 0x7fa00000))
;;(assert_return (invoke "f32.load") (f32.const nan:0x200000))
;;(invoke "reset")
;;(assert_return (invoke "i32.load") (i32.const 0x0))
;;(assert_return (invoke "f32.load") (f32.const 0.0))
;;(invoke "f32.store")
;;(assert_return (invoke "i32.load") (i32.const 0x7fa00000))
;;(assert_return (invoke "f32.load") (f32.const nan:0x200000))
;;(invoke "reset")
;;(assert_return (invoke "i32.load") (i32.const 0x0))
;;(assert_return (invoke "f32.load") (f32.const 0.0))
;;(invoke "i32.store")
;;(assert_return (invoke "i32.load") (i32.const 0x7fa00000))
;;(assert_return (invoke "f32.load") (f32.const nan:0x200000))
