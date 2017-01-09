//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) {
  this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

function getPropertiesString(obj) {
  var props = []
  for (var x in obj) {
    props.push(x);
  }
  props = props.sort();
  return props.join();
}

var Tests = [function () {
    var obj = {}; // Starts with preinitialized object literal type T0
    obj.a1 = 1; // New type T1 with inlineSlotCapacity 0, a1 goes to auxSlot of capacity 4, new TypePath
    obj.a2 = 2; // New type T2, a2 goes to auxSlot [1], TypePath remains same with capacity 10
    obj.a3 = 3; // New Type T3, a3 goes to auxSlot[2], TypePath remains same with capacity 10
    delete obj.a3; // Go to Type T3, Both current type T3 and predecessor type T2 are not ObjectHeaderInline no movement needed
    assert.isTrue(obj.a1 === 1);
    assert.isTrue(obj.a2 === 2);
    assert.isTrue(obj.a3 === undefined);
  },
  function () {
    var obj = {}; // Starts with preinitialized object literal type T0
    obj.b1 = 1; // New type T1 with inlineSlotCapacity 0, b1 goes to auxSlot of capacity 4, new TypePath
    obj.b2 = 2; // New type T2, b2 goes to auxSlot [1], TypePath remains same with capacity 10

    var obj1 = {};
    obj1.b1 = 1; // Share same type T1 with obj, T1 is marked as Shared
    obj1.b3 = 3; // Branched TypePath and created new Type T3, T1 have 2 successors

    delete obj1.b3; // Go to Type T1, Both current type T3 and predecessor type T1 are not ObjectHeaderInline no movement needed
    assert.isTrue(obj.b1 === 1);
    assert.isTrue(obj.b2 === 2);
    assert.isTrue(obj.b3 === undefined);

    assert.isTrue(obj1.b1 === 1);
    assert.isTrue(obj1.b2 === undefined);
    assert.isTrue(obj1.b3 === undefined);
  },
  function () {
    var obj = {
      c1: 1
    }; // Type T0, ObjectHeaderInlined with inlineSlotCapacity 2, c1 goes to inlineSlot[0] same as auxSlot address
    obj.c2 = 2; // New type T1 a2 goes to inlineSlot 1

    delete obj.c2; // Goes back to T0, Both current type T1 and predecessor type T0 are ObjectHeaderInline with same inlineSlotCapacity so no movement
    assert.isTrue(obj.c1 === 1);
    assert.isTrue(obj.c2 === undefined);
  },
  function () {
    var obj = {
      d1: 1,
      d2: 2
    }; // Type T, ObjectHeaderInlined with inlineSlotCapacity 2, d1 goes to inlineSlot[0] and d2 to inlineSlot[1]
    var obj1 = {
      d1: 1,
      d2: 2
    }; // Share Type with Obj and now its marked Shared

    delete obj.d2; // Goes back to T0, Both current type and predecessor type are ObjectHeaderInline with same inlineSlotCapacity so no movement
    assert.isTrue(obj.d1 === 1);
    assert.isTrue(obj.d2 === undefined);
    assert.isTrue(obj1.d1 === 1);
    assert.isTrue(obj1.d2 === 2);
  },
  function () {
    function foo() {
      this.e1 = 1; // Type T1
      this.e2 = 2; // Type T2
      this.e3 = 3; // Type T3
      this.e4 = 4; // Type T4
      this.e5 = 5; // Type T5
    }
    var obj = new foo();
    var obj1 = new foo();
    // This will shrink the inlineSlotCapacity from 8 to 6
    var obj2 = new foo();

    delete obj1.e5; // Move to Type T4
    assert.isTrue(obj.e1 === 1);
    assert.isTrue(obj.e2 === 2);
    assert.isTrue(obj.e3 === 3);
    assert.isTrue(obj.e4 === 4);
    assert.isTrue(obj.e5 === 5);

    assert.isTrue(obj1.e1 === 1);
    assert.isTrue(obj1.e2 === 2);
    assert.isTrue(obj1.e3 === 3);
    assert.isTrue(obj1.e4 === 4);
    assert.isTrue(obj1.e5 === undefined);

    assert.isTrue(obj2.e1 === 1);
    assert.isTrue(obj2.e2 === 2);
    assert.isTrue(obj2.e3 === 3);
    assert.isTrue(obj2.e4 === 4);
    assert.isTrue(obj2.e5 === 5);
  },
  function () {
    function foo() {
      this.f1 = 1;
      this.f2 = 2;
      this.f3 = 3;
      this.f4 = 4;
      this.f5 = 5;
      this.f6 = 6;
      this.f7 = 7;
      this.f8 = 8;
    }
    var obj = new foo();
    var obj1 = new foo();
    // No shrinking as current inlineSlot 8 is aligned
    var obj2 = new foo();

    delete obj1.f8; // Move to Type till f7
    assert.isTrue(obj1.f1 === 1);
    assert.isTrue(obj1.f2 === 2);
    assert.isTrue(obj1.f3 === 3);
    assert.isTrue(obj1.f4 === 4);
    assert.isTrue(obj1.f5 === 5);
    assert.isTrue(obj1.f6 === 6);
    assert.isTrue(obj1.f7 === 7);
    assert.isTrue(obj1.f8 === undefined);
  },
  function () {
    function foo() {
      this.g1 = 1;
      this.g2 = 2;
      this.g3 = 3;
      this.g4 = 4;
      this.g5 = 5;
      this.g6 = 6;
      this.g7 = 7;
      this.g8 = 8;
    }
    var obj = new foo();
    obj.g9 = 9; // Create auxSlots, Move g1-g6 to inlineSlots and g7-g8 to auxSlot. Add g9 to auxSlot

    delete obj.g9; // ReOptimize - shift g1-g6 up and move g7-g8 to inlineSlot
    assert.isTrue(obj.g1 === 1);
    assert.isTrue(obj.g2 === 2);
    assert.isTrue(obj.g3 === 3);
    assert.isTrue(obj.g4 === 4);
    assert.isTrue(obj.g5 === 5);
    assert.isTrue(obj.g6 === 6);
    assert.isTrue(obj.g7 === 7);
    assert.isTrue(obj.g8 === 8);
    assert.isTrue(obj.g9 === undefined);
  },
  function () {
    var obj = {};
    obj.h1 = 1; // Type T1, inlineSlotSize 0, h1 in auxSlot[0]
    obj.h2 = 2; // Type T2, h2 in auxSlot[1]
    obj[0] = 4; // Created objectArray
    obj.h3 = 3; // Type T3, h3 in auxSlot[2]
    delete obj.h3; // Move to T2
    assert.isTrue(obj.h1 === 1);
    assert.isTrue(obj.h2 === 2);
    assert.isTrue(obj.h3 === undefined);
    assert.isTrue(obj[0] === 4);
  },
  function () {
    function foo() {
      this.i1 = 1,
      this.i2 = 2
    }
    var obj = new foo();
    var obj1 = new foo();
    var obj2 = new foo();

    obj[0] = 10; // DeoptimizeObjectHeaderInlining and create objectArray, no predecessor type
    obj[1] = 11;
    delete obj.i2; // No predecessor type
    assert.isTrue(obj.i1 === 1);
    assert.isTrue(obj.i2 === undefined);
    assert.isTrue(obj[0] === 10);
    assert.isTrue(obj[1] === 11);
  },
  function () {
    var obj = {
      j1: 1,
      j2: 2,
      j3: 3,
      j4: 4
    }

    // Populate forInCache
    for (var i in obj) {}

    var expectedProps = "j1j2j3j4";
    var elemCount = 0;
    var propsString = "";
    for (var i in obj) {
      propsString += i;
      elemCount++;
      if (elemCount == 2) {
        delete obj.j4; // delete last property
      } else if (elemCount == 3) {
        obj.j4 = 5; // Add property back
      }
    }
    assert.isTrue(propsString === expectedProps);
  },
  function () {
    var obj = {
      k1: 1,
      k2: 2,
      k3: 3,
      k4: 4
    }

    // Populate forInCache
    for (var i in obj) {}

    var expectedProps = "k1k2k3k4";
    var elemCount = 0;
    var propsString = "";
    for (var i in obj) {
      propsString += i;
      elemCount++;
      if (elemCount == 2) {
        delete obj.k4; // delete last property
        obj.k4 = 5; // Add property back
        obj.k5 = 6; // Shouldn't be enumerated
      }
    }
    assert.isTrue(propsString === expectedProps);
  },
  function () {
    var obj = {
      l1: 1,
      l2: 2,
      l3: 3
    }

    // No prepopulated cache
    var expectedProps = "l1l2l3";
    var elemCount = 0;
    var propsString = "";
    for (var i in obj) {
      propsString += i;
      elemCount++;
      if (elemCount == 2) {
        delete obj.l3; // delete last property
        obj.l3 = 5 // Add property back
      }
    }
    assert.isTrue(propsString === expectedProps);
  },
  function () {
    var obj = {
      m1: 1,
      m2: 2,
      m3: 3
    };

    // Same type as obj
    var obj1 = {
      m1: 1,
      m2: 2,
      m3: 3
    };

    // No prepopulated cache
    var expectedProps = "m1m2m3";
    var elemCount = 0;
    var propsString = "";
    for (var i in obj1) {
      propsString += i;
      elemCount++;
      if (elemCount == 2) {
        delete obj.l3; // No optimization as forInCache is type based
        obj.l3 = 5 // Add property back
      }
    }
    assert.isTrue(propsString === expectedProps);
  },
  function () {
    var obj = {
      n1: 1,
      n2: 2
    };

    obj.n3 = 3; // Converted to auxSlots, predecessor type is ObjectHeaderInlined
    obj[10] = 10; // Added objectArray
    delete obj.n3;
    assert.isTrue(obj[10] === 10);
    assert.isTrue(obj.n3 === undefined);
  },
  function () {
    var arrObj0 = {};
    var func1 = function () {
      delete arrObj0.length;
      arrObj0 = {
        method0: function () {},
        method1: function () {}
      };
    };
    var func4 = function () {
      arrObj0[15] = 1;
      func1();
      return arrObj0.length = arrObj0;
    };
    func4();
    func4();
  },
  function () {
    var obj1 = {};
    obj1.o1 = 1;
    var obj2 = {};
    obj2.o2 = 1; // Set a property (last)
    obj2.__proto__ = obj1; // Modify the prototype
    delete obj2.o2; // Trigger delete last property
    assert.areEqual(1, obj2.o1, "");
  },
  function () {
    var obj1 = {};
    obj1.p1 = 1;
    var obj2 = {};
    obj2.__proto__ = obj1; // Change prototype first
    obj2.p2 = 1; // Add a last property
    delete obj2.p2; // Trigger delete last property
    assert.areEqual(1, obj2.p1, "");
  },
  function () {
    var obj1 = {};
    obj1.q1 = 1;
    obj1.q2 = 1;
    var obj2 = {};
    obj2.q3 = 1; // Add last property to object
    obj1.q4 = 1; // Add another property to prototype
    obj2.__proto__ = obj1; // Change prototype
    delete obj2.q3; // Delete last property
    assert.areEqual('q1,q2,q4', getPropertiesString(obj2), "");
  },
  function () {
    var obj1 = {};
    obj1.r1 = 1;
    obj1.r2 = 1;
    var obj2 = {};
    obj2.r3 = 1; // Add a property
    obj2.r4 = 1; // Add another (last) property
    obj2.__proto__ = obj1; // Change prototype
    delete obj2.r4; // Trigger delete last property
    assert.areEqual('r1,r2,r3', getPropertiesString(obj2), "");
    delete obj2.r3;
    assert.areEqual('r1,r2', getPropertiesString(obj2), "");
  }
]

for (var i = 0; i < 3; ++i) {
  for (var j = 0; j < Tests.length; ++j) {
    Tests[j]();
  }
}

WScript.Echo("Pass");
