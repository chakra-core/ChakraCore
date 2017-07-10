//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var {fixupI64Return} = WScript.LoadScriptFile("./wasmutils.js");
WScript.Flag("-wasmI64");

let passed = true;
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


let ffi = {};
var mod = new WebAssembly.Module(readbuffer('math.wasm'));
var exports = new WebAssembly.Instance(mod, ffi).exports;
check(0, "exports.ctz", 1);
check(2, "exports.ctz", 4);
check(31, "exports.ctz", -Math.pow(2,31));
check(32, "exports.ctz", 0);

//i32 div/rem
check(2,"exports.i32_div_s_by_two", 5);
check("Overflow","exports.i32_div_s_over", 5);
check("Overflow","exports.i32_div_s_over", 5, 0);
check("Division by zero","exports.i32_div_s", 5, 0);
check("Overflow","exports.i32_div_s", -2147483648, -1);
check("Division by zero","exports.i32_div_u", 5, 0);
check("Division by zero","exports.i32_rem_u", 5, 0);
check("Division by zero","exports.i32_rem_s", 5, 0);
check(-2,"exports.i32_div_s", 5, -2);
check(2,"exports.i32_div_u", 5, 2);
check(0,"exports.i32_rem_u", 5, 1);
check(0,"exports.i32_rem_s", 5, -1);
check(1,"exports.i32_rem_s", 5, 2);

fixupI64Return(exports, Object.keys(exports).filter(key => key.toLowerCase().includes("i64")));

//i64 div/rem
check("Overflow","exports.i64_div_s_over");
check("Division by zero","exports.i64_div_s", 5, 0);
check("Division by zero","exports.i64_div_u", 5, 0);
check("Division by zero","exports.i64_rem_s", 5, 0);
check("Division by zero","exports.i64_rem_u", 5, 0);
check("0xfffffffffffffffe","exports.i64_div_s", 5, -2);
check("0x2","exports.i64_div_u", 5, 2);
check("0x0","exports.i64_rem_s", 5, -1);
check("0xc","exports.i64_rem_u", -4, 16);

check("0x40", "exports.ctzI64", 0);
check("0x40", "exports.ctzI64", "0");
check("0x0", "exports.ctzI64", "1");
check("0x1f", "exports.ctzI64", "" + -Math.pow(2,31));
check("0x3a", "exports.ctzI64", "0x3400000000000000");
check("0x3f", "exports.ctzI64", "-9223372036854775808");

if(passed) {
  print("Passed");
}
