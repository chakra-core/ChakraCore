//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "With different types on right side",
        body: function () {
            assert.isFalse(1n === false);
            assert.isFalse(1n === 1);
            assert.isFalse(1n === "1");
            assert.isFalse(1n === "1.0");            
            assert.isFalse(1n === 1.0);        
            assert.isFalse(1n === {a: 1});
            assert.isFalse(1n === null);
            assert.isFalse(1n === undefined);
            assert.isFalse(1n === []);
            assert.isFalse(1n === Symbol());            
        }
    },
    {
        name: "With different types on left side",
        body: function () {
            assert.isFalse(true === 1n);
            assert.isFalse(1 === 1n);
            assert.isFalse("1" === 1n);
            assert.isFalse("1.0" === 1n);            
            assert.isFalse(1.0 === 1n);        
            assert.isFalse({a: 1} === 1n);
            assert.isFalse(null === 1n);
            assert.isFalse(undefined === 1n);
            assert.isFalse([] === 1n);
            assert.isFalse(Symbol() === 1n);            
        }
    },
    {
        name: "With same type BigInt",
        body: function () {
            assert.isTrue(0n === -0n);
            assert.isTrue(1n === 1n);
            assert.isTrue(1n === BigInt(1n));
            assert.isTrue(BigInt(2n) === BigInt(2n));
            assert.isTrue(BigInt(3n) === 3n);
            assert.isTrue(1n === 2n-1n);
            var x = 2n;
            var y = 2n;
            assert.isTrue(x === y);
            assert.isTrue(--x === 1n);
            assert.isTrue(y === 2n);
            assert.isFalse(x === y);
        }
    },
    {
        name: "Cross scope",
        body: function () {
            var y = 1n;
            var f = () => {
                return y;
            };
            var g = () => {
                return 1n;
            };
            var x = 1n;
            assert.isTrue(x === f());
            assert.isTrue(x === g());
            var o = {a: 1n};
            assert.isTrue(x == o.a);
        }
    },    
    {
        name: "Marshal",
        body: function () {
            let child_global = WScript.LoadScript('var val = 5n;', 'samethread');
            assert.isTrue(5n === child_global.val);
        }
    },    
    {
        name: "Module",
        body: function () {
            WScript.RegisterModuleSource("exporter.js", "export var val = 5n;");
            WScript.RegisterModuleSource("importer.js", 
            `   
                import { val } from 'exporter.js';
                assert.isTrue(5n === val);
            `);
            WScript.LoadScriptFile("importer.js", "module");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
