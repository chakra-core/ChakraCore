//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.Flag("-wasmI64");
async function main() {
  const {instance: {exports: {foo}}} = await WebAssembly.instantiate(readbuffer("binaries/bug_fitsdword.wasm"));
  foo();
}

main().then(() => console.log("PASSED"), console.log);
