//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/* global assert,testRunner */ // eslint rule
WScript.LoadScriptFile("../UnitTestFramework/UnitTestFramework.js");
WScript.LoadScriptFile("../WasmSpec/testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("../WasmSpec/testsuite/harness/wasm-module-builder.js");
WScript.Flag("-off:wasmdeferred");

function makeReservedTest(name, body, msg) {
  return {
    name,
    body() {
      const builder = new WasmModuleBuilder();
      builder.addFunction(null, kSig_v_i).addBody(body);
      try {
        new WebAssembly.Module(builder.toBuffer());
        assert.fail("Expected an exception");
      } catch (e) {
        if (!(e instanceof WebAssembly.CompileError) || RegExp(msg, "i").test(e.message)) {
          return;
        }
        assert.fail(`Expected error message: ${msg}. Got ${e.message}`);
      }
    }
  }
}

const tests = [
  makeReservedTest("memory.size reserved", [kExprMemorySize, 1], "memory.size reserved value must be 0"),
  makeReservedTest("memory.grow reserved", [kExprMemoryGrow, 1], "memory.grow reserved value must be 0"),
  makeReservedTest("call_indirect reserved", [kExprCallIndirect, 1], "call_indirect reserved value must be 0"),
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
