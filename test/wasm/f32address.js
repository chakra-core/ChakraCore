//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
this.WScript.LoadScriptFile("..\\UnitTestFramework\\SimdJsHelpers.js");
var memory = null;
var ffi = {spectest: {print: print}};
var mod = new WebAssembly.Module(readbuffer('f32address.wasm'));
var module = new WebAssembly.Instance(mod, ffi);


equal(2145386496, module.exports['i32.load']());
equal(NaN       , module.exports['f32.load']());
module.exports['reset']();
equal(0         , module.exports['i32.load']());
equal(0         , module.exports['f32.load']());
module.exports['f32.store']();
equal(1325350912, module.exports['i32.load']());
equal(2141192192, module.exports['f32.load']());
module.exports['reset']();
equal(0         , module.exports['i32.load']());
equal(0         , module.exports['f32.load']());
module.exports['i32.store']();
equal(2141192192, module.exports['i32.load']());
equal(NaN       , module.exports['f32.load']());

equal(8364032    , module.exports['i32.load1']());
equal(1.1720505197163628e-38, module.exports['f32.load1']());
module.exports['reset1']();
equal(0         , module.exports['i32.load1']());
equal(0         , module.exports['f32.load1']());
module.exports['f32.store1']();
equal(1325350912, module.exports['i32.load1']());
equal(2141192192, module.exports['f32.load1']());
module.exports['reset1']();
equal(0         , module.exports['i32.load1']());
equal(0         , module.exports['f32.load1']());
module.exports['i32.store1']();
equal(2141192192, module.exports['i32.load1']());
equal(NaN, module.exports['f32.load1']());


module.exports['i32.store']();

equal(8355744   , module.exports['i32.load2']());
equal(1.1708891235491304e-38, module.exports['f32.load2']());
module.exports['reset2']();
equal(0         , module.exports['i32.load2']());
equal(0         , module.exports['f32.load2']());
module.exports['f32.store2']();
equal(1325350912, module.exports['i32.load2']());
equal(2141192192, module.exports['f32.load2']());
module.exports['reset2']();
equal(0         , module.exports['i32.load2']());
equal(0         , module.exports['f32.load2']());
module.exports['i32.store2']();
equal(2141192192, module.exports['i32.load2']());
equal(NaN       , module.exports['f32.load2']());

print("PASSED");

//To Generate Baselines
// print(module.exports['i32.load']());
// print(module.exports['f32.load']());
// module.exports['reset']();
// print(module.exports['i32.load']());
// print(module.exports['f32.load']());
// module.exports['f32.store']();
// print(module.exports['i32.load']());
// print(module.exports['f32.load']());
// module.exports['reset']();
// print(module.exports['i32.load']());
// print(module.exports['f32.load']());
// module.exports['i32.store']();
// print(module.exports['i32.load']());
// print(module.exports['f32.load']());
//
//
// print();
// print(module.exports['i32.load1']());
// print(module.exports['f32.load1']());
// module.exports['reset1']();
// print(module.exports['i32.load1']());
// print(module.exports['f32.load1']());
// module.exports['f32.store1']();
// print(module.exports['i32.load1']());
// print(module.exports['f32.load1']());
// module.exports['reset1']();
// print(module.exports['i32.load1']());
// print(module.exports['f32.load1']());
// module.exports['i32.store1']();
// print(module.exports['i32.load1']());
// print(module.exports['f32.load1']());
//
// module.exports['i32.store']();
//
// print();
// print(module.exports['i32.load2']());
// print(module.exports['f32.load2']());
// module.exports['reset2']();
// print(module.exports['i32.load2']());
// print(module.exports['f32.load2']());
// module.exports['f32.store2']();
// print(module.exports['i32.load2']());
// print(module.exports['f32.load2']());
// module.exports['reset2']();
// print(module.exports['i32.load2']());
// print(module.exports['f32.load2']());
// module.exports['i32.store2']();
// print(module.exports['i32.load2']());
// print(module.exports['f32.load2']());
