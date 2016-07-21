(module
  (func
    (block
      (if (i32.const 0)
        (then $l (br $l (i32.const 1)))
        (else (br $l (i32.const 42)))
      )
    )
  )
)
