//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.Flag("-off:wasmdeferred");
function test(nNestedBlocks) {
  // print(`Test(${nNestedBlocks})`)
  let nestedBlocks = "";
  for (let i = 0; i < nNestedBlocks; ++i) {
    nestedBlocks += "(block (result i32) ";
  }
  nestedBlocks += "(i32.const 5)";
  for (let i = 0; i < nNestedBlocks; ++i) {
    nestedBlocks += ")";
  }
  const buf = WebAssembly.wabt.convertWast2Wasm(`(module
    (func (export "foo") (result i32)
      ${nestedBlocks}
    )
  )`);
  try {
    new WebAssembly.Module(buf);
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
