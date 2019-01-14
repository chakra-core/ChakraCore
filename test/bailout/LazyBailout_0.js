//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = [];
a.reduce(function () {}, 0);
a.reduce(function () {}, 0);

// Same error but does not use built ins:
var h = function(a){
  return a;
}

var g = function (a) {
  if (h(a)) {
      return { prop0: a, prop1: a.length };
  } 
}

var f = function (a) {
  let {prop0, prop1} = g(a);
}

var b = [];
f(b);
f(b);

print("pass")