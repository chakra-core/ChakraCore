//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// #1
function test0() {
  function leaf() {
    return 100;
  }
  var obj1 = {};
  var func0 = function () {
    return leaf();
  };
  var func2 = function () {
    if (!(func0.call() % -2147483646)) {
      arrObj0;
    }
  };
  obj1.method1 = func2;
  obj1.method1();
}
test0();
test0();
test0();
test0();


// #2
function foo(a, b)
{
  a %= 3;
  b = b >>a;
  return b;
}

v1 = foo(4,2);
foo(4,2);
foo(4,2);
foo(4,2);
foo(4,2);

v2 = foo(4,2);

if(v1 != v2)
  print("ERROR: Functional error in jit code");
else
  print('PASS'); //Test failure causes assertion /error
