;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (memory 1)
  (data (i32.const 0) "abcdefghijklmnopqrstuvwxyz")
  (func (export "goodload") (param $i i32) (result i32) (i32.load8_u offset=0 (get_local $i)))
  (func (export "goodstore") (param $i i32) (i32.store8 offset=0 (get_local $i) (get_local 0)))
  (func (export "badload") (param $i i32) (result i32) (i32.load offset=4294967295 (get_local $i)))
  (func (export "badstore") (param $i i32) (i32.store offset=100000 (get_local $i) (get_local 0)))
)
