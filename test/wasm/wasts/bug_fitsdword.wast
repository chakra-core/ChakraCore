;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (func (export "foo") (result i64) (i64.extend_u/i32 (i32.const 0x80000000)))
  (func (export "bar") (result i64) (i64.extend_u/i32 (i32.const 0xabcdef12)))
)
