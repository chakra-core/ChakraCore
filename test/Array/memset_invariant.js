//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function makeTest(name, v1, v2) {
  const f1 = `function ${name}(arr) {
    for(var i = 0; i < 50; ++i) {arr[i] = ${v1};}
    return arr;
  }`;
  const f2 = `function ${name}V(v, arr) {
    for(var i = 0; i < 10; ++i) {arr[i] = v;}
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
  {name: "memset8bit", v1: "16&255", v2: "16&255"},
];

let passed = true;
for(const test of tests) {
  const testName = test.name;
  const fns = makeTest(testName, test.v1, test.v2);
  for(const fn of fns) {
    const a1 = fn(new Array(10));
    const a2 = fn(new Array(10));
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

function memsetSymbol() {const s = Symbol(); const arr = new Array(10); for(let i = 0; i < 10; ++i) {arr[i] = s;} return arr;}
function memsetSymbolV(v) {const arr = new Array(10); for(let i = 0; i < 10; ++i) {arr[i] = v;} return arr;}
function checkSymbols() {
  const s = Symbol();
  // Since symbol are unique, and we want to compare the result, we have to pass the same symbol everytime
  const a1 = memsetSymbolV(s);
  const a2 = memsetSymbolV(s);
  for(let i = 0; i < a1.length; ++i) {
    if(a1[i] !== a2[i]) {
      passed = false;
      // need explicit toString() for Symbol
      print(`memsetSymbolV: a1[${i}](${a1[i].toString()}) != a2[${i}](${a2 && a2[i].toString() || ""})`);
      break;
    }
  }

  memsetSymbol();
  const symbolArray = memsetSymbol();
  for(let i = 0; i < symbolArray.length; ++i) {
    if(typeof symbolArray[i] !== typeof s) {
      passed = false;
      print(`memsetSymbol: symbolArray[${i}] is not a Symbol`);
      break;
    }
  }
}
checkSymbols();

print(passed ? "PASSED" : "FAILED");
