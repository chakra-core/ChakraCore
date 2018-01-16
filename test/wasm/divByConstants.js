//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var {fixupI64Return} = WScript.LoadScriptFile("./wasmutils.js");
WScript.Flag("-wasmI64");

var CreateBaseline = false; // Set True to generate Baseline data. Initialize results array with console output.
var debug = false; // True for printing debug messages
var debugTestNum = -1; // Set test number to run a specific test. Set to default -1 otherwise.

var test_values = [-5, 5, 124, 248, 654, 987, -1026, 98768, -88754, 1<<32, -(1<<32),  (1<<32)-1, (1<<31)-1, -(1<<31)+1, 1<<25, -1<<25, 65536, 46341];

var results = 
    [-5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,2147483647,-2147483647,33554432,-33554432,65536,46341,5,-5,-124,-248,-654,-987,1026,-98768,88754,-1,1,0,-2147483647,2147483647,
    -33554432,33554432,-65536,-46341,-1,1,41,82,218,329,-342,32922,-29584,0,0,0,715827882,-715827882,11184810,-11184810,21845,15447,-1,1,24,49,130,197,-205,19753,-17750,
    0,0,0,429496729,-429496729,6710886,-6710886,13107,9268,0,0,17,35,93,141,-146,14109,-12679,0,0,0,306783378,-306783378,4793490,-4793490,9362,6620,1,-1,-41,-82,-218,-329,
    342,-32922,29584,0,0,0,-715827882,715827882,-11184810,11184810,-21845,-15447,1,-1,-24,-49,-130,-197,205,-19753,17750,0,0,0,-429496729,429496729,-6710886,6710886,-13107,-9268,
    0,0,-17,-35,-93,-141,146,-14109,12679,0,0,0,-306783378,306783378,-4793490,4793490,-9362,-6620,0,0,15,31,81,123,-128,12346,-11094,0,0,0,268435455,-268435455,4194304,
    -4194304,8192,5792,0,0,13,27,72,109,-114,10974,-9861,0,0,0,238609294,-238609294,3728270,-3728270,7281,5149,0,0,9,19,50,75,-78,7597,-6827,0,0,0,165191049,-165191049,2581110,
    -2581110,5041,3564,0,0,3,7,19,29,-31,2992,-2689,0,0,0,65075262,-65075262,1016800,-1016800,1985,1404,0,0,2,4,10,16,-17,1646,-1479,0,0,0,35791394,-35791394,559240,-559240,1092,
    772,0,0,1,2,6,9,-10,987,-887,0,0,0,21474836,-21474836,335544,-335544,655,463,0,0,1,2,5,8,-8,823,-739,0,0,0,17895697,-17895697,279620,-279620,546,386,0,0,0,0,0,0,
    -1,98,-88,0,0,0,2145338,-2145338,33520,-33520,65,46,0,0,0,0,0,0,0,9,-8,0,0,0,214769,-214769,3355,-3355,6,4,0,0,0,0,0,0,0,0,0,0,0,0,21475,-21475,335,-335,0,0,5,-5,-124,
    -248,-654,-987,1026,-98768,88754,-1,1,0,-2147483647,2147483647,-33554432,33554432,-65536,-46341,5,-5,-124,-248,-654,-987,1026,-98768,88754,-1,1,0,-2147483647,2147483647,
    -33554432,33554432,-65536,-46341,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-5,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,1,0,0,0,0,-2,2,1,2,0,0,0,2,-2,1,-1,0,1
    ,-1,2,-2,1,0,0,0,4,3,4,2,-1,3,-4,1,-1,0,2,-2,2,-2,1,1,-5,5,5,3,3,0,-4,5,-1,1,-1,0,1,-1,2,-2,2,1,-2,2,1,2,0,0,0,2,-2,1,-1,0,1,-1,2,-2,1,0,0,0,4,3,4,2,-1,3,-4,1,-1,0,2,-2,
    2,-2,1,1,-5,5,5,3,3,0,-4,5,-1,1,-1,0,1,-1,2,-2,2,1,-5,5,12,10,10,7,-4,12,-8,1,-1,0,1,-1,2,-2,2,1,-5,5,13,26,25,25,-27,15,-28,1,-1,0,21,-21,20,-20,9,17,-5,5,4,8,54,27,-66,
    8,-74,1,-1,0,7,-7,32,-32,16,21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,2147483647,-2147483647,33554432,-33554432,65536,46341,
    -5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,2,-2,33554432,-33554432,65536,46341,-5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,0,0,33554432,-33554432,65536,46341];

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
    print(`${funName}(${[...args]}) \t produced ${result} \texpected ${expected}`);
  }
}

function GenerateBaseline(funName, ...args)
{
  let fun = eval(funName);
  var result;
  try {
     result = fun(...args);
  } catch (e) {
    result = e.message;
  }
return result;
}

const wasmModuleText = `(module
  (func (export "i32_div_1")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 1)) )
  (func (export "i32_div_n1") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const -1)) )

  (func (export "i32_div_3")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 3)) )
  (func (export "i32_div_5")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 5)) )
  (func (export "i32_div_7")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 7)) )
  (func (export "i32_div_n3") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const -3)) )
  (func (export "i32_div_n5") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const -5)) )
  (func (export "i32_div_n7") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const -7)) )
  (func (export "i32_div_8")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 8)) )
  (func (export "i32_div_9")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 9)) )

  (func (export "i32_div_13") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 13)) )
  (func (export "i32_div_33") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 33)) )
  (func (export "i32_div_60") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 60)) )
  
  (func (export "i32_div_100")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 100)) )
  (func (export "i32_div_120")  (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 120)) )
  
  (func (export "i32_div_1001") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 1001)) )
  (func (export "i32_div_9999") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 9999)) )
  (func (export "i32_div_99999") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 99999)) )

  (func (export "i32_div_max0") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 4294967295)) )
  (func (export "i32_div_max1") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 4294967295)) )
  (func (export "i32_div_max2") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const 2147483648)) )
  (func (export "i32_div_min0") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const -429496729)) )
  (func (export "i32_div_min1") (param $x i32) (result i32)  (i32.div_s (get_local $x) (i32.const -2147483647)) )

  (func (export "i32_rem_3")  (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 3)) )
  (func (export "i32_rem_5")  (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 5)) )
  (func (export "i32_rem_7")  (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 7)) )
  (func (export "i32_rem_n3") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const -3)) )
  (func (export "i32_rem_n5") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const -5)) )
  (func (export "i32_rem_n7") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const -7)) )

  (func (export "i32_rem_14") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 14)) )
  (func (export "i32_rem_37") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 37)) )
  (func (export "i32_rem_120") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 120)) )
  (func (export "i32_rem_Max0") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 4294967295)) )
  (func (export "i32_rem_Max1") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const 2147483648)) )
  (func (export "i32_rem_Min0") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const -429496729)) )
  (func (export "i32_rem_Min1") (param $x i32) (result i32)  (i32.rem_s (get_local $x) (i32.const -2147483647)) )
)`;

const mod = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(wasmModuleText));
const {exports} = new WebAssembly.Instance(mod);

var fns = [exports.i32_div_1, 
           exports.i32_div_n1,
           exports.i32_div_3,
           exports.i32_div_5,
           exports.i32_div_7,
           exports.i32_div_n3,
           exports.i32_div_n5,
           exports.i32_div_n7,
           exports.i32_div_8,
           exports.i32_div_9,
           exports.i32_div_13,
           exports.i32_div_33,
           exports.i32_div_60,
           exports.i32_div_100,
           exports.i32_div_120,
           exports.i32_div_1001,
           exports.i32_div_9999,
           exports.i32_div_99999,
           exports.i32_div_max0,
           exports.i32_div_max1,
           exports.i32_div_max2,
           exports.i32_div_min0,
           exports.i32_div_min1,
           exports.i32_rem_3,
           exports.i32_rem_5,
           exports.i32_rem_7,
           exports.i32_rem_n3,
           exports.i32_rem_n5,
           exports.i32_rem_n7,
           exports.i32_rem_14,
           exports.i32_rem_37,
           exports.i32_rem_120,
           exports.i32_rem_Max0,
           exports.i32_rem_Max1,
           exports.i32_rem_Min0,
           exports.i32_rem_Min1];


/*Math test for int div strength reduction*/

function testSignedDivStrengthReduction() {
  var i = 0;
  fns.forEach(function (fn) {
      test_values.forEach(function (value) {
        if(debug && debugTestNum == -1)
        {
          print("Test# "+ i + " " + fn + " ("+ value + ") \t Expected:" + results[i] + "\t Found:" + GenerateBaseline(fn, value));
        }
        else if(debug && debugTestNum == i)
        {
          print("Test# "+ i + " " + fn + " ("+ value + ") \t Expected:" + results[i] + "\t Found:" + GenerateBaseline(fn, value));
        }
        else
        {
          check(results[i], fn, value);
        }
        ++i;
      }, this);
  }, this);
}

if( CreateBaseline )
{
  var tmp = [];
  var i = 0;
    fns.forEach(function (fn) {
      test_values.forEach(function (value) {
        if(debug)
        {
          print("Test #"+i++ + " " + fn + "\t(" + value + ")\t Result: "+ GenerateBaseline(fn, value));
        }
        tmp.push(GenerateBaseline(fn, value));
      }, this);
  }, this);
  print("[" + tmp + "]");
}
else
{

  // var a = new Date().getTime();
  // for (var i = 0; i < 1; ++i)
      testSignedDivStrengthReduction();
  // var b = new Date().getTime() - a;
  // print("ElapsedTime = " + b);

}

if(passed) {
  print("Passed");
}
