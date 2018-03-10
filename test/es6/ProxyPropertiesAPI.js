//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

let tests = [
   {
       name: "GetProxyProperties: no argugments",
       body: function () {
           let properties = WScript.GetProxyProperties();
           assert.isUndefined(properties, "ProxyProperties of nothing should be undefined.");
        }
   },
   {
        name: "GetProxyProperties: non-proxy arguments",
        body: function () {
            let properties = WScript.GetProxyProperties(undefined);
            assert.isUndefined(properties, "ProxyProperties of undefined should be undefined.");
            properties = WScript.GetProxyProperties(1);
            assert.isUndefined(properties, "ProxyProperties of number should be undefined.");
            properties = WScript.GetProxyProperties({});
            assert.isUndefined(properties, "ProxyProperties of non-proxy object should be undefined.");
        }
    },
    {
        name: "GetProxyProperties: revocable Proxy",
        body: function () {
            let revocable = Proxy.revocable({someProperty: true, otherProperty: false}, {otherProperty: true, newProperty: 5});
            let proxy = revocable.proxy;
            let properties = WScript.GetProxyProperties(proxy);

            let names = Object.getOwnPropertyNames(properties);
            assert.areEqual(names.length, 3, "proxy properties names should have length 3.");
            assert.isTrue(names.includes("target"));
            assert.isTrue(names.includes("handler"));
            assert.isTrue(names.includes("revoked"));
            assert.isFalse(properties.revoked, "Revoked bool should be false.");

            names = Object.getOwnPropertyNames(properties.target);
            assert.areEqual(names.length, 2, "proxy properties target names should have length 2.");
            assert.areEqual(properties.target.someProperty, true);
            assert.areEqual(properties.target.otherProperty, false);
            
            names = Object.getOwnPropertyNames(properties.handler);
            assert.areEqual(names.length, 2, "proxy properties handler names should have length 2.");
            assert.areEqual(properties.handler.newProperty, 5);
            assert.areEqual(properties.handler.otherProperty, true);

            revocable.revoke();
            properties = WScript.GetProxyProperties(proxy);

            names = Object.getOwnPropertyNames(properties);
            assert.areEqual(names.length, 3, "proxy properties names for revokes proxy should have length 3.");
            assert.isTrue(names.includes("target"));
            assert.isTrue(names.includes("handler"));
            assert.isTrue(properties.revoked, "Revoked bool should be true.");

            assert.isUndefined(properties.target, "Target of revoked proxy should be undefined.");
            assert.isUndefined(properties.handler, "Handler of revoked proxy should be undefined.");
        }
    },
    {
        name: "GetProxyProperties: normal Proxy",
        body: function () {
            let proxy = new Proxy({someProperty: true, otherProperty: false}, {otherProperty: true, newProperty: 5});
            let properties = WScript.GetProxyProperties(proxy);

            let names = Object.getOwnPropertyNames(properties);
            assert.areEqual(names.length, 3, "proxy properties names should have length 3");
            assert.isTrue(names.includes("target"));
            assert.isTrue(names.includes("handler"));
            assert.isTrue(names.includes("revoked"));
            assert.isFalse(properties.revoked, "Revoked bool should be false.");

            names = Object.getOwnPropertyNames(properties.target);
            assert.areEqual(names.length, 2, "proxy properties target names should have length 2");
            assert.areEqual(properties.target.someProperty, true);
            assert.areEqual(properties.target.otherProperty, false);
            
            names = Object.getOwnPropertyNames(properties.handler);
            assert.areEqual(names.length, 2, "proxy properties handler names should have length 2");
            assert.areEqual(properties.handler.newProperty, 5);
            assert.areEqual(properties.handler.otherProperty, true);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });