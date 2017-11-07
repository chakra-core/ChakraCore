//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var mod = new WebAssembly.Module(readbuffer('misc.wasm'));
var a = new WebAssembly.Instance(mod).exports;
print(a.f32copysign(-40.0,2.0)); // == 40.0
print(a.f32copysign(-40.0,-2.0)); // == -40.0
print(a.f32copysign(-1.0,2.0)); // == 1.0
print(a.f32copysign(-1.0,-2.0)); // == -1.0
print(a.f32copysign(255.0,-1.0)); // == -255.0
print(a.f32copysign(255.0,1.0)); // == 255.0
print(a.eqz(0)); // == 1
print(a.eqz(-1)); // == 0
print(a.eqz(1)); // == 0
print(a.trunc(0.5)); // == 0
print(a.trunc(-1.5)); // == -1
print(a.trunc(NaN)); // == NaN
print(a.trunc(-NaN)); // == NaN
print(a.trunc(Infinity)); // == Infinity
print(a.trunc(-Infinity)); // == -Infinity
print(a.f64trunc(0.5)); // == 0
print(a.f64trunc(-1.5)); // == -1
print(a.f64trunc(NaN)); // == NaN
print(a.f64trunc(-NaN)); // == NaN
print(a.f64trunc(Infinity)); // == Infinity
print(a.f64trunc(-Infinity)); // == -Infinity
print(a.ifeqz(0)); // == 1
print(a.ifeqz(-1)); // == 0
print(a.nearest(-0.1)); // == 0
print(a.nearest(-0.7)); // == -1
print(a.nearest(-1.5)); // == -2
print(a.nearest(NaN)); // == NaN
print(a.nearest(-NaN)); // == NaN
print(a.nearest(Infinity)); // == Infinity
print(a.nearest(-Infinity)); // == -Infinity
print(a.f64nearest(-0.1)); // == 0
print(a.f64nearest(-0.7)); // == -1
print(a.f64nearest(-1.5)); // == -2
print(a.f64nearest(NaN)); // == NaN
print(a.f64nearest(-NaN)); // == NaN
print(a.f64nearest(Infinity)); // == Infinity
print(a.f64nearest(-Infinity)); // == -Infinity
print(a.f64copysign(255.0,-1.0)); // == -255.0
