//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/* global assert,testRunner */ // eslint rule
WScript.LoadScriptFile("../UnitTestFrameWork/UnitTestFrameWork.js");
WScript.LoadScriptFile("wasmutils.js");
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-module-builder.js");
WScript.Flag("-wasmI64");

const comparisonOperators = [
  {name: "i64.eqz", op: kExprI64Eqz, arity: 1, check({high, low}) {return high === 0 && low === 0;}},
  {name: "i64.eq", op: kExprI64Eq, arity: 2, check(a, b) {return a.high === b.high && a.low === b.low;}},
  {name: "i64.ne", op: kExprI64Ne, arity: 2, check(a, b) {return a.high !== b.high || a.low !== b.low;}},
  {name: "i64.lt_s", op: kExprI64LtS, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return a.high < b.high;
    }
    return (a.low>>>0) < (b.low>>>0);
  }},
  {name: "i64.lt_u", op: kExprI64LtU, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) < (b.high>>>0);
    }
    return (a.low>>>0) < (b.low>>>0);
  }},
  {name: "i64.gt_s", op: kExprI64GtS, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return a.high > b.high;
    }
    return (a.low>>>0) > (b.low>>>0);
  }},
  {name: "i64.gt_u", op: kExprI64GtU, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) > (b.high>>>0);
    }
    return (a.low>>>0) > (b.low>>>0);
  }},
  {name: "i64.le_s", op: kExprI64LeS, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return a.high < b.high;
    }
    return (a.low>>>0) <= (b.low>>>0);
  }},
  {name: "i64.le_u", op: kExprI64LeU, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) < (b.high>>>0);
    }
    return (a.low>>>0) <= (b.low>>>0);
  }},
  {name: "i64.ge_s", op: kExprI64GeS, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return a.high > b.high;
    }
    return (a.low>>>0) >= (b.low>>>0);
  }},
  {name: "i64.ge_u", op: kExprI64GeU, arity: 2, check(a, b) {
    if (a.high !== b.high) {
      return (a.high>>>0) > (b.high>>>0);
    }
    return (a.low>>>0) >= (b.low>>>0);
  }},
];
function makeFunction(builder, name, op, type, arity) {
  const params = [];
  const body = [];
  for (let i = 0; i < arity; ++i) {
    params.push(type);
    body.push(kExprGetLocal, i);
  }
  body.push(op)
  builder
    .addFunction(name, makeSig(params, [kWasmI32]))
    .addBody(body)
    .exportFunc();
}
const I32values = [0, 1, 10, -1, -5, 5,
                124, -1026, 98768, -88754,
                1<<32, -(1<<32), (1<<32)-1, 1<<31, -(1<<31), 1<<25, -1<<25];

const comparisonTests = comparisonOperators.map(({name, op, arity, check}) => ({
  name: `test ${name}`,
  body() {
    const builder = new WasmModuleBuilder();
    makeFunction(builder, "i64", op, kWasmI64, arity);
    const {exports: {i64}} = builder.instantiate();
    const tested = {};

    for (let i = 0; i < I32values.length; ++i) {
      const secondLoopIt = arity == 1 ? i : I32values.length;
      for (let j = i; j < secondLoopIt; ++j) {
        const v0 = I32values[i];
        const v1 = I32values[j];

        const test = (a, b, c, d) => {
          const left = {high: a, low: b};
          const right = {high: c, low: d};
          const key = i64ToString(left) + i64ToString(right);
          if (tested[key]) {
            return;
          }
          tested[key] = true;
          const i64Res = i64(left, right) === 1;
          const checkRes = check(left, right);
          const msg = `${name}(${i64ToString(left)}, ${i64ToString(right)})`;
          if (argv.verbose > 1) {
            console.log(`${msg} = ${i64Res}`);
          }
          assert.areEqual(checkRes, i64Res, msg);
        };

        // Try all possible combinations
        for (let iShuffle = 0; iShuffle < (1 << 4); ++iShuffle) {
          const a = (iShuffle & 1) ? v1 : v0;
          const b = (iShuffle & 2) ? v1 : v0;
          const c = (iShuffle & 4) ? v1 : v0;
          const d = (iShuffle & 8) ? v1 : v0;
          test(a, b, c, d);
        }
      }
    }
  }
}))

const tests = [
  ...comparisonTests,
];

WScript.LoadScriptFile("../UnitTestFrameWork/yargs.js");
const argv = yargsParse(WScript.Arguments, {
  number: ["start", "end", "verbose"],
  default: {
    verbose: 1,
    start: 0,
    end: tests.length
  }
}).argv;

const todoTests = tests.slice(argv.start, argv.end);

testRunner.run(todoTests, {verbose: argv.verbose > 0});
