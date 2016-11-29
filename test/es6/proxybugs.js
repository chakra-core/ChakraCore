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
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
