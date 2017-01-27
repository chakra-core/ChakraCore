//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.Flag("-wasmI64");
function createView(bytes) {
  const buffer = new ArrayBuffer(bytes.length);
  const view = new Uint8Array(buffer);
  for (let i = 0; i < bytes.length; ++i) {
    view[i] = bytes.charCodeAt(i);
  }
  return view;
}
async function main() {
  const {instance: {exports: {foo}}} = await WebAssembly.instantiate(readbuffer("binaries/bug_fitsdword.wasm"));
  foo();

  try {
    new WebAssembly.Module(createView(`\x00asm\x0d\x00\x00\x00\xff\xff\xff\xff\x7f\x00\x00\x00`));
    console.log("Should have had an error");
  } catch (e) {
    if (!(e instanceof WebAssembly.CompileError)) {
      throw e;
    }
  }
  try {
    new WebAssembly.Module(createView(`\x00asm\x0d\x00\x00\x00\x7f\x00\x00\x00`));
    console.log("Should have had an error");
  } catch (e) {
    if (!(e instanceof WebAssembly.CompileError)) {
      throw e;
    }
  }
}

main().then(() => console.log("PASSED"), console.log);
