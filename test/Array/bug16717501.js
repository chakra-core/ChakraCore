//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  for (var vnlgev in [1 .__parent__ = '']) {
  }
  // Value too big to be a tagged int on 32 bit platforms
  return 1518500249 in [];
}
// Trigger jit
for (let i = 0; i < 1000; ++i) {
  test0();
}

console.log("pass");
