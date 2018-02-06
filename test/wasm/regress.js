//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("../UnitTestFramework/UnitTestFramework.js");
WScript.LoadScriptFile("../WasmSpec/testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("../WasmSpec/testsuite/harness/wasm-module-builder.js");

const tests = [{
  name: "polymorphic operators regression",
  usesWabt: true,
  body() {
    const toValidate = {
      "stack-polymorphic-yield": `
    (module
      (func (result i32)
        (block (result i32)
          (loop (br 0))
          unreachable
          drop
        )
      )
    )`,
      "stack-polymorphic-br_table": `
    (module
      (func (result i32)
        (block (result i32)
          unreachable
          drop
          (br_table 0 0)
        )
      )
    )`,
      "stack-polymorphic-br": `
    (module
      (func (result i32)
        unreachable
        drop
        br 0
      )
    )`,
      "stack-polymorphic-return": `
    (module
      (func (result i32)
        unreachable
        drop
        return
      )
    )`,
    };
    for (const key in toValidate) {
      assert.isTrue(WebAssembly.validate(WebAssembly.wabt.convertWast2Wasm(toValidate[key])), `Module ${key} should be valid`);
    }
  }
}, {
  name: "tracing regression test",
  usesWabt: false,
  body() {
    const tracingPrefix = 0xf0;
    const builder = new WasmModuleBuilder();
    const typeIndex = builder.addType(kSig_v_v);
    builder.addFunction("foo", typeIndex).addBody([
      tracingPrefix, 0x0,
    ]).exportFunc();

    try {
      const {exports: {foo}} = new WebAssembly.Instance(new WebAssembly.Module(builder.toBuffer()));
      foo();
      assert.fail("Failed: Tracing prefix should not be accepted");
    } catch (e) {
      if (!e.message.includes("Tracing opcodes not allowed")) {
        assert.fail("Wrong error: " + e);
      }
    }
  }
}, {
  name: "Error message truncation test",
  usesWabt: false,
  body() {
    const tracingPrefix = 0xf0;
    const builder = new WasmModuleBuilder();
    const typeIndex = builder.addType(kSig_v_v);
    const exportedName = Array(5000).fill("").map((_, i) => i.toString()).join("");
    builder.addFunction(exportedName, typeIndex).addBody([
      // Use the same body as the test above to trigger an exception
      tracingPrefix, 0x0,
    ]).exportFunc();
    try {
      const {exports} = new WebAssembly.Instance(new WebAssembly.Module(builder.toBuffer()));
      exports[exportedName]();
      assert.fail("Failed: Compilation error expected");
    } catch (e) {
      // Make sure all platforms correctly truncate the error message to the expected length
      const marker = "function ";
      const index = e.message.indexOf(marker);
      assert.isTrue(index !== -1);
      assert.isTrue(e.message.length - index === 2047);
      const sub = e.message.substring(index + marker.length, e.message.length);
      const funcNameLengthUsed = 2047 - marker.length;
      const funcNameUsed = exportedName.substring(0, funcNameLengthUsed);
      assert.areEqual(funcNameUsed, sub, "Error message truncation unexpected result");
    }
  }
}];

WScript.LoadScriptFile("../UnitTestFramework/yargs.js");
const argv = yargsParse(WScript.Arguments, {
  boolean: ["verbose", "wabt"],
  number: ["start", "end"],
  default: {
    verbose: true,
    wabt: true,
    start: 0,
    end: tests.length
  }
}).argv;

let todoTests = tests
  .slice(argv.start, argv.end);
if (!argv.wabt) {
  todoTests = todoTests.filter(t => !t.usesWabt);
}

testRunner.run(todoTests, {verbose: argv.verbose});
