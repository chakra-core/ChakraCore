//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = new Array(0x15000); //makes this sparse

var i=0;

for(var i=50;i<60;i++)
{
  a[i] = i+10;
}

for(var i=0;i<10;i++)
{
  a[i] = i+20;
}

for(var i=100;i<110;i++)
{
  a[i] = i*10;
}

var b = new Array(0x15000); //makes this sparse

for(var i=50;i<60;i++)
{
  a[i] = i+10;
}

for(var i=0;i<10;i++)
{
  a[i] = i+20;
}

for(var i=100;i<110;i++)
{
  a[i] = i+40;
}

var c = a.concat(b);

var  d = a.slice(10);

var x = [];
x[0xFFFFFFFF] = 0;
x[0xFFFFFFFE] = 1;
x[0xFFFFFFFD] = 2;

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog(`${c[50]}`, true);
    telemetryLog(`${c[0]}`, true);

    telemetryLog(`${a.shift()}`, true);
    telemetryLog(`${a[7]}`, true);
    telemetryLog(`${a[8]}`, true);
    telemetryLog(`${a.shift()}`, true);
    telemetryLog(`${a.length}`, true);

    telemetryLog(`${d[41]}`, true);
    telemetryLog(`${d[90]}`, true);

    a.splice(45,3,"a","b","c");

    telemetryLog(`${a[45]}`, true);
    telemetryLog(`${a[46]}`, true);
    telemetryLog(`${a[50]}`, true);
    telemetryLog(`${a[100]}`, true);
    telemetryLog(`${a.length}`, true);

    telemetryLog(`${x[0xFFFFFFFF]} ${x.length}`, true);
    telemetryLog(`${x[0xFFFFFFFE]} ${x.length === 0xFFFFFFFF}`, true);
    telemetryLog(`${x[0xFFFFFFFD]} ${x.length === 0xFFFFFFFF}`, true);

    emitTTDLog(ttdLogURI);
}