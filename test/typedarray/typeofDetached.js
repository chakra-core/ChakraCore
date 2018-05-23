//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const obj = {};
const f32 = new Float32Array();
function foo() {
  return typeof f32[obj.missingprop & 1];
}
ArrayBuffer.detach(f32.buffer);
for (let i = 0; i < 1000; ++i) {
  foo();
}
foo();
console.log("pass");
