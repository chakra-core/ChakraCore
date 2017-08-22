//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

try {
    var failed = false;
    function getErrorFunction(global) {
        return function () {
            failed = true;
            WScript.Echo("Error when tainting '" + global + "'!");
        }
    }

    function generalTainting() {
        // tainting built-in object constructors and functions
        Date = getErrorFunction("Date");
        Object = getErrorFunction("Object");
        Number = getErrorFunction("Number");
        RegExp = getErrorFunction("RegExp");
        String = getErrorFunction("String");
        Boolean = getErrorFunction("Boolean");
        Error = getErrorFunction("Error");
        TypeError = getErrorFunction("TypeError");
        RangeError = getErrorFunction("RangeError");
        Map = getErrorFunction("Map");

        Math = {
            abs: getErrorFunction("Math.abs"),
            floor: getErrorFunction("Math.floor"),
            max: getErrorFunction("Math.max"),
            pow: getErrorFunction("Math.pow")
        };

        isFinite = getErrorFunction("isFinite");
        isNaN = getErrorFunction("isNaN");
    }

    function objectTainting() {
        Object.create = getErrorFunction("Object.create");
        Object.defineProperty = getErrorFunction("Object.defineProperty");
        Object.getPrototypeOf = getErrorFunction("Object.getPrototypeOf");
        Object.isExtensible = getErrorFunction("Object.isExtensible");
        Object.getOwnPropertyNames = getErrorFunction("Object.getOwnPropertyNames");
        Object.prototype.hasOwnProperty = getErrorFunction("Object.prototype.hasOwnProperty");
    }

    function arrayTainting() {
        Array.prototype.forEach = getErrorFunction("Array.prototype.forEach");
        Array.prototype.indexOf = getErrorFunction("Array.prototype.indexOf");
        Array.prototype.push = getErrorFunction("Array.prototype.push");
        Array.prototype.join = getErrorFunction("Array.prototype.join");
    }

    function stringTainting() {
        String.prototype.match = getErrorFunction("String.prototype.match");
        String.prototype.replace = getErrorFunction("String.prototype.replace");
        String.prototype.toLowerCase = getErrorFunction("String.prototype.toLowerCase");
        String.prototype.toUpperCase = getErrorFunction("String.prototype.toUpperCase");
    }

    function otherProtototypeTainting() {
        Function.prototype.bind = getErrorFunction("Function.prototype.bind");
        Date.prototype.getDate = getErrorFunction("Date.prototype.getDate");
        RegExp.prototype.test = getErrorFunction("RegExp.prototype.test");
    }

    function runTests() {
        failed = false;

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat().format(5);
        new Intl.Collator().compare(null, "");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat().format(5);
        new Intl.Collator().compare(null, "");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat("en", { month: "short" }).format(5);
        new Intl.Collator().compare("en", "");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat("en", { month: "short" }).format(5);
        new Intl.Collator().compare("en", "");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat("en", { month: "short" }).format(5);
        new Intl.Collator().compare("en", "");

        if (failed === false) {
            WScript.Echo("Passed pre-init tainting!");
        }
    }

    objectTainting();
    arrayTainting();
    stringTainting();
    otherProtototypeTainting();
    generalTainting();
    runTests();

} catch (e) {
    WScript.Echo(e);
}
