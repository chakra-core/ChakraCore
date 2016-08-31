;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module

  (func $test_unreachable (result i32) (unreachable) (i32.const 1))
  (export "test_unreachable" $test_unreachable)


)

(assert_return (invoke "test_unreachable") (i32.const 1))
