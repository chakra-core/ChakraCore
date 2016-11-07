;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

(module
    (func (export "i64_div_s_by_two") (param $x i64) (result i64)
        (i64.div_s (get_local $x) (i64.const 2))
    )
    (func (export "i64_div_s_over") (result i64)
        (i64.div_s (i64.const 0x8000000000000000  ) (i64.const 0xffffffffffffffff))
    )
    (func (export "rem_s177") (result i64) (i64.rem_s (i64.const 0x7fffffffffffffff) (i64.const -1)))
)
