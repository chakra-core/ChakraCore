;; Test files can define multiple modules. Test that implementations treat
;; each module independently from the other.

(module
  (func $foo (result i32) (i32.const 0))
  (export "foo" $foo)
)

(assert_return (invoke "foo") (i32.const 0))

;; Another module, same function name, different contents.

(module
  (func $foo (result i32) (i32.const 1))
  (export "foo" $foo)
)

(assert_return (invoke "foo") (i32.const 1))


(module
  ;; Test that we can use the empty string as a symbol.
  (func (result f32) (f32.const 0x1.91p+2))
  (export "" 0)

  ;; Test that we can use common libc names without conflict.
  (func $malloc (result f32) (f32.const 0x1.92p+2))
  (export "malloc" $malloc)

  ;; Test that we can use some libc hidden names without conflict.
  (func $_malloc (result f32) (f32.const 0x1.93p+2))
  (func $__malloc (result f32) (f32.const 0x1.94p+2))
  (func (result f32) (f32.const 0x1.95p+2))
  (export "_malloc" $_malloc)
  (export "__malloc" $__malloc)

  ;; Test that we can use non-alphanumeric names.
  (func (result f32) (f32.const 0x1.96p+2))
  (export "~!@#$%^&*()_+`-={}|[]\\:\";'<>?,./ " 5)

  ;; Test that we can use names beginning with a digit.
  (func (result f32) (f32.const 0x1.97p+2))
  (export "0" 6)

  ;; Test that we can use names beginning with an underscore.
  (func $_ (result f32) (f32.const 0x1.98p+2))
  (export "_" $_)

  ;; Test that we can use names beginning with a dollar sign.
  (func (result f32) (f32.const 0x1.99p+2))
  (export "$" 8)

  ;; Test that we can use names beginning with an at sign.
  (func (result f32) (f32.const 0x2.00p+2))
  (export "@" 9)

  ;; Test that we can use names that have special meaning in JS.
  (func $NaN (result f32) (f32.const 0x2.01p+2))
  (func $Infinity (result f32) (f32.const 0x2.02p+2))
  (func $if (result f32) (f32.const 0x2.03p+2))
  (export "NaN" $NaN)
  (export "Infinity" $Infinity)
  (export "if" $if)
)

(assert_return (invoke "") (f32.const 0x1.91p+2))
(assert_return (invoke "malloc") (f32.const 0x1.92p+2))
(assert_return (invoke "_malloc") (f32.const 0x1.93p+2))
(assert_return (invoke "__malloc") (f32.const 0x1.94p+2))
(assert_return (invoke "~!@#$%^&*()_+`-={}|[]\\:\";'<>?,./ ") (f32.const 0x1.96p+2))
(assert_return (invoke "0") (f32.const 0x1.97p+2))
(assert_return (invoke "_") (f32.const 0x1.98p+2))
(assert_return (invoke "$") (f32.const 0x1.99p+2))
(assert_return (invoke "@") (f32.const 0x2.00p+2))
(assert_return (invoke "NaN") (f32.const 0x2.01p+2))
(assert_return (invoke "Infinity") (f32.const 0x2.02p+2))
(assert_return (invoke "if") (f32.const 0x2.03p+2))

(module
  ;; Test that we can use indices instead of names to reference imports,
  ;; functions and parameters.
  (import "spectest" "print" (param i32))
  (import "spectest" "print" (param i32))
  (func (param i32) (param i32)
    (call_import 0 (get_local 0))
    (call_import 1 (get_local 1))
  )
  (export "print32" 0)
)

;;(invoke "print32" (i32.const 42) (i32.const 123))
