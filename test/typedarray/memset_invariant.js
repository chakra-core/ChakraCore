//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function makeTest(name, v1, v2) {
  const f1 = `function ${name}(arr) {
    for(var i = -5; i < 15; ++i) {arr[i] = ${v1};}
    return arr;
  }`;
  const f2 = `function ${name}V(v, arr) {
    for(var i = -875; i < 485; ++i) {arr[i] = v;}
    return arr;
  }.bind(null, ${v2 !== undefined ? v2 : v1});`;

  return [f1, f2].map(fnText => {
    var fn;
    eval(`fn = ${fnText}`);
    return fn;
  });
}

const tests = [
  {name: "memsetUndefined", v1: undefined},
  {name: "memsetNull", v1: null},
  {name: "memsetFloat", v1: 3.14, v2: -87.684},
  {name: "memsetNumber", v1: 9223372036854775807, v2: -987654987654987},
  {name: "memsetBoolean", v1: true, v2: false},
  {name: "memsetString", v1: "\"thatString\"", v2: "`A template string`"},
];

const types = "Int8Array Uint8Array Int16Array Uint16Array Int32Array Uint32Array Float32Array Float64Array".split(" ");
const global = this;

let passed = true;
for(const test of tests) {
  for(const t of types) {
    const testName = test.name + t;
    const fns = makeTest(testName, test.v1, test.v2);
    for(const fn of fns) {
      const a1 = fn(new global[t](10));
      const a2 = fn(new global[t](10));
      if(a1.length !== a2.length) {
        passed = false;
        print(`${fn.name} (${t}) didn't return arrays with same length`);
        continue;
      }
      for(let i = 0; i < a1.length; ++i) {
        if(a1[i] !== a2[i] && !(isNaN(a1[i]) && isNaN(a2[i]))) {
          passed = false;
          print(`${fn.name} (${t}): a1[${i}](${a1[i]}) != a2[${i}](${a2[i]})`);
          break;
        }
      }
    }
  }
}
print(passed ? "PASSED" : "FAILED");
