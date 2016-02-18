;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(memory 16777216 16777216 0)
(func (result i32)
(return (i32.const 2))
)
(func (param i32) (result i32)
(if (i32.ges (i32.const 26) (i32.const 25)) (setlocal 0 (i32.add (getlocal 0) (i32.const 4))))

(if_else
    (i32.ges (i32.const 22) (i32.const 25))
    (setlocal 0 (i32.add (getlocal 0) (i32.const 4)))
    (setlocal 0 (i32.sub (getlocal 0) (i32.const 5)))
)
(block
    (setlocal 0 (i32.add (getlocal 0) (i32.const 4)))
    (setlocal 0 (i32.add (getlocal 0) (i32.const 4)))
    (setlocal 0 (i32.add (getlocal 0) (call 0)))
)
(i32.store 0 10000 (getlocal 0) (i32.add (getlocal 0) (i32.const 7)))
(setlocal 0 (i32.load 0 10000 (getlocal 0)))
(return (i32.add (getlocal 0) (i32.const 42)))
)

(export "a" 0)