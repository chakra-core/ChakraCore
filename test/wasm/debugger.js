//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const realPrint = print;
WScript.Echo = print = console.log = () => {};
if (WScript.Arguments.indexOf("-v") !== -1) {
  WScript.Echo = print = console.log = realPrint;
}

function attach() {
  return new Promise(r => WScript.Attach(() => {
    print("attached");
    r();
  }));
}
function detach() {
  return new Promise(r => WScript.Detach(() => {
    print("detached");
    r();
  }));
}
const buf = readbuffer("debugger.wasm");
const {exports: {a, b, c}} = new WebAssembly.Instance(new WebAssembly.Module(buf), {test: {
  foo: function(val) {
    print(val);
    // causes exception
    return val.b.c;
  }
}});

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
    for(let i = 0; i < 20; ++i) {
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
    for(let i = 0; i < 20; ++i) {
      runC();
    }
  })
  .then(attach)
  .then(() => {
    for(let i = 0; i < 20; ++i) {
      runC();
    }
  })
  .then(() => {
    let testValue = 0;
    const {exports: {c: newC}} = new WebAssembly.Instance(new WebAssembly.Module(buf), {test: {
      foo: function(val) {
        testValue = val;
      }
    }});
    newC(15);
    if (testValue !== 15) {
      realPrint("Invalid assignment through import under debugger");
    }
  })
  .then(detach)
  .then(() => realPrint("PASSED"), realPrint);
