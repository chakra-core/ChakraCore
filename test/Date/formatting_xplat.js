//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var isWindows = !WScript.Platform || WScript.Platform.OS == 'win32';

if (!isWindows) {
    var results = [
        'Sun Mar 11 2012 00:00:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:01:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:02:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:03:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:04:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:05:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:06:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:07:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:08:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:09:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:10:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:11:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:12:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:13:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:14:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:15:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:16:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:17:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:18:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:19:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:20:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:21:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:22:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:23:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:24:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:25:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:26:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:27:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:28:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:29:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:30:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:31:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:32:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:33:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:34:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:35:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:36:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:37:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:38:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:39:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:40:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:41:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:42:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:43:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:44:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:45:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:46:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:47:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:48:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:49:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:50:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:51:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:52:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:53:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:54:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:55:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:56:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:57:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:58:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 00:59:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:00:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:01:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:02:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:03:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:04:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:05:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:06:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:07:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:08:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:09:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:10:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:11:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:12:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:13:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:14:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:15:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:16:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:17:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:18:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:19:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:20:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:21:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:22:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:23:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:24:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:25:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:26:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:27:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:28:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:29:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:30:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:31:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:32:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:33:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:34:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:35:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:36:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:37:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:38:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:39:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:40:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:41:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:42:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:43:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:44:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:45:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:46:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:47:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:48:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:49:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:50:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:51:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:52:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:53:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:54:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:55:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:56:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:57:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:58:00 GMT-0800 (Pacific Standard Time)',
        'Sun Mar 11 2012 01:59:00 GMT-0800 (Pacific Standard Time)',

        'Sat Nov 02 2013 00:00:00 GMT-0700 (Pacific Daylight Time)',
        'Sun Nov 03 2013 00:00:00 GMT-0700 (Pacific Daylight Time)'
    ];

    var result_index = 0;
    function CUT_NAME(str) {
        return str.replace("(PST)", "(Pacific Standard Time)")
                  .replace("(PDT)", "(Pacific Daylight Time)");
    }

    function CHECK(d) {
        d = CUT_NAME(d.toString());
        if (d != results[result_index])
            throw new Error("\nFailed at index: " + result_index
              + "\nexpected -> " + results[result_index] + "\n"
              + "output   -> " + d);

        result_index++;
    }

    for (var i = 0; i < 2 * 60; i++) {
        var d = new Date(2012, 2, 11, 0, i, 0);
        CHECK(d);
    }

    var date;
    date = new Date(2013, 10, 2);
    CHECK(date);
    date = new Date(2013, 10, 3);
    CHECK(date);
}

console.log("PASS")
