//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = {x:20, y:30};
Object.freeze(a);

var b = {x:20, y:30};
Object.freeze(b);

var c = {x:20, y:30};
Object.freeze(c);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    a.z = 50;
    try 
    {
        Object.defineProperty(a, 'ohai', { value: 17 });
    }
    catch(e)
    {
        telemetryLog(`${e}`, true);
    }

    telemetryLog(`${Object.getOwnPropertyNames(a)}`, true);
    telemetryLog(`${Object.isFrozen(a)}`, true);

    delete b.x;
    telemetryLog(`${Object.getOwnPropertyNames(b)}`, true);
    telemetryLog(`${Object.isFrozen(b)}`, true);
    telemetryLog(`${b.x}`, true);

    a.c = 40;
    a.c = 60;
    telemetryLog(`${Object.getOwnPropertyNames(c)}`, true);
    telemetryLog(`${Object.isFrozen(c)}`, true);
    telemetryLog(`${c.x}`, true);

    emitTTDLog(ttdLogURI);
}