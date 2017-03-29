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

            if (diff < 0) throw new Error ("Timer interval has failed. diff < 0");
            pre_time = now;
        }

        // wait rand time until next trial
        for(var i = 0, to = rand(); i < to; i++) {
            now = Date.now();
        }

        if (now < pre_time) throw new Error ("Timer interval has failed. now < pre_time");
    }
} // !win32
