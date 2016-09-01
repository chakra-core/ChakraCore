//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
	Let const assignments from debugger
	Redeclaration in debugger
*/
let a = 100; 
let b = 200;/**bp:evaluate('let a = 1')**/
WScript.Echo(a==100);
WScript.Echo(b==200);
WScript.Echo('PASSED'); /**bp:locals(1)**/