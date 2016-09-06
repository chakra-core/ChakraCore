//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
	Let/Const assignment from debugger
	Redeclaration in script
*/

foo();
foo();


function foo(){
    let b = 200; /**loc(bp1):evaluate('let a = 1')**/
    let a = 100; /**loc(bp2):evaluate('a')**/
    WScript.Echo('PASSED'); /**loc(bp3):evaluate('a');evaluate('let a = 1')**/
}


function Run(){
    foo();
    foo();
    foo();/**bp:enableBp('bp1');enableBp('bp2');enableBp('bp3')**/
	foo; /**bp:disableBp('bp1');disableBp('bp2');disableBp('bp3')**/
}

WScript.Attach(Run)

