//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
//NOTE: this may break if enumeration order policy is changed in Chakra but that doesn't mean we have a bug in TTD
//

function makeobj(n)
{
    var obj = { };

    for(var i = 0; i < n; ++i)
    {
        obj[i] = 1;
    }

    return obj;
}

function testdelete(n)
{
    for(var propToDelete = 0; propToDelete <= n; ++propToDelete)
    {
        for(var iterToDelete = 0; iterToDelete <= n; ++iterToDelete)
        {
            for(var iterToAdd = 0; iterToAdd <= n; ++iterToAdd)
            {

                telemetryLog(`testing with ${n} properties`, true);
                telemetryLog(`deleting property number ${propToDelete} on iteration ${iterToDelete}`, true);
                telemetryLog(`adding a property on iteration ${iterToAdd}`, true);

                var o = makeobj(n);
                var iter = 0;

                for(var i in o)
                {
                    if(iter == iterToDelete)
                        delete o[propToDelete];

                    if(iter == iterToAdd)
                        o["xxx"] = 1;

                    telemetryLog(`${i}`, true);

                    ++iter;
                }
            }
        }
    }
}

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    testdelete(2);

    emitTTDLog(ttdLogURI);
}
