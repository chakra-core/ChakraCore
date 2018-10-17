//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Setting proxy object on Map and WeakMap",
        body() {
            [WeakMap, Map].forEach(function(ctor) {
                var target = {};
                let p = new Proxy(target, {});
                let map = new ctor();
                map.set(p, 101);
                assert.areEqual(map.get(p), 101, ctor.name + " map should be able to set and get the proxy object");
                p.x = 20;
                assert.areEqual(target.x, 20, "target object should work as expected even after proxy object is added to map");
            });
        }
    },
    {
        name: "Setting proxy object on Map and WeakMap - multiple sets and delete",
        body() {
            [WeakMap, Map].forEach(function(ctor) {
                var target = {};
                let p = new Proxy(target, {});
                let map = new ctor();
                map.set(p, 101);
                assert.areEqual(map.get(p), 101);
                map.delete(p);
                assert.areEqual(map.get(p), undefined, ctor.name + " map can remove the proxy object properly");
                map.set(p, 102);
                assert.areEqual(map.get(p), 102, ctor.name + " proxy object can be set again and it returns 102");
                p.x = 20;
                assert.areEqual(target.x, 20, "target object should work as expected even after proxy object is added to map");
            });
        }
    },
    {
        name: "Assertion validation : returning descriptor during getOwnPropertyDescriptor should not pollute the descriptor",
        body() {
            var target = {};
            var handler = {};
            var getOwnPropertyDescriptorCalled = false;
            handler['defineProperty'] = function () {
                assert.fail("This function will not be called as 'getOwnPropertyDescriptor' will add accessor");
            };

            handler['getOwnPropertyDescriptor'] = function (t, property) {
                getOwnPropertyDescriptorCalled = true;
                Object.defineProperty(t, 'abc', { set: function () { } });
                return Reflect.getOwnPropertyDescriptor(t, property);
            };

            var proxy = new Proxy(target, handler);
            proxy.abc = undefined;
            assert.isTrue(getOwnPropertyDescriptorCalled);
        }
    },
    {
        name: "Assertion validation : returning descriptor with writable false should not defineProperty again.",
        body() {
            var target = {};
            var handler = {};
            var getOwnPropertyDescriptorCalled = false;
            handler['defineProperty'] = function () {
                assert.fail("This function will not be called as 'getOwnPropertyDescriptor' will add property with writable false");
            };

            handler['getOwnPropertyDescriptor'] = function (t, property) {
                getOwnPropertyDescriptorCalled = true;
                Object.defineProperty(t, 'abc', { value : 1, writable : false });
                return Reflect.getOwnPropertyDescriptor(t, property);
            };

            var proxy = new Proxy(target, handler);
            proxy.abc = undefined;
            assert.isTrue(getOwnPropertyDescriptorCalled);
        }
    },
    {
        name: "No property found at getOwnPropertyDescriptor will call defineProperty",
        body() {
            var target = {};
            var handler = {};
            var definePropertyCalled = false;
            var getOwnPropertyDescriptorCalled = false;
            handler['defineProperty'] = function () {
                definePropertyCalled = true;
            };

            handler['getOwnPropertyDescriptor'] = function (t, property) {
                getOwnPropertyDescriptorCalled = true;
                return Reflect.getOwnPropertyDescriptor(t, property);
            };

            var proxy = new Proxy(target, handler);
            proxy.abc = undefined;
            assert.isTrue(definePropertyCalled);
            assert.isTrue(getOwnPropertyDescriptorCalled);
        }
    },
    {
        name: "Cross-site on proxy exercising function trap - no function handler provided",
        body() {
            var targetCalled = false;
            var func4 = function () { targetCalled = true; };
            var v0 = new Proxy(func4, {});

            var anotherScript = `function foo() {
                    var a = undefined;
                    v0(a) > 1;
                }`;
            var sc0 = WScript.LoadScript(anotherScript, 'samethread');
            sc0.v0 = v0;
            sc0.foo();
            assert.isTrue(targetCalled);
        }
    },
    {
        name: "Type confusion in JavascriptProxy::SetPropertyTrap when using a Symbol",
        body: function () {
            try{ Reflect.set((new Proxy({}, {has: function(){ return true; }})), 'abc', 0x44444444, new Uint32Array); } catch(e){}
            try{ Reflect.set((new Proxy({}, {has: function(){ return true; }})), 'abc', 0x44444444, new Uint32Array); } catch(e){}

            var obj1 = Object.create(new Proxy({}, {}));
            obj1[Symbol.species] = 0;
        }
    },
    {
        name: "Cross-site on proxy exercising function trap - with 'apply' function trap",
        body() {
            var trapCalled = false;
            var func4 = function () {};
            var handler = {
                apply : function(a, b, c) {
                    trapCalled = true;
                }
            };
            var v0 = new Proxy(func4, handler);

            var anotherScript = `function foo() {
                    var a = undefined;
                    v0(a) > 1;
                }`;
            var sc0 = WScript.LoadScript(anotherScript, 'samethread');
            sc0.v0 = v0;
            sc0.foo();
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in getPrototypeOf trap",
        body() {
            var trapCalled = false;
            var handler = {
                getPrototypeOf : function(a, b) {
                    trapCalled = true;
                    obj.revoke();
                    return {};
                }
            };

            var obj = Proxy.revocable({}, handler);
            Object.getPrototypeOf(obj.proxy);
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in setPrototypeOf trap",
        body() {
            var trapCalled = false;
            var handler = {
                setPrototypeOf : function(a, b) {
                    trapCalled = true;
                    obj.revoke();
                    return true;
                }
            };

            var obj = Proxy.revocable({}, handler);
            var ret = Object.setPrototypeOf(obj.proxy, {});
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in isExtensible trap",
        body() {
            var trapCalled = false;
            var handler = {
                isExtensible : function(a, b) {
                    trapCalled = true;
                    obj.revoke();
                    return true;
                }
            };

            var obj = Proxy.revocable({}, handler);
            var ret = Object.isExtensible(obj.proxy);
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in preventExtensions trap",
        body() {
            var trapCalled = false;
            var handler = {
                preventExtensions : function(a, b) {
                    trapCalled = true;
                    obj.revoke();
               }
            };

            var obj = Proxy.revocable({}, handler);
            Object.preventExtensions(obj.proxy);
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in getOwnPropertyDescriptor trap",
        body() {
            var trapCalled = false;
            var handler = {
                getOwnPropertyDescriptor : function(a, b, c) {
                    trapCalled = true;
                    obj.revoke();
                }
            };

            var obj = Proxy.revocable({}, handler);
            Object.getOwnPropertyDescriptor(obj.proxy, 'a');
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in getOwnPropertyDescriptor trap, undefined argument",
        body() {
            var trapCalled = false;
            var handler = {
                getOwnPropertyDescriptor: function (a, b, c) {
                    trapCalled = true;
                    obj.revoke();

                    // used to cause AV
                    a[undefined] = new String();
                }
            };

            var obj = Proxy.revocable({}, handler);
            Object.getOwnPropertyDescriptor(obj.proxy);
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in getOwnPropertyDescriptor trap, not undefined return",
        body() {
            var trapCalled = false;
            var handler = {
                getOwnPropertyDescriptor: function (a, b, c) {
                    trapCalled = true;
                    let result = Object.getOwnPropertyDescriptor(obj, 'proxy');

                    obj.revoke();

                    // used to cause AV
                    return result;
                }
            };

            var obj = Proxy.revocable({}, handler);
            Object.getOwnPropertyDescriptor(obj.proxy);
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy by invoking getOwnPropertyDescriptor trap but no handler present",
        body() {
            var trapCalled = false;
            var handler = {
                get(t, propertyKey) {
                    trapCalled = true;
                    if (propertyKey === "getOwnPropertyDescriptor")
                        obj.revoke();
                }
            };

            var obj = Proxy.revocable({a:0}, new Proxy({}, handler));
            Object.getOwnPropertyDescriptor(obj.proxy, 'a');
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in has trap",
        body() {
            var trapCalled = false;
            var handler = {
                has : function(a, b, c) {
                    trapCalled = true;
                    obj.revoke();
                    return false;
                }
            };

            var obj = Proxy.revocable({}, handler);
            'a' in obj.proxy;
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in get trap",
        body() {
            var trapCalled = false;
            var handler = {
                get : function (a, b, c) {
                    trapCalled = true;
                    obj.revoke();
                    return {};
                }
            };

            var obj = Proxy.revocable({}, handler);
            var ret = obj.proxy.a;
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in set trap",
        body() {
            var trapCalled = false;
            var handler = {
                set : function (a, b, c) {
                    trapCalled = true;
                    obj.revoke();
                    return {};
                }
            };

            var obj = Proxy.revocable({}, handler);
            obj.proxy.a = 10;
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in deleteProperty trap",
        body() {
            var trapCalled = false;
            var handler = {
                deleteProperty : function (a, b, c) {
                    trapCalled = true;
                    obj.revoke();
                    return {};
                }
            };

            var obj = Proxy.revocable({}, handler);
            delete obj.proxy.a;
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in ownKeys trap",
        body() {
            var trapCalled = false;
            var handler = {
                ownKeys : function (a, b, c) {
                    trapCalled = true;
                    obj.revoke();
                    return {};
                }
            };

            var obj = Proxy.revocable({}, handler);
            Object.keys(obj.proxy);
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Missing item from ownKeys trap should throw error",
        body() {
            var keys = ['a'];
            keys[100] = 'b';
            var proxy = new Proxy({ a: 1, b: 2 }, {
                ownKeys: function () { return keys; }
            });

            assert.throws(
                () => Object.keys(proxy),
                TypeError);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in apply trap",
        body() {
            var trapCalled = false;
            var handler = {
                get apply () {
                    trapCalled = true;
                    obj.revoke();
                }
            };

            var obj = Proxy.revocable(() => {}, handler);
            obj.proxy();
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Assertion validation : revoking the proxy in construct trap",
        body() {
            var trapCalled = false;
            var handler = {
                get construct () {
                    trapCalled = true;
                    obj.revoke();
                    return () => { return {}; };
                }
            };

            var obj = Proxy.revocable(function() {}, handler);
            new obj.proxy();
            assert.isTrue(trapCalled);
        }
    },
    {
        name: "Extending a proxy with an es6 class",
        body() {
            function parent() { this.noTrap = true; }
            var proxyNoTrap = new Proxy(parent, {});
            var handler = {
                construct : function () {
                    this.other = true;
                    return { trap: true };
                }
            }
            var proxyWithTrap = new Proxy(parent, handler );

            class NoTrap extends proxyNoTrap
            {
                constructor()
                {
                    super();
                    this.own = true;
                }
                a () { return true; }
            }

            class WithTrap extends proxyWithTrap
            {
                constructor()
                {
                    super();
                    this.own = true;
                }
                a () { return true; }
            }
            
            var notrap = new NoTrap();
            assert.isTrue(notrap.own);
            assert.isTrue(notrap.a());
            assert.isTrue(notrap.noTrap);
            
            var withtrap = new WithTrap();
            assert.isTrue(withtrap.own);
            assert.isUndefined(withtrap.a);
            assert.isTrue(withtrap.trap);
            assert.isUndefined(withtrap.other);
            assert.isUndefined(withtrap.noTrap);
        }
    },
    {
        name: "Constructing object from a proxy object (which has proxy as target) should not fire an Assert (OS# 17516464)",
        body() {
            function Foo(a) {
                this.x = a;
            }
            var proxy = new Proxy(Foo, {});
            var proxy1 = new Proxy(proxy, {});
            var proxy2 = new Proxy(proxy1, {});
            var obj1 = new proxy2(10);
            assert.areEqual(10, obj1.x);

            var obj2 = Reflect.construct(proxy2, [20]);
            assert.areEqual(20, obj2.x);
        }
    },
    {
        name: "Proxy's ownKeys is returning duplicate keys should throw",
        body() {
            var proxy = new Proxy({}, {
                ownKeys: function (t) {
                    return ["a", "a"];
                }
            });
            assert.throws(()=> { Object.keys(proxy);}, TypeError, "proxy's ownKeys is returning duplicate keys", "Proxy's ownKeys trap returned duplicate keys");
        }
    },
    {
        name : "Proxy with DefineOwnProperty trap should not get descriptor properties twice",
        body() {
            const desc = { };
            let counter = 0;
            let handler = {
                defineProperty : function (oTarget, sKey, oDesc) { 
                    return Reflect.defineProperty(oTarget, sKey, oDesc); 
                }
            };
            Object.defineProperty(desc, "writable", { get: function () { ++counter; return true; }});
            Object.defineProperty(new Proxy({}, handler), "test", desc);
            assert.areEqual(1, counter, "Writable property on descriptor should only be checked once");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
