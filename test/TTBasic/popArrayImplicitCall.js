//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var obj = {};

Object.prototype.push = Array.prototype.push;
Object.prototype.pop = Array.prototype.pop;
var x;
Object.defineProperty(obj, "length", {get: function() {x = true; return 5;}});

x = false;


WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    try
    {
        var len = obj.pop();
    }
    catch (e)
    {
        telemetryLog('caught exception calling pop', true);
    }

    telemetryLog(`${x}`, true);
    telemetryLog(`${len}`, true);

    emitTTDLog(ttdLogURI);
}
