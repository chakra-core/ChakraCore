//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//Configuration: configs\SIMD.xml
//Testcase Number: 144
//Bailout Testing: ON
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:6 -maxsimplejitruncount:4 -werexceptionsupport  -forcedeferparse -bgjit- -loopinterpretcount:1 -force:atom -force:fixdataprops -off:lossyinttypespec -force:rejit -ForceArrayBTree -force:ScriptFunctionWithInlineCache -off:ArrayCheckHoist -off:BoundCheckHoist -off:checkthis -off:trackintusage -off:aggressiveinttypespec -off:fefixedmethods -off:DelayCapture -off:ParallelParse -off:bailonnoprofile -off:BoundCheckElimination -ValidateInlineStack -off:LoopCountBasedBoundCheckHoist -off:floattypespec -off:ArrayMissingValueCheckHoist -off:JsArraySegmentHoist
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
  var loopInvariant = shouldBailout ? 7 : 4;
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
  var initSimdVar0_Float32x4 = SIMD.Float32x4(-613337068,61748951,1191920964,-603642948.9);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(202,559820669,-600546841,-250464089);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(3556,23048,10027,23406,15303,20390,11218,1040);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(57,73,27,65,94,109,54,39,38,93,50,22,51,102,115,121);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(2299364215,74,2191949124,1008992599);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(2332,16861,37523,56336,30981,12378,7615,37523);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(233,245,177,73,24,90,144,158,180,196,114,65,297383924,71,170,148);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(true,false,true,true);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(true,true,true,false,false,false,true,false);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(false,false,false,true,false,true,true,true,true,true,true,false,false,true,true,true);
  var func0 = function(argMath0,argMath1){
    if(shouldBailout){
      leaf = leaf;
    }
    leaf();
    return SIMD.Int16x8.extractLane(initSimdVar2_Int16x8,1);
  };
  var func1 = function(){
    (function(){
      e = SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,1);
      if(shouldBailout){
        return  'somestring'
      }
      obj0.length= makeArrayLength((-- f));
      var uniqobj0 = obj0;
    })();
    return SIMD.Int8x16.extractLane(SIMD.Int8x16.add(SIMD.Int8x16.swizzle(initSimdVar3_Int8x16,5,7,5,15,0,1,8,1,0,0,7,6,10,11,13,13),initSimdVar3_Int8x16),2);
  };
  var func2 = function(argMath2){
    litObj0.prop0=func1;
    var simdVar0_Uint32x4 = SIMD.Uint32x4.load1(uic8,387172824 % (uic8.length - (128 / uic8.BYTES_PER_ELEMENT / 8 )));
    return argMath2;
  };
  var func3 = function(argMath3,argMath4,argMath5,argMath6){
    var simdVar1_Float32x4 = SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(-1328858244,-1525871583,457268526,1017616266));
    var uniqobj1 = [''];
    var uniqobj2 = uniqobj1[__counter%uniqobj1.length];
    uniqobj2.toString();
    return SIMD.Bool16x8.anyTrue(SIMD.Bool16x8.or(SIMD.Uint16x8.equal(initSimdVar5_Uint16x8,initSimdVar5_Uint16x8),SIMD.Bool16x8.not(initSimdVar8_Bool16x8_alias,initSimdVar8_Bool16x8_alias)));
  };
  var func4 = function(argMath7,argMath8){
    return ((SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,4) / (func3.call(arrObj0 , arguments[(((SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,7) >= 0 ? SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,7) : 0)) & 0XF)], arrObj0, ary, ((argMath8 > protoObj1.length)||(argMath7 !== protoObj0.length))) == 0 ? 1 : func3.call(arrObj0 , arguments[(((SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,7) >= 0 ? SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,7) : 0)) & 0XF)], arrObj0, ary, ((argMath8 > protoObj1.length)||(argMath7 !== protoObj0.length))))) ? parseInt("-11000011100011000001110100000", 2) : SIMD.Int16x8.extractLane(initSimdVar2_Int16x8,3));
  };
  obj0.method0 = func3;
  obj0.method1 = func4;
  obj1.method0 = func4;
  obj1.method1 = func1;
  arrObj0.method0 = obj1.method0;
  arrObj0.method1 = func2;
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
  var IntArr0 = new Array(-474715470,-6232622756420927488,65536,285958823);
  var IntArr1 = [];
  var FloatArr0 = [];
  var VarArr0 = [protoObj0,4638627500864586752,260777878,-7725231065082494976,-379189476,-6.5731459904732E+18,-1911972489430278400,710811926,-1971348070,7237610284929473536];
  var a = -1;
  var b = -203;
  var c = -2147483647;
  var d = -2147483648;
  var e = -6.09736383123945E+18;
  var f = -15797601;
  var g = -6;
  var h = -669487610.9;
  arrObj0[0] = 1684106969.1;
  arrObj0[1] = 34677251;
  arrObj0[2] = 95531277.1;
  arrObj0[3] = -9;
  arrObj0[4] = 137254149;
  arrObj0[5] = 1;
  arrObj0[6] = 869873889.1;
  arrObj0[7] = -1481533539;
  arrObj0[8] = 1363942029.1;
  arrObj0[9] = 2147483647;
  arrObj0[10] = 3.6378054931372E+18;
  arrObj0[11] = -262734808;
  arrObj0[12] = -692658955;
  arrObj0[13] = 8.84053962257929E+18;
  arrObj0[14] = -74058336;
  arrObj0[arrObj0.length-1] = 65537;
  arrObj0.length = makeArrayLength(367944381);
  ary[0] = 14;
  ary[1] = -1867554831;
  ary[2] = -1278011805;
  ary[3] = -7.63422666769733E+18;
  ary[4] = 1942313288;
  ary[5] = -84;
  ary[6] = 54372378;
  ary[7] = 175;
  ary[8] = -2.1637242118795E+18;
  ary[9] = -72;
  ary[10] = -1073741824;
  ary[11] = -537225439;
  ary[12] = 218715533;
  ary[13] = -2.42826393188909E+18;
  ary[14] = -1073741824;
  ary[ary.length-1] = 577170510;
  ary.length = makeArrayLength(-359410852);
  protoObj0 = Object.create(obj0);
  protoObj1 = Object.create(obj1);
  var initSimdVar1_Int32x4_alias = initSimdVar1_Int32x4;;
  var initSimdVar3_Int8x16_alias = initSimdVar3_Int8x16;;
  var initSimdVar5_Uint16x8_alias = initSimdVar5_Uint16x8;;
  var initSimdVar8_Bool16x8_alias = initSimdVar8_Bool16x8;;
  var initSimdVar9_Bool8x16_alias = initSimdVar9_Bool8x16;;
  this.prop0 = -6.00230435422991E+18;
  this.prop1 = 917470499.1;
  obj0.prop0 = 74;
  obj0.prop1 = 1556884599.1;
  obj0.length = makeArrayLength(-189);
  protoObj0.prop0 = -1861925077.9;
  protoObj0.prop1 = 433418241;
  protoObj0.length = makeArrayLength(1491176196.1);
  obj1.prop0 = -460957253.9;
  obj1.prop1 = -842987034;
  obj1.length = makeArrayLength(-3.20475160195987E+18);
  protoObj1.prop0 = 625414530.1;
  protoObj1.prop1 = 116;
  protoObj1.length = makeArrayLength(-2.52842529711435E+18);
  arrObj0.prop0 = 45;
  arrObj0.prop1 = 128;
  arrObj0.length = makeArrayLength(106);
  IntArr1[10] = -2.63538201742249E+18;
  IntArr1[11] = 1421401367.1;
  IntArr1[IntArr1.length] = -3.85491378845693E+18;
  IntArr1[2] = -1.78012486606259E+18;
  IntArr1[9] = 1058293475;
  IntArr1[13] = -6.34632330019594E+18;
  IntArr1[0] = 2;
  IntArr1[8] = 2031820217.1;
  IntArr1[4] = 7.25802183627491E+18;
  IntArr1[1] = 1234373046;
  IntArr1[5] = 68;
  IntArr1[12] = -428774369.9;
  IntArr1[7] = -7.64791562106298E+18;
  IntArr1[6] = -834464702;
  FloatArr0[0] = -2147483648;
  FloatArr0[1] = 110;
  var simdVar2_Float32x4 = SIMD.Float32x4.sqrt(initSimdVar0_Float32x4);
  protoObj1.prop5=Math.floor(IntArr0[((shouldBailout ? (IntArr0[12] = 'x') : undefined ), 12)]);
  obj0.prop0 |=(++ protoObj0.prop0);
  obj0.method1.call(obj1 , SIMD.Int32x4.extractLane(SIMD.Int32x4.select(SIMD.Bool32x4.replaceLane(initSimdVar7_Bool32x4,2,false),initSimdVar1_Int32x4,SIMD.Int32x4.fromUint16x8Bits(SIMD.Uint16x8(61697,19729,27082,45595,45399,20098,39387,27061))),0), ary);
  var __loopvar0 = loopInvariant;
  LABEL0: 
  while((parseInt("-0xC5"))) {
    if (__loopvar0 > loopInvariant + 3) break;
    __loopvar0++;
    var simdVar3_Uint32x4 = SIMD.Uint32x4(4123355464,2991471250,735111536,1398251167);
    var uniqobj3 = litObj1;
    var simdVar4_Bool32x4 = SIMD.Float32x4.greaterThanOrEqual(SIMD.Float32x4.fromInt8x16Bits(SIMD.Int8x16(45,16,62,23,89,97,15,64,46,111,93,98,51,93,97,71)),SIMD.Float32x4.add(SIMD.Float32x4.load(f32,1258247694 % (f32.length - (128 / f32.BYTES_PER_ELEMENT / 8 ))),simdVar2_Float32x4));
    var __loopvar1 = loopInvariant,__loopSecondaryVar1_0 = loopInvariant - 12;
    for(;(__loopvar1 < loopInvariant + 12) && arrObj0.length < (obj1.method0.call(obj1 , (obj1.prop0 < a), VarArr0)); arrObj0.length++) {
      __loopvar1 += 3;
      __loopSecondaryVar1_0 += 4;
      var simdVar5_Int16x8 = SIMD.Int16x8.swizzle(initSimdVar2_Int16x8,5,4,0,6,6,2,2,3);
      litObj6 = {57: ((IntArr1[(11)], ((obj1.prop0 < a)&&(obj1.prop0 != obj1.prop0))) * obj1.method0.call(obj1 , (obj1.prop0 < a), VarArr0) - obj1.prop0), prop1: SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,5), prop2: (test0.caller), prop3: (+ SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,5)), prop4: (SIMD.Uint32x4.extractLane(simdVar3_Uint32x4,1) ? FloatArr0[(__loopSecondaryVar1_0 + 1)] : ((SIMD.Bool8x16.extractLane(initSimdVar9_Bool8x16,8) | arrObj0.method1.call(obj1 , IntArr1[((shouldBailout ? (IntArr1[__loopvar0 + 1] = 'x') : undefined ), __loopvar0 + 1)])) && ui16[(SIMD.Int8x16.extractLane(initSimdVar3_Int8x16_alias,7)) & 255]))};
      if((obj0.method0.call(protoObj1 , SIMD.Int16x8.extractLane(simdVar5_Int16x8,3), litObj6, FloatArr0, (-- d)) instanceof ((typeof Array == 'function' ) ? Array : Object))) {
        f >>>=(test0.caller);
        
      }
      else {
        var __loopvar3 = loopInvariant,__loopSecondaryVar3_0 = loopInvariant;
        LABEL1: 
        for(;;__loopvar3++) {
          if (__loopvar3 == loopInvariant + 4) break;
          __loopSecondaryVar3_0 -= 2;
          e = ((SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,1) ? func1.call(litObj6 ) : SIMD.Float32x4.extractLane(SIMD.Float32x4.sqrt(SIMD.Float32x4.fromUint8x16Bits(SIMD.Uint8x16(58,116,201,237,87,144,21,145,55,132,62,232,108,147,45,35))),2)) ? (SIMD.Float32x4.extractLane(simdVar2_Float32x4,1) != (IntArr1.pop())) : (protoObj1.prop5 != protoObj0.prop1));
          obj1.prop1 =SIMD.Float32x4.extractLane(SIMD.Float32x4.fromUint16x8Bits(SIMD.Uint16x8(28712,65217,15846,39932,58338,57220,44556,62615)),2);
          obj6 = Object.create(protoObj1);
          protoObj1.prop5 = ([1, 2, 3] instanceof ((typeof String == 'function' ) ? String : Object));
          protoObj0.prop1 = SIMD.Uint8x16.extractLane(SIMD.Uint8x16.fromInt8x16Bits(SIMD.Int8x16(81,69,26,59,77,8,17,1,90,120,106,52,93,58,96,47)),3);
        }
      }
      var simdVar6_Bool16x8 = SIMD.Int16x8.lessThanOrEqual(SIMD.Int16x8.add(simdVar5_Int16x8,simdVar5_Int16x8),simdVar5_Int16x8);
      var simdVar7_Bool8x16 = SIMD.Uint8x16.notEqual(initSimdVar6_Uint8x16,initSimdVar6_Uint8x16);
    }
  }
  var uniqobj4 = [protoObj1, obj1, obj1, obj1];
  uniqobj4[__counter%uniqobj4.length].method1();
  if((SIMD.Uint16x8.extractLane(SIMD.Uint16x8.not(initSimdVar5_Uint16x8,SIMD.Uint16x8.shiftLeftByScalar(initSimdVar5_Uint16x8_alias,3)),6) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object))) {
    var __loopvar1 = loopInvariant - 12,__loopSecondaryVar1_0 = loopInvariant,__loopSecondaryVar1_1 = loopInvariant;
    LABEL0: 
    for (var _strvar0 in litObj1) {
      if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
      __loopSecondaryVar1_1 += 2;
      if (__loopvar1 === loopInvariant) break;
      if (__loopSecondaryVar1_1 > loopInvariant + 8) break;
      __loopvar1 += 4;
      __loopSecondaryVar1_0 += 3;
      litObj1[_strvar0] = SIMD.Int8x16.extractLane(SIMD.Int8x16.mul(initSimdVar3_Int8x16,SIMD.Int8x16.neg(initSimdVar3_Int8x16)),11);
      var simdVar8_Float32x4 = SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(1398965302,3508463544,542420937,1293641376));
ary0 = arguments;
      var simdVar9_Float32x4 = SIMD.Float32x4.abs(simdVar2_Float32x4);
      var simdVar10_Uint16x8 = SIMD.Uint16x8.add(SIMD.Uint16x8.select(SIMD.Uint16x8.notEqual(initSimdVar5_Uint16x8,initSimdVar5_Uint16x8),initSimdVar5_Uint16x8_alias,SIMD.Uint16x8.addSaturate(initSimdVar5_Uint16x8_alias,SIMD.Uint16x8.xor(initSimdVar5_Uint16x8,initSimdVar5_Uint16x8_alias))),initSimdVar5_Uint16x8_alias);
      var __loopvar2 = loopInvariant - 9,__loopSecondaryVar2_0 = loopInvariant - 3,__loopSecondaryVar2_1 = loopInvariant + 3;
      while((g)) {
        __loopSecondaryVar2_0++;
        if (__loopvar2 >= loopInvariant + -3) break;
        __loopvar2 += 3;
        __loopSecondaryVar2_1--;
        var simdVar11_Bool = SIMD.Bool8x16.extractLane(SIMD.Bool8x16.check(initSimdVar9_Bool8x16),10);
        var __loopvar3 = loopInvariant,__loopSecondaryVar3_0 = loopInvariant;
        do {
          __loopvar3 += 4;
          __loopSecondaryVar3_0 -= 4;
          if (__loopvar3 === loopInvariant + 16) break;
          protoObj0.length = makeArrayLength((false instanceof ((typeof RegExp == 'function' ) ? RegExp : Object)));
          ary[(((SIMD.Int8x16.extractLane(initSimdVar3_Int8x16_alias,14) >= 0 ? SIMD.Int8x16.extractLane(initSimdVar3_Int8x16_alias,14) : 0)) & 0XF)] = (1582022763.1 >>> simdVar11_Bool);
        } while(((SIMD.Bool16x8.allTrue(initSimdVar8_Bool16x8_alias) % SIMD.Int8x16.extractLane(initSimdVar3_Int8x16_alias,14))))
      }
    }
    var simdVar12_Uint32x4 = SIMD.Uint32x4.fromInt16x8Bits(SIMD.Int16x8(3158,5029,27270,2217,10774,31240,24262,2765));
    litObj1 = litObj1;
    
    var simdVar13_Float32x4 = SIMD.Float32x4.swizzle(SIMD.Float32x4.fromInt8x16Bits(SIMD.Int8x16(35,111,98,46,41,91,36,111,92,5,33,117,55,15,51,28)),1,3,2,0);
    var simdVar14_Float32x4 = SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(3166886871,2488561574,4272739381,3873224057));
  }
  else {
    var simdVar15_Uint32x4 = SIMD.Uint32x4.shiftRightByScalar(SIMD.Uint32x4.select(SIMD.Float32x4.lessThanOrEqual(SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(3172350589,375211066,294433892,148942292)),SIMD.Float32x4.shuffle(simdVar2_Float32x4,initSimdVar0_Float32x4,5,1,1,6)),initSimdVar4_Uint32x4,SIMD.Uint32x4.check(initSimdVar4_Uint32x4)),2);
    var simdVar16_Float32x4 = SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(858073238,-980740470,1189828069,-170));
    var simdVar17_Bool8x16 = SIMD.Uint8x16.equal(initSimdVar6_Uint8x16,initSimdVar6_Uint8x16);
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
  WScript.Echo('protoObj1.prop5 = ' + (protoObj1.prop5|0));
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
  WScript.Echo('simdVar2_Float32x4 = ' + (simdVar2_Float32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4 = ' + (initSimdVar1_Int32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4_alias = ' + (initSimdVar1_Int32x4_alias.toString()));
  WScript.Echo('initSimdVar2_Int16x8 = ' + (initSimdVar2_Int16x8.toString()));
  WScript.Echo('initSimdVar3_Int8x16 = ' + (initSimdVar3_Int8x16.toString()));
  WScript.Echo('initSimdVar3_Int8x16_alias = ' + (initSimdVar3_Int8x16_alias.toString()));
  WScript.Echo('initSimdVar4_Uint32x4 = ' + (initSimdVar4_Uint32x4.toString()));
  WScript.Echo('initSimdVar5_Uint16x8 = ' + (initSimdVar5_Uint16x8.toString()));
  WScript.Echo('initSimdVar5_Uint16x8_alias = ' + (initSimdVar5_Uint16x8_alias.toString()));
  WScript.Echo('initSimdVar6_Uint8x16 = ' + (initSimdVar6_Uint8x16.toString()));
  WScript.Echo('initSimdVar7_Bool32x4 = ' + (initSimdVar7_Bool32x4.toString()));
  WScript.Echo('initSimdVar8_Bool16x8 = ' + (initSimdVar8_Bool16x8.toString()));
  WScript.Echo('initSimdVar8_Bool16x8_alias = ' + (initSimdVar8_Bool16x8_alias.toString()));
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
test0();
// Run Simple JIT
test0();
test0();
test0();
test0();

// run JITted code
runningJITtedCode = true;
test0();
test0();
test0();

// run code with bailouts enabled
shouldBailout = true;
test0();


// Baseline total processor time: 00:00:00.5625000
// Test total processor time: 00:00:02.1562500
// 
// Addl Dump Info: 
//Baseline length = 48490

//Test length = 34693

//Baseline length = 48490

//Test length = 34693

//Baseline length = 48490

//Test length = 34693

// 
// Baseline output:
// Skipping first 1185 lines of output...
// initSimdVar4_Uint32x4 = SIMD.Uint32x4(151880568, 74, 44465477, 1008992599)
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(2332, 16861, 37523, 56336, 30981, 12378, 7615, 37523)
// initSimdVar5_Uint16x8_alias = SIMD.Uint16x8(2332, 16861, 37523, 56336, 30981, 12378, 7615, 37523)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(233, 245, 177, 73, 24, 90, 144, 158, 180, 196, 114, 65, 244, 71, 170, 148)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, false, true, true)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, true, true, false, false, false, true, false)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, true, true, false, false, false, true, false)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(false, false, false, true, false, true, true, true, true, true, true, false, false, true, true, true)
// initSimdVar9_Bool8x16_alias = SIMD.Bool8x16(false, false, false, true, false, true, true, true, true, true, true, false, false, true, true, true)
// sumOfary = -1232327475
// subset_of_ary = 14,-1867554831,-1278011805,-718162283,1942313288,-84,54372378,175,-755730538,-72,-1073741824
// sumOfIntArr0 = -1069081281x
// subset_of_IntArr0 = -474715470,-880390849,65536,285958823,,,,,,,
// sumOfIntArr1 = -405091259
// subset_of_IntArr1 = 2,1234373046,-183020613,,346365137,68,-834464702,-2112120186,2031820217.10000000,1058293475,-756088311
// sumOfFloatArr0 = -2147483538
// subset_of_FloatArr0 = -1,110
// sumOfVarArr0 = 0[object Object]485460051-410587508-379189476-23147241-1176015921-2143557547
// subset_of_VarArr0 = [object Object],857977414,260777878,-410587508,-379189476,-23147241,-299958963,710811926,-1971348070,1892789355
// 
// 
// Test output:
// Skipping first 842 lines of output...
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(2332, 16861, 37523, 56336, 30981, 12378, 7615, 37523)
// initSimdVar5_Uint16x8_alias = SIMD.Uint16x8(2332, 16861, 37523, 56336, 30981, 12378, 7615, 37523)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(233, 245, 177, 73, 24, 90, 144, 158, 180, 196, 114, 65, 244, 71, 170, 148)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, false, true, true)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, true, true, false, false, false, true, false)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, true, true, false, false, false, true, false)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(false, false, false, true, false, true, true, true, true, true, true, false, false, true, true, true)
// initSimdVar9_Bool8x16_alias = SIMD.Bool8x16(false, false, false, true, false, true, true, true, true, true, true, false, false, true, true, true)
// sumOfary = -1232327475
// subset_of_ary = 14,-1867554831,-1278011805,-718162283,1942313288,-84,54372378,175,-755730538,-72,-1073741824
// sumOfIntArr0 = -1069081281
// subset_of_IntArr0 = -474715470,-880390849,65536,285958823
// sumOfIntArr1 = -405091259
// subset_of_IntArr1 = 2,1234373046,-183020613,,346365137,68,-834464702,-2112120186,2031820217.10000000,1058293475,-756088311
// sumOfFloatArr0 = -2147483538
// subset_of_FloatArr0 = -1,110
// sumOfVarArr0 = 0[object Object]485460051-410587508-379189476-23147241-1176015921-2143557547
// subset_of_VarArr0 = [object Object],857977414,260777878,-410587508,-379189476,-23147241,-299958963,710811926,-1971348070,1892789355
// FATAL ERROR: jshost.exe failed due to exception code c0000005
// 
// Reduced Switches:
