//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
//NOTE: this may break if enumeration order policy is changed in Chakra but that doesn't mean we have a bug in TTD
//

var x = { a: 2, b: 3 };
var o = { a: 2, b: 3 }

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog("Delete current element", true);

    for(var i in x)
    {
        if(x[i] == 2)
            delete x[i];
        else
            telemetryLog(`${x[i]}`, true);
    }

    telemetryLog("Delete former element", true);

    var n = 0;
    for(var i in o)
    {
        if(n++ == 1)
            delete o.a;
        telemetryLog(`${i}`, true);
    }

    emitTTDLog(ttdLogURI);
}