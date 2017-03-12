//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// WinBlue 266843
// -debuglaunch -targeted -maxinterpretcount:1

// Test #0: issue in the bug: shared bailout does not support conditional branch instrs, 
// such as branch with BailOnImplicitCalls.
var shouldBailout = false;
function test0() {
    var obj1 = {};
    var arrObj0 = {};
    var func0 = function (argObj0) {
        var __loopvar4 = 0;
        do {
            __loopvar4++;
        } while (((shouldBailout ? (this.prop1 = {
                            valueOf : function () {
                                WScript.Echo('this.prop1 valueOf');
                                return 3;
                            }
                        }, 1) : 1)) && __loopvar4 < 3)
        var __loopvar3 = 0;
        for (; obj1.prop1 < (1); __loopvar3++ + obj1.prop1++) {
            if (__loopvar3 > 3)
                break;
        }
    }
    arrObj0.method0 = func0;
    obj0 = obj1;
    arrObj0.method0.call(obj0, 1);
};
test0();
shouldBailout = true;
test0();

// Test #1: a variation of the issue from the bug.
function test1() {
    var obj0 = {};
    var obj1 = {};
    var arrObj0 = {};
    var func1 = function () {
        if ((obj0.prop1 >= arrObj0.length)) 
        {
        }
        else {
            return f;
        }
        ary.pop();
    }
    var ary = new Array(10);
    var f = 1;
    var __loopvar0 = 0;
    do {
        __loopvar0++;
        func1(obj0);
        obj1.prop0 = (obj0.prop1 = {
                valueOf : function () {
                    return 3;
                }
            }, 1);
    } while ((1) && __loopvar0 < 3)
    WScript.Echo("ary.length = " + ary.length);
};
test1();

// Test #2: make sure that hoisting of (only) aux bailout on AND works correctly.
function test2()
{
  var obj0 = {};
  obj0.method0 = function() {}; 
  var __loopvar1 = 1;
  do 
  {
    obj0.method0(2147483647 & 1);
  } 
  while(__loopvar1 < 1)
}
test2(); 
test2(); 

// Test #3: variation of test2.
// Generating IgnoreException bailout shared with implicit calls bailout for the case
// when implicit calls bailout is not inside helper block.
function test3()
{
  var obj1 = {};
  var loopvar = 0;
  function func1()
  {
    while (loopvar < 1 && obj1.length < new Object())
    {
      obj1.length;
      ++loopvar;
    }
  }
  func1();
}
test3(); 
test3(); 

// Test for shared implicit call + ignore exception bailout on LdFld.
// This was AV'ing on ARM because prevInstr in LowerLdFld was set to ldFldInstr which
// got removed as part of generating shared bailout.
function test4()
{
  var arrObj0 = {};
  var func0 = function() {};
  func0(arrObj0);
};
test4(); 
test4(); 

// Test for change to convert all throwing helpers to dubegger bailout instrs,
// for the case when there is typeof (which converts to helper with IgnoreException bailout) 
// following by a branch: we generate fast bracn and lower typeof which is prev instr as part 
// of lowering the branch, rather then lowering it inside LowerRange.
function test5()
{
  function func1() {}
  var ary = new Array(10);
  var obj1 = {};
  var func0 = function(argArr91,argMath92){}
  function leaf() { return 100; };
  var arrObj0 = {};
  var obj0 = {};

  func1(obj1, leaf, ((arrObj0[(((454028936 >= 0 ? 454028936 : 0)) & 0XF)] instanceof ((typeof Object == 'function' ) ? Object : Object)) >= ary[(0)]), (((arrObj0[(((454028936 >= 0 ? 454028936 : 0)) & 0XF)] instanceof ((typeof Object == 'function' ) ? Object : Object)) < ary[(0)]) < (arrObj0[(((454028936 >= 0 ? 454028936 : 0)) & 0XF)] instanceof ((typeof Object == 'function' ) ? Object : Object)))); 

  if (runningJITtedCode)
  {
    obj1.prop0=arrObj0[(6)];
  }
  
  return (~ (func0.call(arrObj0 , obj0, arrObj0, leaf, obj0) != func0.call(obj0 , obj1, arrObj0, leaf, obj0)));
}
test5();
var runningJITtedCode = true;
test5();

// Test to make sure that polymorphicCacheIndex in bailout info is correct. 
// See WinBlue 276053.
var shouldBailout = false;
function test6()
{
  var func2 = function(argFunc5)
  {
    var u = (shouldBailout ? 
      (Object.defineProperty(this, 'prop1', 
        {set: function(_x) { WScript.Echo('this.prop1 setter'); }, configurable: true}), 1) : 
      1);
  }
  this.prop0 = 1; 
  function bar3 (argFunc12, argMath13)
  {
    this.prop1 = 1; 
    this.prop0;
  }
  c = func2(1); 
  bar3(1, 1); 
};
test6(); 
shouldBailout = true;
test6(); 

WScript.Echo("done");
