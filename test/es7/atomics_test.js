//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ESNext SharedArrayBuffer tests

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var atomicsFunctionsList = [
    {op : Atomics.add, fullname: "Atomics.add", length : 3},
    {op : Atomics.and, fullname: "Atomics.and",  length : 3},
    {op : Atomics.compareExchange, fullname: "Atomics.compareExchange", length : 4},
    {op : Atomics.exchange, fullname: "Atomics.exchange", length : 3},
    {op : Atomics.load, fullname: "Atomics.load", length : 2},
    {op : Atomics.or, fullname: "Atomics.or", length : 3},
    {op : Atomics.store, fullname: "Atomics.store", length : 3},
    {op : Atomics.sub, fullname: "Atomics.sub", length : 3},
    {op : Atomics.xor, fullname: "Atomics.xor", length : 3},
];

var atomicsFunctionsList2 = [
    {op : Atomics.wait, fullname: "Atomics.wait", onlyInt32:true, length : 4},
    {op : Atomics.wake, fullname: "Atomics.wake", onlyInt32:true, length : 3},
];

var allAtomicsFunctionsList = [...atomicsFunctionsList, ...atomicsFunctionsList2, {op : Atomics.isLockFree, fullname: "Atomics.isLockFree", length : 1}];

var IntViews = [
    {ctor : Int8Array},
    {ctor : Int16Array},
    {ctor : Int32Array},
    {ctor : Uint8Array},
    {ctor : Uint16Array},
    {ctor : Uint32Array},
];

var tests = [{
		name : "Atomics's operation sanity validation",
		body : function () {
			assert.throws(() => Atomics(), TypeError, "Atomics cannot be called", "Function expected");
			assert.throws(() => new Atomics(), TypeError, "Atomics cannot be called with new", "Object doesn't support this action");
            for (var {op, length} of allAtomicsFunctionsList) {
                assert.areEqual(typeof op, "function", "Atomics has " + op.name + " as a function");
                assert.areEqual(op.length, length, "Validating " + op.name + ".length");
                assert.throws(() => new op(), TypeError, op.name +" cannot be called with new", "Function is not a constructor");
            }
		}
	}, {
		name : "Atomics's operation allowed on SharedArrayBuffer only",
		body : function () {
            var nonInt32Views = [new Int8Array(new SharedArrayBuffer(8)),
                    new Uint8Array(new SharedArrayBuffer(8)),
                    new Int16Array(new SharedArrayBuffer(8)),
                    new Uint16Array(new SharedArrayBuffer(8)),
                    new Uint32Array(new SharedArrayBuffer(8))];

            for (var {op, fullname} of allAtomicsFunctionsList) {
                assert.throws(() => op(), RangeError, op.name +" requires parameter", fullname+": function called with too few arguments");
            }
            for (var {op, fullname, onlyInt32} of [...atomicsFunctionsList, ...atomicsFunctionsList2]) {
                [undefined, null, 1, {}, [], new Array()].forEach(function(o) {
                    assert.throws(() => op(o, 0, 0, 0), TypeError, op.name +" is not allowed on this object",
                                        "Atomics function called with invalid typed array object");
                });
                    if (onlyInt32)
                    {
                        assert.throws(() => op(new Int8Array(1), 0, 0, 0), TypeError, op.name +" is not allowed on Int8Array of ArrayBuffer",
                                        "The operation is not supported on this typed array type");
                        for (var j of nonInt32Views) {
                            assert.throws(() => op(new Int8Array(1), 0, 0, 0), TypeError, op.name +" is not allowed",
                                            "The operation is not supported on this typed array type");
                        }
                    }
                    else
                    {
                        assert.throws(() => op(new Int8Array(1), 0, 0, 0), TypeError, op.name +" is not allowed on Int8Array of ArrayBuffer",
                                        "SharedArrayBuffer object expected");
                    }
                    assert.throws(() => op(new Float32Array(1), 0, 0, 0), TypeError, op.name +" is not allowed on Float32Array of ArrayBuffer",
                                        "The operation is not supported on this typed array type");
                    assert.throws(() => op(new Float32Array(new SharedArrayBuffer(8)), 0, 0, 0), TypeError, op.name +" is not allowed on Float32Array",
                                        "The operation is not supported on this typed array type");
                    assert.throws(() => op(new Float64Array(new SharedArrayBuffer(8)), 0, 0, 0), TypeError, op.name +" is not allowed on Float64Array",
                                        "The operation is not supported on this typed array type");
            }
		}
	},
    {
		name : "Atomics.add/and/exchange/or/store/sub/xor negative scenario",
		body : function () {
            atomicsFunctionsList.forEach(function(atomicFunction) {
                if (atomicFunction.length != 3) {
                    return;
                }
                    
                IntViews.forEach(function(item) {
                    [[0, 4, 4], [3, 2, 8]].forEach(function([offset, length, elements]) {
                        var sab = new SharedArrayBuffer(item.ctor.BYTES_PER_ELEMENT * elements);
                        var view = new item.ctor(sab, offset * item.ctor.BYTES_PER_ELEMENT, length);
                        
                        assert.throws(() => atomicFunction.op(view), RangeError, "Calling "+atomicFunction.fullname+" with 1 param only is not valid", atomicFunction.fullname+": function called with too few arguments");
                        assert.throws(() => atomicFunction.op(view, 0), RangeError, "Calling "+atomicFunction.fullname+" with 2 params only is not valid", atomicFunction.fullname+": function called with too few arguments");
                        [undefined, 1.1, "hi", NaN, {}].forEach(function (index) {
                            atomicFunction.op(view, index, 1);
                        });
                        
                        [-1, Infinity, -Infinity].forEach(function (index) {
                           assert.throws(() => atomicFunction.op(view, index, 1), RangeError, "Only positive interger allowed, not " + index, "Access index is out of range");
                        });
                        
                        assert.throws(() => atomicFunction.op(view, elements, 1), RangeError, "index is out of bound " + elements, "Access index is out of range");
                        assert.throws(() => atomicFunction.op(view, length, 1), RangeError, "index is out of bound " + length, "Access index is out of range");
                        
                        Object.defineProperty(view, 'length', {
                            get : function () {
                                return elements + 10;
                            }
                        });
                        assert.throws(() => atomicFunction.op(view, elements + 1, 1), RangeError, "Changing 'length' property does not affect the index", "Access index is out of range");

                    });
                });
            });
		}
	},
    {
		name : "Atomics.compareExchange negative scenario",
		body : function () {
            IntViews.forEach(function(item) {
                [[0, 4, 4], [3, 2, 8]].forEach(function([offset, length, elements]) {
                    var sab = new SharedArrayBuffer(item.ctor.BYTES_PER_ELEMENT * elements);
                    var view = new item.ctor(sab, offset * item.ctor.BYTES_PER_ELEMENT, length);
                    
                    assert.throws(() => Atomics.compareExchange(view), RangeError, "Calling Atomics.compareExchange with 1 param only is not valid", "Atomics.compareExchange: function called with too few arguments");
                    assert.throws(() => Atomics.compareExchange(view, 0), RangeError, "Calling Atomics.compareExchange with 2 params only is not valid", "Atomics.compareExchange: function called with too few arguments");
                    assert.throws(() => Atomics.compareExchange(view, 0, 0), RangeError, "Calling Atomics.compareExchange with 3 params only is not valid", "Atomics.compareExchange: function called with too few arguments");
                    [undefined, 1.1, "hi", NaN, {}].forEach(function (index) {
                        Atomics.compareExchange(view, index, 0, 0);
                    });
                    
                    [-1, Infinity, -Infinity].forEach(function (index) {
                        assert.throws(() => Atomics.compareExchange(view, index, 0, 0), RangeError, "Only positive interger allowed not, " + index, "Access index is out of range");
                    });
                    
                    assert.throws(() => Atomics.compareExchange(view, elements, 0, 0), RangeError, "index is out of bound " + elements, "Access index is out of range");
                    assert.throws(() => Atomics.compareExchange(view, length, 0, 0), RangeError, "index is out of bound " + length, "Access index is out of range");
                });
            });
		}
	},
    {
		name : "Atomics.load negative scenario",
		body : function () {
            IntViews.forEach(function(item) {
                [[0, 4, 4], [3, 2, 8]].forEach(function([offset, length, elements]) {
                    var sab = new SharedArrayBuffer(item.ctor.BYTES_PER_ELEMENT * elements);
                    var view = new item.ctor(sab, offset * item.ctor.BYTES_PER_ELEMENT, length);
                    
                    assert.throws(() => Atomics.load(view), RangeError, "Calling Atomics.load with 1 param only is not valid", "Atomics.load: function called with too few arguments");
                    [undefined, 1.1, "hi", NaN, {}].forEach(function (index) {
                        Atomics.load(view, index);
                    });
                    
                    [-1, Infinity, -Infinity].forEach(function (index) {
                        assert.throws(() => Atomics.load(view, index), RangeError, "Only positive interger allowed, not " + index, "Access index is out of range");
                    });
                    
                    assert.throws(() => Atomics.load(view, elements), RangeError, "index is out of bound " + elements, "Access index is out of range");
                    assert.throws(() => Atomics.load(view, length), RangeError, "index is out of bound " + length, "Access index is out of range");
                });
            });
		}
	},
    {
		name : "Atomics.wait negative scenario",
		body : function () {
            [[0, 4, 4], [3, 2, 8]].forEach(function([offset, length, elements]) {
                var sab = new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT * elements);
                var view = new Int32Array(sab, offset * Int32Array.BYTES_PER_ELEMENT, length);
                
                assert.throws(() => Atomics.wait(view), RangeError, "Calling Atomics.wait with 1 param only is not valid", "Atomics.wait: function called with too few arguments");
                assert.throws(() => Atomics.wait(view, 0), RangeError, "Calling Atomics.wait with 1 param only is not valid", "Atomics.wait: function called with too few arguments");
                [undefined, 1.1, "hi", NaN, {}].forEach(function (index) {
                    Atomics.wait(view, index, 1);
                });
                
                [-1, Infinity, -Infinity].forEach(function (index) {
                    assert.throws(() => Atomics.wait(view, index, 0), RangeError, "Only positive interger allowed, not " + index, "Access index is out of range");
                });
                
                assert.throws(() => Atomics.wait(view, elements, 0), RangeError, "index is out of bound " + elements, "Access index is out of range");
                assert.throws(() => Atomics.wait(view, length, 0), RangeError, "index is out of bound " + length, "Access index is out of range");
            });
		}
	},
    {
		name : "Atomics.wake negative scenario",
		body : function () {
            [[0, 4, 4], [3, 2, 8]].forEach(function([offset, length, elements]) {
                var sab = new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT * elements);
                var view = new Int32Array(sab, offset * Int32Array.BYTES_PER_ELEMENT, length);
                
                assert.throws(() => Atomics.wake(view), RangeError, "Calling Atomics.wake with 1 param only is not valid", "Atomics.wake: function called with too few arguments");
                [undefined, 1.1, "hi", NaN, {}].forEach(function (index) {
                    Atomics.wake(view, index, 1);
                });
                
                [-1, Infinity, -Infinity].forEach(function (index) {
                    assert.throws(() => Atomics.wake(view, index, 1), RangeError, "Only positive interger allowed, not " + index, "Access index is out of range");
                });
                
                assert.throws(() => Atomics.wake(view, elements, 1), RangeError, "index is out of bound " + elements, "Access index is out of range");
                assert.throws(() => Atomics.wake(view, length, 1), RangeError, "index is out of bound " + length, "Access index is out of range");
            });
		}
	},
    {
		name : "Atomics.add/and/exchange/or/store/sub/xor functionality",
		body : function () {
            function ValidateAtomicOpFunctionality(ctor, atomicsOp, data) {
                [[0, 8, 8], [6, 2, 8]].forEach(function([offset, length, elements]) {
                    var sab = new SharedArrayBuffer(elements * ctor.BYTES_PER_ELEMENT);
                    var view = new ctor(sab, offset * ctor.BYTES_PER_ELEMENT, length);
                    
                    for (var {initValue, opValue, expectedReturn, expectedValue} of data) {
                        if (initValue !== undefined) {
                            view[1] = initValue;
                        }

                        var ret = atomicsOp(view, 1, opValue);
                        assert.areEqual(ret, expectedReturn, "return value validation " + ctor.name + " " + atomicsOp.name);
                        assert.areEqual(view[1], expectedValue, "final value validation " + ctor.name + " " + atomicsOp.name);
                        assert.areEqual(Atomics.load(view, 1), expectedValue, "final value validation " + ctor.name + " " + atomicsOp.name);
                        assert.areEqual(view[0], 0, "view[0] should not be affected");
                    }
                });
            }

            ValidateAtomicOpFunctionality(Int8Array, Atomics.add, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: undefined, opValue : 0x7B, expectedReturn : 6, expectedValue : -127},
                    {initValue: undefined, opValue : 10, expectedReturn : -127, expectedValue : -117},
                    {initValue: 0, opValue : 0x104, expectedReturn : 0, expectedValue : 4}] );

            ValidateAtomicOpFunctionality(Uint8Array, Atomics.add, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: undefined, opValue : 0xFB, expectedReturn : 6, expectedValue : 1},
                    {initValue: undefined, opValue : 0x80, expectedReturn : 1, expectedValue : 0x81},
                    {initValue: 0, opValue : 0x104, expectedReturn : 0, expectedValue : 4}] );

            ValidateAtomicOpFunctionality(Int16Array, Atomics.add, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: undefined, opValue : 0x7FFB, expectedReturn : 6, expectedValue : -32767},
                    {initValue: undefined, opValue : 20, expectedReturn : -32767, expectedValue : -32747},
                    {initValue: 0, opValue : 0x10004, expectedReturn : 0, expectedValue : 4}] );

            ValidateAtomicOpFunctionality(Uint16Array, Atomics.add, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: undefined, opValue : 0xFFFB, expectedReturn : 6, expectedValue : 1},
                    {initValue: undefined, opValue : 0x8000, expectedReturn : 1, expectedValue : 0x8001},
                    {initValue: 0, opValue : 0x10004, expectedReturn : 0, expectedValue : 4}] );

            ValidateAtomicOpFunctionality(Int32Array, Atomics.add, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: undefined, opValue : 0x7FFFFFFB, expectedReturn : 6, expectedValue : -0x7FFFFFFF},
                    {initValue: undefined, opValue : 0x7FFFFFFF, expectedReturn : -0x7FFFFFFF, expectedValue : 0},
                    {initValue: 0, opValue : 0x100000004, expectedReturn : 0, expectedValue : 4}] );
                    
            ValidateAtomicOpFunctionality(Uint32Array, Atomics.add, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: undefined, opValue : 0xFFFFFFFB, expectedReturn : 6, expectedValue : 1},
                    {initValue: undefined, opValue : 0x7FFFFFFF, expectedReturn : 1, expectedValue : 0x80000000},
                    {initValue: 0, opValue : 0x100000004, expectedReturn : 0, expectedValue : 4}] );

            // Atomics.and tests
            ValidateAtomicOpFunctionality(Int8Array, Atomics.and, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 0}, 
                    {initValue: 0x5C, opValue : 0x1B, expectedReturn : 0x5C, expectedValue : 0x18},
                    {initValue: undefined, opValue : 0x108, expectedReturn : 0x18, expectedValue : 8},
                    {initValue: 0x7F, opValue : 0x80, expectedReturn : 0x7F, expectedValue : 0}] );

            ValidateAtomicOpFunctionality(Uint8Array, Atomics.and, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 0}, 
                    {initValue: 0x5C, opValue : 0x1B, expectedReturn : 0x5C, expectedValue : 0x18},
                    {initValue: undefined, opValue : 0x108, expectedReturn : 0x18, expectedValue : 8},
                    {initValue: 0xFF, opValue : 0x101, expectedReturn : 0xFF, expectedValue : 1}] );

            ValidateAtomicOpFunctionality(Int16Array, Atomics.and, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 0}, 
                    {initValue: 0x7FFB, opValue : 0x1006, expectedReturn : 0x7FFB, expectedValue : 0x1002},
                    {initValue: undefined, opValue : 0x10008, expectedReturn : 0x1002, expectedValue : 0},
                    {initValue: 0x7FFF, opValue : 0x8004, expectedReturn : 0x7FFF, expectedValue : 4}] );

            ValidateAtomicOpFunctionality(Uint16Array, Atomics.and, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 0}, 
                    {initValue: 0xBFFC, opValue : 0xA00B, expectedReturn : 0xBFFC, expectedValue : 0xA008},
                    {initValue: undefined, opValue : 0x10009, expectedReturn : 0xA008, expectedValue : 8},
                    {initValue: 0xFFFF, opValue : 0x10004, expectedReturn : 0xFFFF, expectedValue : 4}] );

            ValidateAtomicOpFunctionality(Int32Array, Atomics.and, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 0}, 
                    {initValue: 0x5FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : 0x5FFFFFFC, expectedValue : 0x1FFFFFF8},
                    {initValue: undefined, opValue : 0x8, expectedReturn : 0x1FFFFFF8, expectedValue : 8},
                    {initValue: 0x7FFFFFFF, opValue : 0x100000004, expectedReturn : 0x7FFFFFFF, expectedValue : 4}] );
                    
            ValidateAtomicOpFunctionality(Uint32Array, Atomics.and, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 0}, 
                    {initValue: 0x5FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : 0x5FFFFFFC, expectedValue : 0x1FFFFFF8},
                    {initValue: undefined, opValue : 0x8, expectedReturn : 0x1FFFFFF8, expectedValue : 8},
                    {initValue: 0x9FFFFFFF, opValue : 0x100000004, expectedReturn : 0x9FFFFFFF, expectedValue : 4}] );

            // Atomics.exchange tests
            ValidateAtomicOpFunctionality(Int8Array, Atomics.exchange, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 4}, 
                    {initValue: 0x9C, opValue : 0x1B, expectedReturn : -100, expectedValue : 0x1B},
                    {initValue: undefined, opValue : 0x108, expectedReturn : 0x1B, expectedValue : 8}]);


            ValidateAtomicOpFunctionality(Uint8Array, Atomics.exchange, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 4}, 
                    {initValue: 0x9C, opValue : 0x1B, expectedReturn : 0x9C, expectedValue : 0x1B},
                    {initValue: undefined, opValue : 0x108, expectedReturn : 0x1B, expectedValue : 8}]);

            ValidateAtomicOpFunctionality(Int16Array, Atomics.exchange, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 4}, 
                    {initValue: 0x9FFB, opValue : 0x1006, expectedReturn : -24581, expectedValue : 0x1006},
                    {initValue: undefined, opValue : 0x10008, expectedReturn : 0x1006, expectedValue : 8}]);

            ValidateAtomicOpFunctionality(Uint16Array, Atomics.exchange, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 4}, 
                    {initValue: 0x9FFB, opValue : 0x1006, expectedReturn : 0x9FFB, expectedValue : 0x1006},
                    {initValue: undefined, opValue : 0x10008, expectedReturn : 0x1006, expectedValue : 8}]);

            ValidateAtomicOpFunctionality(Int32Array, Atomics.exchange, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 4}, 
                    {initValue: 0x9FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : -1610612740, expectedValue : -1073741829},
                    {initValue: undefined, opValue : 8, expectedReturn : -1073741829, expectedValue : 8}] );
                    
            ValidateAtomicOpFunctionality(Uint32Array, Atomics.exchange, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 4}, 
                    {initValue: 0x5FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : 0x5FFFFFFC, expectedValue : 0xBFFFFFFB},
                    {initValue: undefined, opValue : 0x100000004, expectedReturn : 0xBFFFFFFB, expectedValue : 4}] );
                    
            // Atomics.or tests
            ValidateAtomicOpFunctionality(Int8Array, Atomics.or, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: 0x5C, opValue : 0x1F, expectedReturn : 0x5C, expectedValue : 0x5F},
                    {initValue: undefined, opValue : 0x120, expectedReturn : 0x5F, expectedValue : 0x7F}] );

            ValidateAtomicOpFunctionality(Uint8Array, Atomics.or, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: 0x5C, opValue : 0x8F, expectedReturn : 0x5C, expectedValue : 0xDF},
                    {initValue: undefined, opValue : 0x120, expectedReturn : 0xDF, expectedValue : 0xFF}] );

            ValidateAtomicOpFunctionality(Int16Array, Atomics.or, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: 0x700B, opValue : 0x0F00, expectedReturn : 0x700B, expectedValue : 0x7F0B},
                    {initValue: undefined, opValue : 0x100F6, expectedReturn : 0x7F0B, expectedValue : 0x7FFF}] );

            ValidateAtomicOpFunctionality(Uint16Array, Atomics.or, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: 0xBFFC, opValue : 0xA00B, expectedReturn : 0xBFFC, expectedValue : 0xBFFF},
                    {initValue: 0x10, opValue : 0x10009, expectedReturn : 0x10, expectedValue : 0x19}] );

            ValidateAtomicOpFunctionality(Int32Array, Atomics.or, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: 0x5FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : 0x5FFFFFFC, expectedValue : -1},
                    {initValue: 0x100000004, opValue : 0x80, expectedReturn : 0x4, expectedValue : 0x84}] );
                    
            ValidateAtomicOpFunctionality(Uint32Array, Atomics.or, [{initValue: 2, opValue : 4, expectedReturn : 2, expectedValue : 6}, 
                    {initValue: 0x5FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : 0x5FFFFFFC, expectedValue : 0xFFFFFFFF},
                    {initValue: 0x100000004, opValue : 0x80, expectedReturn : 0x4, expectedValue : 0x84}] );

            // Atomics.store tests
            ValidateAtomicOpFunctionality(Int8Array, Atomics.store, [{initValue: 0, opValue : 30, expectedReturn : 30, expectedValue : 30}, 
                    {initValue: undefined, opValue : 0xFF, expectedReturn : 0xFF, expectedValue : -1}] );

            ValidateAtomicOpFunctionality(Uint8Array, Atomics.store, [{initValue: 0, opValue : 30, expectedReturn : 30, expectedValue : 30}, 
                    {initValue: undefined, opValue : 0x1FF, expectedReturn : 0x1FF, expectedValue : 0xFF}] );
                    
            ValidateAtomicOpFunctionality(Int16Array, Atomics.store, [{initValue: 0, opValue : 30, expectedReturn : 30, expectedValue : 30}, 
                    {initValue: undefined, opValue : 0xFFFF, expectedReturn : 0xFFFF, expectedValue : -1}] );
                    
            ValidateAtomicOpFunctionality(Uint16Array, Atomics.store, [{initValue: 0, opValue : 30, expectedReturn : 30, expectedValue : 30}, 
                    {initValue: undefined, opValue : 0x1FFFF, expectedReturn : 0x1FFFF, expectedValue : 0xFFFF}] );
                    
            ValidateAtomicOpFunctionality(Int32Array, Atomics.store, [{initValue: 0, opValue : 30, expectedReturn : 30, expectedValue : 30}, 
                    {initValue: undefined, opValue : 0xFFFFFFFF, expectedReturn : 0xFFFFFFFF, expectedValue : -1}] );
                    
            ValidateAtomicOpFunctionality(Uint32Array, Atomics.store, [{initValue: 0, opValue : 30, expectedReturn : 30, expectedValue : 30}, 
                    {initValue: undefined, opValue : 0x1FFFFFFFF, expectedReturn : 0x1FFFFFFFF, expectedValue : 0xFFFFFFFF}] );
             
            // Atomics.sub tests
            ValidateAtomicOpFunctionality(Int8Array, Atomics.sub, [{initValue: 12, opValue : 4, expectedReturn : 12, expectedValue : 8}, 
                    {initValue: 50, opValue : 75, expectedReturn : 50, expectedValue : -25}, 
                    {initValue: 0x9C, opValue : 0x1B, expectedReturn : -100, expectedValue : -127},
                    {initValue: 0x7F, opValue : 0x80, expectedReturn : 0x7F, expectedValue : -1}] );

            ValidateAtomicOpFunctionality(Uint8Array, Atomics.sub, [{initValue: 12, opValue : 4, expectedReturn : 12, expectedValue : 8}, 
                    {initValue: 50, opValue : 75, expectedReturn : 50, expectedValue : 231},
                    {initValue: 0x9C, opValue : 0x1B, expectedReturn : 0x9C, expectedValue : 129},
                    {initValue: 0x7F, opValue : 0x80, expectedReturn : 0x7F, expectedValue : 255}] );

            ValidateAtomicOpFunctionality(Int16Array, Atomics.sub, [{initValue: 12, opValue : 4, expectedReturn : 12, expectedValue : 8}, 
                    {initValue: 50, opValue : 75, expectedReturn : 50, expectedValue : -25},
                    {initValue: 0x7FFF, opValue : 0x8004, expectedReturn : 0x7FFF, expectedValue : -5}] );

            ValidateAtomicOpFunctionality(Uint16Array, Atomics.sub, [{initValue: 12, opValue : 4, expectedReturn : 12, expectedValue : 8}, 
                    {initValue: 50, opValue : 75, expectedReturn : 50, expectedValue : 65511},
                    {initValue: 0x7FFF, opValue : 0x8004, expectedReturn : 0x7FFF, expectedValue : 65531}] );

            ValidateAtomicOpFunctionality(Int32Array, Atomics.sub, [{initValue: 12, opValue : 4, expectedReturn : 12, expectedValue : 8}, 
                    {initValue: 50, opValue : 75, expectedReturn : 50, expectedValue : -25},
                    {initValue: 0x7FFFFFFF, opValue : 0x100000004, expectedReturn : 0x7FFFFFFF, expectedValue : 2147483643}] );
                    
            ValidateAtomicOpFunctionality(Uint32Array, Atomics.sub, [{initValue: 12, opValue : 4, expectedReturn : 12, expectedValue : 8}, 
                    {initValue: 50, opValue : 75, expectedReturn : 50, expectedValue : 0xFFFFFFE7},
                    {initValue: 0x9FFFFFFF, opValue : 5, expectedReturn : 0x9FFFFFFF, expectedValue : 0x9FFFFFFA}] );   
                    
            // Atomics.xor tests
            ValidateAtomicOpFunctionality(Int8Array, Atomics.xor, [{initValue: 10, opValue : 20, expectedReturn : 10, expectedValue : 30}, 
                    {initValue: 0x5C, opValue : 0x8F, expectedReturn : 0x5C, expectedValue : -45}] );

            ValidateAtomicOpFunctionality(Uint8Array, Atomics.xor, [{initValue: 10, opValue : 20, expectedReturn : 10, expectedValue : 30}, 
                    {initValue: 0x5C, opValue : 0x8F, expectedReturn : 0x5C, expectedValue : 0xD3}] );

            ValidateAtomicOpFunctionality(Int16Array, Atomics.xor, [{initValue: 10, opValue : 20, expectedReturn : 10, expectedValue : 30}, 
                    {initValue: 0x700B, opValue : 0x8F00, expectedReturn : 0x700B, expectedValue : -245}] );

            ValidateAtomicOpFunctionality(Uint16Array, Atomics.xor, [{initValue: 10, opValue : 20, expectedReturn : 10, expectedValue : 30}, 
                    {initValue: 0x700B, opValue : 0x8F00, expectedReturn : 0x700B, expectedValue : 0xFF0B}] );

            ValidateAtomicOpFunctionality(Int32Array, Atomics.xor, [{initValue: 10, opValue : 20, expectedReturn : 10, expectedValue : 30}, 
                    {initValue: 0x5FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : 0x5FFFFFFC, expectedValue : -536870905}] );
                    
            ValidateAtomicOpFunctionality(Uint32Array, Atomics.xor, [{initValue: 10, opValue : 20, expectedReturn : 10, expectedValue : 30}, 
                    {initValue: 0x5FFFFFFC, opValue : 0xBFFFFFFB, expectedReturn : 0x5FFFFFFC, expectedValue : 0xE0000007}] );
                    
        }
	},
    {
		name : "Atomics.load functionality",
		body : function () {
            [[0, 4, 4], [3, 2, 8]].forEach(function([offset, length, elements]) {
                IntViews.forEach(function ({ctor}) {
                    var sab = new SharedArrayBuffer(ctor.BYTES_PER_ELEMENT * elements);
                    var view = new ctor(sab, offset * ctor.BYTES_PER_ELEMENT, length);
                    view[1] = 20;
                    var ret = Atomics.load(view, 1);
                    assert.areEqual(ret, 20, "value validation " + ctor.name);
                    assert.areEqual(view[0], 0, "view[0] should not be affected");
                });
            });
		}
	},
    {
		name : "Atomics.compareExchange functionality",
		body : function () {
            IntViews.forEach(function({ctor}) {
                [[0, 8, 8], [6, 2, 8]].forEach(function([offset, length, elements]) {
                    var sab = new SharedArrayBuffer(elements * ctor.BYTES_PER_ELEMENT);
                    var view = new ctor(sab, offset * ctor.BYTES_PER_ELEMENT, length);
                    view[1] = 10;
                    var ret = Atomics.compareExchange(view, 1, 10, 20);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], 20, "value validation " + ctor.name);

                    var ret = Atomics.compareExchange(view, 1, 5, 30);
                    assert.areEqual(ret, 20, "compared failed -  return value validation " + ctor.name);
                    assert.areEqual(view[1], 20, "compared failed - value validation " + ctor.name);
                });                
            });
		}
	},
    {
		name : "Atomics operations ToInteger verification",
		body : function () {
            var valueOf = {
				valueOf : () => 1
			};
			var toString = {
				toString : () => 1
			};

            IntViews.forEach(function({ctor}) {
                [undefined, NaN, null, Number.POSITIVE_INFINITY, true, "1", "blah", 1.1, valueOf, toString].forEach(function(value) {
                    var sab = new SharedArrayBuffer(2 * ctor.BYTES_PER_ELEMENT);
                    var view = new ctor(sab);
                    var num = value | 0;

                    view[1] = 10;
                    var ret = Atomics.add(view, 1, value);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], 10 + num, "final value validation " + ctor.name);

                    view[1] = 10;
                    var ret = Atomics.and(view, 1, value);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], 10 & num, "final value validation " + ctor.name);

                    view[1] = 10;
                    var ret = Atomics.compareExchange(view, 1, 10, value);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], num, "final value validation " + ctor.name);

                    view[1] = 10;
                    var ret = Atomics.exchange(view, 1, value);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], num, "final value validation " + ctor.name);

                    view[1] = 10;
                    var ret = Atomics.or(view, 1, value);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], 10 | num, "final value validation " + ctor.name);

                    view[1] = 10;
                    var ret = Atomics.sub(view, 1, value);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], 10 - num, "final value validation " + ctor.name);

                    view[1] = 10;
                    var ret = Atomics.store(view, 1, value);
                    if (value !== Number.POSITIVE_INFINITY)
                    {
                        assert.areEqual(ret, num, "return value validation " + ctor.name);
                    }
                    assert.areEqual(view[1], num, "final value validation " + ctor.name);

                    view[1] = 10;
                    var ret = Atomics.xor(view, 1, value);
                    assert.areEqual(ret, 10, "return value validation " + ctor.name);
                    assert.areEqual(view[1], 10 ^ num, "final value validation " + ctor.name);
                    
                });
            });
		}
	},
    {
		name : "Atomics.add buffer/length invalidation",
		body : function () {
            var sab = new SharedArrayBuffer(16);
            var view = new Int8Array(sab, 0, 8);
            
            var lengthChange = {
                valueOf : function() {
                    view.length = 0;
                    return 2;
                }
            };
            
            var lengthChange1 = {
                valueOf : function() {
                    Object.defineProperty(view, 'length', {
                        get : function () {
                            return 0;
                        }
                    });
                    return 2;
                }
            }

            var bufferChange = {
                valueOf : function() {
                    view.buffer = null;
                    return 2;
                }
            }
            
            view[1] = 0;
            Atomics.add(view, 1, lengthChange);
            assert.areEqual(view[1], 2, "Changing view's length does not affect the 'add'");

            view[1] = 0;
            Atomics.add(view, 1, lengthChange1);
            assert.areEqual(view[1], 2, "Changing view's length by defineProperty does not affect the 'add'");

            view[1] = 0;
            Atomics.add(view, 1, bufferChange);
            assert.areEqual(view[1], 2, "Changing the buffer does not affect the 'add'");
		}
	},
    {
		name : "Atomics ops on subclassed shared array buffer",
		body : function () {
            class MySAB extends SharedArrayBuffer {
            }
            var sab = new MySAB(16);
            assert.isTrue(sab instanceof MySAB, "object is instance of subclass of SharedArrayBuffer");
            assert.isTrue(sab instanceof SharedArrayBuffer, "object is instance of SharedArrayBuffer");
            var view = new Int8Array(sab, 0, 8);
            view[1] = 10;
            var ret = Atomics.add(view, 1, 20);
            assert.areEqual(ret, 10, "return value validation ");
            assert.areEqual(Atomics.load(view, 1), 30, "final value validation");
		}
	},
    {
		name : "Atomics.wait index test",
		body : function () {
            var view = new Int32Array(new SharedArrayBuffer(8));
            assert.doesNotThrow(() => Atomics.wait(view, 0, 0, 0), "Atomics.wait is allowed on the index 0, where the view's length is 2");
            assert.doesNotThrow(() => Atomics.wait(view, "0", 0, 0), "ToNumber : Atomics.wait is allowed on the index 0, where the view's length is 2");
            assert.throws(() => Atomics.wait(view, 2, 0, 0), RangeError, "Index is greater than the view's length", "Access index is out of range");
            assert.throws(() => Atomics.wait(view, -1, 0, 0), RangeError, "Negative index is not allowed", "Access index is out of range");
		}
	},
    {
		name : "Atomics.wait value match",
		body : function () {
            var view = new Int32Array(new SharedArrayBuffer(8));
            [2, "2", {valueOf : () => 2}].forEach(function(value) {
                view[0] = 1;
                var ret = Atomics.wait(view, 0, value, 0);
                assert.areEqual(ret, "not-equal", "value is not matching with 1 and 'wait' will return as 'not-equal'");
            });
            
            [10, "10", {valueOf : () => 10}].forEach(function(value) {
                view[0] = 10;
                var ret = Atomics.wait(view, 0, value, 5);
                assert.areEqual(ret, "timed-out", "value should match with 10 and 'wait' will wait till time-out");
            });
            
            view[0] = 10;
            var ret = Atomics.wait(view, 0, 10, Number.NEGATIVE_INFINITY);
            assert.areEqual(ret, "timed-out", "Negative infinity will be treated as 0 and so this will time-out");
		}
	},
    {
		name : "Atomics on virtual typedarray",
		body : function () {
            [2**15, 2**20, 2**25].forEach(function(size) {
                IntViews.forEach(function(item) {
                    var view = new item.ctor(new SharedArrayBuffer(size));
                    Atomics.add(view, 0, 10);
                    Atomics.store(view, 2**10, 20);
                    assert.areEqual(Atomics.load(view, 0), 10);
                    assert.areEqual(Atomics.load(view, 2**10), 20);
                });
            });
		}
	},
];

testRunner.runTests(tests, {
	verbose : WScript.Arguments[0] != "summary"
});
