//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var passed = true;

function test1() {
  var sc4 = WScript.LoadScript('function test(){ obj2.prop4 = {needMarshal:true}; }', 'samethread');
  var obj1 = new Proxy({}, { set: function (target, property, value) { Reflect.set(value); } })
  sc4.obj2 = obj1;
  sc4.test();
  obj1.prop4 = { needMarshal: false };
  obj1.prop5 = { needMarshal: false };
};
test1();


function test2() {
  var bug = new Proxy(new Array(1), { has: () => true });
  var a = bug.concat();
  if (a[0] !== undefined || a.length !== 1) {
    passed = false;
  } else {
    passed &= true;
  }
}
test2();

function test3() {
  var obj1 = {};
  var arrObj0 = {};
  var x = 1
  var proxyHandler = {};
  proxyHandler['get'] = function () { };
  proxyHandler['defineProperty'] = function (target, property, descriptor) {
    return Reflect.defineProperty(target, property, descriptor);
  };
  proxyHandler['isExtensible'] = function (target) {
    arrObj0.prop0;
    arrObj0 = new Proxy(arrObj0, proxyHandler);
    return Reflect.isExtensible(target);
  };
  arrObj0 = new Proxy(arrObj0, proxyHandler);
  arrObj0 = new Proxy(arrObj0, proxyHandler);
  do {
    var sc3 = WScript.LoadScript('function test(){arrObj0.length = arrObj0[obj1];}', 'samethread');
    sc3.obj1 = obj1;
    sc3.arrObj0 = arrObj0;
    sc3.test();
  } while (x--);
}
test3();

function test4() {
  var func3 = function () { };
  var ary = Array();
  var proxyHandler = {};
  var ownkeys = Reflect.ownKeys(ary);
  proxyHandler['ownKeys'] = function () {
    func3() == 0;
    return ownkeys;
  };

  ary = new Proxy(ary, proxyHandler);
  var sc2 = WScript.LoadScript('function test(){for (var x in ary);}', 'samethread');
  sc2.ary = ary;
  sc2.func3 = func3;
  sc2.test();
}
test4();

print(passed ? "passed" : "failed");
