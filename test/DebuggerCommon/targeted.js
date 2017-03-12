//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function g()
{
	var x = 2;
	x++;
	throw "this is an exception";
	x++;
}

function f()
{
	var c = 5;
	var obj = { x: 2, y: 3, o2: {a: 44}};
	try {
		g();   /**bp(test):locals();resume('step_into');locals();resume('step_into');locals();**/
	} catch(ex) {
		WScript.Echo("caught: " + ex);
	}
	d = 7;
	c = 7; /**bp(bp1):stack();**/
	c = 8; /**bp:locals(2)**/
	c = 9; 
}
for(var i = 0; i < 5; ++i)
	f();
	
function g1() { }
function f1() {
	var x = 2;
	g1.apply(this, arguments);  /**bp:locals(1);**/
	x++;					   /**bp:locals(1);**/
}

f1(10);