//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = /World/;
var y = new RegExp("l", "g");

y.exec("Hello World");

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`y.lastIndex: ${y.lastIndex}`, true); //3

    ////
    var m = "Hello World".match(x);
    y.exec("Hello World");
    ////

    telemetryLog(`m.index: ${m.index}`, true); //6
    telemetryLog(`post update -- y.lastIndex: ${y.lastIndex}`, true); //4
}