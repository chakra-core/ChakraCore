//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validation of the 'Function' code being present while debug attach

var shouldBailout = false;
function test0(){
  var obj0 = {};
  var obj1 = {};
  var func1 = function(){
    var __loopvar2 = 0;                 /**bp:locals()**/
    do {
      __loopvar2++;
      (function(p0, p1, p2, p3){
        if(((obj1.length &= 1), (-- obj1.length), 1, 1)) {
        }
        var __loopvar4 = 0;
        while(((shouldBailout ? (Object.defineProperty(obj0, 'length', {writable: false, enumerable: false, configurable: true}), 1) : 1)) && __loopvar4 < 3) {
          __loopvar4++;
        }
      })(1);
      obj0 = obj1;
    } while(((obj0.length += Math.abs((Function("") instanceof ((typeof RegExp == 'function' ) ?RegExp: Object))))) && __loopvar2 < 3)
  }
  obj0.method0 = func1;                    /**bp:locals()**/
  ((obj0.method0(), 1));
};

test0();
WScript.Attach(function () {
				test0();
				WScript.Echo("Pass")
				});
