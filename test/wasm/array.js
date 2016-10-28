//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const blob = WScript.LoadBinaryFile('array.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;
print(a["goodload"](0));
try {
  print(a["badload"](0));
}
catch(e) {
  print(e.message.includes("out of bounds") ? "PASSED" : "FAILED");
}
try {
  a["badstore"](0);
}
catch(e) {
  print(e.message.includes("out of bounds") ? "PASSED" : "FAILED");
}
