//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
passed = true;
function check(expected, funName, ...args)
{
  let fun = eval(funName);
  var result;
  try {
     result = fun(...args);
  } catch (e) {
    result = e.message;
  }

  if(result != expected) {
    passed = false;
    print(`${funName}(${[...args]}) produced ${result}, expected ${expected}`);
  }
}

var mod = new WebAssembly.Module(readbuffer('i64cf.wasm'));
var a = new WebAssembly.Instance(mod).exports;

check(21, "a.while");

if (passed)
{
    print("Passed");
}
