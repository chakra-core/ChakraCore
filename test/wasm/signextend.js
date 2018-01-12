//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/* global assert,testRunner */ // eslint rule
WScript.Flag("-WasmSignExtends");
WScript.Flag("-WasmI64");
WScript.LoadScriptFile("../UnitTestFramework/UnitTestFramework.js");

function makeCSETest(type, op1, op2, tests) {
  return {
    name: op2 ? `${type}.${op1}/${type}.${op2} no cse` : `${type}.${op1} cse`,
    body() {
      const buf = WebAssembly.wabt.convertWast2Wasm(`
(module
  (func (export "func") (param $x ${type}) (result ${type})
    (${type}.sub
      (${type}.${op1} (get_local $x))
      (${type}.${op2 || op1} (get_local $x))
    )
  )
)`);
      const mod = new WebAssembly.Module(buf);
      const {exports: {func}} = new WebAssembly.Instance(mod);
      const name = op2 ? `${op1}/${op2}` : op1;
      const assertion = op2 ? assert.areNotEqual : assert.areEqual;
      let j = 0;
      for (let i = 0; i < tests.length * 10; ++i) {
        const input = tests[j++ % tests.length];
        const result = func(input);
        if (type === "i64") {
          assertion(0, result.low|0, `${type}.${name}(0x${input.toString(16)}) = 0x${(result.low|0).toString(16)} low`);
          if (!op2) {
            assertion(0, result.high|0, `${type}.${name}(0x${input.toString(16)}) = 0x${(result.high|0).toString(16)} high`);
          }
        } else {
          assertion(0, result|0, `${type}.${name}(0x${input.toString(16)})`);
        }
      }
    }
  };
}

const tests = [
  makeCSETest("i32", "extend8_s" , null, [0xFF, 1, 0x1FF]),
  makeCSETest("i32", "extend16_s", null, [0xFF, 1, 0xFFFF, 0x1FFFF]),
  makeCSETest("i64", "extend8_s" , null, [0xFF, 1, 0x1FF]),
  makeCSETest("i64", "extend16_s", null, [0xFF, 1, 0xFFFF, 0x1FFFF]),
  makeCSETest("i64", "extend32_s", null, [0xFF, 1, 0xFFFF, 0xFFFFFFFF, {low: 0xFFFFFFFF, high: 1}]),

  makeCSETest("i32", "extend8_s" , "extend16_s", [0xFF, 0x1FF, 0xFF4F, 0x1FF4F]),
  makeCSETest("i32", "extend16_s", "extend8_s" , [0xFF, 0x1FF, 0xFF4F, 0x1FF4F]),

  makeCSETest("i64", "extend8_s" , "extend16_s", [0xFF, 0x1FF, 0xFF4F, 0x1FF4F]),
  makeCSETest("i64", "extend8_s" , "extend32_s", [0xFF, 0x1FF, 0xFF4F, 0x1FF4F]),
  makeCSETest("i64", "extend16_s", "extend8_s" , [0xFF, 0xFF4F, 0x1FF4F]),
  makeCSETest("i64", "extend16_s", "extend32_s", [0xFFFF, 0x14FFF]),
  makeCSETest("i64", "extend32_s", "extend8_s" , [0xFF, 0xFF4F, 0xFFF4FFFF, {low: 0x4FFFFFFF, high: 1}]),
  makeCSETest("i64", "extend32_s", "extend16_s", [0xFF4F, 0xFFF4FFFF, {low: 0x4FFFFFFF, high: 1}]),
];

WScript.LoadScriptFile("../UnitTestFramework/yargs.js");
const argv = yargsParse(WScript.Arguments, {
  boolean: ["verbose"],
  number: ["start", "end"],
  default: {
    verbose: true,
    start: 0,
    end: tests.length
  }
}).argv;

const todoTests = tests
  .slice(argv.start, argv.end);

testRunner.run(todoTests, {verbose: argv.verbose});
