//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var a = WScript.LoadWasmFile('basic.wast', {foo: function(a){print(a); return 2;}});

if (typeof Wasm === "undefined") {
  throw new Error("Missing Wasm Object");
}
const instantiateModuleType = typeof Wasm.instantiateModule;
if (instantiateModuleType !== "function") {
  throw new Error(`Wrong Wasm.instantiateModule type. Expected function, Got ${instantiateModuleType}`);
}
try {
  Wasm.instantiateModule();
  print("instantiateModule requires arguments");
  print("FAILED");
} catch (e) {
  print(e.message);
}


const arrBuffer = new ArrayBuffer(10);
try {
  Wasm.instantiateModule(arrBuffer);
  print("instantiateModule requires FFI");
  print("FAILED");
} catch (e) {
  print(e.message);
}

try {
  Wasm.instantiateModule(12, {});
  print("instantiateModule requires an ArrayBuffer as first argument");
  print("FAILED");
} catch (e) {
  print(e.message);
}

try {
  Wasm.instantiateModule(arrBuffer, "some string");
  print("instantiateModule requires an Object as second argument");
  print("FAILED");
} catch (e) {
  print(e.message);
}
