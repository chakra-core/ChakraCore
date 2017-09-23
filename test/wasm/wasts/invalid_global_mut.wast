;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------

;;(module
;;  (import "spectest" "global" (global i32))
;;)
(assert_malformed (module "\00asm\01\00\00\00\02\94\80\80\80\00\01\08\73\70\65\63\74\65\73\74\06\67\6c\6f\62\61\6c\03\7f\02") "invalid mutability")

;;(module
;;  (global i32 (i32.const 0))
;;)
(assert_malformed (module "\00asm\01\00\00\00\06\86\80\80\80\00\01\7f\ff\41\00\0b") "invalid mutability")
(assert_malformed (module "\00asm\01\00\00\00\06\86\80\80\80\00\01\7f\d4\41\00\0b") "invalid mutability")
(assert_malformed (module "\00asm\01\00\00\00\06\86\80\80\80\00\01\7f\02\41\00\0b") "invalid mutability")
