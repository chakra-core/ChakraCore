//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = /World/;
var y = new RegExp("l", "g");
var z = new RegExp("l", "g");

y.exec("Hello World");
z.lastIndex = -1;

var re = /abc/i;
var re1 = new RegExp(re, "gm");

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`re.global == ${re.global}`, true); //false
    telemetryLog(`re.multiline == ${re.multiline}`, true); //false
    telemetryLog(`re.ignoreCase == ${re.ignoreCase}`, true); //true

    telemetryLog(`re1.global == ${re1.global}`, true); //true
    telemetryLog(`re1.multiline == ${re1.multiline}`, true); //true
    telemetryLog(`re1.ignoreCase == ${re1.ignoreCase}`, true); //false

    telemetryLog(`y.lastIndex: ${y.lastIndex}`, true); //3
    telemetryLog(`z.lastIndex: ${z.lastIndex}`, true); //3

    ////
    var m = "Hello World".match(x);
    y.exec("Hello World");
    ////

    telemetryLog(`m.index: ${m.index}`, true); //6
    telemetryLog(`post update -- y.lastIndex: ${y.lastIndex}`, true); //4

    emitTTDLog(ttdLogURI);
}