//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function top1() {
    var xx = new Object();
    Object.defineProperty(xx, "yy", { set: function(val) {telemetryLog("in nested setter1", true); this.val = 10;} });
    var z = function() {
       xx.yy = 20;
       telemetryLog(`val is ${xx.yy}`, true);
    }
    return z;
}
var foo1 = top1();

function top2() {
    var xx = new Object();
    Object.defineProperty(xx, "yy", { get: function() { return this; },
    set: function(val) {telemetryLog("in nested setter2", true); this.val = 11;} });
    var z = function() {
       xx.yy = 20;
       telemetryLog(`val is ${xx.yy}`, true);
       telemetryLog(`val is ${xx.yy.val}`, true);
    }
    return z;
}
var foo2 = top2();

function top3() {
    Object.defineProperty(this, "yy", { get: function() { return this; },
    set: function(val) {telemetryLog("in nested setter3"); this.val = 12;} });
    var z = function() {
       yy = 20;
       telemetryLog(`val is ${yy}`, true);
       telemetryLog(`val is ${yy.val}`, true);    
    }
    return z;
}
var foo3 = top3();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog("test1: nested setter without getter", true);
    foo1();

    telemetryLog("test2: nested setter and setter", true);
    foo2();

    telemetryLog("test3: nested setter and setter from this", true);
    foo3();

    emitTTDLog(ttdLogURI);
}