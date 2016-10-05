//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f()
{
	var x = {a: 2, b: 3};
	eval("x.a++;/**bp:locals(1);stack()**/ x.b++;");

	eval("x.c = \"test\\\"string\";/**bp:locals(1);stack()**/ x.b++;");

	var test = "this is \" a string"; /**bp:locals(1);stack()**/
	test = test + "another string";
}
f();
f();
WScript.Echo("pass");
