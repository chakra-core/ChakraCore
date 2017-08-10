//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
var func12 = function (arg1 = ( { get : function f1 () { }}
, { get : function f213 () { }}
, { get : function f214 () { }}
,(function f2() {}))) 
{
};
func12();
}
test0();

WScript.Echo('pass');
