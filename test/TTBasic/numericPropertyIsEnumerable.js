//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var myobj = { a: "apple", 101: 1 }

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`myobj.propertyIsEnumerable('a'): ${myobj.propertyIsEnumerable('a')}`, true);
    telemetryLog(`myobj.propertyIsEnumerable(101): ${myobj.propertyIsEnumerable(101)}`, true);
    telemetryLog(`myobj.propertyIsEnumerable("101"): ${myobj.propertyIsEnumerable("101")}`, true);
    telemetryLog(`myobj.propertyIsEnumerable("10"): ${myobj.propertyIsEnumerable("10")}`, true);

    for (o in myobj)
    {
        telemetryLog(`${o} is enumerable ${myobj.propertyIsEnumerable(o)}`, true);
    }

    emitTTDLog(ttdLogURI);
}