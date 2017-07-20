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
            assert.throws( () => { Object.getOwnPropertyDescriptor(obj.proxy, 'a'); }, TypeError);
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
            
            var obj = Proxy.revocable(() => {}, handler);
            new obj.proxy();
            assert.isTrue(trapCalled);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
