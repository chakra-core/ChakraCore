//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Configuration: configs\SIMD.xml
//Testcase Number: 767
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport  -MaxLinearStringCaseCount:2 -MaxLinearIntCaseCount:2 -bgjit- -loopinterpretcount:1 -MinSwitchJumpTableSize:3 -forceserialized -force:fieldhoist -off:polymorphicinlinecache -force:fieldcopyprop -sse:3 -force:interpreterautoprofile -force:inline -ExpirableCollectionTriggerThreshold:1 -ExpirableCollectionGCCount:1 -ForceExpireOnNonCacheCollect -force:atom -force:rejit -force:ScriptFunctionWithInlineCache -off:lossyinttypespec -force:fixdataprops -off:bailonnoprofile -off:trackintusage -off:ArrayCheckHoist -off:ParallelParse -off:checkthis -ForceArrayBTree -off:aggressiveinttypespec -off:DelayCapture -off:fefixedmethods -ValidateInlineStack -off:LoopCountBasedBoundCheckHoist -off:NativeArray -off:ArrayLengthHoist -off:BoundCheckElimination -off:BoundCheckHoist -off:JsArraySegmentHoist -off:EliminateArrayAccessHelperCall
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
  var loopInvariant = shouldBailout ? 8 : 0;
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
  var initSimdVar0_Float32x4 = SIMD.Float32x4(195,-5.30416727717133E+18,1073741823,2147483650);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(1046316903,-526740040,-49,-703219856);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(13214,1877,3717,14267,7750,28282,-182,13933);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(102,52,32,17,46,30,66,5,126,101,1,60,15,54,67,61);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(2560750732,869395544,57857164,2191110554);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(17189,54150,2033879522,21561,38581,363,57159,43717);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(43,200,193,228,78,555916606,35,0,193,164,162,119,254,43,17,671756426);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(true,false,true,false);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(false,true,false,true,true,false,false,true);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(true,-235060264,false,true,true,false,true,false,true,false,false,false,true,true,119,true);
  var func0 = function(){
    return (SIMD.Float32x4.extractLane(SIMD.Float32x4.min(SIMD.Float32x4.splat(),initSimdVar0_Float32x4),1) && -1);
  };
  var func1 = function(argMath0,argMath1,argMath2,argMath3){
    var __loopvar3 = loopInvariant - 9,__loopSecondaryVar3_0 = loopInvariant;
    LABEL0: 
    while((SIMD.Bool8x16.anyTrue(initSimdVar9_Bool8x16)) && ((__loopvar3 <= loopInvariant))) {
      __loopvar3 += 3;
      __loopSecondaryVar3_0 += 2;
      if(shouldBailout){
        return  'somestring'
      }
    }
    var __loopvar3 = loopInvariant - 6,__loopSecondaryVar3_0 = loopInvariant,__loopSecondaryVar3_1 = loopInvariant - 3;
    LABEL0: 
    LABEL1: 
    for (var _strvar4 in ary) {
      if(typeof _strvar4 === 'string' && _strvar4.indexOf('method') != -1) continue;
      __loopvar3 += 2;
      __loopSecondaryVar3_1++;
      if (__loopvar3 === loopInvariant + 2) break;
      if (__loopSecondaryVar3_1 >= loopInvariant) break;
      __loopSecondaryVar3_0++;
      ary[_strvar4] = SIMD.Bool8x16.allTrue(initSimdVar9_Bool8x16);
      return 4294967296;
    }
    protoObj0.prop5={82: 122481634, prop0: argMath1, prop2: (arrObj0.prop1 = (typeof(this.prop1)  != 'boolean') ), prop3: SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,0), prop4: (argMath2 && (ary.reverse()))};
    return SIMD.Int8x16.extractLane(SIMD.Int8x16.mul(SIMD.Int8x16.shuffle(initSimdVar3_Int8x16,initSimdVar3_Int8x16,9,26,16,23,16,29,10,20,22,1,30,18,8,12,4,23),initSimdVar3_Int8x16),2);
  };
  var func2 = function(){
    return SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,3);
  };
  var func3 = function(){
    return SIMD.Float32x4.extractLane(SIMD.Float32x4.shuffle(initSimdVar0_Float32x4,SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(173032756,3619792060,2144095121,80330805)),6,0,2,1),1);
  };
  var func4 = function(){
    if(shouldBailout){
      return  'somestring'
    }
    return f64[(213) & 255];
  };
  obj0.method0 = func2;
  obj0.method1 = func0;
  obj1.method0 = func1;
  obj1.method1 = obj0.method0;
  arrObj0.method0 = obj0.method0;
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
  var IntArr0 = [];
  var IntArr1 = new Array(-1524572126556133376,650793050,4294967297,815491204,2147483650,20,226,-1046436826,-5902072442624884736,101,-2);
  var FloatArr0 = [];
  var VarArr0 = new Array(aliasOfarrObj0,1631311672090513664,1051363976,101,241,3.04586304641605E+18,-86595182,594419839,-999289758);
  var a = -2147483648;
  var b = 1917300851;
  var c = -2.9319827959429E+18;
  var d = 6.96235056656659E+18;
  var e = 1000259416.1;
  var f = 156911272.1;
  var g = 3.60101398306777E+18;
  var h = 1734649631.1;
  arrObj0[0] = 29578940;
  arrObj0[1] = -7.62239868434692E+18;
  arrObj0[2] = 1253625864;
  arrObj0[3] = 1410357277.1;
  arrObj0[4] = 1053830589;
  arrObj0[5] = -204;
  arrObj0[6] = 926690971.1;
  arrObj0[7] = -566820446;
  arrObj0[8] = 1163731160;
  arrObj0[9] = 2.34238339076286E+18;
  arrObj0[10] = 2147483647;
  arrObj0[11] = -132;
  arrObj0[12] = 207;
  arrObj0[13] = -2.08098353773813E+18;
  arrObj0[14] = 598099498;
  arrObj0[arrObj0.length-1] = -332549618;
  arrObj0.length = makeArrayLength(-226);
  ary[0] = 1745653911.1;
  ary[1] = -1463402632.9;
  ary[2] = 30;
  ary[3] = 11782556;
  ary[4] = 1476930632;
  ary[5] = 65537;
  ary[6] = 138;
  ary[7] = 574363580;
  ary[8] = 1748450616;
  ary[9] = 1069946847;
  ary[10] = -472448596;
  ary[11] = -25040210;
  ary[12] = -90;
  ary[13] = 4.55284121846351E+18;
  ary[14] = -901397106;
  ary[ary.length-1] = 4.61115264120437E+18;
  ary.length = makeArrayLength(7.26772538237543E+18);
  protoObj0 = Object.create(obj0);
  protoObj1 = Object.create(obj1);
  var aliasOfarrObj0 = arrObj0;;
  var initSimdVar1_Int32x4_alias = initSimdVar1_Int32x4;;
  var initSimdVar8_Bool16x8_alias = initSimdVar8_Bool16x8;;
  this.prop0 = -592495492;
  this.prop1 = -48769218;
  obj0.prop0 = 105;
  obj0.prop1 = -200;
  obj0.length = makeArrayLength(127110786);
  protoObj0.prop0 = 8;
  protoObj0.prop1 = 2005744449.1;
  protoObj0.length = makeArrayLength(4294967297);
  obj1.prop0 = 4.29124849090908E+17;
  obj1.prop1 = -780456264;
  obj1.length = makeArrayLength(-3);
  protoObj1.prop0 = -113;
  protoObj1.prop1 = 157000481;
  protoObj1.length = makeArrayLength(327655762);
  arrObj0.prop0 = -563950144;
  arrObj0.prop1 = 65535;
  arrObj0.length = makeArrayLength(21790119.1);
  aliasOfarrObj0.prop0 = 1949659917;
  aliasOfarrObj0.prop1 = 5.68589764278741E+18;
  aliasOfarrObj0.length = makeArrayLength(155);
  IntArr0[1] = 129;
  IntArr0[2] = -1500876622.9;
  IntArr0[0] = -7.65045305166924E+18;
  VarArr0[9] = -1728294235;
  VarArr0[10] = -2;
  var simdVar0_Uint8x16 = SIMD.Uint8x16.neg(initSimdVar6_Uint8x16);
  var simdVar1_Uint16x8 = SIMD.Uint16x8.fromFloat32x4Bits(SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(-628525035,-58,-600610188,-871769747)));
  var __loopvar0 = loopInvariant,__loopSecondaryVar0_0 = loopInvariant;
  for(; e < (SIMD.Uint32x4.extractLane(SIMD.Uint32x4.add(initSimdVar4_Uint32x4,SIMD.Uint32x4.fromUint16x8Bits(SIMD.Uint16x8(12374,62229,61241,20365,56596,58648,43292,1123))),3)); SIMD.Int32x4.extractLane(SIMD.Int32x4.fromUint16x8Bits(SIMD.Uint16x8(53332,41728,24200,57047,2240,58688,48974,31469)),3)) {
    if (__loopvar0 < loopInvariant - 3) break;
    __loopvar0--;
    __loopSecondaryVar0_0 += 2;
    protoObj0.prop0 /=-1.10751791721119E+18;
    var simdVar2_Bool = SIMD.Bool8x16.allTrue(SIMD.Int8x16.lessThanOrEqual(initSimdVar3_Int8x16,initSimdVar3_Int8x16));
    var simdVar3_Float32x4 = SIMD.Float32x4.minNum(SIMD.Float32x4.check(SIMD.Float32x4.sub(initSimdVar0_Float32x4,SIMD.Float32x4.load3(f64,411601950 % (f64.length - (128 / f64.BYTES_PER_ELEMENT / 8 ))))),SIMD.Float32x4.max(initSimdVar0_Float32x4,initSimdVar0_Float32x4));
    var uniqobj0 = [obj1, protoObj1];
    uniqobj0[__counter%uniqobj0.length].method0(/.?/gm,SIMD.Int32x4.extractLane(SIMD.Int32x4.fromUint16x8Bits(SIMD.Uint16x8(53332,41728,24200,57047,2240,58688,48974,31469)),3),arrObj0,/.?/gm);
  }
  protoObj0.prop5=Math.round(SIMD.Float32x4.extractLane(SIMD.Float32x4.load3(i16,2092801197 % (i16.length - (128 / i16.BYTES_PER_ELEMENT / 8 ))),2));
  var simdVar4_Float32x4 = SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(162,-693225695,-746946933,-110));
  f &=(VarArr0.unshift(SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,1), (e !== e), SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,9), SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4), (obj1.length += (IntArr0[(((obj1.method0.call(aliasOfarrObj0 , /.?/gm, obj0.method1.call(obj0 ), protoObj1, /^[a7]$/gim) >= 0 ? obj1.method0.call(aliasOfarrObj0 , /.?/gm, obj0.method1.call(obj0 ), protoObj1, /^[a7]$/gim) : 0)) & 0XF)] instanceof ((typeof Array == 'function' ) ? Array : Object))), SIMD.Bool32x4.anyTrue(SIMD.Bool32x4.xor(initSimdVar7_Bool32x4,initSimdVar7_Bool32x4)), SIMD.Float32x4.extractLane(SIMD.Float32x4.replaceLane(SIMD.Float32x4.div(SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(2941624029,4165479232,780458827,2397744895)),initSimdVar0_Float32x4),1,65535),1), protoObj0.prop5, (typeof(obj0.length)  == 'object') , (790579693.1 < (test0.caller))));
  if(((typeof(obj0.length)  == 'object')  == (Object.create({prop0: parseInt("1073741823", 10)}, {}) * (SIMD.Bool16x8.extractLane(SIMD.Uint16x8.lessThan(SIMD.Uint16x8.shiftRightByScalar(initSimdVar5_Uint16x8,5),initSimdVar5_Uint16x8),1) - (func3.call(litObj1 ) * (f32[(0) & 255] + SIMD.Int32x4.extractLane(SIMD.Int32x4.swizzle(SIMD.Int32x4.load(ui16,345231056 % (ui16.length - (128 / ui16.BYTES_PER_ELEMENT / 8 ))),2,2,1,0),2))))))) {
    var uniqobj1 = [protoObj0, obj1, obj1];
    var uniqobj2 = uniqobj1[__counter%uniqobj1.length];
    uniqobj2.method1();
  }
  else {
    var uniqobj3 = [obj0, arrObj0, protoObj0, protoObj0, arrObj0];
    var uniqobj4 = uniqobj3[__counter%uniqobj3.length];
    uniqobj4.method0();
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
  WScript.Echo('aliasOfarrObj0.prop0 = ' + (aliasOfarrObj0.prop0|0));
  WScript.Echo('aliasOfarrObj0.prop1 = ' + (aliasOfarrObj0.prop1|0));
  WScript.Echo('aliasOfarrObj0.length = ' + (aliasOfarrObj0.length|0));
  WScript.Echo('protoObj0.prop5 = ' + (protoObj0.prop5|0));
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
  WScript.Echo('simdVar4_Float32x4 = ' + (simdVar4_Float32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4 = ' + (initSimdVar1_Int32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4_alias = ' + (initSimdVar1_Int32x4_alias.toString()));
  WScript.Echo('initSimdVar2_Int16x8 = ' + (initSimdVar2_Int16x8.toString()));
  WScript.Echo('initSimdVar3_Int8x16 = ' + (initSimdVar3_Int8x16.toString()));
  WScript.Echo('initSimdVar4_Uint32x4 = ' + (initSimdVar4_Uint32x4.toString()));
  WScript.Echo('initSimdVar5_Uint16x8 = ' + (initSimdVar5_Uint16x8.toString()));
  WScript.Echo('simdVar1_Uint16x8 = ' + (simdVar1_Uint16x8.toString()));
  WScript.Echo('initSimdVar6_Uint8x16 = ' + (initSimdVar6_Uint8x16.toString()));
  WScript.Echo('simdVar0_Uint8x16 = ' + (simdVar0_Uint8x16.toString()));
  WScript.Echo('initSimdVar7_Bool32x4 = ' + (initSimdVar7_Bool32x4.toString()));
  WScript.Echo('initSimdVar8_Bool16x8 = ' + (initSimdVar8_Bool16x8.toString()));
  WScript.Echo('initSimdVar8_Bool16x8_alias = ' + (initSimdVar8_Bool16x8_alias.toString()));
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
};

// generate profile
test0();
// Run Simple JIT
test0();
test0();

// run JITted code
runningJITtedCode = true;
test0();


// Baseline total processor time: 00:00:00.2968750
// Test total processor time: 00:00:00.8125000
// 
// Addl Dump Info: 
//Baseline length = 12880

//Test length = 12880

//Baseline length = 12880

//Test length = 12880

//Baseline length = 12880

//Test length = 12880

// 
// Baseline output:
// Skipping first 333 lines of output...
// initSimdVar4_Uint32x4 = SIMD.Uint32x4(413267085, 869395544, 57857164, 43626907)
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(17189, 54150, 35298, 21561, 38581, 363, 57159, 43717)
// simdVar1_Uint16x8 = SIMD.Uint16x8(55840, 52757, 0, 49768, 12886, 52751, 55450, 52815)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(43, 200, 193, 228, 78, 62, 35, 0, 193, 164, 162, 119, 254, 43, 17, 138)
// simdVar0_Uint8x16 = SIMD.Uint8x16(213, 56, 63, 28, 178, 194, 221, 0, 63, 92, 94, 137, 2, 213, 239, 118)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, false, true, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(false, true, false, true, true, false, false, true)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(false, true, false, true, true, false, false, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, true, false, true, true, false, true, false, true, false, false, false, true, true, true, true)
// sumOfary = 1976373180
// subset_of_ary = false,-1463402632.90000000,30,11782556,1476930632,65537,138,574363580,1748450616,1069946847,-472448596
// sumOfIntArr0 = -1901700362
// subset_of_IntArr0 = -400823562,129,-1500876622.90000000
// sumOfIntArr1 = -1882813380
// subset_of_IntArr1 = -807916683,650793050,3,815491204,3,20,226,-1046436826,-1494744892,101,-2
// sumOfFloatArr0 = 0
// subset_of_FloatArr0 = 
// sumOfVarArr0 = NaN
// subset_of_VarArr0 = 54150,false,164,false,100,false,65535,0,false,false,
// 
// 
// Test output:
// Skipping first 333 lines of output...
// initSimdVar4_Uint32x4 = SIMD.Uint32x4(413267085, 869395544, 57857164, 43626907)
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(17189, 54150, 35298, 21561, 38581, 363, 57159, 43717)
// simdVar1_Uint16x8 = SIMD.Uint16x8(55840, 52757, 0, 49768, 12886, 52751, 55450, 52815)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(43, 200, 193, 228, 78, 62, 35, 0, 193, 164, 162, 119, 254, 43, 17, 138)
// simdVar0_Uint8x16 = SIMD.Uint8x16(213, 56, 63, 28, 178, 194, 221, 0, 63, 92, 94, 137, 2, 213, 239, 118)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, false, true, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(false, true, false, true, true, false, false, true)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(false, true, false, true, true, false, false, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, true, false, true, true, false, true, false, true, false, false, false, true, true, true, true)
// sumOfary = 1976373180
// subset_of_ary = false,-1463402632.90000000,30,11782556,1476930632,65537,138,574363580,1748450616,1069946847,-472448596
// sumOfIntArr0 = -1901700362
// subset_of_IntArr0 = -400823562,129,-1500876622.90000000
// sumOfIntArr1 = -1882813380
// subset_of_IntArr1 = -807916683,650793050,3,815491204,3,20,226,-1046436826,-1494744892,101,-2
// sumOfFloatArr0 = 0
// subset_of_FloatArr0 = 
// sumOfVarArr0 = NaN
// subset_of_VarArr0 = 54150,false,164,false,100,false,65535,0,false,false,
// 
// Reduced Switches:
