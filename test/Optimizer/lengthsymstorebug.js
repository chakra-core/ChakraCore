//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var c = [];
function foo(b) {
  var g;
  if (!b) {
    for (g = c.length;--g >= 0;) {
      if (c) {
        break;
      }
    }
  } else {
    for (g = c.length;--g >= 0;) {
    }
  }

  if (0 > g) {
  
    return;
  }

  for (var i = c.length; --i > g;) {
  }
}

foo();
foo();
print("Passed");
