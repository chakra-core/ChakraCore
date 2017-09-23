This is a rough list of "tests to write". Everything here should either be
specified in [Semantics.md](https://github.com/WebAssembly/design/blob/master/Semantics.md),
have a link to an open issue/PR, or be obvious. Comments/corrections/additions
welcome.

Linear memory semantics:
 - test that newly allocated memory (program start and `grow_memory`) is zeroed
 - test that `grow_memory` does a full 32-bit unsigned check for page-size divisibility
 - test that load/store addreses are full int32 (or int64), and not OCaml int
 - test that when allocating 4GiB, accessing index -1 fails
 - test that too-big `grow_memory` fails appropriately
 - test that too-big linear memory initial allocation fails
 - test that one can clobber the entire contents of the linear memory without corrupting: call stack, local variables, program execution.
 - test that an i64 store with 4-byte alignment that's 4 bytes out of bounds traps without storing anything.

Misc optimizer bait:
 - test that the scheduler doesn't move a trapping div past a call which may not return
 - test that linearized multidimensional array accesses can have overindexing in interesting ways
 - test that 32-bit loop induction variables that wrap aren't promoted to 64-bit
 - test that code after a non-obviously infinite loop is not executed

Misc x86 optimizer bait:
 - test that oeq handles NaN right in if, if-else, and setcc cases

SIMD (post-MVP):
 - test that SIMD insert/extract don't canonicalize NaNs
 - test that SIMD lanes are in little-endian order
 - test non-constant-index and out-of-bounds shuffle masks
 - test that subnormals work as intended
 - test that byte-misaligned accesses work

Threads (post-MVP):
 - test that thread-local variables are actually thread-local
 - test that atomic operations that isLockFree says are lock-free actually are
   (is this possible?)
 - test that isLockFree is true for datatypes that the spec says should
   always be lock-free
 - test that 16-bit and 8-bit cmpxchg does a wrapped 8-bit or 16-bit compare

FMA (post-MVP):
 - http://www.vinc17.org/software/fma-tests.c
