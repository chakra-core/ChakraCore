//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var func3 = function () 
{
  var sc4 = WScript.LoadScript('function test(){ obj2.prop4 = {needMarshal:true}; }', 'samethread');
  var obj1=new Proxy({}, {set:function(target, property, value) { Reflect.set(value);}})
  sc4.obj2 = obj1;
  sc4.test();
  obj1.prop4 = {needMarshal:false};
  obj1.prop5 = {needMarshal:false};
};
func3();



var bug = new Proxy(new Array(1), {has: () => true});
var a = bug.concat();
if (a[0] !== undefined || a.length !== 1) {
    print("failed");
} else {
    print("passed");
}
