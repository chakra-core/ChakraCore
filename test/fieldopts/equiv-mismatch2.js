//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var shouldBailout = false;
var runningJITtedCode = false;
var reuseObjects = false;
var randomGenerator = function(inputseed) {
    var seed = inputseed;
    return function() {
    // Robert Jenkins' 32 bit integer hash function.
    seed = ((seed + 0x7ed55d16) + (seed << 12))  & 0xffffffff;
    seed = ((seed ^ 0xc761c23c) ^ (seed >>> 19)) & 0xffffffff;
    seed = ((seed + 0x165667b1) + (seed << 5))   & 0xffffffff;
    seed = ((seed + 0xd3a2646c) ^ (seed << 9))   & 0xffffffff;
    seed = ((seed + 0xfd7046c5) + (seed << 3))   & 0xffffffff;
    seed = ((seed ^ 0xb55a4f09) ^ (seed >>> 16)) & 0xffffffff;
    return (seed & 0xfffffff) / 0x10000000;
    };
};;
var intArrayCreatorCount = 0;
function GenerateArray(seed, arrayType, arraySize, missingValuePercent, typeOfDeclaration) {
   Math.random = randomGenerator(seed);
   var result, codeToExecute, thisArrayName, maxMissingValues = Math.floor(arraySize * missingValuePercent), noOfMissingValuesAdded = 0;
   var contents = [];
   var isVarArray = arrayType == 'var';
   function IsMissingValue(allowedMissingValues) {
       return Math.floor(Math.random() * 100) < allowedMissingValues
   }

   thisArrayName = 'tempIntArr' + intArrayCreatorCount++;

   for (var arrayIndex = 0; arrayIndex < arraySize; arrayIndex++) {
       if (isVarArray && arrayIndex != 0) {
           arrayType = Math.floor(Math.random() * 100) < 50 ? 'int' : 'float';
       }

       if(noOfMissingValuesAdded < maxMissingValues && IsMissingValue(missingValuePercent)) {
           noOfMissingValuesAdded++;
           contents.push('');
       } else {
           var randomValueGenerated;
           if (arrayType == 'int') {
               randomValueGenerated = Math.floor(Math.random() * seed);
           } else if (arrayType == 'float') {
               randomValueGenerated = Math.random() * seed;
           } else if (arrayType == 'var') {
               randomValueGenerated = '\'' + (Math.random() * seed).toString(36) + '\'';
           }
           contents.push(randomValueGenerated);
       }
   }

   if(contents.length == 1 && typeOfDeclaration == 'constructor') {
       contents.push(Math.floor(Math.random() * seed));
   }
   if(typeOfDeclaration == 'literal') {
       codeToExecute = 'var ' + thisArrayName + ' = [' + contents.join() + '];';
   } else {
       codeToExecute = 'var ' + thisArrayName + ' = new Array(' + contents.join() + ');';
   }

   codeToExecute += 'result =  ' + thisArrayName + ';';
   eval(codeToExecute);
   return result;
}
;

function getRoundValue(n) {
  if(n % 1 == 0) // int number
    return n % 2147483647;
  else // float number
    return n.toFixed(8);
 return n;
};

var print = WScript.Echo;
WScript.Echo = function(n) { /* suppress output to avoid huge baseline match but preserve the side-effects at call sites */ };

function formatOutput(n) {{
 return n.replace(/[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?/g, function(match) {{return getRoundValue(parseFloat(match));}} );
}};
function sumOfArrayElements(prev, curr, index, array) {
    return (typeof prev == "number" && typeof curr == "number") ? curr + prev : 0
}
;
var __counter = 0;
function test0(){
  var loopInvariant = shouldBailout ? 0 : 5;
  var GiantPrintArray = [];
  __counter++;;
  function makeArrayLength(x) { if(x < 1 || x > 4294967295 || x != x || isNaN(x) || !isFinite(x)) return 100; else return Math.floor(x) & 0xffff; };;
  function leaf() { return 100; };
  var obj0 = {};
  var protoObj0 = {};
  var obj1 = {};
  var protoObj1 = {};
  var arrObj0 = {};
  var litObj0 = {prop1: 3.14159265358979};
  var litObj1 = {prop0: 0, prop1: 1};
  var arrObj0 = {};
  var func0 = function(argMath0,argMath1,argMath2 /*= argMath1*/){
    return (arrObj0.length |= (typeof(arrObj0.length)  == 'undefined') );
  };
  var func1 = function(){
    return (((e >= this.prop1)||(f < protoObj1.prop0)) * (((obj0.prop1 !== protoObj1.length)||(arrObj0.length != g)) + +null));
  };
  var func2 = function(){
    strvar0 = strvar7[5%strvar7.length];
    arrObj0.prop1 /=((new RangeError()) instanceof ((typeof RegExp == 'function' ) ? RegExp : Object));
    var __loopvar1 = loopInvariant,__loopSecondaryVar1_0 = loopInvariant;
    for (var _strvar0 in ary) {
      if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
      __loopvar1 -= 2;
      __loopSecondaryVar1_0--;
      ary[_strvar0] = Math.pow((func2.caller), (protoObj0.prop0 += (((test0.caller) * ((func2.caller) - (func2.caller))) instanceof ((typeof Boolean == 'function' ) ? Boolean : Object))));
      (function(){
      })();
      _strvar0 = (func2.caller);
      strvar4 = 'Ö$'+'+.'+'%!' + '}%'.concat(((obj0.length = func1()) instanceof ((typeof RegExp == 'function' ) ? RegExp : Object)));
    }
    return (((d /= func1.call(litObj1 )) * arrObj0[(((((shouldBailout ? (arrObj0[((((shouldBailout ? (Object.defineProperty(obj1, 'prop1', {writable: false, enumerable: true, configurable: true }), (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255])) : (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255]))) >= 0 ? ( (shouldBailout ? (Object.defineProperty(obj1, 'prop1', {writable: false, enumerable: true, configurable: true }), (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255])) : (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255]))) : 0) & 0xF)] = 'x') : undefined ), (shouldBailout ? (Object.defineProperty(obj1, 'prop1', {writable: false, enumerable: true, configurable: true }), (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255])) : (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255]))) >= 0 ? (shouldBailout ? (Object.defineProperty(obj1, 'prop1', {writable: false, enumerable: true, configurable: true }), (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255])) : (shouldBailout ? (h = { valueOf: function() { WScript.Echo('h valueOf'); return 3; } }, ui32[(f64[(protoObj1.length) & 255]) & 255]) : ui32[(f64[(protoObj1.length) & 255]) & 255])) : 0)) & 0XF)] - uic8.length) ? ((protoObj1.prop1 != g)||(obj1.prop1 != d)) : (shouldBailout ? (Object.defineProperty(arrObj0, 'prop0', {writable: true, enumerable: false, configurable: true }), arguments[((((b >>= ary[((((i8[(91) & 255] >> -1536822999.9) >= 0 ? (i8[(91) & 255] >> -1536822999.9) : 0)) & 0XF)]) >= 0 ? (b >>= ary[((((i8[(91) & 255] >> -1536822999.9) >= 0 ? (i8[(91) & 255] >> -1536822999.9) : 0)) & 0XF)]) : 0)) & 0XF)]) : arguments[((((b >>= ary[((((i8[(91) & 255] >> -1536822999.9) >= 0 ? (i8[(91) & 255] >> -1536822999.9) : 0)) & 0XF)]) >= 0 ? (b >>= ary[((((i8[(91) & 255] >> -1536822999.9) >= 0 ? (i8[(91) & 255] >> -1536822999.9) : 0)) & 0XF)]) : 0)) & 0XF)]));
  };
  var func3 = function(argMath3,argMath4,...argArr5){
    var re1 = /^(?:(\d[a7]+\d)[a7]+(?!.))/g;
    func1();
    return Math.abs(((({valueOf: function() { return 65537;}, prop7: i16[709111450], prop6: {valueOf: function() { return -2147483648;}, prop1: (d += 115)}, prop4: (obj0.prop1 /= (typeof(protoObj0.length)  != 'undefined') ), prop3: (argMath4 === obj0.length), prop1: argMath4, 83: arrObj0[((((527182132 instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) >= 0 ? (527182132 instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) : 0)) & 0XF)], 62: ('1' + '^$!+'.charCodeAt((h)%7))} * argMath4 + ('%×Q!%' + '%,¢!;7%l' || (func3.caller))) && (typeof(protoObj0.prop0)  != 'undefined') ) !== (typeof(f)  != 'string') ));
  };
  var func4 = function(){
    function func5 () {
    }
    obj6 = new func5();
    return (typeof(h)  == 'number') ;
    return (test0.caller);
  };
  obj0.method0 = func3;
  obj0.method1 = func2;
  obj1.method0 = obj0.method0;
  obj1.method1 = obj0.method0;
  arrObj0.method0 = func3;
  arrObj0.method1 = func3;
  var ary = new Array(10);
  var i8 = new Int8Array(256);
  var i16 = new Int16Array(256);
  var i32 = new Int32Array(256);
  var ui8 = new Uint8Array(256);
  var ui16 = new Uint16Array(256);
  var ui32 = new Uint32Array(256);
  var f32 = new Float32Array(256);
  var f64 = new Float64Array(256);
  var uic8 = new Uint8ClampedArray(256);
  var IntArr0 = new Array(41,-840401897,2147483647,-737091287459698432,2073021661,-622597859,4294967296,4049808157226898432,-1073741824,-92706176,-73);
  var IntArr1 = [6016909847483059200,-2147483646,-216446918726729152,-478410706549293760,-1415934990,-1462370209,-168];
  var FloatArr0 = new Array();
  var VarArr0 = new Array('#',-7.15739939346669E+18,-27344852,-210972656,-813745142,65536,1556601913,4294967297,7263023039494802432);
  var a = 1252918647.1;
  var b = 120;
  var c = 2093313666;
  var d = -4.00291011644038E+18;
  var e = -256460535;
  var f = true;
  var g = 621525664;
  var h = 2147483647;
  var strvar0 = '1' + '^$!+';
  var strvar1 = '#' + '#!f!';
  var strvar2 = '#' + '#!f!';
  var strvar3 = 'Ðä'+'$o'+'$`' + '#a';
  var strvar4 = '±)K²$' + ')(6%$,f@';
  var strvar5 = 'Ðä'+'$o'+'$`' + '#a';
  var strvar6 = 'Ðä'+'$o'+'$`' + '#a';
  var strvar7 = 'F' + '!%H!';
  arrObj0[0] = +undefined;
  arrObj0[1] = -559911196.9;
  arrObj0[2] = 104;
  arrObj0[3] = 946969434;
  arrObj0[4] = +0;
  arrObj0[5] = -81;
  arrObj0[6] = undefined;
  arrObj0[7] = -2147483649;
  arrObj0[8] = 114;
  arrObj0[9] = +null;
  arrObj0[10] = 8.61626927758237E+18;
  arrObj0[11] = 254;
  arrObj0[12] = 237;
  arrObj0[13] = 4.53231215898624E+18;
  arrObj0[14] = +null;
  arrObj0[arrObj0.length-1] = 87;
  arrObj0.length = makeArrayLength(-240);
  ary[0] = 352456150;
  ary[1] = 1073741823;
  ary[2] = -4.60776890876879E+18;
  ary[3] = 214975552.1;
  ary[4] = -204611709;
  ary[5] = 759989774.1;
  ary[6] = +Infinity;
  ary[7] = -1032965232;
  ary[8] = -1209001971.9;
  ary[9] = -2147483648;
  ary[10] = -411847650;
  ary[11] = -2147483648;
  ary[12] = 65536;
  ary[13] = -1432897141.9;
  ary[14] = 710489564;
  ary[ary.length-1] = 166;
  ary.length = makeArrayLength(+Infinity);
  var protoObj0 = Object.create(obj0);
  var protoObj1 = Object.create(obj1);
  var aliasOfi16 = i16;;
  this.prop0 = -3.28790484587579E+18;
  this.prop1 = 317474964.1;
  obj0.prop0 = 0;
  obj0.prop1 = 5;
  obj0.length = makeArrayLength(-267259884);
  protoObj0.prop0 = 309791201;
  protoObj0.prop1 = -2147483649;
  protoObj0.length = makeArrayLength(-6.61778274392096E+17);
  obj1.prop0 = 1346446687;
  obj1.prop1 = +undefined;
  obj1.length = makeArrayLength(612356277);
  protoObj1.prop0 = -3;
  protoObj1.prop1 = -5.42001747651146E+18;
  protoObj1.length = makeArrayLength(8.17686394885755E+18);
  arrObj0.prop0 = 1538442027.1;
  arrObj0.prop1 = 3;
  arrObj0.length = makeArrayLength(+null);
  WScript.Echo(strvar3 >(test0.caller));
  strvar4 = strvar3[5%strvar3.length];
  with (protoObj1) {
    strvar4 = strvar4.concat((test0.caller));
    WScript.Echo(strvar2 >=(({prop0: (~ (protoObj0.prop1 !== this.prop1)), prop1: (-811083942 instanceof ((typeof Function == 'function' ) ? Function : Object)), prop2: (-945286020 * ((-811083942 instanceof ((typeof Function == 'function' ) ? Function : Object)) - 5.0008137661454E+18)), prop3: ((shouldBailout ? (Object.defineProperty(obj0, 'length', {get: function() { WScript.Echo('obj0.length getter'); return 3; }, configurable: true }), -46168042) : -46168042) * (test0.caller) + (! 'Ðä'+'$o'+'$`' + '#a')), prop4: (shouldBailout ? (Object.defineProperty(obj0, 'length', {writable: false, enumerable: true, configurable: true }), Object.create({66: -98, prop0: null, prop1: '#', prop2: NaN, prop4: true, prop5: b, prop6: protoObj1.length, prop7: arrObj0.length})) : Object.create({66: -98, prop0: null, prop1: '#', prop2: NaN, prop4: true, prop5: b, prop6: protoObj1.length, prop7: arrObj0.length})), prop5: ((~ undefined) ? (-472897588 instanceof ((typeof Array == 'function' ) ? Array : Object)) : protoObj0.method0.call(protoObj1 , strvar2, FloatArr0, VarArr0)), prop6: f32[(protoObj1.method0(strvar5,IntArr0,FloatArr0)) & 255]} + (test0.caller)) ? arrObj0.length : {20: ((shouldBailout ? obj0.method1 = func2 : 1), obj0.method1()), 31: VarArr0[(5)], prop1: (shouldBailout ? (Object.defineProperty(this, 'prop0', {writable: false, enumerable: false, configurable: true }), ((g == this.prop0)&&(obj1.prop1 === d))) : ((g == this.prop0)&&(obj1.prop1 === d))), prop3: ary[(16)], prop4: ((- protoObj0.method1()) >= (shouldBailout ? (Object.defineProperty(protoObj1, 'prop1', {get: function() { WScript.Echo('protoObj1.prop1 getter'); return 3; }, set: function(_x) { WScript.Echo('protoObj1.prop1 setter'); }, configurable: true }), obj1.method1(strvar7,ary,VarArr0)) : obj1.method1(strvar7,ary,VarArr0))), prop6: ((obj1.prop1 <= obj0.prop1)||(obj1.length > arrObj0.prop1)), valueOf: function() { return 2147483650;}}));
    obj6 = Object.create(protoObj0);
  }
  var __loopvar0 = loopInvariant,__loopSecondaryVar0_0 = loopInvariant;
  do {
    __loopSecondaryVar0_0 += 2;
    if (__loopvar0 >= loopInvariant + 2) break;
    __loopvar0++;
    strvar6 = (',' + '%%Q*').replace('T', '1' + '^$!+') + arrObj0[((((((shouldBailout ? obj6.method0 = obj0.method0 : 1), obj6.method0(strvar5,IntArr0,FloatArr0)), h) >= 0 ? (((shouldBailout ? obj6.method0 = obj0.method0 : 1), obj6.method0(strvar5,IntArr0,FloatArr0)), h) : 0)) & 0XF)];
    strvar0 = strvar5.concat(obj1.prop1).concat((typeof(arrObj0.length)  != 'string') );
  } while((i16[((((shouldBailout ? obj6.method0 = obj0.method0 : 1), obj6.method0(strvar5,IntArr0,FloatArr0)), h)) & 255]))
  obj0.prop1={20: ((shouldBailout ? obj0.method1 = func2 : 1), obj0.method1()), 31: VarArr0[(5)], prop1: (shouldBailout ? (Object.defineProperty(this, 'prop0', {writable: false, enumerable: false, configurable: true }), ((g == this.prop0)&&(obj1.prop1 === d))) : ((g == this.prop0)&&(obj1.prop1 === d))), prop3: ary[(16)], prop4: ((- protoObj0.method1()) >= (shouldBailout ? (Object.defineProperty(protoObj1, 'prop1', {get: function() { WScript.Echo('protoObj1.prop1 getter'); return 3; }, set: function(_x) { WScript.Echo('protoObj1.prop1 setter'); }, configurable: true }), obj1.method1(strvar7,ary,VarArr0)) : obj1.method1(strvar7,ary,VarArr0))), prop6: ((obj1.prop1 <= obj0.prop1)||(obj1.length > arrObj0.prop1)), valueOf: function() { return 2147483650;}};
  WScript.Echo('a = ' + (a|0));
  WScript.Echo('b = ' + (b|0));
  WScript.Echo('c = ' + (c|0));
  WScript.Echo('d = ' + (d|0));
  WScript.Echo('e = ' + (e|0));
  WScript.Echo('f = ' + (f|0));
  WScript.Echo('g = ' + (g|0));
  WScript.Echo('h = ' + (h|0));
  WScript.Echo('this.prop0 = ' + (this.prop0|0));
  WScript.Echo('this.prop1 = ' + (this.prop1|0));
  WScript.Echo('obj0.prop0 = ' + (obj0.prop0|0));
  WScript.Echo('obj0.prop1 = ' + (obj0.prop1|0));
  WScript.Echo('obj0.length = ' + (obj0.length|0));
  WScript.Echo('protoObj0.prop0 = ' + (protoObj0.prop0|0));
  WScript.Echo('protoObj0.prop1 = ' + (protoObj0.prop1|0));
  WScript.Echo('protoObj0.length = ' + (protoObj0.length|0));
  WScript.Echo('obj1.prop0 = ' + (obj1.prop0|0));
  WScript.Echo('obj1.prop1 = ' + (obj1.prop1|0));
  WScript.Echo('obj1.length = ' + (obj1.length|0));
  WScript.Echo('protoObj1.prop0 = ' + (protoObj1.prop0|0));
  WScript.Echo('protoObj1.prop1 = ' + (protoObj1.prop1|0));
  WScript.Echo('protoObj1.length = ' + (protoObj1.length|0));
  WScript.Echo('arrObj0.prop0 = ' + (arrObj0.prop0|0));
  WScript.Echo('arrObj0.prop1 = ' + (arrObj0.prop1|0));
  WScript.Echo('arrObj0.length = ' + (arrObj0.length|0));
  WScript.Echo('obj6.prop0 = ' + (obj6.prop0|0));
  WScript.Echo('obj6.prop1 = ' + (obj6.prop1|0));
  WScript.Echo('obj6.length = ' + (obj6.length|0));
  WScript.Echo('strvar0 = ' + (strvar0));
  WScript.Echo('strvar1 = ' + (strvar1));
  WScript.Echo('strvar2 = ' + (strvar2));
  WScript.Echo('strvar3 = ' + (strvar3));
  WScript.Echo('strvar4 = ' + (strvar4));
  WScript.Echo('strvar5 = ' + (strvar5));
  WScript.Echo('strvar6 = ' + (strvar6));
  WScript.Echo('strvar7 = ' + (strvar7));
  WScript.Echo('arrObj0[0] = ' + (arrObj0[0]|0));
  WScript.Echo('arrObj0[1] = ' + (arrObj0[1]|0));
  WScript.Echo('arrObj0[2] = ' + (arrObj0[2]|0));
  WScript.Echo('arrObj0[3] = ' + (arrObj0[3]|0));
  WScript.Echo('arrObj0[4] = ' + (arrObj0[4]|0));
  WScript.Echo('arrObj0[5] = ' + (arrObj0[5]|0));
  WScript.Echo('arrObj0[6] = ' + (arrObj0[6]|0));
  WScript.Echo('arrObj0[7] = ' + (arrObj0[7]|0));
  WScript.Echo('arrObj0[8] = ' + (arrObj0[8]|0));
  WScript.Echo('arrObj0[9] = ' + (arrObj0[9]|0));
  WScript.Echo('arrObj0[10] = ' + (arrObj0[10]|0));
  WScript.Echo('arrObj0[11] = ' + (arrObj0[11]|0));
  WScript.Echo('arrObj0[12] = ' + (arrObj0[12]|0));
  WScript.Echo('arrObj0[13] = ' + (arrObj0[13]|0));
  WScript.Echo('arrObj0[14] = ' + (arrObj0[14]|0));
  WScript.Echo('arrObj0[arrObj0.length-1] = ' + (arrObj0[arrObj0.length-1]|0));
  WScript.Echo('arrObj0.length = ' + (arrObj0.length|0));
  WScript.Echo('ary[0] = ' + (ary[0]|0));
  WScript.Echo('ary[1] = ' + (ary[1]|0));
  WScript.Echo('ary[2] = ' + (ary[2]|0));
  WScript.Echo('ary[3] = ' + (ary[3]|0));
  WScript.Echo('ary[4] = ' + (ary[4]|0));
  WScript.Echo('ary[5] = ' + (ary[5]|0));
  WScript.Echo('ary[6] = ' + (ary[6]|0));
  WScript.Echo('ary[7] = ' + (ary[7]|0));
  WScript.Echo('ary[8] = ' + (ary[8]|0));
  WScript.Echo('ary[9] = ' + (ary[9]|0));
  WScript.Echo('ary[10] = ' + (ary[10]|0));
  WScript.Echo('ary[11] = ' + (ary[11]|0));
  WScript.Echo('ary[12] = ' + (ary[12]|0));
  WScript.Echo('ary[13] = ' + (ary[13]|0));
  WScript.Echo('ary[14] = ' + (ary[14]|0));
  WScript.Echo('ary[ary.length-1] = ' + (ary[ary.length-1]|0));
  WScript.Echo('ary.length = ' + (ary.length|0));
  for (var i = 0; i < GiantPrintArray.length; i++) {
  WScript.Echo(GiantPrintArray[i]);
  }
;
  WScript.Echo('sumOfary = ' +  ary.slice(0, 23).reduce(function(prev, curr) {{ return '' + prev + curr; }},0));
  WScript.Echo('subset_of_ary = ' +  ary.slice(0, 11));;
  WScript.Echo('sumOfIntArr0 = ' +  IntArr0.slice(0, 23).reduce(function(prev, curr) {{ return '' + prev + curr; }},0));
  WScript.Echo('subset_of_IntArr0 = ' +  IntArr0.slice(0, 11));;
  WScript.Echo('sumOfIntArr1 = ' +  IntArr1.slice(0, 23).reduce(function(prev, curr) {{ return '' + prev + curr; }},0));
  WScript.Echo('subset_of_IntArr1 = ' +  IntArr1.slice(0, 11));;
  WScript.Echo('sumOfFloatArr0 = ' +  FloatArr0.slice(0, 23).reduce(function(prev, curr) {{ return '' + prev + curr; }},0));
  WScript.Echo('subset_of_FloatArr0 = ' +  FloatArr0.slice(0, 11));;
  WScript.Echo('sumOfVarArr0 = ' +  VarArr0.slice(0, 23).reduce(function(prev, curr) {{ return '' + prev + curr; }},0));
  WScript.Echo('subset_of_VarArr0 = ' +  VarArr0.slice(0, 11));;
};

// generate profile
test0();
// Run Simple JIT
test0();

// run JITted code
runningJITtedCode = true;
test0();

// run code with bailouts enabled
shouldBailout = true;
test0();

print('pass');
