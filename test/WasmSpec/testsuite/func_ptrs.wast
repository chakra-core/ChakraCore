(module
    (type    (func))                           ;; 0: void -> void
    (type $S (func))                           ;; 1: void -> void
    (type    (func (param)))                   ;; 2: void -> void
    (type    (func (result i32)))              ;; 3: void -> i32
    (type    (func (param) (result i32)))      ;; 4: void -> i32
    (type $T (func (param i32) (result i32)))  ;; 5: i32 -> i32
    (type $U (func (param i32)))               ;; 6: i32 -> void

    (func (type 0))
    (func (type $S))

    (func $one (type 4) (i32.const 13))
    (export "one" $one)

    (func $two (type $T) (i32.add (get_local 0) (i32.const 1)))
    (export "two" $two)

    ;; Both signature and parameters are allowed (and required to match)
    ;; since this allows the naming of parameters.
    (func $three (type $T) (param $a i32) (result i32) (i32.sub (get_local 0) (i32.const 2)))
    (export "three" $three)

    (import $print "spectest" "print" (type 6))
    (func $four (type $U) (call_import $print (get_local 0)))
    (export "four" $four)
)
(assert_return (invoke "one") (i32.const 13))
(assert_return (invoke "two" (i32.const 13)) (i32.const 14))
(assert_return (invoke "three" (i32.const 13)) (i32.const 11))
(invoke "four" (i32.const 83))

(assert_invalid (module (func (type 42))) "unknown function type 42")
(assert_invalid (module (import "spectest" "print" (type 43))) "unknown function type 43")

(module
    (type $T (func (param) (result i32)))
    (type $U (func (param) (result i32)))
    (table $t1 $t2 $t3 $u1 $u2 $t1 $t3)

    (func $t1 (type $T) (i32.const 1))
    (func $t2 (type $T) (i32.const 2))
    (func $t3 (type $T) (i32.const 3))
    (func $u1 (type $U) (i32.const 4))
    (func $u2 (type $U) (i32.const 5))

    (func $callt (param $i i32) (result i32)
        (call_indirect $T (get_local $i))
    )
    (export "callt" $callt)

    (func $callu (param $i i32) (result i32)
        (call_indirect $U (get_local $i))
    )
    (export "callu" $callu)
)

(assert_return (invoke "callt" (i32.const 0)) (i32.const 1))
(assert_return (invoke "callt" (i32.const 1)) (i32.const 2))
(assert_return (invoke "callt" (i32.const 2)) (i32.const 3))
(assert_trap   (invoke "callt" (i32.const 3)) "indirect call signature mismatch")
(assert_trap   (invoke "callt" (i32.const 4)) "indirect call signature mismatch")
(assert_return (invoke "callt" (i32.const 5)) (i32.const 1))
(assert_return (invoke "callt" (i32.const 6)) (i32.const 3))
(assert_trap   (invoke "callt" (i32.const 7)) "undefined table index 7")
(assert_trap   (invoke "callt" (i32.const 100)) "undefined table index 100")
(assert_trap   (invoke "callt" (i32.const -1)) "undefined table index -1")

(assert_trap   (invoke "callu" (i32.const 0)) "indirect call signature mismatch")
(assert_trap   (invoke "callu" (i32.const 1)) "indirect call signature mismatch")
(assert_trap   (invoke "callu" (i32.const 2)) "indirect call signature mismatch")
(assert_return (invoke "callu" (i32.const 3)) (i32.const 4))
(assert_return (invoke "callu" (i32.const 4)) (i32.const 5))
(assert_trap   (invoke "callu" (i32.const 5)) "indirect call signature mismatch")
(assert_trap   (invoke "callu" (i32.const 6)) "indirect call signature mismatch")
(assert_trap   (invoke "callu" (i32.const 7)) "undefined table index 7")
(assert_trap   (invoke "callu" (i32.const -1)) "undefined table index -1")

(module
    (type $T (func (result i32)))
    (table 0 1)

    (import $print_i32 "spectest" "print" (param i32))

    (func $t1 (type $T) (i32.const 1))
    (func $t2 (type $T) (i32.const 2))

    (func $callt (param $i i32) (result i32)
        (call_indirect $T (get_local $i)))
    (export "callt" $callt)
)

(assert_return (invoke "callt" (i32.const 0)) (i32.const 1))
(assert_return (invoke "callt" (i32.const 1)) (i32.const 2))
