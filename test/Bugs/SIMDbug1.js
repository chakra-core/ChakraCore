//Configuration: configs\SIMD.xml
//Testcase Number: 84
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:1 -maxsimplejitruncount:3 -werexceptionsupport  -MinSwitchJumpTableSize:3 -MaxLinearStringCaseCount:2 -MaxLinearIntCaseCount:2 -forcedeferparse -force:rejit -force:atom -force:ScriptFunctionWithInlineCache -off:fefixedmethods -off:ArrayCheckHoist -force:fixdataprops -off:DelayCapture -off:trackintusage -off:ParallelParse -off:BoundCheckElimination -ForceArrayBTree -ValidateInlineStack -off:aggressiveinttypespec
//Baseline Switches: -nonative -werexceptionsupport  -PrintSystemException  -simdjs
//Arch: X86
//Flavor: debug
//BuildName: full
//BuildRun: 160408_1210
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
  var loopInvariant = shouldBailout ? 5 : 0;
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
  var initSimdVar0_Float32x4 = SIMD.Float32x4(65535,-103,1.5075917433652E+18,-2.16615373785935E+18);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(685034769,-39,102426315,-347720658);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(-38,12875,10027,9127,11526,2971,4847,14696);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(122,76,47,21,53,22,94,127,104,175,7,33,12,95,21,78);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(1001306347,2092887379,4219646051,1663425254);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(44734,49699,34142,211,35688,24976,8865,44822);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(160,362678783,95,45,-501561787,255,171,236,100,22,111,166,181,4,84,8);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(false,false,true,false);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(false,true,true,false,true,true,true,true);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(false,false,true,true,true,true,54,true,false,false,false,false,false,true,false,false);
  var func0 = function(){
    var simdVar0_Int32x4 = SIMD.Int32x4.add(initSimdVar1_Int32x4,SIMD.Int32x4.mul(SIMD.Int32x4.splat(),initSimdVar1_Int32x4));
    (function(){
    'use strict';;
    })();
    var simdVar1_Float32x4 = SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(98,849177859,-1084690023,-846766451));
    return (b >>= arrObj0[(((SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,3) >= 0 ? SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,3) : 0)) & 0XF)]);
  };
  var func1 = function(argMath0){
    var simdVar2_Int8x16 = SIMD.Int8x16.fromUint8x16Bits(SIMD.Uint8x16(231,154,90,87,94,66,177,185,92,203,73,70,185,184,12,53));
    var simdVar3_Uint32x4 = SIMD.Uint32x4(2811590361,4212849623,984770853,1448824078);
    var simdVar4_Int32x4 = SIMD.Int32x4.splat(-171);
    return (SIMD.Float32x4.extractLane(SIMD.Float32x4.replaceLane(initSimdVar0_Float32x4,1,1073741823),0) === SIMD.Int32x4.extractLane(SIMD.Int32x4.not(SIMD.Int32x4.fromUint16x8Bits(SIMD.Uint16x8(27750,19493,53829,10929,28227,28439,17617,17560)),initSimdVar1_Int32x4),2));
  };
  var func2 = function(argMath1,argMath2,argMath3,argMath4){
    return (argMath3 || argMath3);
  };
  var func3 = function(argMath5,argMath6){
    var simdVar5_Int8x16 = SIMD.Int8x16.load(i32,179969725 % (i32.length - (128 / i32.BYTES_PER_ELEMENT / 8 )));
    return arrObj0[(((((SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,2) ? func2.call(obj0 , SIMD.Bool8x16.allTrue(SIMD.Int8x16.lessThan(initSimdVar3_Int8x16,initSimdVar3_Int8x16)), SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,6), arrObj0, (-9.02652980340363E+18 * argMath6 - (-1862528944 % SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16_alias,10)))) : SIMD.Int16x8.extractLane(initSimdVar2_Int16x8,7)) !== SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,2)) >= 0 ? ((SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,2) ? func2.call(obj0 , SIMD.Bool8x16.allTrue(SIMD.Int8x16.lessThan(initSimdVar3_Int8x16,initSimdVar3_Int8x16)), SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,6), arrObj0, (-9.02652980340363E+18 * argMath6 - (-1862528944 % SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16_alias,10)))) : SIMD.Int16x8.extractLane(initSimdVar2_Int16x8,7)) !== SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,2)) : 0)) & 0XF)];
  };
  var func4 = function(){
    if(SIMD.Uint16x8.extractLane(SIMD.Uint16x8.mul(SIMD.Uint16x8.swizzle(initSimdVar5_Uint16x8,6,1,3,7,3,7,5,4),initSimdVar5_Uint16x8),3)) {
      arrObj0 = protoObj1;
      var uniqobj0 = {prop0: Math.cos(protoObj1.prop0), prop1: SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4_alias), prop2: (ary.push())
, prop3: obj1.prop1, prop4: (SIMD.Int16x8.extractLane(initSimdVar2_Int16x8,7) / (SIMD.Uint8x16.extractLane(SIMD.Uint8x16.addSaturate(initSimdVar6_Uint8x16_alias,initSimdVar6_Uint8x16_alias),2) == 0 ? 1 : SIMD.Uint8x16.extractLane(SIMD.Uint8x16.addSaturate(initSimdVar6_Uint8x16_alias,initSimdVar6_Uint8x16_alias),2))), prop5: (e %= (SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,3) ? SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4_alias) : ui16[4294967296])), prop6: ui16[4294967296], prop7: (obj0.prop0 > f)};
      var uniqobj1 = {prop0: -117};
      uniqobj0.prop7 >>=(protoObj1.prop1 ^ 87);
      if(shouldBailout){
        return  'somestring'
      }
      GiantPrintArray.push('uniqobj1.prop0 = ' + (uniqobj1.prop0|0));
    }
    else {
    }
    return ((new Array()) instanceof ((typeof RegExp == 'function' ) ? RegExp : Object));
  };
  obj0.method0 = func4;
  obj0.method1 = func3;
  obj1.method0 = obj0.method0;
  obj1.method1 = obj0.method1;
  arrObj0.method0 = obj0.method1;
  arrObj0.method1 = obj0.method0;
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
  var IntArr0 = [139,3907536646722417152,-190,-1,-166,-514034635524096576];
  var IntArr1 = new Array(-517597735,7428601623572395008,408032378,1319610150,2147483650,1553152899,4061540591982024704,-7982036117119077376,216,-7707779033642333184,-430485707,-9092033065949389824,-968639513,2147483647,865894389);
  var FloatArr0 = new Array(12);
  var VarArr0 = new Array();
  var a = -238;
  var b = 313023262.1;
  var c = 26;
  var d = -33503287;
  var e = -1046981973;
  var f = 923261381;
  var g = 201441037;
  var h = 65536;
  arrObj0[0] = -79;
  arrObj0[1] = 9.1876674915093E+18;
  arrObj0[2] = 1063290317;
  arrObj0[3] = 550316884;
  arrObj0[4] = -926511203;
  arrObj0[5] = 7.65461650217185E+18;
  arrObj0[6] = 247;
  arrObj0[7] = 1868718281;
  arrObj0[8] = -144;
  arrObj0[9] = 788737512.1;
  arrObj0[10] = -4.25002059115573E+18;
  arrObj0[11] = -112699600.9;
  arrObj0[12] = 543259654;
  arrObj0[13] = -1887528332.9;
  arrObj0[14] = 1750636645;
  arrObj0[arrObj0.length-1] = -241;
  arrObj0.length = makeArrayLength(-422891873);
  ary[0] = -756262382;
  ary[1] = 879308470.1;
  ary[2] = -210;
  ary[3] = 65536;
  ary[4] = 127962566;
  ary[5] = -353433090;
  ary[6] = -991863055;
  ary[7] = 1471842196.1;
  ary[8] = -13;
  ary[9] = -651771025.9;
  ary[10] = 7.32523574580659E+18;
  ary[11] = -1502524257;
  ary[12] = 286118458;
  ary[13] = 6.92586010761588E+18;
  ary[14] = -666179450.9;
  ary[ary.length-1] = -579122715;
  ary.length = makeArrayLength(-2);
  protoObj0 = Object.create(obj0);
  protoObj1 = Object.create(obj1);
  var initSimdVar6_Uint8x16_alias = initSimdVar6_Uint8x16;;
  var initSimdVar7_Bool32x4_alias = initSimdVar7_Bool32x4;;
  var aliasOfVarArr0 = VarArr0;;
  this.prop0 = 2;
  this.prop1 = 248;
  obj0.prop0 = 1211586297;
  obj0.prop1 = -2.11531997479267E+18;
  obj0.length = makeArrayLength(-2147483649);
  protoObj0.prop0 = -1496248022.9;
  protoObj0.prop1 = -957744856;
  protoObj0.length = makeArrayLength(-1110282062);
  obj1.prop0 = 9;
  obj1.prop1 = 3;
  obj1.length = makeArrayLength(-192);
  protoObj1.prop0 = +NaN;
  protoObj1.prop1 = 247;
  protoObj1.length = makeArrayLength(342539950);
  arrObj0.prop0 = -8332099;
  arrObj0.prop1 = 650492937;
  arrObj0.length = makeArrayLength(undefined);
  FloatArr0[8] = 4294967295;
  FloatArr0[9] = 48079027;
  FloatArr0[6] = 822778065.1;
  FloatArr0[2] = -496168303;
  FloatArr0[11] = 3.72043787675966E+18;
  FloatArr0[1] = -2.65993840992275E+18;
  FloatArr0[4] = 1272805315.1;
  FloatArr0[0] = 513849958.1;
  FloatArr0[7] = 939987269;
  FloatArr0[5] = 1935059981;
  FloatArr0[10] = -1661840040.9;
  FloatArr0[3] = 225101357;
  protoObj0.prop5={prop0: SIMD.Float32x4.extractLane(SIMD.Float32x4.add(SIMD.Float32x4.fromInt32x4Bits(SIMD.Int32x4(180220209,1844702018,-217482056,-35)),SIMD.Float32x4.fromInt32x4Bits(SIMD.Int32x4(240,-170,-919987614,-54))),3), prop1: SIMD.Bool8x16.allTrue(SIMD.Uint8x16.lessThanOrEqual(initSimdVar6_Uint8x16,initSimdVar6_Uint8x16_alias)), prop2: (++ this.prop0)};
  var simdVar6_Float32x4 = SIMD.Float32x4.reciprocalSqrtApproximation(initSimdVar0_Float32x4,initSimdVar0_Float32x4);
  function func5 () {
    this.prop0 = SIMD.Uint8x16.extractLane(SIMD.Uint8x16.fromInt16x8Bits(SIMD.Int16x8(26841,5698,14561,3440,26051,27221,13477,12076)),1);
  }
  obj7 = new func5();
  if (shouldBailout) {
    (shouldBailout ? (Object.defineProperty(obj7, 'prop0', {set: function(_x) {     obj1.prop0 = SIMD.Bool8x16.extractLane(initSimdVar9_Bool8x16,2);
    }, configurable: true }), SIMD.Uint8x16.extractLane(SIMD.Uint8x16.mul(initSimdVar6_Uint8x16,initSimdVar6_Uint8x16_alias),1)) : SIMD.Uint8x16.extractLane(SIMD.Uint8x16.mul(initSimdVar6_Uint8x16,initSimdVar6_Uint8x16_alias),1));
  }
  obj0.prop0 = i32[(16) & 255];
  var simdVar7_Float32x4 = SIMD.Float32x4.max(initSimdVar0_Float32x4,initSimdVar0_Float32x4);
  func3.call(litObj0 , /[a7]/gim, /(?=\s\b\w)$/i);
  var __loopvar0 = loopInvariant,__loopSecondaryVar0_0 = loopInvariant,__loopSecondaryVar0_1 = loopInvariant;
  LABEL0: 
  for (var _strvar29 in IntArr1) {
    if(typeof _strvar29 === 'string' && _strvar29.indexOf('method') != -1) continue;
    __loopvar0 += 4;
    if (__loopvar0 == loopInvariant + 16) break;
    __loopSecondaryVar0_0 -= 4;
    __loopSecondaryVar0_1++;
    IntArr1[_strvar29] = (+ SIMD.Int8x16.extractLane(initSimdVar3_Int8x16,12));
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
  WScript.Echo('protoObj0.prop5.prop0 = ' + (protoObj0.prop5.prop0|0));
  WScript.Echo('protoObj0.prop5.prop1 = ' + (protoObj0.prop5.prop1|0));
  WScript.Echo('protoObj0.prop5.prop2 = ' + (protoObj0.prop5.prop2|0));
  WScript.Echo('obj7.prop0 = ' + (obj7.prop0|0));
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
  WScript.Echo('simdVar6_Float32x4 = ' + (simdVar6_Float32x4.toString()));
  WScript.Echo('simdVar7_Float32x4 = ' + (simdVar7_Float32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4 = ' + (initSimdVar1_Int32x4.toString()));
  WScript.Echo('initSimdVar2_Int16x8 = ' + (initSimdVar2_Int16x8.toString()));
  WScript.Echo('initSimdVar3_Int8x16 = ' + (initSimdVar3_Int8x16.toString()));
  WScript.Echo('initSimdVar4_Uint32x4 = ' + (initSimdVar4_Uint32x4.toString()));
  WScript.Echo('initSimdVar5_Uint16x8 = ' + (initSimdVar5_Uint16x8.toString()));
  WScript.Echo('initSimdVar6_Uint8x16 = ' + (initSimdVar6_Uint8x16.toString()));
  WScript.Echo('initSimdVar6_Uint8x16_alias = ' + (initSimdVar6_Uint8x16_alias.toString()));
  WScript.Echo('initSimdVar7_Bool32x4 = ' + (initSimdVar7_Bool32x4.toString()));
  WScript.Echo('initSimdVar7_Bool32x4_alias = ' + (initSimdVar7_Bool32x4_alias.toString()));
  WScript.Echo('initSimdVar8_Bool16x8 = ' + (initSimdVar8_Bool16x8.toString()));
  WScript.Echo('initSimdVar9_Bool8x16 = ' + (initSimdVar9_Bool8x16.toString()));
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
  WScript.Echo('sumOfaliasOfVarArr0 = ' +  aliasOfVarArr0.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  WScript.Echo('subset_of_aliasOfVarArr0 = ' +  aliasOfVarArr0.slice(0, 11));;
};

// generate profile
test0();
// Run Simple JIT
test0();
test0();
test0();

// run JITted code
runningJITtedCode = true;
test0();


// Baseline total processor time: 00:00:00.4531250
// Test total processor time: 00:00:01
// 
// Addl Dump Info: 
//Baseline length = 16375

//Test length = 13800

//Baseline length = 16375

//Test length = 13800

//Baseline length = 16375

//Test length = 13800

// 
// Baseline output:
// Skipping first 426 lines of output...
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(44734, 49699, 34142, 211, 35688, 24976, 8865, 44822)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(160, 255, 95, 45, 69, 255, 171, 236, 100, 22, 111, 166, 181, 4, 84, 8)
// initSimdVar6_Uint8x16_alias = SIMD.Uint8x16(160, 255, 95, 45, 69, 255, 171, 236, 100, 22, 111, 166, 181, 4, 84, 8)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(false, false, true, false)
// initSimdVar7_Bool32x4_alias = SIMD.Bool32x4(false, false, true, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(false, true, true, false, true, true, true, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(false, false, true, true, true, true, true, true, false, false, false, false, false, true, false, false)
// sumOfary = 1668888666
// subset_of_ary = -756262382,879308470.10000000,-210,65536,127962566,-353433090,-991863055,1471842196.10000000,-13,-651771025.90000000,1746701876
// sumOfIntArr0 = 1614608435
// subset_of_IntArr0 = 139,2066393537,-190,-1,-166,-451785166
// sumOfIntArr1 = -745473027
// subset_of_IntArr1 = 12,12,12,1319610150,3,1553152899,849744717,-784618271,216,-1633911908,-430485707
// sumOfFloatArr0 = 1766572878
// subset_of_FloatArr0 = 513849958.10000000,-1504765384,-496168303,225101357,1272805315.10000000,1935059981,822778065.10000000,939987269,1,48079027,-1661840040.90000000
// sumOfVarArr0 = 0
// subset_of_VarArr0 = 
// sumOfaliasOfVarArr0 = 0
// subset_of_aliasOfVarArr0 = 
// 
// 
// Test output:
// Skipping first 365 lines of output...
// this.prop1 = 248
// obj0.prop0 = 0
// obj0.prop1 = -1137584896
// obj0.length = 100
// protoObj0.prop0 = -1496248022
// protoObj0.prop1 = -957744856
// protoObj0.length = 100
// obj1.prop0 = 9
// obj1.prop1 = 3
// obj1.length = 100
// protoObj1.prop0 = 0
// protoObj1.prop1 = 247
// protoObj1.length = 48814
// arrObj0.prop0 = -8332099
// arrObj0.prop1 = 650492937
// arrObj0.length = 100
// ASSERTION 31260: (e:\nagy\git\chakra5\core\lib\runtime\Types/RecyclableObject.inl, line 29) Ensure instance is a RecyclableObject
//  Failure: (Is(aValue))
// FATAL ERROR: jshost.exe failed due to exception code c0000420
// 
// Reduced Switches:
