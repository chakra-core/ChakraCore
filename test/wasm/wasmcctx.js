//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function attach() {
  return new Promise(r => WScript.Attach(() => {
    r();
  }));
}
function detach() {
  return new Promise(r => WScript.Detach(() => {
    r();
  }));
}

const ccx = WScript.LoadScriptFile("wasmcctxmodule.js", "samethread");
let exports;
function createModule() {
  exports = ccx.createModule();
  const {mem, table, lastModule, lastInstance} = ccx;
  /**bp:locals();bp:evaluate('String(lastInstance)');bp:evaluate('String(lastModule)');bp:evaluate('String(mem)');bp:evaluate('String(table)')**/
}

let id = 0;
function runTest(fn) {
  try {
    //debugger;
    fn(++id|0);
  } catch (e) {
    if (!(e instanceof (ccx.MyExceptionExport))) {
      print(`Unexpected error: ${e.stack}`);
    }
  } finally {
    if (ccx.testValue !== id) {
      print(`Expected ${ccx.testValue} to be ${id}`);
    }
  }
}

function runTests({a, c}) {
  runTest(a);
  runTest(c);
}

function run() {
  runTests(exports);
}

createModule();
run();
attach()
  .then(createModule)
  .then(detach)
  .then(run)
  .then(attach)
  .then(createModule)
  .then(run)
  .then(detach)
  .then(() => print("PASSED"), print);
