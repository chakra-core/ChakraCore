//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var y = Symbol.for('bob');

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    var x = Symbol.for('ted');
    var z = Symbol.for('bob');

    telemetryLog(`x === y: ${x === y}`, true); //false
    telemetryLog(`z === y: ${z === y}`, true); //true

    var zo = Symbol('bob');

    telemetryLog(`zo === y: ${zo === y}`, true); //false

    emitTTDLog(ttdLogURI);
}
