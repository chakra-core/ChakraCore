//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

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
    print('FAIL');
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


function test5() {
  function makeArrayLength() {
  }
  function leaf() {
  }
  var obj1 = {};
  var arrObj0 = {};
  var func1 = function () {
  };
  obj1.method0 = func1;
  obj1.method1 = func1;
  var protoObj1 = Object();
  var proxyHandler = {};
  var v0 = new Proxy(obj1.method0, proxyHandler);
  var sc9 = WScript.LoadScript('', 'samethread');
  sc9.arrObj0 = arrObj0;
  sc9.obj1 = obj1;
  sc9.protoObj1 = protoObj1;
  sc9.v0 = v0;
  sc9.makeArrayLength = makeArrayLength;
  sc9.leaf = leaf;
  var sc9_cctx = sc9.WScript.LoadScript('function test() {var b = 1; arrObj0.length= makeArrayLength((arrObj0.length != arrObj0.prop4));\n       var d = obj1.method1.call(protoObj1 , (((new v0((protoObj1.length >>>= -866043558),(protoObj1.prop0 = 1),leaf,leaf)) , (b ? 1 : 16678541)) & (typeof(arrObj0.prop4)  == \'number\') ), ((1) * (d %= (1 * obj1.length - b)) + obj1.method0.call(arrObj0 , ("a" instanceof ((typeof RegExp == \'function\' ) ? RegExp : Object)), (\'prop0\' in arrObj0), leaf, leaf)), leaf, leaf);\n       var uniqobj21 = Object.create(arrObj0);\n       arrObj0.length= makeArrayLength(((1 ) % -520343586));\n       ;\n       }');
  sc9_cctx.test();
}
test5();

function test6() {
  var global = WScript.LoadScript("", "samethread");
  var OProxy = global.Proxy;
  var desc;
  var p = new OProxy({}, {
      defineProperty: function(_, __, _desc) {
      desc = _desc;
      return desc;
    }
  });
  p.a = 0;
}
test6();

function test7() {
  var obj0 = {};
  var arrObj0 = {};
  var func3 = function () {
    return typeof func3.caller == 'object';
  };
  obj0.method0 = func3;
  var protoObj0 = Object(obj0);
  var proxyHandler = {};
  do {
  } while (protoObj0.method0());
  var v0 = new Proxy(obj0.method0, proxyHandler);
  var sc0 = WScript.LoadScript('', 'samethread');
  sc0.v0 = v0;
  sc0.arrObj0 = arrObj0;
  sc0.obj0 = obj0;
  sc0.protoObj0 = protoObj0;
  var sc0_cctx = sc0.WScript.LoadScript('function foo() { var _oo1obj = (function(_oo1a) {\n  var _oo1obj = {};\n  _oo1obj.prop1 = v0(arrObj0);\n  return _oo1obj;\n})(typeof(arrObj0.prop0)  == \'object\') ;\n }');
  sc0_cctx.foo();
}
test7();

// In Chakra when calling "foo(arg1, arg2, ...)", we assume all the
// participating components -- "foo", "arg1", "arg2"... -- are either from
// executing context (caller) or properly cross-site marshalled.
//
// BUG: JavascriptProxy implementation has a lot of direct calls to
//            trap(handler, target, ...)
// "handler", "target" were based on proxy context at proxy creation time.
// They both need marshalling to executing context.

// Except: apply & construct traps are behind CrossSiteProxyCallTrap thus they
// work fine without marshalling themselves.

// defineProperty trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x, name, desc) { return Object.defineProperty(x, name, desc) }',
    'samethread');
  var p = new Proxy({}, {
    defineProperty: g.test
  });

  g.test(p, 'abc', {value: 1});
  //
  // This call invokes the trap in executing "g" context. Since the trap (value
  // g.test) itself is also from "g" context, CrossSite::CommonThunk determines
  // no marshalling required. However, handler/target args were from "p"
  // context and need marshalling.
})();

// deleteProperty trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x, name) { delete x.name }', 'samethread');
  var p = new Proxy({}, {
    deleteProperty: g.test
  });
  g.test(p);
})();

// get trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x) { x.name }', 'samethread');
  var p = new Proxy({}, {
    get: g.test
  });
  g.test(p);
})();

// getOwnPropertyDescriptor trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x, name) { Object.getOwnPropertyDescriptor(x, name) }',
    'samethread');
  var p = new Proxy({}, {
    getOwnPropertyDescriptor: g.test
  });
  g.test(p);
})();

// getPrototypeOf trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x) { x.name; return x.__proto__ }', 'samethread');
  var p = new Proxy({}, {
    getPrototypeOf: g.test
  });

  g.test({name: p});
  //
  // This test is a bit different to others because getPrototypeOf is called
  // implicitly when marshalling object and prototype chain.
  //
  // Above call passes a crosss-site object to "g" context. That call accesses
  // x.name (the proxy) in "g" context, thus will marshal value proxy p and
  // its prototype chain to "g" context. Accessing prototype chain calls the
  // trap (value g.test) in "g" context. Since the trap itself is also from "g"
  // context, CrossSite::CommonThunk determines no marshalling required.
  // However, handler/target args were from "p" context and need marshalling.
})();

// has trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x, name) { name in x }', 'samethread');
  var p = new Proxy({}, {
    has: g.test
  });
  g.test(p);
})();

// isExtensible trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x) { return Object.isExtensible(x) }', 'samethread');
  var p = new Proxy({}, {
    isExtensible: g.test
  });
  g.test(p);
})();

// ownKeys trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x) { return Object.getOwnPropertyNames(x) }',
    'samethread');
  var p = new Proxy(function() {}, {
    ownKeys: g.test
  });

  g.test(p);
})();

// preventExtensions trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x) { return Object.preventExtensions(x) }', 'samethread');
  var p = new Proxy({}, {
    preventExtensions: g.test
  });
  g.test(p);
})();

// set trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x, name, val) { x[name] = val }', 'samethread');
  var p = new Proxy({}, {
    set: g.test
  });

  g.test(p, 'abc', 1);
})();

// setPrototypeOf trap needs handler/target marshalling
(function() {
  var g = WScript.LoadScript(
    'function test(x, proto) { return Object.setPrototypeOf(x, proto) }',
    'samethread');
  var p = new Proxy({}, {
    setPrototypeOf: g.test
  });

  g.test(p, {});
})();


print('PASS');
