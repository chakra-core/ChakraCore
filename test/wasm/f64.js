//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var mod = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(`
(module
  (func (export "min") (param f64 f64) (result f64)
    (f64.min (get_local 0) (get_local 1))
  )

  (func (export "max") (param f64 f64) (result f64)
    (f64.max (get_local 0) (get_local 1))
  )
)`));
var {min, max} = new WebAssembly.Instance(mod).exports;

function test(fn, expectedRes, ...args) {
  const res = fn(...args);
  if (isNaN(expectedRes)) {
    if (!isNaN(res)) {
      console.log(`Failed: ${fn}(${args.join(", ")}) expected NaN. Got ${res}`);
    }
  } else if (Object.is(expectedRes, -0)) {
    if (!Object.is(res, -0)) {
      console.log(`Failed: ${fn}(${args.join(", ")}) expected -0. Got ${res}`);
    }
  } else if (res !== expectedRes) {
      console.log(`Failed: ${fn}(${args.join(", ")}) expected ${Math.fround(expectedRes)}. Got ${res}`);
  }
}

function testMin(low, high) {
  test(min, low, low, high);
  test(min, low, high, low);
}
function testMax(low, high) {
  test(max, high, low, high);
  test(max, high, high, low);
}
function testMinMax(low, high) {
  testMin(low, high);
  testMax(low, high);
}

testMinMax(11, 11.01);
testMinMax(11.01, 1/0);
testMinMax(-0, 0);
testMin(NaN, 11.01);
testMin(NaN, 11);
testMax(11.01, NaN);
testMax(-NaN, NaN);

console.log("PASSED");