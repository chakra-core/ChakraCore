//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


function foo()
{
	WScript.Echo("In function foo");
	debugger;

	var a1 = new Array(10);
	a1[0] = {a:"10"};
	eval('var inEval = 10;\n inEval++;\n');
	debugger;
}

function bar()
{
	WScript.Echo("In function bar");
	debugger;
	var a2 = {b:10};
	debugger;
	
	return a2.b;
}
foo();
bar();

WScript.Attach(foo);
