//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validation of the bug 543550

var m = Map.prototype;
m;	/**bp:evaluate('m',2)**/
WScript.Echo("Pass");

function test1()
{
	"use strict"
	function bar() {
	}
	bar; /**bp:evaluate('bar', 2)**/
}
test1();