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

function* testGeneratorInternal(arg1) {
    while (arg1 < 5) {
        yield ++arg1;
    }
}


function* testGenerator(arg1) {
    var int = testGeneratorInternal(arg1);
    for (var curr = int.next(); curr && !curr.done; curr = int.next()) {
        yield curr.value;
    }
}

var gen = testGenerator(1);

function yieldOne() {
    var v1 = gen.next();
    var val = JSON.stringify(v1.value, undefined, '');
    log(`gen.next() = {value: ${val}, done: ${v1.done}}`, true);
}

function consumeRemainder() {
    var v1;
    do {
        v1 = gen.next();
        var val = 'undefined';
        if (v1.value !== undefined) {
            val = JSON.stringify(v1.value, undefined, '');
        }
        log(`gen.next() = {value: ${val}, done: ${v1.done}}`, true);
    } while (v1 && !v1.done);
}

WScript.SetTimeout(() => {
    yieldOne();
}, 50);

WScript.SetTimeout(() => {
    yieldOne();
}, 100);

WScript.SetTimeout(() => {
    consumeRemainder();
    writeTTDLog(ttdLogURI);
}, 200);