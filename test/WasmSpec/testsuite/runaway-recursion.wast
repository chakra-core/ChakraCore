(module
  ;; Implementations are required to have every call consume some abstract
  ;; resource towards exhausting some abstract finite limit, such that
  ;; infinitely recursive testcases reliably trap in finite time. This is
  ;; because otherwise applications could come to depend on it on those
  ;; implementations and be incompatible with implementations that don't do
  ;; it (or don't do it under the same circumstances).
  (func (call 0))
  (export "runaway" 0)

  (func $a (call $b))
  (func $b (call $a))
  (export "mutual_runaway" $a)
)

(assert_trap (invoke "runaway") "call stack exhausted")
(assert_trap (invoke "mutual_runaway") "call stack exhausted")
