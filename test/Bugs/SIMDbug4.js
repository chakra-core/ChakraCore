//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Configuration: configs\SIMD.xml
//Testcase Number: 4583
//Bailout Testing: ON
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:1 -maxsimplejitruncount:1 -werexceptionsupport  -MaxLinearStringCaseCount:2 -MaxLinearIntCaseCount:2 -force:fieldcopyprop -bgjit- -loopinterpretcount:1 -force:fieldhoist -off:polymorphicinlinecache -MinSwitchJumpTableSize:2 -forcedeferparse -force:atom -force:rejit -force:ScriptFunctionWithInlineCache -off:ArrayCheckHoist -ForceArrayBTree -force:fixdataprops -off:fefixedmethods -off:checkthis -off:lossyinttypespec -off:aggressiveinttypespec -off:trackintusage -off:ParallelParse -off:BoundCheckElimination -off:DelayCapture -off:NativeArray -off:ArrayLiteralFastPath -off:LoopCountBasedBoundCheckHoist -off:ArrayLengthHoist -off:bailonnoprofile -off:JsArraySegmentHoist -ValidateInlineStack -off:BoundCheckHoist -off:objtypespec  -stress:BailOnNoProfile  -SkipFuncCountForBailOnNoProfile:5  -off:EliminateArrayAccessHelperCall
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
  var loopInvariant = shouldBailout ? 10 : 1;
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
  var initSimdVar0_Float32x4 = SIMD.Float32x4(-1080561150,1309603417,-932893612,1082239043.1);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(8,-202,-525646322,200);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(25551,26714,22118,18278,13919,7523,13482,24565);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(65,33,88,39,57,38,36,15,19,57,36,42,30,12,68,99);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(2043456508,3236037933,1678129789,3758808827);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(-707321887,53218,45235,39919,18851,26209,48753,39759);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(27,138,207,246,79,81,18,60,184,-180626010,192,130,47,173,-319294184,179);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(-1716548761,true,false,false);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(false,true,false,false,false,true,false,true);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(true,false,false,true,false,false,true,-17,true,true,true,true,true,true,false,true);
  var func0 = function(){
    function func5 () {
      this.prop0 = (shouldBailout ? (Object.defineProperty(protoObj1, 'length', {get: function() { WScript.Echo('protoObj1.length getter'); return 3; }, configurable: true }), (typeof(protoObj1.length)  != 'number') ) : (typeof(protoObj1.length)  != 'number') );
    }
    obj6 = new func5();
    return (test0.caller);
  };
  var func1 = function(argMath0,argMath1,argMath2){
    protoObj0.prop0 = SIMD.Float32x4.extractLane(initSimdVar0_Float32x4_alias,2);
    var simdVar0_Float32x4 = SIMD.Float32x4.min(initSimdVar0_Float32x4_alias,SIMD.Float32x4.swizzle(initSimdVar0_Float32x4,3,1,0,2));
    func0.call(obj1 );
    return SIMD.Float32x4.extractLane(SIMD.Float32x4.fromInt16x8Bits(SIMD.Int16x8(7669,32148,14057,5953,1809,20820,21708,10077)),1);
  };
  var func2 = function(argMath3,argMath4){
    if(shouldBailout){
      return  'somestring'
    }
    argMath4 = argMath4;
    return SIMD.Int8x16.extractLane(initSimdVar3_Int8x16,4);
  };
  var func3 = function(){
    func0();
    var simdVar1_Float32x4 = SIMD.Float32x4.max(initSimdVar0_Float32x4_alias,SIMD.Float32x4.max(initSimdVar0_Float32x4_alias,initSimdVar0_Float32x4));
    return ((SIMD.Uint32x4.extractLane(SIMD.Uint32x4.fromFloat32x4Bits(SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(1451170739,3297011264,518452889,2115593953))),0) ? ((ui8[((typeof(SIMD.Float32x4.extractLane(initSimdVar0_Float32x4_alias,3))  != 'number') ) & 255] instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) * (SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4) + SIMD.Float32x4.extractLane(initSimdVar0_Float32x4_alias,3))) : SIMD.Float32x4.extractLane(SIMD.Float32x4.load(ui8,783166114 % (ui8.length - (128 / ui8.BYTES_PER_ELEMENT / 8 ))),2)) > (typeof(obj1.length)  == 'boolean') );
  };
  var func4 = function(argMath5,argMath6,argMath7){
    return (g = ui16[(SIMD.Uint8x16.extractLane(SIMD.Uint8x16.add(initSimdVar6_Uint8x16,initSimdVar6_Uint8x16),0)) & 255]);
  };
  obj0.method0 = func2;
  obj0.method1 = func1;
  obj1.method0 = func4;
  obj1.method1 = obj0.method1;
  arrObj0.method0 = func2;
  arrObj0.method1 = obj1.method0;
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
  var IntArr0 = new Array(12);
  var IntArr1 = GenerateArray(2499635, 'int', 7, 0, 'literal');
  var FloatArr0 = new Array(-2147483646,160,-5.82512582115321E+18,-158794611,-2100590948,-1489799502.9,2);
  var VarArr0 = new Array(protoObj1,-352480688,555733311,-1928751046);
  var a = 3.93359111525617E+18;
  var b = -559195216;
  var c = -47315602.9;
  var d = 528721753.1;
  var e = 2;
  var f = -426669557;
  var g = 751176909;
  var h = -387737437;
  arrObj0[0] = -6.55860812284464E+18;
  arrObj0[1] = 4.4486716750172E+18;
  arrObj0[2] = 260757061.1;
  arrObj0[3] = -1036100909.9;
  arrObj0[4] = 21;
  arrObj0[5] = -2147483648;
  arrObj0[6] = 1208108042.1;
  arrObj0[7] = 410074837;
  arrObj0[8] = -238384595;
  arrObj0[9] = 276758607;
  arrObj0[10] = -2147483647;
  arrObj0[11] = 2147483650;
  arrObj0[12] = -898928516;
  arrObj0[13] = -133196391.9;
  arrObj0[14] = 65535;
  arrObj0[arrObj0.length-1] = 290570143;
  arrObj0.length = makeArrayLength(-2073943406);
  ary[0] = 4.06178539076517E+17;
  ary[1] = 0;
  ary[2] = -1011343998;
  ary[3] = -1017117633;
  ary[4] = 47701894;
  ary[5] = 37292684;
  ary[6] = 999318215;
  ary[7] = 1083553631;
  ary[8] = 357284066;
  ary[9] = 970624332;
  ary[10] = 1195355961.1;
  ary[11] = 124;
  ary[12] = -17570664;
  ary[13] = -121852196.9;
  ary[14] = -36;
  ary[ary.length-1] = 65535;
  ary.length = makeArrayLength(-2099535855.9);
  protoObj0 = Object.create(obj0);
  protoObj1 = Object.create(obj1);
  var initSimdVar0_Float32x4_alias = initSimdVar0_Float32x4;;
  var initSimdVar1_Int32x4_alias = initSimdVar1_Int32x4;;
  var initSimdVar2_Int16x8_alias = initSimdVar2_Int16x8;;
  var initSimdVar3_Int8x16_alias = initSimdVar3_Int8x16;;
  var initSimdVar4_Uint32x4_alias = initSimdVar4_Uint32x4;;
  var initSimdVar5_Uint16x8_alias = initSimdVar5_Uint16x8;;
  var aliasOfi8 = i8;;
  this.prop0 = 1059781405;
  this.prop1 = 65536;
  obj0.prop0 = 854733456;
  obj0.prop1 = 728277799;
  obj0.length = makeArrayLength(969007269);
  protoObj0.prop0 = -1733286208;
  protoObj0.prop1 = -1460109833.9;
  protoObj0.length = makeArrayLength(1697329556);
  obj1.prop0 = -197;
  obj1.prop1 = -186561909;
  obj1.length = makeArrayLength(900743509.1);
  protoObj1.prop0 = +null;
  protoObj1.prop1 = 283397832;
  protoObj1.length = makeArrayLength(2147483650);
  arrObj0.prop0 = 256397415.1;
  arrObj0.prop1 = -1004340958;
  arrObj0.length = makeArrayLength(2147483647);
  IntArr0[7] = -7.85235587633314E+18;
  IntArr0[IntArr0.length] = 1309633595.1;
  IntArr0[IntArr0.length] = 701206378;
  IntArr0[10] = 993102364;
  IntArr0[4] = -628818225;
  IntArr0[9] = -318118791;
  IntArr0[2] = -385394203;
  IntArr0[0] = -5.03167620292375E+18;
  IntArr0[5] = -728188801;
  IntArr0[6] = 94;
  IntArr0[11] = -7.40444638039172E+18;
  IntArr0[1] = -930956774.9;
  VarArr0[6] = 1471657295.1;
  VarArr0[VarArr0.length] = 21;
  VarArr0[4] = -582699345;
  VarArr0[5] = -650355558;
  VarArr0[8] = -2026926200;
  function func6 (arg0, arg1) {
    this.prop0 = arg0;
    this.prop2 = arg1;
  }
  var uniqobj0 = new func6(this.prop1,((g >= this.prop1)&&(h != f)));
  if (shouldBailout) {
    (shouldBailout ? (Object.defineProperty(uniqobj0, 'prop0', {set: function(_x) {     protoObj0.length = makeArrayLength((SIMD.Int32x4.extractLane(SIMD.Int32x4.mul(initSimdVar1_Int32x4,initSimdVar1_Int32x4_alias),1) === true));
    }, configurable: true }), VarArr0[((shouldBailout ? (VarArr0[6] = 'x') : undefined ), 6)]) : VarArr0[((shouldBailout ? (VarArr0[6] = 'x') : undefined ), 6)]);
  }
  if((-96 < SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,1))) {
    
    var __loopvar1 = loopInvariant - 3,__loopSecondaryVar1_0 = loopInvariant;
    do {
      if (__loopvar1 >= loopInvariant + -1) break;
      __loopvar1++;
      __loopSecondaryVar1_0++;
      var __loopvar2 = loopInvariant,__loopSecondaryVar2_0 = loopInvariant - 3,__loopSecondaryVar2_1 = loopInvariant + 6;
      LABEL0: 
      LABEL1: 
      while((func4((SIMD.Float32x4.extractLane(SIMD.Float32x4.fromInt32x4Bits(SIMD.Int32x4(-1102746072,-185,-78,197)),2) * SIMD.Bool32x4.extractLane(SIMD.Bool32x4.check(initSimdVar7_Bool32x4),3) + SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,12)),((new Object()) instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)),leaf))) {
        __loopvar2 -= 4;
        __loopSecondaryVar2_1 -= 2;
        if (__loopvar2 == loopInvariant - 16) break;
        __loopSecondaryVar2_0++;
      }
      var simdVar2_Bool16x8 = SIMD.Uint16x8.lessThan(SIMD.Uint16x8.replaceLane(initSimdVar5_Uint16x8_alias,4,20810),initSimdVar5_Uint16x8);
      var simdVar3_Uint16x8 = SIMD.Uint16x8.fromInt32x4Bits(SIMD.Int32x4(42,360362486,-564251954,1827846746));
      var simdVar4_Bool = SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4);
      obj1.prop5=obj0.method0;
      var func7 = function(argMath8,argMath9,argMath10,argMath11){
        obj1.method0.call(arrObj0 , (typeof(obj1.prop1)  != 'string') , SIMD.Float32x4.extractLane(SIMD.Float32x4.check(initSimdVar0_Float32x4_alias),0), leaf);
        var __loopvar3 = loopInvariant,__loopSecondaryVar3_0 = loopInvariant;
        for (var _strvar4 in IntArr0) {
          if(typeof _strvar4 === 'string' && _strvar4.indexOf('method') != -1) continue;
          __loopSecondaryVar3_0++;
          if (__loopvar3 > loopInvariant + 3) break;
          __loopvar3++;
          IntArr0[_strvar4] = ((_strvar4 !== _strvar4)&&(a == _strvar4));
          arrObj0.prop0 ^=((_strvar4 !== _strvar4)&&(a == _strvar4));
        }
        arrObj0.prop5=obj1.prop5;
        __loopSecondaryVar1_0++;;
        return 477900318;
      };
    } while((SIMD.Uint32x4.extractLane(SIMD.Uint32x4.swizzle(SIMD.Uint32x4.fromFloat32x4Bits(SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(1158777112,2681099364,2419694733,1574248969))),3,2,2,0),0)))
    var simdVar5_Float32x4 = SIMD.Float32x4.reciprocalSqrtApproximation(initSimdVar0_Float32x4_alias,initSimdVar0_Float32x4);
  }
  else {
    var simdVar6_Int8x16 = SIMD.Int8x16.shiftRightByScalar(initSimdVar3_Int8x16,14);
    
    var __loopvar1 = loopInvariant,__loopSecondaryVar1_0 = loopInvariant,__loopSecondaryVar1_1 = loopInvariant + 3;
    LABEL0: 
    for(;;__loopSecondaryVar1_0 += 3) {
      if (__loopvar1 > loopInvariant + 9) break;
      __loopvar1 += 3;
      __loopSecondaryVar1_1--;
      var simdVar7_Float32x4 = SIMD.Float32x4.abs(SIMD.Float32x4.fromInt16x8Bits(SIMD.Int16x8(29179,9336,27513,16447,9215,14534,27700,4297)));
      arrObj0.prop0 =(arrObj0.length && 3.30553214329242E+18);
      protoObj0.method0.call(uniqobj0 , arrObj0, leaf);
      var __loopvar2 = loopInvariant + 10,__loopSecondaryVar2_0 = loopInvariant,__loopSecondaryVar2_1 = loopInvariant - 3;
      LABEL1: 
      for(;;) {
        __loopvar2 -= 4;
        __loopSecondaryVar2_0 -= 4;
        if (__loopvar2 <= loopInvariant) break;
        __loopSecondaryVar2_1++;
        var __loopvar3 = loopInvariant;
        LABEL2: 
        for (var _strvar0 in f32) {
          if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
          if (__loopvar3 >= loopInvariant + 2) break;
          __loopvar3++;
          f32[_strvar0] = ary[(loopInvariant)];
        }
        var __loopvar3 = loopInvariant;
        do {
          if (__loopvar3 >= loopInvariant + 2) break;
          __loopvar3++;
          obj0.length = makeArrayLength((-- e));
          a = SIMD.Float32x4.extractLane(SIMD.Float32x4.fromUint16x8Bits(SIMD.Uint16x8(48418,50613,5102,32910,4582,45342,59137,46852)),0);
        } while((IntArr1[((shouldBailout ? (IntArr1[__loopSecondaryVar2_0 - 2] = 'x') : undefined ), __loopSecondaryVar2_0 - 2)]))
        var __loopvar3 = loopInvariant;
        LABEL2: 
        while((SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4))) {
          __loopvar3 -= 3;
          if (__loopvar3 == loopInvariant - 12) break;
          protoObj1.prop0 &=(protoObj0.prop0 === -4.05938901106374E+16);
          var id30 = (shouldBailout ? (Object.defineProperty(protoObj0, 'length', {writable: false, enumerable: false, configurable: true }), SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4)) : SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4));
          continue ;
          protoObj0.prop4 = SIMD.Int8x16.extractLane(initSimdVar3_Int8x16,13);
          arrObj0.prop1 =4.63704935357578E+18;
        }
        var simdVar8_Uint16x8 = SIMD.Uint16x8.fromInt8x16Bits(SIMD.Int8x16(90,120,105,125,98,127,21,82,14,9,24,16,1,64,35,79));
        
        protoObj1.prop4 = VarArr0[(__loopSecondaryVar2_0 - 2)];
      }
      arrObj0.length = makeArrayLength(((shouldBailout ? func4 = func4 : 1), func4(SIMD.Float32x4.extractLane(initSimdVar0_Float32x4_alias,2),(arrObj0.length && 3.30553214329242E+18),leaf)));
    }
  }
  protoObj1.method0(-501529895.9,obj0.method0.call(litObj0 , arrObj0, leaf),leaf);
  if((f === obj0.method0.call(litObj0 , arrObj0, leaf))) {
    var simdVar9_Int16x8 = SIMD.Int16x8.or(SIMD.Int16x8.fromUint32x4Bits(SIMD.Uint32x4(1876171124,2474318760,2410277206,955380195)),initSimdVar2_Int16x8);
    var uniqobj1 = obj1;
  }
  else {
    var __loopvar1 = loopInvariant - 7;
    LABEL0: 
    LABEL1: 
    do {
      __loopvar1 += 2;
      if (__loopvar1 > loopInvariant + 2) break;
      var simdVar10_Float32x4 = SIMD.Float32x4.div(initSimdVar0_Float32x4_alias,SIMD.Float32x4.replaceLane(initSimdVar0_Float32x4,3,1352341));
      obj0.prop5=SIMD.Int32x4.extractLane(initSimdVar1_Int32x4,3);
      this.prop0 = (obj0.prop5++ );
      var simdVar11_Uint32x4 = SIMD.Uint32x4.fromUint8x16Bits(SIMD.Uint8x16(86,49,53,98,154,31,86,78,60,215,15,161,83,1,214,212));
      var simdVar12_Int16x8 = SIMD.Int16x8.shuffle(initSimdVar2_Int16x8_alias,SIMD.Int16x8.shuffle(SIMD.Int16x8.addSaturate(SIMD.Int16x8.shuffle(SIMD.Int16x8.fromInt8x16Bits(SIMD.Int8x16(92,1,59,62,43,6,14,119,105,35,69,20,106,57,14,82)),SIMD.Int16x8.fromInt8x16Bits(SIMD.Int8x16(53,29,36,11,25,35,102,77,67,51,114,38,57,121,15,101)),3,13,8,2,10,11,10,0),SIMD.Int16x8.fromInt32x4Bits(SIMD.Int32x4(29,14,-157766975,247))),SIMD.Int16x8.add(SIMD.Int16x8.subSaturate(initSimdVar2_Int16x8,SIMD.Int16x8.and(initSimdVar2_Int16x8_alias,initSimdVar2_Int16x8)),SIMD.Int16x8.fromUint32x4Bits(SIMD.Uint32x4(2638956216,1013840484,2628696842,1715142262))),1,2,10,15,13,9,9,0),4,0,3,9,11,10,3,4);
      var simdVar13_Uint8x16 = SIMD.Uint8x16.addSaturate(SIMD.Uint8x16.mul(initSimdVar6_Uint8x16,SIMD.Uint8x16.replaceLane(SIMD.Uint8x16.or(SIMD.Uint8x16.not(initSimdVar6_Uint8x16,initSimdVar6_Uint8x16),initSimdVar6_Uint8x16),3,214)),initSimdVar6_Uint8x16);
    } while((Object.create({prop0: SIMD.Float32x4.extractLane(SIMD.Float32x4.replaceLane(initSimdVar0_Float32x4_alias,3,-1839655327),3), prop1: SIMD.Float32x4.extractLane(SIMD.Float32x4.replaceLane(initSimdVar0_Float32x4_alias,3,-1839655327),3), prop2: (++ f), prop3: d, prop4: obj0.method1.call(protoObj1 , leaf, aliasOfi8[(((obj1.prop1++ ) * (SIMD.Int32x4.extractLane(SIMD.Int32x4.fromFloat32x4Bits(SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(-503834812,-1217566879,199949452,-735702356))),1) + ary[((shouldBailout ? (ary[__loopvar1 + 1] = 'x') : undefined ), __loopvar1 + 1)]))) & 255], obj1.method1.call(obj0 , leaf, SIMD.Float32x4.extractLane(SIMD.Float32x4.shuffle(initSimdVar0_Float32x4,initSimdVar0_Float32x4,0,6,5,0),3), SIMD.Int32x4.extractLane(initSimdVar1_Int32x4,3)))})))
  }
  var simdVar14_Int8x16 = SIMD.Int8x16.shuffle(SIMD.Int8x16.fromInt16x8Bits(SIMD.Int16x8(1195,6501,3596,14798,27157,425,15791,1644)),initSimdVar3_Int8x16,14,9,7,19,14,7,21,24,24,7,29,7,19,16,13,0);
  protoObj0.method0.call(protoObj0 , arrObj0, leaf);
  obj7 = Object.create(arrObj0);
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
  WScript.Echo('uniqobj0.prop0 = ' + (uniqobj0.prop0|0));
  WScript.Echo('uniqobj0.prop2 = ' + (uniqobj0.prop2|0));
  WScript.Echo('obj7.prop0 = ' + (obj7.prop0|0));
  WScript.Echo('obj7.prop1 = ' + (obj7.prop1|0));
  WScript.Echo('obj7.length = ' + (obj7.length|0));
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
  WScript.Echo('initSimdVar0_Float32x4_alias = ' + (initSimdVar0_Float32x4_alias.toString()));
  WScript.Echo('initSimdVar1_Int32x4 = ' + (initSimdVar1_Int32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4_alias = ' + (initSimdVar1_Int32x4_alias.toString()));
  WScript.Echo('initSimdVar2_Int16x8 = ' + (initSimdVar2_Int16x8.toString()));
  WScript.Echo('initSimdVar2_Int16x8_alias = ' + (initSimdVar2_Int16x8_alias.toString()));
  WScript.Echo('initSimdVar3_Int8x16 = ' + (initSimdVar3_Int8x16.toString()));
  WScript.Echo('initSimdVar3_Int8x16_alias = ' + (initSimdVar3_Int8x16_alias.toString()));
  WScript.Echo('simdVar14_Int8x16 = ' + (simdVar14_Int8x16.toString()));
  WScript.Echo('initSimdVar4_Uint32x4 = ' + (initSimdVar4_Uint32x4.toString()));
  WScript.Echo('initSimdVar4_Uint32x4_alias = ' + (initSimdVar4_Uint32x4_alias.toString()));
  WScript.Echo('initSimdVar5_Uint16x8 = ' + (initSimdVar5_Uint16x8.toString()));
  WScript.Echo('initSimdVar5_Uint16x8_alias = ' + (initSimdVar5_Uint16x8_alias.toString()));
  WScript.Echo('initSimdVar6_Uint8x16 = ' + (initSimdVar6_Uint8x16.toString()));
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


// Baseline total processor time: 00:00:00.3125000
// Test total processor time: 00:00:00.6562500
// 
// Addl Dump Info: 
//Baseline length = 15055

//Test length = 4023

//Baseline length = 15055

//Test length = 4022

//Baseline length = 15055

//Test length = 4022

// 
// Baseline output:
// Skipping first 354 lines of output...
// simdVar14_Int8x16 = SIMD.Int8x16(108, 106, 57, 39, 108, 57, 38, 19, 19, 57, 12, 57, 39, 65, 61, -85)
// initSimdVar4_Uint32x4 = SIMD.Uint32x4(2043456508, 1088554286, 1678129789, 1611325180)
// initSimdVar4_Uint32x4_alias = SIMD.Uint32x4(2043456508, 1088554286, 1678129789, 1611325180)
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(8161, 53218, 45235, 39919, 18851, 26209, 48753, 39759)
// initSimdVar5_Uint16x8_alias = SIMD.Uint16x8(8161, 53218, 45235, 39919, 18851, 26209, 48753, 39759)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(27, 138, 207, 246, 79, 81, 18, 60, 184, 166, 192, 130, 47, 173, 24, 179)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, true, false, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(false, true, false, false, false, true, false, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, false, false, true, false, false, true, true, true, true, true, true, true, true, false, true)
// sumOfary = 1215961780x1083553631x970624332x124x-121852196.96553500
// subset_of_ary = 1011945205,0,-1011343998,-1017117633,47701894,37292684,x,1083553631,x,970624332,x
// sumOfIntArr0 = -920985433
// subset_of_IntArr0 = -129411507,-930956774.90000000,-385394203,,-628818225,-728188801,94,-1018957859,,-318118791,993102364
// sumOfIntArr1 = 8914565
// subset_of_IntArr1 = 312606,55451,1995027,1224303,2239855,1701867,1385456
// sumOfFloatArr0 = -1324414504
// subset_of_FloatArr0 = -2147483646,160,-1870197285,-158794611,-2100590948,-1489799502.90000000,2
// sumOfVarArr0 = 0[object Object]-559232995-1928751046-582699345-650355558x21-2026926200
// subset_of_VarArr0 = [object Object],-352480688,555733311,-1928751046,-582699345,-650355558,x,21,-2026926200
// 
// 
// Test output:
// Skipping first 75 lines of output...
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(8161, 53218, 45235, 39919, 18851, 26209, 48753, 39759)
// initSimdVar5_Uint16x8_alias = SIMD.Uint16x8(8161, 53218, 45235, 39919, 18851, 26209, 48753, 39759)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(27, 138, 207, 246, 79, 81, 18, 60, 184, 166, 192, 130, 47, 173, 24, 179)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, true, false, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(false, true, false, false, false, true, false, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(true, false, false, true, false, false, true, true, true, true, true, true, true, true, false, true)
// sumOfary = 1387773430
// subset_of_ary = 1011945205,0,-1011343998,-1017117633,47701894,37292684,999318215,1083553631,357284066,970624332,1195355961.10000000
// sumOfIntArr0 = -920985433
// subset_of_IntArr0 = -129411507,-930956774.90000000,-385394203,,-628818225,-728188801,94,-1018957859,,-318118791,993102364
// sumOfIntArr1 = 8914565
// subset_of_IntArr1 = 312606,55451,1995027,1224303,2239855,1701867,1385456
// sumOfFloatArr0 = -1324414504
// subset_of_FloatArr0 = -2147483646,160,-1870197285,-158794611,-2100590948,-1489799502.90000000,2
// sumOfVarArr0 = 0[object Object]-559232995-1928751046-582699345-1193481905-2026926200
// subset_of_VarArr0 = [object Object],-352480688,555733311,-1928751046,-582699345,-650355558,1471657295.10000000,21,-2026926200
// ASSERTION 9028: (e:\nagy\git\chakra5\core\lib\runtime\Library/JavascriptNumber.inl, line 128) We should only produce a NaN with this value
//  Failure: (!IsNan(value) || ToSpecial(value) == k_Nan || ToSpecial(value) == 0x7FF8000000000000ull)
// FATAL ERROR: jshost.exe failed due to exception code c0000420
// 
// Reduced Switches:
