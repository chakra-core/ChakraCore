//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
function func_0(){ return "" };

var tests = [
  {
    name: "Expect exception with invalid file name to WScript.LoadScript",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadScript(``, ``, {
                toString: function () {
                    func_0();
                }}
            );        
        }, Error, 
        "Should throw for invalid input to WScript.LoadScript", 
        "Unsupported argument type inject type.");
    }
  },
  {
    name: "Expect exception with invalid type to WScript.LoadScript",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadScript(``, {
                toString: function () {
                    func_0();
                }}, ``
            );        
        }, Error, 
        "Should throw for invalid input to WScript.LoadScript", 
        "Unsupported argument type inject type.");
    }
  },
  {
    name: "Expect exception with invalid content to WScript.LoadScript",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadScript({
                toString: function () {
                    func_0();
                }}, ``, ``
            );        
        }, Error, 
        "Should throw for invalid input to WScript.LoadScript", 
        "Unsupported argument type inject type.");
    }
  },
  {
    name: "Expect exception with invalid file name to WScript.LoadModule",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadModule(``, ``, {
                toString: function () {
                    func_1();
                }}
            );        
        }, ReferenceError, 
        "'func_1' is not defined");
    }
  },
  {
    name: "Expect exception with invalid type to WScript.LoadModule",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadModule(``, {
                toString: function () {
                    func_1();
                }}, ``
            );        
        }, ReferenceError, 
        "Should throw for invalid input to WScript.LoadModule", 
        "'func_1' is not defined");
    }
  },
  {
    name: "Expect exception with invalid content to WScript.LoadModule",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadModule({
                toString: function () {
                    func_1();
                }}, ``, ``
            );        
        }, ReferenceError, 
        "Should throw for invalid input to WScript.LoadModule", 
        "'func_1' is not defined");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
