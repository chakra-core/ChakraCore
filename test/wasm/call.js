//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const verbose = false;
const defaultffi = {spectest: {print}};

main().catch(e => print(e));
async function main() {
  print("Compile call.0.wasm");
  const mod1 = await WebAssembly.compile(readbuffer("binaries/call.0.wasm"));
  print("Instanciate call.0.wasm");
  const inst1 = new WebAssembly.Instance(mod1, {});
  testInstance(inst1);

  print("Compile call.1.wasm");
  const mod2 = new WebAssembly.Module(readbuffer("binaries/call.1.wasm"));
  print("Instanciate call.1.wasm");
  const inst2 = new WebAssembly.Instance(mod2, Object.assign({}, defaultffi, {test: inst1.exports}));

  testInstance(inst2);

  const overwriteMethods = {
    "i_func-i32": print,
    "i_func-i64": () => print("Should have trapped"),
    "i_func-f32": print,
    "i_func-f64": print,
    "i_func->i32": () => 5,
    "i_func->i64": () => print("Will trap when converting return value"),
    "i_func->f32": () => 3.14,
    "i_func->f64": () => -5.47,
    "i_func-i32->i32": a => a - 1,
    "i_func-i64->i64": () => print("Should have trapped"),
    "i_func-f32->f32": a => a + 1.4,
    "i_func-f64->f64": a => a * 2,
  };
  const imports = Object.assign({}, inst1.exports, overwriteMethods);
  print("Instanciate call.1.wasm");
  const inst3 = new WebAssembly.Instance(mod2, Object.assign({}, defaultffi, {test: imports}));

  const tests = [
    {run: invoke, name: "func-i32", params: [48]},
    {run: assertTrap, name: "func-i64"},
    {run: invoke, name: "func-f32", params: [5.148]},
    {run: invoke, name: "func-f64", params: [8.476]},
    {run: assertReturn, name: "func->i32", check: a => a === 5},
    {run: assertTrap, name: "func->i64"},
    {run: assertReturn, name: "func->f32", check: a => Math.fround(a) === Math.fround(3.14)},
    {run: assertReturn, name: "func->f64", check: a => a === -5.47},
    {run: assertReturn, name: "func-i32->i32", params: [7], check: a => a === 6},
    {run: assertTrap, name: "func-i64->i64"},
    {run: assertReturn, name: "func-f32->f32", params: [2.6], check: a => Math.fround(a) === Math.fround(4.0)},
    {run: assertReturn, name: "func-f64->f64", params: [6], check: a => a === 12},
    {run: assertReturn, name: "i32", params: [4], check: a => a === (4 - 1 + 5)},
    {run: assertTrap, name: "i64"},
    {run: assertReturn, name: "f32", params: [25.1], check: a => Math.fround(a) === Math.fround(3.14 + 1.4 + 25.1)},
    {run: assertReturn, name: "f64", params: [45], check: a => a === (45 * 2 - 5.47)},
  ];

  let testsPassed = 0;
  for (const test of tests) {
    const {run, name, params = [], check} = test;
    const testName = `js-${name}`;
    try {
      testsPassed += +run(inst3.exports[name].bind(null, ...params), testName, check);
    } catch (e) {
      print(`${testName} failed. unexpectedly threw: ${e}`);
    }
  }
  print(`${testsPassed}/${tests.length} tests passed\n\n`);
}

function invoke(method) {
  method();
  return true;
}

function testInstance(instance) {
  const {exports} = instance;
  const tests = retrieveTestMethods(exports);
  let testsPassed = 0;
  for (const methodName of tests) {
    const method = exports[methodName];
    try {
      if (methodName.startsWith("$assert_return")) {
        testsPassed += +assertReturn(method, methodName, a => a === 1);
      } else if (methodName.startsWith("$assert_trap")) {
        testsPassed += +assertTrap(method, methodName);
      } else if (methodName.startsWith("$action")) {
        testsPassed += +invoke(method);
      }
    } catch (e) {
      print(`${methodName} failed. unexpectedly threw: ${e}`);
    }
  }
  print(`${testsPassed}/${tests.length} tests passed\n\n`);
}

function assertReturn(method, methodName, check) {
  const val = method();
  if (!check(val)) {
    print(`${methodName} failed.`);
  } else {
    if (verbose) {
      print(`${methodName} passed.`);
    }
    return true;
  }
}

function assertTrap(method, methodName) {
  try {
    method();
    print(`${methodName} failed. Expected an error to be thrown`);
  } catch (e) {
    if (e instanceof WebAssembly.RuntimeError || e instanceof TypeError) {
      if (verbose) {
        print(`${methodName} passed. Error thrown: ${e}`);
      }
      return true;
    }
    throw e;
  }
}

function retrieveTestMethods(exports) {
  return Object.keys(exports)
    .map(key => /_(\d+)$/.exec(key))
    .filter(m => !!m)
    .map(m => ({order: parseInt(m[1]), key: m.input}))
    .sort((a, b) => a.order - b.order)
    .map(({key}) => key);
}
