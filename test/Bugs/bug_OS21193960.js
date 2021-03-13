//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "OS21193960: Proxy [[Construct]] trap confuses super call flag",
        body: function () {
            function test(){
              let ctorCount = 0;
              function ctor() {
                  if (new.target !== undefined) {
                      ctorCount++;
                      this.prop0 = 123;
                      assert.isTrue(proxy === new.target, "proxy === new.target");
                  } else {
                      assert.fail('call ctor');
                  }
              }

              let ctor_prototype = ctor.prototype;
              let getCount = 0;
              let proxyHandler = {
                ['get'](handler, prop, target) {
                  getCount++;
                  assert.isTrue(prop !== 'prop0', "prop !== 'prop0'");
                  switch(prop) {
                    case 'prototype':
                      return ctor_prototype;
                  }
                }
              };

              var proxy = new Proxy(ctor, proxyHandler);
              let newProxy = new proxy;

              assert.isTrue(proxy !== newProxy, 'proxy !== newProxy');
              assert.areEqual('object', typeof newProxy, '"object" === typeof newProxy');
              assert.areEqual(1, ctorCount, "1 === ctorCount");

              assert.areEqual(123, newProxy.prop0, "123 === newProxy.prop0");
              assert.areEqual(1, getCount, "1 === getCount");
            };

            test();
            test();
        }
    },
    {
        name: "Proxy target is a base class",
        body: function () {
            function test(){
              let ctorCount = 0;
              class ctor {
                  constructor() {
                  if (new.target !== undefined) {
                      ctorCount++;
                      this.prop0 = 123;
                      assert.isTrue(proxy === new.target, "proxy === new.target");
                  } else {
                      assert.fail('call ctor');
                  }
                }
              }

              let ctor_prototype = ctor.prototype;
              let getCount = 0;
              let proxyHandler = {
                ['get'](handler, prop, target) {
                  getCount++;
                  assert.isTrue(prop !== 'prop0', "prop !== 'prop0'");
                  switch(prop) {
                    case 'prototype':
                      return ctor_prototype;
                  }
                }
              };

              var proxy = new Proxy(ctor, proxyHandler);
              let newProxy = new proxy;

              assert.isTrue(proxy !== newProxy, 'proxy !== newProxy');
              assert.areEqual('object', typeof newProxy, '"object" === typeof newProxy');
              assert.areEqual(1, ctorCount, "1 === ctorCount");

              assert.areEqual(123, newProxy.prop0, "123 === newProxy.prop0");
              assert.areEqual(2, getCount, "2 === getCount");
            };

            test();
            test();
        }
    },
    {
        name: "Proxy target is a derived class",
        body: function () {
            function test(){
              let baseCount = 0;
              class base {
                constructor() {
                  if (new.target !== undefined) {
                      baseCount++;
                      assert.isTrue(proxy === new.target, "proxy === new.target");
                  } else {
                      assert.fail('call base');
                  }
                }
              };

              let ctorCount = 0;
              class ctor extends base {
                  constructor() {
                  if (new.target !== undefined) {
                      ctorCount++;
                      super();
                      this.prop0 = 123;
                      assert.isTrue(proxy === new.target, "proxy === new.target");
                  } else {
                      assert.fail('call ctor');
                  }
                }
              }

              let ctor_prototype = ctor.prototype;
              let getCount = 0;
              let proxyHandler = {
                ['get'](handler, prop, target) {
                  getCount++;
                  assert.isTrue(prop !== 'prop0', "prop !== 'prop0'");
                  switch(prop) {
                    case 'prototype':
                      return ctor_prototype;
                  }
                }
              };

              var proxy = new Proxy(ctor, proxyHandler);
              let newProxy = new proxy;

              assert.isTrue(proxy !== newProxy, 'proxy !== newProxy');
              assert.areEqual('object', typeof newProxy, '"object" === typeof newProxy');
              assert.areEqual(1, ctorCount, "1 === ctorCount");
              assert.areEqual(1, baseCount, "1 === baseCount");

              assert.areEqual(123, newProxy.prop0, "123 === newProxy.prop0");
              assert.areEqual(2, getCount, "2 === getCount");
            };

            test();
            test();
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
