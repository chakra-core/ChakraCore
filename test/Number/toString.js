//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

runTest('new Number("444" + "123")');
runTest('new Number(-444123)');
runTest('new Number("444" + "123.789123456789875436")');
runTest('new Number(-444123.78963636363636363636)');
runTest('new Number(0)');
runTest('1e21');

function runTest(numberToTestAsString)
{
    writeLine("Test: " + numberToTestAsString);
    var n = eval(numberToTestAsString + ";");

    writeLine("n.toString():  " + n.toString());
    writeLine("n.toString(10):  " + n.toString(10));
    writeLine("n.toString(8):  " + n.toString(8));
    writeLine("n.toString(2):  " + n.toString(2));
    writeLine("n.toString(16):  " + n.toString(16));
    writeLine("n.toString(25):  " + n.toString(25));

    writeLine("n.toFixed():  " + n.toFixed());
    writeLine("n.toFixed(0):  " + n.toFixed(0));
    writeLine("n.toFixed(2):  " + n.toFixed(2));
    writeLine("n.toFixed(5):  " + n.toFixed(5));
    writeLine("n.toFixed(20):  " + n.toFixed(20));
    safeCall(function () { n.toFixed(-1); });
    safeCall(function () { n.toFixed(21); });

    writeLine("n.toExponential():  " + n.toExponential());
    writeLine("n.toExponential(undefined):  " + n.toExponential(undefined));
    writeLine("n.toExponential(2):  " + n.toExponential(2));
    writeLine("n.toExponential(5):  " + n.toExponential(5));

    writeLine("n.toPrecision():  " + n.toPrecision());
    writeLine("n.toPrecision(2):  " + n.toPrecision(2));
    writeLine("n.toPrecision(5):  " + n.toPrecision(5));
    writeLine("n.toPrecision(20):  " + n.toPrecision(20));

    // test toFixed toString round formatting
    if ( !(1.25499999999999989342.toFixed(2) + "" == "1.25") ||
         !(1.255.toFixed(2) + "" == "1.25") ||
         !(1.245.toFixed(2) + "" == "1.25") ||
         !(8.255.toFixed(2) + "" == "8.26") ) {
        throw Error("1.255.toFixed(2) != 1.25 or 8.255.toFixed(2) != 8.26 ??");
    }

    if (-4.223372036854776e+12 + "" != -4.223372036854776e+12.toFixed(3)) {
        // original number is;
        // -4223372036854.77587890625
        // We don't know the 8 after 775
        // Our default approach is to pick upperBound
        throw Error("-4.223372036854776e+12 -> -4223372036854.776");
    }
    writeLine("");
}

// Helpers

function writeLine(str)
{
    WScript.Echo("" + str);
}

function safeCall(func)
{
    try
    {
        return func();
    }
    catch (ex)
    {
        writeLine(ex.name + ": " + ex.message);
    }
}
