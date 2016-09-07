//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Make sure that when we step out into jitted frame under debugger for "new" operator, 
// we bail out and continue debugging in interpreter mode.
// WinBlue 325839 is about the case when we missed putting debugger bailout for NewScObjectNoArg,
// as due to optimization this bytecode doesn't result into a (script) call.

function foo()
{
  WScript.Echo("foo"); /**bp:resume('step_out');stack();**/
}

function test_objectNoArg()
{
  new foo();
  var y = 1;	// We should bail out to here.
}

var oldArray = Array;
function MyArray()
{
  WScript.Echo("MyArray"); /**bp:resume('step_out');stack();**/
  return oldArray.apply(this, arguments);
}

function test_arrayNoArg()
{
  Array = MyArray;
  new Array();
  Array = oldArray;
  var y = 1;	// We should bail out to here.
}

test_objectNoArg();
test_arrayNoArg();
