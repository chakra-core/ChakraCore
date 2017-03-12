//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x1 = {p01: 0};
Object.preventExtensions(x1);
var y1 = {p01: 0};

////

var f2 = function f(o) { var tepm = o["p02_2"]; o["p02_2"] = 10; };
var x2 = {p02_1: 0};
f2(x2);
var y2 = {p02_1: 0};
Object.preventExtensions(y2);
f2(y2);

////

var x3 = {p03: 0};
Object.preventExtensions(x3);
delete x3["p03"];
var y3 = {p03: 0}; // new object with same layout as previous one.
y3["p03"] = 2;
Object.preventExtensions(y3); // Earlier bug: this was associated a with evolved type.
y3["p03"] = 3; // Can modify writable property but not on evolved type.

////

var x5 = {p05: 0};
Object.seal(x5);
Object.defineProperty(x5, "p05", {writable: false, value: 100}); // this should succeed
var y5 = {p05: 0};
Object.seal(y5); // Now y shares same s.d.t.h as x used to before defineProperty on x.
Object.defineProperty(y5, "p05", {writable: false, value: 200}); // this should succeed again (defineProperty on x should not corrupt shared t.h.)

var values6 = [0, 1, 2, 3];
var x6 = {p06: 0};
x6["p06"] = values6[0];
Object.freeze(x6);
x6["p06"] = values6[1];  // this should not work
var y6 = {p06: 0};
Object.seal(y6);
y6["p06"] = values6[2];   // this should work
var z6 = {p06: 0};
Object.preventExtensions(z6);
z6["p06"] = values6[3];   // this should work

var x9 = {p09: 0};
x9[0] = 0;
Object.preventExtensions(x9);
x9[1] = 1; // Should fail.
Object.defineProperty(x9, '0', {value: 2}); // Another way to add a property, should fail as well.

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`Object.isExtensible(x1): ${Object.isExtensible(x1)}`, true);
    telemetryLog(`Object.isExtensible(y1): ${Object.isExtensible(y1)}`, true);

    ////

    telemetryLog(`y2["p02_2"]: ${y2["p02_2"]}`, true);

    ////

    telemetryLog(`Object.isExtensible(x3): ${Object.isExtensible(x3)}`, true);
    telemetryLog(`y3["p03"]: ${y3["p03"]}`, true); //3

    ////

    telemetryLog(`y5["p05"]: ${y5["p05"]}`, true); //200

    ////

    telemetryLog(`[x6["p06"], y6["p06"], z6["p06"]]: ${[x6["p06"], y6["p06"], z6["p06"]]}`, true); //[0, 2, 3]

    ////

    telemetryLog(`x9[0]: ${x9[0]}`, true); //0
    telemetryLog(`x9[1]: ${x9[1]}`, true); //undef

    emitTTDLog(ttdLogURI);
}
