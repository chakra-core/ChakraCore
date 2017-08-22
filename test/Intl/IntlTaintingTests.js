//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function throwFunction() { throw new Error(); }
function verifyPropertyTainting(property, func, attributes) {
    try {
        attributes.configurable = true;
        Object.defineProperty(Object.prototype, property, attributes);
        var result = func();
        delete Object.prototype[property];
        return result;
    }
    catch (e) {
        delete Object.prototype[property];
        throw e;
    }
}
//Pass
verifyPropertyTainting("bob", function () { return Intl.Collator.supportedLocalesOf(); }, { value: throwFunction })
//Actual tests, much pass (Bug 362896)
verifyPropertyTainting("enumerable", function () { return Intl.Collator.supportedLocalesOf(); }, { value: throwFunction })
verifyPropertyTainting("get", function () { return Intl.Collator.supportedLocalesOf(); }, { value: throwFunction })
verifyPropertyTainting("set", function () { return Intl.Collator.supportedLocalesOf(); }, { value: throwFunction })

//Testing to make sure regex doesn't change.
"a".match(/(a)/);
var before = {};
Object.getOwnPropertyNames(RegExp).forEach(function (key) { before[key] = RegExp[key]; });
new Intl.NumberFormat("en-US", { style: "currency", currency: "USD" }).format(5);
new Intl.Collator().compare("a", "b");
new Intl.DateTimeFormat().format(new Date());
Object.getOwnPropertyNames(RegExp).forEach(function (key) { if (RegExp[key] !== before[key]) WScript.Echo("Built-In regex implementation overwrote the global constructor's value."); });
try {
    var failed = false;
    function getErrorFunction(global) {
        return function () {
            failed = true;
            WScript.Echo("Error when tainting '" + global + "'!");
        }
    }

    function generalTainting() {
        failed = false;
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

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat().format(5);
        new Intl.Collator().compare(null, "");
        if (failed === false) {
            WScript.Echo("Passed general tainting!");
        }
    }

    function objectTainting() {
        failed = false;
        Object.create = getErrorFunction("Object.create");
        Object.defineProperty = getErrorFunction("Object.defineProperty");
        Object.getPrototypeOf = getErrorFunction("Object.getPrototypeOf");
        Object.isExtensible = getErrorFunction("Object.isExtensible");
        Object.getOwnPropertyNames = getErrorFunction("Object.getOwnPropertyNames");
        Object.prototype.hasOwnProperty = getErrorFunction("Object.prototype.hasOwnProperty");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat().format(5);
        new Intl.Collator().compare(null, "");

        if (failed === false) {
            WScript.Echo("Passed object prototype tainting!");
        }
    }

    function arrayTainting() {
        failed = false;
        Array.prototype.forEach = getErrorFunction("Array.prototype.forEach");
        Array.prototype.indexOf = getErrorFunction("Array.prototype.indexOf");
        Array.prototype.push = getErrorFunction("Array.prototype.push");
        Array.prototype.join = getErrorFunction("Array.prototype.join");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat("en", { month: "short" }).format(5);
        new Intl.Collator().compare("en", "");

        if (failed === false) {
            WScript.Echo("Passed array prototype tainting!");
        }
    }

    function stringTainting() {
        failed = false;
        String.prototype.match = getErrorFunction("String.prototype.match");
        String.prototype.replace = getErrorFunction("String.prototype.replace");
        String.prototype.toLowerCase = getErrorFunction("String.prototype.toLowerCase");
        String.prototype.toUpperCase = getErrorFunction("String.prototype.toUpperCase");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat("en", { month: "short" }).format(5);
        new Intl.Collator().compare("en", "");

        if (failed === false) {
            WScript.Echo("Passed string prototype tainting!");
        }
    }

    function otherProtototypeTainting() {
        failed = false;
        Function.prototype.bind = getErrorFunction("Function.prototype.bind");
        Date.prototype.getDate = getErrorFunction("Date.prototype.getDate");
        RegExp.prototype.test = getErrorFunction("RegExp.prototype.test");

        new Intl.NumberFormat().format(5);
        new Intl.DateTimeFormat("en", { month: "short" }).format(5);
        new Intl.Collator().compare("en", "");

        if (failed === false) {
            WScript.Echo("Passed other tainting!");
        }
    }

    objectTainting();
    arrayTainting();
    stringTainting();
    otherProtototypeTainting();
    generalTainting();

} catch (e) {
    WScript.Echo(e);
}

Intl.NumberFormat = undefined;

(0.0).toLocaleString();
