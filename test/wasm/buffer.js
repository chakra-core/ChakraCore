//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function scope() {
  // Use a function so the original buffer gets collected once we exit that scope
  const buf = readbuffer('buffer.wasm');
  const view = new Uint8Array(buf);
  const {exports: {a, b}} = Wasm.instantiateModule(view, {});
  // Change the buffer after creating the wasm module
  view.fill(0);
  return {a, b};
}

const {a, b} = scope();
CollectGarbage();
const res1 = a(1.4, 2.47);
CollectGarbage();
const res2 = b(-1.47, 5);
CollectGarbage();
print(res1 !== Math.fround(1.4) || res2 !== Math.fround(-1.47) ? "FAILED" : "PASSED");
