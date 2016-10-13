;; Test files can define multiple modules. Test that implementations treat
;; each module independently from the other.

(module
  (func (export "foo") (result i32) (i32.const 0))
)

(assert_return (invoke "foo") (i32.const 0))

;; Another module, same function name, different contents.

(module
  (func (export "foo") (result i32) (i32.const 1))
)

(assert_return (invoke "foo") (i32.const 1))


(module
  ;; Test that we can use the empty string as a symbol.
  (func (export "") (result f32) (f32.const 0x1.91p+2))

  ;; Test that we can use names beginning with a digit.
  (func (export "0") (result f32) (f32.const 0x1.97p+2))

  ;; Test that we can use names beginning with an underscore.
  (func (export "_") (result f32) (f32.const 0x1.98p+2))

  ;; Test that we can use names beginning with a dollar sign.
  (func (export "$") (result f32) (f32.const 0x1.99p+2))

  ;; Test that we can use names beginning with an at sign.
  (func (export "@") (result f32) (f32.const 0x2.00p+2))

  ;; Test that we can use non-alphanumeric names.
  (func (export "~!@#$%^&*()_+`-={}|[]\\:\";'<>?,./ ") (result f32) (f32.const 0x1.96p+2))

  ;; Test that we can use names that have special meaning in JS.
  (func (export "NaN") (result f32) (f32.const 0x2.01p+2))
  (func (export "Infinity") (result f32) (f32.const 0x2.02p+2))
  (func (export "if") (result f32) (f32.const 0x2.03p+2))

  ;; Test that we can use common libc names without conflict.
  (func (export "malloc") (result f32) (f32.const 0x1.92p+2))

  ;; Test that we can use some libc hidden names without conflict.
  (func (export "_malloc") (result f32) (f32.const 0x1.93p+2))
  (func (export "__malloc") (result f32) (f32.const 0x1.94p+2))
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
  ;; exports, functions and parameters.
  (import "spectest" "print" (func (param i32)))
  (func (import "spectest" "print") (param i32))
  (func (param i32) (param i32)
    (call 0 (get_local 0))
    (call 1 (get_local 1))
  )
  (export "print32" (func 2))
)

(invoke "print32" (i32.const 42) (i32.const 123))
