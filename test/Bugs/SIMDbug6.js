//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Configuration: configs\SIMD.xml
//Testcase Number: 289
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport  -force:ScriptFunctionWithInlineCache -force:rejit -force:fixdataprops -force:atom -off:aggressiveinttypespec -off:DelayCapture -off:BoundCheckElimination -off:lossyinttypespec -off:ArrayCheckHoist -off:fefixedmethods -off:trackintusage -off:ParallelParse -off:BoundCheckHoist -ForceArrayBTree
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
  var loopInvariant = shouldBailout ? 11 : 3;
  var GiantPrintArray = [];
  __counter++;;
  function makeArrayLength(x) { if(x < 1 || x > 4294967295 || x != x || isNaN(x) || !isFinite(x)) return 100; else return Math.floor(x) & 0xffff; };;
  function leaf() { return 100; };
  var obj0 = {};
  var protoObj0 = {};
  var obj1 = {};
  var arrObj0 = {};
  var litObj0 = {prop1: 3.14159265358979};
  var litObj1 = {prop0: 0, prop1: 1};
  var arrObj0 = {};
  var initSimdVar0_Float32x4 = SIMD.Float32x4(-75,3.94664310149867E+18,-215,-244);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(1027520433,-192,-1045327576,437543230);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(16406,20174,20886,17683,12110,31182,15884,785);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(66,29,2,10,55,95,3,71,76,19,41,51,4,18,98,9);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(1166206409,1276935115,2929198808,3863396418);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(55524,36225,54827,36518,8933,60060,-220,42508);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(68,179,22,72,77,-231,103,5,35,19,211,83,70,238,151,131);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(false,true,true,true);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(false,true,false,true,true,false,false,false);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(true,true,false,false,false,true,true,false,false,true,true,true,true,false,true,false);
  var func0 = function(){
    return (protoObj0.prop0 * (SIMD.Int8x16.extractLane(initSimdVar3_Int8x16,5) - -2147483649));
  };
  var func1 = function(argMath0){
    if(shouldBailout){
      return  'somestring'
    }
    var simdVar0_Bool16x8 = SIMD.Bool16x8.check(initSimdVar8_Bool16x8);
    var simdVar1_Uint32x4 = SIMD.Uint32x4.shuffle(initSimdVar4_Uint32x4,SIMD.Uint32x4.shiftLeftByScalar(SIMD.Uint32x4.load2(i32,307267316 % (i32.length - (128 / i32.BYTES_PER_ELEMENT / 8 ))),3),0,0,3,0);
    return i32[(9) & 255];
  };
  var func2 = function(argMath1,argMath2,argMath3){
    func0();
    var simdVar2_Uint16x8 = SIMD.Uint16x8.subSaturate(SIMD.Uint16x8.fromUint32x4Bits(SIMD.Uint32x4(2346215372,3401970764,538110511,2398978728)),initSimdVar5_Uint16x8);
    var __loopvar3 = loopInvariant;
    do {
      __loopvar3 += 2;
      if (__loopvar3 >= loopInvariant + 6) break;
      if(shouldBailout){
        return  'somestring'
      }
      if(shouldBailout){
        return  'somestring'
      }
      if(shouldBailout){
        return  'somestring'
      }
      break ;
      protoObj0.length= makeArrayLength(SIMD.Uint16x8.extractLane(SIMD.Uint16x8.fromInt32x4Bits(SIMD.Int32x4(319272245,-1515222578,396367661,746954471)),4));
    } while((SIMD.Uint8x16.extractLane(SIMD.Uint8x16.addSaturate(initSimdVar6_Uint8x16,SIMD.Uint8x16.mul(SIMD.Uint8x16.fromInt32x4Bits(SIMD.Int32x4(-1123141860,0,19,1479583554)),initSimdVar6_Uint8x16_alias)),1)))
    return SIMD.Uint32x4.extractLane(initSimdVar4_Uint32x4,0);
  };
  var func3 = function(){
    if(shouldBailout){
      return  'somestring'
    }
    litObj1 = protoObj0;
    var simdVar3_Float32x4 = SIMD.Float32x4.reciprocalApproximation(initSimdVar0_Float32x4,initSimdVar0_Float32x4);
    return ((SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,6) ? SIMD.Int32x4.extractLane(initSimdVar1_Int32x4,1) : (test0.caller)) >= (! SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,3)));
  };
  var func4 = function(){
    var simdVar4_Float32x4 = SIMD.Float32x4.reciprocalApproximation(initSimdVar0_Float32x4,initSimdVar0_Float32x4);
    var simdVar5_Int32x4 = SIMD.Int32x4.load3(ui16,1931594024 % (ui16.length - (128 / ui16.BYTES_PER_ELEMENT / 8 )));
    return (SIMD.Bool16x8.anyTrue(SIMD.Bool16x8.not(initSimdVar8_Bool16x8,SIMD.Uint16x8.lessThan(initSimdVar5_Uint16x8_alias,SIMD.Uint16x8.shiftLeftByScalar(SIMD.Uint16x8.and(initSimdVar5_Uint16x8,initSimdVar5_Uint16x8),0)))) < (((function () {;}) instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)) && ((e = (typeof(SIMD.Uint32x4.extractLane(SIMD.Uint32x4.load1(f32,312616037 % (f32.length - (128 / f32.BYTES_PER_ELEMENT / 8 ))),0))  != null) ) ? ((function () {;}) instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)) : d)));
  };
  obj0.method0 = func1;
  obj0.method1 = func1;
  obj1.method0 = obj0.method0;
  obj1.method1 = func2;
  arrObj0.method0 = obj1.method0;
  arrObj0.method1 = obj1.method1;
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
  var IntArr0 = new Array(-157,6079861113276905472,206,2147483647,-239,-768143001,368461340,-462555199,231093359,-2024701372,67);
  var IntArr1 = new Array(-2147483647,-3,65536,7068889641523933184,-1330498047988726784,994010442,-3273511637595628544,-93,616349317,62,2,-509750045,-446088646);
  var FloatArr0 = new Array();
  var VarArr0 = new Array(litObj1,-8205539574532737024,1138742293,-828650736,1,-44,8.25499393910184E+18,163646929.1,-2147483646,-1,-402974496,217,-1055316335);
  var a = -960756127;
  var b = -594542576;
  var c = 27;
  var d = -174979717.9;
  var e = 338376718.1;
  var f = -387341259;
  var g = 106;
  var h = -435223910;
  arrObj0[0] = 65535;
  arrObj0[1] = 136;
  arrObj0[2] = 378586042.1;
  arrObj0[3] = 70;
  arrObj0[4] = 520719247;
  arrObj0[5] = -1227816350.9;
  arrObj0[6] = 320652498.1;
  arrObj0[7] = 718083965;
  arrObj0[8] = -2147483649;
  arrObj0[9] = 1073741823;
  arrObj0[10] = -157;
  arrObj0[11] = 944261979;
  arrObj0[12] = -83;
  arrObj0[13] = 687630803;
  arrObj0[14] = 1923374948;
  arrObj0[arrObj0.length-1] = 1050936606;
  arrObj0.length = makeArrayLength(650280558);
  ary[0] = -4.53110563574008E+16;
  ary[1] = 45;
  ary[2] = 197;
  ary[3] = -1835769380.9;
  ary[4] = -969570426.9;
  ary[5] = -7.4474367143882E+18;
  ary[6] = 65537;
  ary[7] = 897992600;
  ary[8] = -2.73694540066112E+18;
  ary[9] = 1073741823;
  ary[10] = -3.23104585626568E+18;
  ary[11] = 2;
  ary[12] = -662461842;
  ary[13] = 2147483650;
  ary[14] = 37;
  ary[ary.length-1] = -130252722;
  ary.length = makeArrayLength(2109423230);
  var aliasOfobj0 = obj0;;
  protoObj0 = Object.create(obj0);
  var initSimdVar2_Int16x8_alias = initSimdVar2_Int16x8;;
  var initSimdVar5_Uint16x8_alias = initSimdVar5_Uint16x8;;
  var initSimdVar6_Uint8x16_alias = initSimdVar6_Uint8x16;;
  var aliasOfFloatArr0 = FloatArr0;;
  this.prop0 = -1868277617.9;
  this.prop1 = 6.37995271999948E+18;
  obj0.prop0 = -831351519;
  obj0.prop1 = -1879598310.9;
  obj0.length = makeArrayLength(4.88530553829776E+18);
  aliasOfobj0.prop0 = 36460905;
  aliasOfobj0.prop1 = 97;
  aliasOfobj0.length = makeArrayLength(289467747);
  protoObj0.prop0 = 197;
  protoObj0.prop1 = 1545718456.1;
  protoObj0.length = makeArrayLength(2147483650);
  obj1.prop0 = 8.93087238294598E+18;
  obj1.prop1 = 248759632;
  obj1.length = makeArrayLength(8.41925188018692E+18);
  arrObj0.prop0 = 1870469669;
  arrObj0.prop1 = -3.13108045890483E+18;
  arrObj0.length = makeArrayLength(215);
  FloatArr0[2] = 2;
  FloatArr0[3] = -2126812343;
  FloatArr0[0] = 536664542;
  FloatArr0[FloatArr0.length] = -1004625772.9;
  FloatArr0[1] = 1.06676413964947E+18;
  var simdVar6_Uint8x16 = SIMD.Uint8x16.add(SIMD.Uint8x16.fromInt32x4Bits(SIMD.Int32x4(155,-1046600986,-183,82566181)),initSimdVar6_Uint8x16_alias);
  aliasOfobj0.prop5=obj1.method1;
  if((FloatArr0[(11)] <= ({} instanceof ((typeof Array == 'function' ) ? Array : Object)))) {
  }
  else {
    if(((test0.caller) && (arrObj0.length *= SIMD.Int8x16.extractLane(initSimdVar3_Int8x16,3)))) {
      arrObj0.length= makeArrayLength(SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,3));
      var simdVar7_Float32x4 = SIMD.Float32x4.reciprocalSqrtApproximation(initSimdVar0_Float32x4,SIMD.Float32x4.fromInt8x16Bits(SIMD.Int8x16(93,39,58,115,109,96,112,40,2,61,54,71,68,56,117,57)));
      var simdVar8_Int16x8 = SIMD.Int16x8.or(SIMD.Int16x8.select(SIMD.Bool16x8.and(SIMD.Int16x8.equal(initSimdVar2_Int16x8,initSimdVar2_Int16x8_alias),initSimdVar8_Bool16x8),SIMD.Int16x8.check(SIMD.Int16x8.check(SIMD.Int16x8.add(initSimdVar2_Int16x8_alias,initSimdVar2_Int16x8))),SIMD.Int16x8.addSaturate(initSimdVar2_Int16x8_alias,initSimdVar2_Int16x8)),initSimdVar2_Int16x8);
      var simdVar9_Int8x16 = SIMD.Int8x16.not(initSimdVar3_Int8x16,SIMD.Int8x16.load(f32,947419746 % (f32.length - (128 / f32.BYTES_PER_ELEMENT / 8 ))));
      var simdVar10_Bool16x8 = SIMD.Uint16x8.lessThanOrEqual(initSimdVar5_Uint16x8_alias,SIMD.Uint16x8.addSaturate(initSimdVar5_Uint16x8_alias,initSimdVar5_Uint16x8_alias));
      g /=SIMD.Uint32x4.extractLane(SIMD.Uint32x4.sub(initSimdVar4_Uint32x4,SIMD.Uint32x4.or(initSimdVar4_Uint32x4,initSimdVar4_Uint32x4)),1);
    }
    else {
    }
    var uniqobj0 = [arrObj0, obj1];
    var uniqobj1 = uniqobj0[__counter%uniqobj0.length];
    uniqobj1.method1(aliasOfobj0,leaf,obj0);
    var uniqobj2 = [protoObj0, obj0];
    uniqobj2[__counter%uniqobj2.length].method1(/(?!.)/g);
    function func5 (arg0, arg1) {
      this.prop0 = arg0;
      this.prop2 = arg1;
    }
    obj6 = new func5(SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,2),(SIMD.Int8x16.extractLane(initSimdVar3_Int8x16,3) ? SIMD.Bool8x16.extractLane(SIMD.Bool8x16.check(SIMD.Bool8x16.check(SIMD.Bool8x16.or(initSimdVar9_Bool8x16,initSimdVar9_Bool8x16))),14) : ((new RegExp('xyz')) instanceof ((typeof Object == 'function' ) ? Object : Object))));
    var simdVar11_Float32x4 = SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(1909427210,2042027406,109426520,2050190551));
    var simdVar12_Float32x4 = SIMD.Float32x4.min(SIMD.Float32x4.reciprocalApproximation(SIMD.Float32x4.reciprocalApproximation(simdVar11_Float32x4,SIMD.Float32x4.fromUint16x8Bits(SIMD.Uint16x8(25274,58434,24927,14824,38838,22710,62612,29343))),SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(-669766637,-798080608,46,-137))),simdVar11_Float32x4);
  }
  var simdVar13_Int16x8 = SIMD.Int16x8.shiftLeftByScalar(SIMD.Int16x8.fromUint16x8Bits(SIMD.Uint16x8(21530,25197,60321,24721,61786,65142,8235,13201)),(FloatArr0[(11)] <= ({} instanceof ((typeof Array == 'function' ) ? Array : Object))));
  var simdVar14_Float32x4 = SIMD.Float32x4.max(initSimdVar0_Float32x4,SIMD.Float32x4.max(initSimdVar0_Float32x4,SIMD.Float32x4.reciprocalApproximation(initSimdVar0_Float32x4,SIMD.Float32x4.splat(-493827965))));
  var simdVar15_Float32x4 = SIMD.Float32x4.max(SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(659455299,-308484040,-195,-401432026)),initSimdVar0_Float32x4);
  litObj1.prop0=(SIMD.Bool8x16.anyTrue(SIMD.Bool8x16.and(SIMD.Bool8x16.replaceLane(initSimdVar9_Bool8x16,10,false),SIMD.Uint8x16.equal(simdVar6_Uint8x16,SIMD.Uint8x16.swizzle(simdVar6_Uint8x16,4,13,1,12,10,13,6,15,10,1,5,2,0,15,0,5)))) ? ({} instanceof ((typeof Array == 'function' ) ? Array : Object)) : ((test0.caller) % (aliasOfFloatArr0[(13)] > (null instanceof ((typeof Array == 'function' ) ? Array : Object)))));
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
  WScript.Echo('aliasOfobj0.prop0 = ' + (aliasOfobj0.prop0|0));
  WScript.Echo('aliasOfobj0.prop1 = ' + (aliasOfobj0.prop1|0));
  WScript.Echo('aliasOfobj0.length = ' + (aliasOfobj0.length|0));
  WScript.Echo('protoObj0.prop0 = ' + (protoObj0.prop0|0));
  WScript.Echo('protoObj0.prop1 = ' + (protoObj0.prop1|0));
  WScript.Echo('protoObj0.length = ' + (protoObj0.length|0));
  WScript.Echo('obj1.prop0 = ' + (obj1.prop0|0));
  WScript.Echo('obj1.prop1 = ' + (obj1.prop1|0));
  WScript.Echo('obj1.length = ' + (obj1.length|0));
  WScript.Echo('arrObj0.prop0 = ' + (arrObj0.prop0|0));
  WScript.Echo('arrObj0.prop1 = ' + (arrObj0.prop1|0));
  WScript.Echo('arrObj0.length = ' + (arrObj0.length|0));
  WScript.Echo('litObj1.prop0 = ' + (litObj1.prop0|0));
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
  WScript.Echo('simdVar14_Float32x4 = ' + (simdVar14_Float32x4.toString()));
  WScript.Echo('simdVar15_Float32x4 = ' + (simdVar15_Float32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4 = ' + (initSimdVar1_Int32x4.toString()));
  WScript.Echo('initSimdVar2_Int16x8 = ' + (initSimdVar2_Int16x8.toString()));
  WScript.Echo('initSimdVar2_Int16x8_alias = ' + (initSimdVar2_Int16x8_alias.toString()));
  WScript.Echo('simdVar13_Int16x8 = ' + (simdVar13_Int16x8.toString()));
  WScript.Echo('initSimdVar3_Int8x16 = ' + (initSimdVar3_Int8x16.toString()));
  WScript.Echo('initSimdVar4_Uint32x4 = ' + (initSimdVar4_Uint32x4.toString()));
  WScript.Echo('initSimdVar5_Uint16x8 = ' + (initSimdVar5_Uint16x8.toString()));
  WScript.Echo('initSimdVar5_Uint16x8_alias = ' + (initSimdVar5_Uint16x8_alias.toString()));
  WScript.Echo('initSimdVar6_Uint8x16 = ' + (initSimdVar6_Uint8x16.toString()));
  WScript.Echo('initSimdVar6_Uint8x16_alias = ' + (initSimdVar6_Uint8x16_alias.toString()));
  WScript.Echo('simdVar6_Uint8x16 = ' + (simdVar6_Uint8x16.toString()));
  WScript.Echo('initSimdVar7_Bool32x4 = ' + (initSimdVar7_Bool32x4.toString()));
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
  WScript.Echo('sumOfaliasOfFloatArr0 = ' +  aliasOfFloatArr0.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  WScript.Echo('subset_of_aliasOfFloatArr0 = ' +  aliasOfFloatArr0.slice(0, 11));;
};

// generate profile
test0();
// Run Simple JIT
test0();
test0();

// run JITted code
runningJITtedCode = true;
test0();


// Baseline total processor time: 00:00:00.3281250
// Test total processor time: 00:00:00.5312500
// 
// Addl Dump Info: 
//Baseline length = 14944

//Test length = 11456

//Baseline length = 14944

//Test length = 11456

//Baseline length = 14944

//Test length = 11456

// 
// Baseline output:
// Skipping first 337 lines of output...
// initSimdVar5_Uint16x8_alias = SIMD.Uint16x8(55524, 36225, 54827, 36518, 8933, 60060, 65316, 42508)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(68, 179, 22, 72, 77, 25, 103, 5, 35, 19, 211, 83, 70, 238, 151, 131)
// initSimdVar6_Uint8x16_alias = SIMD.Uint8x16(68, 179, 22, 72, 77, 25, 103, 5, 35, 19, 211, 83, 70, 238, 151, 131)
// simdVar6_Uint8x16 = SIMD.Uint8x16(223, 179, 22, 72, 51, 59, 5, 198, 108, 18, 210, 82, 107, 202, 130, 135)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(false, true, true, true)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(false, true, false, true, true, false, false, false)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, true, false, false, false, true, true, false, false, true, true, true, true, false, true, false)
// sumOfary = -892435882
// subset_of_ary = -1809225012,45,197,-1835769380.90000000,-969570426.90000000,-386192505,65537,897992600,-335724207,1073741823,-1030007341
// sumOfIntArr0 = 1594344177
// subset_of_IntArr0 = -157,2102704881,206,0,-239,-768143001,368461340,-462555199,231093359,-2024701372,67
// sumOfIntArr1 = 389056133
// subset_of_IntArr1 = 0,-3,65536,1806461787,-161856986,994010442,-1910134523,-93,616349317,62,2
// sumOfFloatArr0 = 1172391081
// subset_of_FloatArr0 = 536664542,1619680938,2,-2126812343,-1004625772.90000000
// sumOfVarArr0 = 0[object Object]-1146390458-1844056420-1100274896-2147483646-1-1395054228-1055316335
// subset_of_VarArr0 = [object Object],-237954443,1138742293,-828650736,1,-44,407516802,163646929.10000000,-2147483646,-1,-402974496
// sumOfaliasOfFloatArr0 = 1172391081
// subset_of_aliasOfFloatArr0 = 536664542,1619680938,2,-2126812343,-1004625772.90000000
// 
// 
// Test output:
// Skipping first 251 lines of output...
// simdVar6_Uint8x16 = SIMD.Uint8x16(223, 179, 22, 72, 51, 59, 5, 198, 108, 18, 210, 82, 107, 202, 130, 135)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(false, true, true, true)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(false, true, false, true, true, false, false, false)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, true, false, false, false, true, true, false, false, true, true, true, true, false, true, false)
// sumOfary = -892435882
// subset_of_ary = -1809225012,45,197,-1835769380.90000000,-969570426.90000000,-386192505,65537,897992600,-335724207,1073741823,-1030007341
// sumOfIntArr0 = 1594344177
// subset_of_IntArr0 = -157,2102704881,206,0,-239,-768143001,368461340,-462555199,231093359,-2024701372,67
// sumOfIntArr1 = 389056133
// subset_of_IntArr1 = 0,-3,65536,1806461787,-161856986,994010442,-1910134523,-93,616349317,62,2
// sumOfFloatArr0 = 1172391081
// subset_of_FloatArr0 = 536664542,1619680938,2,-2126812343,-1004625772.90000000
// sumOfVarArr0 = 0[object Object]-1146390458-1844056420-1100274896-2147483646-1-1395054228-1055316335
// subset_of_VarArr0 = [object Object],-237954443,1138742293,-828650736,1,-44,407516802,163646929.10000000,-2147483646,-1,-402974496
// sumOfaliasOfFloatArr0 = 1172391081
// subset_of_aliasOfFloatArr0 = 536664542,1619680938,2,-2126812343,-1004625772.90000000
// ASSERTION 19816: (e:\nagy\git\chakra5\core\lib\Backend\amd64\EncoderMD.cpp, line 355) Expected stackSym offset to be set.
//  Failure: (opnd->AsSymOpnd()->m_sym->AsStackSym()->m_offset)
// FATAL ERROR: jshost.exe failed due to exception code c0000420
// 
// Reduced Switches:
