//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test(a, b, c) {
	arguments;/**bp:evaluate('arguments', 1)**/
    arguments;/**bp:evaluate('arguments.callee', 1)**/
	arguments;/**bp:evaluate('arguments.callee.caller', 1)**/
}
function bar(){	
	test(1, 2, 3);
}
var arrow = () => {};
bar(); 
WScript.Echo('Pass');/**bp:evaluate('arrow',1)**/


