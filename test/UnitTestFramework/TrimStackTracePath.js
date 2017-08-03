//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function TrimStackTracePath(line) {
    if (line) {
        line = line.replace(/ \(.+([\\\/]test[\\\/]|[\\\/]unittest[\\\/]).[^\\\/]*./ig, " (");
        // normalize output by replacing leading whitespace (\s+) of each stack line (starting with 'at ') with 3 spaces (to preserve current baselines)
        line = line.replace(/^\s+at /gm, '   at ');
    }
    return line;
}
