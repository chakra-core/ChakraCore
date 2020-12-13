//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
       name: "Class member captures super as index reference via a lambda",
       body: function ()
       {
           class Base {}
           class Derived extends Base {
               test() {
                   const _s = name => super[name];
                   return _s('constructor').name;
               }
           }
           
           assert.areEqual("Base", new Derived().test());
       }
    },
    {
       name: "Class member captures super as dot reference via a lambda",
       body: function ()
       {
           class Base {}
           class Derived extends Base {
               test() {
                   const _s = () => super.constructor;
                   return _s().name;
               }
           }
           
           assert.areEqual("Base", new Derived().test());
       }
    },
    {
       name: "Class member captures super as dot reference via object with a getter",
       body: function ()
       {
           class Base {}
           class Derived extends Base {
               test() {
                   const _super = Object.create(null, {
                       constructor: {
                           get: () => super.constructor
                       }
                   });
                   
                   return _super.constructor.name;
               }
           }
           
           assert.areEqual("Base", new Derived().test());
       }
    },
    {
       name: "Class member captures super in lambda via getter from outer super reference",
       body: function ()
       {
           class Base {}
           class Derived extends Base {
               test() {
                   const con = super.constructor;
                   const prop2 = {
                       constructor: {
                           get: () => con
                       }
                   };
                   const _super2 = Object.create(null, prop2);
                   return _super2.constructor.name;
               }
           }
           
           assert.areEqual("Base", new Derived().test());
       }
    },
    {
       name: "Class member trying to write to non-writable property of receiver object via super-dot-assignment throws",
       body: function ()
       {
           class Base {}
           class Derived extends Base {
               test() {
                   assert.areEqual(this, obj, 'this === obj');

                   super.prop = 'something';
               }
           }

           var obj = new Derived();
           Object.defineProperty(obj, 'prop', { writable:false, value:'nothing' });
           assert.throws(()=>obj.test(), TypeError, 'Class methods are strict mode code. Cannot write to non-writable properties.', 'Assignment to read-only properties is not allowed in strict mode');
           assert.areEqual('nothing', obj.prop);
       }
    },
    {
       name: "Class member trying to write to writable property of receiver object via super-dot-assignment is fine",
       body: function ()
       {
           class Base {}
           class Derived extends Base {
               test() {
                   assert.areEqual(this, obj, 'this === obj');

                   super.prop = 'something';
               }
           }

           var obj = new Derived();
           Object.defineProperty(obj, 'prop', { writable:true, value:'nothing' });
           obj.test();
           assert.isTrue(obj.hasOwnProperty('prop'));
           assert.areEqual('something', obj.prop);
       }
    },
    {
       name: "Function writing to non-writable property of receiver object via super-dot-assignment throws in strict mode",
       body: function ()
       {
           function ctor() { }
           ctor.prototype = {
               test() {
                   'use strict';
                   super.prop = 'something';
               }
           };

           var obj = new ctor();
           Object.defineProperty(obj, 'prop', { writable:false, value:'nothing' });
           assert.throws(()=>obj.test(), TypeError, 'Strict mode code throws if we try to write to non-writable properties.', 'Assignment to read-only properties is not allowed in strict mode');
           assert.areEqual('nothing', obj.prop);
       }
    },
    {
       name: "Function writing to non-writable property of receiver object via super-dot-assignment silently fails in sloppy mode",
       body: function ()
       {
           function ctor() { }
           ctor.prototype = {
               test() {
                   super.prop = 'something';
               }
           };

           var obj = new ctor();
           Object.defineProperty(obj, 'prop', { writable:false, value:'nothing' });
           obj.test();
           assert.areEqual('nothing', obj.prop);
       }
    },
    {
       name: "Writing property to receiver via super-dot-assignment when receiver prototype-chain contains non-writable property doesn't throw",
       body: function ()
       {
           function ctor() { }
           ctor.prototype = {
               test() {
                   'use strict';
                   super.prop = 'something';
               }
           };

           var obj = new ctor();
           Object.defineProperty(obj.__proto__, 'prop', { writable:false, value:'nothing' });
           obj.test();
           assert.isTrue(obj.hasOwnProperty('prop'));
           assert.areEqual('something', obj.prop);
           assert.areEqual('nothing', obj.__proto__.prop);
       }
    },
    {
       name: "Writing property to receiver via super-dot-assignment when receiver prototype-chain contains getter but no setter doesn't throw",
       body: function ()
       {
           function ctor() { }
           ctor.prototype = {
               test() {
                   'use strict';
                   super.prop = 'something';
               }
           };

           var obj = new ctor();
           Object.defineProperty(obj.__proto__, 'prop', { get:()=>'nothing' });
           obj.test();
           assert.isTrue(obj.hasOwnProperty('prop'));
           assert.areEqual('something', obj.prop);
           assert.areEqual('nothing', obj.__proto__.prop);
       }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
