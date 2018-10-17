//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

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
        "ScriptError");
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
        "ScriptError");
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
        "ScriptError");
    }
  },
  {
    name: "Expect exception with invalid file name to WScript.LoadModule",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadModule(``, ``, {
                toString: function () {
                    func_0();
                }}
            );        
        }, Error, 
        "Should throw for invalid input to WScript.LoadModule", 
        "ScriptError");
    }
  },
  {
    name: "Expect exception with invalid type to WScript.LoadModule",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadModule(``, {
                toString: function () {
                    func_0();
                }}, ``
            );        
        }, Error, 
        "Should throw for invalid input to WScript.LoadModule", 
        "ScriptError");
    }
  },
  {
    name: "Expect exception with invalid content to WScript.LoadModule",
    body: function () {  
        assert.throws(function () { 
            WScript.LoadModule({
                toString: function () {
                    func_0();
                }}, ``, ``
            );        
        }, Error, 
        "Should throw for invalid input to WScript.LoadModule", 
        "ScriptError");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
