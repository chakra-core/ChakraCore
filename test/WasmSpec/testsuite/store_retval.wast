(module
    (memory 1)

    (import $print_i32 "spectest" "print" (param i32))
    (import $print_i64 "spectest" "print" (param i64))
    (import $print_f32 "spectest" "print" (param f32))
    (import $print_f64 "spectest" "print" (param f64))

    (func $run
        (local $i32 i32) (local $i64 i64) (local $f32 f32) (local $f64 f64)
        (call_import $print_i32 (set_local $i32 (i32.const 1)))
        (call_import $print_i64 (set_local $i64 (i64.const 2)))
        (call_import $print_f32 (set_local $f32 (f32.const 3)))
        (call_import $print_f64 (set_local $f64 (f64.const 4)))

        (call_import $print_i32 (i32.store (i32.const 0) (i32.const 11)))
        (call_import $print_i64 (i64.store (i32.const 0) (i64.const 12)))
        (call_import $print_f32 (f32.store (i32.const 0) (f32.const 13)))
        (call_import $print_f64 (f64.store (i32.const 0) (f64.const 14)))

        (call_import $print_i32 (i32.store8 (i32.const 0) (i32.const 512)))
        (call_import $print_i32 (i32.store16 (i32.const 0) (i32.const 65536)))
        (call_import $print_i64 (i64.store8 (i32.const 0) (i64.const 512)))
        (call_import $print_i64 (i64.store16 (i32.const 0) (i64.const 65536)))
        (call_import $print_i64 (i64.store32 (i32.const 0) (i64.const 4294967296)))
    )
    (export "run" $run)
)

(invoke "run")

(assert_invalid
    (module (func (local $i32 i32) (local $i64 i64)
        (set_local $i64 (set_local $i32 (i32.const 1)))))
    "type mismatch: expression has type i32 but the context requires i64"
)
(assert_invalid
    (module (func (local $i32 i32) (local $i64 i64)
        (set_local $i32 (set_local $i64 (i64.const 1)))))
    "type mismatch: expression has type i64 but the context requires i32"
)
(assert_invalid
    (module (func (local $f32 f32) (local $f64 f64)
        (set_local $f64 (set_local $f32 (f32.const 1)))))
    "type mismatch: expression has type f32 but the context requires f64"
)
(assert_invalid
    (module (func (local $f32 f32) (local $f64 f64)
        (set_local $f32 (set_local $f64 (f64.const 1)))))
    "type mismatch: expression has type f64 but the context requires f32"
)
