//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*

See:
    gh-249
    OS#14101048

According to #sec-forbidden-extensions
For all built-in functions `func`, `func.hasOwnProperty('arguments')` and `func.hasOwnProperty('caller')` must return false.

    Array.prototype.push.hasOwnProperty('arguments') // === false
    Array.prototype.push.hasOwnProperty('caller') // === false

*/

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

let isVerbose = WScript.Arguments[0] != "summary";

let global = this;

function getFunctions(ns) {
    return Object.getOwnPropertyNames(ns)
        .filter(x => {
            // print(x);
            if (['caller', 'arguments'].includes(x)) {
                return false;
            }
            return typeof ns[x] === 'function'
        })
        .sort()
        .map(x => {
            return [x, ns[x]];
        })
}

let builtins = {
    "global": getFunctions(global).filter(x => ![
        'getFunctions',
    ].includes(x[0])),

    "Object.prototype": getFunctions(Object.prototype),
    "String.prototype": getFunctions(String.prototype),
    "RegExp.prototype": getFunctions(RegExp.prototype),
    "Function.prototype": getFunctions(Function.prototype),
    "Array.prototype": getFunctions(Array.prototype),
    "Uint8Array.prototype": getFunctions(Uint8Array.prototype),
    "Uint8ClampedArray.prototype": getFunctions(Uint8ClampedArray.prototype),
    "Uint16Array.prototype": getFunctions(Uint16Array.prototype),
    "Uint32Array.prototype": getFunctions(Uint32Array.prototype),
    "Int8Array.prototype": getFunctions(Int8Array.prototype),
    "Int16Array.prototype": getFunctions(Int16Array.prototype),
    "Int32Array.prototype": getFunctions(Int32Array.prototype),
};

if (typeof Intl !== "undefined") {
    builtins["Intl"] = getFunctions(Intl);
    builtins["Intl.Collator"] = getFunctions(Intl.Collator);
    builtins["Intl.Collator.prototype"] = getFunctions(Intl.Collator.prototype);
    builtins["Intl.DateTimeFormat"] = getFunctions(Intl.DateTimeFormat);
    builtins["Intl.DateTimeFormat.prototype"] = getFunctions(Intl.DateTimeFormat.prototype);
    builtins["Intl.NumberFormat"] = getFunctions(Intl.NumberFormat);
    builtins["Intl.NumberFormat.prototype"] = getFunctions(Intl.NumberFormat.prototype);
}

var tests = [
    {
        name: "builtin functions hasOwnProperty('arguments'|'caller') === false",
        body: function () {
            function test(f, p, name) {
                if (isVerbose) {
                    print(`Checking: ${name}.hasOwnProperty('${p}') === false`);
                }
                assert.areEqual(false, f.hasOwnProperty(p), `Expected (${name}.hasOwnProperty('${p}') === false) but actually true`);
            }

            for (let ns in builtins) {
                let functions = builtins[ns];
                for (let f of functions) {
                    let funcName = f[0];
                    let func = f[1];
                    let fullName = `${ns}.${funcName}`;

                    test(func, 'arguments', fullName);
                    test(func, 'caller', fullName);
                }
            }
        }
    }
];

testRunner.runTests(tests, { verbose: isVerbose });