//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Note: see function  ArraySpliceHelper of JavascriptArray.cpp

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var restorePropertyFromDescriptor = function (obj, prop, desc) {
    if (typeof desc == 'undefined') {
        delete obj[prop];
    } else {
        Object.defineProperty(obj, prop, desc);
    }
}

var tests = [
    {
        name: "OS7342663:OOB writes using type confusion in InternalCopyArrayElements",
        body: function ()
        {
            function test() {
                var arr1 = [0xdead, 0xbabe, 0xdead, 0xbabe];

                class MyArray extends Uint32Array { }
                Object.defineProperty(MyArray, Symbol.species, { value: function() { return arr1; } });

                var float_val = 0xdaddeadbabe * 4.9406564584124654E-324;
                var test = [float_val, float_val, float_val, float_val];
                test.length = 0x1000;
                test.__proto__ = new MyArray(0);

                var res = Array.prototype.slice.apply(test, []);  // OOB write
                assert.areEqual(0x1000, res.length, "res.length == 0x1000");
                assert.areEqual(float_val, res[0], "res[0] == float_val");
                assert.areEqual(float_val, res[1], "res[1] == float_val");
                assert.areEqual(float_val, res[2], "res[2] == float_val");
                assert.areEqual(float_val, res[3], "res[3] == float_val");
                assert.areEqual(undefined, res[4], "res[4] == float_val");
                assert.areEqual(undefined, res[0xfff], "res[0xfff] == undefined");
            }
            test();
            test();
            test();
        }
    },
    {
        name: "OS7342689:OOB writes using type confusion in InternalFillFromPrototypes",
        body: function ()
        {
            function test() {
                var arr1 = [0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead,
                0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe,
                0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead,
                0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe];

                class MyArray extends Uint32Array { }
                Object.defineProperty(MyArray, Symbol.species, { value: function() { return arr1; } });

                var float_val = 0xdaddeadbabe * 4.9406564584124654E-324;
                var test = [{}];
                delete test[0];
                test.length = 0x1000;
                var src = [float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val,
                float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val,
                float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val,
                float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val,
                float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val, float_val];
                test.__proto__ = src;
                test.__proto__.__proto__ = new MyArray(0);

               //this will write 0xfffc0daddeadbabe to [arr1] + 0x1D8
                var res = Array.prototype.slice.apply(test, [])
                assert.areEqual(0x1000, res.length, "res.length == 0x1000");
                assert.areEqual(float_val, res[0], "res[0] == float_val");
                assert.areEqual(float_val, res[1], "res[1] == float_val");
                assert.areEqual(float_val, res[2], "res[2] == float_val");
                assert.areEqual(float_val, res[src.length-1], "res[src.length-1] == float_val");
                assert.areEqual(undefined, res[src.length], "res[src] == undefined");
                assert.areEqual(undefined, res[0xfff], "res[0xfff] == undefined");
            }
            test();
            test();
            test();
        }
    },
    {
        name: "OS7307908:type confusion in Array.prototype.slice",
        body: function ()
        {
            function test() {
                var arr = [1, 2]
                var desc = Object.getOwnPropertyDescriptor(arr.constructor, Symbol.species);

               //Our species function will get called during chakra!Js::JavascriptArray::SliceHelper<unsigned int>
                Object.defineProperty(
                    arr.constructor,
                    Symbol.species,
                    {
                        value : function()
                        {
                           //change 'arr' from TypeIds_NativeIntArray to TypeIds_Array
                            arr[0] = WScript;

                           //return a TypeIds_NativeIntArray so we can read back out the 64 bit pointer as two 32bit ints.
                            return [];
                        }
                    }
                );

               //trigger the bug and retrieve a TypeIds_NativeIntArray array containing a pointer.
                var brr = arr.slice();

                assert.areEqual(2, brr.length, "brr.length == 2");
                assert.areEqual(WScript, brr[0], "brr[0] == WScript");
                assert.areEqual(2, brr[1], "brr[0] == WScript");

                restorePropertyFromDescriptor(arr.constructor, Symbol.species, desc);
            }
            test();
            test();
            test();
        }
    },
    {
        name: "OS7342791:type confusion in Array.from",
        body: function ()
        {
            function test() {
                var arr1 = [0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe, 0xdead, 0xbabe];

                var float_val = 0xdaddeadbabe * 4.9406564584124654E-324;
                var test = [float_val, float_val, float_val, float_val];
                delete test[0];
                delete test[1];
                delete test[2];

                var res = Array.from.apply(function(){return arr1}, [test]);
                assert.areEqual(4, res.length, "res.length == 4");
                assert.areEqual(undefined, res[0], "res[0] == undefined");
                assert.areEqual(undefined, res[1], "res[1] == undefined");
                assert.areEqual(undefined, res[2], "res[2] == undefined");
                assert.areEqual(float_val, res[3], "res[3] == float_val");

                assert.areEqual(['1','2','3'], Array.from.apply(()=>new Array(), ["123"]), "Array.from on iterable");
                assert.areEqual([1,2,3], Array.from.apply(()=>new Array(), [{"0":1, "1":2, "2":3, "length":3}]), "Array.from on non-iterable");
            }
            test();
            test();
            test();
        }
    },
    {
        name: "OS7342844:type confusion in Array.of",
        body: function ()
        {
            function test() {
                var brr = Array.of.call(()=>[ 1, 2, 3, 4 ],
                    WScript, // supply 2 copies of target so the brr array will have a length of 2 and we can read the 64bit pointer.
                    WScript
                );

                assert.areEqual(2, brr.length, "brr.length == 2");
                assert.areEqual(WScript, brr[0], "res[0] == WScript");
                assert.areEqual(WScript, brr[1], "res[1] == WScript");
                assert.areEqual(undefined, brr[2], "res[2] == undefined");
                assert.areEqual(undefined, brr[3], "res[3] == undefined");
            }
            test();
            test();
            test();
        }
    },
    {
        name: "OS7342907:type confusion in Array.prototype.map",
        body: function ()
        {
            function test() {
                var arr = [ 1, 2 ];
                var desc = Object.getOwnPropertyDescriptor(arr.constructor, Symbol.species);

                Object.defineProperty(
                    arr.constructor,
                    Symbol.species,
                    {
                        value : function()
                        {
                            return [];
                        }
                    }
                );

               //The value returned from our callback is directly set into the array whose type we create via the species.
                var brr = arr.map( function( v )
                    {
                        if( v == 1 )
                            return WScript;
                    }
                );

                assert.areEqual(2, brr.length, "brr.length == 2");
                assert.areEqual(WScript, brr[0], "brr[0] == WScript");
                assert.areEqual(undefined, brr[1], "brr[1] == undefined");

                restorePropertyFromDescriptor(arr.constructor, Symbol.species, desc);
            }
            test();
            test();
            test();
        }
    },
    {
        name: "type confusion in Array.prototype.map with Proxy",
        body: function ()
        {
            function test() {
                var d = [1,2,3];
                class dummy {
                    constructor() {
                        return d;
                    }
                }

                var handler = {
                    get: function(target, name) {
                        if(name == "length") {
                            return 0x100;
                        }

                        return {[Symbol.species] : dummy};
                    },

                    has: function(target, name) {
                        return true;
                    }
                };

                var p = new Proxy([], handler);
                var a = new Array(1,2,3);

                function test(){
                    return 0x777777777777;
                }

                var o = a.map.call(p, test);
                assert.areEqual(Array(0x100).fill(0x777777777777), o);
            }
            test();
            test();
            test();
        }
    },
    {
        name: "OS7342965:type confusion in Array.prototype.splice",
        body: function ()
        {
            function test() {
               //create a TypeIds_Array holding two 64 bit values (The same amount of space for four 32 bit values).
                var arr = [ WScript, WScript ];
                var desc = Object.getOwnPropertyDescriptor(arr.constructor, Symbol.species);

               //Our species function will get called during chakra!Js::JavascriptArray::EntrySplice
                Object.defineProperty(
                    arr.constructor,
                    Symbol.species,
                    {
                        value : function()
                        {
                           //return a TypeIds_NativeIntArray so we can read back out a 64 bit pointer as two 32bit ints.
                            return [ 1, 2, 3, 4 ];
                        }
                    }
                );

               //trigger the bug and retrieve a TypeIds_NativeIntArray array containing a pointer. The helper
               //method ArraySegmentSpliceHelper<Var> will directly copy over the TypeIds_Array segment data
               //into the TypeIds_NativeIntArray segment.
                var brr = arr.splice( 0, 2 );

                assert.areEqual(2, brr.length, "brr.length == 2");
                assert.areEqual(WScript, brr[0], "brr[0] == WScript");
                assert.areEqual(WScript, brr[1], "brr[1] == WScript");
                assert.areEqual(undefined, brr[2], "brr[2] == undefined");
                assert.areEqual(undefined, brr[3], "brr[3] == undefined");

                restorePropertyFromDescriptor(arr.constructor, Symbol.species, desc);
            }
            test();
            test();
            test();
        }
    },
    {
        name: "type confusion in Array.prototype.join",
        body: function ()
        {
            function test() {
                var a = [0, 1, 2, 3];
                var b = [];
                delete a[0];
                Object.setPrototypeOf(a, b)
                Object.defineProperty(b, "0",
                    {
                        get: function() {
                                a[2] = "abc";
                                return -1;
                            }
                    });

                assert.areEqual("-1,1,abc,3", a.join());
            }
            test();
            test();
            test();
        }
    },
    {
        name: "type confusion in Array.prototype.indexOf",
        body: function ()
        {
            function test() {
                var float_val = 0xdaddeadbabe * 4.9406564584124654E-324;
                var a = [0, 1, 2, 3];
                var b = [];
                delete a[1];
                Object.setPrototypeOf(a, b);
                Object.defineProperty(b, "1",
                    {
                        get: function() {
                                a[2] = float_val; //"abc";
                                return -1;
                            }
                    });

                assert.areEqual(3, a.indexOf(3));
            }
            test();
            test();
            test();
        }
    },
    {
        name: "type confusion in Array.prototype.lastIndexOf",
        body: function ()
        {
            function test() {
                var float_val = 0xdaddeadbabe * 4.9406564584124654E-324;
                var a = [3, 2, 1, 0];
                var b = [];
                delete a[3];
                Object.setPrototypeOf(a, b);
                Object.defineProperty(b, "3",
                    {
                        get: function() {
                                a[1] = float_val; //"abc";
                                return -1;
                            }
                    });

                assert.areEqual(0, a.lastIndexOf(3));
            }
            test();
            test();
            test();
        }
    },
    {
        name: "type confusion in Function.prototype.apply",
        body: function ()
        {
            function test() {
                var t = [1,2,3];

                function f(){
                    var h = [];
                    var a = [...arguments]

                    for(item in a){
                        var n = new Number(a[item]);

                        if( n < 0) {
                            n = n + 0x100000000;
                        }

                        h.push(n.toString(16));
                    }

                    return h;
                }

                var q = f;

                t.length = 20;
                var o = {};
                Object.defineProperty(o, '3', {
                    get: function() {
                        var ta = [];
                        ta.fill.call(t, "natalie");
                        return 5;
                    }
                });

                t.__proto__ = o;

                var j = [];
                assert.areEqual("1,2,3,5,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN,NaN", f.apply(null, t).toString());
            }
            test();
            test();
            test();
        }
    },
    {
        name: "[MSRC34910] type confusion in Array.prototype.filter",
        body: function ()
        {
            function mappingFn(elem, index, arr) {
                arr[1] = 'hello';
                return true;
            }

            var arr = [1, 2, 3];

            var desc = Object.getOwnPropertyDescriptor(arr.constructor, Symbol.species);
            Object.defineProperty(arr.constructor, Symbol.species, { get : function () {  return function() { return [22, 33]; } } } );
            var b = Array.prototype.filter.call(arr, mappingFn);
            assert.areEqual('hello', b[1]);

            restorePropertyFromDescriptor(arr.constructor, Symbol.species, desc);
        }
    },
    {
        name: "[MSRC35046] heap overflow in Array.prototype.splice",
        body: function ()
        {
            var a = [];
            var o = {};

            Object.defineProperty(o, 'constructor', {
                get: function() {
                    a.length = 0xfffffffe;
                    [].fill.call(a, 0, 0xfffff000, 0xfffffffe);
                    return Array;
                }});

            a.__proto__ = o;
            var q = new Array(50).fill(1.1);

            var b = [].splice.call(a, 0, 0, ...q);
            assert.areEqual(50, a.length);
            assert.areEqual(q, a);
        }
    },
    {
        name: "[MSRC35086] type confusion in FillFromPrototypes",
        body: function ()
        {
            var a = new Array(0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x12121212);

            var handler = {
                    getPrototypeOf: function(target, name) {
                        return a;
                    }
                };

            var p = new Proxy([], handler);
            var b = [{}, [], "abc"];

            b.__proto__ = p;
            b.length = 4;
            var c = [[],"abc",1145324612];

            a.shift.call(b);
            assert.areEqual(3, b.length);
            assert.areEqual([], b[0]);
            assert.areEqual("abc", b[1]);
            assert.areEqual(undefined, b[2]);
        }
    },
    {
        name: "[MSRC35272] type confusion in JSON.parse",
        body: function ()
        {
            var a = 1;
            var once = false;
            function f(){
                if(!once){
                    a = new Array(2)
                    this[2] = a;
                }
                once = true;
                return 0x41414141;
            }

            var r = JSON.parse("[1111, 22222, 333333]", f);
            assert.areEqual(0x41414141, r);
        }
    },
    {
        name: "[MSRC35383] type confusion in Array.prototype.concat",
        body: function ()
        {
            var n = [];
            for (var i = 0; i < 0x10; i++)
                n.push([0x11111111, 0x11111111, 0, 0x11111111,0x11111111, 0x11111111, 0, 0x11111111,0x11111111, 0x11111111, 0, 0x11111111,0x11111111,0x11111111, 0, 0x11111111,0x11111111 ,1 ,2 ,3 ,4]);

            class fake extends Object {
                static get [Symbol.species]() { return function() {  return n[3]; }; };
            }

            var f = function(a){ return a; }

            var x = ["dabao", 0, 0, 0x41414141];
            var y = new Proxy(x, {
                get: function(t, p, r) {
                    return (p == "constructor") ? fake : x[p];
                }
            });

            assert.areEqual(x, Array.prototype.concat.apply(y));
        }
    },
    {
        name: "[MSRC35389] type confusion in Array.prototype.splice",
        body: function ()
        {
            var arr = [0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344]
            class fake extends Object {
                static get [Symbol.species]() { return function() {
                    return arr;
                }; };
            }

            var x = [0, 2, 0, 0x41414141];
            var y = new Proxy(x, {
                get: function(t, p, r) {
                    return (p == "constructor") ? fake : x[p];
                }
            });

            Array.prototype.splice.apply(y);
            assert.areEqual(x, y);
        }
    },
    {
        name: "[MSRC35389 variation] type confusion in Array.prototype.slice",
        body: function ()
        {
            var arr = [0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344,0x41424344];

            class fake extends Object {
                static get [Symbol.species]() { return function() {
                    return arr;
                }; };
            }

            var x = [0, 2, 0, 0x41414141];
            var y = new Proxy(x, {
                get: function(t, p, r) {
                    if (p == "constructor")
                        return fake
                    if (p == 'length');
                        return 1;
                    return x[p];
                },
                has: function() {
                    return false;
                }
            });

            assert.areEqual([0x41424344], Array.prototype.slice.call(y));
        }
    },
    {
        name: "[MSRC34994,35226] heap overflow in Array.prototype.reverse",
        body: function ()
        {
            var count = 0;
            arr = new Array(100);
            var desc = Object.getOwnPropertyDescriptor(Array.prototype, 1);
            Object.defineProperty(Array.prototype, 1, { get: function () {
                    count++;
                    if (count == 1) {
                        arr.push(null);
                    }
                }});

            arr.reverse();
            restorePropertyFromDescriptor(Array.prototype, 1, desc);
            assert.areEqual(101, arr.length);
        }
    },
    {
        name: "Heap overread when splice mutates the array when executing slice",
        body: function ()
        {
            var getterCalled = false;
            var a = [1, 2];
            for (var i = 0; i < 100 * 1024; i++) {
                a.push(i);
            }
            delete a[0]; // Make a missing item
            var protoObj = [11];
            Object.defineProperty(protoObj, '0', {
                get : function () {
                    getterCalled = true;
                    Object.setPrototypeOf(a, Array.prototype);
                    a.splice(0); // head seg is now length=0
                    return 42;
                },
                configurable : true
            });
            Object.setPrototypeOf(a, protoObj);
            var b = a.slice();
            assert.isTrue(getterCalled);
            assert.areEqual(0, a.length, "Getter will splice the array to zero length");
            assert.areEqual(100 * 1024 + 2, b.length, "Validating that slice will return the full array even though splice is deleting the whole array");
        }
    },
    {
        name: "reverse : Mutating the array's length from prototype should not be reversed",
        body: function ()
        {
            var getterCalled = false;
            var a = [11, 22, 33];
            a.length = 5;
            var o = {};
            Object.defineProperty(o, '4' , {
                get: function () {
                    getterCalled = true;
                    a[5] = 55;
                    a[6] = 66;
                    a.length = 8; // Changing the length of the array while we are in the reverse call
                    return 44 ;
                }, set : function(ab) {}, configurable : true
            });
            
            a.__proto__ = o;
            var r = [].reverse.call(a);
            a.__proto__ = Array.prototype;
            assert.isTrue(getterCalled);
            assert.areEqual(55, a[5], 'a[5] is added during the reverse call, so it should not be reversed');
            assert.areEqual(66, a[6], 'a[6] is added during the reverse call, so it should not be reversed');
            assert.areEqual(undefined, a[7], 'a[7] is undefined the reverse call, so it should remain undefined');
        }
    },
    {
        name: "reverse : Making current array an ES5Array from prototype should be part of reverse",
        body: function ()
        {
            var getterCalled = false;
            var a = [0, 1, 2, 3, 4];
            a.length = 6;
            var o = {};
            Object.defineProperty(o, '5' , {
                get: function () {
                    Object.defineProperty(a, '1', {
                        get : function() { getterCalled = true; return 11;},
                        set : function(ab) { }, configurable : true });
                    return 51 ;
                }, set : function(ab) { }, configurable : true
            });
            a.__proto__ = o;

            [].reverse.call(a);
            a.__proto__ = Array.prototype;
            assert.isTrue(getterCalled);
            assert.areEqual([51,11,3,2,11,], a, 'getter on a[1] is called when introduced during prototype walk on reverse call');
        }
    },
    {
        name: "reverse : Proxy object in the prototype chain",
        body: function ()
        {
            var getTrapCalled = false;
            var arr = [11, 22, 33];
            arr.length = 4;
            var handler = {
                has : function() {
                    return true;
                },
                get : function(target, name) {
                    if (name == "3") {
                        getTrapCalled = true;
                        arr[4] = 55;
                        arr[5] = 66;
                        arr.length = 6;
                        return 44;
                    }
                }
            };
            var p = new Proxy({}, handler);
            arr.__proto__ = p;
            [].reverse.call(arr);
            arr.__proto__ = Array.prototype;
            assert.isTrue(getTrapCalled);
            assert.areEqual([44,33,22,11,55,66], arr, 'Properties added in get trap should not part of the reverse logic (55 and 66 are remained on same position)');
        }
    },
    {
        name: "shift : Mutating the array's length from prototype should not be part of shift",
        body: function ()
        {
            var getterCalled = false;
            var a = [11, 22, 33];
            a.length = 5;
            var o = {};
            
            Object.defineProperty(o, '4' , {
                get: function () {
                    getterCalled = true;
                    a[5] = 55;
                    a[6] = 66;
                    a.length = 8;
                    return 44;
                }, set : function(ab) {}, configurable : true
            });
            a.__proto__ = o;
            var r = [].shift.call(a);
            a.__proto__ = Array.prototype;
            assert.isTrue(getterCalled);
            assert.areEqual([22,33,,44], a, 'a[5] and a[6] is not part of the shift');
            assert.areEqual(4, a.length, 'We started with length == 5 and shift will decrement by 1');
        }
    },
    {
        name: "unshift : Mutating the array's length from prototype should not be part of unshift",
        body: function ()
        {
            var getterCalled = false;
            var arr = [11, 22, 33];
            var obj = {};
            arr.length = 4;
            Object.defineProperty(obj, "3", {get : function() { 
                getterCalled = true;
                arr[4] = 66;
                arr[5] = 77;
                return 55;
                }, set : function(bb) {}, configurable: true});
            arr.__proto__ = obj;
            var obj1 = [].unshift.call(arr, 201, 202);
            arr.__proto__ = Array.prototype;
            
            assert.isTrue(getterCalled);
            assert.areEqual([201,202,11,,33,55], arr, '66 and 77 were added after length deduced so they are not part of unshift');
            assert.areEqual(6, arr.length, 'we begin length == 4 and unshift adds 2 more');
        }
    },
    {
        name: "sort : Mutating the array's length from prototype should not be part of actual sort",
        body: function ()
        {
            var getterCalled = false;
            var arr = [33, 11, 22];
            var obj = {};
            arr.length = 4;
            Object.defineProperty(obj, "3", {get : function() { 
                getterCalled = true;
                arr[4] = 77;
                arr[5] = 16;
                return 101;
            }, set : function(bb) {}, configurable: true});
            arr.__proto__ = obj;
            var obj1 = [].sort.call(arr);
            arr.__proto__ = Array.prototype;

            assert.isTrue(getterCalled);
            assert.areEqual([101,11,22,,77,16], arr, '77 and 16 are not part of the sort so they are not sorted');
            assert.areEqual(6, arr.length);
        }
    },
    {
        name: "unshift : setter on the prototype will be called even though it is outside of current array's length",
        body: function ()
        {
            var setterCalled = false;
            var protoObj = {};
            var arr = [1, 2];
            // setter is put on the index outside of the current's array length (which is 2).
            Object.defineProperty(protoObj, 3, {get : function() {  }, set : function(a) { setterCalled = true;} });
            arr.__proto__ = protoObj;
            Array.prototype.unshift.call(arr, 101, 104);
            assert.isTrue(setterCalled);
        }
    },
    {
        name: "slice : the method slice should get property from prototype which is a proxy",
        body: function ()
        {
            var arr = [];
            arr.length = 100;
            arr.__proto__ = new Proxy([16], {} );

            arr.push(1);
            var ret = arr.slice(0, 10);
            assert.areEqual(16, ret[0]);
        }
    },
    {
        name: "splice : the method splice should get property from prototype which is a proxy",
        body: function ()
        {
            var v1 = [];
            v1.length = 20;
            var hasCalled = 0;
            v1.__proto__ = new Proxy([222], {has : function() { hasCalled++;} });
            v1.push(1);
            assert.areEqual(222, v1[0]);
            var ret  = v1.splice(0, 10);
            assert.areEqual(20, hasCalled);
        }
    },
];
testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
