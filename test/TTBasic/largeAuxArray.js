//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var o = { };

function AddAccessorProperty()
{
    // add accessor property (converts to DictionaryTypeHandler)
    Object.defineProperty(o, "a", { get: function () { return 10; } , configurable: true} );
}

function AddPropertiesToObjectArray()
{
    // add enough properties to convert to BigDictionaryTypeHandler
    for (var i = 0; i < 25000; i++) {
        o["p" + i] = 0;
    }
}

AddAccessorProperty();
AddPropertiesToObjectArray();
AddAccessorProperty();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`o.a === 10: ${o.a === 10}`, true);

    emitTTDLog(ttdLogURI);
}
