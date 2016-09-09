(module "\00asm\0b\00\00\00")
(module "\00asm" "\0b\00\00\00")

(assert_invalid (module "") "unexpected end")
(assert_invalid (module "\01") "unexpected end")
(assert_invalid (module "\00as") "unexpected end")
(assert_invalid (module "\01") "unexpected end")
(assert_invalid (module "asm\00") "magic header not detected")

(assert_invalid (module "\00asm") "unexpected end")
(assert_invalid (module "\00asm\0b") "unexpected end")
(assert_invalid (module "\00asm\0b\00\00") "unexpected end")
(assert_invalid (module "\00asm\10\00\00\00") "unknown binary version")

