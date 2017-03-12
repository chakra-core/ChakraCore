This directory contains tests for the core WebAssembly semantics, as described in [Semantics.md](https://github.com/WebAssembly/design/blob/master/Semantics.md) and specified by the [spec interpreter](https://github.com/WebAssembly/spec/blob/master/interpreter/spec).

Tests are written in the [S-Expression script format](https://github.com/WebAssembly/spec/blob/master/interpreter/README.md#s-expression-syntax) defined by the interpreter.

The test suite can be run with the spec interpreter as follows:
```
./run.py --wasm <path-to-wasm-interpreter>
```
where the path points to the spec interpreter executable (or a tool that understands similar options). If the binary is in the working directory, this option can be omitted.

In addition, the option `--js <path-to-js-interpreter>` can be given to point to a stand-alone JavaScript interpreter supporting the WebAssembly API. If provided, all tests are also executed in JavaScript.
