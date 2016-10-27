(module
  (memory 1)
  (func $bad (drop (i32.load offset=4294967296 (i32.const 0))))
)
