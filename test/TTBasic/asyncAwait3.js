//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function resolveWait(x) {
    return new Promise(resolve => {
        WScript.SetTimeout(() => {
            resolve(x);
        }, 100);
    });
}

async function awaitLater(x) {
    const p_a = resolveWait(10);
    const p_b = resolveWait(20);
    return x + await p_a + await p_b;
}

WScript.SetTimeout(function () {
    awaitLater(10).then(v => {
        telemetryLog(v.toString(), true);
        emitTTDLog(ttdLogURI);
    });
}, 0);
