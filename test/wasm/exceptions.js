//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function trap (f, a, b)
{
    try { return f(a, b); }
    catch (e) { return e.message; };
}

const blob = WScript.LoadBinaryFile('exceptions.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;

print(trap(a.i32_div_s_by_two, 5, 2));
print(trap(a.i32_div_s_over, 5, 2));
print(trap(a.i32_div_s_over, 5, 0));
print(trap(a.i32_div_s, 5, 0));
print(trap(a.i32_div_s, -2147483648, -1));
print(trap(a.i32_div_u, 5, 0));
print(trap(a.i32_rem_u, 5, 0));
print(trap(a.i32_rem_s, 5, 0));

print(trap(a.i32_div_s, 5, -2));
print(trap(a.i32_div_u, 5, 2));

print(trap(a.i32_rem_u, 5, 1));
print(trap(a.i32_rem_s, 5, -1));
print(trap(a.i32_rem_s, 5, 2));
