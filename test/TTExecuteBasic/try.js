//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var e = 8;

function x() { throw 7; }

function y() {
    var i;
    for (i = 0; i < 10; i++) {
        try {
            if (i % 2 == 0) {
                try {
                    x();
                }
                catch (e) {
                    telemetryLog(`Inner catch: ${e}`, true);
                    if (i % 3) {
                        throw e;
                    }
                    if (i % 5) {
                        return e;
                    }
                }
                finally {
                    telemetryLog(`Finally: ${i}`, true);
                    continue;
                }
            }
        }
        catch (e) {
            telemetryLog(`Outer catch: ${e}`, true);
        }
        finally {
            telemetryLog(`Outer finally: ${i}`, true);
            if (++i % 9 == 0)
                return e;
        }
    }
}

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    y();

    emitTTDLog(ttdLogURI);
}
