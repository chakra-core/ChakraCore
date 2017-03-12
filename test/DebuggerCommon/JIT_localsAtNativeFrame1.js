//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validating simple locals inspection at lower stack frame.

function F1() {
    var a = new Array;                       
    var sub = -10;                            /**bp:stack();locals();setFrame(1);locals();setFrame(2);locals();setFrame(3);locals();**/
    return a.length;
}

function F2(a1)
{
    var j = 10;
    var j1 = {x:"x", y : 31};
    var m = 20 + a1.toString();                           
    F1();
}

function Run(arg1)
{
    F2(arg1);
}

Run(12);
Run([3,5,8])
Run({a:"aa"});
Run([3,5,8]);

WScript.Echo("Pass");

