//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test(func) {
  console.log(func.toString());
  try {
    var result = func();
    if (result && result.next) {
      result.next();
    }
  } catch (e) {
    // Ignore
  }
}

function testFunctions() {
  function/*ß*/ a/*ß*/()/*ß*/ { console.log('a'); }
  test(a);
  var b = /*ß*/(/*ß*/)/*ß*/ => { console.log('b'); }
  test(b);
  async/*ß*/ function/*ß*/ c/*ß*/()/*ß*/ { console.log('c'); }
  test(c);
  function/*ß*/ */*ß*/d/*ß*/(/*ß*/)/*ß*/ { console.log('d'); }
  test(d);
}
testFunctions();

var objectMemberTest  = {
  a/*ß*/() /*ß*/{ console.log('a'); },
  b: /*ß*/()/*ß*/ => { console.log('b'); },
  async/*ß*/ c/*ß*/()/*ß*/ { console.log('c'); },
  */*ß*/ d/*ß*/()/*ß*/ { console.log('d'); },
  ['e']/*ß*/()/*ß*/ { console.log('e'); },
  async/*ß*/ ['f']/*ß*/()/*ß*/ { console.log('f'); },
  */*ß*/ ['g']/*ß*/()/*ß*/ { console.log('g'); },
  get/*ß*/()/*ß*/ { console.log('get'); },
  set/*ß*/()/*ß*/ { console.log('set'); },
  [/]/.exec(']')]/*ß*/()/*ß*/ { console.log('regex'); },
  [(function () { return 'h'})()]/*ß*/()/*ß*/ { console.log('function'); },
}

for (var i of Object.keys(objectMemberTest)) {
  test(objectMemberTest[i]);
}

var objectAccessorTest = {
  get/*ß*/ a/*ß*/()/*ß*/ { console.log('getter'); },
  set /*ß*/a/*ß*/(x)/*ß*/ { console.log('setter'); },
}

var d = Object.getOwnPropertyDescriptor(objectAccessorTest, 'a');
console.log(d.get.toString())
d.get();
console.log(d.set.toString())
d.set(0);

class ClassTest {
  constructor/*ß*/()/*ß*/ {}
  static /*ß*/a/*ß*/()/*ß*/ {}
  static /*ß*/async/*ß*/ b()/*ß*/ {}
  static /*ß*/*/*ß*/ c/*ß*/()/*ß*/ {}
  static /*ß*/['d']/*ß*/()/*ß*/ {}
  static /*ß*/async /*ß*/['e']/*ß*/()/*ß*/ {}
  static /*ß*/* /*ß*/['f']/*ß*/()/*ß*/ {}

  g/*ß*/()/*ß*/ {}
  async/*ß*/ h/*ß*/()/*ß*/ {}
  */*ß*/ i/*ß*/()/*ß*/ {}
  ['j']/*ß*/()/*ß*/ {}
  async/*ß*/ ['k']/*ß*/()/*ß*/ {}
  * /*ß*/['l']/*ß*/()/*ß*/ {}
}

var classInstance = new ClassTest();

for(var i of ['a', 'b', 'c', 'd', 'e', 'f']) {
  test(ClassTest[i]);
}

for(var i of ['g', 'h', 'i', 'j', 'k', 'l']) {
  test(classInstance[i]);
}
test(classInstance.constructor)

async function awaitTests() {
  return {
    [await 'a']/*ß*/()/*ß*/ { console.log("await a"); }
  }
}
awaitTests().then(o => {
  for (var i of Object.keys(o)) {
    test(o[i]);
  }
});

function * yieldTests() {
  return {
    [yield 'a']/*ß*/()/*ß*/ { console.log("yield a"); }
  }
}

var it = yieldTests();
var last;
do {
  last = it.next();
} while (!last.done);
for (var i of Object.keys(last.value)) {
  test(last.value[i]);
}