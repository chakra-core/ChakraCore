//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [
  {
    name: "Bug Issue 238 - lastIndex can be made not writable",
    body: function () {
        const re = /./;
        re.lastIndex = 2;
        let desc = Object.getOwnPropertyDescriptor(re, "lastIndex");
        assert.areEqual(true, desc.writable, "lastIndex property should be created writable");
        re.lastIndex = 5;
        assert.areEqual(5, re.lastIndex, "writing to lastIndex should succeed");
        Object.defineProperty(re, "lastIndex", {writable : false});
        desc = Object.getOwnPropertyDescriptor(re, "lastIndex");
        assert.areEqual(false, desc.writable, "making lastIndex not writable should succeed");
        assert.areEqual(5, re.lastIndex);
    
        assert.throws(()=>{"use strict"; re.lastIndex = 10}, TypeError, "TypeError for writing to non-writeable property in strict mode");
        assert.doesNotThrow(()=>{re.lastIndex = 10}, "No error for writing to non-writable property in sloppy mode");
        assert.areEqual(5, re.lastIndex, "Writing to non-writable lastIndex property should not succeed");
        assert.throws(()=>{ Object.defineProperty(re, "lastIndex", {writable : true})}, TypeError, "Attempting to revert writable state of lastIndex should throw");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
