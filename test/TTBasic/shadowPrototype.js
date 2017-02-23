//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function check(o, v)
{
    o.value(v);
}

function first()
{
}

function isFirst(v) { telemetryLog(`v is ${v} expecting 1.`, true); }
function isSecond(v) { telemetryLog(`v is ${v} expecting 2.`, true); }

first.value = isFirst;

function second()
{
}
second.prototype = first;

function third()
{}

third.prototype = new second();

var obj1 = new third();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    check(obj1, 1);

    third.prototype.value = isSecond;

    check(obj1, 2);

    delete third.prototype.value;

    check(obj1, 1);

    emitTTDLog(ttdLogURI);
}
