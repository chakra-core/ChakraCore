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
  const {instance: {exports: {foo, bar}}} = await WebAssembly.instantiate(readbuffer("binaries/bug_fitsdword.wasm"));
  foo();
  bar();

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

  {
    const mod = new WebAssembly.Module(readbuffer("binaries/bugDeferred.wasm"));
    const instance1 = new WebAssembly.Instance(mod);
    const instance2 = new WebAssembly.Instance(mod);

    // Change the type of the function on the first instance
    instance1.exports.foo.asdf = 5;
    instance1.exports.foo();
    // Make sure the entrypoint has been correctly updated on the second instance
    instance2.exports.foo();
  }
}

main().then(() => console.log("PASSED"), console.log);
