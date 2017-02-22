This is a rough list of "tests to write". Everything here should either be
specified in [Semantics.md](https://github.com/WebAssembly/design/blob/master/Semantics.md),
have a link to an open issue/PR, or be obvious. Comments/corrections/additions
welcome.

Operator semantics:
 - ~~test that promote/demote is bit-preserving if not NaN~~
 - ~~test that clz/ctz handle zero~~
 - ~~test that numbers slightly outside of the int32 range round into the int32 range in floating-to-int32 conversion~~
 - ~~test that neg, abs, copysign, reinterpretcast, store+load, set+get, preserve the sign bit and significand bits of NaN and don't canonicalize~~
 - ~~test that shifts don't mask their shift count. 32 is particularly nice to test.~~
 - ~~test that `page_size` returns a power of 2~~
 - ~~test that arithmetic operands are evaluated left-to-right~~
 - ~~test that call and store operands are evaluated left-to-right too~~
 - ~~test that call and argument operands of call_indirect are evaluated left-to-right, too~~
 - ~~test that select arguments are evaluated left-to-right, too~~
 - ~~test that br_if arguments are evaluated left-to-right, too~~
 - ~~test that br_table arguments are evaluated left-to-right, too~~
 - ~~test that add/sub/mul/wrap/wrapping-store silently wrap on overflow~~
 - ~~test that sdiv/udiv/srem/urem trap on divide-by-zero~~
 - ~~test that sdiv traps on overflow~~
 - ~~test that srem doesn't trap when the corresponding sdiv would overflow~~
 - ~~test that float-to-integer conversion traps on overflow and invalid~~
 - ~~test that unsigned operations are properly unsigned~~
 - ~~test that signed integer div rounds toward zero~~
 - ~~test that signed integer mod has the sign of the dividend~~
 - ~~test that select preserves all NaN bits~~

Floating point semantics:
 - ~~test for round-to-nearest rounding~~
 - ~~test for ties-to-even rounding~~
 - ~~test that all operations with floating point inputs correctly handle all their NaN, -0, 0, Infinity, and -Infinity special cases~~
 - ~~test that signaling NaN is indistinguishable from quiet NaN~~
 - ~~test that all operations that can overflow produce Infinity and with the correct sign~~
 - ~~test that all operations that can divide by zero produce Infinity with the correct sign~~
 - ~~test that all operations that can have an invalid produce NaN~~
 - ~~test that all operations that can have underflow behave correctly~~
 - ~~test that nearestint doesn't do JS-style Math.round or C-style round(3) rounding~~
 - ~~test that signalling NaN doesn't cause weirdness~~
 - ~~test that signalling/quiet NaNs can have sign bits and payloads in literals~~
 - ~~test that conversion from int32/int64 to float32 rounds correctly~~
 - ~~test that [relaxed optimizations](https://gcc.gnu.org/wiki/FloatingPointMath) are not done~~

Linear memory semantics:
 - test that loading from null works
 - ~~test that loading from constant OOB traps and is not DCE'd or folded (pending [discussion](https://github.com/WebAssembly/design/blob/master/Semantics.md#out-of-bounds))~~
 - test that loading from "beyond the STACKPTR" succeeds
 - test that "stackptr + (linearmemptr - stackptr)" loads from linearmemptr.
 - test loading "uninitialized" things from aliased stack frames return what's there
 - test that loadwithoffset traps in overflow cases
 - test that newly allocated memory (program start and `grow_memory`) is zeroed
 - test that `grow_memory` does a full 32-bit unsigned check for page-size divisibility
 - test that load/store addreses are full int32 (or int64), and not OCaml int
 - test that when allocating 4GiB, accessing index -1 fails
 - ~~test that linear memory is little-endian for all integers and floats~~
 - test that unaligned and misaligned accesses work, even if slow
 - ~~test that runaway recursion traps~~
 - test that too-big `grow_memory` fails appropriately
 - test that too-big linear memory initial allocation fails
 - ~~test that non-pagesize `grow_memory` fails~~
 - test that one can clobber the entire contents of the linear memory without corrupting: call stack, local variables, program execution.
 - test that an i64 store with 4-byte alignment that's 4 bytes out of bounds traps without storing anything.

Function pointer semantics:
 - test that function addresses are monotonic indices, and not actual addresses.
 - ~~test that function pointers work [correctly](https://github.com/WebAssembly/design/issues/89)~~

Expression optimizer bait:
 - ~~test that `a+1<b+1` isn't folded to `a<b`~~
 - ~~test that that demote-promote, wrap+sext, wrap+zext, shl+ashr, shl+lshr, div+mul, mul+div aren't folded away~~
 - ~~test that converting int32 to float and back isn't folded away~~
 - ~~test that converting int64 to double and back isn't folded away~~
 - ~~test that `float(double(float(x))+double(y))` is not `float(x)+float(y)` (and so on for other operators)~~
 - ~~test that `x*0.0` is not folded to `0.0`~~
 - ~~test that `0.0/x` is not folded to `0.0`~~
 - ~~test that `x != x` is not folded to false, `x == x` is not folded to true, `x < x` is not folded to false, etc.~~
 - ~~test that signed integer div of negative by constant power of 2 is not ashr~~
 - ~~test unsigned and signed division by 3, 5, 7~~
 - ~~test that floating-point division by immediate 0 and -0 is defined~~
 - ~~test that floating-point (x*y)/y isn't folded to x~~
 - ~~test that floating-point (x+y)-y isn't folded to x~~
 - ~~test that ult/ugt/etc (formed with a not operator) aren't folded to oge/ole/etc.~~
 - ~~test that floating point add/mul aren't reassociated even when tempting~~
 - ~~test that floating point mul+add isn't folded to fma even when tempting~~
 - ~~test that floating point sqrt(x*x+y*y) isn't folded to hypot even when tempting~~
 - ~~test that 1/x isn't translated into reciprocal-approximate~~
 - ~~test that 1/sqrt(x) isn't approximated either~~
 - ~~test that fp division by non-power-2 constant gets full precision (isn't a multiply-by-reciprocal deal)?~~
 - ~~test that x<y?x:y is not folded to min, etc.~~

Misc optimizer bait:
 - ~~test that the impl doesn't constant-fold away or DCE away or speculate operations that should trap, such as `1/0u`, `1/0`, `1%0u`, `1%0, convertToInt(NaN)`, `INT_MIN/-1` and so on.~~
 - test that likely constant folding uses the correct rounding mode
 - test that the scheduler doesn't move a trapping div past a call which may not return
 - ~~test that redundant-load elimination, dead-store elimination, and/or store+load forwarding correctly respect interfering stores of different types (aka no TBAA)~~
 - test that linearized multidimensional array accesses can have overindexing in interesting ways
 - test that 32-bit loop induction variables that wrap aren't promoted to 64-bit
 - test that functions with C standard library names aren't assumed to have C standarad library semantics
 - test that code after a non-obviously infinite loop is not executed

Misc x86 optimizer bait:
 - test that oeq handles NaN right in if, if-else, and setcc cases

Misc x87-isms:
 - ~~test for invalid Precision-Control-style x87 math~~
 - ~~test for invalid -ffloat-store-style x87 math~~
 - ~~test for evaluating intermediate results at greater precision~~
 - ~~test for loading and storing NaNs~~

Validation errors:
 - ~~sign-extend load from int64 to int32 etc.~~
 - ~~fp-promote load and fp-demote store~~
 - alignment greater than the size of a load or store (https://github.com/WebAssembly/spec/issues/302)

SIMD (post-MVP):
 - test that SIMD insert/extract don't canonicalize NaNs
 - test that SIMD lanes are in little-endian order
 - test non-constant-index and out-of-bounds shuffle masks
 - test that subnormals work as intended
 - test that byte-misaligned accesses work

Threads (post-MVP):
 - test that thread-local variables are actually thread-local
 - test that 8-bit, 16-bit, 32-bit, and 64-bit (on wasm64) atomic operations
   are lock-free (is this possible?)
