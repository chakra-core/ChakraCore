//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const buf = readbuffer("debugger.wasm");
let testValue;
function createModule() {
  const {exports} = new WebAssembly.Instance(new WebAssembly.Module(buf), {test: {
    foo: function(val) {
      testValue = val;
      // causes exception
      return val.b.c;
    }
  }});
  return exports;
}

let id = 0;
function runTest(fn) {
  try {
    fn(++id|0);
  } catch (e) {
    if (!(e instanceof TypeError)) {
      print(`Unexpected error: ${e.stack}`);
    }
  } finally {
    if (testValue !== id) {
      print(`Expected ${testValue} to be ${id}`);
    }
  }
}

function runTests({a, b, c}) {
  runTest(a);
  runTest(b);
  runTest(c);
}
