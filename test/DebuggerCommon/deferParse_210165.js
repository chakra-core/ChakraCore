//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Configuration: full.xml
//Testcase Number: 1315
//Bailout Testing: ON
//Switches:  -targeted -dbgbaseline   -maxinterpretcount:1  -args attach 
//Baseline Switches:  -nonative -debuglaunch  -targeted -dbgbaseline 
//Branch:  fbl_ie_script_dev
//Build: 121217-1126
//Arch: AMD64
//MachineName: IEVM-1895F89FA2
var shouldBailout = false;
var iCount = 0;
var reuseObjects = false;
var PolymorphicFuncObjArr = [];
var PolyFuncArr = [];

function GetPolymorphicFunction() {
    if (PolyFuncArr.length > 1) {
        var myFunc = PolyFuncArr.shift();
        PolyFuncArr.push(myFunc);
        return myFunc;
    } else {
        return PolyFuncArr[0];
    }
}

function GetObjectwithPolymorphicFunction() {
    /*
    if (reuseObjects) {
        if (PolymorphicFuncObjArr.length > 1) {
            var myFunc = PolymorphicFuncObjArr.shift();
            PolymorphicFuncObjArr.push(myFunc);
            return myFunc
        } else {
            return PolymorphicFuncObjArr[0];
        }
    } else {*/
        var obj = {};
        obj.polyfunc = PolyFuncArr[0];
        //PolymorphicFuncObjArr.push(obj)
        return obj
    //}
};

function InitPolymorphicFunctionArray() {
    for (var i = 0; i < arguments.length; i++) {
        PolyFuncArr.push(arguments[i])
    }
};

function test0() {
    /*
    function makeArrayLength(x) {
        if (x < 1 || x > 4294967295 || x != x || isNaN(x) || !isFinite(x)) return 100;
        else return Math.floor(x) & 0xffff;
    };;

    function leaf() {
        return 100;
    };
    var obj0 = {};
    var obj1 = {};
    var arrObj0 = {};
    var litObj0 = {
        prop1: 3.14159265358979
    };
    var litObj1 = {
        prop0: 0,
        prop1: 1
    };
    var arrObj0 = {};*/
    var func0 = function (argArr0, argObj1) {/*
        if (((arrObj0.prop0 * (-149 !== ui16[(argArr0[(((((shouldBailout ? (argArr0[(((argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)]) >= 0 ? (argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)]) : 0) & 0xF)] = "x") : undefined), argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)]) >= 0 ? argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)] : 0)) & 0XF)]) & 255]) + leaf.call(argObj1)) != (ui16[(argArr0[(((((shouldBailout ? (argArr0[(((argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)]) >= 0 ? (argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)]) : 0) & 0xF)] = "x") : undefined), argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)]) >= 0 ? argArr0[((((1 * 1 + -1024566845) >= 0 ? (1 * 1 + -1024566845) : 0)) & 0XF)] : 0)) & 0XF)]) & 255] && (shouldBailout ? (d = {
            valueOf: function () { WScript.Echo('d valueOf');
                return 3;
            }
        }, 1) : 1)))) {
            f = 1;
        } else {
            argObj1.length = -3;
            ary0 = arguments;
            b >>>= ary0[((shouldBailout ? (ary0[1] = "x") : undefined), 1)];
            c = (!(+0 instanceof((typeof Boolean == 'function') ? Boolean : Object)));
        }*/
        //var __loopvar3 = 0;
        /*
        while (-910020387.9) {
            if (__loopvar3 > 3) break;
            __loopvar3++;
            WScript.Echo("e = " + (e|0));
            return 1;
            return obj1.prop0;
            var obj5 = obj1;
        }*/
        //arrObj0 = obj0;
        //return cpa8[((c <<= (f *= leaf()))) & 255];
    }
    /*
  var func1 = function(argFunc2){
    return (cpa8[((func0.call(obj0 , 1, obj1) < (func0.call(obj0 , 1, obj1), (shouldBailout ? (b = { valueOf: function() { WScript.Echo('b valueOf'); return 3; } }, ((-2147483648 * 3.03638661398507E+18 + 1) * ((-4 * 1 + -3) - 1))) : ((-2147483648 * 3.03638661398507E+18 + 1) * ((-4 * 1 + -3) - 1)))))) & 255] * (((this.prop0 *= (ary[((shouldBailout ? (ary[1] = "x") : undefined ), 1)] ? (i16.length * (((obj1.prop0 = 1) ? arrObj0[(((1 >= 0 ? 1 : 0)) & 0XF)] : (ary.unshift(a, b, obj0.prop0, d, obj1.prop0, h, a, arrObj0.prop0, c, arrObj0.prop1, arrObj0.length, this.prop0, this.prop0))) - (('method0' in obj0) ? arrObj0[(((((shouldBailout ? (arrObj0[(((1) >= 0 ? ( 1) : 0) & 0xF)] = "x") : undefined ), 1) >= 0 ? 1 : 0)) & 0XF)] : (-- f)))) : (arrObj0[(((((shouldBailout ? (arrObj0[(((arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) >= 0 ? ( arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) : 0) & 0xF)] = "x") : undefined ), arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) >= 0 ? arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)] : 0)) & 0XF)]))) * ({} instanceof ((typeof Function == 'function' ) ? Function : Object)) - ui32[1]) + ((d++ ) / (1 == 0 ? 1 : 1))));
  }*/
    function func2() {
        return 0;
    }
    /*
    obj0.method0 = func2;
    obj1.method0 = func2;
    arrObj0.method0 = func2;
    var ary = new Array(10);
    var i8 = new Int8Array(256);
    var i16 = new Int16Array(256);
    var i32 = new Int32Array(256);
    var ui8 = new Uint8Array(256);
    var ui16 = new Uint16Array(256);
    var ui32 = new Uint32Array(256);
    var f32 = new Float32Array(256);
    var f64 = new Float64Array(256);
    var cpa8 = WScript.CreateCanvasPixelArray(ui8);;
    var floatary = [-1.5, -0.5, -0, 1.5, 12.987, 12.123, 100.33, 8.8, 5.5, 44.66, 42.24, 124.07, -0.99, 56.65, 42.24];
    var intfloatary = [1, 4, -1, -6, -0, +0, 55, -100, 2.56, -3.14, 6.6, 42, 2.3, 67, 1.97, -24, 77.99];
    var intary = [4, 66, 767, -100, 0, 1213, 34, 42, 55, -123, 567, 77, -234, 88, 11, -66];
    var a = 1;
    var b = 1;
    var c = 1;
    var d = 1;
    var e = 1;
    var f = -2147483648;
    var g = 1;
    var h = 830575065.1;
    this.prop0 = 1;
    this.prop1 = 1;
    obj0.prop0 = -981563483;
    obj0.prop1 = -2147483648;
    obj0.length = 5.86817585348816E+18;
    obj1.prop0 = -1384132225;
    obj1.prop1 = 1;
    obj1.length = 1;
    arrObj0.prop0 = 646876129.1;
    arrObj0.prop1 = -1515486246.9;
    arrObj0.length = makeArrayLength(-1057784464);
    arrObj0[0] = -3;
    arrObj0[1] = -2;
    arrObj0[2] = 1;
    arrObj0[3] = -2;
    arrObj0[4] = -4.83037565615671E+18;
    arrObj0[5] = 1;
    arrObj0[6] = 2147483647;
    arrObj0[7] = -1;
    arrObj0[8] = 1;
    arrObj0[9] = -3;
    arrObj0[10] = 1;
    arrObj0[11] = -219;
    arrObj0[12] = 15077001;
    arrObj0[13] = 1;
    arrObj0[14] = 1;
    arrObj0[arrObj0.length - 1] = 1;
    arrObj0.length = makeArrayLength(-575127848.9);
    ary[0] = -339226426.9;
    ary[1] = 42887848;
    ary[2] = -1027139006;
    ary[3] = 1;
    ary[4] = 1;
    ary[5] = 1;
    ary[6] = 1;
    ary[7] = 1;
    ary[8] = 106;
    ary[9] = -2147483648;
    ary[10] = 1;
    ary[11] = -1038535437;
    ary[12] = 1;
    ary[13] = 1;
    ary[14] = 1;
    ary[ary.length - 1] = 469999974;
    ary.length = makeArrayLength(1);
*/
    function bar0() {
        //f = this.prop1;
        //e *= -2147051577;
        WScript.Echo(func2); /**bp:locals() **/
        //WScript.Echo(func2.call);
        /*obj1, obj1, ((cpa8[((func0.call(obj0, 1, obj1) < (func0.call(obj0, 1, obj1), (shouldBailout ? (b = {
            valueOf: function () {
                WScript.Echo('b valueOf');
                return 3;
            }
        }, ((-2147483648 * 3.03638661398507E+18 + 1) * ((-4 * 1 + -3) - 1))) : ((-2147483648 * 3.03638661398507E+18 + 1) * ((-4 * 1 + -3) - 1)))))) & 255] * (((this.prop0 *= (ary[((shouldBailout ? (ary[1] = "x") : undefined), 1)] ? (i16.length * (((obj1.prop0 = 1) ? arrObj0[(((1 >= 0 ? 1 : 0)) & 0XF)] : (ary.unshift(a, b, obj0.prop0, d, obj1.prop0, h, a, arrObj0.prop0, c, arrObj0.prop1, arrObj0.length, this.prop0, this.prop0))) - (('method0' in obj0) ? arrObj0[(((((shouldBailout ? (arrObj0[(((1) >= 0 ? (1) : 0) & 0xF)] = "x") : undefined), 1) >= 0 ? 1 : 0)) & 0XF)] : (--f)))) : (arrObj0[(((((shouldBailout ? (arrObj0[(((arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) >= 0 ? (arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) : 0) & 0xF)] = "x") : undefined), arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) >= 0 ? arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)] : 0)) & 0XF)]))) * ({}
        instanceof((typeof Function == 'function') ? Function : Object)) - ui32[1]) + ((d++) / (1 == 0 ? 1 : 1)))) === (f++))*/
        
        
    }
    InitPolymorphicFunctionArray(bar0);;
    var __polyobj = GetObjectwithPolymorphicFunction();;

    function func4() {
        eval("");
        /*
        arrObj0.method0.call(obj1, arrObj0, (!intfloatary[(((((shouldBailout ? (intfloatary[((((-101 instanceof((typeof String == 'function') ? String : Object))) >= 0 ? ((-101 instanceof((typeof String == 'function') ? String : Object))) : 0) & 0xF)] = "x") : undefined), (-101 instanceof((typeof String == 'function') ? String : Object))) >= 0 ? (-101 instanceof((typeof String == 'function') ? String : Object)) : 0)) & 0XF)]));
        f >>>= 1;
        return i16[(((intfloatary[(14)] >= ((arrObj0[(1)] == ((+-0) < (0 ? 2147483647 : ((func2.call(obj1, obj1, ((cpa8[((func0.call(obj0, 1, obj1) < (func0.call(obj0, 1, obj1), (shouldBailout ? (b = {
            valueOf: function () {
                WScript.Echo('b valueOf');
                return 3;
            }
        }, ((-2147483648 * 3.03638661398507E+18 + 1) * ((-4 * 1 + -3) - 1))) : ((-2147483648 * 3.03638661398507E+18 + 1) * ((-4 * 1 + -3) - 1)))))) & 255] * (((this.prop0 *= (ary[((shouldBailout ? (ary[1] = "x") : undefined), 1)] ? (i16.length * (((obj1.prop0 = 1) ? arrObj0[(((1 >= 0 ? 1 : 0)) & 0XF)] : (ary.unshift(a, b, obj0.prop0, d, obj1.prop0, h, a, arrObj0.prop0, c, arrObj0.prop1, arrObj0.length, this.prop0, this.prop0))) - (('method0' in obj0) ? arrObj0[(((((shouldBailout ? (arrObj0[(((1) >= 0 ? (1) : 0) & 0xF)] = "x") : undefined), 1) >= 0 ? 1 : 0)) & 0XF)] : (--f)))) : (arrObj0[(((((shouldBailout ? (arrObj0[(((arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) >= 0 ? (arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) : 0) & 0xF)] = "x") : undefined), arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)]) >= 0 ? arrObj0[(((obj0.prop0 >= 0 ? obj0.prop0 : 0)) & 0XF)] : 0)) & 0XF)]))) * ({}
        instanceof((typeof Function == 'function') ? Function : Object)) - ui32[1]) + ((d++) / (1 == 0 ? 1 : 1)))) === (f++))) instanceof((typeof RegExp == 'function') ? RegExp : Object)) * ((new Object()) instanceof((typeof Object == 'function') ? Object : Object)) - (1 ? floatary[(1)] : (shouldBailout ? (arrObj0.length = {
            valueOf: function () {
                WScript.Echo('arrObj0.length valueOf');
                return 3;
            }
        }, ((func0.call(obj1, arrObj0, arrObj0) - (++g)) * (1 !== (115 ? -2147483648 : obj0.prop0)))) : ((func0.call(obj1, arrObj0, arrObj0) - (++g)) * (1 !== (115 ? -2147483648 : obj0.prop0))))))))) | ([1, 2, 3] instanceof((typeof RegExp == 'function') ? RegExp : Object)))) ? (~ui32[2147483647]) : ui16[((shouldBailout ? (Object.defineProperty(obj0, 'length', {
            writable: false,
            enumerable: true,
            configurable: true
        }), ary[((shouldBailout ? (ary[1] = "x") : undefined), 1)]) : ary[((shouldBailout ? (ary[1] = "x") : undefined), 1)])) & 255])) & 255];*/
    }
    //var __loopvar0 = 0;
    //for (var strvar19 in ui32) 
    
        //if (strvar19.indexOf('method') != -1) continue;
        //if (__loopvar0++ > 3) break;
        /*ui32[strvar19] = */
        /*
        (floatary[(((((shouldBailout ? (floatary[(((((~(d++)) ? intfloatary[(11)] : ((((new Error("abc")) instanceof((typeof Error == 'function') ? Error : Object)) ? i8[(-2147483648) & 255] : (h = 1)) ? (obj1.prop1 == arrObj0[(((1 >= 0 ? 1 : 0)) & 0XF)]) : intary[(1)]))) >= 0 ? (((~(d++)) ? intfloatary[(11)] : ((((new Error("abc")) instanceof((typeof Error == 'function') ? Error : Object)) ? i8[(-2147483648) & 255] : (h = 1)) ? (obj1.prop1 == arrObj0[(((1 >= 0 ? 1 : 0)) & 0XF)]) : intary[(1)]))) : 0) & 0xF)] = "x") : undefined), ((~(d++)) ? intfloatary[(11)] : ((((new Error("abc")) instanceof((typeof Error == 'function') ? Error : Object)) ? i8[(-2147483648) & 255] : (h = 1)) ? (obj1.prop1 == arrObj0[(((1 >= 0 ? 1 : 0)) & 0XF)]) : intary[(1)]))) >= 0 ? ((~(d++)) ? intfloatary[(11)] : ((((new Error("abc")) instanceof((typeof Error == 'function') ? Error : Object)) ? i8[(-2147483648) & 255] : (h = 1)) ? (obj1.prop1 == arrObj0[(((1 >= 0 ? 1 : 0)) & 0XF)]) : intary[(1)])) : 0)) & 0XF)] | ((g -= (~intary[(13)])) ? (shouldBailout ? (obj1.prop1 = {
            valueOf: function () {
                WScript.Echo('obj1.prop1 valueOf');
                return 3;
            }
        }, 1) : 1) : (b++)));*/
        __polyobj.polyfunc.call();
        //obj0.method0.call(arrObj0, arrObj0, (__polyobj.polyfunc.call(arrObj0, ary)));
        //obj0 = arrObj0;
        //floatary.pop();
        //obj0.method0.call(obj0, obj0, (b++));
    
    /*
    if (shouldBailout) {
        return 'somestring'
    }*/
    //obj1.method0.call(obj1, obj0, 1);
    //var __loopvar0 = 0;
    /*
  while(((ary[(((((shouldBailout ? (ary[(((bar0.call(arrObj0 , floatary)) >= 0 ? ( bar0.call(arrObj0 , floatary)) : 0) & 0xF)] = "x") : undefined ), bar0.call(arrObj0 , floatary)) >= 0 ? bar0.call(arrObj0 , floatary) : 0)) & 0XF)] >= (floatary[(((((shouldBailout ? (floatary[((((bar0.call(arrObj0 , floatary) ? f32[((obj1.prop0 -= 1)) & 255] : ((true instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)) * func1.call(obj1 , leaf) - bar0.call(arrObj0 , floatary)))) >= 0 ? ( (bar0.call(arrObj0 , floatary) ? f32[((obj1.prop0 -= 1)) & 255] : ((true instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)) * func1.call(obj1 , leaf) - bar0.call(arrObj0 , floatary)))) : 0) & 0xF)] = "x") : undefined ), (bar0.call(arrObj0 , floatary) ? f32[((obj1.prop0 -= 1)) & 255] : ((true instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)) * func1.call(obj1 , leaf) - bar0.call(arrObj0 , floatary)))) >= 0 ? (bar0.call(arrObj0 , floatary) ? f32[((obj1.prop0 -= 1)) & 255] : ((true instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)) * func1.call(obj1 , leaf) - bar0.call(arrObj0 , floatary))) : 0)) & 0XF)] * 1 - (([1, 2, 3] instanceof ((typeof Array == 'function' ) ? Array : Object)) === __polyobj.polyfunc.call(obj0 , ary))))) && __loopvar0 < 3) {
    __loopvar0++;
    floatary.pop();
  }
  WScript.Echo("a = " + (a|0));
  WScript.Echo("b = " + (b|0));
  WScript.Echo("c = " + (c|0));
  WScript.Echo("d = " + (d|0));
  WScript.Echo("e = " + (e|0));
  WScript.Echo("f = " + (f|0));
  WScript.Echo("g = " + (g|0));
  WScript.Echo("h = " + (h|0));
  WScript.Echo("this.prop0 = " + (this.prop0|0));
  WScript.Echo("this.prop1 = " + (this.prop1|0));
  WScript.Echo("obj0.prop0 = " + (obj0.prop0|0));
  WScript.Echo("obj0.prop1 = " + (obj0.prop1|0));
  WScript.Echo("obj0.length = " + (obj0.length|0));
  WScript.Echo("obj1.prop0 = " + (obj1.prop0|0));
  WScript.Echo("obj1.prop1 = " + (obj1.prop1|0));
  WScript.Echo("obj1.length = " + (obj1.length|0));
  WScript.Echo("arrObj0.prop0 = " + (arrObj0.prop0|0));
  WScript.Echo("arrObj0.prop1 = " + (arrObj0.prop1|0));
  WScript.Echo("arrObj0.length = " + (arrObj0.length|0));
  WScript.Echo("arrObj0[0] = " + (arrObj0[0]|0));
  WScript.Echo("arrObj0[1] = " + (arrObj0[1]|0));
  WScript.Echo("arrObj0[2] = " + (arrObj0[2]|0));
  WScript.Echo("arrObj0[3] = " + (arrObj0[3]|0));
  WScript.Echo("arrObj0[4] = " + (arrObj0[4]|0));
  WScript.Echo("arrObj0[5] = " + (arrObj0[5]|0));
  WScript.Echo("arrObj0[6] = " + (arrObj0[6]|0));
  WScript.Echo("arrObj0[7] = " + (arrObj0[7]|0));
  WScript.Echo("arrObj0[8] = " + (arrObj0[8]|0));
  WScript.Echo("arrObj0[9] = " + (arrObj0[9]|0));
  WScript.Echo("arrObj0[10] = " + (arrObj0[10]|0));
  WScript.Echo("arrObj0[11] = " + (arrObj0[11]|0));
  WScript.Echo("arrObj0[12] = " + (arrObj0[12]|0));
  WScript.Echo("arrObj0[13] = " + (arrObj0[13]|0));
  WScript.Echo("arrObj0[14] = " + (arrObj0[14]|0));
  WScript.Echo("arrObj0[arrObj0.length-1] = " + (arrObj0[arrObj0.length-1]|0));
  WScript.Echo("arrObj0.length = " + (arrObj0.length|0));
  WScript.Echo("ary[0] = " + (ary[0]|0));
  WScript.Echo("ary[1] = " + (ary[1]|0));
  WScript.Echo("ary[2] = " + (ary[2]|0));
  WScript.Echo("ary[3] = " + (ary[3]|0));
  WScript.Echo("ary[4] = " + (ary[4]|0));
  WScript.Echo("ary[5] = " + (ary[5]|0));
  WScript.Echo("ary[6] = " + (ary[6]|0));
  WScript.Echo("ary[7] = " + (ary[7]|0));
  WScript.Echo("ary[8] = " + (ary[8]|0));
  WScript.Echo("ary[9] = " + (ary[9]|0));
  WScript.Echo("ary[10] = " + (ary[10]|0));
  WScript.Echo("ary[11] = " + (ary[11]|0));
  WScript.Echo("ary[12] = " + (ary[12]|0));
  WScript.Echo("ary[13] = " + (ary[13]|0));
  WScript.Echo("ary[14] = " + (ary[14]|0));
  WScript.Echo("ary[ary.length-1] = " + (ary[ary.length-1]|0));
  WScript.Echo("ary.length = " + (ary.length|0));*/
};

// generate profile
test0();
WScript.Attach(test0);
