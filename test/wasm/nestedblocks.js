//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.Flag("-off:wasmdeferred");
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-module-builder.js");

function test(nNestedBlocks) {
  const builder = new WasmModuleBuilder();
  const iType = builder.addType({params: [], results: [kWasmI32]});
  const body = [];
  for (let i = 0; i < nNestedBlocks; ++i) {
    body.push(kExprBlock|0, kWasmI32|0);
  }
  body.push(kExprI32Const|0, 5);
  for (let i = 0; i < nNestedBlocks; ++i) {
    body.push(kExprEnd|0);
  }
  builder
    .addFunction("foo", iType)
    .addBody(body)
    .exportFunc();
  try {
    new WebAssembly.Module(builder.toBuffer());
    return true;
  } catch (e) {
    if (e.message.includes("Maximum supported nested blocks reached")) {
      return false;
    } else {
      print(`FAILED. Unexpected error: ${e.message}`);
    }
  }
}

let blocks = 1001;
let inc = 1000;
let direction = true;

while (inc !== 0) {
  if (test(blocks)) {
    if (direction) {
      blocks += inc;
    } else {
      direction = true;
      inc >>= 1;
      blocks += inc;
    }
  } else {
    if (!direction) {
      blocks -= inc;
    } else {
      direction = false;
      inc >>= 1;
      blocks -= inc;
    }
  }

  if (blocks > 100000 || blocks < 0) {
    print(`FAILED. Nested blocks reached ${blocks} blocks deep. Expected an error by now`);
    break;
  }
}
print("PASSED");
// print(blocks);
