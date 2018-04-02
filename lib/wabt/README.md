[![Build Status](https://travis-ci.org/WebAssembly/wabt.svg?branch=master)](https://travis-ci.org/WebAssembly/wabt) [![Windows status](https://ci.appveyor.com/api/projects/status/79hqj5l0qggw645d/branch/master?svg=true)](https://ci.appveyor.com/project/WebAssembly/wabt/branch/master)

# WABT: The WebAssembly Binary Toolkit

WABT (we pronounce it "wabbit") is a suite of tools for WebAssembly, including:

 - **wat2wasm**: translate from [WebAssembly text format](https://webassembly.github.io/spec/core/text/index.html) to the [WebAssembly binary format](https://webassembly.github.io/spec/core/binary/index.html)
 - **wasm2wat**: the inverse of wat2wasm, translate from the binary format back to the text format (also known as a .wat)
 - **wasm-objdump**: print information about a wasm binary. Similiar to objdump.
 - **wasm-interp**: decode and run a WebAssembly binary file using a stack-based interpreter
 - **wat-desugar**: parse .wat text form as supported by the spec interpreter (s-expressions, flat syntax, or mixed) and print "canonical" flat format
 - **wasm2c**: convert a WebAssembly binary file to a C source and header

These tools are intended for use in (or for development of) toolchains or other
systems that want to manipulate WebAssembly files. Unlike the WebAssembly spec
interpreter (which is written to be as simple, declarative and "speccy" as
possible), they are written in C/C++ and designed for easier integration into
other systems. Unlike [Binaryen](https://github.com/WebAssembly/binaryen) these
tools do not aim to provide an optimization platform or a higher-level compiler
target; instead they aim for full fidelity and compliance with the spec (e.g.
1:1 round-trips with no changes to instructions).

## Online Demos

Wabt has been compiled to JavaScript via emscripten. Some of the functionality is available in the following demos:

- [index](https://cdn.rawgit.com/WebAssembly/wabt/aae5a4b7/demo/index.html)
- [wat2wasm](https://cdn.rawgit.com/WebAssembly/wabt/aae5a4b7/demo/wat2wasm/)
- [wasm2wat](https://cdn.rawgit.com/WebAssembly/wabt/aae5a4b7/demo/wasm2wat/)

## Cloning

Clone as normal, but don't forget to get the submodules as well:

```console
$ git clone --recursive https://github.com/WebAssembly/wabt
$ cd wabt
```

This will fetch the testsuite and gtest repos, which are needed for some tests.

## Building (macOS and Linux)

You'll need [CMake](https://cmake.org). If you just run `make`, it will run
CMake for you, and put the result in `out/clang/Debug/` by default:

> Note: If you are on macOS, you will need to use CMake version 3.2 or higher

```console
$ make
```

This will build the default version of the tools: a debug build using the Clang
compiler.

There are many make targets available for other configurations as well. They
are generated from every combination of a compiler, build type and
configuration.

 - compilers: `gcc`, `clang`, `gcc-i686`, `gcc-fuzz`
 - build types: `debug`, `release`
 - configurations: empty, `asan`, `msan`, `lsan`, `ubsan`, `no-re2c`,
   `no-tests`

They are combined with dashes, for example:

```console
$ make clang-debug
$ make gcc-i686-release
$ make clang-debug-lsan
$ make gcc-debug-no-re2c
```

You can also run CMake yourself, the normal way:

```console
$ mkdir build
$ cd build
$ cmake ..
...
```

## Building (Windows)

You'll need [CMake](https://cmake.org). You'll also need
[Visual Studio](https://www.visualstudio.com/) (2015 or newer) or 
[MinGW](http://www.mingw.org/).

You can run CMake from the command prompt, or use the CMake GUI tool. See
[Running CMake](https://cmake.org/runningcmake/) for more information.

When running from the commandline, create a new directory for the build
artifacts, then run cmake from this directory:

```console
> cd [build dir]
> cmake [wabt project root] -DCMAKE_BUILD_TYPE=[config] -DCMAKE_INSTALL_PREFIX=[install directory] -G [generator]
```

The `[config]` parameter should be a CMake build type, typically `DEBUG` or `RELEASE`.

The `[generator]` parameter should be the type of project you want to generate,
for example `"Visual Studio 14 2015"`. You can see the list of available
generators by running `cmake --help`.

To build the project, you can use Visual Studio, or you can tell CMake to do it:

```console
> cmake --build [wabt project root] --config [config] --target install
```

This will build and install to the installation directory you provided above.

So, for example, if you want to build the debug configuration on Visual Studio 2015:

```console
> mkdir build
> cd build
> cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=..\bin -G "Visual Studio 14 2015"
> cmake --build .. --config DEBUG --target install
```

## Changing the lexer

If you make changes to `src/wast-lexer.cc`, you'll need to install
[re2c](http://re2c.org). Before you upload your PR, please run `make
update-re2c` to update the prebuilt C sources in `src/prebuilt/`.

CMake will detect if you don't have re2c installed and use the prebuilt source
files instead.

## Running wat2wasm and wast2json

Some examples:

```sh
# parse and typecheck test.wat
$ out/wat2wasm test.wat

# parse test.wat and write to binary file test.wasm
$ out/wat2wasm test.wat -o test.wasm

# parse spec-test.wast, and write verbose output to stdout (including the
# meaning of every byte)
$ out/wat2wasm spec-test.wast -v

# parse spec-test.wast, and write files to spec-test.json. Modules are written
# to spec-test.0.wasm, spec-test.1.wasm, etc.
$ out/wast2json spec-test.wast -o spec-test.json
```

You can use `-h` to get additional help:

```console
$ out/wat2wasm -h
```

Or try the [online demo](https://cdn.rawgit.com/WebAssembly/wabt/aae5a4b7/demo/wat2wasm/).

## Running wasm2wat

Some examples:

```sh
# parse binary file test.wasm and write text file test.wat
$ out/wasm2wat test.wasm -o test.wat

# parse test.wasm and write test.wat
$ out/wasm2wat test.wasm -o test.wat
```

You can use `-h` to get additional help:

```console
$ out/wasm2wat -h
```

Or try the [online demo](https://cdn.rawgit.com/WebAssembly/wabt/aae5a4b7/demo/wasm2wat/).

## Running wasm-interp

Some examples:

```sh
# parse binary file test.wasm, and type-check it
$ out/wasm-interp test.wasm

# parse test.wasm and run all its exported functions
$ out/wasm-interp test.wasm --run-all-exports

# parse test.wasm, run the exported functions and trace the output
$ out/wasm-interp test.wasm --run-all-exports --trace

# parse test.json and run the spec tests
$ out/wasm-interp test.json --spec

# parse test.wasm and run all its exported functions, setting the value stack
# size to 100 elements
$ out/wasm-interp test.wasm -V 100 --run-all-exports
```

You can use `-h` to get additional help:

```console
$ out/wasm-interp -h
```

## Running the test suite

See [test/README.md](test/README.md).

## Sanitizers

To build with the [LLVM sanitizers](https://github.com/google/sanitizers),
append the sanitizer name to the target:

```console
$ make clang-debug-asan
$ make clang-debug-msan
$ make clang-debug-lsan
$ make clang-debug-ubsan
```

There are configurations for the Address Sanitizer (ASAN), Memory Sanitizer
(MSAN), Leak Sanitizer (LSAN) and Undefine Behavior Sanitizer (UBSAN). You can
read about the behaviors of the sanitizers in the link above, but essentially
the Address Sanitizer finds invalid memory accesses (use after free, access
out-of-bounds, etc.), Memory Sanitizer finds uses of uninitialized memory, 
the Leak Sanitizer finds memory leaks, and the Undefined Behavior Sanitizer
finds undefined behavior (surprise!).

Typically, you'll just want to run all the tests for a given sanitizer:

```console
$ make test-asan
```

You can also run the tests for a release build:

```console
$ make test-clang-release-asan
...
```

The Travis bots run all of these tests (and more). Before you land a change,
you should run them too. One easy way is to use the `test-everything` target:

```console
$ make test-everything
```

To run everything the Travis bots do, you can use the following scripts:

```console
$ CC=gcc scripts/travis-build.sh
$ CC=gcc scripts/travis-test.sh
$ CC=clang scripts/travis-build.sh
$ CC=clang scripts/travis-test.sh
```
