//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const module1src = `
(module
  (func (export "longsig")
    (param ${(new Array(1000)).fill("f32").join(" ")})
    (result f32)
    (f32.const 0)
  )
)`;

const module2src = `
(module
  (import "declmod" "shortsig" (func $foo (param f32) (result f32)))
)`;

const buf1 = WebAssembly.wabt.convertWast2Wasm(module1src);
const mod1 = new WebAssembly.Module(buf1);
const {exports: {longsig}} = new WebAssembly.Instance(mod1, {});

const buf2 = WebAssembly.wabt.convertWast2Wasm(module2src);
const mod2 = new WebAssembly.Module(buf2);
try {
  new WebAssembly.Instance(mod2, {declmod: {shortsig: longsig}});
  throw new Error("Expected link error");
} catch (err) {
  if (err.message !== "Cannot link import longsig[0](f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f3...)->f32 to shortsig(f32)->f32 in link table due to a signature mismatch") {
    throw err;
  }
}

const module3src = `
(module
  (import "declmod" "notLongEnoughSig"
    (func $foo
      (param ${(new Array(999)).fill("f32").join(" ")})
      (result f32)
    )
  )
)`;

const buf3 = WebAssembly.wabt.convertWast2Wasm(module3src);
const mod3 = new WebAssembly.Module(buf3);
try {
  new WebAssembly.Instance(mod3, {declmod: {notLongEnoughSig: longsig}});
  throw new Error("Expected link error");
} catch (err) {
  if (err.message !== "Cannot link import longsig[0](f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f3...)->f32 to notLongEnoughSig(f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f...)->f32 in link table due to a signature mismatch") {
    throw err;
  }
}
print("PASSED");
