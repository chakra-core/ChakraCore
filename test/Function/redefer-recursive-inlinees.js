//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test that recursive inlinees are properly tracked by redeferral. Run with -recyclerconcurrentstress to trigger frequent redeferral.

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
  var loopInvariant = shouldBailout ? 5 : 12;
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
  var func0 = function(argMath0,argMath1 = (-148 >> (((! this.prop0) | 3) << ary[(((Object.prototype.prop1 >= 0 ? Object.prototype.prop1 : 0)) & 0XF)])),argMath2 = (((new RegExp('xyz')) instanceof ((typeof Number == 'function' ) ? Number : Object)), ary[(((Object.prototype.prop1 >= 0 ? Object.prototype.prop1 : 0)) & 0XF)], (ary.unshift(ary[(((leaf() >= 0 ? leaf() : 0)) & 0XF)], argMath1, (leaf() * (argMath0 + Math.round(-1.62608014185702E+18))), leaf(), argMath1))),argMath3){
    if(shouldBailout){
      return  'somestring'
    }
    litObj1.prop0=((argMath0 != obj0.prop1)&&(this.prop1 >= argMath2));
    return (~ 1815727712.1);
  };
  var func1 = function(){
    var __loopvar1 = loopInvariant + 8,__loopSecondaryVar1_0 = loopInvariant,__loopSecondaryVar1_1 = loopInvariant - 3;
    LABEL0: 
    for (var _strvar0 in f32) {
      if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
      __loopvar1 -= 3;
      __loopSecondaryVar1_0 += 3;
      __loopSecondaryVar1_1++;
      f32[_strvar0] = -5.39327267986099E+18;
      var func5 = function(){
        var __loopvar3 = loopInvariant - 6,__loopSecondaryVar3_0 = loopInvariant - 3;
        LABEL0: 
        LABEL1: 
        do {
          if (__loopvar3 > loopInvariant) break;
          if (__loopSecondaryVar3_0 === loopInvariant) break;
          __loopvar3 += 2;
          __loopSecondaryVar3_0++;
        } while((('caller' & (func0(obj1,(! b),-72,leaf) * func0.call(Object.prototype , litObj0, (_strvar0 -= 1738924735.1), uic8[(_strvar0) & 255], leaf) + 'caller'))))
        return func0(litObj1,'caller','caller',leaf);
      };
      func5();
    }
    function func6 (){
      GiantPrintArray.push('obj0.prop2 = ' + (obj0.prop2|0));
      if(shouldBailout){
        return  'somestring'
      }
      var func7 = function(argMath4){
        return -738581528;
      };
      return func7.call(obj0 , /(?!.(\b\s)+.\b\W(?=\b.))/gi);
    }
    return uic8[(obj1.prop2) & 255];
  };
  var func2 = function(...argArr5){
    return arguments[((((argArr5.reverse()) >= 0 ? (argArr5.reverse()) : 0)) & 0XF)];
  };
  var func3 = function(){
    var fPolyProp = function (o) {
      if (o!==undefined) {
        WScript.Echo(o.prop0 + ' ' + o.prop1 + ' ' + o.prop2);
      }
    };
    fPolyProp(litObj0); 
    fPolyProp(litObj1); 

    obj0 = obj0;
    protoObj1.length= makeArrayLength((- ((protoObj1.prop0 > arrObj0.prop1)||(obj1.prop2 < protoObj0.prop0))));
    return obj1.prop1;
  };
  var func4 = function(){
    obj1.prop0=arrObj0[(12)];
    eval("'use strict';");
    return ui16.length;
  };
  obj0.method0 = func0;
  obj0.method1 = func2;
  obj1.method0 = func1;
  obj1.method1 = func1;
  arrObj0.method0 = func4;
  arrObj0.method1 = func1;
  Object.prototype.method0 = arrObj0.method0;
  Object.prototype.method1 = func4;
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
  var IntArr0 = new Array(65537,-252,-7534443540062868480,-139912883,183,4294967295,-8,-3534281615684059136,-2147483646);
  var IntArr1 = new Array();
  var FloatArr0 = [-1.65663272360615E+18,65537,-9.18570115267099E+18];
  var VarArr0 = [protoObj1,8.1039954840962E+18,-2859983991833802752,-162068960,186382691,489218302.1,-2147483649,-287597399,6.50325114561036E+18,-975199350.9,0];
  var a = 254;
  var b = -265919721;
  arrObj0[0] = -154;
  arrObj0[1] = 74;
  arrObj0[2] = 2;
  arrObj0[3] = 493847347.1;
  arrObj0[4] = -1220404880;
  arrObj0[5] = -1.79800004203085E+17;
  arrObj0[6] = -1116565737;
  arrObj0[7] = -9.11168563055195E+18;
  arrObj0[8] = -1325194954.9;
  arrObj0[9] = 1950420840;
  arrObj0[10] = -127;
  arrObj0[11] = 21;
  arrObj0[12] = 2147483650;
  arrObj0[13] = 65537;
  arrObj0[14] = -6.30345011183866E+18;
  arrObj0[arrObj0.length-1] = 138;
  arrObj0.length = makeArrayLength(2);
  ary[0] = -680817128;
  ary[1] = -527813232;
  ary[2] = -832088952;
  ary[3] = 99;
  ary[4] = 980156429.1;
  ary[5] = 0;
  ary[6] = -1920459204;
  ary[7] = -1073741824;
  ary[8] = 191;
  ary[9] = -93163429;
  ary[10] = -7.55165157285184E+18;
  ary[11] = -2147483646;
  ary[12] = -2147483648;
  ary[13] = 210;
  ary[14] = 2147483647;
  ary[ary.length-1] = 5.65030043299318E+18;
  ary.length = makeArrayLength(-997651294);
  Object.prototype.prop0 = -2147483646;
  Object.prototype.prop1 = -1435611367.9;
  Object.prototype.prop2 = 2147483650;
  Object.prototype.length = makeArrayLength(1167596854);
  var protoObj0 = Object.create(obj0);
  var protoObj1 = Object.create(obj1);
  var aliasOfi8 = i8;;
  this.prop0 = -2125793252.9;
  this.prop1 = 0;
  obj0.prop0 = 9.18874020750804E+18;
  obj0.prop1 = -1263508179.9;
  obj0.prop2 = -984620769.9;
  obj0.length = makeArrayLength(-1.33525439363332E+18);
  protoObj0.prop0 = -498747962;
  protoObj0.prop1 = 4294967295;
  protoObj0.prop2 = 745478336;
  protoObj0.length = makeArrayLength(-5330384.9);
  obj1.prop0 = -1306917901.9;
  obj1.prop1 = -1051552568;
  obj1.prop2 = -1062743416;
  obj1.length = makeArrayLength(219466416.1);
  protoObj1.prop0 = 596169680;
  protoObj1.prop1 = 251;
  protoObj1.prop2 = 957019650.1;
  protoObj1.length = makeArrayLength(3);
  arrObj0.prop0 = -93;
  arrObj0.prop1 = 92507008.1;
  arrObj0.prop2 = 65537;
  arrObj0.length = makeArrayLength(1250292512.1);
  Object.prototype.prop0 = 2043613941;
  Object.prototype.prop1 = -973764540;
  Object.prototype.prop2 = -7.58318524387149E+18;
  Object.prototype.length = makeArrayLength(5.44936228952327E+18);
  IntArr1[3] = -367883042;
  IntArr1[0] = -1336332573;
  IntArr1[IntArr1.length] = -637078889;
  IntArr1[2] = 5.63258304547574E+18;
  FloatArr0[5] = -119993699;
  FloatArr0[3] = -8.92058227174718E+16;
  FloatArr0[FloatArr0.length] = 65536;
  FloatArr0[7] = -1915497990.9;
  FloatArr0[4] = 979829498;
  var proxyHandler = {};
  proxyHandler['deleteProperty'] = function(target, property) {
  WScript.Echo('DeleteProperty trap called');
  return Reflect.deleteProperty(target, property);
  }

  proxyHandler['definePropery'] = function(target, property, descriptor) {
  WScript.Echo('DefinePropery trap called');
  Reflect.defineProperty(target, property, descriptor);
  }

  proxyHandler['apply'] = function(target, thisArg, argumentsList) {
  WScript.Echo('Apply trap called');
  return Reflect.apply(target, thisArg, argumentsList);
  }

  proxyHandler['construct'] = function(target, argumentsList) {
  WScript.Echo('Construct trap called');
  return Reflect.construct(target, argumentsList);
  }

;
  Object.prototype.prop0 = 'caller';
  var uniqobj0 = [obj1, protoObj1];
  var uniqobj1 = uniqobj0[__counter%uniqobj0.length];
  uniqobj1.method1();
  var func8 = function(argMath6){
    var __loopvar1 = loopInvariant - 6,__loopSecondaryVar1_0 = loopInvariant;
    LABEL0: 
    while((argMath6)) {
      __loopvar1 += 2;
      __loopSecondaryVar1_0++;
      if (__loopvar1 == loopInvariant + 2) break;
      var uniqobj2 = [protoObj0, obj0, obj0];
      var uniqobj3 = uniqobj2[__counter%uniqobj2.length];
      uniqobj3.method0(obj0,protoObj0.prop0,Math.cos(this.prop0),leaf);
      function func9 () {
      }
      var uniqobj4 = new func9();
    }
    return arrObj0.method1.call(litObj0 );
  };
  var __loopvar0 = loopInvariant;
  LABEL0: 
  for(;;) {
    __loopvar0--;
    if (__loopvar0 < loopInvariant - 4) break;
    //Snippet:getter9.ecs  flag getter setter to data property 
    //<no-strict> : This tag indicates that this snippet should not be inserted in a strict mode test case. This is not a comment for Exprgen engine.
    
    function __getterfoo9(obj1) {
        GiantPrintArray.push(obj1.__getterprop9);
    }
    // CSE when used within while-loop
    var v0 = 0; // TODO: check with louis if it matters to put this stmt after next stmt.
    var v1='caller';
    while(v0 < 5) {
    	v1='caller';
    	v1-=v0;
    	v0++;
    }
    protoObj0.prop0='caller'+v1;
    var __getterCtor9 = function () { }
    var __getterproto9 = {};
    __getterCtor9.prototype = __getterproto9;
    var inst = new __getterCtor9();
    var __getterdesc9 = { get: function () { GiantPrintArray.push("getter"); 
                         return _Value }, set: function (x) 
                         { _Value = x; },configurable:true }
    Object.defineProperty(__getterproto9, "__getterprop9", __getterdesc9);
    __getterproto9.__getterprop9 = ((Object.prototype.length * (typeof(Object.prototype.prop0)  == 'string')  + ui8[(__loopvar0 - 1) & 255]) % i8[(loopInvariant) & 255]);
    try {
      var __loopvar3 = loopInvariant - 3;
      for (var _strvar0 of ui8) {
        if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
        if (__loopvar3 >= loopInvariant + -1) break;
        __loopvar3++;
        //Snippet 1: basic inlining test
        obj1.prop0 = (function(x,y,z) {
          var uniqobj5 = [arrObj0, protoObj1, obj1, protoObj1];
          var uniqobj6 = uniqobj5[__counter%uniqobj5.length];
          uniqobj6.method0();
          GiantPrintArray.push("Snippet 1: ",x,y,z);
          return obj1.method1.call(arrObj0 );
        })((ui32[(140) & 255] ? ('caller' ? 'caller' : (Math.sin(_strvar0) * ((typeof(Object.prototype.prop0)  == 'string')  + (typeof(obj1.prop0)  == 'object') ))) : ui8[(-2123989455) & 255]),(typeof(obj1.prop0)  == 'undefined') ,(({} instanceof ((typeof Array == 'function' ) ? Array : Object)) * obj1.prop2 + ((b < _strvar0)&&(_strvar0 >= protoObj1.prop1))));
        
        function func10 (){
        'use strict';;
          return obj0.method1.call(protoObj0 , FloatArr0);
        }
      }
      obj0.method1.call(obj0 , ary);
      var __loopvar3 = loopInvariant;
      for (var _strvar0 in f64) {
        if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
        if (__loopvar3 >= loopInvariant + 2) break;
        __loopvar3++;
      }
      var __loopvar3 = loopInvariant,__loopSecondaryVar3_0 = loopInvariant;
      LABEL1: 
      for (var _strvar0 in i8) {
        if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
        __loopSecondaryVar3_0 += 4;
        __loopvar3--;
        i8[_strvar0] = (Infinity * 2123015056);
        (function(argMath7,argMath8 = ('prop0' in protoObj0),...argArr9){
          __loopSecondaryVar3_0 += 3;;
          func3();
        })(IntArr0,protoObj1.prop1,FloatArr0);
      }
    } catch(ex) {
      WScript.Echo(ex.message);
    } finally {
      protoObj0.method0=((typeof(protoObj1.prop2)  != 'string')  ? ((obj1.prop0 = obj0.prop0) ? (typeof(obj1.prop1)  == 'string')  : (this.prop0 *= i8[(loopInvariant) & 255])) : 'caller');
    }
    __getterfoo9(inst);
    var func11 = function(argMath10,argMath11 = 'caller'){
      return (Object.prototype.length !== arrObj0.prop1);
    };
    if (shouldBailout)
        Object.defineProperty(__getterproto9,
                  "__getterprop9",{value:10,configurable:true });
    __getterfoo9(inst);
    
    var __loopvar1 = loopInvariant - 6;
    for(; protoObj1.prop0 < ((a-- )); protoObj1.prop0++) {
      __loopvar1 += 2;
      if (__loopvar1 == loopInvariant + 2) break;
      var fPolyProp = function (o) {
        if (o!==undefined) {
          WScript.Echo(o.prop0 + ' ' + o.prop1 + ' ' + o.prop2);
        }
      };
      fPolyProp(litObj0); 
      fPolyProp(litObj1); 

      var __loopvar2 = loopInvariant,__loopSecondaryVar2_0 = loopInvariant,__loopSecondaryVar2_1 = loopInvariant - 6;
      for(;;) {
        __loopSecondaryVar2_0 -= 4;
        __loopSecondaryVar2_1 += 2;
        if (__loopvar2 === loopInvariant + 12) break;
        __loopvar2 += 4;
        function func12 () {
        }
        obj6 = new func12();
        var __loopvar3 = loopInvariant + 3,__loopSecondaryVar3_0 = loopInvariant;
        LABEL1: 
        do {
          __loopSecondaryVar3_0 += 2;
          if (__loopvar3 < loopInvariant) break;
          __loopvar3--;
          var __loopvar4 = loopInvariant - 10;
          LABEL2: 
          while(((Object.prototype.prop1 &= (((b > protoObj1.length)&&(obj0.prop1 === obj0.prop1)) instanceof ((typeof Boolean == 'function' ) ? Boolean : Object))))) {
            if (__loopvar4 >= loopInvariant + -3) break;
            __loopvar4 += 3;
          }
          var __loopSecondaryVar4_0 = loopInvariant - 6;
          for(var __loopvar4 = loopInvariant;;) {
            if (__loopvar4 >= loopInvariant + 8) break;
            __loopvar4 += 4;
            __loopSecondaryVar4_0 += 2;
            obj7 = new func12();
            var __loopvar5 = loopInvariant;
            LABEL2: 
            for (var _strvar0 of i16) {
              if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
              if (__loopvar5 >= loopInvariant + 2) break;
              __loopvar5++;
              var uniqobj7 = [protoObj1, arrObj0];
              var uniqobj8 = uniqobj7[__counter%uniqobj7.length];
              uniqobj8.method1();
            }
            var uniqobj9 = [protoObj1, obj1, obj1, obj1, obj1];
            uniqobj9[__counter%uniqobj9.length].method0();
          }
        } while((obj1.method1()))
        a =(f64[((a-- )) & 255] != 87544291);
      }
      protoObj1.length= makeArrayLength(-26);
      // Snippet: Stack allocation of closure : bug# 607452 - pointers of loop body frame is not updated while boxing and hence it refers old FD
      
      function v2() {
      	// boxing happens here
          function v3() {
              try {
                   throw 'ex';
      			obj1 = obj1;
              } catch (v4) { GiantPrintArray.push(v4); }
          }
          var __loopvar1000 = loopInvariant;
      LABEL0: 
      LABEL1: 
      for (var _strvar0 in IntArr0) {
        if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
        if (__loopvar1000 <= loopInvariant - 8) break;
        __loopvar1000 -= 4;
      
              v3();
      		var v5 = ui32[(__loopvar1 + 1) & 255];
      		
      		// bailout here and we go back to interpreter. 
      		// bug was that value of v5 was not updated while in loopbody frame and  boxing of v3() happened
      		if(shouldBailout) {
      			IntArr1 + 1; // implicit call on valueOf
      		}
      		var __loopvar3 = loopInvariant,__loopSecondaryVar3_0 = loopInvariant,__loopSecondaryVar3_1 = loopInvariant - 13;
        LABEL2: 
        LABEL3: 
        do {
          __loopSecondaryVar3_0 += 2;
          __loopvar3 += 4;
          __loopSecondaryVar3_1 += 4;
          var func14 = function(argMath12,...argArr13){
            // ABCE#5 Snippet - Reset primary induction variable based on secondary induction variable
            
            // Re-assign primary/secondary IV at the beginning/end of the loop
            function v6(v7) {
            	var v8 =  arrObj0.length;
            	//addVar<number>:index1
            	var v9 = 100;
            	//addVar<number>:index2
            	
            	// Primary and secondary variables are reset sometimes in the loop
            	var __loopvar1001 = loopInvariant;
            for (var _strvar0 in VarArr0) {
              if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
              __loopvar1001 -= 2;
              if (__loopvar1001 < loopInvariant - 8) break;
            
            		 
            		argMath12 ^=(func1.call(obj1 ) ? (arrObj0.method1.call(Object.prototype ) >= ((new Error('abc')) instanceof ((typeof Array == 'function' ) ? Array : Object))) : func8.call(obj0 , arrObj0));
            		v7[v8] = ((protoObj0.prop1 *= 309867058) * ((typeof(Object.prototype.prop0)  == 'undefined')  - (protoObj1.length && parseInt("0x581DC6C1"))));
            		ary0 = arguments;
            				
            		// This will make either v8 or v9 as primary induction variable
            		if(v8 <= 1) break;
            		
            		 v9 = v8
            		GiantPrintArray.push(v7[ v9]);		
            	}
            
            }
            v6(shouldBailout? ary : ary);
            var __loopvar5 = loopInvariant,__loopSecondaryVar5_0 = loopInvariant,__loopSecondaryVar5_1 = loopInvariant;
            LABEL0: 
            LABEL1: 
            for (var _strvar1 in argArr13) {
              if(typeof _strvar1 === 'string' && _strvar1.indexOf('method') != -1) continue;
              __loopvar5++;
              __loopSecondaryVar5_1--;
              if (__loopvar5 > loopInvariant + 4) break;
              if (__loopSecondaryVar5_0 >= loopInvariant + 8) break;
              __loopSecondaryVar5_0 += 4;
              argArr13[_strvar1] = parseInt("1", 4);
              obj1.length= makeArrayLength(ary[(__loopSecondaryVar5_0 + 2)]);
              litObj0.prop0=func1;
            }
            var __loopvar5 = loopInvariant;
            while((func8.call(litObj1 , arrObj0))) {
              if (__loopvar5 == loopInvariant + 12) break;
              __loopvar5 += 4;
              var uniqobj10 = [protoObj1, obj1, obj1, obj1, obj1];
              uniqobj10[__counter%uniqobj10.length].method0();
              var __loopvar6 = loopInvariant,__loopSecondaryVar6_0 = loopInvariant - 12,__loopSecondaryVar6_1 = loopInvariant - 11;
              LABEL0: 
              LABEL1: 
              for (var _strvar0 in i16) {
                if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
                __loopSecondaryVar6_0 += 4;
                __loopSecondaryVar6_1 += 3;
                __loopvar6 += 3;
                i16[_strvar0] = ((Object.prototype.length > obj0.prop0)&&(__loopvar1 !== _strvar0));
                var uniqobj11 = [protoObj0, obj0, obj0, obj0, obj0];
                var uniqobj12 = uniqobj11[__counter%uniqobj11.length];
                uniqobj12.method1(IntArr0);
              }
              obj1.method0.call(protoObj1 );
              protoObj1.prop2 &=func3.call(arrObj0 );
            }
            return ui32[(__loopvar1 + 1) & 255];
          };
          var __loopvar4 = loopInvariant,__loopSecondaryVar4_0 = loopInvariant;
          LABEL4: 
          while((((new Error('abc')) instanceof ((typeof Array == 'function' ) ? Array : Object)))) {
            if (__loopvar4 == loopInvariant - 12) break;
            __loopvar4 -= 4;
            __loopSecondaryVar4_0 += 3;
            function func15 (argMath14){
            'use strict';;
              var __loopvar6 = loopInvariant,__loopSecondaryVar6_0 = loopInvariant,__loopSecondaryVar6_1 = loopInvariant - 10;
              while(((((new RegExp('xyz')) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) | protoObj1.length))) {
                __loopvar6++;
                __loopSecondaryVar6_0 += 2;
                __loopSecondaryVar6_1 += 3;
                if (__loopvar6 >= loopInvariant + 3) break;
                // apply6.ecs
                var v10 = {
                  v11: function() {
                    return function bar() {
                      var func16 = function(){
                        var func17 = function(argMath15 = obj1.method0.call(obj0 ),argMath16){
                          obj1.prop0 *=obj1.method0.call(obj0 );
                          GiantPrintArray.push('obj0.prop2 = ' + (obj0.prop2|0));
                      (Object.defineProperty(litObj0, 'prop1', {writable: true, enumerable: false, configurable: true }));
                          litObj0.prop1 = argMath15;
                          obj0.prop1 >>=(protoObj0.prop0 %= Math.ceil((argMath15 + 'caller')));
                          return argMath14;
                        };
                        var uniqobj13 = [protoObj1, obj1];
                        uniqobj13[__counter%uniqobj13.length].method0();
                        if(((typeof('caller')  == 'string')  & Math.fround((obj0.prop2 != (typeof(protoObj0.prop0)  == 'undefined') )))) {
                        }
                        else {
                          protoObj0.prop1 |=(((protoObj1.prop0 == argMath14) !== (argMath14 == protoObj1.prop1)) ? ((++ argMath14) & 'caller') : IntArr1[(__loopSecondaryVar3_0 + 1)]);
                          GiantPrintArray.push('Object.prototype.prop0 = ' + (Object.prototype.prop0|0));
                        }
                        return 691061221.1;
                      };
                      this.method0.apply(protoObj0, arguments);
                    }
                  }
                };
                var v12 = Object.prototype;
                
                v12.prototype = {
                    method0 : function(){
                        var uniqobj14 = [obj0, arrObj0, arrObj0];
                        var uniqobj15 = uniqobj14[__counter%uniqobj14.length];
                        uniqobj15.method1(IntArr1);
                        return ((typeof(argMath14)  == 'string')  * (protoObj0.method1.call(litObj1 , FloatArr0) * (argMath14 + ((argMath14 > Object.prototype.prop1)||(argMath14 >= a)))) - func0.call(arrObj0 , litObj1, ((-1362173613.9 === 65536) * (arrObj0[(17)] - (+ argMath14))), arrObj0[(__loopSecondaryVar6_1 + 1)], leaf));
                    },
                    method1 : function(){
                        return ((argMath14 != argMath14)&&(obj0.prop0 > __loopvar1));
                    },
                    v13 : function() {
                        return (typeof(argMath14)  == 'boolean') ;
                    },
                    v14 : function() {
                        protoObj1.method0.call(arrObj0 );
                        return ui16[(99) & 255];
                    },
                    v15 : function() {
                        return (typeof(IntArr0[(((((new protoObj0.method1(ary)).prop2  != (argMath14 * (-52 + -109497452))) >= 0 ? ((new protoObj0.method1(ary)).prop2  != (argMath14 * (-52 + -109497452))) : 0)) & 0XF)])  != 'string') ;
                    },
                    v16 : function() {
                        var func18 = function(argMath17,argMath18,argMath19,argMath20){
                          return 21;
                        };
                        return (argMath14 == VarArr0[(__loopSecondaryVar3_0 + 1)]);
                    }
                    };
                v12.v13 = v10.v11();
                
                v12.v13.prototype = {
                    v17 : ((((arrObj0.prop1 >>= b), (-210 + 20), (__loopvar0 * (674566727 + 214)), (FloatArr0.push('caller', -1436252807.9, VarArr0[(((undefined >= 0 ? undefined : 0)) & 0XF)], ('-14' in FloatArr0), Math.cos(argMath14), (protoObj0.prop1 = obj0.prop2), (obj1.prop0 >>> argMath14), 'caller', 'caller', (typeof(protoObj0.prop1)  != 'string') , (argMath14-- ), (arrObj0.prop0 >>>= argMath14), (4294967297 != obj0.prop2), (true instanceof ((typeof Number == 'function' ) ? Number : Object)), ui16[(__loopvar6 + 1) & 255]))
                , 'caller', (1 < 9.12200043913691E+18)) * (true instanceof ((typeof Function == 'function' ) ? Function : Object)) + ((130920751 / (-2 == 0 ? 1 : -2)) - (obj1.prop0 %= -362067695.9))) ? argMath14 : ([1, 2, 3] instanceof ((typeof Error == 'function' ) ? Error : Object))),
                    method0 : function(v17) {
                        this.v17 = v17;
                        this.method0 = function(){
                            var __loopvar8 = loopInvariant + 12,__loopSecondaryVar8_0 = loopInvariant,__loopSecondaryVar8_1 = loopInvariant - 3;
                            LABEL0: 
                            LABEL1: 
                            do {
                              __loopSecondaryVar8_1++;
                              if (__loopvar8 === loopInvariant) break;
                              if (__loopSecondaryVar8_1 == loopInvariant + 1) break;
                              __loopvar8 -= 4;
                              __loopSecondaryVar8_0 += 2;
                              var uniqobj16 = [arrObj0];
                              uniqobj16[__counter%uniqobj16.length].method1();
                            } while(((++ protoObj0.length)))
                            return func2.call(protoObj1 , IntArr1);
                        }
                        this.method1 = function(){
                            return parseInt("0x30F4B79153D93800");
                        }
                   }
                };
                
                obj0.method0.call(Object.prototype , protoObj1, 1117073434, protoObj0.prop0, leaf);
                
                v12.v14 = v10.v11();
                
                v12.v14.prototype = {
                    v18 : argMath14,
                    v19 : 'caller',
                    method0 : function(v18,v19) {
                        this.v18 = v18;
                        this.v19 = v19;
                    },
                    method1 : function(){
                        var __loopvar8 = loopInvariant,__loopSecondaryVar8_0 = loopInvariant + 3;
                        LABEL0: 
                        LABEL1: 
                        for (var _strvar0 in i32) {
                          if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
                          __loopvar8 += 3;
                          if (__loopvar8 >= loopInvariant + 9) break;
                          __loopSecondaryVar8_0--;
                          i32[_strvar0] = IntArr1[(((-3.28034532975588E+18 >= 0 ? -3.28034532975588E+18 : 0)) & 0XF)];
                        }
                        return (protoObj0.prop2 >>>= ((([1, 2, 3] instanceof ((typeof Error == 'function' ) ? Error : Object)) || (obj0.prop1 >= Object.prototype.prop2)) - (b < Object.prototype.prop1)));
                    }
                };
                
                
                v12.v15 = v10.v11();
                
                v12.v15.prototype = {
                    v20 : obj0,
                    method0 : function(v21) {
                        this.v20 = new v12.v13(v21);
                        return func4.call(obj1 );
                    }
                };
                
                (function(){
                  GiantPrintArray.push('argMath14 = ' + (argMath14|0));
                  argMath14 >>>=ui32[(([1, 2, 3] instanceof ((typeof Error == 'function' ) ? Error : Object))) & 255];
                  __loopSecondaryVar3_0++;;
                  protoObj1 = arrObj0;
                })();
                
                v12.v16 = v10.v11();
                
                v12.v16.prototype = {
                    v22 : protoObj1,
                    v23 : obj1,
                    method0 : function(v10,v24,v25) {
                        this.v22 = new v12.v13(v10);
                        this.v23 = new v12.v13(v24,v25);
                        return 3.98846568406549E+18;
                    }
                };
                
                var v26 = new v12.v13((true instanceof ((typeof Function == 'function' ) ? Function : Object)));
                var v27 = new v12.v13(func8(arrObj0),(argMath14 &= arguments[(__loopvar3 + 1)]));
                var func19 = function(){
                  var __loopvar9 = loopInvariant - 3,__loopSecondaryVar9_0 = loopInvariant - 9,__loopSecondaryVar9_1 = loopInvariant;
                  LABEL0: 
                  do {
                    __loopvar9++;
                    __loopSecondaryVar9_0 += 3;
                    if (__loopvar9 >= loopInvariant) break;
                    __loopSecondaryVar9_1--;
                (Object.defineProperty(protoObj1, 'prop1', {writable: true, enumerable: false, configurable: true }));
                    protoObj1.prop1 = ((arrObj0.length = (new obj1.method0()).prop2 ) >> (arguments[(10)] ? Math.asin((argMath14++ )) : uic8[(__loopvar3 + 3) & 255]));
                    var uniqobj17 = {nd0: {lf0: {prop0: -6.09294887915789E+18, prop1: 0, prop2: -694362919, length: -912593483 , method0: func8, method1: func3}, nd1: {nd0: {lf0: {prop0: 1578421518, prop1: -1827590936.9, prop2: -2147483648, length: -154329056.9 , method0: obj0.method0, method1: obj0.method0}}, nd1: {lf0: {prop0: 110, prop1: 7.46054486139116E+18, prop2: 480429300, length: -161238286 , method0: func14, method1: func14}, lf1: {prop0: 4.11320542931751E+18, prop1: 4.80896003601085E+17, prop2: 1240384349, length: 8.42446970407366E+18 , method0: obj1.method1, method1: func2}, lf2: {prop0: -1533690068.9, prop1: 65535, prop2: -2147483649, length: 375398804 , method0: func14, method1: obj1.method0}}}}};
                    uniqobj17.nd0.nd1.nd1.lf1.length= makeArrayLength(((obj1.prop0 != obj0.prop1)&&(obj0.prop1 <= obj0.prop2)));
                  } while((obj1.prop0))
                  return protoObj1.prop1;
                };
                var v28 = new v12.v13(arguments[(((obj1.method1() >= 0 ? obj1.method1() : 0)) & 0XF)],('caller' || (typeof(argMath14)  == 'boolean') ),Math.sin(-3));
                GiantPrintArray.push(v28.v17);
                
                var __loopvar8 = loopInvariant;
                LABEL0: 
                do {
                  __loopvar8 -= 3;
                  var func20 = function(){
                    argMath14 = ('caller' || (typeof(argMath14)  == 'boolean') );
                    var y = (b >>>= ((protoObj0.prop0 !== obj1.prop1)||(obj0.prop0 === protoObj0.prop2)));
                    argMath14 = (++ a);
                (Object.defineProperty(arrObj0, 'prop5', {writable: true, enumerable: false, configurable: true }));
                    arrObj0.prop5 = i8[((typeof((ary.push(obj1.prop1, 'caller', ((function () {;}) instanceof ((typeof RegExp == 'function' ) ? RegExp : Object)), (+ y)))
                )  != 'object') ) & 255];
                    return 429103469;
                  };
                  Object.prototype.prop0=Math.atan2(obj0.method1.call(obj0 , FloatArr0), (('method1' in obj1) != ((new RegExp('xyz')) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object))));
                  var __loopvar9 = loopInvariant,__loopSecondaryVar9_0 = loopInvariant,__loopSecondaryVar9_1 = loopInvariant - 13;
                  LABEL1: 
                  LABEL2: 
                  while((func4())) {
                    __loopvar9++;
                    __loopSecondaryVar9_1 += 4;
                    if (__loopvar9 > loopInvariant + 4) break;
                    __loopSecondaryVar9_0++;
                    var uniqobj18 = {nd0: {nd0: {lf0: {prop0: 883926220, prop1: -86526389, prop2: -2147483649, length: -81785924.9 , method0: arrObj0.method1, method1: func19}, lf1: {prop0: -40117320, prop1: 371258621, prop2: 1.69541289069625E+18, length: 223 , method0: func14, method1: func19}, lf2: {prop0: -337068388, prop1: 6.61287193693544E+18, prop2: -39724350.9, length: 0 , method0: func20, method1: obj0.method1}}}, lf1: {prop0: 95702827, prop1: -1472737849, prop2: 1607684580.1, length: 65536 , method0: arrObj0.method1, method1: obj1.method0}};
                    Object.prototype.length= makeArrayLength((Object.prototype.prop0++ ));
                    GiantPrintArray.push('uniqobj18.lf1.prop1 = ' + (uniqobj18.lf1.prop1|0));
                (Object.defineProperty(arrObj0, 'prop2', {writable: true, enumerable: false, configurable: true }));
                    arrObj0.prop2 = arguments[((((uniqobj18.nd0.nd0.lf0.prop1 %= (-2055400894.9)) >= 0 ? (uniqobj18.nd0.nd0.lf0.prop1 %= (-2055400894.9)) : 0)) & 0XF)];
                  }
                  protoObj1.method1();
                } while((obj0.method1.call(obj0 , FloatArr0)) && ((__loopvar8 > loopInvariant - 9)))
                
                var v29 = new v12.v14((- (typeof(obj0.prop0)  != 'boolean') ));
                var v30 = new v12.v14(b,((((new EvalError()) instanceof ((typeof Boolean == 'function' ) ? Boolean : Object)) === ((506323644 >> __loopvar0) >>> (FloatArr0.push(arrObj0[(loopInvariant)]))
                )) ? (Math.imul(i16[(__loopvar4 - 3) & 255], ((arrObj0.prop1 !== obj0.prop2)&&(argMath14 <= protoObj0.prop2))) * (FloatArr0.push(arrObj0[(loopInvariant)]))
                 + -4.35403036899742E+18) : (typeof(a)  == 'string') ));
                obj0.prop0 %=(Math.imul(i16[(__loopvar4 - 3) & 255], ((arrObj0.prop1 !== obj0.prop2)&&(argMath14 <= protoObj0.prop2))) * (FloatArr0.push(arrObj0[(loopInvariant)]))
                 + -4.35403036899742E+18);
                var v31 = new v12.v14(('caller' ? ((new Array()) instanceof ((typeof Error == 'function' ) ? Error : Object)) : ui32[(__loopvar0 - 1) & 255]),arrObj0[(((((function () {;}) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) >= 0 ? ((function () {;}) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) : 0)) & 0XF)],(true instanceof ((typeof Error == 'function' ) ? Error : Object)));
                GiantPrintArray.push(v31.v19);
                
                var v32 = new v12.v15((arrObj0[(((((function () {;}) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) >= 0 ? ((function () {;}) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) : 0)) & 0XF)] ^ ((function () {;}) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object))));
                var v33 = new v12.v15(Object.prototype.prop1,(typeof(Object.prototype.prop1)  != 'object') );
                var v34 = new v12.v15(-1251206392.9,func19(),(+ aliasOfi8[(__loopSecondaryVar4_0 + 1) & 255]));
                GiantPrintArray.push(v34.v20);
                
                var v35 = new v12.v16(((arrObj0.method0() * ((argMath14 *= (obj1.prop1 % argMath14)) - 'caller')) << 'caller'));
                var v36 = new v12.v16(((-- obj0.prop0) | ((function () {;}) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object))),'caller');
                protoObj0.prop1 %=(b += ((-164 % 1131221497.1) - ((typeof(obj0.prop1)  != 'boolean')  ? (protoObj0.length = 1.36146611177469E+18) : 'caller')));
                var v37 = new v12.v16(IntArr0[(__loopSecondaryVar3_1 + 1)],((-164 % 1131221497.1) - ((typeof(obj0.prop1)  != 'boolean')  ? (protoObj0.length = 1.36146611177469E+18) : 'caller')),(Object.prototype.prop1 > arrObj0.prop0));
                GiantPrintArray.push(v37.v22);
                GiantPrintArray.push(v37.v23);
                var func21 = function(argMath21,argMath22,argMath23 = (Object.prototype.prop0 ? 7.75470678307751E+18 : Object.prototype.prop2)){
                  return i32[((argMath14-- )) & 255];
                };
                break ;
                obj1.prop1 <<=(([1, 2, 3] instanceof ((typeof Object == 'function' ) ? Object : Object)) >>> obj1.method0.call(Object.prototype ));
              }
        (Object.defineProperty(obj1, 'prop2', {writable: true, enumerable: false, configurable: true }));
              obj1.prop2 = ((new RegExp('xyz')) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object));
              return i16[((- ((new Error('abc')) instanceof ((typeof Array == 'function' ) ? Array : Object)))) & 255];
            }
            var fPolyProp = function (o) {
              if (o!==undefined) {
                WScript.Echo(o.prop0 + ' ' + o.prop1 + ' ' + o.prop2);
              }
            };
            fPolyProp(litObj0); 
            fPolyProp(litObj1); 
        
          }
          protoObj0.prop0 *=((80 instanceof ((typeof Error == 'function' ) ? Error : Object)) * (func14(leaf,ary) - 'caller'));
        } while(((((new RegExp('xyz')) instanceof ((typeof EvalError == 'function' ) ? EvalError : Object)) | protoObj1.length)) && ((__loopvar3 < loopInvariant + 12)))
              GiantPrintArray.push(v5);
          }
      
      }
      v2();
    }
    (function(){
      // proxyvar_array.ecs - array
      var v38 = new Proxy(IntArr1,  proxyHandler);
      // addarray:proxyarr
      var uniqobj19 = [protoObj1, obj1];
      uniqobj19[__counter%uniqobj19.length].method0();
      var uniqobj20 = [obj1];
      uniqobj20[__counter%uniqobj20.length].method0();
      var __loopvar2 = loopInvariant,__loopSecondaryVar2_0 = loopInvariant + 12,__loopSecondaryVar2_1 = loopInvariant;
      for (var _strvar0 in protoObj1) {
        if(typeof _strvar0 === 'string' && _strvar0.indexOf('method') != -1) continue;
        __loopSecondaryVar2_0 -= 4;
        __loopSecondaryVar2_1--;
        if (__loopvar2 >= loopInvariant + 2) break;
        __loopvar2++;
      }
    })();
    (function(argMath24 = Math.cos(((ary.slice(14,0)) ? (- (ary.slice(14,0))) : obj0.prop1)),argMath25 = ((this.prop1 < obj1.length)&&(protoObj0.prop2 === obj0.prop2)),argMath26){
      argMath26 =(obj1.length === a);
    })(ui8[(loopInvariant) & 255],arrObj0[((((obj1.prop1 > obj1.prop1) >= 0 ? (obj1.prop1 > obj1.prop1) : 0)) & 0XF)],VarArr0);
  }
  // ABCE#3 Snippet - Array objects + CSE in expressions
  
  GiantPrintArray.push('snippet');
  // Use expression in array access and guard check that gets CSEd
  // Verify array objects with bound checks hoisted
  function v39(v40, v41){
  	var v42 = 0;
  	var v43 =  Object.prototype.length;
  	//addVar<number>:index
  
  	var __loopvar1002 = loopInvariant,__loopSecondaryVar1002_0 = loopInvariant - 15;
  LABEL0: 
  while((((((obj0.prop1 === 7.1268032550465E+18) != -4.62314564028928E+18) | (+ (protoObj1.prop2 /= protoObj0.prop2))) * 831578761 - (((new func1()).prop0  ? (typeof(protoObj0.prop0)  != 'undefined')  : 'caller') * (+ IntArr0[(__loopvar1002 - 1)]) - (IntArr1.push((-- obj1.prop1), (-- obj1.prop1), ((new RangeError()) instanceof ((typeof func8 == 'function' ) ? func8 : Object)), (arrObj0.prop1 * protoObj0.prop0 + 2147483647), protoObj1.method1(), ((new RangeError()) instanceof ((typeof String == 'function' ) ? String : Object))))
  )))) {
    __loopvar1002--;
    __loopSecondaryVar1002_0 += 4;
    if (__loopvar1002 < loopInvariant - 4) break;
  
  	
  		 
  		
  		// CSE as well as bound check tracking
  		v40[(((b <= obj1.prop0)||(Object.prototype.prop2 != protoObj1.prop2)) ? -43 : (VarArr0.splice(1,6, protoObj1.prop0, 1747240571.1, (protoObj1.prop2 ? (obj1.prop2 * ((arrObj0.prop0 & protoObj0.length) - (typeof(Object.prototype.prop1)  != 'number') )) : (+ arrObj0.method1.call(obj1 ))), Math.ceil({prop4: (typeof(protoObj0.prop2)  != 'undefined') , prop3: (obj0.prop2 != obj0.prop2), prop2: Object.create({64: obj0.prop0, prop0: obj0.prop2, prop1: 7.74405122140488E+18, prop3: protoObj1.prop1, prop4: protoObj1.length, prop5: protoObj1.prop1}), prop1: arrObj0.method1.call(obj1 ), prop0: (Function('') instanceof ((typeof String == 'function' ) ? String : Object))}), obj1.prop0, (protoObj1.length != Object.prototype.prop0), ((+ (-2.6946890203271E+18 instanceof ((typeof Error == 'function' ) ? Error : Object))) instanceof ((typeof obj0.method1 == 'function' ) ? obj0.method1 : Object)), Math.imul(obj0.method1.call(obj1 , FloatArr0), ((arrObj0.prop2 &= b) ? i8[(134) & 255] : (517046397.1 * protoObj0.prop2 - protoObj1.prop1))), IntArr1[(8)], (typeof(protoObj0.prop2)  != 'object') , (+ (obj1.prop0 >>= f64[(79) & 255])))))] =  (Math.asin((6.42081223106955E+18 || (protoObj1.prop0 += b))) - func3.call(Object.prototype ));
  		
  		 
  		
  		v42 <<= v40[(((b <= obj1.prop0)||(Object.prototype.prop2 != protoObj1.prop2)) ? -43 : (VarArr0.splice(1,6, protoObj1.prop0, 1747240571.1, (protoObj1.prop2 ? (obj1.prop2 * ((arrObj0.prop0 & protoObj0.length) - (typeof(Object.prototype.prop1)  != 'number') )) : (+ arrObj0.method1.call(obj1 ))), Math.ceil({prop4: (typeof(protoObj0.prop2)  != 'undefined') , prop3: (obj0.prop2 != obj0.prop2), prop2: Object.create({64: obj0.prop0, prop0: obj0.prop2, prop1: 7.74405122140488E+18, prop3: protoObj1.prop1, prop4: protoObj1.length, prop5: protoObj1.prop1}), prop1: arrObj0.method1.call(obj1 ), prop0: (Function('') instanceof ((typeof String == 'function' ) ? String : Object))}), obj1.prop0, (protoObj1.length != Object.prototype.prop0), ((+ (-2.6946890203271E+18 instanceof ((typeof Error == 'function' ) ? Error : Object))) instanceof ((typeof obj0.method1 == 'function' ) ? obj0.method1 : Object)), Math.imul(obj0.method1.call(obj1 , FloatArr0), ((arrObj0.prop2 &= b) ? i8[(134) & 255] : (517046397.1 * protoObj0.prop2 - protoObj1.prop1))), IntArr1[(8)], (typeof(protoObj0.prop2)  != 'object') , (+ (obj1.prop0 >>= f64[(79) & 255])))))];
  		
  	}
  
  }
  // Pass arrays or array objects 
  v39(shouldBailout? arrObj0 : IntArr0,  10000 );
  
  
  WScript.Echo('a = ' + (a|0));
  WScript.Echo('b = ' + (b|0));
  WScript.Echo('this.prop0 = ' + (this.prop0|0));
  WScript.Echo('this.prop1 = ' + (this.prop1|0));
  WScript.Echo('obj0.prop0 = ' + (obj0.prop0|0));
  WScript.Echo('obj0.prop1 = ' + (obj0.prop1|0));
  WScript.Echo('obj0.prop2 = ' + (obj0.prop2|0));
  WScript.Echo('obj0.length = ' + (obj0.length|0));
  WScript.Echo('protoObj0.prop0 = ' + (protoObj0.prop0|0));
  WScript.Echo('protoObj0.prop1 = ' + (protoObj0.prop1|0));
  WScript.Echo('protoObj0.prop2 = ' + (protoObj0.prop2|0));
  WScript.Echo('protoObj0.length = ' + (protoObj0.length|0));
  WScript.Echo('obj1.prop0 = ' + (obj1.prop0|0));
  WScript.Echo('obj1.prop1 = ' + (obj1.prop1|0));
  WScript.Echo('obj1.prop2 = ' + (obj1.prop2|0));
  WScript.Echo('obj1.length = ' + (obj1.length|0));
  WScript.Echo('protoObj1.prop0 = ' + (protoObj1.prop0|0));
  WScript.Echo('protoObj1.prop1 = ' + (protoObj1.prop1|0));
  WScript.Echo('protoObj1.prop2 = ' + (protoObj1.prop2|0));
  WScript.Echo('protoObj1.length = ' + (protoObj1.length|0));
  WScript.Echo('arrObj0.prop0 = ' + (arrObj0.prop0|0));
  WScript.Echo('arrObj0.prop1 = ' + (arrObj0.prop1|0));
  WScript.Echo('arrObj0.prop2 = ' + (arrObj0.prop2|0));
  WScript.Echo('arrObj0.length = ' + (arrObj0.length|0));
  WScript.Echo('Object.prototype.prop0 = ' + (Object.prototype.prop0|0));
  WScript.Echo('Object.prototype.prop1 = ' + (Object.prototype.prop1|0));
  WScript.Echo('Object.prototype.prop2 = ' + (Object.prototype.prop2|0));
  WScript.Echo('Object.prototype.length = ' + (Object.prototype.length|0));
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
  WScript.Echo('Object.prototype.prop0 = ' + (Object.prototype.prop0|0));
  WScript.Echo('Object.prototype.prop1 = ' + (Object.prototype.prop1|0));
  WScript.Echo('Object.prototype.prop2 = ' + (Object.prototype.prop2|0));
  WScript.Echo('Object.prototype.length = ' + (Object.prototype.length|0));
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
test0();

// run JITted code
runningJITtedCode = true;
test0();


// Baseline total processor time: 00:00:16.1093750
// Test total processor time: 00:02:05.5000000
// 
// Addl Dump Info: 
//Baseline length = 219452

//Test length = 114239

//Baseline length = 219452

//Test length = 172952

//Baseline length = 219452

//Test length = 164652

// 
// Baseline output:
// Skipping first 8801 lines of output...
// Snippet 1: 
// 0
// false
// 0
// getter
// NaN
// getter
// NaN
// snippet
// sumOfary = 0-680817128-527813232-1904181990-1920459204-691-93163429-754207282-2147483646-830578102
// subset_of_ary = -680817128,-527813232,-832088952,99,980156429.10000000,0,-1920459204,-1073741824,191,-93163429,-754207282
// sumOfIntArr0 = 65537-252-1389266167-1543784572-8-1361269967-2147483646
// subset_of_IntArr0 = 65537,-252,-1389266167,-139912883,183,1,-8,-1361269967,-2147483646
// sumOfIntArr1 = 0-408473904-367883042-637078889-1051552569-1051552570falseNaN0false
// subset_of_IntArr1 = -1336332573,,97390817,-367883042,-637078889,-1051552569,-1051552570,false,NaN,0,false
// sumOfFloatArr0 = 0-192684291-1221465361-787558325-1378829747-1915497990.90000000
// subset_of_FloatArr0 = -63975519,65537,-1221465361,-561153202,979829498,-119993699,65536,-1915497990.90000000
// sumOfVarArr0 = 0[object Object]467326443-2012225683-1635681071-2-632493755-975199350.90000000
// subset_of_VarArr0 = [object Object],467326443,-2012225683,-162068960,186382691,489218302.10000000,-2,-287597399,1203918061,-975199350.90000000,0
// 
// 
// Test output:
// Skipping first 6597 lines of output...
// 0
// false
// 0
// getter
// NaN
// getter
// NaN
// snippet
// sumOfary = 0-680817128-527813232-1904181990-1920459204-691-93163429-754207282-2147483646-830578102
// subset_of_ary = -680817128,-527813232,-832088952,99,980156429.10000000,0,-1920459204,-1073741824,191,-93163429,-754207282
// sumOfIntArr0 = 65537-252-1389266167-1543784572-8-1361269967-2147483646
// subset_of_IntArr0 = 65537,-252,-1389266167,-139912883,183,1,-8,-1361269967,-2147483646
// sumOfIntArr1 = 0-408473904-367883042-637078889-1051552569-1051552570falseNaN0false
// subset_of_IntArr1 = -1336332573,,97390817,-367883042,-637078889,-1051552569,-1051552570,false,NaN,0,false
// sumOfFloatArr0 = 0-192684291-1221465361-787558325-1378829747-1915497990.90000000
// subset_of_FloatArr0 = -63975519,65537,-1221465361,-561153202,979829498,-119993699,65536,-1915497990.90000000
// sumOfVarArr0 = 0[object Object]467326443-2012225683-1635681071-2-632493755-975199350.90000000
// subset_of_VarArr0 = [object Object],467326443,-2012225683,-162068960,186382691,489218302.10000000,-2,-287597399,1203918061,-975199350.90000000,0
// FATAL ERROR: jshost.exe failed due to exception code c0000005
// 
// Reduced Switches:
