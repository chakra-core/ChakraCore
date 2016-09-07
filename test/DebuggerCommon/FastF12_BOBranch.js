//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// WinBlue 218307: loop condition branch becomes dead as condition is always true.
// Make sure that the branch and b/o for debugger on it aren't splitted into separate blocks,
// and that StatementBoundary pragmas are processed correctly for RecordThrowMap.

function test0(){
  var d = 1;
  function bar3 (){
    e = 1; /**bp:stack();**/
  }
  for(var i = 0; i < 1 && bar3.call(); i++) {
    Math.sin();
  }
};
test0(); 
test0(); 
WScript.Echo("PASS");
