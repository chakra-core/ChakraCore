//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0(){
  function a(b = function c() {}) { return () => { return 6; } };
  [0].reduce(function d() {}, 0);
  a()();
}
test0();

console.log("pass");
