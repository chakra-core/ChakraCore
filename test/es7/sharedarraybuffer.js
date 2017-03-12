//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES7 SharedArrayBuffer tests

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var typedArrayList = [
    Int8Array,
    Uint8Array,
    Int16Array,
    Uint16Array,
    Int32Array,
    Uint32Array,
    Uint8ClampedArray,
    Float32Array,
    Float64Array
];

var tests = [{
		name : "SharedArrayBuffer sanity validation",
		body : function () {
			assert.doesNotThrow(() => new SharedArrayBuffer(1), "SharedArrayBuffer is valid type");
			assert.areEqual(typeof SharedArrayBuffer.prototype, "object", "SharedArrayBuffer has prototype object");

			assert.throws(() => SharedArrayBuffer(1), TypeError, "Calling SharedArrayBuffer without 'new' is an error", "SharedArrayBuffer: cannot be called without the new keyword");
			var sab = new SharedArrayBuffer(1);
			assert.areEqual(sab.byteLength, 1, "SharedArrayBuffer's object has byteLength property");
			assert.areEqual(sab.constructor, SharedArrayBuffer, "SharedArrayBuffer's object has constructor property");
			assert.areEqual(sab.slice, SharedArrayBuffer.prototype.slice, "SharedArrayBuffer's object has slice property");
			assert.areEqual(typeof sab.slice, "function", "slice is function");
			assert.areEqual(sab.slice.length, 2, "SharedArrayBuffer.prototype.slice.length == 2");
			assert.areEqual(String(sab), "[object SharedArrayBuffer]", "Calling toString on SharedArrayBuffer's object will return '[object SharedArrayBuffer]'");

			assert.doesNotThrow(() => new SharedArrayBuffer(), "Calling SharedArrayBuffer without any arguments is valid syntax");
		}
	},
    {
        name: "SharedArrayBuffer.prototype.slice behavior with non-SharedArrayBuffer parameters",
        body: function() {
            assert.throws(function () { SharedArrayBuffer.prototype.slice.apply('string'); }, TypeError, "SharedArrayBuffer.prototype.slice throws TypeError if this parameter is not an SharedArrayBuffer", "SharedArrayBuffer object expected");
            assert.throws(function () { SharedArrayBuffer.prototype.slice.apply(); }, TypeError, "SharedArrayBuffer.prototype.slice throws TypeError if there is no this parameter", "SharedArrayBuffer object expected");
            assert.throws(function () { SharedArrayBuffer.prototype.slice.call(); }, TypeError, "SharedArrayBuffer.prototype.slice throws TypeError if it is called directly", "SharedArrayBuffer object expected");
            assert.throws(function () { SharedArrayBuffer.prototype.slice.call(undefined); }, TypeError, "SharedArrayBuffer.prototype.slice throws TypeError if it is called directly", "SharedArrayBuffer object expected");
            assert.throws(function () { SharedArrayBuffer.prototype.slice(); }, TypeError, "SharedArrayBuffer.prototype.slice throws TypeError if it is called directly", "SharedArrayBuffer object expected");
            assert.throws(function () { SharedArrayBuffer.prototype.slice(undefined); }, TypeError, "SharedArrayBuffer.prototype.slice throws TypeError if it is called directly", "SharedArrayBuffer object expected");
        }
    },
    {
		name : "Validating SharedArrayBuffer with different size",
		body : function () {
			function validate(size, expectedSize) {
				var sab = new SharedArrayBuffer(size);
				assert.areEqual(sab.byteLength, expectedSize, "byteLength should be " + expectedSize);
			}
			validate(0, 0);
			validate(1024, 1024);
			validate(false, 0);
			validate(undefined, 0);
			validate(NaN, 0);
			validate("16", 16);
			validate("hello", 0);
			validate(1.1, 1);
			validate({ valueOf : () => 10 }, 10);
			validate({ toString : () => 10 }, 10);

			assert.doesNotThrow(() => new SharedArrayBuffer(1, 2, 3), "Calling SharedArrayBuffer with more than one arguments is a valid syntax");
			assert.throws(() => new SharedArrayBuffer(-1), RangeError, "Calling SharedArrayBuffer with negative size is an error", "Array length must be a finite positive integer");
		}
	},
    {
		name : "Validating SharedArrayBuffer's byteLength does not change",
		body : function () {
			var sab = new SharedArrayBuffer(1);
			sab.byteLength = 12;
			assert.areEqual(sab.byteLength, 1, "Explicit change to byteLength does not modify the byteLength");
		}
	},
    {
		name : "TypedArray on SharedArrayBuffer",
		body : function () {
            function validate(constructor, elements, range) {
                var sab = new SharedArrayBuffer(elements*constructor.BYTES_PER_ELEMENT);
                    var view = new constructor(sab);
                    assert.areEqual(view.length, elements, constructor.name + " with length " + elements);
                    assert.areEqual(view.buffer, sab, constructor.name + " with buffer");
                    assert.areEqual(view.byteOffset, 0, constructor.name + " with byteOffset " + view.byteOffset);
                    assert.areEqual(view.byteLength, elements*constructor.BYTES_PER_ELEMENT, constructor.name + " with byteLength " + elements*constructor.BYTES_PER_ELEMENT);
                
                for ([offset, len] of range) {
                    var v = new constructor(sab, offset*constructor.BYTES_PER_ELEMENT, len);
                    assert.areEqual(v.length, len, constructor.name + " with length " + len);
                    assert.areEqual(v.buffer, sab, constructor.name + " with buffer");
                    assert.areEqual(v.byteOffset, offset*constructor.BYTES_PER_ELEMENT, constructor.name + " with byteOffset " + view.byteOffset);
                    assert.areEqual(v.byteLength, len * constructor.BYTES_PER_ELEMENT, constructor.name + " with byteLength " + elements*constructor.BYTES_PER_ELEMENT);
                    if (len > 0) {
                        v[0] = 10;
                        assert.areEqual(v[0], view[offset], constructor.name);
                    }
                }
            };
            for (var i of typedArrayList) {
                validate(i, 8, [[0, 0], [0, 1], [6, 0], [6, 2], [0, 8]]);
            }
            
            function validateThrows(constructor, elements, range) {
                var sab = new SharedArrayBuffer(elements*constructor.BYTES_PER_ELEMENT);
                for ([offset, len] of range) {
                    assert.throws(() => new constructor(sab, offset*constructor.BYTES_PER_ELEMENT, len), RangeError);
                }
            }

            for (var i of typedArrayList) {
                validateThrows(i, 8, [[-1, 0], [20, 1], [6, -1], [6, 5]]);
            }
		}
	},
    {
		name : "TypedArray indices validation on SharedArrayBuffer",
		body : function () {
            function validate(constructor, elements, data) {
                var sab = new SharedArrayBuffer(elements*constructor.BYTES_PER_ELEMENT);
                var view = new constructor(sab);
                for ([index, test, expected] of data) {
                    view[index] = test;
                    assert.areEqual(view[index], expected, constructor.name);
                }
            };
            validate(Int8Array, 8, [[0, 10, 10], [7, 0x7F, 0x7F], [7, 0x80, -128], [1, 1000, -24]]);
            validate(Uint8Array, 8, [[0, 10, 10], [7, 0xFF, 0xFF], [7, -20, 236], [1, 1000, 232]]);
            validate(Int16Array, 8, [[0, 10, 10], [7, 0x7FFF, 0x7FFF], [7, 0x8000, -0x8000], [1, (0xFFFF + 1) *2 + 23, 23]]);
            validate(Uint16Array, 8, [[0, 10, 10], [7, 0xFFFF, 0xFFFF], [7, -120, 0x10000 - 120], [1, (0xFFFF + 1) *2 + 23, 23]]);
            validate(Int32Array, 8, [[0, 10, 10], [7, 0x7FFFFFFF, 0x7FFFFFFF], [7, 0x80000000, -0x80000000], [1, (0xFFFFFFFF + 1) *2 + 23, 23]]);
            validate(Uint32Array, 8, [[0, 10, 10], [7, 0xFFFFFFFF, 0xFFFFFFFF], [7, -20, 0xFFFFFFFF - 20 + 1], [1, (0xFFFFFFFF + 1) *2 + 23, 23]]);
            
            for (var i of typedArrayList) {
                validate(i, 8, [[-1, 20, undefined], ["-1", 20, undefined], [NaN, 20, undefined]]);
            }
		}
	},
    {
		name : "DataView functionality on SharedArrayBuffer",
		body : function () {
            
            function validate(dataView, setFunc, getFunc, offset, value) {
                dataView[setFunc](offset, value);
                assert.areEqual(value, dataView[getFunc](offset), "DataView "+ setFunc + " and " + getFunc + " validation");
            }

            [['setInt8', 'getInt8', 0x10], ['setUint8', 'getUint8', 0x9F], ['setInt16', 'getInt16', 0x6FFF],
            ['setUint16', 'getUint16', 0x9FFF], ['setInt32', 'getInt32', 0x6FFFFFFF], ['setUint32', 'getUint32', 0x9FFFFFFF],
            ['setFloat32', 'getFloat32', 2], ['setFloat64', 'getFloat64', 4]].forEach(function([setFunc, getFunc, v]) {
                [[0, 8], [8, 8]].forEach(function([byteOffset, byteLength]) {
                    var sab = new SharedArrayBuffer(16);
                    var dv = new DataView(sab, byteOffset, byteLength);
                    validate(dv, setFunc, getFunc, 0, v);
                });
            });
		}
	},
];

testRunner.runTests(tests, {
	verbose : WScript.Arguments[0] != "summary"
});
