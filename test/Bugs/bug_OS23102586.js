//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// force:deferparse

function test0() {
  var k;
  
  function foo(a = function() { +k; }) {
      a();
      function bar() { a }
  };
  
  eval('')
  foo();
}
test0();
console.log('pass')
