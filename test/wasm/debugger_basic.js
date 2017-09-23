//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const realPrint = print;
WScript.Echo = print = console.log = () => {};
if (WScript.Arguments.indexOf("--verbose") !== -1) {
  WScript.Echo = print = console.log = realPrint;
}
let isAttached = WScript.Arguments.indexOf("debuglaunch") !== -1;
function attach() {
  if (isAttached) return Promise.resolve();
  return new Promise(r => WScript.Attach(() => {
    isAttached = true;
    print("attached");
    r();
  }));
}
function detach() {
  if (!isAttached) return Promise.resolve();
  return new Promise(r => WScript.Detach(() => {
    isAttached = false;
    print("detached");
    r();
  }));
}
const buf = WebAssembly.wabt.convertWast2Wasm(`(module
  (import "test" "mem" (memory 1))
  (import "test" "table" (table 15 anyfunc))
  (import "test" "foo" (func $foo (param i32)))
  (func (export "a") (param i32)
    (call $foo (get_local 0))
  )
  (func $b (export "b") (param i32)
    (call $foo (get_local 0))
  )

  (func $c (export "c") (param i32)
    (call $d (get_local 0))
  )
  (func $d (param i32)
    (call $b (get_local 0))
  )
)`);
function makeInstance(foo) {
  const mem = new WebAssembly.Memory({initial: 1});
  const table = new WebAssembly.Table({initial: 15, element: "anyfunc"});
  const module = new WebAssembly.Module(buf);
  const instance = new WebAssembly.Instance(module, {test: {mem, table, foo}});
  debugger;
  return instance;
}
const {exports: {a, b, c}} = makeInstance(val => {
  print(val);
  // causes exception
  return val.b.c;
});

let id = 0;
function runTest(fn) {
  print(fn.name);
  fn(id++|0);
}

function caught(e) {
  if (!(e instanceof TypeError)) {
    realPrint(`Unexpected error: ${e.stack}`);
  }
}

function runA() {
  debugger;
  try {runTest(a);} catch (e) {caught(e);}
}
function runB() {
  try {runTest(b);} catch (e) {caught(e);}
}
function runC() {
  try {runTest(c);} catch (e) {caught(e);}
}

// preparse b
runB();
attach()
  .then(runA)
  .then(detach)
  .then(runA)
  .then(attach)
  .then(runB)
  .then(detach)
  .then(runB)
  .then(() => {
    let p = Promise.resolve();
    for (let i = 0; i < 20; ++i) {
      p = p
        .then(attach)
        .then(runB)
        .then(detach);
      if (i % 4 < 2) {
        p = p.then(runB);
      }
    }
    return p;
  })
  .then(() => {
    for (let i = 0; i < 20; ++i) {
      runC();
    }
  })
  .then(attach)
  .then(() => {
    for (let i = 0; i < 20; ++i) {
      runC();
    }
  })
  .then(() => {
    let testValue = 0;
    const {exports: {c: newC}} = makeInstance(val => {
      debugger;
      testValue = val;
    });
    debugger;
    newC(15);
    if (testValue !== 15) {
      realPrint("Invalid assignment through import under debugger");
    }
  })
  .then(detach)
  .then(() => realPrint("PASSED"), realPrint);
