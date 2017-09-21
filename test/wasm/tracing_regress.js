//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("../WasmSpec/testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("../WasmSpec/testsuite/harness/wasm-module-builder.js");

const tracingPrefix = 0xf0;
const builder = new WasmModuleBuilder();
const typeIndex = builder.addType(kSig_v_v);
builder.addFunction("foo", typeIndex).addBody([
  tracingPrefix, 0x0,
]).exportFunc();

try {
  const {exports: {foo}} = new WebAssembly.Instance(new WebAssembly.Module(builder.toBuffer()));
  foo();
  console.log("Failed: Tracing prefix should not be accepted");
} catch (e) {
  if (e.message.includes("Tracing opcodes not allowed")) {
    console.log("pass");
  } else {
    console.log("Wrong error: " + e);
  }
}
