//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function trap (f, v)
{
    try { return f(v); }
    catch (e) { return e.message};
}

const blob = WScript.LoadBinaryFile('trunc.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;
print ("=i32=");
print(trap(a.i32_trunc_u_f64,Number.NaN));
print(trap(a.i32_trunc_s_f64,Number.NaN));
print(trap(a.i32_trunc_u_f32,Number.NaN));
print(trap(a.i32_trunc_s_f32,Number.NaN));
print ("====");
print(a.i32_trunc_u_f64(0.0));
print(a.i32_trunc_u_f64(4294967294));
print(a.i32_trunc_u_f64(4294967295.0));
print(trap(a.i32_trunc_u_f64,-40.0));
print(trap(a.i32_trunc_u_f64,4294967296.0));
print ("====");
print(a.i32_trunc_s_f64(-1.0));
print(a.i32_trunc_s_f64(0.0));
print(a.i32_trunc_s_f64(-2147483647.0));
print(a.i32_trunc_s_f64(-2147483648.0));
print(trap(a.i32_trunc_s_f64,-2147483649.0));
print ("====");
print(a.i32_trunc_s_f64(0.0));
print(a.i32_trunc_s_f64(2147483647.0));
print(trap(a.i32_trunc_s_f64,2147483648.0));
print ("====");
print(a.i32_trunc_u_f32(0.0));
print(a.i32_trunc_u_f32(4294967040.0));
print(trap(a.i32_trunc_u_f32,4294967296.0));
print ("====");
print(a.i32_trunc_s_f32(0.0));
print(a.i32_trunc_s_f32(2147483520.0));
print(trap(a.i32_trunc_s_f32,2147483647.0));
print ("====");
print(a.i32_trunc_s_f32(0.0));
print(a.i32_trunc_s_f32(-2147483520.0));
print(trap(a.i32_trunc_s_f32,-2147483800.0));
print ("=i64=");
print(trap(a.i64_trunc_u_f64,Number.NaN));
print(trap(a.i64_trunc_s_f64,Number.NaN));
print(trap(a.i64_trunc_u_f32,Number.NaN));
print(trap(a.i64_trunc_s_f32,Number.NaN));
print ("====");
print(a.i64_trunc_u_f64(14757395258967641292));
print(trap(a.i64_trunc_u_f64,18446744073709550592));
print(a.i64_trunc_u_f64(0));
print(a.i64_trunc_u_f64(-0.5));
print(trap(a.i64_trunc_u_f64,-1));
print ("====");
print(a.i64_trunc_s_f64(9223372036854774784));
print(trap(a.i64_trunc_s_f64,9223372036854775296));
print ("====");
print(a.i64_trunc_s_f64(-3689348814741910324));
print(trap(a.i64_trunc_s_f64,Math.pow(-2,64)));
print ("====");
print(trap(a.i64_trunc_s_f64,18446742974197923840));
print(trap(a.i64_trunc_s_f64,18446743523953737728));
print(a.i64_trunc_u_f32(0));
print(a.i64_trunc_u_f32(-0.5));
print(trap(a.i64_trunc_u_f32,-1));
print ("====");
print(a.i64_trunc_s_f32(9223371487098961920));
print(trap(a.i64_trunc_s_f32,9223371761976868864));
print ("====");
print(a.i64_trunc_s_f32(-9223372036854775808));
print(trap(a.i64_trunc_s_f32,Math.pow(-2,64)));
