//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
  this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
  {
      name: "Add, delete, modify properties after freezing",
      body: function () {
          let a = {x: 42};
          
          Object.freeze(a);
          assert.isFalse(Object.isExtensible(a));
          assert.isTrue(Object.isSealed(a));
          assert.isTrue(Object.isFrozen(a));

          // cannot add new properties
          a.y = 17;
          assert.isFalse(a.hasOwnProperty('y'));
          assert.throws(function () { 'use strict'; a.y = 17; }, TypeError, 
            "Should throw on creating a new property in frozen object in strict mode", 
            "Cannot create property for a non-extensible object");

          // cannot delete properties
          assert.isFalse(delete a.x);
          assert.isTrue(a.hasOwnProperty('x'));
          assert.throws(function () { 'use strict'; delete a.x; }, TypeError, 
            "Should throw on creating a new property in frozen object in strict mode", 
            "Calling delete on 'x' is not allowed in strict mode");

          // cannot change prototype
          let b = {};
          assert.throws(function () { 'use strict'; Object.setPrototypeOf(a, b); }, TypeError, 
            "Should throw on creating a new property in sealed object in strict mode", 
            "Cannot create property for a non-extensible object");

          // existing properties should be set to non-writable and non-configurable
          let descr = Object.getOwnPropertyDescriptor(a, 'x');
          assert.isFalse(descr.configurable);
          assert.isFalse(descr.writable);
      }
  },
  {
    name: "Add, delete, modify indexed elements of an array after freezing",
    body: function () {
        let a = [42];
        a[2] = 43;
        
        Object.freeze(a);
        assert.isFalse(Object.isExtensible(a));
        assert.isTrue(Object.isSealed(a));
        assert.isTrue(Object.isFrozen(a));

        // the array cannot be extended
        a[3] = 17;
        assert.areEqual(3, a.length);
        assert.isFalse(a.hasOwnProperty('3'))
        assert.throws(function () { 'use strict'; a[3] = 17; }, TypeError, 
          "Should throw on creating a new property in frozen object in strict mode", 
          "Cannot create property for a non-extensible object");

        // a hole cannot be filled
        a[1] = 17;
        assert.areEqual(3, a.length);
        assert.isFalse(a.hasOwnProperty('1'))
        assert.throws(function () { 'use strict'; a[1] = 17; }, TypeError, 
          "Should throw on creating a new property in frozen object in strict mode", 
          "Cannot create property for a non-extensible object");

        // existing elements cannot be deleted
        assert.isFalse(delete a[0]);
        assert.isTrue(a.hasOwnProperty('0'));
        assert.throws(function () { 'use strict'; delete a[0]; }, TypeError, 
          "Should throw on creating a new property in frozen object in strict mode", 
          "Calling delete on '0' is not allowed in strict mode");

        // existing elements cannot be modified
        let descr = Object.getOwnPropertyDescriptor(a, '0');
        assert.isFalse(descr.configurable);
        assert.isFalse(descr.writable);
        a[0] = 17;
        assert.areEqual(42, a[0]);
        assert.throws(function () { 'use strict'; a[0] = 17; }, TypeError, 
          "Should throw on creating a new property in frozen object in strict mode", 
          "Assignment to read-only properties is not allowed in strict mode");

        // the special 'length' property also cannot be modified
        let descr_len = Object.getOwnPropertyDescriptor(a, 'length');
        assert.isFalse(descr_len.configurable);
        assert.isFalse(descr_len.writable);
    }
  },
  {
    // what is the spec??? 
    // v8 doesn't allow freezing typed arrays at all
    name: "Add, delete, modify indexed elements of a typed array after freezing",
    body: function () {
        let a = new Int8Array(1);
        a[0] = 42;

        Object.freeze(a); // should it throw?

        assert.isFalse(Object.isExtensible(a));
        assert.isTrue(Object.isSealed(a));
        assert.isTrue(Object.isFrozen(a)); // should it return false?

        // current behavior:
        // even though the array is frozen it's ok to modify existing elements
        a[0] = 17;
        assert.areEqual(17, a[0]);
    }
  },
];

testRunner.runTests(tests, { verbose: false /*so no need to provide baseline*/ });