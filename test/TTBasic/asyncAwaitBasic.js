//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var log;
if (typeof telemetryLog !== 'undefined') {
    log = telemetryLog;
}
else if (typeof console !== 'undefined' && typeof console.log !== 'undefined') {
    log = function (msg, shouldWrite) {
        if (shouldWrite) {
            console.log(msg);
        }
    }
}
else {
    log = function (msg, shouldWrite) {
        if (shouldWrite) {
            WScript.Echo(msg);
        }
    };
}

var setTimeoutX;
if (typeof setTimeout !== 'undefined') {
    setTimeoutX = setTimeout;
}
else {
    setTimeoutX = WScript.SetTimeout;
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

var ttdLogUriX;
if (typeof ttdLogURI !== 'undefined') {
    ttdLogUriX = ttdLogURI;
}
/////////////////

async function f1(a, b, c) {
    log('f1 starting', true);
    return { a, b, c };
}

async function f2(d, e, f) {
    log('f2 starting', true);
    let x = await f1(d + 10, e + 20, f + 30);
    return x;
}

async function f3() {
    log('f3 starting', true);
    var x = await f2(1, 2, 3);
    var xstr = JSON.stringify(x, undefined, 0);
    log(`x = ${xstr}`, true);
}

setTimeoutX(() => {
    f3().then(() => {
        log('done', true);
        writeTTDLog(ttdLogUriX);
    }, (err) => {
        log(`error:  ${err}`, true);
        writeTTDLog(ttdLogUriX);
    })
}, 20);
