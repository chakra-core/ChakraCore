//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function printError(e) {
    print(e.name);
    print(e.number);
    print(e.description);
}

var isMac = (WScript.Platform && WScript.Platform.OS == 'darwin');

var expects = [
    '#1', // 0
    'In finally',
    'Error: Out of stack space', // 2
    '#2',
    'In finally', // 4
    'Error: Out of stack space',
    '#3', // 6
    'In finally',
    'Error: Out of stack space', // 8
    'testing stack overflow handling with catch block',
    'Error: Out of stack space', // 10
    'testing stack overflow handling with finally block',
    'Error: Out of stack space' ]; // 12

if (!isMac) // last test (sometimes) we hit timeout before we hit stackoverflow.
    expects.push('Error: Out of stack space')

expects.push('END');

var index = 0;
function printLog(str) {
    if (expects[index++] != str) {
        WScript.Echo('At ' + (index - 1) + ' expected \n' + expects[index - 1] + '\nOutput:' + str);
        WScript.Quit(1);
    }
}

for (var i = 1; i < 4; i++) {
    printLog("#" + i);
    try {
        try {
            function f() {
                f();
            }
            f();
        } finally {
            printLog("In finally");
        }
    }
    catch (e) {
        printLog(e);
    }
}

printLog("testing stack overflow handling with catch block");
try {
    function stackOverFlowCatch() {
        try {
            stackOverFlowCatch();
            while (true) {
            }

        }
        catch (e) {
            throw e;
        }
    }
    stackOverFlowCatch();
}
catch (e) {
    printLog(e);
}

printLog("testing stack overflow handling with finally block");
try
{
    function stackOverFlowFinally() {
        try {
            stackOverFlowFinally();
            while (true) {
            }
        }
        finally {
            DoSomething();
        }
    }
    stackOverFlowFinally();
}
catch(e) {
    printLog(e);
}

function DoSomething()
{
}

// 10K is not enough for our osx setup.
// for bigger numbers, we hit to timeout on CI (before we actually hit to S.O)
if (!isMac) {
    try
    {
        var count = 20000;

        var a = {};
        var b = a;

        for (var i = 0; i < count; i++)
        {
            a.x = {};
            a = a.x;
        }
        eval("JSON.stringify(b)");
    }
    catch(e) {
        printLog(e);
    }
}

printLog('END'); // do not remove this

WScript.Echo("Pass");
