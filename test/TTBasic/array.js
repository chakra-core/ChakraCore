//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = [3, null, {}];
var y = x;
y.baz = 5;

var q = [1, 2];
q.length = 5;

var qq = [1, 2, 3, 4, 5];
qq.pop();
qq.pop();

var qqq = [1, 2, 3, 4, 5];
delete qqq[3];

var o = new Object();
o.length = 10;

var o1 = new Object();
var a = [1000,2000,3000];

a.x = 40;
a[o] = 50;

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`Array.isArray(x): ${Array.isArray(x)}`, true);  //true
    telemetryLog(`x.length: ${x.length}`, true); //3

    telemetryLog(`x === y: ${x === y}`, true); //true
    telemetryLog(`x.baz: ${x.baz}`, true); //5
    telemetryLog(`x[0]: ${x[0]}`, true); //3
    telemetryLog(`y[1]: ${y[1]}`, true); //null
    telemetryLog(`x[5]: ${x[5]}`, true); //undefined

    ////
    x[1] = "non-null";
    x[5] = { bar: 3 };
    x.push(10);
    ////

    telemetryLog(`post update -- y[1]: ${y[1]}`, true); //non-null
    telemetryLog(`post update -- x[5] !== null: ${x[5] !== null}`, true); //true
    telemetryLog(`post update -- x[5].bar: ${x[5].bar}`, true); //3
    telemetryLog(`post update -- y[6]: ${y[6]}`, true); //10

    telemetryLog(`q.length: ${q.length}`, true); //5
    telemetryLog(`q[3]: ${q[3]}`, true); //undefined

    telemetryLog(`qq.length: ${qq.length}`, true); //3
    telemetryLog(`qq[3]: ${qq[3]}`, true); //undefined

    telemetryLog(`qqq.length: ${qqq.length}`, true); //5
    telemetryLog(`qqq[3]: ${qq[undefined]}`, true); //undefined

    telemetryLog(`a[o]: ${a[o]}`, true); //50
    telemetryLog(`a[o1]: ${a[o1]}`, true); //50
    telemetryLog(`a["[object Object]"]: ${a["[object Object]"]}`, true); //50
    telemetryLog(`a["[object" + " Object]"]: ${a["[object" + " Object]"]}`, true); //50

    emitTTDLog(ttdLogURI);
}