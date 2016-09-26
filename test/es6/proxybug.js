//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var protoObj1 = {};
var func3 = function () 
{
  var sc4 = WScript.LoadScript('function test(){protoObj1.prop4 = {}; func1();}', 'samethread');
  sc4.protoObj1 = protoObj1;
  sc4.func1 = function () 
  {
    protoObj1 = new Proxy({}, {set:function(target, property, value) { Reflect.set(value);}});
  };
  sc4.test();
};
func3();
func3();


var bug = new Proxy(new Array(1), {has: () => true});
var a = bug.concat();
if (a[0] !== undefined || a.length !== 1) {
    print("failed");
} else {
    print("passed");
}
