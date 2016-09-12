//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validating bug 271356

var shouldBailout = false;
function test0(){
  var obj0 = {};
  var obj1 = {};
  var arrObj0 = {};
  var func0 = function(){
    (arrObj0[(((((shouldBailout ? (arrObj0[(((ary[(((((shouldBailout ? (ary[(((1) >= 0 ? ( 1) : 0) & 0xF)] = "x") : undefined ), 1) >= 0 ? 1 : 0)) & 0XF)]) >= 0 ? ( ary[(((((shouldBailout ? (ary[(((1) >= 0 ? ( 1) : 0) & 0xF)] = "x") : undefined ), 1) >= 0 ? 1 : 0)) & 0XF)]) : 0) & 0xF)] = "x") : undefined ), ary[(((((shouldBailout ? (ary[(((1) >= 0 ? ( 1) : 0) & 0xF)] = "x") : undefined ), 1) >= 0 ? 1 : 0)) & 0XF)]) >= 0 ? ary[(((((shouldBailout ? (ary[(((1) >= 0 ? ( 1) : 0) & 0xF)] = "x") : undefined ), 1) >= 0 ? 1 : 0)) & 0XF)] : 0)) & 0XF)] ? 1 : ((f64[(obj0.length) & 255] != ((new RegExp("xyz")) instanceof ((typeof Number == 'function' ) ? Number : Object))) ^ 1));
  }
  var ary = new Array(10);
  var ui8 = new Uint8Array(256);
  var f64 = new Float64Array(256);
  var f = 1;
  arrObj0[0] = 1; 
  try {
    obj0.prop0 = 1; /**bp:locals();stack()**/
    var __loopvar2 = 0;
    do {
      __loopvar2++;
      f <<=(shouldBailout ? (Object.defineProperty(this, 'prop1', {get: function() { WScript.Echo('this.prop1 getter'); return 3; }, set: function(_x) { WScript.Echo('this.prop1 setter'); }, configurable: true}), (func0(), 1)) : (func0(), 1));
    } while((1) && __loopvar2 < 3)
  } catch(ex) {
    WScript.Echo(ex.message);  } finally {
    var __loopvar2 = 0;
    for(var strvar0 in ui8 ) {
      if(strvar0.indexOf('method') != -1) continue;
      if(__loopvar2++ > 3) break;
      try {
        ary0 = arguments; 
        this.prop1 = func0.call(obj1 ); 
      } catch(ex) {
        WScript.Echo(ex.message);      } finally {
        arrObj0 = 1;
      }
    }
  }
11};

// generate profile
test0(); 
test0(); 
test0(); 
test0(); 

// run JITted code
runningJITtedCode = true;
test0(); 
test0(); 
test0(); 
test0(); 

// run code with bailouts enabled
shouldBailout = true;
test0(); 

WScript.Echo("Pass");

