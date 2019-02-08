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
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
