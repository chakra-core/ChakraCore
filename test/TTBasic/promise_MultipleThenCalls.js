//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let val = -7

let turi = '';
let elog = () => {};
let tlog = (msg, val) => { console.log(msg); }
if (typeof telemetryLog !== 'undefined') {
    tlog = telemetryLog;
    elog = emitTTDLog;
    turi = ttdLogURI;
}

function addThens()
{
    let resolveFunc;
    const p = new Promise((resolve, reject) => {
        resolveFunc = resolve;
    });
    
    WScript.SetTimeout(() => {
        p.then(() => {
            val = val * 3;
        });
    }, 200);
    
    WScript.SetTimeout(() => {        
        p.then(() => {
            val = val + 21
        });
    }, 300);

    WScript.SetTimeout(resolveFunc, 400);
    WScript.SetTimeout(testFunction, 1000);
}

WScript.SetTimeout(addThens, 100);

function testFunction()
{
    tlog(`val is ${val} (Expect 0)`, true);

    elog(turi);
}
