//Configuration: configs\SIMD.xml
//Testcase Number: 291
//Bailout Testing: ON
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport  -MaxLinearStringCaseCount:2 -MaxLinearIntCaseCount:2 -bgjit- -loopinterpretcount:1 -MinSwitchJumpTableSize:3 -force:rejit -force:ScriptFunctionWithInlineCache -force:fixdataprops -force:atom -off:lossyinttypespec -off:ParallelParse -off:fefixedmethods -off:checkthis -off:DelayCapture -off:ArrayLengthHoist -off:ArrayCheckHoist -ForceArrayBTree -off:trackintusage -off:bailonnoprofile
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
  var loopInvariant = shouldBailout ? 10 : 9;
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
  var initSimdVar0_Float32x4 = SIMD.Float32x4(-1874459392,116,4.68591140246336E+18,-2147483647);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(210268018,-215660674,102,34);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(3705,2734,3403,23027,20761,32151,18607,13488);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(101,62,106,50,127,90,115,73,52,55,0,10,112,114,56,97);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(1071188118,1934777615,983857782,2438123274);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(47187,56187,35154,31913,29852,47176,6872,54876);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(139,219,120,233,234,90,75,207,60,212,18,175,123,17,222,14);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(false,false,true,false);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(-99,false,true,true,false,true,false,true);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(false,false,true,true,true,true,true,true,false,true,true,true,true,false,false,false);
  var func0 = function(argMath0){
    var simdVar0_Float32x4 = SIMD.Float32x4.max(initSimdVar0_Float32x4,SIMD.Float32x4.max(initSimdVar0_Float32x4,SIMD.Float32x4.fromInt8x16Bits(SIMD.Int8x16(99,54,110,90,34,103,27,119,7,31,122,70,18,60,28,55))));
    var __loopvar3 = loopInvariant;
    LABEL0: 
    LABEL1: 
    for (var _strvar1 in ary) {
      if(typeof _strvar1 === 'string' && _strvar1.indexOf('method') != -1) continue;
      if (__loopvar3 <= loopInvariant - 6) break;
      __loopvar3 -= 3;
    }
    var simdVar1_Uint32x4 = SIMD.Uint32x4.and(SIMD.Uint32x4.fromInt32x4Bits(SIMD.Int32x4(-165,223,938661303,-150930781)),SIMD.Uint32x4.and(initSimdVar4_Uint32x4,SIMD.Uint32x4.load(i8,1887541398 % (i8.length - (128 / i8.BYTES_PER_ELEMENT / 8 )))));
    return ((test0.caller) * ((test0.caller), (SIMD.Uint32x4.extractLane(initSimdVar4_Uint32x4,3) * (SIMD.Bool8x16.allTrue(SIMD.Uint8x16.greaterThanOrEqual(SIMD.Uint8x16.shiftLeftByScalar(initSimdVar6_Uint8x16,17),initSimdVar6_Uint8x16)) * SIMD.Uint16x8.extractLane(SIMD.Uint16x8.shiftRightByScalar(SIMD.Uint16x8.add(initSimdVar5_Uint16x8,initSimdVar5_Uint16x8),9),3) + SIMD.Uint32x4.extractLane(SIMD.Uint32x4.check(SIMD.Uint32x4.fromFloat32x4(SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(543754930,1475247953,4110849948,1465038070)))),3)) - (ary.unshift((+     (shouldBailout ? leaf() : leaf())), arrObj0[(((SIMD.Uint32x4.extractLane(initSimdVar4_Uint32x4,1) >= 0 ? SIMD.Uint32x4.extractLane(initSimdVar4_Uint32x4,1) : 0)) & 0XF)], SIMD.Int32x4.extractLane(initSimdVar1_Int32x4,2), SIMD.Bool16x8.allTrue(initSimdVar8_Bool16x8), (argMath0 >>>= SIMD.Bool16x8.extractLane(SIMD.Int16x8.greaterThanOrEqual(SIMD.Int16x8.add(initSimdVar2_Int16x8,initSimdVar2_Int16x8),SIMD.Int16x8.subSaturate(SIMD.Int16x8.neg(SIMD.Int16x8.add(initSimdVar2_Int16x8,initSimdVar2_Int16x8)),SIMD.Int16x8.fromUint32x4Bits(SIMD.Uint32x4(760523360,1044323417,602619037,2627186729)))),3)), SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,1), SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,1), SIMD.Bool8x16.anyTrue(SIMD.Bool8x16.xor(initSimdVar9_Bool8x16,initSimdVar9_Bool8x16)), SIMD.Float32x4.extractLane(SIMD.Float32x4.maxNum(initSimdVar0_Float32x4,SIMD.Float32x4.replaceLane(initSimdVar0_Float32x4,3,1073741823)),1)))), (SIMD.Uint32x4.extractLane(SIMD.Uint32x4.check(SIMD.Uint32x4.fromFloat32x4(SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(543754930,1475247953,4110849948,1465038070)))),3) * ((- SIMD.Int16x8.extractLane(SIMD.Int16x8.addSaturate(initSimdVar2_Int16x8,initSimdVar2_Int16x8),0)) - (SIMD.Uint32x4.extractLane(initSimdVar4_Uint32x4,3) * (SIMD.Bool8x16.allTrue(SIMD.Uint8x16.greaterThanOrEqual(SIMD.Uint8x16.shiftLeftByScalar(initSimdVar6_Uint8x16,17),initSimdVar6_Uint8x16)) * SIMD.Uint16x8.extractLane(SIMD.Uint16x8.shiftRightByScalar(SIMD.Uint16x8.add(initSimdVar5_Uint16x8,initSimdVar5_Uint16x8),9),3) + SIMD.Uint32x4.extractLane(SIMD.Uint32x4.check(SIMD.Uint32x4.fromFloat32x4(SIMD.Float32x4.fromUint32x4(SIMD.Uint32x4(543754930,1475247953,4110849948,1465038070)))),3)) - (ary.unshift((+     (shouldBailout ? leaf() : leaf())), arrObj0[(((SIMD.Uint32x4.extractLane(initSimdVar4_Uint32x4,1) >= 0 ? SIMD.Uint32x4.extractLane(initSimdVar4_Uint32x4,1) : 0)) & 0XF)], SIMD.Int32x4.extractLane(initSimdVar1_Int32x4,2), SIMD.Bool16x8.allTrue(initSimdVar8_Bool16x8), (argMath0 >>>= SIMD.Bool16x8.extractLane(SIMD.Int16x8.greaterThanOrEqual(SIMD.Int16x8.add(initSimdVar2_Int16x8,initSimdVar2_Int16x8),SIMD.Int16x8.subSaturate(SIMD.Int16x8.neg(SIMD.Int16x8.add(initSimdVar2_Int16x8,initSimdVar2_Int16x8)),SIMD.Int16x8.fromUint32x4Bits(SIMD.Uint32x4(760523360,1044323417,602619037,2627186729)))),3)), SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,1), SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,1), SIMD.Bool8x16.anyTrue(SIMD.Bool8x16.xor(initSimdVar9_Bool8x16,initSimdVar9_Bool8x16)), SIMD.Float32x4.extractLane(SIMD.Float32x4.maxNum(initSimdVar0_Float32x4,SIMD.Float32x4.replaceLane(initSimdVar0_Float32x4,3,1073741823)),1)))))), arguments[(((((shouldBailout ? (arguments[(((SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,0)) >= 0 ? ( SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,0)) : 0) & 0xF)] = 'x') : undefined ), SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,0)) >= 0 ? SIMD.Float32x4.extractLane(initSimdVar0_Float32x4,0) : 0)) & 0XF)]) - (test0.caller));
  };
  var func1 = function(argMath1){
    return (((shouldBailout ? func0 = func0 : 1), func0((arrObj0.prop0 >>= arguments[(17)]))) == arrObj0[(((((shouldBailout ? (arrObj0[((((/a/ instanceof ((typeof func0 == 'function' ) ? func0 : Object))) >= 0 ? ( (/a/ instanceof ((typeof func0 == 'function' ) ? func0 : Object))) : 0) & 0xF)] = 'x') : undefined ), (/a/ instanceof ((typeof func0 == 'function' ) ? func0 : Object))) >= 0 ? (/a/ instanceof ((typeof func0 == 'function' ) ? func0 : Object)) : 0)) & 0XF)]);
  };
  var func2 = function(argMath2){
    if(shouldBailout){
      return  'somestring'
    }
    return argMath2;
  };
  var func3 = function(argMath3,argMath4,argMath5,argMath6){
    return SIMD.Uint32x4.extractLane(SIMD.Uint32x4.shiftLeftByScalar(SIMD.Uint32x4.and(SIMD.Uint32x4.not(initSimdVar4_Uint32x4,initSimdVar4_Uint32x4),initSimdVar4_Uint32x4),4),1);
  };
  var func4 = function(argMath7){
    var simdVar2_Int8x16 = SIMD.Int8x16.or(SIMD.Int8x16.splat(),SIMD.Int8x16.fromFloat32x4Bits(SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(251,-1616718043,-226880211,-255))));
    return arrObj0.length;
  };
  obj0.method0 = func0;
  obj0.method1 = obj0.method0;
  obj1.method0 = func3;
  obj1.method1 = obj0.method1;
  arrObj0.method0 = obj0.method0;
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
  var IntArr0 = [5717358252208901120,-940233652];
  var IntArr1 = [];
  var FloatArr0 = [];
  var VarArr0 = new Array(obj0,-1435609178,-227,2.2962890857547E+18,9.0546406489471E+18,-243843201.9,195372214,65535,-1472407297080871936,447273857.1,-2147483648,-7.60409543613813E+17);
  var a = -811258519;
  var b = 1006298385;
  var c = -36;
  var d = -2147483647;
  var e = 980984808;
  var f = 8.84043019732881E+18;
  var g = 536184828;
  var h = 4294967295;
  arrObj0[0] = 5.26250192737421E+18;
  arrObj0[1] = 8.79055556503428E+18;
  arrObj0[2] = -806997813.9;
  arrObj0[3] = -122746411;
  arrObj0[4] = -2147483649;
  arrObj0[5] = 4.87217133636867E+18;
  arrObj0[6] = 4294967295;
  arrObj0[7] = 249618123;
  arrObj0[8] = -1225079138;
  arrObj0[9] = -1655445923;
  arrObj0[10] = 8.91552238021502E+18;
  arrObj0[11] = 4294967295;
  arrObj0[12] = -717576038.9;
  arrObj0[13] = -1004831875;
  arrObj0[14] = -1087380478;
  arrObj0[arrObj0.length-1] = -1584311712.9;
  arrObj0.length = makeArrayLength(-823077889.9);
  ary[0] = 204;
  ary[1] = 1010447895;
  ary[2] = 694103950;
  ary[3] = 255;
  ary[4] = 3.76364403121017E+18;
  ary[5] = -1779378495.9;
  ary[6] = 2147483647;
  ary[7] = -7.30621963792457E+18;
  ary[8] = -1073741824;
  ary[9] = 174;
  ary[10] = 4294967295;
  ary[11] = -422111779;
  ary[12] = 2104067459;
  ary[13] = -1758183873;
  ary[14] = -114;
  ary[ary.length-1] = 103;
  ary.length = makeArrayLength(-131);
  protoObj0 = Object.create(obj0);
  protoObj1 = Object.create(obj1);
  var initSimdVar1_Int32x4_alias = initSimdVar1_Int32x4;;
  var initSimdVar3_Int8x16_alias = initSimdVar3_Int8x16;;
  var initSimdVar5_Uint16x8_alias = initSimdVar5_Uint16x8;;
  var initSimdVar8_Bool16x8_alias = initSimdVar8_Bool16x8;;
  var aliasOfi32 = i32;;
  this.prop0 = undefined;
  this.prop1 = 219586969;
  obj0.prop0 = 645099644;
  obj0.prop1 = -75;
  obj0.length = makeArrayLength(127);
  protoObj0.prop0 = -1360912949;
  protoObj0.prop1 = -4.85384437578292E+16;
  protoObj0.length = makeArrayLength(67574376);
  obj1.prop0 = 95;
  obj1.prop1 = -787768925;
  obj1.length = makeArrayLength(244);
  protoObj1.prop0 = -134;
  protoObj1.prop1 = 242;
  protoObj1.length = makeArrayLength(430375081.1);
  arrObj0.prop0 = 909690905.1;
  arrObj0.prop1 = -2083962043.9;
  arrObj0.length = makeArrayLength(-1501694975);
  IntArr1[0] = -146;
  IntArr1[3] = 93;
  IntArr1[1] = 9.12123118977903E+18;
  IntArr1[2] = -2.5019158920087E+18;
  FloatArr0[1] = 5.77684199078761E+18;
  FloatArr0[0] = -2147483647;
  var simdVar3_Float32x4 = SIMD.Float32x4.check(SIMD.Float32x4.fromUint32x4Bits(SIMD.Uint32x4(2635231271,2210339166,918277437,3811698519)));
  var uniqobj0 = [obj0, protoObj0, protoObj0, arrObj0, protoObj0, arrObj0];
  var uniqobj1 = uniqobj0[__counter%uniqobj0.length];
  uniqobj1.method0((IntArr0.reverse()));
  
  function func5 (arg0) {
    this.prop0 = arg0;
  }
  var uniqobj2 = new func5(ary[(14)]);
  var __loopSecondaryVar0_0 = loopInvariant;
  for(var __loopvar0 = loopInvariant + 3;(__loopvar0 >= loopInvariant - -1);) {
    __loopSecondaryVar0_0 += 3;
    if (__loopSecondaryVar0_0 > loopInvariant + 12) break;
    __loopvar0--;
  }
  if(((obj1.length >= protoObj1.length)&&(protoObj0.length < g))) {
    var simdVar4_Float32x4 = SIMD.Float32x4.reciprocalSqrtApproximation(SIMD.Float32x4.check(initSimdVar0_Float32x4),initSimdVar0_Float32x4);
    var simdVar5_Uint8x16 = SIMD.Uint8x16.fromFloat32x4Bits(SIMD.Float32x4.fromInt32x4(SIMD.Int32x4(-197,-954581051,-1288378941,-352456084)));
    GiantPrintArray.push('protoObj1.prop1 = ' + (protoObj1.prop1|0));
    var simdVar6_Bool = SIMD.Bool32x4.anyTrue(SIMD.Bool32x4.not(SIMD.Uint32x4.lessThan(SIMD.Uint32x4.replaceLane(SIMD.Uint32x4.load(i32,262471981 % (i32.length - (128 / i32.BYTES_PER_ELEMENT / 8 ))),0,1819068541),SIMD.Uint32x4.check(initSimdVar4_Uint32x4)),initSimdVar7_Bool32x4));
    var simdVar7_Float32x4 = SIMD.Float32x4.reciprocalSqrtApproximation(initSimdVar0_Float32x4,simdVar4_Float32x4);
  }
  else {
    var simdVar8_Int8x16 = SIMD.Int8x16.neg(initSimdVar3_Int8x16);
    (function(){
    })();
  }
  protoObj1.length= makeArrayLength(((arrObj0.length === arrObj0.length)||(protoObj0.prop1 <= h)));
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
  WScript.Echo('uniqobj2.prop0 = ' + (uniqobj2.prop0|0));
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
  WScript.Echo('simdVar3_Float32x4 = ' + (simdVar3_Float32x4.toString()));
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
  
  for (var i = 0; i < GiantPrintArray.length; i++) {
  WScript.Echo(GiantPrintArray[i]);
  };
  
  
  WScript.Echo('sumOfary = ' +  ary.slice(0, 23).reduce(function(prev, curr) {{ return prev + curr; }},0));
  
  WScript.Echo('subset_of_ary = ' +  ary.slice(0, 11));;  // <----------------                                                   ERROR HERE
  
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

// run code with bailouts enabled
shouldBailout = true;
test0();


// Baseline total processor time: 00:00:00.3125000
// Test total processor time: 00:00:00.4843750
// 
// Addl Dump Info: 
//Baseline length = 15575

//Test length = 10129

//Baseline length = 15575

//Test length = 10129

//Baseline length = 15575

//Test length = 10129

// 
// Baseline output:
// Skipping first 411 lines of output...
// initSimdVar4_Uint32x4 = SIMD.Uint32x4(1071188118, 1934777615, 983857782, 290639627)
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(47187, 56187, 35154, 31913, 29852, 47176, 6872, 54876)
// initSimdVar5_Uint16x8_alias = SIMD.Uint16x8(47187, 56187, 35154, 31913, 29852, 47176, 6872, 54876)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(139, 219, 120, 233, 234, 90, 75, 207, 60, 212, 18, 175, 123, 17, 222, 14)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(false, false, true, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, false, true, true, false, true, false, true)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, false, true, true, false, true, false, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(false, false, true, true, true, true, true, true, false, true, true, true, true, false, false, false)
// protoObj1.prop1 = 242
// sumOfary = NaN
// subset_of_ary = 100,,102,false,0,219,219,false,116,100,
// sumOfIntArr0 = 1127530004
// subset_of_IntArr0 = -940233652,2067763732
// sumOfIntArr1 = 195160164
// subset_of_IntArr1 = -146,1378377443,-1183217279,93
// sumOfFloatArr0 = 550216286
// subset_of_FloatArr0 = 0,550216287
// sumOfVarArr0 = 0[object Object]-1435609178-1616625117-243843201.91953722-438931326-1-268835303
// subset_of_VarArr0 = [object Object],-1435609178,-227,432017028,1419706214,-243843201.90000000,195372214,65535,-1544610117,447273857.10000000,-1
// 
// 
// Test output:
// Skipping first 242 lines of output...
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(139, 219, 120, 233, 234, 90, 75, 207, 60, 212, 18, 175, 123, 17, 222, 14)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(false, false, true, false)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, false, true, true, false, true, false, true)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, false, true, true, false, true, false, true)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(false, false, true, true, true, true, true, true, false, true, true, true, true, false, false, false)
// protoObj1.prop1 = 242
// sumOfary = NaN
// subset_of_ary = 100,,102,false,0,219,219,false,116,100,
// sumOfIntArr0 = 1127530004
// subset_of_IntArr0 = -940233652,2067763732
// sumOfIntArr1 = 195160164
// subset_of_IntArr1 = -146,1378377443,-1183217279,93
// sumOfFloatArr0 = 550216286
// subset_of_FloatArr0 = 0,550216287
// sumOfVarArr0 = 0[object Object]-1435609178-1616625117-243843201.91953722-438931326-1-268835303
// subset_of_VarArr0 = [object Object],-1435609178,-227,432017028,1419706214,-243843201.90000000,195372214,65535,-1544610117,447273857.10000000,-1
// ASSERTION 22912: (e:\nagy\git\chakra5\core\lib\Backend\GlobOptBailOut.cpp, line 683) (uint)sym->m_instrDef->GetArgOutCount( true) == origArgOutCount - this->currentBlock->globOptData.argOutCount + (instr->m_opcode == Js::OpCode::NewScObject || instr->m_opcode == Js::OpCode::NewScObjArray || instr->m_opcode == Js::OpCode::NewScObjectSpread || instr->m_opcode == Js::OpCode::NewScObjArraySpread)
//  Failure: ((uint)sym->m_instrDef->GetArgOutCount( true) == origArgOutCount - this->currentBlock->globOptData.argOutCount + (instr->m_opcode == Js::OpCode::NewScObject || instr->m_opcode == Js::OpCode::NewScObjArray || instr->m_opcode == Js::OpCode::NewScObjectSpread || instr->m_opcode == Js::OpCode::NewScObjArraySpread))
// FATAL ERROR: jshost.exe failed due to exception code c0000420
// 
// Reduced Switches:
