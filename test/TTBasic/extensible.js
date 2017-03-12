//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = {x:20, y:30};
Object.preventExtensions(a);

var b = {x:20, y:30};
Object.preventExtensions(b);

var c = {x:20, y:30};
Object.preventExtensions(c);

var d = {x:20, y:30};
Object.preventExtensions(d);

var e = {get x() {return 0;}, y:30};
Object.preventExtensions(e);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    a.z = 50;
    telemetryLog(`${Object.getOwnPropertyNames(a)}`, true);
    telemetryLog(`${Object.isExtensible(a)}`, true);

    delete b.x;
    telemetryLog(`${b.x}`, true);
    telemetryLog(`${Object.isExtensible(b)}`, true);

    c.x = 40;
    c.y = 60;
    telemetryLog(`${Object.getOwnPropertyNames(c)}`, true);
    telemetryLog(`${Object.isExtensible(c)}`, true);
    telemetryLog(`${c.x}`, true);

    delete d.x;
    Object.defineProperty(d, "y", {configurable: false});
    telemetryLog(`${Object.isSealed(d)}`, true);
    Object.defineProperty(d, "y", {writable: false});
    telemetryLog(`${Object.isFrozen(d)}`, true);

    delete e.x;
    Object.defineProperty(e, "y", {configurable: false});
    telemetryLog(`${Object.isSealed(e)}`, true);
    Object.defineProperty(e, "y", {writable: false});
    telemetryLog(`${Object.isFrozen(e)}`, true);

    emitTTDLog(ttdLogURI);
}