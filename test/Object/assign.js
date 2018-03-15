//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "simple copy",
        body: function ()
        {
            let orig = {};
            let sym = Symbol("c");
            orig.a = 1;
            orig.b = "asdf";
            orig[sym] = "qwert";
            let newObj = Object.assign({}, orig);
            assert.areEqual(newObj.b, orig.b);
            assert.areEqual(newObj.a, orig.a);
            assert.areEqual(newObj[sym], orig[sym]);
        }
    },
    {
        name: "non-path type handler",
        body: function ()
        {
            let orig = {};
            orig.a = 1;
            orig.b = "asdf";
            delete orig.a;
            let newObj = Object.assign({}, orig);
            assert.areEqual(newObj.b, orig.b);
            assert.areEqual(newObj.a, orig.a);
        }
    },
    {
        name: "has getter",
        body: function ()
        {
            let orig = {};
            orig.a = 1;
            Object.defineProperty(orig, 'b', {
                get: function() { return "asdf"; }, enumerable: true
              });
            let newObj = Object.assign({}, orig);
            assert.areEqual(newObj.b, orig.b);
            assert.areEqual(newObj.a, orig.a);
        }
    },
    {
        name: "has setter",
        body: function ()
        {
            let orig = {};
            orig.a = 1;
            Object.defineProperty(orig, 'b', {
                set: function() {  }, enumerable: true
              });
            let newObj = Object.assign({}, orig);
            assert.areEqual(newObj.b, orig.b);
            assert.areEqual(newObj.a, orig.a);
        }
    },
    {
        name: "different proto",
        body: function ()
        {
            let protoObj = {};
            let orig = Object.create(protoObj);
            orig.a = 1;
            orig.b = "asdf";
            
            let newObj = Object.assign({}, orig);
            assert.areEqual(newObj.b, orig.b);
            assert.areEqual(newObj.a, orig.a);
        }
    },
    {
        name: "non-enumerable",
        body: function ()
        {
            let orig = {};
            orig.a = 1;
            Object.defineProperty(orig, 'b', {
                value: "asdf", enumerable: false
              });
            
            let newObj = Object.assign({}, orig);
            assert.areEqual(newObj.b, undefined);
            assert.areEqual(newObj.a, orig.a);
        }
    },
    {
        name: "proto accessor",
        body: function ()
        {
            Object.defineProperty(Object.prototype, 'b', {
                get: function() { return "asdf"; }
              });
            let orig = {};
            orig.a = 1;
            
            let newObj = Object.assign({}, orig);
            assert.areEqual(newObj.b, "asdf");
            assert.areEqual(newObj.a, orig.a);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
