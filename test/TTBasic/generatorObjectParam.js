//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var log;
if (typeof telemetryLog === 'undefined') {
    log = function (msg, shouldWrite) {
        if (shouldWrite) {
            WScript.Echo(msg);
        }
    };
}
else {
    log = telemetryLog;
}

var writeTTDLog;
if (typeof emitTTDLog === 'undefined') {
    writeTTDLog = function (uri) {
        // no-op
    };
}
else {
    writeTTDLog = emitTTDLog;
}

/////////////////

var p = { v: 5 };

function* testGenerator(arg1, arg2) {
    var i = 100;
    var j = 1000;
    var k = 10000;
    arg2.v++;
    p.v++;
    yield { arg1: arg1++, arg2, i: ++i, j: j++, k: k++, p };
    arg2.v++;
    p.v++;
    yield { arg1: arg1++, arg2, i: ++i, j: j++, k: k++, p };
    arg2.v++;
    p.v++;
    yield { arg1: arg1++, arg2, i: ++i, j: j++, k: k++, p };
    arg2.v++;
    p.v++;
    yield { arg1: arg1++, arg2, i: ++i, j: j++, k: k++, p };
}

var gen = testGenerator(10, { v: 2000, q: 3000 });

function yieldOne() {
    var v1 = gen.next();
    var val = JSON.stringify(v1.value, undefined, '');
    log(`gen.next() = {value: ${val}, done: ${v1.done}}`, true);
}


WScript.SetTimeout(() => {
    yieldOne();
}, 50);

WScript.SetTimeout(() => {
    yieldOne();
}, 200);

WScript.SetTimeout(() => {
    yieldOne();
    writeTTDLog(ttdLogURI);
}, 350);