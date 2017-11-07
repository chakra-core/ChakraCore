//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var {fixupI64Return} = WScript.LoadScriptFile("./wasmutils.js");
const cases = {
  basic: 0,
  export: 1,
  exportMut: 2,
  mutable: 3,
  import: 4,
  impInit: 5,
  impInitMut: 6,
  count: 7,
};
const casesNames = Object.keys(cases);

const invalidCases = {
  i32: [cases.exportMut, cases.impInitMut],
  i64: [cases.export, cases.exportMut, cases.import, cases.impInit, cases.impInitMut],
  f32: [cases.exportMut, cases.impInitMut],
  f64: [cases.exportMut, cases.impInitMut],
};

const mod1 = new WebAssembly.Module(readbuffer("binaries/global.wasm"));
const {exports} = new WebAssembly.Instance(mod1, {test: {
  i32: 234,
  i64: () => console.log("shouldn't use i64 import"),
  f32: 8.47,
  f64: 78.145
}});
fixupI64Return(exports, "get-i64");

function printAllGlobals(type) {
  console.log(`Print all ${type}`);
  const getter = exports[`get-${type}`];
  // print exported global
  console.log(`exported ${type}: ${exports[type]}`);
  for(let iCase = 0; iCase < cases.count; ++iCase) {
    const caseName = casesNames[iCase];
    try {
      const val = getter(iCase);
      console.log(`${caseName}: ${val}`);
    } catch (e) {
      if (!(e instanceof WebAssembly.RuntimeError && invalidCases[type].includes(iCase))) {
        console.log(`${caseName}: Unexpected error thrown: ${e}`);
      }
    }
  }
  console.log("")
}

["i32", "i64", "f32", "f64"].forEach(printAllGlobals);
console.log("Modify mutable globals");
exports["set-i32"](456789);
exports["set-i64"]({high: -0xD2A08, low: 0x70000000});
exports["set-f32"](45.78);
exports["set-f64"](65.7895);
["i32", "i64", "f32", "f64"].forEach(printAllGlobals);

console.log("Invalid cases");
const mod3 = new WebAssembly.Module(readbuffer("binaries/i64_invalid_global_import.wasm"));
try {
  new WebAssembly.Instance(mod3, {test: {global: 5}});
  console.log("should have trap");
} catch (e) {
  if (e instanceof TypeError) {
    console.log(`Should be invalid type conversion: ${e.message}`);
  } else {
    console.log(`Invalid error ${e}`);
  }
}

const mod4 = new WebAssembly.Module(readbuffer("binaries/i64_invalid_global_export.wasm"));
try {
  new WebAssembly.Instance(mod4, {});
  console.log("should have trap");
} catch (e) {
  if (e instanceof TypeError) {
    console.log(`Should be invalid type conversion: ${e.message}`);
  } else {
    console.log(`Invalid error ${e}`);
  }
}

try {
  const mod5 = new WebAssembly.Module(readbuffer("binaries/invalid_global_init.wasm"));
  console.log("should have trap");
} catch (e) {
  if (e instanceof WebAssembly.CompileError) {
    console.log(`Should be invalid init expr: ${e.message}`);
  } else {
    console.log(`Invalid error ${e}`);
  }
}
