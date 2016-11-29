;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft Corporation and contributors. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
(module
  (global $iImport (import "test" "i32") i32)
;;(global $lImport (import "test" "i64") i64) ;; illegal to import i64
  (global $fImport (import "test" "f32") f32)
  (global $dImport (import "test" "f64") f64)

  (global $iBasic i32 (i32.const -2))
  (global $iExport (export "i32") i32 (i32.const 10))
;;(global $iExportMut (export "i32") (mut i32) (i32.const 8)) ;; illegal to export mutable global
  (global $iMutable (mut i32) (i32.const 0))
  (global $iImpInit i32 (get_global $iImport))
;;(global $iImpInitMut (mut i32) (get_global $iImport)) ;; illegal to import mutable global

  (global $lBasic i64 (i64.const 0x422000800040))
;;(global $lExport (export "i64") i64 (i64.const 10)) ;; illegal to export i64
;;(global $lExportMut (export "i64") (mut i64) (i64.const 789)) ;; illegal to export mutable global
  (global $lMutable (mut i64) (i64.const 0))
;;(global $lImpInit i64 (get_global $lImport)) ;; illegal to import i64
;;(global $lImpInitMut (mut i64) (get_global $lImport)) ;; illegal to import mutable global

  (global $fBasic f32 (f32.const 3.14))
  (global $fExport (export "f32") f32 (f32.const -54.87))
;;(global $fExportMut (export "f32") (mut f32) (f32.const 8)) ;; illegal to export mutable global
  (global $fMutable (mut f32) (f32.const 0))
  (global $fImpInit f32 (get_global $fImport))
;;(global $fImpInitMut (mut f32) (get_global $fImport)) ;; illegal to import mutable global

  (global $dBasic f64 (f64.const -8745.544))
  (global $dExport (export "f64") f64 (f64.const 7861.477))
;;(global $dExportMut (export "f64") (mut f64) (f64.const 97)) ;; illegal to export mutable global
  (global $dMutable (mut f64) (f64.const 0))
  (global $dImpInit f64 (get_global $dImport))
;;(global $dImpInitMut (mut f64) (get_global $dImport)) ;; illegal to import mutable global

  (func (export "get-i32") (param i32) (result i32)
    (block (block (block (block (block (block (block (br_table 0 1 2 3 4 5 6 (get_local 0)) (unreachable))
    ;; case 0
    (return (get_global $iBasic)))
    ;; case 1
    (return (get_global $iExport)))
    ;; case 2
    (unreachable)
    ;;(return (get_global $iExportMut))
    )
    ;; case 3
    (return (get_global $iMutable)))
    ;; case 4
    (return (get_global $iImport)))
    ;; case 5
    (return (get_global $iImpInit)))
    ;; case 6
    (unreachable)
    ;;(get_global $iImpInitMut)
  )
  (func (export "set-i32") (param i32)
    (set_global $iMutable (get_local 0))
  )

  (func (export "get-i64") (param i32) (result i64)
    (block (block (block (block (block (block (block (br_table 0 1 2 3 4 5 6 (get_local 0)) (unreachable))
    ;; case 0
    (return (get_global $lBasic)))
    ;; case 1
    (unreachable)
    ;;(return (get_global $lExport))
    )
    ;; case 2
    (unreachable)
    ;;(return (get_global $lExportMut))
    )
    ;; case 3
    (return (get_global $lMutable)))
    ;; case 4
    (unreachable)
    ;;(return (get_global $lImport))
    )
    ;; case 5
    (unreachable)
    ;;(return (get_global $lImpInit))
    )
    ;; case 6
    (unreachable)
    ;;(get_global $lImpInitMut)
  )
  (func (export "set-i64") (param i64)
    (set_global $lMutable (get_local 0))
  )

  (func (export "get-f32") (param i32) (result f32)
    (block (block (block (block (block (block (block (br_table 0 1 2 3 4 5 6 (get_local 0)) (unreachable))
    ;; case 0
    (return (get_global $fBasic)))
    ;; case 1
    (return (get_global $fExport)))
    ;; case 2
    (unreachable)
    ;;(return (get_global $fExportMut))
    )
    ;; case 3
    (return (get_global $fMutable)))
    ;; case 4
    (return (get_global $fImport)))
    ;; case 5
    (return (get_global $fImpInit)))
    ;; case 6
    (unreachable)
    ;;(get_global $fImpInitMut)
  )
  (func (export "set-f32") (param f32)
    (set_global $fMutable (get_local 0))
  )

  (func (export "get-f64") (param i32) (result f64)
    (block (block (block (block (block (block (block (br_table 0 1 2 3 4 5 6 (get_local 0)) (unreachable))
    ;; case 0
    (return (get_global $dBasic)))
    ;; case 1
    (return (get_global $dExport)))
    ;; case 2
    (unreachable)
    ;;(return (get_global $dExportMut))
    )
    ;; case 3
    (return (get_global $dMutable)))
    ;; case 4
    (return (get_global $dImport)))
    ;; case 5
    (return (get_global $dImpInit)))
    ;; case 6
    (unreachable)
    ;;(get_global $dImpInitMut)
  )
  (func (export "set-f64") (param f64)
    (set_global $dMutable (get_local 0))
  )
)



