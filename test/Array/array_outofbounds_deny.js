//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var helpers = function helpers() {
    //private
    var undefinedAsString = "undefined";

    var isInBrowser = function isInBrowser() {
        return typeof (document) !== undefinedAsString;
    };

    return {
        // public
        getDummyObject: function () {
            //return isInBrowser() ? document : {};
            return {};
        },

        writeln: function writeln() {
            var line = "", i;
            for (i = 0; i < arguments.length; i += 1) {
                line = line.concat(arguments[i])
            }
            if (!isInBrowser()) {
                WScript.Echo(line);
            } else {
                document.writeln(line);
                document.writeln("<br/>");
            }
        },

        printObject: function printObject(o) {
            var name;
            for (name in o) {
                this.writeln(name, o.hasOwnProperty(name) ? "" : " (inherited)", ": ", o[name]);
            }
        }
    }
}(); // helpers module.

var executedTestCount = 2;
var passedTestCount = 0;

helpers.writeln("*** Running test #1 (0): Deny out-of-bounds write");
try {
    let arr = new Array(2);
    helpers.writeln("arr.length is " + arr.length);

    arr[0] = 0;
    helpers.writeln("arr[0] is " + arr[0]);

    arr[1] = 1;
    helpers.writeln("arr[1] is " + arr[1]);

    arr[100] = 100;
    helpers.writeln("FAILED");
} catch (e) {
    helpers.writeln(e.name + ": " + e.message);
    helpers.writeln("PASSED");
    ++passedTestCount;
}


helpers.writeln("*** Running test #2 (1): Deny out-of-bounds read");
try {
    let arr = new Array(2);
    helpers.writeln("arr.length is " + arr.length);

    arr[0] = 0;
    helpers.writeln("arr[0] is " + arr[0]);

    arr[1] = 1;
    helpers.writeln("arr[1] is " + arr[1]);

    helpers.writeln("arr[100] is " + arr[100]);
    helpers.writeln("FAILED");
} catch (e) {
    helpers.writeln(e.name + ": " + e.message);
    helpers.writeln("PASSED");
    ++passedTestCount;
}

helpers.writeln("Summary of tests: total executed: ", executedTestCount,
        "; passed: ", passedTestCount, "; failed: ", executedTestCount - passedTestCount);