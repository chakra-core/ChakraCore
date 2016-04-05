;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
  (func  (result i32)
    (if (i32.const 1) (return (i32.const 2)) (return (i32.const 3))))
  (export "foo" 0)
)
