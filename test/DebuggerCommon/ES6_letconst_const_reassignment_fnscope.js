//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


/*
    Const Reassignment inside function
*/

function foo(){
    const a = 1;
    a; /**loc(bp1):evaluate('a++');evaluate('a')**/
    WScript.Echo('PASSED');
}

function Run(){
    foo();
    foo();
    foo();/**bp:enableBp('bp1')**/
	foo; /**bp:disableBp('bp1')**/
}

WScript.Attach(Run);
