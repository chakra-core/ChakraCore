//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Configuration: configs\SIMD.xml
//Testcase Number: 89
//Bailout Testing: ON
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:5 -maxsimplejitruncount:6 -werexceptionsupport  -forcejitloopbody -force:rejit -force:atom
//Baseline Switches: -nonative -werexceptionsupport  -PrintSystemException  -simdjs
//Arch: X86
//Flavor: debug
//BuildName: full
//BuildRun: 160404_0313
//BuildId: 
//BuildHash: 
//BinaryPath: E:\nagy\perf-test\ExprGen
//MachineName: B-NAMOST-CHAKRA
//InstructionSet: 
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
WScript.Echo = function(n) { if(!n) print(n); else print(formatOutput(n.toString())); };

function formatOutput(n) {{
 return n.replace(/[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?/g, function(match) {{return getRoundValue(parseFloat(match));}} );
}};
function sumOfArrayElements(prev, curr, index, array) {
    return (typeof prev == "number" && typeof curr == "number") ? curr + prev : 0
}
;
var __counter = 0;
function test0(){
  var loopInvariant = shouldBailout ? 9 : 1;
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
  var initSimdVar0_Float32x4 = SIMD.Float32x4(670478263,225,1073741823,-208);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(17,81579394,-399931217,-977456905);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(21761,737,17012,28198,4355,24401,1295,20346);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(107,34,80,87,124,109,127,102,66,87,43,111,10,121,2,62);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(63155734,1559126943,2675464672,3900630106);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(1881,39146,34309,45662,10012,59000,1511,63714);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(210,56,165,95,31,112,222,233,219,244,306022185,13,144,-1042051429,220,77);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(true,false,false,false);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(true,true,true,true,true,false,269198866,true);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(true,true,false,true,true,false,false,-1297676219,false,false,false,true,true,true,false,true);
  var func0 = function(argMath0,argMath1,argMath2,argMath3){
    return SIMD.Bool16x8.allTrue(initSimdVar8_Bool16x8);
  };
  var func1 = function(argMath4,argMath5,argMath6){
    return argMath6;
  };
  var func2 = function(argMath7){
    var simdVar0_Uint16x8 = SIMD.Uint16x8.shiftLeftByScalar(SIMD.Uint16x8.fromUint32x4Bits(SIMD.Uint32x4(3324148853,2535032209,794934118,2247279738)),(! arguments[((shouldBailout ? (arguments[5] = 'x') : undefined ), 5)]));
    obj0.prop1 =(-- argMath7);
    protoObj1.length = makeArrayLength(SIMD.Float32x4.extractLane(SIMD.Float32x4.shuffle(SIMD.Float32x4.check(initSimdVar0_Float32x4),initSimdVar0_Float32x4,1,2,7,6),1));
    return (SIMD.Float32x4.extractLane(SIMD.Float32x4.load2(ui8,542597458 % (ui8.length - (128 / ui8.BYTES_PER_ELEMENT / 8 ))),2) ? func0(leaf,litObj1,(! arguments[((shouldBailout ? (arguments[5] = 'x') : undefined ), 5)]),ary) : argMath7);
  };
  var func3 = function(argMath8,argMath9,argMath10,argMath11){
    var uniqobj0 = [''];
    uniqobj0[__counter%uniqobj0.length].toString();
    c =g;
    return (SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4_alias) | SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,11));
  };
  var func4 = function(argMath12,argMath13,argMath14){
    return argMath12;
  };
  obj0.method0 = func2;
  obj0.method1 = func0;
  obj1.method0 = func2;
  obj1.method1 = obj0.method0;
  arrObj0.method0 = func2;
  arrObj0.method1 = func0;
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
  var IntArr0 = [];
  var IntArr1 = [];
  var FloatArr0 = new Array(657944115,-115,256392092,124,1035435824,953609078,109956164.1,-1029087289);
  var VarArr0 = [arrObj0,7.03033852365978E+18,66,-254,3.32425302077546E+18,-4.58436682471616E+18,3,570937364,4294967296,-362790647.9,-1];
  var a = 4294967295;
  var b = -4.28455360713621E+18;
  var c = 2113102104.1;
  var d = -221374279;
  var e = 167;
  var f = 603592138;
  var g = 1012677932.1;
  var h = -2129643436;
  arrObj0[0] = -1988091023.9;
  arrObj0[1] = 1800755040.1;
  arrObj0[2] = 4294967295;
  arrObj0[3] = 5.29731385439777E+18;
  arrObj0[4] = -73;
  arrObj0[5] = 1448266545.1;
  arrObj0[6] = -247549663;
  arrObj0[7] = -321233585;
  arrObj0[8] = 1032182616;
  arrObj0[9] = -3.24363686906165E+17;
  arrObj0[10] = 65536;
  arrObj0[11] = 1738730557.1;
  arrObj0[12] = 55587362;
  arrObj0[13] = -187;
  arrObj0[14] = 8.46331634025623E+18;
  arrObj0[arrObj0.length-1] = 1.294292603303E+18;
  arrObj0.length = makeArrayLength(6.81879604400472E+17);
  ary[0] = 252;
  ary[1] = -1693617860;
  ary[2] = 2.0004903003283E+18;
  ary[3] = -732560678;
  ary[4] = 972988095;
  ary[5] = 869878620;
  ary[6] = 98;
  ary[7] = 1.72766582639316E+18;
  ary[8] = -40909343;
  ary[9] = 131;
  ary[10] = 251;
  ary[11] = -2147483648;
  ary[12] = 751326237;
  ary[13] = -1.55124194557924E+18;
  ary[14] = -600212049;
  ary[ary.length-1] = 1744886664.1;
  ary.length = makeArrayLength(-2147483648);
  protoObj0 = Object.create(obj0);
  protoObj1 = Object.create(obj1);
  var initSimdVar1_Int32x4_alias = initSimdVar1_Int32x4;;
  var initSimdVar2_Int16x8_alias = initSimdVar2_Int16x8;;
  var initSimdVar7_Bool32x4_alias = initSimdVar7_Bool32x4;;
  var initSimdVar8_Bool16x8_alias = initSimdVar8_Bool16x8;;
  var initSimdVar9_Bool8x16_alias = initSimdVar9_Bool8x16;;
  var aliasOff32 = f32;;
  this.prop0 = 65537;
  this.prop1 = -1153172433;
  obj0.prop0 = -283319710;
  obj0.prop1 = 7.48477693549019E+18;
  obj0.length = makeArrayLength(-4.21608643334903E+17);
  protoObj0.prop0 = 1253498509.1;
  protoObj0.prop1 = 2147483647;
  protoObj0.length = makeArrayLength(-1537518283);
  obj1.prop0 = 146;
  obj1.prop1 = -1999136558.9;
  obj1.length = makeArrayLength(133821055.1);
  protoObj1.prop0 = 3;
  protoObj1.prop1 = 166984540.1;
  protoObj1.length = makeArrayLength(1.83642329555125E+18);
  arrObj0.prop0 = -966831775;
  arrObj0.prop1 = 830705057.1;
  arrObj0.length = makeArrayLength(-1785733773.9);
  IntArr0[11] = 2059058032;
  IntArr0[IntArr0.length] = -1015483041;
  IntArr0[8] = 2020572665;
  IntArr0[1] = -772128793;
  IntArr0[5] = -249;
  IntArr0[3] = 937809482.1;
  IntArr0[12] = 2147483647;
  IntArr0[7] = 906437166;
  IntArr0[IntArr0.length] = -710102017.9;
  IntArr0[10] = 1398878675;
  IntArr0[9] = 0;
  IntArr0[4] = 0;
  IntArr0[0] = 1740310903;
  IntArr1[4] = 1954522356.1;
  IntArr1[1] = 3;
  IntArr1[3] = -132;
  IntArr1[0] = -526272723;
  IntArr1[2] = -8.18280042525858E+18;
  FloatArr0[10] = 65537;
  FloatArr0[FloatArr0.length] = -5.68754061798111E+18;
  FloatArr0[FloatArr0.length] = -5.9896034905415E+18;
  FloatArr0[8] = -1438745488;
  litObj0.prop1 = IntArr0[(((((shouldBailout ? (IntArr0[(((obj0.method0.call(arrObj0 , SIMD.Float32x4.extractLane(SIMD.Float32x4.load2(i32,1999436700 % (i32.length - (128 / i32.BYTES_PER_ELEMENT / 8 ))),2))) >= 0 ? ( obj0.method0.call(arrObj0 , SIMD.Float32x4.extractLane(SIMD.Float32x4.load2(i32,1999436700 % (i32.length - (128 / i32.BYTES_PER_ELEMENT / 8 ))),2))) : 0) & 0xF)] = 'x') : undefined ), obj0.method0.call(arrObj0 , SIMD.Float32x4.extractLane(SIMD.Float32x4.load2(i32,1999436700 % (i32.length - (128 / i32.BYTES_PER_ELEMENT / 8 ))),2))) >= 0 ? obj0.method0.call(arrObj0 , SIMD.Float32x4.extractLane(SIMD.Float32x4.load2(i32,1999436700 % (i32.length - (128 / i32.BYTES_PER_ELEMENT / 8 ))),2)) : 0)) & 0XF)];
  var simdVar1_Float32x4 = SIMD.Float32x4.sqrt(SIMD.Float32x4.load2(ui8,1841739273 % (ui8.length - (128 / ui8.BYTES_PER_ELEMENT / 8 ))));
  var simdVar2_Bool16x8 = SIMD.Bool16x8.check(SIMD.Uint16x8.lessThanOrEqual(SIMD.Uint16x8.swizzle(SIMD.Uint16x8.load(ui32,1898207359 % (ui32.length - (128 / ui32.BYTES_PER_ELEMENT / 8 ))),1,7,1,3,0,5,3,6),initSimdVar5_Uint16x8));
  var simdVar3_Int32x4 = SIMD.Int32x4.fromFloat32x4Bits(SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(18848130,1946293512,55,42)));
  obj0.length=SIMD.Int32x4.extractLane(initSimdVar1_Int32x4_alias,0);
  var uniqobj1 = [obj1, arrObj0, protoObj0, arrObj0];
  var uniqobj2 = uniqobj1[__counter%uniqobj1.length];
  uniqobj2.method0((((shouldBailout ? obj1.method0 = obj1.method0 : 1), obj1.method0()) !== (test0.caller)));
  var __loopvar0 = loopInvariant - 3;
  LABEL0: 
  LABEL1: 
  for (var _strvar26 in i16) {
    if(typeof _strvar26 === 'string' && _strvar26.indexOf('method') != -1) continue;
    __loopvar0++;
    i16[_strvar26] = (test0.caller);
    protoObj0.method0=IntArr0[((shouldBailout ? (IntArr0[loopInvariant - 3] = 'x') : undefined ), loopInvariant - 3)];
    
    var simdVar4_Uint16x8 = SIMD.Uint16x8.fromUint8x16Bits(SIMD.Uint8x16(202,125,229,136,1,179,65,239,95,168,13,198,145,233,69,244));
    litObj6 = {prop0: (test0.caller), prop1: -1527985403, prop2: SIMD.Int32x4.extractLane(initSimdVar1_Int32x4,1), prop3: (test0.caller)};
    var simdVar5_Int16x8 = SIMD.Int16x8.addSaturate(SIMD.Int16x8.load(i16,1101290507 % (i16.length - (128 / i16.BYTES_PER_ELEMENT / 8 ))),initSimdVar2_Int16x8);
  }
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
  WScript.Echo('litObj0.prop1 = ' + (litObj0.prop1|0));
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
  WScript.Echo('initSimdVar0_Float32x4 = ' + (initSimdVar0_Float32x4.toString()));
  WScript.Echo('simdVar1_Float32x4 = ' + (simdVar1_Float32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4 = ' + (initSimdVar1_Int32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4_alias = ' + (initSimdVar1_Int32x4_alias.toString()));
  WScript.Echo('simdVar3_Int32x4 = ' + (simdVar3_Int32x4.toString()));
  WScript.Echo('initSimdVar2_Int16x8 = ' + (initSimdVar2_Int16x8.toString()));
  WScript.Echo('initSimdVar2_Int16x8_alias = ' + (initSimdVar2_Int16x8_alias.toString()));
  WScript.Echo('initSimdVar3_Int8x16 = ' + (initSimdVar3_Int8x16.toString()));
  WScript.Echo('initSimdVar4_Uint32x4 = ' + (initSimdVar4_Uint32x4.toString()));
  WScript.Echo('initSimdVar5_Uint16x8 = ' + (initSimdVar5_Uint16x8.toString()));
  WScript.Echo('initSimdVar6_Uint8x16 = ' + (initSimdVar6_Uint8x16.toString()));
  WScript.Echo('initSimdVar7_Bool32x4 = ' + (initSimdVar7_Bool32x4.toString()));
  WScript.Echo('initSimdVar7_Bool32x4_alias = ' + (initSimdVar7_Bool32x4_alias.toString()));
  WScript.Echo('initSimdVar8_Bool16x8 = ' + (initSimdVar8_Bool16x8.toString()));
  WScript.Echo('initSimdVar8_Bool16x8_alias = ' + (initSimdVar8_Bool16x8_alias.toString()));
  WScript.Echo('simdVar2_Bool16x8 = ' + (simdVar2_Bool16x8.toString()));
  WScript.Echo('initSimdVar9_Bool8x16 = ' + (initSimdVar9_Bool8x16.toString()));
  WScript.Echo('initSimdVar9_Bool8x16_alias = ' + (initSimdVar9_Bool8x16_alias.toString()));
  for (var i = 0; i < GiantPrintArray.length; i++) {
  WScript.Echo(GiantPrintArray[i]);
  };
  WScript.Echo('sumOfary = ' +  ary.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  WScript.Echo('subset_of_ary = ' +  ary.slice(0, 11));;
  WScript.Echo('sumOfIntArr0 = ' +  IntArr0.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  WScript.Echo('subset_of_IntArr0 = ' +  IntArr0.slice(0, 11));;
  WScript.Echo('sumOfIntArr1 = ' +  IntArr1.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  WScript.Echo('subset_of_IntArr1 = ' +  IntArr1.slice(0, 11));;
  WScript.Echo('sumOfFloatArr0 = ' +  FloatArr0.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  WScript.Echo('subset_of_FloatArr0 = ' +  FloatArr0.slice(0, 11));;
  WScript.Echo('sumOfVarArr0 = ' +  VarArr0.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  WScript.Echo('subset_of_VarArr0 = ' +  VarArr0.slice(0, 11));;
};

// generate profile
test0();
test0();
test0();
test0();
test0();
// Run Simple JIT
test0();
test0();
test0();
test0();
test0();
test0();

// run JITted code
runningJITtedCode = true;
test0();
test0();

// run code with bailouts enabled
shouldBailout = true;
test0();


// Baseline total processor time: 00:00:01.1093750
// Test total processor time: 00:00:03.2187500
// 
// Addl Dump Info: 
//Baseline length = 50619

//Test length = 11138

//Baseline length = 50619

//Test length = 11138

//Baseline length = 50619

//Test length = 11138

// 
// Baseline output:
// Skipping first 1213 lines of output...
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(1881, 39146, 34309, 45662, 10012, 59000, 1511, 63714)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(210, 56, 165, 95, 31, 112, 222, 233, 219, 244, 41, 13, 144, 155, 220, 77)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, false, false, false)
// initSimdVar7_Bool32x4_alias = SIMD.Bool32x4(true, false, false, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, true, true, true, true, false, true, true)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, true, true, true, true, false, true, true)
// simdVar2_Bool16x8 = SIMD.Bool16x8(true, true, true, true, true, true, true, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, true, false, true, true, false, false, true, false, false, false, true, true, true, false, true)
// initSimdVar9_Bool8x16_alias = SIMD.Bool8x16(true, true, false, true, true, false, false, true, false, false, false, true, true, true, false, true)
// sumOfary = 629033428
// subset_of_ary = 252,-1693617860,1999971496,-732560678,972988095,869878620,98,280151765,-40909343,131,251
// sumOfIntArr0 = 0x-686939800-249x419252888-710102017.90000000
// subset_of_IntArr0 = x,-772128793,,937809482.10000000,0,-249,x,906437166,2020572665,0,1398878675
// sumOfIntArr1 = -1970333508
// subset_of_IntArr1 = -526272723,3,-1251099461,-132,1954522356.10000000
// sumOfFloatArr0 = -1998551399
// subset_of_FloatArr0 = 657944115,-115,256392092,124,1035435824,953609078,109956164.10000000,-1029087289,-1438745488,,65537
// sumOfVarArr0 = 0[object Object]228849762-309705774-1403145278-362790647.90000000-1
// subset_of_VarArr0 = [object Object],1419628360,66,-254,1528523013,-176729774,3,570937364,2,-362790647.90000000,-1
// 
// 
// Test output:
// Skipping first 248 lines of output...
// initSimdVar7_Bool32x4_alias = SIMD.Bool32x4(true, false, false, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, true, true, true, true, false, true, true)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, true, true, true, true, false, true, true)
// simdVar2_Bool16x8 = SIMD.Bool16x8(true, true, true, true, true, true, true, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, true, false, true, true, false, false, true, false, false, false, true, true, true, false, true)
// initSimdVar9_Bool8x16_alias = SIMD.Bool8x16(true, true, false, true, true, false, false, true, false, false, false, true, true, true, false, true)
// sumOfary = 629033428
// subset_of_ary = 252,-1693617860,1999971496,-732560678,972988095,869878620,98,280151765,-40909343,131,251
// sumOfIntArr0 = 9728319510.20000000
// subset_of_IntArr0 = 1740310903,-772128793,,937809482.10000000,0,-249,,906437166,2020572665,0,1398878675
// sumOfIntArr1 = -1970333508
// subset_of_IntArr1 = -526272723,3,-1251099461,-132,1954522356.10000000
// sumOfFloatArr0 = -1998551399
// subset_of_FloatArr0 = 657944115,-115,256392092,124,1035435824,953609078,109956164.10000000,-1029087289,-1438745488,,65537
// sumOfVarArr0 = 0[object Object]228849762-309705774-1403145278-362790647.90000000-1
// subset_of_VarArr0 = [object Object],1419628360,66,-254,1528523013,-176729774,3,570937364,2,-362790647.90000000,-1
// ASSERTION 25392: (e:\nagy\git\chakra5\core\lib\Backend\GlobOpt.cpp, line 19829) IsUninitialized() || IsLikelyInt() || isForLoopBackEdgeCompensation
//  Failure: (IsUninitialized() || IsLikelyInt() || isForLoopBackEdgeCompensation)
// FATAL ERROR: jshost.exe failed due to exception code c0000420
// 
// Reduced Switches:
