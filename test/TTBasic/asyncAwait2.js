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

async function awaitImm(x) {
    const a = await resolveWait(1);
    const b = await resolveWait(2);
    return x + a + b;
}

awaitImm(1).then(v => {
    telemetryLog(v.toString(), true);
    emitTTDLog(ttdLogURI);
});
