;; Test `call_indirect` operator

(module
  ;; Auxiliary definitions
  (type $proc (func))
  (type $out-i32 (func (result i32)))
  (type $out-i64 (func (result i64)))
  (type $out-f32 (func (result f32)))
  (type $out-f64 (func (result f64)))
  (type $over-i32 (func (param i32) (result i32)))
  (type $over-i64 (func (param i64) (result i64)))
  (type $over-f32 (func (param f32) (result f32)))
  (type $over-f64 (func (param f64) (result f64)))
  (type $f32-i32 (func (param f32 i32) (result i32)))
  (type $i32-i64 (func (param i32 i64) (result i64)))
  (type $f64-f32 (func (param f64 f32) (result f32)))
  (type $i64-f64 (func (param i64 f64) (result f64)))

  (func $const-i32 (type $out-i32) (i32.const 0x132))
  (func $const-i64 (type $out-i64) (i64.const 0x164))
  (func $const-f32 (type $out-f32) (f32.const 0xf32))
  (func $const-f64 (type $out-f64) (f64.const 0xf64))

  (func $id-i32 (type $over-i32) (get_local 0))
  (func $id-i64 (type $over-i64) (get_local 0))
  (func $id-f32 (type $over-f32) (get_local 0))
  (func $id-f64 (type $over-f64) (get_local 0))

  (func $f32-i32 (type $f32-i32) (get_local 1))
  (func $i32-i64 (type $i32-i64) (get_local 1))
  (func $f64-f32 (type $f64-f32) (get_local 1))
  (func $i64-f64 (type $i64-f64) (get_local 1))

  (table
    $const-i32 $const-i64 $const-f32 $const-f64
    $id-i32 $id-i64 $id-f32 $id-f64
    $f32-i32 $i32-i64 $f64-f32 $i64-f64
    $fac $fib $even $odd
    $runaway $mutual-runaway1 $mutual-runaway2
  )

  ;; Typing

  (func "type-i32" (result i32) (call_indirect $out-i32 (i32.const 0)))
  (func "type-i64" (result i64) (call_indirect $out-i64 (i32.const 1)))
  (func "type-f32" (result f32) (call_indirect $out-f32 (i32.const 2)))
  (func "type-f64" (result f64) (call_indirect $out-f64 (i32.const 3)))

  (func "type-index" (result i64)
    (call_indirect $over-i64 (i32.const 5) (i64.const 100))
  )

  (func "type-first-i32" (result i32)
    (call_indirect $over-i32 (i32.const 4) (i32.const 32))
  )
  (func "type-first-i64" (result i64)
    (call_indirect $over-i64 (i32.const 5) (i64.const 64))
  )
  (func "type-first-f32" (result f32)
    (call_indirect $over-f32 (i32.const 6) (f32.const 1.32))
  )
  (func "type-first-f64" (result f64)
    (call_indirect $over-f64 (i32.const 7) (f64.const 1.64))
  )

  (func "type-second-i32" (result i32)
    (call_indirect $f32-i32 (i32.const 8) (f32.const 32.1) (i32.const 32))
  )
  (func "type-second-i64" (result i64)
    (call_indirect $i32-i64 (i32.const 9) (i32.const 32) (i64.const 64))
  )
  (func "type-second-f32" (result f32)
    (call_indirect $f64-f32 (i32.const 10) (f64.const 64) (f32.const 32))
  )
  (func "type-second-f64" (result f64)
    (call_indirect $i64-f64 (i32.const 11) (i64.const 64) (f64.const 64.1))
  )

  ;; Dispatch

  (func "dispatch" (param i32 i64) (result i64)
    (call_indirect $over-i64 (get_local 0) (get_local 1))
  )

  ;; Recursion

  (func "fac" $fac (param i64) (result i64)
    (if (i64.eqz (get_local 0))
      (i64.const 1)
      (i64.mul
        (get_local 0)
        (call_indirect $over-i64 (i32.const 12)
          (i64.sub (get_local 0) (i64.const 1))
        )
      )
    )
  )

  (func "fib" $fib (param i64) (result i64)
    (if (i64.le_u (get_local 0) (i64.const 1))
      (i64.const 1)
      (i64.add
        (call_indirect $over-i64 (i32.const 13)
          (i64.sub (get_local 0) (i64.const 2))
        )
        (call_indirect $over-i64 (i32.const 13)
          (i64.sub (get_local 0) (i64.const 1))
        )
      )
    )
  )

  (func "even" $even (param i32) (result i32)
    (if (i32.eqz (get_local 0))
      (i32.const 44)
      (call_indirect $over-i32 (i32.const 15)
        (i32.sub (get_local 0) (i32.const 1))
      )
    )
  )
  (func "odd" $odd (param i32) (result i32)
    (if (i32.eqz (get_local 0))
      (i32.const 99)
      (call_indirect $over-i32 (i32.const 14)
        (i32.sub (get_local 0) (i32.const 1))
      )
    )
  )

  ;; Stack exhaustion

  ;; Implementations are required to have every call consume some abstract
  ;; resource towards exhausting some abstract finite limit, such that
  ;; infinitely recursive test cases reliably trap in finite time. This is
  ;; because otherwise applications could come to depend on it on those
  ;; implementations and be incompatible with implementations that don't do
  ;; it (or don't do it under the same circumstances).

  (func "runaway" $runaway (call_indirect $proc (i32.const 16)))

  (func "mutual-runaway" $mutual-runaway1 (call_indirect $proc (i32.const 18)))
  (func $mutual-runaway2 (call_indirect $proc (i32.const 17)))
)

(assert_return (invoke "type-i32") (i32.const 0x132))
(assert_return (invoke "type-i64") (i64.const 0x164))
(assert_return (invoke "type-f32") (f32.const 0xf32))
(assert_return (invoke "type-f64") (f64.const 0xf64))

(assert_return (invoke "type-index") (i64.const 100))

(assert_return (invoke "type-first-i32") (i32.const 32))
(assert_return (invoke "type-first-i64") (i64.const 64))
(assert_return (invoke "type-first-f32") (f32.const 1.32))
(assert_return (invoke "type-first-f64") (f64.const 1.64))

(assert_return (invoke "type-second-i32") (i32.const 32))
(assert_return (invoke "type-second-i64") (i64.const 64))
(assert_return (invoke "type-second-f32") (f32.const 32))
(assert_return (invoke "type-second-f64") (f64.const 64.1))

(assert_return (invoke "dispatch" (i32.const 5) (i64.const 2)) (i64.const 2))
(assert_return (invoke "dispatch" (i32.const 5) (i64.const 5)) (i64.const 5))
(assert_return (invoke "dispatch" (i32.const 12) (i64.const 5)) (i64.const 120))
(assert_return (invoke "dispatch" (i32.const 13) (i64.const 5)) (i64.const 8))
(assert_trap (invoke "dispatch" (i32.const 0) (i64.const 2)) "indirect call signature mismatch")
(assert_trap (invoke "dispatch" (i32.const 15) (i64.const 2)) "indirect call signature mismatch")
(assert_trap (invoke "dispatch" (i32.const 20) (i64.const 2)) "undefined table index")
(assert_trap (invoke "dispatch" (i32.const -1) (i64.const 2)) "undefined table index")
(assert_trap (invoke "dispatch" (i32.const 1213432423) (i64.const 2)) "undefined table index")

(assert_return (invoke "fac" (i64.const 0)) (i64.const 1))
(assert_return (invoke "fac" (i64.const 1)) (i64.const 1))
(assert_return (invoke "fac" (i64.const 5)) (i64.const 120))
(assert_return (invoke "fac" (i64.const 25)) (i64.const 7034535277573963776))

(assert_return (invoke "fib" (i64.const 0)) (i64.const 1))
(assert_return (invoke "fib" (i64.const 1)) (i64.const 1))
(assert_return (invoke "fib" (i64.const 2)) (i64.const 2))
(assert_return (invoke "fib" (i64.const 5)) (i64.const 8))
(assert_return (invoke "fib" (i64.const 20)) (i64.const 10946))

(assert_return (invoke "even" (i32.const 0)) (i32.const 44))
(assert_return (invoke "even" (i32.const 1)) (i32.const 99))
(assert_return (invoke "even" (i32.const 100)) (i32.const 44))
(assert_return (invoke "even" (i32.const 77)) (i32.const 99))
(assert_return (invoke "odd" (i32.const 0)) (i32.const 99))
(assert_return (invoke "odd" (i32.const 1)) (i32.const 44))
(assert_return (invoke "odd" (i32.const 200)) (i32.const 99))
(assert_return (invoke "odd" (i32.const 77)) (i32.const 44))

(assert_trap (invoke "runaway") "call stack exhausted")
(assert_trap (invoke "mutual-runaway") "call stack exhausted")


;; Invalid typing

(assert_invalid
  (module
    (type (func))
    (func $type-void-vs-num (i32.eqz (call_indirect 0 (i32.const 0))))
  )
  "type mismatch"
)
(assert_invalid
  (module
    (type (func (result i64)))
    (func $type-num-vs-num (i32.eqz (call_indirect 0 (i32.const 0))))
  )
  "type mismatch"
)

(assert_invalid
  (module
    (type (func (param i32)))
    (func $arity-0-vs-1 (call_indirect 0 (i32.const 0)))
  )
  "arity mismatch"
)
(assert_invalid
  (module
    (type (func (param f64 i32)))
    (func $arity-0-vs-2 (call_indirect 0 (i32.const 0)))
  )
  "arity mismatch"
)
(assert_invalid
  (module
    (type (func))
    (func $arity-1-vs-0 (call_indirect 0 (i32.const 0) (i32.const 1)))
  )
  "arity mismatch"
)
(assert_invalid
  (module
    (type (func))
    (func $arity-2-vs-0
      (call_indirect 0 (i32.const 0) (f64.const 2) (i32.const 1))
    )
  )
  "arity mismatch"
)

(assert_invalid
  (module
    (type (func (param i32 i32)))
    (func $arity-nop-first
      (call_indirect 0 (i32.const 0) (nop) (i32.const 1) (i32.const 2))
    )
  )
  "arity mismatch"
)
(assert_invalid
  (module
    (type (func (param i32 i32)))
    (func $arity-nop-mid
      (call_indirect 0 (i32.const 0) (i32.const 1) (nop) (i32.const 2))
    )
  )
  "arity mismatch"
)
(assert_invalid
  (module
    (type (func (param i32 i32)))
    (func $arity-nop-last
      (call_indirect 0 (i32.const 0) (i32.const 1) (i32.const 2) (nop))
    )
  )
  "arity mismatch"
)

(assert_invalid
  (module
    (type (func (param i32)))
    (func $type-func-void-vs-i32 (call_indirect 0 (nop) (i32.const 1)))
  )
  "type mismatch"
)
(assert_invalid
  (module
    (type (func (param i32)))
    (func $type-func-num-vs-i32 (call_indirect 0 (i64.const 1) (i32.const 0)))
  )
  "type mismatch"
)

(assert_invalid
  (module
    (type (func (param i32 i32)))
    (func $type-first-void-vs-num
      (call_indirect 0 (i32.const 0) (nop) (i32.const 1))
    )
  )
  "type mismatch"
)
(assert_invalid
  (module
    (type (func (param i32 i32)))
    (func $type-second-void-vs-num
      (call_indirect 0 (i32.const 0) (i32.const 1) (nop))
    )
  )
  "type mismatch"
)
(assert_invalid
  (module
    (type (func (param i32 f64)))
    (func $type-first-num-vs-num
      (call_indirect 0 (i32.const 0) (f64.const 1) (i32.const 1))
    )
  )
  "type mismatch"
)
(assert_invalid
  (module
    (type (func (param f64 i32)))
    (func $type-second-num-vs-num
      (call_indirect 0 (i32.const 0) (i32.const 1) (f64.const 1))
    )
  )
  "type mismatch"
)


;; Unbound type

(assert_invalid
  (module (func $unbound-type (call_indirect 1 (i32.const 0))))
  "unknown function type"
)
(assert_invalid
  (module (func $large-type (call_indirect 10001232130000 (i32.const 0))))
  "unknown function type"
)
