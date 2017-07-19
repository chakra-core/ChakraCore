//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const cliArgs = WScript.Arguments || [];

if (cliArgs.indexOf("--help") !== -1) {
  print("usage: ch memory.js -args [-verbose] -endargs");
  WScript.quit(0);
}

// Parse arguments
let verbose = 0;
while (true) {
  const iVerbose = cliArgs.indexOf("-verbose");
  if (iVerbose === -1) {
    break;
  }
  cliArgs.splice(iVerbose, 1);
  ++verbose;
}

function testTransfer(buffer) {
  if (ArrayBuffer.transfer) {
    try {
      ArrayBuffer.transfer(buffer);
      print("Failed. Expected an error when trying to transfer ");
    } catch (e) {
      if (verbose > 1) {
        print(`Passed. Expected error: ${e.message}`);
      }
    }
  } else {
    print("ArrayBuffer.tranfer is missing");
  }
}



function test({init, max, checkOOM} = {}) {
  if (verbose) {
    print(`Testing memory {init: ${init|0}, max: ${max}}`);
  }
  const moduleTxt = `
  (module
    (memory (export "mem") ${init|0} ${max !== undefined ? max|0 : ""})
    (func (export "grow") (param i32) (result i32) (grow_memory (get_local 0)))
    (func (export "current") (result i32) (current_memory))
    (func (export "load") (param i32) (result i32) (i32.load (get_local 0)))
    (func (export "store") (param i32 i32) (i32.store (get_local 0) (get_local 1)))
  )`;
  if (verbose > 1) {
    print(moduleTxt);
  }
  const module = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(moduleTxt));
  let instance;
  try {
    instance = new WebAssembly.Instance(module);
  } catch (e) {
    if (!checkOOM || !e.message.includes("Failed to create WebAssembly.Memory")) {
      print(`FAILED. failed to instanciate module with error: ${e}`);
    } else if (verbose) {
      print(e.message);
    }
    return;
  }
  const {exports: {grow, current, load, store, mem}} = instance;
  function testReadWrite(index, value, currentSize) {
    const shouldTrap = index < 0 || (index + 4) > currentSize;
    const commonMsg = op => `trap on ${op}(${index}, ${value})`;
    try {
      store(index, value);
      if (shouldTrap) {
        print(`Failed. Expected ${commonMsg("store")}`);
        return;
      }
    } catch (e) {
      if (shouldTrap) {
        if (verbose) {
          print(`Passed. Expected ${commonMsg("store")}`);
        }
      } else {
        print(`Failed. Unexpected ${commonMsg("store")}`);
      }
    }

    try {
      const loadedValue = load(index);
      if (shouldTrap) {
        print(`Failed. Expected ${commonMsg("load")}`);
        return;
      }
      if ((loadedValue|0) !== (value|0)) {
        print(`Failed. Expected value ${value|0} after load. Got ${loadedValue|0}`);
      }
    } catch (e) {
      if (shouldTrap) {
        if (verbose) {
          print(`Passed. Expected ${commonMsg("load")}`);
        }
      } else {
        print(`Failed. Unexpected ${commonMsg("load")}`);
      }
    }
  }
  function run(delta) {
    testTransfer(mem.buffer);
    const beforePages = current();
    const growRes = grow(delta);
    if (growRes !== -1 && growRes !== beforePages) {
      print(`FAILED. Expected grow(${delta}) to return ${beforePages}`);
    }
    const afterPages = current();
    if (growRes !== -1 && beforePages + delta !== afterPages) {
      print(`FAILED. Expected to have ${beforePages + delta} pages. Got ${afterPages} pages`);
    }
    if (verbose) {
      print(`current: ${beforePages}, grow(${delta}): ${growRes}, after: ${afterPages}`);
    }
    const currentSize = afterPages * 0x10000;
    testReadWrite(-4, -4, currentSize);
    testReadWrite(-3, -3, currentSize);
    testReadWrite(-2, -2, currentSize);
    testReadWrite(-1, -1, currentSize);
    testReadWrite(0, 6, currentSize);
    testTransfer(mem.buffer);
    testReadWrite(1, 7, currentSize);
    testTransfer(mem.buffer);
    testReadWrite(1, 7, currentSize);
    testReadWrite(currentSize - 4, 457, currentSize);
    testReadWrite(currentSize - 3, -98745, currentSize);
    testReadWrite(currentSize - 2, 786452, currentSize);
    testReadWrite(currentSize - 1, -1324, currentSize);
    testReadWrite(currentSize, 123, currentSize);
    testTransfer(mem.buffer);
  }
  run(0);
  run(3);
  run(5);
  run(1 << 13);
}

test({init: 0});
test({init: 0, max: 5});
test({init: 0, max: 10});
test({init: 5});
test({init: 5, max: 10});
// test({init: 1 << 14, checkOOM: true}); // ArrayBuffer will throw OOM instead of returning a null buffer
try {
  test({init: 1 << 15});
  print("Failed. Expected an error when allocating WebAssembly.Memory too big");
} catch (e) {
  if (verbose) {
    print(`Passed. Expected error: ${e.message}`);
  }
}
print("PASSED");
