//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(x) {
    this.x = x;
}

var f = new foo(10);

foo.prototype = { y : 10 };

var f1 = new foo(20);

function bar(x, y) {
    this.x1 = x;
    this.x2 = x;
    this.x3 = x;
    this.x4 = x;
    this.x5 = x;
    this.x6 = x;
    this.x7 = x;
    this.x8 = x;
    this.x9 = x;
    
    this.y1 = y;
    this.y2 = y;
    this.y3 = y;
    this.y4 = y;
    this.y5 = y;
    this.y6 = y;
    this.y7 = y;
    this.y8 = y;
    this.y9 = y;
}

var b1 = new bar(10, 20);
var b2 = new bar(30, 40);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`f.y ${f.y}`, true);
    telemetryLog(`f1.y ${f1.y}`, true);
    telemetryLog(`b2.y8 ${b2.y8}`, true);

    emitTTDLog(ttdLogURI);
}