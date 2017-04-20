//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// test custom GetSystemTime caching on xplat
if (WScript.Platform && WScript.Platform.OS != "win32") {
    function rand() {
        return parseInt(Math.random() * 1e2) + 50;
    }

    for (var j = 0; j < 1e2; j++) {
        var pre_time = Date.now(), now;
        for(var i = 0; i < 1e6; i++) {
            now = Date.now();
            var diff = now - pre_time

            // INTERVAL_FOR_TICK_BACKUP = 5
            // So, anything beyond 5ms is not subject to our testing here.
            if (diff < 0 && Math.abs(diff) <= 5)
            {
                throw new Error ("Timer interval has failed. diff < 0 -> " + diff);
            }

            pre_time = now;
        }

        // wait rand time until next trial
        for(var i = 0, to = rand(); i < to; i++) {
            now = Date.now();
        }

        // INTERVAL_FOR_TICK_BACKUP = 5
        // So, anything beyond 5ms is not subject to our testing here.
        if (now < pre_time && Math.abs(now - pre_time) <= 5)
        {
            throw new Error ("Timer interval has failed. now < pre_time");
        }
    }
} // !win32

print("PASS");
