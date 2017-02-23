//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = {x:20, y:30};
Object.seal(a);

var b = {x:20, y:30};
Object.seal(b);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    a.x = 40;
    a.z = 50;
    telemetryLog(`${Object.getOwnPropertyNames(a)}`, true);
    telemetryLog(`${Object.isSealed(a)}`, true);
    telemetryLog(`${a.x}`, true);

    try 
    {
        Object.defineProperty(a, 'x', { get: function() { return 'ohai'; } }); 
    }
    catch(e)
    {
        telemetryLog(`${e}`, true);
    }

    b.x = 40;
    delete b.x;
    b.z = 50;
    telemetryLog(`${Object.getOwnPropertyNames(b)}`, true);
    telemetryLog(`${Object.isSealed(b)}`, true);
    telemetryLog(`${b.x}`, true);

    emitTTDLog(ttdLogURI);
}