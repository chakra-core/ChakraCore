//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
	simple case of __proto__
*/
var a = {
    x : 2
};
var b = {
    y : 2
};
b.__proto__ = a;
b; /**bp:locals(2)**/
WScript.Echo('PASSED');