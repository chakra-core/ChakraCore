//Configuration: configs\SIMD.xml
//Testcase Number: 11272
//Switches:-simd128typespec -asmjs-  -PrintSystemException  -simdjs  -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport  -MaxLinearStringCaseCount:2 -MaxLinearIntCaseCount:2 -MinSwitchJumpTableSize:3 -force:polymorphicinlinecache -bgjit- -loopinterpretcount:1 -force:fieldcopyprop -force:fieldhoist -forceserialized -sse:3 -force:rejit -force:atom -force:ScriptFunctionWithInlineCache -off:trackintusage -off:DelayCapture -off:ParallelParse -force:fixdataprops -ForceArrayBTree -off:checkthis -off:aggressiveinttypespec -off:fefixedmethods -off:ArrayCheckHoist -off:lossyinttypespec -off:LoopCountBasedBoundCheckHoist -ValidateInlineStack -off:ArrayLengthHoist -off:BoundCheckHoist -off:BoundCheckElimination -off:bailonnoprofile -off:LdLenIntSpec -off:objtypespec  -off:ArrayLiteralFastPath -off:TypedArrayTypeSpec -off:ArrayMissingValueCheckHoist
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
  var loopInvariant = shouldBailout ? 12 : 2;
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
  var initSimdVar0_Float32x4 = SIMD.Float32x4(-149,828800656,-934593775,1073741823);
  var initSimdVar1_Int32x4 = SIMD.Int32x4(-718676762,-456100437,1809089054,781444359);
  var initSimdVar2_Int16x8 = SIMD.Int16x8(10379,28613,24309,3592,17467,27154,28383,31717);
  var initSimdVar3_Int8x16 = SIMD.Int8x16(64,117,100,4,2,1338674515,47,55,105,116,66,83,46,41,13,119);
  var initSimdVar4_Uint32x4 = SIMD.Uint32x4(2760154710,30561623,729515999,2829070869);
  var initSimdVar5_Uint16x8 = SIMD.Uint16x8(35356,3722,30378,41053,29566,54278,30031,59689);
  var initSimdVar6_Uint8x16 = SIMD.Uint8x16(213,38,68,214,251,63,154,184,236,17,127,232,179,101,108,237);
  var initSimdVar7_Bool32x4 = SIMD.Bool32x4(true,false,true,true);
  var initSimdVar8_Bool16x8 = SIMD.Bool16x8(true,false,false,true,false,true,true,false);
  var initSimdVar9_Bool8x16 = SIMD.Bool8x16(false,true,true,false,307363458,true,false,false,false,false,false,false,false,false,true,true);
  var func0 = function(argMath0,argMath1,argMath2,argMath3){
    var simdVar0_Int32x4 = SIMD.Int32x4.not(initSimdVar1_Int32x4,initSimdVar1_Int32x4);
    function func5 (){
      return 2009370204.1;
    }
    return SIMD.Uint16x8.extractLane(SIMD.Uint16x8.and(SIMD.Uint16x8.fromInt16x8Bits(SIMD.Int16x8(12976,3128,26355,11994,26360,1616,17746,24864)),initSimdVar5_Uint16x8_alias),2);
  };
  var func1 = function(){
    return (Function('') instanceof ((typeof Function == 'function' ) ? Function : Object));
  };
  var func2 = function(argMath4,argMath5,argMath6,argMath7){
    if((('prop0' in litObj1) + i8[(220) & 255])) {
      argMath7 +=SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16_alias,2);
      obj1.prop0 *=SIMD.Float32x4.extractLane(SIMD.Float32x4.sqrt(initSimdVar0_Float32x4_alias),0);
      var uniqobj0 = Object.create(protoObj1);
      obj7 = Object.create(protoObj1);
      if(shouldBailout){
        return  'somestring'
      }
    }
    else {
      argMath5 /=SIMD.Bool32x4.anyTrue(initSimdVar7_Bool32x4);
      if(shouldBailout){
        return  'somestring'
      }
      arrObj0.prop1 = (-- c);
    }
    return ('prop0' in litObj1);
  };
  var func3 = function(argMath8,argMath9,argMath10,argMath11){
    var uniqobj1 = [''];
    uniqobj1[__counter%uniqobj1.length].toString();
    return SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8_alias,0);
  };
  var func4 = function(argMath12,argMath13,argMath14,argMath15){
    return SIMD.Float32x4.extractLane(SIMD.Float32x4.fromInt32x4Bits(SIMD.Int32x4(882141262,924398660,-799593496,1696504498)),1);
  };
  obj0.method0 = func0;
  obj0.method1 = func0;
  obj1.method0 = func1;
  obj1.method1 = obj1.method0;
  arrObj0.method0 = func2;
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
  var IntArr0 = [-322509106,6065360096879089664,336758773,778452002,-903855595,3,152,0,-853813087,1711331434,-230,-2922610536106356736,7171602590659137536,-464431971,4294967297];
  var IntArr1 = new Array(-90,6237485009547411456,1045848063,-5614739868221666304,-115,-194,994483294,-2147483648,1410331031934708736,90381838,168053098);
  var FloatArr0 = [701108744,-162,-3,990613004,4294967297,-2104870702.9,-36,425038623,222178694.1,-525840383,-36645101,-1320731696.9,4294967297,65536,-3];
  var VarArr0 = [protoObj1,-19,-1602269509,34,4294967297,7.94588831119519E+18,-2147483649,761479928.1,6320019455743738880,9052437588806851584,-512910600,-8.07079163232295E+17,2];
  var a = -2018812065.9;
  var b = -41538590.9;
  var c = 2147483650;
  var d = -1435319366;
  var e = -229;
  var f = 1097358098.1;
  var g = -4949522.9;
  var h = -3;
  arrObj0[0] = -2147483649;
  arrObj0[1] = -1325790983;
  arrObj0[2] = 780131302.1;
  arrObj0[3] = 65535;
  arrObj0[4] = -42109843;
  arrObj0[5] = 1469493525.1;
  arrObj0[6] = 4294967295;
  arrObj0[7] = 150;
  arrObj0[8] = 76;
  arrObj0[9] = -1073741824;
  arrObj0[10] = -7.81880538044835E+17;
  arrObj0[11] = 1.37647413159174E+18;
  arrObj0[12] = 153;
  arrObj0[13] = 2.53007992773199E+18;
  arrObj0[14] = 1335660441;
  arrObj0[arrObj0.length-1] = -190;
  arrObj0.length = makeArrayLength(9.55982317110221E+17);
  ary[0] = 419859167;
  ary[1] = -777465978;
  ary[2] = 75;
  ary[3] = 161;
  ary[4] = -1173418449.9;
  ary[5] = -1993490590.9;
  ary[6] = 468159196;
  ary[7] = 550682877.1;
  ary[8] = 332983000.1;
  ary[9] = 149;
  ary[10] = 1049806682;
  ary[11] = -2032705837.9;
  ary[12] = -1;
  ary[13] = 1;
  ary[14] = -1073741824;
  ary[ary.length-1] = 1704093378.1;
  ary.length = makeArrayLength(2.29972378228467E+18);
  protoObj0 = Object.create(obj0);
  protoObj1 = Object.create(obj1);
  var initSimdVar0_Float32x4_alias = initSimdVar0_Float32x4;;
  var initSimdVar2_Int16x8_alias = initSimdVar2_Int16x8;;
  var initSimdVar5_Uint16x8_alias = initSimdVar5_Uint16x8;;
  var initSimdVar6_Uint8x16_alias = initSimdVar6_Uint8x16;;
  var initSimdVar8_Bool16x8_alias = initSimdVar8_Bool16x8;;
  var aliasOff32 = f32;;
  this.prop0 = -2088406371;
  this.prop1 = 1524769742.1;
  obj0.prop0 = -401372385.9;
  obj0.prop1 = 3.99703941291249E+18;
  obj0.length = makeArrayLength(-2147483646);
  protoObj0.prop0 = -5.1857824325672E+18;
  protoObj0.prop1 = -695961754;
  protoObj0.length = makeArrayLength(-1961631417.9);
  obj1.prop0 = 383167932;
  obj1.prop1 = -163;
  obj1.length = makeArrayLength(65536);
  protoObj1.prop0 = 233;
  protoObj1.prop1 = -222135039;
  protoObj1.length = makeArrayLength(-2147483647);
  arrObj0.prop0 = NaN;
  arrObj0.prop1 = -1;
  arrObj0.length = makeArrayLength(3.48096685789072E+18);
  obj0.method1.call(litObj1 , SIMD.Uint16x8.extractLane(initSimdVar5_Uint16x8,3), leaf, Object.create(obj0, {}), arrObj0);
  var simdVar1_Float32x4 = SIMD.Float32x4.max(initSimdVar0_Float32x4_alias,SIMD.Float32x4.div(SIMD.Float32x4.min(SIMD.Float32x4.check(initSimdVar0_Float32x4_alias),initSimdVar0_Float32x4),SIMD.Float32x4.sub(initSimdVar0_Float32x4_alias,initSimdVar0_Float32x4)));
  var simdVar2_Bool = SIMD.Bool32x4.allTrue(initSimdVar7_Bool32x4);
  if((SIMD.Bool8x16.extractLane(initSimdVar9_Bool8x16,1) ? (protoObj0.method0.call(protoObj0 , SIMD.Bool8x16.extractLane(initSimdVar9_Bool8x16,1), leaf, SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,0), arrObj0) - f64[(SIMD.Uint8x16.extractLane(initSimdVar6_Uint8x16,1)) & 255]) : SIMD.Bool32x4.allTrue(SIMD.Bool32x4.replaceLane(initSimdVar7_Bool32x4,2,false)))) {
  }
  else {
  }
  var simdVar3_Float32x4 = SIMD.Float32x4.fromInt16x8Bits(SIMD.Int16x8(8070,6033,845,22259,31903,18774,710,20727));
  var simdVar4_Uint8x16 = SIMD.Uint8x16.and(SIMD.Uint8x16.sub(initSimdVar6_Uint8x16,SIMD.Uint8x16.fromInt16x8Bits(SIMD.Int16x8(10885,6051,22026,30366,24263,9030,14474,16823))),SIMD.Uint8x16.fromInt8x16Bits(SIMD.Int8x16(53,122,7,101,27,18,110,45,28,68,102,85,124,1,50,118)));
  var uniqobj2 = [obj0, protoObj0, protoObj0, protoObj0];
  var uniqobj3 = uniqobj2[__counter%uniqobj2.length];
  uniqobj3.method1((protoObj1.length *= SIMD.Bool8x16.anyTrue(SIMD.Uint8x16.equal(initSimdVar6_Uint8x16,simdVar4_Uint8x16))),leaf,obj1.length,arrObj0);
  WScript.Echo('a = ' + (a|0));
  WScript.Echo('b = ' + (b|0));
  WScript.Echo('c = ' + (c|0));
  WScript.Echo('d = ' + (d|0));
  WScript.Echo('e = ' + (e|0));
  WScript.Echo('f = ' + (f|0));
  WScript.Echo('g = ' + (g|0));
  WScript.Echo('h = ' + (h|0));
  WScript.Echo('simdVar2_Bool = ' + (simdVar2_Bool|0));
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
  WScript.Echo('simdVar1_Float32x4 = ' + (simdVar1_Float32x4.toString()));
  WScript.Echo('simdVar3_Float32x4 = ' + (simdVar3_Float32x4.toString()));
  WScript.Echo('initSimdVar1_Int32x4 = ' + (initSimdVar1_Int32x4.toString()));
  WScript.Echo('initSimdVar2_Int16x8 = ' + (initSimdVar2_Int16x8.toString()));
  WScript.Echo('initSimdVar2_Int16x8_alias = ' + (initSimdVar2_Int16x8_alias.toString()));
  WScript.Echo('initSimdVar3_Int8x16 = ' + (initSimdVar3_Int8x16.toString()));
  WScript.Echo('initSimdVar4_Uint32x4 = ' + (initSimdVar4_Uint32x4.toString()));
  WScript.Echo('initSimdVar5_Uint16x8 = ' + (initSimdVar5_Uint16x8.toString()));
  WScript.Echo('initSimdVar5_Uint16x8_alias = ' + (initSimdVar5_Uint16x8_alias.toString()));
  WScript.Echo('initSimdVar6_Uint8x16 = ' + (initSimdVar6_Uint8x16.toString()));
  WScript.Echo('initSimdVar6_Uint8x16_alias = ' + (initSimdVar6_Uint8x16_alias.toString()));
  WScript.Echo('simdVar4_Uint8x16 = ' + (simdVar4_Uint8x16.toString()));
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


// Baseline total processor time: 00:00:00.3125000
// Test total processor time: 00:00:00.4687500
// 
// Addl Dump Info: 
//Baseline length = 15100

//Test length = 11530

//Baseline length = 15100

//Test length = 11530

//Baseline length = 15100

//Test length = 11530

// 
// Baseline output:
// Skipping first 333 lines of output...
// initSimdVar5_Uint16x8 = SIMD.Uint16x8(35356, 3722, 30378, 41053, 29566, 54278, 30031, 59689)
// initSimdVar5_Uint16x8_alias = SIMD.Uint16x8(35356, 3722, 30378, 41053, 29566, 54278, 30031, 59689)
// initSimdVar6_Uint8x16 = SIMD.Uint8x16(213, 38, 68, 214, 251, 63, 154, 184, 236, 17, 127, 232, 179, 101, 108, 237)
// initSimdVar6_Uint8x16_alias = SIMD.Uint8x16(213, 38, 68, 214, 251, 63, 154, 184, 236, 17, 127, 232, 179, 101, 108, 237)
// simdVar4_Uint8x16 = SIMD.Uint8x16(16, 120, 1, 37, 17, 0, 108, 0, 4, 0, 32, 69, 40, 1, 48, 36)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, false, true, true)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, false, false, true, false, true, true, false)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, false, false, true, false, true, true, false)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(false, true, true, false, true, true, false, false, false, false, false, false, false, false, true, true)
// sumOfary = -1451496171.40000050
// subset_of_ary = 419859167,-777465978,75,161,-1173418449.90000000,-1993490590.90000000,468159196,550682877.10000000,332983000.10000000,149,1049806682
// sumOfIntArr0 = 2037895496
// subset_of_IntArr0 = -322509106,27759040,336758773,778452002,-903855595,3,152,0,-853813087,1711331434,-230
// sumOfIntArr1 = 1173502468
// subset_of_IntArr1 = -90,990232356,1045848063,-325751892,-115,-194,994483294,-1,357739828,90381838,168053098
// sumOfFloatArr0 = 6940851107.30000100
// subset_of_FloatArr0 = 701108744,-162,-3,990613004,3,-2104870702.90000000,-36,425038623,222178694.10000000,-525840383,-36645101
// sumOfVarArr0 = 0[object Object]-19-1230938288-613996289-512910600-1825595510
// subset_of_VarArr0 = [object Object],-19,-1602269509,34,3,1111667365,-2,761479928.10000000,2022978668,1803242510,-512910600
// 
// 
// Test output:
// Skipping first 248 lines of output...
// initSimdVar6_Uint8x16_alias = SIMD.Uint8x16(213, 38, 68, 214, 251, 63, 154, 184, 236, 17, 127, 232, 179, 101, 108, 237)
// simdVar4_Uint8x16 = SIMD.Uint8x16(16, 120, 1, 37, 17, 0, 108, 0, 4, 0, 32, 69, 40, 1, 48, 36)
// initSimdVar7_Bool32x4 = SIMD.Bool32x4(true, false, true, true)
// initSimdVar8_Bool16x8 = SIMD.Bool16x8(true, false, false, true, false, true, true, false)
// initSimdVar8_Bool16x8_alias = SIMD.Bool16x8(true, false, false, true, false, true, true, false)
// initSimdVar9_Bool8x16 = SIMD.Bool8x16(false, true, true, false, true, true, false, false, false, false, false, false, false, false, true, true)
// sumOfary = -1451496171.40000050
// subset_of_ary = 419859167,-777465978,75,161,-1173418449.90000000,-1993490590.90000000,468159196,550682877.10000000,332983000.10000000,149,1049806682
// sumOfIntArr0 = 2037895496
// subset_of_IntArr0 = -322509106,27759040,336758773,778452002,-903855595,3,152,0,-853813087,1711331434,-230
// sumOfIntArr1 = 1173502468
// subset_of_IntArr1 = -90,990232356,1045848063,-325751892,-115,-194,994483294,-1,357739828,90381838,168053098
// sumOfFloatArr0 = 6940851107.30000100
// subset_of_FloatArr0 = 701108744,-162,-3,990613004,3,-2104870702.90000000,-36,425038623,222178694.10000000,-525840383,-36645101
// sumOfVarArr0 = 0[object Object]-19-1230938288-613996289-512910600-1825595510
// subset_of_VarArr0 = [object Object],-19,-1602269509,34,3,1111667365,-2,761479928.10000000,2022978668,1803242510,-512910600
// ASSERTION 17088: (e:\nagy\git\chakra5\core\lib\Backend\GlobOptSimd128.cpp, line 38) valueType.IsSimd128()
//  Failure: (valueType.IsSimd128())
// FATAL ERROR: jshost.exe failed due to exception code c0000420
// 
// Reduced Switches:
