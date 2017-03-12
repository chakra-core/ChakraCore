//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Validating literals as in indices : fully qualified name support

var obj = {};
var x = 2;
obj[1] = function () {
x;
x;      /**bp:stack()**/
};
obj[x] = function () {
x;
x;      /**bp:stack()**/
};

obj[1+3] = function () {
x;
x;      /**bp:stack()**/
};
obj[3.4456] = function () {
x;
x;      /**bp:stack()**/
};

obj[1]();
obj[2]();
obj[4]();
obj["3.4456"]();
WScript.Echo("Pass");
