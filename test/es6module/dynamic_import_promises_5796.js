//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Dynamic import should always resolve or reject a promise
// and it should never throw an unhandled exception
// https://github.com/Microsoft/ChakraCore/issues/5796

const promises = [];

function testDynamicImport(testCase, shouldThrow = false, errorType = URIError)
{
    if (shouldThrow) {
        promises.push(testCase
            .then(() => print("Promise should be rejected"))
            .catch (e => {if (!(e instanceof errorType)) throw new Error("fail");})
            .catch (() => print("Wrong error type"))
            );
    } else {
       promises.push(testCase.then(() => true).catch(e => print ("Test case failed")));
    }
}

// Invalid specifiers, these produce promises rejected with URIErros
testDynamicImport(import(undefined), true);
testDynamicImport(import(true), true);
testDynamicImport(import(false), true);
testDynamicImport(import({}), true);
testDynamicImport(import(' '), true);

WScript.RegisterModuleSource("case1", 'this is a syntax error');
WScript.RegisterModuleSource("case2", 'import "helper1";');
WScript.RegisterModuleSource("helper1", 'this is a syntax error');
WScript.RegisterModuleSource("case3", 'import "case1";');
WScript.RegisterModuleSource("case4", 'throw new TypeError("error");');
WScript.RegisterModuleSource("case5", 'import "case3";');
WScript.RegisterModuleSource("case6", 'import "case4";');
WScript.RegisterModuleSource("helper2", 'throw new TypeError("error");');
WScript.RegisterModuleSource("case7", 'import "helper2";');
WScript.RegisterModuleSource("passThrough", 'import "helper3"');
WScript.RegisterModuleSource("helper3", 'throw new TypeError("error");');
WScript.RegisterModuleSource("case8", 'import "passThrough";');
WScript.RegisterModuleSource("case9", 'import "case8";');

// syntax error at first level
testDynamicImport(import("case1"), true, SyntaxError);
// syntax error at second level
testDynamicImport(import("case2"), true, SyntaxError);
// syntax error at second level from already imported module
testDynamicImport(import("case3"), true, SyntaxError);
// Type Error at run time at first level
testDynamicImport(import("case4"), true, TypeError);
// Syntax error at 3rd level
testDynamicImport(import("case5"), true, SyntaxError);
// Indirectly re-Import the module that threw the type error
// Promise should be resolved as the child module won't be evaluated twice
testDynamicImport(import("case6"), true, TypeError);
// Type Error at run time at second level
testDynamicImport(import("case7"), true, TypeError);
// Type Error at run time at third level
testDynamicImport(import("case8"), true, TypeError);
// Type Error at run time in a child that has already thrown
testDynamicImport(import("case9"), true, TypeError);

Promise.all(promises).then(() => print ("pass"));
