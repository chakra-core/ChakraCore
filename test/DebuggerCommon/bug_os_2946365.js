//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
function foo() {
    var x = 1; /**bp:locals(2);**/
    WScript.Echo("pass");
}
WScript.Attach(foo);
