//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f(a)
{
	var x = a;      /**bp(oneshot):stack();disableBp('oneshot')**/      // fires and disables itself
	var y = a + a;  /**bp(neverhit):log('ERROR: should never fire')**/  // should never fire
	x += y;         /**bp(foo):stack();locals()**/   
	x--;            /**bp(bar):disableBp('foo');**/  // disables breakpoint 'foo'
}

f(10); /**bp:deleteBp('neverhit')**/ 
f(20);
f(30);
f(40); /**bp:enableBp('foo');enableBp('bar')**/   // enables breakpoint 'foo' and 'bar'
f(50);
f(60);
f(70);
WScript.Echo("pass");
