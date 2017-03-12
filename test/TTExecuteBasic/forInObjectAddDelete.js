//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
//NOTE: this may break if enumeration order policy is changed in Chakra but that doesn't mean we have a bug in TTD
//

var x = { a: 1, b: 2};
var obj = { a: 1, b: 2, c: 15};

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog("1st enumeration", true);
    for(var i in x)
    {
        if(x[i] == 1)
        {
            delete x.a;
            delete x.b;
            x.c = 3;
            x.d = 4;
        }
        else
            telemetryLog(`${x[i]}`, true);
    }

    telemetryLog("2nd enumeration", true);
    for (var i in obj) {
        if (obj[i] == 1) {
            delete obj.a;
            delete obj.b;
            obj.c = 3;
            obj.d = 4;
        }
        else
            telemetryLog(`${obj[i]}`, true);
    }

    emitTTDLog(ttdLogURI);
}

