//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const buf = WebAssembly.wabt.convertWast2Wasm(`(module
  (import "test" "foo" (func $foo (param i32)))
  (func $a (export "a") (param i32)
    (call $foo (get_local 0))
  )
  (func $c (export "c") (param i32)
    (call $d (get_local 0))
  )
  (func $d (param i32)
    (call $a (get_local 0))
  )
)`);
var testValue;
class MyException extends Error {}
var MyExceptionExport = MyException;
var lastModule;
var lastInstance;
var mem = new WebAssembly.Memory({initial: 1});
var table = new WebAssembly.Table({element: "anyfunc", initial: 15});

function createModule() {
  lastModule = new WebAssembly.Module(buf);
  lastInstance = new WebAssembly.Instance(lastModule, {test: {
    foo: function(val) {
      testValue = val;
      throw new MyException();
    }
  }});
  return lastInstance.exports;
}
