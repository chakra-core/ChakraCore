//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
  this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
  {
      name: "Add, delete, modify properties after sealing",
      body: function () {
          let a = {x: 42};
          
          Object.seal(a);
          assert.isFalse(Object.isExtensible(a));
          assert.isTrue(Object.isSealed(a));

          // cannot add properties
          a.y = 17;
          assert.isFalse(a.hasOwnProperty('y'));
          assert.throws(function () { 'use strict'; a.y = 17; }, TypeError, 
            "Should throw on creating a new property in sealed object in strict mode", 
            "Cannot create property for a non-extensible object");

          // cannot delete properties
          assert.isFalse(delete a.x);
          assert.isTrue(a.hasOwnProperty('x'));
          assert.throws(function () { 'use strict'; delete a.x; }, TypeError, 
            "Should throw on creating a new property in sealed object in strict mode", 
            "Calling delete on 'x' is not allowed in strict mode");

          // cannot change prototype
          let b = {};
          assert.throws(function () { 'use strict'; Object.setPrototypeOf(a, b); }, TypeError, 
            "Should throw on creating a new property in sealed object in strict mode", 
            "Cannot create property for a non-extensible object");

          // ok to modify the existing property
          a.x = 17;
          assert.areEqual(17, a.x);
      }
  },
  {
    name: "Add, delete, modify indexed elements of an array after sealing",
    body: function () {
        let a = [42];
        a[2] = 43;
        
        Object.seal(a);
        assert.isFalse(Object.isExtensible(a));
        assert.isTrue(Object.isSealed(a));

        // the array cannot be extended
        a[3] = 17;
        assert.areEqual(3, a.length);
        assert.isFalse(a.hasOwnProperty('3'))
        assert.throws(function () { 'use strict'; a[3] = 17; }, TypeError, 
          "Should throw on creating a new property in sealed object in strict mode", 
          "Cannot create property for a non-extensible object");

        // a hole cannot be filled
        a[1] = 17;
        assert.areEqual(3, a.length);
        assert.isFalse(a.hasOwnProperty('1'))
        assert.throws(function () { 'use strict'; a[1] = 17; }, TypeError, 
          "Should throw on creating a new property in sealed object in strict mode", 
          "Cannot create property for a non-extensible object");

        // existing elements cannot be deleted
        assert.isFalse(delete a[0]);
        assert.isTrue(a.hasOwnProperty('0'));
        assert.throws(function () { 'use strict'; delete a[0]; }, TypeError, 
          "Should throw on creating a new property in sealed object in strict mode", 
          "Calling delete on '0' is not allowed in strict mode");

        // ok to modify an existing element
        a[0] = 17;
        assert.areEqual(17, a[0]);
    }
  },
  {
    name: "Add, delete, modify indexed elements of a typed array after sealing",
    body: function () {
        let a = new Int8Array(1);
        a[0] = 42;

        Object.seal(a);
        assert.isFalse(Object.isExtensible(a));
        assert.isTrue(Object.isSealed(a));

        /* 
        Typed arrays never allow adding or removing elements - should we test
        that attempt to add in strict mode doesn't throw as for standard arrays?
        (that's the current behavior of v8 and Chakra)
        Not clear what the spec is. 

        assert.throws(function () { 'use strict'; a[1] = 17; }, TypeError, 
          "Should throw on creating a new property in sealed object in strict mode", 
          "Cannot create property for a non-extensible object");
        */

        // ok to modify the existing element
        a[0] = 17;
        assert.areEqual(17, a[0]);
    }
  },
  {
    name: "Modify length of an array after sealing",
    body: function () {
        let a = [42, 17, 33];
        a.length = 4;
        Object.seal(a);

        let descr_len = Object.getOwnPropertyDescriptor(a, 'length');
        assert.isFalse(descr_len.configurable);
        assert.isTrue(descr_len.writable);

        // can increase length but cannot fill the tail
        a.length = 5;
        a[4] = "new!";
        assert.areEqual(5, a.length);
        assert.isFalse(a.hasOwnProperty('4'));

        // cannot truncate by reducing the length below the last defined element
        a.length = 1;
        assert.areEqual(3, a.length);
    }
  },
  {
    name: "Sealed versus frozen",
    body: function () {
        let a = {x: 42};
        Object.seal(a);
        assert.isTrue(Object.isSealed(a));
        assert.isFalse(Object.isFrozen(a));

        // https://tc39.github.io/ecma262/#sec-testintegritylevel (7.3.15)
        // empty objects are effectively frozen after being sealed
        let empty_obj = {};
        Object.seal(empty_obj);
        assert.isTrue(Object.isSealed(empty_obj));
        assert.isTrue(Object.isFrozen(empty_obj));

        // similar to above, a sealed object with all properties individually 
        // set to non-writable and non-configurable is frozen
        let b = {};
        Object.defineProperty(b, 'x', { value: 42, writable: false });
        Object.seal(b);
        assert.isTrue(Object.isSealed(b));
        assert.isTrue(Object.isFrozen(b));

        // standard arrays
        let arr = [42];
        Object.seal(arr);
        assert.isTrue(Object.isSealed(arr));
        assert.isFalse(Object.isFrozen(arr));

        // typed arrays
        let ta = new Int8Array(4);
        Object.seal(ta);
        assert.isTrue(Object.isSealed(ta));
        assert.isFalse(Object.isFrozen(ta));

        // empty typed arrays are effectively frozen after being sealed
        let ta_empty = new Int8Array(0);
        Object.seal(ta_empty);
        assert.isTrue(Object.isSealed(ta_empty));
        assert.isTrue(Object.isFrozen(ta_empty));

        // frozen objects are sealed
        let c = {x: 42};
        Object.freeze(c);
        assert.isTrue(Object.isFrozen(c));
        assert.isTrue(Object.isSealed(c));
    }
  },
];

testRunner.runTests(tests, { verbose: false /*so no need to provide baseline*/ });