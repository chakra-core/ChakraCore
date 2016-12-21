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
}
function run() {
  ccx.runTests(exports);
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
