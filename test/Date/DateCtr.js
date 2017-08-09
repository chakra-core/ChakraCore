//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var RESULTS = [
  "1970-01-10T00:00:01.000Z",
  "1974-01-01T00:00:00.000Z",
  "1970-01-01T00:00:01.974Z",
  "1974-10-01T07:00:00.000Z",
  "1974-10-24T07:00:00.000Z",
  "1974-10-24T07:00:00.000Z",
  "1974-10-24T07:20:00.000Z",
  "1974-10-24T07:20:30.000Z",
  "1974-10-24T07:20:30.040Z",
  "1974-10-24T07:20:30.040Z",
  "-098001-12-01T08:00:00.000Z",
  "1999-12-01T08:00:00.000Z",
  "Invalid Date",
  "Invalid Date",
  "Invalid Date" ];


var index = 0;
function CHECK(str)
{
    if (str != RESULTS[index])
    {
        console.log(str + " != " + RESULTS[index]);
        console.log("FAIL");
        return false;
    }
    index++;
    return true;
}

function Test()
{
    var d;

    d = new Date("Thu Jan 10 05:30:01 UTC+0530 1970"); if (!CHECK(d.toISOString())) return;
    d = new Date("1974"); if (!CHECK(d.toISOString())) return;
    d = new Date(1974); if (!CHECK(d.toISOString())) return;
    d = new Date(1974, 9); if (!CHECK(d.toISOString())) return;
    d = new Date(1974, 9, 24); if (!CHECK(d.toISOString())) return;
    d = new Date(1974, 9, 24, 0); if (!CHECK(d.toISOString())) return;
    d = new Date(1974, 9, 24, 0, 20); if (!CHECK(d.toISOString())) return;
    d = new Date(1974, 9, 24, 0, 20, 30); if (!CHECK(d.toISOString())) return;
    d = new Date(1974, 9, 24, 0, 20, 30, 40); if (!CHECK(d.toISOString())) return;
    d = new Date(1974, 9, 24, 0, 20, 30, 40, 50); if (!CHECK(d.toISOString())) return;

    if (!WScript.Platform || WScript.Platform.OS == "win32")
    {
        d = new Date(2000, -1200001); if (!CHECK(d.toISOString())) return; // Make sure there is no AV for negative month (WOOB 1140748).
    } else {
        index++; // xplat DST info for BC doesn't match to Windows.
    }

    d = new Date(2000, -1); if (!CHECK(d.toISOString())) return;  // Check correctness when month is negative.
    d = new Date("", 1e81); if (!CHECK(d + "")) return; // WOOB 1139099
    d = new Date(); d.setSeconds(Number.MAX_VALUE); if (!CHECK(d + "")) return;  // WOOB 1142298
    d = new Date(); d.setSeconds(-Number.MAX_VALUE); if (!CHECK(d + "")) return; // WOOB 1142298
    console.log("PASS");
}

Test();
