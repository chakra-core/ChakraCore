//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "OS4927797: TypeofElem fastpath mishandling out-of-range scenario",
    body: function () {
        assert.areEqual("undefined", typeof new Int8Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Int8Array");
        assert.areEqual("undefined", typeof new Uint8Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Uint8Array");
        assert.areEqual("undefined", typeof new Uint8ClampedArray()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Uint8ClampedArray");
        assert.areEqual("undefined", typeof new Int16Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Int16Array");
        assert.areEqual("undefined", typeof new Uint16Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Uint16Array");
        assert.areEqual("undefined", typeof new Int32Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Int32Array");
        assert.areEqual("undefined", typeof new Uint32Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Uint32Array");
        assert.areEqual("undefined", typeof new Float32Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Float32Array");
        assert.areEqual("undefined", typeof new Float64Array()[NaN & 0XF], "TypeofElem should return 'undefined' for out-of-range situation in Float64Array");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
