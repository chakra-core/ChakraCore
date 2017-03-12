//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// dump obj through for-in enumerator
function enumObj(obj, lines) {
    for (var p in obj) {
        lines.push("  " + p + ": " + obj[p]);
    }
}

// dump obj through for-in enumerator + verify
function enumAndVerifyObj(obj, lines) {
    var ctr = 0;
    for (var p in obj) {
        var thisl = "  " + p + ": " + obj[p];
        if(lines[ctr] !== thisl) {
            telemetryLog(`Failed on ${lines[ctr]} !== ${thisl}`, true);
        }
        ctr++;
    }
}

// add a bunch of data/attribute properties with different attributes
function addProp(o, prefix) {
    Object.defineProperty(o, prefix + "10", {
        value: "value 10"
    });
    Object.defineProperty(o, prefix + "11", {
        value: "value 11",
        enumerable: true
    });
    Object.defineProperty(o, prefix + "12", {
        value: "value 12",
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(o, prefix + "13", {
        value: "value 13",
        enumerable: true,
        configurable: true,
        writable: true
    });

    Object.defineProperty(o, prefix + "20", {
        get: function() { return "get 20"; },
    });
    Object.defineProperty(o, prefix + "21", {
        get: function () { return "get 21"; },
        enumerable: true,
    });
    Object.defineProperty(o, prefix + "22", {
        get: function () { return "get 22"; },
        enumerable: true,
        configurable: true
    });

    Object.defineProperty(o, prefix + "25", {
        set: function() { echo("do not call 25"); },
    });
    Object.defineProperty(o, prefix + "26", {
        set: function() { echo("do not call 26"); },
        enumerable: true,
    });
    Object.defineProperty(o, prefix + "27", {
        set: function() { echo("do not call 27"); },
        enumerable: true,
        configurable: true
    });
}

function testWithObj(o, lines) {
    addProp(o, "xx");
    addProp(o, "1");
    enumObj(o, lines);
}

var s1 = {abc: -12, def: "hello", 1: undefined, 3: null};
var l1 = [];
testWithObj(s1, l1);

var s2 = [-12, "hello", undefined, null];
var l2 = [];
testWithObj(s2, l2);

// Test Object.defineProperties, Object.create
function testPrototype(proto) {
    Object.defineProperties(proto, {
        name: { value: "SHOULD_NOT_enumerate_prototype" },
        0: { get: function() { return "get 0"; } },
        3: { value: 3 },
        1: { get: function() { return "get 1"; }, enumerable: true },
        5: { value: 5, enumerable: true },
        2: { get: function() { return this.name; }, enumerable: true },
    });

    return Object.create(proto, {
        name: { value: "correct_original_instance" },
        10: { get: function() { return "get 10"; } },
        13: { value: 13 },
        11: { get: function() { return "get 11"; }, enumerable: true },
        15: { value: 15, enumerable: true },
        12: { get: function() { return this.name; }, enumerable: true },        
    });
}

var s3 = testPrototype({});
var l3 = [];
testWithObj(s3, l3);

var s4 = testPrototype([]);
var l4 = [];
testWithObj(s4, l4);

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog('Testing obj enumeration', true);
    enumAndVerifyObj(s1, l1);

    telemetryLog('Testing array enumeration', true);
    enumAndVerifyObj(s2, l2);

    telemetryLog('Testing obj proto enumeration', true);
    enumAndVerifyObj(s3, l3);

    telemetryLog('Testing array proto enumeration', true);
    enumAndVerifyObj(s4, l4);

    emitTTDLog(ttdLogURI);
}