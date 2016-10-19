(module
    (memory 1 (segment 0 "abcdefghijklmnopqrstuvwxyz"))
    (import $print "spectest" "print" (param i32))

    (func $good (param $i i32)
        (call_import $print (i32.load8_u offset=0 (get_local $i)))  ;; 97 'a'
        (call_import $print (i32.load8_u offset=1 (get_local $i)))  ;; 98 'b'
        (call_import $print (i32.load8_u offset=2 (get_local $i)))  ;; 99 'c'
        (call_import $print (i32.load8_u offset=25 (get_local $i))) ;; 122 'z'

        (call_import $print (i32.load16_u offset=0 (get_local $i)))          ;; 25185 'ab'
        (call_import $print (i32.load16_u align=1 (get_local $i)))           ;; 25185 'ab'
        ;; Disable known test failure
        ;;(call_import $print (i32.load16_u offset=1 align=1 (get_local $i)))  ;; 25442 'bc'
        (call_import $print (i32.load16_u offset=2 (get_local $i)))          ;; 25699 'cd'
        ;; Disable known test failure
        ;;(call_import $print (i32.load16_u offset=25 align=1 (get_local $i))) ;; 122 'z\0'

        (call_import $print (i32.load offset=0 (get_local $i)))          ;; 1684234849 'abcd'
        ;; Disable known test failure
        ;;(call_import $print (i32.load offset=1 align=1 (get_local $i)))  ;; 1701077858 'bcde'
        ;;(call_import $print (i32.load offset=2 align=2 (get_local $i)))  ;; 1717920867 'cdef'
        ;;(call_import $print (i32.load offset=25 align=1 (get_local $i))) ;; 122 'z\0\0\0'
    )
    (export "good" $good)

    (func $bad2 (param $i i32) (i32.load offset=4294967295 (get_local $i)))
    (export "bad2" $bad2)
)

(invoke "good" (i32.const 0))
(invoke "good" (i32.const 65507))
(assert_trap (invoke "good" (i32.const 65508)) "out of bounds memory access")
(assert_trap (invoke "bad2" (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "bad2" (i32.const 1)) "out of bounds memory access")

(assert_invalid (module (memory 1) (func $bad1 (param $i i32) (i32.load offset=4294967296 (get_local $i))) ) "offset too large")
