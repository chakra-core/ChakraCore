//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = new Date();
var y = x;

var z = new Date(2012, 1);

y.foo = 3;

var w = Date.now();

var dinfty = new Date(Infinity);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`x === y: ${x === y}`, true); //true
    telemetryLog(`w !== z: ${w !== z.valueOf()}`, true); //true

    telemetryLog(`y.foo: ${y.foo}`, true); //3
    telemetryLog(`x.foo: ${x.foo}`, true); //3

    telemetryLog(`w - z > 0: ${w - z.valueOf() > 0}`, true); //true
    telemetryLog(`x - y: ${x.valueOf() - y.valueOf()}`, true); //0

    try 
    {
        telemetryLog(dinfty.toISOString(), true);
    } 
    catch(e) 
    {
        telemetryLog(`Infinity Date toISOString : ${e.name}  : ${e.message}`, true);
    }
    telemetryLog(`Infinity Date toJSON : ${dinfty.toJSON()}`, true);

    emitTTDLog(ttdLogURI);
}