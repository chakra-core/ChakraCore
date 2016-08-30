//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function F1(a) {
    F2(a, " World");
}
F1("Hello");

function F2(a,b) {
    arguments.callee.caller.arguments[0] += b;
    WScript.Echo(a,b); /**bp:setFrame(1);locals(1)**/
}