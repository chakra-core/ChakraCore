//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.Flag("-wasmIgnoreResponse");

const setTimeout = WScript.SetTimeout;
// Shim the response api
class Response {
  constructor(buffer, shouldFail) {
    this.buffer = buffer;
    this.shouldFail = shouldFail;
  }
  arrayBuffer() {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        if (this.shouldFail) {
          return reject(new Error("Unable to fetch the buffer"));
        }
        resolve(this.buffer);
      }, 100);
    });
  }
}
function fetch(getBuffer) {
  return new Promise((resolve, reject) => {
    setTimeout(() => {
      try {
        resolve(new Response(getBuffer()));
      } catch (e) {
        reject(e);
      }
    }, 100);
  });
}

const defaultModule = WebAssembly.wabt.convertWast2Wasm(`
(module
  (func (export "foo") (result i32) (i32.const 5))
)`);
function validateDefaultInstance({exports: {foo}}) {
  const res = foo();
  if (res !== 5) {
    throw new Error(`Bad result: ${res} !== 5`);
  }
}
function validateDefaultCompile(module) {
  validateDefaultInstance(new WebAssembly.Instance(module));
}
function validateDefaultInstantiate({module, instance}) {
  validateDefaultCompile(module);
  validateDefaultInstance(instance);
}

const defaultImportModule = WebAssembly.wabt.convertWast2Wasm(`
(module
  (global $iImport (import "test" "i32") i32)
  (import "test" "foo" (func $foo (result i32)))
  (func (export "foo") (result i32) (call $foo) (get_global $iImport) (i32.add))
)`);
const defaultImportObject = {test: {i32: 3, foo: () => -2}};
function validateDefaultImportInstance({exports: {foo}}) {
  const res = foo();
  const expected = (defaultImportObject.test.i32 + defaultImportObject.test.foo())|0;
  if (res !== expected) {
    throw new Error(`Bad result: ${res} !== ${expected}`);
  }
}
function validateDefaultImportInstantiate({module, instance}) {
  validateDefaultImportInstance(new WebAssembly.Instance(module, defaultImportObject));
  validateDefaultImportInstance(instance);
}

const tests = [];
let allTestsQueued = false;
const allTestsDone = () => !tests.find(info => info.state !== "done");
function checkIfAllDone() {
  if (allTestsQueued && allTestsDone()) {
    report();
  }
}
function test(fn, description) {
  const testInfo = {
    state: "pending",
    description
  };
  tests.push(testInfo);
  const done = () => {
    testInfo.state = "done";
    checkIfAllDone();
  };

  try {
    testInfo.state = "testing";
    const p = fn();
    p.then(() => {
      testInfo.result = "Passed";
    }, e => {
      testInfo.result = "Failed";
      testInfo.error = e;
    }).then(done);
  } catch (e) {
    // If fn doesn't return a Promise we will throw here
    // WebAssembly.compile should always return a Promise
    testInfo.result = "Failed";
    testInfo.error = e;
    done();
  }
}

function shouldThrow() {
  throw new Error("Should have had an error");
}
function ignoreError() {}

test(() =>
  WebAssembly.compile(defaultModule)
    .then(validateDefaultCompile),
  "WebAssembly.compile(buffer => defaultModule)"
);
test(() =>
  WebAssembly.compile(new Promise((resolve, reject) => setTimeout(() => reject(new Error("Some Error")), 100)))
    .then(shouldThrow, ignoreError),
  "WebAssembly.compile(Promise => timeout Error)"
);
test(() =>
  WebAssembly.compile(Promise.reject())
    .then(shouldThrow, ignoreError),
  "WebAssembly.compile(Promise => Empty Error)"
);
test(() =>
  WebAssembly.compile(Promise.reject(new Error("Promise.reject(new Error()")))
    .then(shouldThrow, ignoreError),
  "WebAssembly.compile(Promise => Error)"
);
test(() =>
  WebAssembly.compile(Promise.resolve("Wrong type"))
    .then(shouldThrow, ignoreError),
  "WebAssembly.compile(Promise => string)"
);
test(() =>
  WebAssembly.compile(fetch(() => defaultModule))
    .then(validateDefaultCompile),
  "WebAssembly.compile(fetch => defaultModule)"
);
test(() =>
  WebAssembly.compile(fetch(() => {throw new Error("Failed to fetch");}))
    .then(shouldThrow, ignoreError),
  "WebAssembly.compile(fetch => Error)"
);
test(() =>
  WebAssembly.compile(new Response(defaultModule))
    .then(validateDefaultCompile),
  "WebAssembly.compile(Response => defaultModule)"
);
test(() =>
  WebAssembly.compile(new Response(defaultModule, true))
    .then(shouldThrow, ignoreError),
  "WebAssembly.compile(Response => Error)"
);

test(() =>
  WebAssembly.instantiate(fetch(() => defaultModule))
    .then(validateDefaultInstantiate),
  "WebAssembly.instantiate(fetch => defaultModule)"
);
test(() =>
  WebAssembly.instantiate(fetch(() => {throw new Error("Failed to fetch");}))
    .then(shouldThrow, ignoreError),
  "WebAssembly.instantiate(fetch => Error)"
);
test(() =>
  WebAssembly.instantiate(new Response(defaultModule))
    .then(validateDefaultInstantiate),
  "WebAssembly.instantiate(Response => defaultModule)"
);
test(() =>
  WebAssembly.instantiate(new Response(defaultModule, true))
    .then(shouldThrow, ignoreError),
  "WebAssembly.instantiate(Response => Error)"
);

test(() =>
  WebAssembly.instantiate(fetch(() => defaultImportModule), defaultImportObject)
    .then(validateDefaultImportInstantiate),
  "WebAssembly.instantiate(fetch => defaultImportModule, defaultImportObject)"
);
test(() =>
  WebAssembly.instantiate(new Response(defaultImportModule), defaultImportObject)
    .then(validateDefaultImportInstantiate),
  "WebAssembly.instantiate(Response => defaultImportModule, defaultImportObject)"
);

allTestsQueued = true;
// In case all the test ran synchronously
checkIfAllDone();

function report() {
  const status = {};
  tests.forEach(info => {
    status[info.result] = (status[info.result]|0) + 1;
    const error = info.error ? "\n" + info.error.stack : "";
    console.log(`${info.result}: ${info.description}${error}`);
  });
  const statusReport = Object.keys(status)
    .map(state => `${state}=${status[state]}`)
    .join(", ");
  console.log(`Report: ${statusReport}`);
}
