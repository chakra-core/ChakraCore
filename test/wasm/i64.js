//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/* global assert,testRunner,yargsParse,i64ToString */ // eslint rule
WScript.Flag("-wasmI64");
WScript.LoadScriptFile("../UnitTestFrameWork/UnitTestFrameWork.js");
WScript.LoadScriptFile("wasmutils.js");
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-module-builder.js");
WScript.LoadScriptFile("../UnitTestFrameWork/yargs.js");
const argv = yargsParse(WScript.Arguments, {
  number: ["start", "end", "verbose"],
  default: {
    verbose: 1,
    start: 0
  }
}).argv;

const comparisonOperators = [
  {name: "i64.eq", op: kExprI64Eq, check(a, b) {return a.high === b.high && a.low === b.low;}},
  {name: "i64.ne", op: kExprI64Ne, check(a, b) {return a.high !== b.high || a.low !== b.low;}},
  {name: "i64.lt_s", op: kExprI64LtS, check(a, b) {
    if (a.high !== b.high) {
      return a.high < b.high;
    }
    return (a.low>>>0) < (b.low>>>0);
  }},
  {name: "i64.lt_u", op: kExprI64LtU, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) < (b.high>>>0);
    }
    return (a.low>>>0) < (b.low>>>0);
  }},
  {name: "i64.gt_s", op: kExprI64GtS, check(a, b) {
    if (a.high !== b.high) {
      return a.high > b.high;
    }
    return (a.low>>>0) > (b.low>>>0);
  }},
  {name: "i64.gt_u", op: kExprI64GtU, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) > (b.high>>>0);
    }
    return (a.low>>>0) > (b.low>>>0);
  }},
  {name: "i64.le_s", op: kExprI64LeS, check(a, b) {
    if (a.high !== b.high) {
      return a.high < b.high;
    }
    return (a.low>>>0) <= (b.low>>>0);
  }},
  {name: "i64.le_u", op: kExprI64LeU, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) < (b.high>>>0);
    }
    return (a.low>>>0) <= (b.low>>>0);
  }},
  {name: "i64.ge_s", op: kExprI64GeS, check(a, b) {
    if (a.high !== b.high) {
      return a.high > b.high;
    }
    return (a.low>>>0) >= (b.low>>>0);
  }},
  {name: "i64.ge_u", op: kExprI64GeU, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) > (b.high>>>0);
    }
    return (a.low>>>0) >= (b.low>>>0);
  }},
];

const I32values = [0, 1, -1, -5, 5, 1<<32, -(1<<32), (1<<32)-1, 1<<31, -(1<<31), 1<<25, -1<<25];
const allPairs = [];
const tested = {};
for (let i = 0; i < I32values.length; ++i) {
  for (let j = i; j < I32values.length; ++j) {
    const v0 = I32values[i];
    const v1 = I32values[j];

    // Try all possible combinations
    for (let iShuffle = 0; iShuffle < (1 << 4); ++iShuffle) {
      const a = (iShuffle & 1) ? v1 : v0;
      const b = (iShuffle & 2) ? v1 : v0;
      const c = (iShuffle & 4) ? v1 : v0;
      const d = (iShuffle & 8) ? v1 : v0;

      const left = {high: a, low: b};
      const right = {high: c, low: d};
      const key = i64ToString(left) + i64ToString(right);
      if (!tested[key]) {
        tested[key] = true;
        allPairs.push([left, right]);
      }
    }
  }
}


function runTest(name, fn, expectedFn) {
  for (const pair of allPairs) {
    const [left, right] = pair;
    const i64Res = fn(left, right);
    const checkRes = expectedFn(left, right);
    const msg = `${name}(${i64ToString(left)}, ${i64ToString(right)})`;
    if (argv.verbose > 1) {
      console.log(`${msg} = ${i64Res}`);
    }
    assert.areEqual(checkRes, i64Res, msg);
  }
}

const comparisonTests = comparisonOperators.map(({name, op, check}) => ({
  name: `test ${name}`,
  body() {
    const builder = new WasmModuleBuilder();
    const body = [kExprGetLocal, 0, kExprGetLocal, 1, op];
    builder
      .addFunction("i64", makeSig([kWasmI64, kWasmI64], [kWasmI32]))
      .addBody(body)
      .exportFunc();
    const {exports: {i64}} = builder.instantiate();

    runTest(name, i64, (left, right) => (check(left, right) ? 1 : 0));
  }
}));

const branchingTests = comparisonOperators.map(({name, op, check}) => ({
  name: `test ${name} branching`,
  body() {
    const falseValue = 15;
    const trueValue = 24;
    const builder = new WasmModuleBuilder();
    const body = [
      kExprI32Const, falseValue,
      kExprSetLocal, 2, // tmp
      kExprGetLocal, 0, kExprGetLocal, 1, op,
      kExprIf, kWasmStmt,
        kExprI32Const, trueValue,
        kExprSetLocal, 2, // tmp
      kExprEnd,
      kExprGetLocal, 2, // return tmp
    ];
    builder
      .addFunction("i64", makeSig([kWasmI64, kWasmI64], [kWasmI32]))
      .addLocals({i32_count: 1})
      .addBody(body)
      .exportFunc();
    const {exports: {i64}} = builder.instantiate();

    runTest(name, i64, (left, right) => (check(left, right) ? trueValue : falseValue));
  }
}));

const tests = [
  ...comparisonTests,
  ...branchingTests,
];

const todoTests = tests.slice(argv.start, argv.end === undefined ? tests.length : argv.end);

testRunner.run(todoTests, {verbose: argv.verbose > 0});
