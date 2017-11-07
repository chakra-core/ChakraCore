//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.Flag("-wasmI64");

function makeModule(type) {
  const funcName = `foo_${type}`;
  const mod = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(`
  (module
    (func (export "${funcName}") (param ${type}) (result ${type})
      (${type}.sub
        (${type}.add (get_local 0) (${type}.const 3))
        (${type}.add (get_local 0) (${type}.const 3))
      )
    )
  )`));
  const {exports} = new WebAssembly.Instance(mod);
  return exports[funcName];
}
const modules = {i32: makeModule("i32"), i64: makeModule("i64")};
function test(type, value) {
  const foo = modules[type];
  let res = foo(value);
  if (type === "i64") {
    res = res.low + res.high;
  }
  if (res !== 0) {
    print(`Error: ${type} (${value} + 3) - (${value} + 3) == ${res} !== 0`);
  }
}
test("i32", 0);
test("i32", 1);
test("i64", 0);
test("i64", 1);
test("i64", 2 * 4294967296);

print("PASSED");
