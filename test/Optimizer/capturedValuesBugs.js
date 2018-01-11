//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

function test0(iter) {
  var dependencies = [];
  var result;
  for (var i = 0; i < iter; ++i) {
    result = (function () {
      var numberOfArgs = arguments.length;
      callback = function () {
        var counter = arguments.length;
        return counter;
      };
      return callback.apply(undefined || this, arguments);
    }).apply(this, dependencies);
    dependencies.push(i);
  }
  return result;
}

assert.areEqual(test0(16), 15, "test0 should return 15");

function test1() {
    var obj0 = { };
    var b = 1;
    prop0 = [];
    for(var i = 0; i < 2; ++i) {
        for(var j = 0; j < 1; ++j) {
            obj0.prop1;
            if(1.1)
                ++b;
            else {
                obj0 = {x:1};
            }
        }
    }
};

test1();
test1();
test1();

function test2() {
    var obj0 = new Object();
    var c;
    var e;
    
    c = 32235;
    
    e = -25689;

    if((1 - (obj0 <= obj0)) ) {
    } else {
        e += 12;
    }

    c = ((e * -4275 ) * (35822 - (17135 ^ (-1))));
    return e;
}
    
assert.areEqual(test2(), -25677, "test2 should return -25677");
assert.areEqual(test2(), -25677, "test2 should return -25677");
assert.areEqual(test2(), -25677, "test2 should return -25677");

function test3() {
    var loopInvariant = 9;
    function leaf() {
    }
    var obj0 = {};
    var obj1 = {};
    var func0 = function (argMath2) {
      for (; argMath2; ) {
        if (loopInvariant == 0) {
          break;
        }
        loopInvariant -= 3;
        var __loopSecondaryVar3_0 = loopInvariant;
        while (ary.shift()) {
          __loopSecondaryVar3_0 = 3;
        }
        leaf.call();
      }
    };
    var func1 = function () {
      var uniqobj4 = { prop1: func0(typeof Object.prototype.prop1) };
    };
    obj0.method0 = func1;
    obj1.method1 = obj0.method0;
    var ary = Array();
    func0(obj1.method1());
    func0(obj1.method1());
}
test3();
print("passed");