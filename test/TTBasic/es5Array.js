//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = [];
x[2] = 5;
x.foo = 3;

Object.defineProperty(x, '1', {
    get: function () { return this.foo + 1; },
    set: function (x) { this.foo = x / 2; }
});

Object.defineProperty(x, 11, {
    get: function () { return this.foo; }
});

var simpleArrayEmptyLength = [];
Object.defineProperty(simpleArrayEmptyLength, "length", {});

var aFixedInfo = [0, 1, 2, 3, 4, 5];
Object.defineProperty(aFixedInfo, "length", { writable: false });
Object.defineProperty(aFixedInfo, "2", { writable: false });

var aFrozen = [0, 1, 2, 3, 4, 5];
Object.freeze(aFrozen);

var oIncFreeze = [0, 1, 2, 3, 4, 5];
for (var i = 0; i < oIncFreeze.length; i++) 
{
    Object.defineProperty(oIncFreeze, i, { writable: false, configurable: false });
}
Object.preventExtensions(oIncFreeze);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`Array.isArray(x): ${Array.isArray(x)}`, true); //true

    telemetryLog(`x.foo: ${x.foo}`, true); //3

    telemetryLog(`x[1]: ${x[1]}`, true); //4
    telemetryLog(`x[11]: ${x[11]}`, true); //3

    ////
    x[1] = 12;
    ////

    telemetryLog(`x[1]: ${x[1]}`, true); //7
    telemetryLog(`x[11]: ${x[11]}`, true); //6

    ////
    telemetryLog(`Object.getOwnPropertyDescriptor(simpleArrayEmptyLength.length): ${JSON.stringify(Object.getOwnPropertyDescriptor(simpleArrayEmptyLength, "length"))}`, true); //asdf

    aFixedInfo[9] = 9; // This would throw in strict mode
    telemetryLog(`aFixedInfo: ${JSON.stringify(aFixedInfo)}`, true); //0, 1, 2, 3, 4, 5
    aFixedInfo[1] = -1;
    aFixedInfo[2] = -2;
    telemetryLog(`aFixedInfo: ${JSON.stringify(aFixedInfo)}`, true); //0, 1, 2, 3, 4, 5

    telemetryLog(`Object.getOwnPropertyDescriptor(aFrozen.length): ${JSON.stringify(Object.getOwnPropertyDescriptor(aFrozen, "length"))}`, true); //asdf

    telemetryLog(`isFrozen: ${Object.isFrozen(oIncFreeze)}`, true); // false, because length writable
    Object.defineProperty(oIncFreeze, "length", { writable: false });
    telemetryLog(`isFrozen: ${Object.isFrozen(oIncFreeze)}`, true);

    emitTTDLog(ttdLogURI);
}