//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function trap (f, v)
{
    try { return f(v); }
    catch (e) { return "Overflow Exception"};
}

const blob = WScript.LoadBinaryFile('trunc.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;

print(trap(a.i32_trunc_u_f64,Number.NaN));
print(trap(a.i32_trunc_s_f64,Number.NaN));
print(trap(a.i32_trunc_u_f32,Number.NaN));
print(trap(a.i32_trunc_s_f32,Number.NaN));

print(trap(a.i32_trunc_u_f64,0.0));
print(trap(a.i32_trunc_u_f64,4294967294));
print(trap(a.i32_trunc_u_f64,4294967295.0));
print(trap(a.i32_trunc_u_f64,-40.0));
print(trap(a.i32_trunc_u_f64,4294967296.0));

print(trap(a.i32_trunc_s_f64,-1.0));
print(trap(a.i32_trunc_s_f64,0.0));
print(trap(a.i32_trunc_s_f64,-2147483647.0));
print(trap(a.i32_trunc_s_f64,-2147483648.0));
print(trap(a.i32_trunc_s_f64,-2147483649.0));

print(trap(a.i32_trunc_s_f64,0.0));
print(trap(a.i32_trunc_s_f64,2147483647.0));
print(trap(a.i32_trunc_s_f64,2147483648.0));

print(trap(a.i32_trunc_u_f32,0.0));
print(trap(a.i32_trunc_u_f32,4294967040.0));
print(trap(a.i32_trunc_u_f32,4294967296.0));

print(trap(a.i32_trunc_s_f32,0.0));
print(trap(a.i32_trunc_s_f32,2147483520.0));
print(trap(a.i32_trunc_s_f32,2147483647.0));

print(trap(a.i32_trunc_s_f32,0.0));
print(trap(a.i32_trunc_s_f32,-2147483520.0));
print(trap(a.i32_trunc_s_f32,-2147483800.0));
