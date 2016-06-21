//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (WScript.arguments.length != 2) {
    WScript.Echo("ERROR: Invalid number of argument");
    WScript.Quit(-1);
}

var input = WScript.arguments(0);
var output = WScript.arguments(1);
var fso = WScript.CreateObject("Scripting.FileSystemObject");

try {
    var f = fso.OpenTextFile(input, 1);
}
catch (e) {
    WScript.Echo("ERROR: unable to open input file " + input);
    WScript.Quit(-1);
}

var str = f.ReadAll();

f.Close();

if (str.length == 0) {
    WScript.Echo("ERROR: input file is empty");
    WScript.Quit(-1);
}

try {
    var out = fso.OpenTextFile(output, 2, true, 0);
}
catch (e) {
    WScript.Echo("ERROR: unable to open output file " + output);
    WScript.Quit(-1);
}

function writeChar(c) {
    var line = false;

    if (c == "\\") {
        c = "\\\\";
    } else if (c == "'" || c == "\"") {
        c = "\\" + c;
    } else if (c == "\r") {
        c = "\\r";
    } else if (c == "\n") {
        c = "\\n";
        line = true;
    }


    out.Write("L'" + c + "'");
    out.Write(",");
    if (line) {
        out.WriteLine("");
    }
}

for (var i = 0; i < str.length; i++) {
    writeChar(str.charAt(i));
}

out.Close();
