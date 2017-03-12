//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


Array.prototype[1] = 100;
function f(param)
{
    var a = new Array(1, param, 3);
    return a;
}

var a1 = f(undefined)[1];
var a2 = f(undefined)[1];

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`${a1}`, true);
    telemetryLog(`${a2}`, true);  // undefined in array parameter should still be set (legacy behavior is missing value)

    emitTTDLog(ttdLogURI);
}
