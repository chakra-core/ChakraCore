//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var b = 0;

function test0() {
  var a = 100;

  for (var i = 1; i < 6; ++i) {
    a;
    test0a();
    for (var j = 0; j < 1; ++j) {
      a = 0xffffffffff;
      for (var k = 0; k < 1; ++k) {
        i = a;
        +i;
      }
    }
    if (i == 0xffffffffff) {
      print('PASSED');
      throw new Error('exit');
    }
  }

  function test0a() {
    a;
    b = b + 1;
    if (b > 100) {
      return;
    }
    return test0();
  }
}

try { test0(); }
catch {}
