//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var log;
if ( typeof telemetryLog === 'undefined' ) {
    log =  function(msg, shouldWrite) {
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


function* testGenerator() {
    var i = 0;
    writeTTDLog(ttdLogURI);
    yield i++;
    writeTTDLog(ttdLogURI);
    yield i++;
    writeTTDLog(ttdLogURI);
    yield i++;
    writeTTDLog(ttdLogURI);
    yield i++;
    writeTTDLog(ttdLogURI);
}

var gen = testGenerator();

function yieldOne() {
    var v1 = gen.next();
    log(`gen.next() = {value: ${v1.value}, done: ${v1.done}}`, true);
}


WScript.SetTimeout(() => {
    yieldOne();
}, 20);

WScript.SetTimeout(() => {
    yieldOne();
}, 40);

WScript.SetTimeout(() => {
    yieldOne();
}, 60);

WScript.SetTimeout(() => {
    yieldOne();
}, 1000);

WScript.SetTimeout(() => {
    writeTTDLog(ttdLogURI);
}, 1000);