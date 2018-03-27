//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var nomissing = [];
nomissing[0] = {};

var missing = [];
missing[1] = {}

function foo(a, i)
{
    return i in a;
}

foo(nomissing, 0);
foo(nomissing, 0);
var res = foo(missing, 0);
WScript.Echo(res ? "FAIL" : "PASS");
